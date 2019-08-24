#ifndef BLINKER_API_H
#define BLINKER_API_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "BlinkerDebug.h"

enum blinker_device_type_t
{
    BLINKER_WIFI,
    BLINKER_PRO_ESP
};

enum blinker_wifi_type_t
{
    BLINKER_DEFAULT_CONFIG,
    BLINKER_ESP_SMARTCONFIG
};

enum blinker_aligenie_type_t
{
    BLINKER_ALIGENIE_NONE,
    BLINKER_ALIGENIE_LIGHT,
    BLINKER_ALIGENIE_OUTLET,
    BLINKER_ALIGENIE_MULTI_OUTLET,
    BLINKER_ALIGENIE_SENSOR
};

enum blinker_dueros_type_t
{
    BLINKER_DUEROS_NONE,
    BLINKER_DUEROS_LIGHT,
    BLINKER_DUEROS_OUTLET,
    BLINKER_DUEROS_MULTI_OUTLET,
    BLINKER_DUEROS_SENSOR
};

enum blinker_miot_type_t
{
    BLINKER_MIOT_NONE,
    BLINKER_MIOT_LIGHT,
    BLINKER_MIOT_OUTLET,
    BLINKER_MIOT_MULTI_OUTLET,
    BLINKER_MIOT_SENSOR
};

typedef struct
{
    const char *name;
    uint8_t wNum;
} BlinkerButton;

typedef struct
{
    const char *name;
} BlinkerNumber;

typedef struct
{
    const char *name;
    uint8_t wNum;
} BlinkerRGB;

typedef struct
{
    const char *name;
    uint8_t wNum;
} BlinkerSlider;

typedef struct
{
    const char *name;
    uint8_t wNum;
} BlinkerTab;

typedef struct
{
    const char *name;
} BlinkerText;

typedef struct
{
    const char *state;
    const char *icon;
    const char *color;
    const char *content;
    const char *text1;
    const char *text2;
    const char *textColor;
} blinker_button_config_t;

typedef struct
{
    const char *icon;
    const char *color;
    const char *unit;
    const char *text;
    const char *value;
} blinker_number_config_t;

typedef struct
{
    uint8_t rgbw[4];
} blinker_rgb_config_t;

typedef struct
{
    const char *color;
    const char *value;
} blinker_slider_config_t;

typedef struct
{
    const char *state;
} blinker_switch_config_t;

typedef struct
{
    uint8_t tab[5];
} blinker_tab_config_t;

typedef struct
{
    const char *icon;
    const char *color;
    const char *text;
    const char *text1;
} blinker_text_config_t;

typedef struct 
{
    enum blinker_device_type_t      type;
    enum blinker_wifi_type_t        wifi;
    enum blinker_aligenie_type_t    aligenie;
    enum blinker_dueros_type_t      dueros;
    enum blinker_miot_type_t        miot;
} blinker_config_t;

typedef struct
{
    blinker_callback_with_begin_t begin;
} blinker_api_t;

extern blinker_api_t Blinker;

void blinker_attach_data(blinker_callback_with_json_arg_t func);

void blinker_button_print(const BlinkerButton *button, const blinker_button_config_t * config);

void blinker_number_print(const BlinkerNumber *number, const blinker_number_config_t *config);

void blinker_rgb_print(const BlinkerRGB *rgb, const blinker_rgb_config_t *config);

void blinker_slider_print(const BlinkerSlider *slider, const blinker_slider_config_t *config);

void blinker_switch_print(const blinker_switch_config_t *config);

void blinker_tab_print(const BlinkerTab *tab, const blinker_tab_config_t *config);

void blinker_text_print(const BlinkerText *text, const blinker_text_config_t *config);

void blinker_button_init(BlinkerButton *button, blinker_callback_with_string_arg_t _func);

void blinker_rgb_init(BlinkerRGB *rgb, blinker_callback_with_rgb_arg_t _func);

void blinker_slider_init(BlinkerSlider *slider, blinker_callback_with_int32_arg_t _func);

void blinker_switch_init(blinker_callback_with_string_arg_t _func);

void blinker_tab_init(BlinkerTab *tab, blinker_callback_with_tab_arg_t _func, blinker_callback_t _func1);

void print(const char *key, const char *value, int8_t isRaw);

void blinker_init(const blinker_config_t *_conf);

#endif
