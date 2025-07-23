#ifndef LIGHT_EFFECTS_H
#define LIGHT_EFFECTS_H

#include <stdbool.h>
#include <stdint.h>

#include "sdkconfig.h"

typedef enum {
    EFFECT_OFF = 0,          // 0 - Off
    EFFECT_STATIC,           // 1 - Static
    EFFECT_FADE,             // 2 - Fade  
    EFFECT_COLOUR_CYCLE,     // 3 - Colour cycle
    EFFECT_LIGHTNING,        // 4 - Lightning
    EFFECT_CANDLE,           // 5 - Candle
    EFFECT_STROBE,           // 6 - Strobe
    EFFECT_MAX               // 7 - boundary marker
} light_effect_t;

// Effect configuration structure
typedef struct {
    light_effect_t type;
    uint32_t brightness;  // 0 to max_duty brightness
    uint8_t speed;        // 0-255 effect speed
    uint32_t r, g, b, w;  // Base color values (0 to max_duty)
    bool enabled;         // Effect system enabled/disabled
    uint32_t max_duty;    // Maximum duty cycle for current driver
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

// Brightness management functions
void light_effects_set_max_brightness_percent(uint8_t max_percent);
uint8_t light_effects_get_max_brightness_percent(void);
uint32_t light_effects_scale_brightness_to_max(uint32_t brightness_driver_value);

#endif