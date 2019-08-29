#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"

#include "BlinkerApi.h"
#include "BlinkerMQTT.h"



// #include <sys/socket.h>
// #include <netdb.h>
// #include "lwip/apps/sntp.h"

// #include "wolfssl/ssl.h"

// #include "esp_wifi.h"
// #include "esp_event_loop.h"
// #include "esp_log.h"

// #include "nvs_flash.h"

// #define WEB_SERVER "www.howsmyssl.com"
// #define WEB_PORT 443
// #define WEB_URL "https://www.howsmyssl.com/a/check"

// #define REQUEST "GET " WEB_URL " HTTP/1.0\r\n" 
//     "Host: "WEB_SERVER"\r\n" 
//     "User-Agent: esp-idf/1.0 espressif\r\n" 
//     "\r\n"

// #define WOLFSSL_DEMO_THREAD_NAME        "wolfssl_client"
// #define WOLFSSL_DEMO_THREAD_STACK_WORDS 8192
// #define WOLFSSL_DEMO_THREAD_PRORIOTY    6

// #define WOLFSSL_DEMO_SNTP_SERVERS       "pool.ntp.org"

// const char send_data[] = REQUEST;
// const int32_t send_bytes = sizeof(send_data);
// char recv_data[1024] = {0};


// char* https_request_data_api;
// int32_t https_request_bytes_api = 0;


static const char *TAG = "BlinkerApi";

blinker_api_t Blinker;

int8_t _parsed = 0;
int8_t autoFormat = 0;
char *send_buf = NULL;
uint32_t autoFormatFreshTime = 0;

void run(void);
void blinker_run(void* pv);
void parse(const char *data);
void aligenie_parse(const char *data);
void dueros_parse(const char *data);
void miot_parse(const char *data);
void blinker_server(uint8_t _type);
void blinker_fresh_sharers(void);

typedef struct
{
    const char *name;
    blinker_callback_with_string_arg_t wfunc;
} blinker_widgets_str_t;

typedef struct
{
    const char *name;
    blinker_callback_with_rgb_arg_t wfunc;
} blinker_widgets_rgb_t;

typedef struct
{
    const char *name;
    blinker_callback_with_int32_arg_t wfunc;
} blinker_widgets_int_t;

typedef struct
{
    const char *name;
    blinker_callback_with_tab_arg_t wfunc;
    blinker_callback_t wfunc1;
} blinker_widgets_tab_t;

blinker_widgets_str_t   _Widgets_str[BLINKER_MAX_WIDGET_SIZE];
blinker_widgets_rgb_t   _Widgets_rgb[BLINKER_MAX_WIDGET_SIZE/2];
blinker_widgets_int_t   _Widgets_int[BLINKER_MAX_WIDGET_SIZE];
blinker_widgets_tab_t   _Widgets_tab[BLINKER_MAX_WIDGET_SIZE];
uint8_t     _wCount_str = 0;
uint8_t     _wCount_rgb = 0;
uint8_t     _wCount_int = 0;
uint8_t     _wCount_tab = 0;

blinker_callback_with_json_arg_t data_availablie_func = NULL;

blinker_callback_with_string_uint8_arg_t    _AliGeniePowerStateFunc_m = NULL;
blinker_callback_with_string_arg_t          _AliGeniePowerStateFunc = NULL;
blinker_callback_with_string_arg_t          _AliGenieSetColorFunc = NULL;
blinker_callback_with_string_arg_t          _AliGenieSetModeFunc = NULL;
blinker_callback_with_string_arg_t          _AliGenieSetcModeFunc = NULL;
blinker_callback_with_int32_arg_t           _AliGenieSetBrightnessFunc = NULL;
blinker_callback_with_int32_arg_t           _AliGenieSetRelativeBrightnessFunc = NULL;
blinker_callback_with_int32_arg_t           _AliGenieSetColorTemperature = NULL;
blinker_callback_with_int32_arg_t           _AliGenieSetRelativeColorTemperature = NULL;
blinker_callback_with_int32_uint8_arg_t     _AliGenieQueryFunc_m = NULL;
blinker_callback_with_int32_arg_t           _AliGenieQueryFunc = NULL;

blinker_callback_with_string_uint8_arg_t    _DuerOSPowerStateFunc_m = NULL;
blinker_callback_with_string_arg_t          _DuerOSPowerStateFunc = NULL;
blinker_callback_with_int32_arg_t           _DuerOSSetColorFunc = NULL;
blinker_callback_with_string_arg_t          _DuerOSSetModeFunc = NULL;
blinker_callback_with_string_arg_t          _DuerOSSetcModeFunc = NULL;
blinker_callback_with_int32_arg_t           _DuerOSSetBrightnessFunc = NULL;
blinker_callback_with_int32_arg_t           _DuerOSSetRelativeBrightnessFunc = NULL;
// blinker_callback_with_int32_arg_t        _DuerOSSetColorTemperature = NULL;
// blinker_callback_with_int32_arg_t        _DuerOSSetRelativeColorTemperature = NULL;
blinker_callback_with_int32_uint8_arg_t     _DuerOSQueryFunc_m = NULL;
blinker_callback_with_int32_arg_t           _DuerOSQueryFunc = NULL;

blinker_callback_with_string_uint8_arg_t    _MIOTPowerStateFunc_m = NULL;
blinker_callback_with_string_arg_t          _MIOTPowerStateFunc = NULL;
blinker_callback_with_int32_arg_t           _MIOTSetColorFunc = NULL;
blinker_callback_with_uint8_arg_t           _MIOTSetModeFunc = NULL;
// blinker_callback_with_string_arg_t       _MIOTSetcModeFunc = NULL;
blinker_callback_with_int32_arg_t           _MIOTSetBrightnessFunc = NULL;
// blinker_callback_with_int32_arg_t        _MIOTSetRelativeBrightnessFunc = NULL;
blinker_callback_with_int32_arg_t           _MIOTSetColorTemperature = NULL;
// blinker_callback_with_int32_arg_t        _MIOTSetRelativeColorTemperature = NULL;
blinker_callback_with_int32_uint8_arg_t     _MIOTQueryFunc_m = NULL;
blinker_callback_with_int32_arg_t           _MIOTQueryFunc = NULL;

void blinker_aligenie_power_state_init(blinker_callback_with_string_arg_t func)
{
    _AliGeniePowerStateFunc = func;
}

void blinker_aligenie_multi_power_state_init(blinker_callback_with_string_uint8_arg_t func)
{
    _AliGeniePowerStateFunc_m = func;
}

void blinker_aligenie_color_init(blinker_callback_with_string_arg_t func)
{
    _AliGenieSetColorFunc = func;
}

void blinker_aligenie_mode_init(blinker_callback_with_string_arg_t func)
{
    _AliGenieSetModeFunc = func;
}

void blinker_aligenie_cancle_mode_init(blinker_callback_with_string_arg_t func)
{
    _AliGenieSetcModeFunc = func;
}

void blinker_aligeinie_brightness_init(blinker_callback_with_int32_arg_t func)
{
    _AliGenieSetBrightnessFunc = func;
}

void blinker_aligenie_relative_brightness_init(blinker_callback_with_int32_arg_t func)
{
    _AliGenieSetRelativeBrightnessFunc = func;
}

void blinker_aligenie_color_temperature_init(blinker_callback_with_int32_arg_t func)
{
    _AliGenieSetColorTemperature = func;
}

void blinker_aligenie_relative_color_temperature_init(blinker_callback_with_int32_arg_t func)
{
    _AliGenieSetRelativeColorTemperature = func;
}

void blinker_aligenie_query_init(blinker_callback_with_int32_arg_t func)
{
    _AliGenieQueryFunc = func;
}

void blinker_aligenie_multi_query_init(blinker_callback_with_int32_uint8_arg_t func)
{
    _AliGenieQueryFunc_m = func;
}

void blinker_aligenie_print(const blinker_aligenie_config_t *config)
{
    cJSON *pValue = cJSON_CreateObject();

    if (config->power_state) cJSON_AddStringToObject(pValue, BLINKER_CMD_POWERSTATE, config->power_state);
    if (config->num) cJSON_AddStringToObject(pValue, "num", config->num);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR, config->color);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR_, config->color);
    if (config->mode) cJSON_AddStringToObject(pValue, BLINKER_CMD_MODE, config->mode);
    if (config->color_temp) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLORTEMP, config->color_temp);
    if (config->brightness) cJSON_AddStringToObject(pValue, BLINKER_CMD_BRIGHTNESS, config->brightness);
    if (config->temp) cJSON_AddStringToObject(pValue, BLINKER_CMD_TEMP, config->temp);
    if (config->humi) cJSON_AddStringToObject(pValue, BLINKER_CMD_HUMI, config->humi);
    if (config->pm25) cJSON_AddStringToObject(pValue, BLINKER_CMD_PM25, config->pm25);

    char _data[BLINKER_MAX_SEND_SIZE] = {0};
    cJSON_PrintPreallocated(pValue, _data, BLINKER_MAX_SEND_SIZE, 0);
    blinker_aligenie_mqtt_print(_data);

    cJSON_Delete(pValue);
}



void blinker_dueros_power_state_init(blinker_callback_with_string_arg_t func)
{
    _DuerOSPowerStateFunc = func;
}

void blinker_dueros_multi_power_state_init(blinker_callback_with_string_uint8_arg_t func)
{
    _DuerOSPowerStateFunc_m = func;
}

void blinker_dueros_color_init(blinker_callback_with_int32_arg_t func)
{
    _DuerOSSetColorFunc = func;
}

void blinker_dueros_mode_init(blinker_callback_with_string_arg_t func)
{
    _DuerOSSetModeFunc = func;
}

void blinker_dueros_cancle_mode_init(blinker_callback_with_string_arg_t func)
{
    _DuerOSSetcModeFunc = func;
}

void blinker_dueros_brightness_init(blinker_callback_with_int32_arg_t func)
{
    _DuerOSSetBrightnessFunc = func;
}

void blinker_dueros_query_init(blinker_callback_with_int32_arg_t func)
{
    _DuerOSQueryFunc = func;
}

void blinker_dueros_multi_query_init(blinker_callback_with_int32_uint8_arg_t func)
{
    _DuerOSQueryFunc_m = func;
}

void blinker_dueros_print(const blinker_dueros_config_t *config)
{
    cJSON *pValue = cJSON_CreateObject();

    if (config->power_state) cJSON_AddStringToObject(pValue, BLINKER_CMD_POWERSTATE, config->power_state);
    if (config->num) cJSON_AddStringToObject(pValue, "num", config->num);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR, config->color);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR_, config->color);
    if (config->mode) cJSON_AddStringToObject(pValue, BLINKER_CMD_MODE, config->mode);
    if (config->brightness) cJSON_AddStringToObject(pValue, BLINKER_CMD_BRIGHTNESS, config->brightness);
    if (config->temp) cJSON_AddStringToObject(pValue, BLINKER_CMD_TEMP, config->temp);
    if (config->humi) cJSON_AddStringToObject(pValue, BLINKER_CMD_HUMI, config->humi);
    if (config->pm25) cJSON_AddStringToObject(pValue, BLINKER_CMD_PM25, config->pm25);
    if (config->pm10) cJSON_AddStringToObject(pValue, BLINKER_CMD_PM25, config->pm10);
    if (config->aqi) cJSON_AddStringToObject(pValue, BLINKER_CMD_PM25, config->aqi);
    if (config->co2) cJSON_AddStringToObject(pValue, BLINKER_CMD_CO2, config->co2);
    if (config->time) cJSON_AddStringToObject(pValue, BLINKER_CMD_CO2, config->time);

    char _data[BLINKER_MAX_SEND_SIZE] = {0};
    cJSON_PrintPreallocated(pValue, _data, BLINKER_MAX_SEND_SIZE, 0);
    blinker_dueros_mqtt_print(_data);

    cJSON_Delete(pValue);
}



void blinker_miot_power_state_init(blinker_callback_with_string_arg_t func)
{
    _MIOTPowerStateFunc = func;
}

void blinker_miot_multi_power_state_init(blinker_callback_with_string_uint8_arg_t func)
{
    _MIOTPowerStateFunc_m = func;
}

void blinker_miot_color_init(blinker_callback_with_int32_arg_t func)
{
    _MIOTSetColorFunc = func;
}

void blinker_miot_mode_init(blinker_callback_with_uint8_arg_t func)
{
    _MIOTSetModeFunc = func;
}

void blinker_miot_brightness_init(blinker_callback_with_int32_arg_t func)
{
    _MIOTSetBrightnessFunc = func;
}

void blinker_miot_color_temperature_init(blinker_callback_with_int32_arg_t func)
{
    _MIOTSetColorTemperature = func;
}

void blinker_miot_query_init(blinker_callback_with_int32_arg_t func)
{
    _MIOTQueryFunc = func;
}

void blinker_miot_multi_query_init(blinker_callback_with_int32_uint8_arg_t func)
{
    _MIOTQueryFunc_m = func;
}

void blinker_miot_print(const blinker_miot_config_t *config)
{
    cJSON *pValue = cJSON_CreateObject();

    if (config->power_state) cJSON_AddStringToObject(pValue, BLINKER_CMD_POWERSTATE, config->power_state);
    if (config->num) cJSON_AddStringToObject(pValue, "num", config->num);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR, config->color);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR_, config->color);
    if (config->mode) cJSON_AddStringToObject(pValue, BLINKER_CMD_MODE, config->mode);
    if (config->color_temp) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLORTEMP, config->color_temp);
    if (config->brightness) cJSON_AddStringToObject(pValue, BLINKER_CMD_BRIGHTNESS, config->brightness);
    if (config->temp) cJSON_AddStringToObject(pValue, BLINKER_CMD_TEMP, config->temp);
    if (config->humi) cJSON_AddStringToObject(pValue, BLINKER_CMD_HUMI, config->humi);
    if (config->pm25) cJSON_AddStringToObject(pValue, BLINKER_CMD_PM25, config->pm25);
    if (config->co2) cJSON_AddStringToObject(pValue, BLINKER_CMD_CO2, config->co2);

    char _data[BLINKER_MAX_SEND_SIZE] = {0};
    cJSON_PrintPreallocated(pValue, _data, BLINKER_MAX_SEND_SIZE, 0);
    blinker_miot_mqtt_print(_data);

    cJSON_Delete(pValue);
}

void blinker_attach_data(blinker_callback_with_json_arg_t func)
{
    data_availablie_func = func;
}

int8_t check_string_num(const char *name, const blinker_widgets_str_t *c, uint8_t count)
{
    for (uint8_t cNum = 0; cNum < count; cNum++)
    {
        if (strcmp(c[cNum].name, name) == 0)
        {
            // BLINKER_LOG_ALL(TAG, "check_string_num, name: %s, num: %d", name, cNum);
            return cNum;
        }
    }

    return BLINKER_OBJECT_NOT_AVAIL;
}

int8_t check_rgb_num(const char *name, const blinker_widgets_rgb_t *c, uint8_t count)
{
    for (uint8_t cNum = 0; cNum < count; cNum++)
    {
        if (strcmp(c[cNum].name, name) == 0)
        {
            // BLINKER_LOG_ALL(TAG, "check_rgb_num, name: %s, num: %d", name, cNum);
            return cNum;
        }
    }

    return BLINKER_OBJECT_NOT_AVAIL;
}

int8_t check_int_num(const char *name, const blinker_widgets_int_t *c, uint8_t count)
{
    for (uint8_t cNum = 0; cNum < count; cNum++)
    {
        if (strcmp(c[cNum].name, name) == 0)
        {
            // BLINKER_LOG_ALL(TAG, "check_rgb_num, name: %s, num: %d", name, cNum);
            return cNum;
        }
    }

    return BLINKER_OBJECT_NOT_AVAIL;
}

int8_t check_tab_num(const char *name, const blinker_widgets_tab_t *c, uint8_t count)
{
    for (uint8_t cNum = 0; cNum < count; cNum++)
    {
        if (strcmp(c[cNum].name, name) == 0)
        {
            // BLINKER_LOG_ALL(TAG, "check_rgb_num, name: %s, num: %d", name, cNum);
            return cNum;
        }
    }

    return BLINKER_OBJECT_NOT_AVAIL;
}

uint8_t attach_widget_string(const char *_name, blinker_callback_with_string_arg_t _func)
{
    int8_t num = check_string_num(_name, _Widgets_str, _wCount_str);

    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_str < BLINKER_MAX_WIDGET_SIZE*2)
        {
            _Widgets_str[_wCount_str].name = _name;
            _Widgets_str[_wCount_str].wfunc = _func;
            _wCount_str++;

            BLINKER_LOG_ALL(TAG, "new widgets: %s, _wCount_str: %d", _name, _wCount_str);
            return _wCount_str;
        }
        else
        {
            return 0;
        }
    }
    else if(num >= 0 )
    {
        BLINKER_ERR_LOG(TAG, "widgets name > %s < has been registered, please register another name!", _name);
        return 0;
    }
    else
    {
        return 0;
    }
}

uint8_t attach_widget_rgb(const char *_name, blinker_callback_with_rgb_arg_t _func)
{
    int8_t num = check_rgb_num(_name, _Widgets_rgb, _wCount_rgb);

    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_rgb < BLINKER_MAX_WIDGET_SIZE*2)
        {
            _Widgets_rgb[_wCount_rgb].name = _name;
            _Widgets_rgb[_wCount_rgb].wfunc = _func;
            _wCount_rgb++;

            BLINKER_LOG_ALL(TAG, "new widgets: %s, _wCount_rgb: %d", _name, _wCount_rgb);
            return _wCount_rgb;
        }
        else
        {
            return 0;
        }
    }
    else if(num >= 0 )
    {
        BLINKER_ERR_LOG(TAG, "widgets name > %s < has been registered, please register another name!", _name);
        return 0;
    }
    else
    {
        return 0;
    }
}

uint8_t attach_widget_int(const char *_name, blinker_callback_with_int32_arg_t _func)
{
    int8_t num = check_int_num(_name, _Widgets_int, _wCount_int);

    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_int < BLINKER_MAX_WIDGET_SIZE*2)
        {
            _Widgets_int[_wCount_int].name = _name;
            _Widgets_int[_wCount_int].wfunc = _func;
            _wCount_int++;

            BLINKER_LOG_ALL(TAG, "new widgets: %s, _wCount_int: %d", _name, _wCount_int);
            return _wCount_int;
        }
        else
        {
            return 0;
        }
    }
    else if(num >= 0 )
    {
        BLINKER_ERR_LOG(TAG, "widgets name > %s < has been registered, please register another name!", _name);
        return 0;
    }
    else
    {
        return 0;
    }
}

uint8_t attach_widget_tab(const char *_name, blinker_callback_with_tab_arg_t _func, blinker_callback_t _func1)
{
    int8_t num = check_tab_num(_name, _Widgets_tab, _wCount_tab);

    if (num == BLINKER_OBJECT_NOT_AVAIL)
    {
        if (_wCount_tab < BLINKER_MAX_WIDGET_SIZE*2)
        {
            _Widgets_tab[_wCount_tab].name = _name;
            _Widgets_tab[_wCount_tab].wfunc = _func;
            _Widgets_tab[_wCount_tab].wfunc1 = _func1;
            _wCount_tab++;

            BLINKER_LOG_ALL(TAG, "new widgets: %s, _wCount_tab: %d", _name, _wCount_tab);
            return _wCount_tab;
        }
        else
        {
            return 0;
        }
    }
    else if(num >= 0 )
    {
        BLINKER_ERR_LOG(TAG, "widgets name > %s < has been registered, please register another name!", _name);
        return 0;
    }
    else
    {
        return 0;
    }
}

void blinker_button_print(const BlinkerButton *button, const blinker_button_config_t * config)
{
    cJSON *pValue = cJSON_CreateObject();
    // cJSON_AddStringToObject(pValue,"mac","xuhongv");

    if (config->state) cJSON_AddStringToObject(pValue, BLINKER_CMD_STATE, config->state);
    if (config->icon) cJSON_AddStringToObject(pValue, BLINKER_CMD_ICON, config->icon);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR, config->color);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR_, config->color);
    if (config->content) cJSON_AddStringToObject(pValue, BLINKER_CMD_CONTENT, config->content);
    if (config->text1) cJSON_AddStringToObject(pValue, BLINKER_CMD_TEXT, config->text1);
    if (config->text2) cJSON_AddStringToObject(pValue, BLINKER_CMD_TEXT1, config->text2);
    if (config->textColor) cJSON_AddStringToObject(pValue, BLINKER_CMD_TEXTCOLOR, config->textColor);

    char _data[128] = {0};
    cJSON_PrintPreallocated(pValue, _data, 128, 0);
    print(button->name, _data, 0);

    cJSON_Delete(pValue);
}

void blinker_number_print(const BlinkerNumber *number, const blinker_number_config_t * config)
{
    cJSON *pValue = cJSON_CreateObject();

    if (config->icon) cJSON_AddStringToObject(pValue, BLINKER_CMD_ICON, config->icon);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR, config->color);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR_, config->color);
    if (config->unit) cJSON_AddStringToObject(pValue, BLINKER_CMD_UNIT, config->unit);
    if (config->text) cJSON_AddStringToObject(pValue, BLINKER_CMD_TEXT, config->text);
    // cJSON *value = cJSON_CreateNumber(config->value);
    // cJSON_AddItemToObject(pValue, BLINKER_CMD_VALUE, value);
    if (config->value) cJSON_AddStringToObject(pValue, BLINKER_CMD_VALUE, config->value);

    char _data[128] = {0};
    cJSON_PrintPreallocated(pValue, _data, 128, 0);
    print(number->name, _data, 0);

    cJSON_Delete(pValue);
}

void blinker_rgb_print(const BlinkerRGB *rgb, const blinker_rgb_config_t * config)
{
    // cJSON *pValue = cJSON_CreateObject();

    // cJSON_AddItemToObject(pValue, BLINKER_CMD_RGB, cJSON_CreateIntArray(config->rgbw, 4));
    char data[24] = {0};

    sprintf(data, "[%d,%d,%d,%d]", config->rgbw[0], config->rgbw[1], config->rgbw[2], config->rgbw[3]);

    // cJSON *pValue = cJSON_CreateObject();

    // char _data[22] = {0};

    // // cJSON_AddItemToObject(pValue, BLINKER_CMD_RGB, cJSON_CreateIntArray(rrgg, 4));

    // sprintf(_data, "[%d, %d, %d, %d]", config->rgbw[0], config->rgbw[1], config->rgbw[2], config->rgbw[3]);

    // cJSON_AddRawToObject(pValue, BLINKER_CMD_RGB, data);

    // BLINKER_LOG_ALL(TAG, "%s", cJSON_PrintUnformatted(pValue));

    // cJSON_Delete(pValue);

    print(rgb->name, data, 1);

    // cJSON_Delete(pValue);
}

void blinker_slider_print(const BlinkerSlider *slider, const blinker_slider_config_t * config)
{
    cJSON *pValue = cJSON_CreateObject();

    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR, config->color);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR_, config->color);
    if (config->value) cJSON_AddStringToObject(pValue, BLINKER_CMD_VALUE, config->value);

    char _data[128] = {0};
    cJSON_PrintPreallocated(pValue, _data, 128, 0);
    print(slider->name, _data, 0);

    cJSON_Delete(pValue);
}

void blinker_switch_print(const blinker_switch_config_t * config)
{
    // cJSON *pValue = cJSON_CreateObject();

    // if (config->state) cJSON_AddStringToObject(pValue, BLINKER_CMD_BUILTIN_SWITCH, config->state);

    // char _data[128] = {0};
    // cJSON_PrintPreallocated(pValue, _data, 128, 0);
    print(BLINKER_CMD_BUILTIN_SWITCH, config->state, 0);

    // cJSON_Delete(pValue);
}

void blinker_tab_print(const BlinkerTab *tab, const blinker_tab_config_t * config)
{
    cJSON *pValue = cJSON_CreateObject();

    // cJSON_AddItemToObject(pValue, BLINKER_CMD_RGB, cJSON_CreateIntArray(config->rgbw, 4));
    char data[24] = {0};

    sprintf(data, "%d%d%d%d%d", config->tab[0], config->tab[1], config->tab[2], config->tab[3], config->tab[4]);

    cJSON_AddStringToObject(pValue, BLINKER_CMD_VALUE, data);

    // cJSON *pValue = cJSON_CreateObject();

    // char _data[22] = {0};

    // // cJSON_AddItemToObject(pValue, BLINKER_CMD_RGB, cJSON_CreateIntArray(rrgg, 4));

    // sprintf(_data, "[%d, %d, %d, %d]", config->rgbw[0], config->rgbw[1], config->rgbw[2], config->rgbw[3]);

    // cJSON_AddRawToObject(pValue, BLINKER_CMD_RGB, data);

    // BLINKER_LOG_ALL(TAG, "%s", cJSON_PrintUnformatted(pValue));
    char _data[128] = {0};
    cJSON_PrintPreallocated(pValue, _data, 128, 0);
    print(tab->name, _data, 0);

    cJSON_Delete(pValue);

    // cJSON_Delete(pValue);
}

void blinker_text_print(const BlinkerText *text, const blinker_text_config_t * config)
{
    cJSON *pValue = cJSON_CreateObject();

    if (config->icon) cJSON_AddStringToObject(pValue, BLINKER_CMD_ICON, config->icon);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR, config->color);
    if (config->color) cJSON_AddStringToObject(pValue, BLINKER_CMD_COLOR_, config->color);
    if (config->text) cJSON_AddStringToObject(pValue, BLINKER_CMD_TEXT, config->text);
    if (config->text1) cJSON_AddStringToObject(pValue, BLINKER_CMD_VALUE, config->text1);

    char _data[128] = {0};
    cJSON_PrintPreallocated(pValue, _data, 128, 0);
    print(text->name, _data, 0);

    cJSON_Delete(pValue);
}

void blinker_button_init(BlinkerButton *button, blinker_callback_with_string_arg_t _func)
{
    button->wNum = attach_widget_string(button->name, _func);
}

void blinker_rgb_init(BlinkerRGB *rgb, blinker_callback_with_rgb_arg_t _func)
{
    rgb->wNum = attach_widget_rgb(rgb->name, _func);
}

void blinker_slider_init(BlinkerSlider *slider, blinker_callback_with_int32_arg_t _func)
{
    slider->wNum = attach_widget_int(slider->name, _func);
}

void blinker_switch_init(blinker_callback_with_string_arg_t _func)
{
    attach_widget_string(BLINKER_CMD_BUILTIN_SWITCH, _func);
}

void blinker_tab_init(BlinkerTab *tab, blinker_callback_with_tab_arg_t _func, blinker_callback_t _func1)
{
    tab->wNum = attach_widget_tab(tab->name, _func, _func1);
}

void widget_string_parse(const char *_wName, cJSON *data)
{
    BLINKER_LOG_ALL(TAG, "_Widgets_str _wName: %s", _wName);

    int8_t num = check_string_num(_wName, _Widgets_str, _wCount_str);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    cJSON *state = cJSON_GetObjectItemCaseSensitive(data, _wName);

    BLINKER_LOG_ALL(TAG, "_Widgets_str num: %d", num);

    if (cJSON_IsString(state) && (state->valuestring != NULL))
    {
        BLINKER_LOG_ALL(TAG, "widget_string_parse isParsed");

        _parsed = 1;

        BLINKER_LOG_ALL(TAG, "widget_string_parse: %s", _wName);

        blinker_callback_with_string_arg_t nbFunc = _Widgets_str[num].wfunc;

        if (nbFunc) nbFunc(state->valuestring);
    }

    cJSON_Delete(state);
}

void widget_rgb_parse(const char *_wName, cJSON *data)
{
    BLINKER_LOG_ALL(TAG, "_Widgets_rgb _wName: %s", _wName);

    int8_t num = check_rgb_num(_wName, _Widgets_rgb, _wCount_rgb);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    cJSON *state = cJSON_GetObjectItemCaseSensitive(data, _wName);

    BLINKER_LOG_ALL(TAG, "_Widgets_rgb num: %d", num);

    if (state != NULL && cJSON_IsArray(state))
    {
        BLINKER_LOG_ALL(TAG, "widget_rgb_parse isParsed");

        _parsed = 1;

        BLINKER_LOG_ALL(TAG, "widget_rgb_parse: %s", _wName);

        blinker_callback_with_rgb_arg_t nbFunc = _Widgets_rgb[num].wfunc;

        if (nbFunc) nbFunc(cJSON_GetArrayItem(state, 0)->valuedouble, cJSON_GetArrayItem(state, 1)->valuedouble, cJSON_GetArrayItem(state, 2)->valuedouble, cJSON_GetArrayItem(state, 3)->valuedouble);
    }

    cJSON_Delete(state);
}

void widget_int_parse(const char *_wName, cJSON *data)
{
    BLINKER_LOG_ALL(TAG, "_Widgets_int _wName: %s", _wName);

    int8_t num = check_int_num(_wName, _Widgets_int, _wCount_int);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    cJSON *state = cJSON_GetObjectItemCaseSensitive(data, _wName);

    BLINKER_LOG_ALL(TAG, "_Widgets_int num: %d", num);

    if (state != NULL && cJSON_IsNumber(state))
    {
        BLINKER_LOG_ALL(TAG, "widget_int_parse isParsed");

        _parsed = 1;

        BLINKER_LOG_ALL(TAG, "widget_int_parse: %s", _wName);

        blinker_callback_with_int32_arg_t nbFunc = _Widgets_int[num].wfunc;

        if (nbFunc) nbFunc(state->valueint);
    }

    cJSON_Delete(state);
}

void widget_tab_parse(const char *_wName, cJSON *data)
{
    BLINKER_LOG_ALL(TAG, "_Widgets_tab _wName: %s", _wName);

    int8_t num = check_tab_num(_wName, _Widgets_tab, _wCount_tab);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    cJSON *state = cJSON_GetObjectItemCaseSensitive(data, _wName);

    BLINKER_LOG_ALL(TAG, "_Widgets_tab num: %d", num);

    if (state != NULL && cJSON_IsString(state))
    {
        BLINKER_LOG_ALL(TAG, "widget_tab_parse isParsed");

        _parsed = 1;

        BLINKER_LOG_ALL(TAG, "widget_tab_parse: %s", _wName);

        blinker_callback_with_tab_arg_t nbFunc = _Widgets_tab[num].wfunc;

        // if (nbFunc) nbFunc(atoi(state->valuestring));

        char tab_data[10] = {0};

        cJSON_PrintPreallocated(state, tab_data, 10, 0);

        BLINKER_LOG_ALL(TAG, "tab_data: %c", tab_data[1]);

        if (nbFunc)
        {
            for (uint8_t num = 0; num < 5; num++)
            {
                BLINKER_LOG_ALL(TAG, "num: %c", tab_data[num + 1]);

                if (tab_data[num + 1] == '1')
                {
                    if (nbFunc) {
                        switch (num)
                        {
                            case 0:
                                nbFunc(BLINKER_CMD_TAB_0);
                                break;
                            case 1:
                                nbFunc(BLINKER_CMD_TAB_1);
                                break;
                            case 2:
                                nbFunc(BLINKER_CMD_TAB_2);
                                break;
                            case 3:
                                nbFunc(BLINKER_CMD_TAB_3);
                                break;
                            case 4:
                                nbFunc(BLINKER_CMD_TAB_4);
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }
    }

    blinker_callback_t wFunc1 = _Widgets_tab[num].wfunc1;
    if (wFunc1) {
        wFunc1();
    }

    cJSON_Delete(state);
}

void widget_parse(cJSON *data)
{
    BLINKER_LOG_ALL(TAG, "widget_parse");

    for (uint8_t wNum = 0; wNum < _wCount_str; wNum++) {
        if (_parsed) return;
        widget_string_parse(_Widgets_str[wNum].name, data);
    }
    for (uint8_t wNum = 0; wNum < _wCount_rgb; wNum++) {
        if (_parsed) return;
        widget_rgb_parse(_Widgets_rgb[wNum].name, data);
    }
    for (uint8_t wNum = 0; wNum < _wCount_int; wNum++) {
        if (_parsed) return;
        widget_int_parse(_Widgets_int[wNum].name, data);
    }
    for (uint8_t wNum = 0; wNum < _wCount_tab; wNum++) {
        if (_parsed) return;
        widget_tab_parse(_Widgets_tab[wNum].name, data);
    }
}

void check_format(void)
{
    if (!autoFormat)
    {
        autoFormat = 1;

        send_buf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
        memset(send_buf, '\0', BLINKER_MAX_SEND_SIZE);
    }
}

void auto_format_data(const char * key, const char * value, int8_t isRaw)
{
    BLINKER_LOG_ALL(TAG, "auto_format_data key: %s, value: %s", key, value);

    if (strlen(send_buf))
    {
        cJSON *root = cJSON_Parse(send_buf);

        if (root != NULL)
        {
            cJSON *check_key = cJSON_GetObjectItemCaseSensitive(root, key);
            if (check_key != NULL)
            {
                cJSON_DeleteItemFromObjectCaseSensitive(root, key);
            }
        }

        if (isJson(value) && !isRaw)
        {
            cJSON *new_item = cJSON_Parse(value);
            cJSON_AddItemToObject(root, key, new_item);
        }
        else
        {
            if (isRaw) 
            {
                cJSON_AddRawToObject(root, key, value);
                BLINKER_LOG_ALL(TAG, "cJSON_AddRawToObject");
            }
            else 
            {
                cJSON_AddStringToObject(root, key, value);
                BLINKER_LOG_ALL(TAG, "cJSON_AddStringToObject");
            }
        }

        cJSON_PrintPreallocated(root, send_buf, BLINKER_MAX_SEND_SIZE, 0);
        BLINKER_LOG_ALL(TAG, "auto_format_data2: %s", send_buf);
        cJSON_Delete(root);
    }
    else
    {
        cJSON *root = cJSON_CreateObject();

        if (isJson(value) && !isRaw)
        {
            cJSON *new_item = cJSON_Parse(value);
            cJSON_AddItemToObject(root, key, new_item);
        }
        else
        {
            if (isRaw) 
            {
                cJSON_AddRawToObject(root, key, value);
                BLINKER_LOG_ALL(TAG, "cJSON_AddRawToObject");
            }
            else 
            {
                cJSON_AddStringToObject(root, key, value);
                BLINKER_LOG_ALL(TAG, "cJSON_AddStringToObject");
            }
        }

        cJSON_PrintPreallocated(root, send_buf, BLINKER_MAX_SEND_SIZE, 0);
        BLINKER_LOG_ALL(TAG, "auto_format_data2: %s", send_buf);
        cJSON_Delete(root);
    }
}

void print(const char * key, const char * value, int8_t isRaw)
{
    check_format();
    auto_format_data(key, value, isRaw);
    if ((millis() - autoFormatFreshTime) >= BLINKER_MSG_AUTOFORMAT_TIMEOUT)
    {
        autoFormatFreshTime = millis();
    }
}

void check_auto_format(void)
{
    if (autoFormat)
    {
        if ((millis() - autoFormatFreshTime) >= BLINKER_MSG_AUTOFORMAT_TIMEOUT)
        {
            if (strlen(send_buf))
            {
                uint8_t need_check = 1;
                if (strstr(send_buf, BLINKER_CMD_ONLINE)) need_check = 0;
                blinker_mqtt_print(send_buf, need_check);
            }
            free(send_buf);
            send_buf = NULL;
            autoFormat = 0;
            BLINKER_LOG_FreeHeap_ALL(TAG);
        }
    }
}

void default_init(const char * key, const char * ssid, const char * pswd)
{
    BLINKER_LOG_ALL(TAG, "KEY: %s, SSID: %s, PSWD: %s", key, ssid, pswd);
    wifi_init_sta(key, ssid, pswd, parse);
    device_register();
    blinker_mqtt_init();
    run();
}

void smart_init(const char * key)
{
    BLINKER_LOG_ALL(TAG, "KEY: %s", key);
    wifi_init_smart(key);
    device_register();
    blinker_mqtt_init();
    run();
}

void blinker_init(const blinker_config_t *_conf)
{
    if (_conf->wifi == BLINKER_DEFAULT_CONFIG)
    {
        Blinker.begin = default_init;
    }
    else
    {
        Blinker.begin = smart_init;
    }
    
    if (_conf->aligenie != NULL)
    {
        ali_type(_conf->aligenie, aligenie_parse);
    }
    if (_conf->dueros != NULL)
    {
        duer_type(_conf->dueros, dueros_parse);
    }
    if (_conf->miot != NULL)
    {
        mi_type(_conf->miot, miot_parse);
    }
}

xTaskHandle pvCreatedTask_ToggleLed4;

void run(void)
{
    xTaskCreate(&blinker_run,
                "blinker_run",
                2048,
                NULL,
                4,
                pvCreatedTask_ToggleLed4);
}

void heart_beat(cJSON *_data)
{
    cJSON *_state = cJSON_GetObjectItemCaseSensitive(_data, BLINKER_CMD_GET);

    if (cJSON_IsString(_state) && (_state->valuestring != NULL))
    {
        if (strcmp(_state->valuestring, BLINKER_CMD_STATE) == 0)
        {
            // char data[512] = "{\"state\":\"online\"}";
            
            // blinker_mqtt_print(data);

            print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE, 0);

            _parsed = 1;
        }
    }

    cJSON_Delete(_state);
}

void parse(const char *data)
{
    BLINKER_LOG_ALL(TAG, "parse get data: %s", data);

    _parsed = 0;

    cJSON *root = cJSON_Parse(data);

    if (root == NULL) 
    {
        cJSON_Delete(root);
        return;
    }

    cJSON *_data = cJSON_GetObjectItemCaseSensitive(root, "data");

    heart_beat(_data);
    widget_parse(_data);

    if (!_parsed)
    {
        if (data_availablie_func)
        {
            data_availablie_func(_data);
        }
    }

    cJSON_Delete(_data);
    cJSON_Delete(root);

    flush();
}

void aligenie_parse(const char *data)
{
    BLINKER_LOG_ALL(TAG, "aligenie parse get data: %s", data);

    cJSON *root = cJSON_Parse(data);

    if (root == NULL) 
    {
        cJSON_Delete(root);
        return;
    }

    cJSON *_data = cJSON_GetObjectItemCaseSensitive(root, "data");

    cJSON *_get = cJSON_GetObjectItemCaseSensitive(_data, "get");
    cJSON *_set = cJSON_GetObjectItemCaseSensitive(_data, "set");

    if (_get != NULL && cJSON_IsString(_get) && (_get->valuestring != NULL))
    {
        if (strcmp(_get->valuestring, BLINKER_CMD_STATE) == 0)
        {
            cJSON *_setNum = cJSON_GetObjectItemCaseSensitive(_data, "num");
            if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_ALL_NUMBER);
            if (_AliGenieQueryFunc_m)
            {
                if (_setNum != NULL && cJSON_IsNumber(_setNum))
                {
                    _AliGenieQueryFunc_m(BLINKER_CMD_QUERY_ALL_NUMBER, _setNum->valueint);
                }
            }
            cJSON_Delete(_setNum);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_POWERSTATE) == 0) {
            cJSON *_setNum = cJSON_GetObjectItemCaseSensitive(_data, "num");
            if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_POWERSTATE_NUMBER);
            if (_AliGenieQueryFunc_m)
            {
                if (_setNum != NULL && cJSON_IsNumber(_setNum))
                {
                    _AliGenieQueryFunc_m(BLINKER_CMD_QUERY_POWERSTATE_NUMBER, _setNum->valueint);
                }
            }
            cJSON_Delete(_setNum);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_COLOR) == 0) {
            if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_COLOR_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_COLOR_) == 0) {
            if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_COLOR_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_COLORTEMP) == 0) {
            if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_COLORTEMP_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_BRIGHTNESS) == 0) {
            if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_BRIGHTNESS_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_TEMP) == 0) {
            if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_TEMP_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_HUMI) == 0) {
            if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_HUMI_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_PM25) == 0) {
            if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_PM25_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_MODE) == 0) {
            if (_AliGenieQueryFunc) _AliGenieQueryFunc(BLINKER_CMD_QUERY_MODE_NUMBER);
        }
    }
    else if (_set != NULL)
    {
        cJSON *_power_state = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_POWERSTATE);
        cJSON *_setNum = cJSON_GetObjectItemCaseSensitive(_set, "num");
        cJSON *_color = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_COLOR);
        cJSON *_color1 = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_COLOR_);
        cJSON *_bright = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_BRIGHTNESS);
        cJSON *_up_bright = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_UPBRIGHTNESS);
        cJSON *_down_bright = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_DOWNBRIGHTNESS);
        cJSON *_color_temp = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_COLORTEMP);
        cJSON *_up_color_temp = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_UPCOLORTEMP);
        cJSON *_down_color_temp = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_DOWNCOLORTEMP);
        cJSON *_mode = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_MODE);
        cJSON *_cmode = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_CANCELMODE);

        if (_power_state != NULL && cJSON_IsString(_power_state) && (_power_state->valuestring != NULL))
        {
            if (_AliGeniePowerStateFunc)
            {
                _AliGeniePowerStateFunc(_power_state->valuestring);
            }
            if (_AliGeniePowerStateFunc_m)
            {
                _AliGeniePowerStateFunc_m(_power_state->valuestring, _setNum->valueint);
            }                
        }
        else if (_color != NULL && cJSON_IsString(_color))
        {
            if (_AliGenieSetColorFunc) _AliGenieSetColorFunc(_color->valuestring);
        }
        else if (_color1 != NULL && cJSON_IsString(_color1))
        {
            if (_AliGenieSetColorFunc) _AliGenieSetColorFunc(_color1->valuestring);
        }
        else if (_bright != NULL && cJSON_IsNumber(_bright))
        {
            if (_AliGenieSetBrightnessFunc) _AliGenieSetBrightnessFunc(_bright->valueint);
        }
        else if (_up_bright != NULL && cJSON_IsNumber(_up_bright))
        {
            if (_AliGenieSetRelativeBrightnessFunc) _AliGenieSetRelativeBrightnessFunc(_up_bright->valueint);
        }
        else if (_down_bright != NULL && cJSON_IsNumber(_down_bright))
        {
            if (_AliGenieSetRelativeBrightnessFunc) _AliGenieSetRelativeBrightnessFunc(_down_bright->valueint);
        }
        else if (_color_temp != NULL && cJSON_IsNumber(_color_temp))
        {
            if (_AliGenieSetColorTemperature) _AliGenieSetColorTemperature(_color_temp->valueint);
        }
        else if (_up_color_temp != NULL && cJSON_IsNumber(_up_color_temp))
        {
            if (_AliGenieSetRelativeColorTemperature) _AliGenieSetRelativeColorTemperature(_up_color_temp->valueint);
        }
        else if (_down_color_temp != NULL && cJSON_IsNumber(_down_color_temp))
        {
            if (_AliGenieSetRelativeColorTemperature) _AliGenieSetRelativeColorTemperature(_down_color_temp->valueint);
        }
        else if (_mode != NULL && cJSON_IsString(_mode))
        {
            if (_AliGenieSetModeFunc) _AliGenieSetModeFunc(_mode->valuestring);
        }
        else if (_cmode != NULL && cJSON_IsString(_cmode))
        {
            if (_AliGenieSetcModeFunc) _AliGenieSetcModeFunc(_cmode->valuestring);
        }
    }

    cJSON_Delete(root);
}

void dueros_parse(const char *data)
{
    BLINKER_LOG_ALL(TAG, "aligenie parse get data: %s", data);

    cJSON *root = cJSON_Parse(data);

    if (root == NULL) 
    {
        cJSON_Delete(root);
        return;
    }

    cJSON *_data = cJSON_GetObjectItemCaseSensitive(root, "data");

    cJSON *_get = cJSON_GetObjectItemCaseSensitive(_data, "get");
    cJSON *_set = cJSON_GetObjectItemCaseSensitive(_data, "set");

    if (_get != NULL && cJSON_IsString(_get) && (_get->valuestring != NULL))
    {
        if (strcmp(_get->valuestring, BLINKER_CMD_AQI) == 0) {
            if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_AQI_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_PM25) == 0) {
            if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_PM25_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_PM10) == 0) {
            if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_PM10_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_CO2) == 0) {
            if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_CO2_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_TEMP) == 0) {
            if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_TEMP_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_HUMI) == 0) {
            if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_HUMI_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_MODE) == 0) {
            if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_MODE_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_TIME_ALL) == 0)
        {
            cJSON *_setNum = cJSON_GetObjectItemCaseSensitive(_data, "num");
            if (_DuerOSQueryFunc) _DuerOSQueryFunc(BLINKER_CMD_QUERY_TIME_NUMBER);
            if (_DuerOSQueryFunc_m)
            {
                if (_setNum != NULL && cJSON_IsNumber(_setNum))
                {
                    _DuerOSQueryFunc_m(BLINKER_CMD_QUERY_TIME_NUMBER, _setNum->valueint);
                }
            }
            cJSON_Delete(_setNum);
        }
    }
    else if (_set != NULL)
    {
        cJSON *_power_state = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_POWERSTATE);
        cJSON *_setNum = cJSON_GetObjectItemCaseSensitive(_set, "num");
        cJSON *_color = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_COLOR);
        cJSON *_color1 = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_COLOR_);
        cJSON *_bright = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_BRIGHTNESS);
        cJSON *_up_bright = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_UPBRIGHTNESS);
        cJSON *_down_bright = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_DOWNBRIGHTNESS);
        cJSON *_mode = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_MODE);
        cJSON *_cmode = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_CANCELMODE);

        if (_power_state != NULL && cJSON_IsString(_power_state) && (_power_state->valuestring != NULL))
        {
            if (_DuerOSPowerStateFunc)
            {
                _DuerOSPowerStateFunc(_power_state->valuestring);
            }
            if (_DuerOSPowerStateFunc_m)
            {
                _DuerOSPowerStateFunc_m(_power_state->valuestring, _setNum->valueint);
            }                
        }
        else if (_color != NULL && cJSON_IsNumber(_color))
        {
            if (_DuerOSSetColorFunc) _DuerOSSetColorFunc(_color->valueint);
        }
        else if (_color1 != NULL && cJSON_IsNumber(_color1))
        {
            if (_DuerOSSetColorFunc) _DuerOSSetColorFunc(_color1->valueint);
        }
        else if (_bright != NULL && cJSON_IsNumber(_bright))
        {
            if (_DuerOSSetBrightnessFunc) _DuerOSSetBrightnessFunc(_bright->valueint);
        }
        else if (_up_bright != NULL && cJSON_IsNumber(_up_bright))
        {
            if (_DuerOSSetRelativeBrightnessFunc) _DuerOSSetRelativeBrightnessFunc(_up_bright->valueint);
        }
        else if (_down_bright != NULL && cJSON_IsNumber(_down_bright))
        {
            if (_DuerOSSetRelativeBrightnessFunc) _DuerOSSetRelativeBrightnessFunc(_down_bright->valueint);
        }
        else if (_mode != NULL && cJSON_IsString(_mode))
        {
            if (_DuerOSSetModeFunc) _DuerOSSetModeFunc(_mode->valuestring);
        }
        else if (_cmode != NULL && cJSON_IsString(_cmode))
        {
            if (_DuerOSSetcModeFunc) _DuerOSSetcModeFunc(_cmode->valuestring);
        }
    }

    cJSON_Delete(root);
}

void miot_parse(const char *data)
{
    BLINKER_LOG_ALL(TAG, "miot parse get data: %s", data);

    cJSON *root = cJSON_Parse(data);

    if (root == NULL) 
    {
        cJSON_Delete(root);
        return;
    }

    cJSON *_data = cJSON_GetObjectItemCaseSensitive(root, "data");

    cJSON *_get = cJSON_GetObjectItemCaseSensitive(_data, "get");
    cJSON *_set = cJSON_GetObjectItemCaseSensitive(_data, "set");

    if (_get != NULL && cJSON_IsString(_get) && (_get->valuestring != NULL))
    {
        if (strcmp(_get->valuestring, BLINKER_CMD_STATE) == 0)
        {
            cJSON *_setNum = cJSON_GetObjectItemCaseSensitive(_data, "num");
            if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_ALL_NUMBER);
            if (_MIOTQueryFunc_m)
            {
                if (_setNum != NULL && cJSON_IsNumber(_setNum))
                {
                    _MIOTQueryFunc_m(BLINKER_CMD_QUERY_ALL_NUMBER, _setNum->valueint);
                }
            }
            cJSON_Delete(_setNum);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_POWERSTATE) == 0) {
            cJSON *_setNum = cJSON_GetObjectItemCaseSensitive(_data, "num");
            if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_POWERSTATE_NUMBER);
            if (_MIOTQueryFunc_m)
            {
                if (_setNum != NULL && cJSON_IsNumber(_setNum))
                {
                    _MIOTQueryFunc_m(BLINKER_CMD_QUERY_POWERSTATE_NUMBER, _setNum->valueint);
                }
            }
            cJSON_Delete(_setNum);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_COLOR) == 0) {
            if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_COLOR_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_COLOR_) == 0) {
            if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_COLOR_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_COLORTEMP) == 0) {
            if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_COLORTEMP_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_BRIGHTNESS) == 0) {
            if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_BRIGHTNESS_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_TEMP) == 0) {
            if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_TEMP_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_HUMI) == 0) {
            if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_HUMI_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_PM25) == 0) {
            if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_PM25_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_PM25) == 0) {
            if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_PM25_NUMBER);
        }
        else if (strcmp(_get->valuestring, BLINKER_CMD_MODE) == 0) {
            if (_MIOTQueryFunc) _MIOTQueryFunc(BLINKER_CMD_QUERY_MODE_NUMBER);
        }
    }
    else if (_set != NULL)
    {
        cJSON *_power_state = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_POWERSTATE);
        cJSON *_setNum = cJSON_GetObjectItemCaseSensitive(_set, "num");
        cJSON *_color = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_COLOR);
        cJSON *_color1 = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_COLOR_);
        cJSON *_bright = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_BRIGHTNESS);
        cJSON *_color_temp = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_COLORTEMP);
        cJSON *_mode = cJSON_GetObjectItemCaseSensitive(_set, BLINKER_CMD_MODE);

        if (_power_state != NULL && cJSON_IsBool(_power_state))
        {
            if (_MIOTPowerStateFunc)
            {
                if (cJSON_IsTrue(_power_state))
                {
                    _MIOTPowerStateFunc("True");
                }
                else
                {
                    _MIOTPowerStateFunc("False");
                }
                
            }
            if (_MIOTPowerStateFunc_m)
            {
                if (cJSON_IsTrue(_power_state))
                {
                    _MIOTPowerStateFunc_m("true", _setNum->valueint);
                }
                else
                {
                    _MIOTPowerStateFunc_m("false", _setNum->valueint);
                }
            }                
        }
        else if (_color != NULL && cJSON_IsNumber(_color))
        {
            if (_MIOTSetColorFunc) _MIOTSetColorFunc(_color->valueint);
        }
        else if (_color1 != NULL && cJSON_IsNumber(_color1))
        {
            if (_MIOTSetColorFunc) _MIOTSetColorFunc(_color1->valueint);
        }
        else if (_bright != NULL && cJSON_IsNumber(_bright))
        {
            if (_MIOTSetBrightnessFunc) _MIOTSetBrightnessFunc(_bright->valueint);
        }
        else if (_color_temp != NULL && cJSON_IsNumber(_color_temp))
        {
            if (_MIOTSetColorTemperature) _MIOTSetColorTemperature(_color_temp->valueint);
        }
        else if (_mode != NULL && cJSON_IsNumber(_mode))
        {
            if (_MIOTSetModeFunc) _MIOTSetModeFunc(_mode->valueint);
        }
    }

    cJSON_Delete(root);
}

void blinker_run(void* pv)
{
    while(1)
    {
        // if (available())
        // {
        //     BLINKER_LOG_FreeHeap(TAG);

        //     // char data[1024];
        //     // strcpy(data, last_read());

        //     BLINKER_LOG_ALL(TAG, "get data: %d", strlen(last_read()));

        //     // TaskHandle_t TaskHandle;    
        //     // TaskStatus_t TaskStatus;

        //     // TaskHandle=xTaskGetHandle("blinker_run");         //根据任务名获取任务句柄

        //     // //获取LED0_Task的任务信息
        //     // vTaskGetInfo((TaskHandle_t  )TaskHandle,        //任务句柄
        //     //             (TaskStatus_t* )&TaskStatus,       //任务信息结构体
        //     //             (BaseType_t    )pdTRUE,            //允许统计任务堆栈历史最小剩余大小
        //     //             (eTaskState    )eInvalid);         //函数自己获取任务运行壮态

        //     // BLINKER_LOG_ALL(TAG, "任务名:                %s\r\n",TaskStatus.pcTaskName);
        //     // BLINKER_LOG_ALL(TAG, "任务编号:              %d\r\n",(int)TaskStatus.xTaskNumber);
        //     // BLINKER_LOG_ALL(TAG, "任务壮态:              %d\r\n",TaskStatus.eCurrentState);
        //     // BLINKER_LOG_ALL(TAG, "任务当前优先级:        %d\r\n",(int)TaskStatus.uxCurrentPriority);
        //     // BLINKER_LOG_ALL(TAG, "任务基优先级:          %d\r\n",(int)TaskStatus.uxBasePriority);
        //     // BLINKER_LOG_ALL(TAG, "任务堆栈基地址:        %#x\r\n",(int)TaskStatus.pxStackBase);
        //     // BLINKER_LOG_ALL(TAG, "任务堆栈历史剩余最小值: %d\r\n",TaskStatus.usStackHighWaterMark);
        //     unsigned portBASE_TYPE xHighWaterMark = uxTaskGetStackHighWaterMark( pvCreatedTask_ToggleLed4 );
        //     BLINKER_LOG_ALL(TAG, "ToggleLed4: %ld", xHighWaterMark);

        //     parse(last_read());
        // }
        check_auto_format();

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}



uint32_t    _smsTime = 0;
uint32_t    _pushTime = 0;
uint32_t    _wechatTime = 0;
uint32_t    _weatherTime = 0;
uint32_t    _aqiTime = 0;

uint8_t check_sms(void)
{
    if ((millis() - _smsTime) >= BLINKER_SMS_MSG_LIMIT || 
        _smsTime == 0) return 1;
    else return 0;
}

uint8_t check_push(void)
{
    if ((millis() - _pushTime) >= BLINKER_PUSH_MSG_LIMIT || 
        _pushTime == 0) return 1;
    else return 0;
}

uint8_t check_wechat(void)
{
    if ((millis() - _wechatTime) >= BLINKER_WECHAT_MSG_LIMIT || 
        _wechatTime == 0) return 1;
    else return 0;
}

uint8_t check_weather(void)
{
    if ((millis() - _weatherTime) >= BLINKER_WEATHER_MSG_LIMIT || 
        _weatherTime == 0) return 1;
    else return 0;
}

uint8_t check_aqi(void)
{
    if ((millis() - _aqiTime) >= BLINKER_WEATHER_MSG_LIMIT || 
        _aqiTime == 0) return 1;
    else return 0;
}

void blinker_sms(const blinker_sms_config_t *config)
{
    if (!(config->message) || strlen(config->message) >= 20) return;

    if (!check_sms()) return;

    _smsTime = millis();

    char data[128];

    strcpy(data, "{\"deviceName\":\"");
    strcat(data, mqtt_device_name());
    if (config->cell)
    {
        strcat(data, "\",\"cel\":\"");
        strcat(data, config->cell);
    }
    strcat(data, "\",\"key\":\"");
    strcat(data, mqtt_auth_key());
    strcat(data, "\",\"msg\":\"");
    strcat(data, config->message);
    strcat(data, "\"}");

    blinker_https_post(BLINKER_SERVER_HOST, "/api/v1/user/device/sms", data);

    blinker_server(BLINKER_CMD_SMS_NUMBER);
}

void blinker_push(const char *msg)
{
    if (strlen(msg) > 64) return;

    if (!check_push()) return;

    _pushTime = millis();

    char data[128];

    strcpy(data, "{\"deviceName\":\"");
    strcat(data, mqtt_device_name());
    strcat(data, "\",\"key\":\"");
    strcat(data, mqtt_auth_key());
    strcat(data, "\",\"msg\":\"");
    strcat(data, msg);
    strcat(data, "\"}");

    blinker_https_post(BLINKER_SERVER_HOST, "/api/v1/user/device/push", data);

    blinker_server(BLINKER_CMD_PUSH_NUMBER);
}

void blinker_wechat(const blinker_wechat_config_t *config)
{
    if (!(config->message) || strlen(config->message) >= 64) return;

    if (!check_wechat()) return;

    _wechatTime = millis();

    char data[128];

    strcpy(data, "{\"deviceName\":\"");
    strcat(data, mqtt_device_name());
    strcat(data, "\",\"key\":\"");
    strcat(data, mqtt_auth_key());
    if (config->title)
    {
        strcat(data, "\",\"title\":\"");
        strcat(data, config->title);
    }
    if (config->state)
    {
        strcat(data, "\",\"state\":\"");
        strcat(data, config->state);
    }
    strcat(data, "\",\"msg\":\"");
    strcat(data, config->message);
    strcat(data, "\"}");

    blinker_https_post(BLINKER_SERVER_HOST, "/api/v1/user/device/wxMsg/", data);

    blinker_server(BLINKER_CMD_WECHAT_NUMBER);
}

void blinker_weather(const char *city)
{
    if (strlen(city) > 40) return;

    if (!check_weather()) return;

    _weatherTime = millis();

    char url[128];

    strcpy(url, "/api/v1/weather/now?");
    strcat(url, "deviceName=");
    strcat(url, mqtt_device_name());
    strcat(url, "&key=");
    strcat(url, mqtt_auth_key());    
    strcat(url, "&location=");
    strcat(url, city);

    blinker_https_get(BLINKER_SERVER_HOST, url);

    blinker_server(BLINKER_CMD_WEATHER_NUMBER);
}

void blinker_attach_weather_data(blinker_callback_with_json_arg_t func)
{
    weather_data(func);
}

void blinker_aqi(const char *city)
{
    if (strlen(city) > 40) return;

    if (!check_aqi()) return;

    _aqiTime = millis();

    char url[128];

    strcpy(url, "/api/v1/weather/aqi?");
    strcat(url, "deviceName=");
    strcat(url, mqtt_device_name());
    strcat(url, "&key=");
    strcat(url, mqtt_auth_key());
    strcat(url, "&location=");
    strcat(url, city);

    blinker_https_get(BLINKER_SERVER_HOST, url);

    blinker_server(BLINKER_CMD_AQI_NUMBER);
}

void blinker_attach_aqi_data(blinker_callback_with_json_arg_t func)
{
    aqi_data(func);
}

void blinker_fresh_sharers(void)
{
    char url[128];

    strcpy(url, "/api/v1/user/device/share/device?");
    strcat(url, "deviceName=");
    strcat(url, mqtt_device_name());
    strcat(url, "&key=");
    strcat(url, mqtt_auth_key());

    blinker_https_get(BLINKER_SERVER_HOST, url);

    blinker_server(BLINKER_CMD_FRESH_SHARERS_NUMBER);
}