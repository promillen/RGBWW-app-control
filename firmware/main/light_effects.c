#include "light_effects.h"
#include "pwm_control.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "sdkconfig.h"
#include <math.h>
#include <stdlib.h>

static const char *TAG = "LIGHT_EFFECTS";

// Effect configuration
static effect_config_t config = {
    .type = EFFECT_FADE,
    .brightness = 128,  // Start with 8-bit value, will be converted during init
    .speed = 50,
    .r = 255, .g = 0, .b = 0, .w = 0,  // Start with 8-bit values
    .enabled = true,
    .max_duty = 255  // Will be set correctly during init
};

// Brightness management
static uint8_t max_brightness_percent = 100;  // Default to 100% maximum brightness

// Effect state variables
static TaskHandle_t effects_task_handle = NULL;
static bool ble_connected = false;
static bool manual_mode = false;
static uint32_t effect_counter = 0;

// Fade effect variables
static float hue = 0.0f;

// Color cycle variables
static uint32_t cycle_timer = 0;
static uint8_t current_color[3] = {255, 0, 0}; // Current RGB color
static uint8_t target_color[3] = {0, 255, 0};  // Target RGB color
static uint32_t transition_duration = 0;
static uint32_t transition_start = 0;

// Lightning variables
static uint32_t lightning_timer = 0;
static bool lightning_active = false;
static uint8_t lightning_phase = 0;
static uint32_t lightning_delay = 0;

// Candle variables
static float candle_base_hue = 0.0f;
static float candle_hue_offset = 0.0f;
static bool candle_initialized = false;

// Strobe variables
static bool strobe_state = false;
static uint32_t strobe_timer = 0;

// Driver-specific timing constants based on Kconfig
#ifdef CONFIG_BOARD_ESP32C3_OLED
    // AL8860 optimized timings (slower, more stable)
    #define EFFECT_UPDATE_INTERVAL_MS 50
    #define FADE_SPEED_MULT 0.001f
#elif defined(CONFIG_BOARD_ESP32C3_NO_OLED)
    // LM3414 optimized timings (faster, more precise)
    #define EFFECT_UPDATE_INTERVAL_MS 20
    #define FADE_SPEED_MULT 0.002f
#else
    #error "No board configuration selected. Please run 'idf.py menuconfig'"
#endif

// Helper function to convert HSV to RGB
static void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
    int i = (int)(h * 6.0f);
    float f = (h * 6.0f) - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i % 6) {
        case 0: *r = v * 255; *g = t * 255; *b = p * 255; break;
        case 1: *r = q * 255; *g = v * 255; *b = p * 255; break;
        case 2: *r = p * 255; *g = v * 255; *b = t * 255; break;
        case 3: *r = p * 255; *g = q * 255; *b = v * 255; break;
        case 4: *r = t * 255; *g = p * 255; *b = v * 255; break;
        case 5: *r = v * 255; *g = p * 255; *b = q * 255; break;
    }
}

// Apply brightness scaling with proper resolution scaling and max brightness limit
static void apply_brightness(uint32_t *r, uint32_t *g, uint32_t *b, uint32_t *w, uint32_t brightness) {
    uint32_t max_duty = config.max_duty;
    
    // Apply brightness scaling (brightness is already in driver resolution)
    *r = (*r * brightness) / max_duty;
    *g = (*g * brightness) / max_duty;
    *b = (*b * brightness) / max_duty;
    *w = (*w * brightness) / max_duty;
    
    // Apply maximum brightness limit
    uint32_t max_allowed = (max_duty * max_brightness_percent) / 100;
    if (*r > max_allowed) *r = max_allowed;
    if (*g > max_allowed) *g = max_allowed;
    if (*b > max_allowed) *b = max_allowed;
    if (*w > max_allowed) *w = max_allowed;
}

// Scale color values to driver resolution
static uint32_t scale_to_driver_resolution(uint8_t color_8bit) {
    return (color_8bit * config.max_duty) / 255;
}

// Generate random color for effects
static void generate_random_color(uint8_t *r, uint8_t *g, uint8_t *b) {
    float random_hue = (float)rand() / RAND_MAX;
    hsv_to_rgb(random_hue, 1.0f, 1.0f, r, g, b);
}

// Linear interpolation between two colors
static uint8_t lerp_color(uint8_t from, uint8_t to, float progress) {
    return from + (uint8_t)((to - from) * progress);
}

// 1. OFF - Turn everything off
static void effect_off(void) {
    pwm_set_rgbw(0, 0, 0, 0);
}

// 2. STATIC - Static color (for manual control)
static void effect_static(void) {
    uint32_t r = config.r, g = config.g, b = config.b, w = config.w;
    apply_brightness(&r, &g, &b, &w, config.brightness);
    pwm_set_rgbw(r, g, b, w);
}

// 3. FADE - Unified fade effect with chip-specific optimization
static void effect_fade(void) {
    uint8_t r, g, b;
    uint32_t scaled_r, scaled_g, scaled_b, scaled_w = 0;
    
    // Calculate speed based on driver type and speed setting
    float speed_factor = (config.speed / 255.0f) * FADE_SPEED_MULT;
    
#ifdef CONFIG_BOARD_ESP32C3_OLED
    // AL8860: Slower, more stable fade
    hue += speed_factor;
#elif defined(CONFIG_BOARD_ESP32C3_NO_OLED)
    // LM3414: Faster, more precise fade using higher resolution
    hue += speed_factor * 2.0f; // Can handle faster transitions
#endif
    
    if (hue >= 1.0f) hue = 0.0f;
    
    hsv_to_rgb(hue, 1.0f, 1.0f, &r, &g, &b);
    
    // Scale to driver resolution
    scaled_r = scale_to_driver_resolution(r);
    scaled_g = scale_to_driver_resolution(g);
    scaled_b = scale_to_driver_resolution(b);
    
    apply_brightness(&scaled_r, &scaled_g, &scaled_b, &scaled_w, config.brightness);
    
    pwm_set_rgbw(scaled_r, scaled_g, scaled_b, scaled_w);
}

// 4. COLOUR CYCLE - Switch between random colors
static void effect_colour_cycle(void) {
    uint32_t scaled_r, scaled_g, scaled_b, scaled_w = 0;
    
    // Calculate transition duration based on speed (slower speed = longer transitions)
    if (transition_duration == 0) {
        transition_duration = (255 - config.speed) * 5 + 50; // 50-1325 ticks
        transition_start = effect_counter;
    }
    
    // Check if we need to start a new transition
    if (effect_counter - transition_start >= transition_duration) {
        // Move target to current and generate new target
        current_color[0] = target_color[0];
        current_color[1] = target_color[1];
        current_color[2] = target_color[2];
        
        // Generate new random target color
        generate_random_color(&target_color[0], &target_color[1], &target_color[2]);
        
        // Reset transition
        transition_start = effect_counter;
        transition_duration = (255 - config.speed) * 5 + 50;
    }
    
    // Calculate progress (0.0 to 1.0)
    float progress = (float)(effect_counter - transition_start) / transition_duration;
    if (progress > 1.0f) progress = 1.0f;
    
    // Smooth easing function for better visual appeal
    progress = progress * progress * (3.0f - 2.0f * progress);
    
    // Interpolate between current and target colors
    uint8_t r = lerp_color(current_color[0], target_color[0], progress);
    uint8_t g = lerp_color(current_color[1], target_color[1], progress);
    uint8_t b = lerp_color(current_color[2], target_color[2], progress);
    
    // Scale to driver resolution
    scaled_r = scale_to_driver_resolution(r);
    scaled_g = scale_to_driver_resolution(g);
    scaled_b = scale_to_driver_resolution(b);
    
    apply_brightness(&scaled_r, &scaled_g, &scaled_b, &scaled_w, config.brightness);
    
    pwm_set_rgbw(scaled_r, scaled_g, scaled_b, scaled_w);
}

// 5. LIGHTNING - Lightning storm effect
// Effect: Creates realistic lightning flashes with bright white strikes followed by dimmer afterglows.
// Multiple lightning bolts can occur in sequence with random timing between storms.
static void effect_lightning(void) {
    uint32_t r = 0, g = 0, b = 0, w = 0;
    
    // Speed affects frequency of lightning strikes
    uint32_t storm_frequency = (255 - config.speed) * 3 + 20; // 20-785 ticks between potential strikes
    
    lightning_timer++;
    
    if (!lightning_active) {
        // Check for new lightning strike
        if (lightning_timer >= storm_frequency && (rand() % 100) < 15) { // 15% chance per check
            lightning_active = true;
            lightning_phase = 0;
            lightning_timer = 0;
            lightning_delay = rand() % 3; // Random delay for realism
        }
    } else {
        // Lightning sequence
        switch (lightning_phase) {
            case 0: // Pre-flash delay
                if (lightning_timer >= lightning_delay) {
                    lightning_phase = 1;
                    lightning_timer = 0;
                }
                break;
                
            case 1: // Main flash (bright white)
                w = config.brightness;
                b = config.brightness / 3; // Cool white tint
                if (lightning_timer >= 2) {
                    lightning_phase = 2;
                    lightning_timer = 0;
                }
                break;
                
            case 2: // Quick dim
                w = config.brightness / 4;
                if (lightning_timer >= 1) {
                    lightning_phase = 3;
                    lightning_timer = 0;
                }
                break;
                
            case 3: // Second flash (if random)
                if (rand() % 3 == 0) { // 33% chance of double flash
                    w = config.brightness * 2 / 3;
                    b = config.brightness / 4;
                }
                if (lightning_timer >= 2) {
                    lightning_phase = 4;
                    lightning_timer = 0;
                }
                break;
                
            case 4: // Afterglow
                w = config.brightness / 8;
                if (lightning_timer >= 5) {
                    lightning_active = false;
                    lightning_timer = 0;
                }
                break;
        }
    }
    
    pwm_set_rgbw(r, g, b, w);
}

// 6. CANDLE - Flickering candle with random base color
// Effect: Chooses a random warm color when effect starts, then creates realistic candle flicker
// by varying the hue slightly and adding random intensity variations
static void effect_candle(void) {
    uint32_t r, g, b, w;
    
    // Initialize candle with random warm color when effect first starts
    if (!candle_initialized) {
        // Generate random hue in warm color range (red to yellow: 0.0 to 0.15)
        candle_base_hue = (float)(rand() % 40) / 255.0f; // 0.0 to ~0.15
        candle_initialized = true;
        ESP_LOGI(TAG, "🕯️ New candle color selected, base hue: %.3f", candle_base_hue);
    }
    
    // Create flickering by varying hue and intensity
    // Speed affects how fast the candle flickers
    float flicker_speed = (config.speed / 255.0f) * 0.1f + 0.02f;
    
    // Generate subtle hue variation around base color
    candle_hue_offset += ((float)rand() / RAND_MAX - 0.5f) * flicker_speed;
    candle_hue_offset = fmaxf(-0.05f, fminf(0.05f, candle_hue_offset)); // Limit range
    
    float current_hue = candle_base_hue + candle_hue_offset;
    if (current_hue < 0.0f) current_hue = 0.0f;
    if (current_hue > 1.0f) current_hue = 1.0f;
    
    // Generate candle flicker intensity (mostly bright with occasional dims)
    float base_intensity = 0.8f + 0.2f * ((float)rand() / RAND_MAX);
    if (rand() % 20 == 0) { // 5% chance of bigger flicker
        base_intensity *= 0.6f;
    }
    
    // Convert HSV to RGB for the flame color
    uint8_t flame_r, flame_g, flame_b;
    hsv_to_rgb(current_hue, 0.9f, base_intensity, &flame_r, &flame_g, &flame_b);
    
    // Scale to driver resolution and apply brightness
    r = (scale_to_driver_resolution(flame_r) * config.brightness) / config.max_duty;
    g = (scale_to_driver_resolution(flame_g) * config.brightness) / config.max_duty;
    b = (scale_to_driver_resolution(flame_b) * config.brightness) / config.max_duty;
    w = (uint32_t)(config.brightness * base_intensity * 0.3f); // Warm white component
    
    pwm_set_rgbw(r, g, b, w);
}

// 7. STROBE - Strobe light effect
// Effect: Rapidly flashes all channels on/off like a party strobe light
static void effect_strobe(void) {
    // Speed controls strobe frequency
    uint32_t strobe_interval = (255 - config.speed) / 8 + 1; // 1-32 ticks
    
    strobe_timer++;
    
    if (strobe_timer >= strobe_interval) {
        strobe_state = !strobe_state;
        strobe_timer = 0;
    }
    
    if (strobe_state) {
        // Full brightness on all channels for maximum strobe effect
        uint32_t intensity = config.brightness;
        pwm_set_rgbw(intensity, intensity, intensity, intensity);
    } else {
        // Off
        pwm_set_rgbw(0, 0, 0, 0);
    }
}

// Main effects task
static void effects_task(void *pvParameters) {
    ESP_LOGI(TAG, "Effects task started for %s", LED_DRIVER_TYPE);
    
    while (1) {
        if (!config.enabled) {
            // Effects disabled
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        // Always run effects unless manually overridden or off
        if (manual_mode && config.type != EFFECT_OFF) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        switch (config.type) {
            case EFFECT_OFF:
                effect_off();
                vTaskDelay(pdMS_TO_TICKS(1000)); // Sleep longer when off
                break;
                
            case EFFECT_STATIC:
                effect_static();
                vTaskDelay(pdMS_TO_TICKS(500)); // Update less frequently for static
                break;
                
            case EFFECT_FADE:
                effect_fade();
                break;
                
            case EFFECT_COLOUR_CYCLE:
                effect_colour_cycle();
                break;

            case EFFECT_LIGHTNING:
                effect_lightning();
                break;
                
            case EFFECT_CANDLE:
                effect_candle();
                break;
                
            case EFFECT_STROBE:
                effect_strobe();
                break;
                
            default:
                ESP_LOGW(TAG, "Unknown effect: %d", config.type);
                config.type = EFFECT_FADE;
                break;
        }
        
        effect_counter++;
        
        // Use driver-optimized update interval
        vTaskDelay(pdMS_TO_TICKS(EFFECT_UPDATE_INTERVAL_MS));
    }
}

// Public functions
void light_effects_init(void) {
    ESP_LOGI(TAG, "Initializing light effects system");
    
    // Set max duty based on driver
    config.max_duty = pwm_get_max_duty();
    
    // Initialize maximum brightness from KConfig if available
#ifdef CONFIG_LED_MAX_BRIGHTNESS_PERCENT
    max_brightness_percent = CONFIG_LED_MAX_BRIGHTNESS_PERCENT;
    ESP_LOGI(TAG, "Maximum brightness set from KConfig: %d%%", max_brightness_percent);
#else
    max_brightness_percent = 100; // Default if no KConfig setting
    ESP_LOGI(TAG, "Maximum brightness set to default: %d%%", max_brightness_percent);
#endif
    
    // Convert initial values from 8-bit to driver resolution
    config.brightness = (config.brightness * config.max_duty) / 255;
    config.r = (config.r * config.max_duty) / 255;
    config.g = (config.g * config.max_duty) / 255;
    config.b = (config.b * config.max_duty) / 255;
    config.w = (config.w * config.max_duty) / 255;
    
    ESP_LOGI(TAG, "Driver: %s, Resolution: %d-bit (max duty: %lu)", 
             LED_DRIVER_TYPE,
             (config.max_duty == 255) ? 8 : 12, 
             (unsigned long)config.max_duty);
    
    // Set default effect when no BLE connection
    if (!ble_connected) {
        config.type = EFFECT_FADE;
        config.enabled = true;
    }
}

void light_effects_start(void) {
    if (effects_task_handle == NULL) {
        xTaskCreate(effects_task, "effects_task", 4096, NULL, 5, &effects_task_handle);
        ESP_LOGI(TAG, "✨ Light effects started with %dms update interval", 
                 EFFECT_UPDATE_INTERVAL_MS);
    }
}

void light_effects_stop(void) {
    if (effects_task_handle != NULL) {
        vTaskDelete(effects_task_handle);
        effects_task_handle = NULL;
        pwm_set_rgbw(0, 0, 0, 0);
        ESP_LOGI(TAG, "Light effects stopped");
    }
}

void light_effects_set_effect(light_effect_t effect) {
    if (effect < EFFECT_MAX) {
        // Reset effect-specific state when changing effects
        if (config.type != effect) {
            effect_counter = 0;
            hue = 0.0f;
            cycle_timer = 0;
            transition_duration = 0;
            lightning_timer = 0;
            lightning_active = false;
            lightning_phase = 0;
            candle_initialized = false;
            strobe_timer = 0;
            strobe_state = false;
        }
        
        config.type = effect;
        ESP_LOGI(TAG, "Effect changed to: %d", effect);
    }
}

void light_effects_set_brightness(uint32_t brightness) {
    // Brightness is always passed in driver resolution
    if (brightness > config.max_duty) {
        brightness = config.max_duty;
    }
    config.brightness = brightness;
    
    // Calculate actual percentage for logging
    uint8_t brightness_percent = (brightness * 100) / config.max_duty;
    uint8_t actual_percent = (brightness_percent * max_brightness_percent) / 100;
    
    ESP_LOGI(TAG, "Brightness set to: %lu/%lu (%.1f%%) - max limit %d%% = actual %.1f%%", 
             (unsigned long)brightness, (unsigned long)config.max_duty,
             (float)brightness_percent, max_brightness_percent, (float)actual_percent);
}

void light_effects_set_speed(uint8_t speed) {
    config.speed = speed;
    ESP_LOGI(TAG, "Speed set to: %d", speed);
}

void light_effects_set_color(uint32_t r, uint32_t g, uint32_t b, uint32_t w) {
    // Colors are always passed in driver resolution
    config.r = (r > config.max_duty) ? config.max_duty : r;
    config.g = (g > config.max_duty) ? config.max_duty : g;
    config.b = (b > config.max_duty) ? config.max_duty : b;
    config.w = (w > config.max_duty) ? config.max_duty : w;
    ESP_LOGI(TAG, "Color set to: R=%lu, G=%lu, B=%lu, W=%lu (max=%lu)", 
             (unsigned long)config.r, (unsigned long)config.g, (unsigned long)config.b, (unsigned long)config.w, (unsigned long)config.max_duty);
}

void light_effects_enable_manual_mode(void) {
    manual_mode = true;
    ESP_LOGI(TAG, "Manual mode enabled - effects paused");
}

void light_effects_disable_manual_mode(void) {
    manual_mode = false;
    ESP_LOGI(TAG, "Manual mode disabled - effects resumed");
}

void light_effects_set_ble_connected(bool connected) {
    ble_connected = connected;
    
    if (connected) {
        ESP_LOGI(TAG, "🔗 BLE connected - ready for control");
        // Don't automatically enable manual mode, let the app control effects
    } else {
        ESP_LOGI(TAG, "🔌 BLE disconnected - starting fade effect");
        light_effects_disable_manual_mode();
        light_effects_set_effect(EFFECT_FADE);
    }
}

light_effect_t light_effects_get_current_effect(void) {
    return config.type;
}

effect_config_t* light_effects_get_config(void) {
    return &config;
}

// Brightness management functions
void light_effects_set_max_brightness_percent(uint8_t max_percent) {
    if (max_percent > 100) {
        max_percent = 100;
    }
    if (max_percent < 1) {
        max_percent = 1;
    }
    
    max_brightness_percent = max_percent;
    ESP_LOGI(TAG, "Maximum brightness limit set to: %d%%", max_percent);
}

uint8_t light_effects_get_max_brightness_percent(void) {
    return max_brightness_percent;
}

uint32_t light_effects_scale_brightness_to_max(uint32_t brightness_driver_value) {
    if (brightness_driver_value > config.max_duty) {
        brightness_driver_value = config.max_duty;
    }
    
    // Scale the brightness value to respect the maximum brightness limit
    uint32_t max_allowed = (config.max_duty * max_brightness_percent) / 100;
    uint32_t scaled_brightness = (brightness_driver_value * max_allowed) / config.max_duty;
    
    return scaled_brightness;
}