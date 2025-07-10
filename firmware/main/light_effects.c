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
    .type = EFFECT_SMOOTH_FADE,
    .brightness = 128,
    .speed = 50,
    .r = 255, .g = 0, .b = 0, .w = 0,
    .enabled = true,
    .max_duty = 255  // Will be set correctly during init
};

// Effect state variables
static TaskHandle_t effects_task_handle = NULL;
static bool ble_connected = false;
static bool manual_mode = false;
static uint32_t effect_counter = 0;
static float hue = 0.0f;
static uint8_t rgb_cycle_state = 0;  // For RGB cycle effect

// Driver-specific timing constants based on Kconfig
#ifdef CONFIG_BOARD_ESP32C3_OLED
    // AL8860 optimized timings (slower, more stable)
    #define EFFECT_UPDATE_INTERVAL_MS 50
    #define FAST_EFFECT_DIVISOR 8
    #define SMOOTH_FADE_SPEED_MULT 0.001f
#elif defined(CONFIG_BOARD_ESP32C3_NO_OLED)
    // LM3414 optimized timings (faster, more precise)
    #define EFFECT_UPDATE_INTERVAL_MS 20
    #define FAST_EFFECT_DIVISOR 4
    #define SMOOTH_FADE_SPEED_MULT 0.002f
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

// Apply brightness scaling with proper resolution scaling
static void apply_brightness(uint32_t *r, uint32_t *g, uint32_t *b, uint32_t *w, uint32_t brightness) {
    uint32_t max_duty = config.max_duty;
    
    // Scale from 8-bit input to driver resolution, then apply brightness
    *r = (*r * max_duty / 255) * brightness / max_duty;
    *g = (*g * max_duty / 255) * brightness / max_duty;
    *b = (*b * max_duty / 255) * brightness / max_duty;
    *w = (*w * max_duty / 255) * brightness / max_duty;
}

// Scale color values to driver resolution
static uint32_t scale_to_driver_resolution(uint8_t color_8bit) {
    return (color_8bit * config.max_duty) / 255;
}

// Smooth fade effect (default)
static void effect_smooth_fade(void) {
    uint8_t r, g, b;
    uint32_t scaled_r, scaled_g, scaled_b, scaled_w = 0;
    
    // Smooth hue transition with driver-optimized speed
    hue += (config.speed / 255.0f) * SMOOTH_FADE_SPEED_MULT;
    if (hue >= 1.0f) hue = 0.0f;
    
    hsv_to_rgb(hue, 1.0f, 1.0f, &r, &g, &b);
    
    // Scale to driver resolution
    scaled_r = scale_to_driver_resolution(r);
    scaled_g = scale_to_driver_resolution(g);
    scaled_b = scale_to_driver_resolution(b);
    
    apply_brightness(&scaled_r, &scaled_g, &scaled_b, &scaled_w, config.brightness);
    
    pwm_set_rgbw(scaled_r, scaled_g, scaled_b, scaled_w);
}

// RGB cycle effect (hard transitions between colors)
static void effect_rgb_cycle(void) {
    static uint32_t last_change = 0;
    uint32_t r = 0, g = 0, b = 0, w = 0;
    
    // Calculate delay based on speed
    uint32_t delay_ticks = (255 - config.speed) * 2 + 50;
    
    if (effect_counter - last_change >= delay_ticks) {
        rgb_cycle_state = (rgb_cycle_state + 1) % 7;
        last_change = effect_counter;
    }
    
    // Set colors based on cycle state, scale to driver resolution
    switch (rgb_cycle_state) {
        case 0: r = config.brightness; break;  // Red
        case 1: g = config.brightness; break;  // Green  
        case 2: b = config.brightness; break;  // Blue
        case 3: r = config.brightness; g = config.brightness; break;  // Yellow
        case 4: r = config.brightness; b = config.brightness; break;  // Magenta
        case 5: g = config.brightness; b = config.brightness; break;  // Cyan
        case 6: r = config.brightness; g = config.brightness; b = config.brightness; break;  // White
    }
    
    pwm_set_rgbw(r, g, b, w);
}

// Breathing effect
static void effect_breathing(void) {
    uint32_t r = config.r, g = config.g, b = config.b, w = config.w;
    
    // Sine wave breathing
    float breath = (sin(effect_counter * (config.speed / 255.0f) * 0.02f) + 1.0f) / 2.0f;
    uint32_t brightness = (uint32_t)(config.brightness * breath);
    
    apply_brightness(&r, &g, &b, &w, brightness);
    pwm_set_rgbw(r, g, b, w);
}

// Twinkle pulse effect
static void effect_twinkle_pulse(void) {
    static uint8_t last_r = 0, last_g = 0, last_b = 0;
    uint8_t r, g, b;
    uint32_t scaled_r, scaled_g, scaled_b, scaled_w = 0;
    
    // Random color changes
    if ((effect_counter % (256 - config.speed)) == 0) {
        hsv_to_rgb((float)rand() / RAND_MAX, 0.8f, 1.0f, &r, &g, &b);
        last_r = r; last_g = g; last_b = b;
    } else {
        r = last_r; g = last_g; b = last_b;
    }
    
    // Scale to driver resolution
    scaled_r = scale_to_driver_resolution(r);
    scaled_g = scale_to_driver_resolution(g);
    scaled_b = scale_to_driver_resolution(b);
    
    // Apply low brightness with occasional flickers
    uint32_t flicker_brightness = config.brightness / 4;
    if ((effect_counter % 50) == 0 && (rand() % 10) == 0) {
        flicker_brightness = config.brightness / 2;
    }
    
    apply_brightness(&scaled_r, &scaled_g, &scaled_b, &scaled_w, flicker_brightness);
    pwm_set_rgbw(scaled_r, scaled_g, scaled_b, scaled_w);
}

// Lightning flash effect
static void effect_lightning_flash(void) {
    static uint32_t flash_timer = 0;
    static bool in_flash = false;
    uint32_t r = 0, g = 0, b = 0, w = 0;
    
    flash_timer++;
    
    if (!in_flash) {
        // Random lightning strikes
        if ((flash_timer > (500 - config.speed * 2)) && (rand() % 100) == 0) {
            in_flash = true;
            flash_timer = 0;
        }
    } else {
        // Lightning flash sequence
        if (flash_timer < 3) {
            // Bright white flash
            w = config.brightness;
            r = config.brightness / 2;  // Cool white
        } else if (flash_timer < 6) {
            // Quick dim
            w = config.brightness / 4;
        } else if (flash_timer < 8) {
            // Second flash
            w = config.brightness * 3 / 4;
        } else {
            // Reset
            in_flash = false;
            flash_timer = 0;
        }
    }
    
    pwm_set_rgbw(r, g, b, w);
}

// Candle flicker effect
static void effect_candle_flicker(void) {
    static float flame_intensity = 1.0f;
    uint32_t r, g, b, w;
    
    // Random flame flicker
    flame_intensity += (((float)rand() / RAND_MAX) - 0.5f) * 0.1f;
    if (flame_intensity < 0.3f) flame_intensity = 0.3f;
    if (flame_intensity > 1.0f) flame_intensity = 1.0f;
    
    // Warm candle colors scaled to driver resolution
    float base_intensity = 0.7f + 0.3f * flame_intensity;
    r = (uint32_t)(config.brightness * base_intensity);
    g = (uint32_t)(config.brightness * base_intensity * 0.4f);
    b = 0;
    w = (uint32_t)(config.brightness * base_intensity * 0.8f);
    
    pwm_set_rgbw(r, g, b, w);
}

// Board-specific effects based on Kconfig
#ifdef CONFIG_BOARD_ESP32C3_OLED
// AL8860 specific effects

// Pulse wave effect - optimized for AL8860's hysteretic control
static void effect_pulse_wave(void) {
    static float wave_phase = 0.0f;
    uint32_t r, g, b, w;
    
    // Slower wave progression for AL8860's natural behavior
    wave_phase += (config.speed / 255.0f) * 0.05f;
    if (wave_phase >= 2.0f * M_PI) wave_phase = 0.0f;
    
    // Triangle wave pattern that works well with hysteretic control
    float intensity;
    if (wave_phase < M_PI) {
        intensity = wave_phase / M_PI;
    } else {
        intensity = 2.0f - (wave_phase / M_PI);
    }
    
    // Apply to warm white for smooth operation
    uint32_t brightness = (uint32_t)(config.brightness * intensity);
    r = config.r * brightness / config.max_duty;
    g = config.g * brightness / config.max_duty;
    b = config.b * brightness / config.max_duty;
    w = brightness;
    
    pwm_set_rgbw(r, g, b, w);
}

// Soft transition effect - leverages AL8860's soft-start capability
static void effect_soft_transition(void) {
    static uint8_t target_state = 0;
    static uint32_t transition_start = 0;
    static uint32_t current_r = 0, current_g = 0, current_b = 0, current_w = 0;
    
    // Define transition targets
    uint32_t targets[4][4] = {
        {config.brightness, 0, 0, 0},                    // Red
        {0, config.brightness, 0, 0},                    // Green
        {0, 0, config.brightness, 0},                    // Blue
        {0, 0, 0, config.brightness}                     // Warm White
    };
    
    // Check if it's time for a new transition
    uint32_t transition_duration = (255 - config.speed) * 10 + 100;
    if (effect_counter - transition_start >= transition_duration) {
        target_state = (target_state + 1) % 4;
        transition_start = effect_counter;
    }
    
    // Smooth interpolation to new target
    float progress = (float)(effect_counter - transition_start) / transition_duration;
    if (progress > 1.0f) progress = 1.0f;
    
    // Ease-in-out for smoother transitions with AL8860
    progress = progress * progress * (3.0f - 2.0f * progress);
    
    uint32_t target_r = targets[target_state][0];
    uint32_t target_g = targets[target_state][1];
    uint32_t target_b = targets[target_state][2];
    uint32_t target_w = targets[target_state][3];
    
    current_r = current_r + (uint32_t)((target_r - current_r) * progress);
    current_g = current_g + (uint32_t)((target_g - current_g) * progress);
    current_b = current_b + (uint32_t)((target_b - current_b) * progress);
    current_w = current_w + (uint32_t)((target_w - current_w) * progress);
    
    pwm_set_rgbw(current_r, current_g, current_b, current_w);
}

#elif defined(CONFIG_BOARD_ESP32C3_NO_OLED)
// LM3414 specific effects

// Precision fade effect - high-resolution fading for LM3414
static void effect_precision_fade(void) {
    static float precise_hue = 0.0f;
    static float hue_step = 0.0f;
    
    uint8_t r, g, b;
    uint32_t scaled_r, scaled_g, scaled_b, scaled_w = 0;
    
    // Ultra-smooth hue progression using LM3414's 12-bit resolution
    hue_step = (config.speed / 255.0f) * 0.0001f;  // Very fine steps
    precise_hue += hue_step;
    if (precise_hue >= 1.0f) precise_hue = 0.0f;
    
    hsv_to_rgb(precise_hue, 1.0f, 1.0f, &r, &g, &b);
    
    // Use full 12-bit resolution for maximum precision
    scaled_r = (r * config.max_duty) / 255;
    scaled_g = (g * config.max_duty) / 255;
    scaled_b = (b * config.max_duty) / 255;
    
    // Apply brightness with 12-bit precision
    scaled_r = (scaled_r * config.brightness) / config.max_duty;
    scaled_g = (scaled_g * config.brightness) / config.max_duty;
    scaled_b = (scaled_b * config.brightness) / config.max_duty;
    
    pwm_set_rgbw(scaled_r, scaled_g, scaled_b, scaled_w);
}

// Fast strobe effect - high-frequency effects for LM3414
static void effect_fast_strobe(void) {
    static bool strobe_state = false;
    static uint32_t last_strobe = 0;
    
    // Fast strobe timing taking advantage of LM3414's capabilities
    uint32_t strobe_interval = (255 - config.speed) / FAST_EFFECT_DIVISOR + 1;
    
    if (effect_counter - last_strobe >= strobe_interval) {
        strobe_state = !strobe_state;
        last_strobe = effect_counter;
    }
    
    uint32_t intensity = strobe_state ? config.brightness : 0;
    
    // Alternate between colors at high frequency
    uint8_t color_cycle = (effect_counter / (strobe_interval * 2)) % 3;
    uint32_t r = 0, g = 0, b = 0, w = 0;
    
    switch (color_cycle) {
        case 0: r = intensity; break;  // Red strobe
        case 1: g = intensity; break;  // Green strobe
        case 2: b = intensity; break;  // Blue strobe
    }
    
    pwm_set_rgbw(r, g, b, w);
}

#endif

// Main effects task
static void effects_task(void *pvParameters) {
    ESP_LOGI(TAG, "Effects task started for %s", LED_DRIVER_TYPE);
    
    while (1) {
        if (!config.enabled || manual_mode) {
            // Effects disabled or in manual mode
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        // If BLE is connected, don't run automatic effects
        if (ble_connected && config.type != EFFECT_STATIC) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        switch (config.type) {
            case EFFECT_OFF:
                pwm_set_rgbw(0, 0, 0, 0);
                break;
                
            case EFFECT_STATIC:
                // Static color - only update once
                {
                    uint32_t r = config.r, g = config.g, b = config.b, w = config.w;
                    apply_brightness(&r, &g, &b, &w, config.brightness);
                    pwm_set_rgbw(r, g, b, w);
                }
                break;
                
            case EFFECT_SMOOTH_FADE:
                effect_smooth_fade();
                break;
                
            case EFFECT_RGB_CYCLE:
                effect_rgb_cycle();
                break;
                
            case EFFECT_BREATHING:
                effect_breathing();
                break;
                
            case EFFECT_TWINKLE_PULSE:
                effect_twinkle_pulse();
                break;
                
            case EFFECT_LIGHTNING_FLASH:
                effect_lightning_flash();
                break;
                
            case EFFECT_CANDLE_FLICKER:
                effect_candle_flicker();
                break;

#ifdef CONFIG_BOARD_ESP32C3_OLED
            case EFFECT_PULSE_WAVE:
                effect_pulse_wave();
                break;
                
            case EFFECT_SOFT_TRANSITION:
                effect_soft_transition();
                break;
#elif defined(CONFIG_BOARD_ESP32C3_NO_OLED)
            case EFFECT_PRECISION_FADE:
                effect_precision_fade();
                break;
                
            case EFFECT_FAST_STROBE:
                effect_fast_strobe();
                break;
#endif
                
            default:
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
    
    // Scale existing color values to new resolution
    config.r = scale_to_driver_resolution(config.r);
    config.g = scale_to_driver_resolution(config.g);
    config.b = scale_to_driver_resolution(config.b);
    config.w = scale_to_driver_resolution(config.w);
    config.brightness = scale_to_driver_resolution(config.brightness);
    
    ESP_LOGI(TAG, "Driver: %s, Resolution: %d-bit (max duty: %lu)", 
             LED_DRIVER_TYPE,
             (config.max_duty == 255) ? 8 : 12, 
             (unsigned long)config.max_duty);
    
    // Set default effect when no BLE connection
    if (!ble_connected) {
        config.type = EFFECT_SMOOTH_FADE;
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
        config.type = effect;
        effect_counter = 0;  // Reset effect state
        hue = 0.0f;
        rgb_cycle_state = 0; // Reset RGB cycle
        ESP_LOGI(TAG, "Effect changed to: %d", effect);
    }
}

void light_effects_set_brightness(uint32_t brightness) {
    // Brightness is always passed in driver resolution
    if (brightness > config.max_duty) {
        brightness = config.max_duty;
    }
    config.brightness = brightness;
    ESP_LOGI(TAG, "Brightness set to: %lu/%lu", (unsigned long)brightness, (unsigned long)config.max_duty);
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
        ESP_LOGI(TAG, "🔗 BLE connected - switching to manual mode");
        light_effects_enable_manual_mode();
    } else {
        ESP_LOGI(TAG, "🔌 BLE disconnected - starting smooth fade effect");
        light_effects_disable_manual_mode();
        light_effects_set_effect(EFFECT_SMOOTH_FADE);
    }
}

light_effect_t light_effects_get_current_effect(void) {
    return config.type;
}

effect_config_t* light_effects_get_config(void) {
    return &config;
}