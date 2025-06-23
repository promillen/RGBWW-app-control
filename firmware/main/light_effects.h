#ifndef LIGHT_EFFECTS_H
#define LIGHT_EFFECTS_H

#include <stdint.h>
#include <stdbool.h>

// Effect types optimized for each driver
typedef enum {
    EFFECT_OFF = 0,
    EFFECT_STATIC,           // Static color (manual control)
    EFFECT_SMOOTH_FADE,      // Default - smooth RGB color cycling
    EFFECT_RGB_CYCLE,        // Hard RGB cycling without fading
    EFFECT_BREATHING,        // Breathing effect with any color
    EFFECT_TWINKLE_PULSE,    // Subtle RGB flicker on low brightness
    EFFECT_LIGHTNING_FLASH,  // Cool white lightning flashes
    EFFECT_CANDLE_FLICKER,   // Warm candle flame effect
#ifdef ESP32_C3_OLED
    // AL8860 specific effects (hysteretic control optimized)
    EFFECT_PULSE_WAVE,       // Optimized for AL8860's natural hysteretic behavior
    EFFECT_SOFT_TRANSITION,  // Leverages AL8860's soft-start capability
#else
    // LM3414 specific effects (high precision optimized)
    EFFECT_PRECISION_FADE,   // High-resolution fading for LM3414
    EFFECT_FAST_STROBE,      // High-frequency effects for LM3414
#endif
    EFFECT_MAX
} light_effect_t;

// Effect configuration structure
typedef struct {
    light_effect_t type;
    uint32_t brightness;     // 0 to max_duty brightness
    uint8_t speed;          // 0-255 effect speed
    uint32_t r, g, b, w;    // Base color values (0 to max_duty)
    bool enabled;           // Effect system enabled/disabled
    uint32_t max_duty;      // Maximum duty cycle for current driver
} effect_config_t;

// Function declarations
void light_effects_init(void);
void light_effects_start(void);
void light_effects_stop(void);
void light_effects_set_effect(light_effect_t effect);
void light_effects_set_brightness(uint32_t brightness);
void light_effects_set_speed(uint8_t speed);
void light_effects_set_color(uint32_t r, uint32_t g, uint32_t b, uint32_t w);
void light_effects_enable_manual_mode(void);
void light_effects_disable_manual_mode(void);
light_effect_t light_effects_get_current_effect(void);
effect_config_t* light_effects_get_config(void);
void light_effects_set_ble_connected(bool connected);

#endif