#pragma once

#include <esp_err.h>
#include <esp_log.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "cJSON.h"
#include "blinker_config.h"

typedef enum {
    BLINKER_BUTTON = 0,
    BLINKER_IMAGE,
    BLINKER_JOYSTICK,
    BLINKER_RGB,
    BLINKER_SLIDER,
    BLINKER_TAB,
    BLINKER_SWITCH,
} blinker_widget_type_t;

typedef struct {
    union {
        int i;
        char *s;
    };
    int array[4];
} blinker_widget_param_val_t;

typedef enum {
    BLINKER_PARAM_POWER_STATE = 0,
    BLINKER_PARAM_COLOR,
    BLINKER_PARAM_MODE,
    BLINKER_PARAM_COLOR_TEMP,
    BLINKER_PARAM_COLOR_TEMP_UP,
    BLINKER_PARAM_COLOR_TEMP_DOWN,
    BLINKER_PARAM_BRIGHTNESS,
    BLINKER_PARAM_BRIGHTNESS_UP,
    BLINKER_PARAM_BRIGHTNESS_DOWN,
    BLINKER_PARAM_TEMP,
    BLINKER_PARAM_TEMP_UP,
    BLINKER_PARAM_TEMP_DOWN,
    BLINKER_PARAM_HUMI,
    BLINKER_PARAM_LEVEL,
    BLINKER_PARAM_LEVEL_UP,
    BLINKER_PARAM_LEVEL_DOWN,
    BLINKER_PARAM_HSWING,
    BLINKER_PARAM_VSWING,
    BLINKER_PARAM_MODE_STATE,
    BLINKER_PARAM_GET_STATE,
} blinker_va_param_type_t;

typedef struct {
    union {
        int i;
        char *s;
    };

    union {
        int num;
        char *state;
    };
    blinker_va_param_type_t type;
} blinker_va_param_cb_t;

typedef void (*blinker_va_cb_t)(const blinker_va_param_cb_t *val);
typedef void (*blinker_widget_cb_t)(const blinker_widget_param_val_t *val);
typedef void (*blinker_json_cb_t)(cJSON *param);
typedef void (*blinker_data_cb_t)(const char *data);
typedef void (*blinker_void_cb_t)(void);

esp_err_t blinker_timeslot_data(cJSON *param, const char *key, const double value);

esp_err_t blinker_timeslot_data_init(const uint16_t interval, const uint8_t times, const blinker_json_cb_t cb);

esp_err_t blinker_data_handler(const blinker_data_cb_t cb);

esp_err_t blinker_heart_beat_handler(const blinker_void_cb_t cb);

esp_err_t blinker_builtin_switch_handler(const blinker_widget_cb_t cb);

esp_err_t blinker_builtin_switch_state(const char *state);

esp_err_t blinker_weather(char **payload, const int city);

esp_err_t blinker_weather_forecast(char **payload, const int city);

esp_err_t blinker_air(char **payload, const int city);

esp_err_t blinker_sms(const char *msg);

esp_err_t blinker_push(const char *msg);

esp_err_t blinker_wechat(const char *msg);

esp_err_t blinker_wechat_template(const char *title, const char *state, const char *msg);

esp_err_t blinker_log(const char *msg);

esp_err_t blinker_config_update(const char *msg);

esp_err_t blinker_config_get(char **payload);

esp_err_t blinker_va_multi_num(cJSON *param, const int num);
 
esp_err_t blinker_aligenie_power_state(cJSON *param, const char *state);

esp_err_t blinker_aligenie_color(cJSON *param, const char *color);

esp_err_t blinker_aligenie_mode(cJSON *param, const char *mode);

esp_err_t blinker_aligenie_color_temp(cJSON *param, const int color_temp);

esp_err_t blinker_aligenie_brightness(cJSON *param, const int bright);

esp_err_t blinker_aligenie_temp(cJSON *param, const int temp);

esp_err_t blinker_aligenie_humi(cJSON *param, const int humi);

esp_err_t blinker_aligenie_pm25(cJSON *param, const int pm25);

esp_err_t blinker_aligenie_level(cJSON *param, const int level);

esp_err_t blinker_aligenie_hswing(cJSON *param, const char *state);

esp_err_t blinker_aligenie_vswing(cJSON *param, const char *state);

esp_err_t blinker_aligenie_print(cJSON* param);

esp_err_t blinker_aligenie_handler_register(blinker_va_cb_t cb);

esp_err_t blinker_dueros_power_state(cJSON *param, const char *state);

esp_err_t blinker_dueros_color(cJSON *param, const char *color);

esp_err_t blinker_dueros_brightness(cJSON *param, const int bright);

esp_err_t blinker_dueros_mode(cJSON *param, const char *mode);

esp_err_t blinker_dueros_color_temp(cJSON *param, const int color_temp);

esp_err_t blinker_dueros_temp(cJSON *param, const int temp);

esp_err_t blinker_dueros_humi(cJSON *param, const int humi);

esp_err_t blinker_dueros_pm25(cJSON *param, const int pm25);

esp_err_t blinker_dueros_pm10(cJSON *param, const int pm10);

esp_err_t blinker_dueros_co2(cJSON *param, const int co2);

esp_err_t blinker_dueros_aqi(cJSON *param, const int aqi);

esp_err_t blinker_dueros_level(cJSON *param, const int level);

esp_err_t blinker_dueros_time(cJSON *param, const int time);

esp_err_t blinker_dueros_print(cJSON* param);

esp_err_t blinker_dueros_handler_register(const blinker_va_cb_t cb);

esp_err_t blinker_miot_power_state(cJSON *param, const char *state);

esp_err_t blinker_miot_color(cJSON *param, int color);

esp_err_t blinker_miot_mode(cJSON *param, int mode);

esp_err_t blinker_miot_mode_state(cJSON *param, const char *mode, const char *state);

esp_err_t blinker_miot_color_temp(cJSON *param, const int color_temp);

esp_err_t blinker_miot_brightness(cJSON *param, const int bright);

esp_err_t blinker_miot_temp(cJSON *param, const int temp);

esp_err_t blinker_miot_humi(cJSON *param, const int humi);

esp_err_t blinker_miot_pm25(cJSON *param, const int pm25);

esp_err_t blinker_miot_co2(cJSON *param, const int co2);

esp_err_t blinker_miot_level(cJSON *param, const int level);

esp_err_t blinker_miot_hswing(cJSON *param, const char *state);

esp_err_t blinker_miot_vswing(cJSON *param, const char *state);

esp_err_t blinker_miot_print(cJSON* param);

esp_err_t blinker_miot_handler_register(const blinker_va_cb_t cb);

esp_err_t blinker_widget_switch(cJSON *param, const char *state);

esp_err_t blinker_widget_icon(cJSON *param, const char *icon);

esp_err_t blinker_widget_color(cJSON *param, const char *color);

esp_err_t blinker_widget_text(cJSON *param, const char *text);

esp_err_t blinker_widget_text1(cJSON *param, const char *text);

esp_err_t blinker_widget_text_color(cJSON *param, const char *color);

esp_err_t blinker_widget_value_number(cJSON *param, const double value);

esp_err_t blinker_widget_value_string(cJSON *param, const char *value);

esp_err_t blinker_widget_unit(cJSON *param, const char *uint);

esp_err_t blinker_widget_image(cJSON *param, const int num);

esp_err_t blinker_widget_rgb_print(const char *key, const int r, const int g, const int b, const int w);

esp_err_t blinker_widget_tab_print(const char *key, const int tab0, const int tab1, const int tab2, const int tab3, const int tab4);

esp_err_t blinker_widget_print(const char *key, cJSON *param);

esp_err_t blinker_widget_add(const char *name, const blinker_widget_type_t type, const blinker_widget_cb_t cb);

esp_err_t blinker_init(void);

#ifdef __cplusplus
}
#endif
