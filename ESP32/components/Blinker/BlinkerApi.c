#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"

#include "BlinkerApi.h"
#include "BlinkerMQTT.h"

static const char *TAG = "BlinkerApi";

int8_t _parsed = 0;
int8_t autoFormat = 0;
char send_buf[BLINKER_MAX_SEND_SIZE] = { 0 };
uint32_t autoFormatFreshTime = 0;

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

void parse(const char *data);
void run(void);

#if defined (CONFIG_BLINKER_DEFAULT_CONFIG)
// void default_init(const char * key, const char * ssid, const char * pswd)
void default_init()
{
    // BLINKER_LOG_ALL(TAG, "KEY: %s, SSID: %s, PSWD: %s", 
    //                 key, ssid, pswd);
    // wifi_init_sta(key, ssid, pswd, parse);

    BLINKER_LOG_ALL(TAG, "KEY: %s, SSID: %s, PSWD: %s", 
                    CONFIG_BLINKER_AUTHKEY, 
                    CONFIG_BLINKER_WLAN_SSID, 
                    CONFIG_BLINKER_WLAN_PSWD);
    wifi_init_sta(CONFIG_BLINKER_WLAN_SSID, 
                CONFIG_BLINKER_WLAN_PSWD);
    // websocket_init();
    device_register();
    blinker_mqtt_init();
    // blinker_fresh_sharers();
    run();
}

#elif defined (CONFIG_BLINKER_SMART_CONFIG)

// void smart_init(const char * key)
void smart_init(void)
{
    // BLINKER_LOG_ALL(TAG, "KEY: %s", CONFIG_BLINKER_AUTHKEY);
    wifi_init_smart();
    // websocket_init();
    device_register();
    blinker_mqtt_init();
    // blinker_fresh_sharers();
    run();
}

#elif defined (CONFIG_BLINKER_AP_CONFIG)

// void smart_init(const char * key)
void ap_init(void)
{
    // BLINKER_LOG_ALL(TAG, "KEY: %s", CONFIG_BLINKER_AUTHKEY);
    // wifi_init_ap();
    // // websocket_init();
    // device_register();
    // blinker_mqtt_init();
    // blinker_fresh_sharers();
    // websocket_init();
    // run();
}

#endif

void blinker_init(void)
{
    BLINKER_LOG_FreeHeap(TAG);

    printf("========================================================\n");

    printf("\n");
    printf(" __       __                __\n");
    printf("/\\ \\     /\\ \\    __        /\\ \\              v%s\n", BLINKER_VERSION);
    printf("\\ \\ \\___ \\ \\ \\  /\\_\\    ___\\ \\ \\/'\\      __   _ __   \n");
    printf(" \\ \\ '__`\\\\ \\ \\ \\/\\ \\ /' _ `\\ \\ , <    /'__`\\/\\`'__\\ \n");
    printf("  \\ \\ \\L\\ \\\\ \\ \\_\\ \\ \\/\\ \\/\\ \\ \\ \\\\`\\ /\\  __/\\ \\ \\./ \n");
    printf("   \\ \\_,__/ \\ \\__\\\\ \\_\\ \\_\\ \\_\\ \\_\\ \\_\\ \\____\\\\ \\_\\  \n");
    printf("    \\/___/   \\/__/ \\/_/\\/_/\\/_/\\/_/\\/_/\\/____/ \\/_/  \n");
    printf("    To better use blinker with your IoT project!\n");
    printf("    Download latest blinker library here!\n");
    printf("    => https://github.com/blinker-iot/blinker-freertos\n\n");

    #if defined (CONFIG_BLINKER_MODE_WIFI)
        printf("    BLINKER_WIFI device init\n");
    #elif defined (CONFIG_BLINKER_MODE_PRO)
        printf("    BLINKER_PRO device init\n");
    #endif

    BLINKER_LOG(TAG, "test");

    printf("\n========================================================\n");

    // button_test();

    // blinker_spiffs_init();
    // blinker_spiffs_auth_check();

    #if defined (CONFIG_BLINKER_ALIGENIE_LIGHT)
        ali_type(BLINKER_ALIGENIE_LIGHT, aligenie_parse);
    #elif defined (CONFIG_BLINKER_ALIGENIE_OUTLET)
        ali_type(BLINKER_ALIGENIE_OUTLET, aligenie_parse);
    #elif defined (CONFIG_BLINKER_ALIGENIE_MULTI_OUTLET)
        ali_type(BLINKER_ALIGENIE_MULTI_OUTLET, aligenie_parse);
    #elif defined (CONFIG_BLINKER_ALIGENIE_SENSOR)
        ali_type(BLINKER_ALIGENIE_SENSOR, aligenie_parse);
    #endif

    #if defined (CONFIG_BLINKER_DUEROS_LIGHT)
        duer_type(BLINKER_DUEROS_LIGHT, dueros_parse);
    #elif defined (CONFIG_BLINKER_DUEROS_OUTLET)
        duer_type(BLINKER_DUEROS_OUTLET, dueros_parse);
    #elif defined (CONFIG_BLINKER_DUEROS_MULTI_OUTLET)
        duer_type(BLINKER_DUEROS_MULTI_OUTLET, dueros_parse);
    #elif defined (CONFIG_BLINKER_DUEROS_SENSOR)
        duer_type(BLINKER_DUEROS_SENSOR, dueros_parse);
    #endif

    #if defined (CONFIG_BLINKER_MIOT_LIGHT)
        mi_type(BLINKER_MIOT_LIGHT, miot_parse);
    #elif defined (CONFIG_BLINKER_MIOT_OUTLET)
        mi_type(BLINKER_MIOT_OUTLET, miot_parse);
    #elif defined (CONFIG_BLINKER_MIOT_MULTI_OUTLET)
        mi_type(BLINKER_MIOT_MULTI_OUTLET, miot_parse);
    #elif defined (CONFIG_BLINKER_MIOT_SENSOR)
        mi_type(BLINKER_MIOT_SENSOR, miot_parse);
    #endif

    #if defined (CONFIG_BLINKER_MODE_WIFI)
        BLINKER_LOG_ALL(TAG, "KEY: %s", CONFIG_BLINKER_AUTHKEY);

        blinker_set_auth(CONFIG_BLINKER_AUTHKEY, parse);
    #elif defined (CONFIG_BLINKER_MODE_PRO)
        BLINKER_LOG_ALL(TAG, "TYPE: %s, KEY: %s", CONFIG_BLINKER_PRO_TYPE, CONFIG_BLINKER_PRO_AUTH);

        blinker_set_type_auth(CONFIG_BLINKER_PRO_TYPE, CONFIG_BLINKER_PRO_AUTH, parse);
    #endif

    #if defined (CONFIG_BLINKER_DEFAULT_CONFIG)
        default_init();
    #elif defined (CONFIG_BLINKER_SMART_CONFIG)
        smart_init();
    #elif defined (CONFIG_BLINKER_AP_CONFIG)
        ap_init();
    #endif
}


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
    // cJSON_PrintPreallocated(pValue, _data, BLINKER_MAX_SEND_SIZE, 0);
    strcpy(_data, cJSON_PrintUnformatted(pValue));
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
    // cJSON_PrintPreallocated(pValue, _data, BLINKER_MAX_SEND_SIZE, 0);
    strcpy(_data, cJSON_PrintUnformatted(pValue));
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
    // cJSON_PrintPreallocated(pValue, _data, BLINKER_MAX_SEND_SIZE, 0);
    strcpy(_data, cJSON_PrintUnformatted(pValue));
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
    // cJSON_PrintPreallocated(pValue, _data, 128, 0);
    strcpy(_data, cJSON_PrintUnformatted(pValue));
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
    // cJSON_PrintPreallocated(pValue, _data, 128, 0);
    strcpy(_data, cJSON_PrintUnformatted(pValue));
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
    // cJSON_PrintPreallocated(pValue, _data, 128, 0);
    strcpy(_data, cJSON_PrintUnformatted(pValue));
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
    // cJSON_PrintPreallocated(pValue, _data, 128, 0);
    strcpy(_data, cJSON_PrintUnformatted(pValue));
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
    // cJSON_PrintPreallocated(pValue, _data, 128, 0);
    strcpy(_data, cJSON_PrintUnformatted(pValue));
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

    // cJSON_Delete(state);
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

    // cJSON_Delete(state);
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

    // cJSON_Delete(state);
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

        // cJSON_PrintPreallocated(state, tab_data, 10, 0);
        strcpy(tab_data, cJSON_PrintUnformatted(state));

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

    // cJSON_Delete(state);
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

        // send_buf = (char*)malloc(BLINKER_MAX_SEND_SIZE*sizeof(char));
        memset(send_buf, '\0', BLINKER_MAX_SEND_SIZE);
    }
}

void heart_beat(cJSON *_data)
{
    cJSON *_state = cJSON_GetObjectItemCaseSensitive(_data, BLINKER_CMD_GET);

    if (cJSON_IsString(_state) && (_state->valuestring != NULL))
    {
        if (strcmp(_state->valuestring, BLINKER_CMD_STATE) == 0)
        {
            // char data[512] = "{\"state\":\"online\"}";
            
            // blinker_print(data);
            print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE, 0);

            _parsed = 1;
        }
    }

    // cJSON_Delete(_state);
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

        // cJSON_PrintPreallocated(root, send_buf, BLINKER_MAX_SEND_SIZE, 0);
        strcpy(send_buf, cJSON_PrintUnformatted(root));
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

        // cJSON_PrintPreallocated(root, send_buf, BLINKER_MAX_SEND_SIZE, 0);
        strcpy(send_buf, cJSON_PrintUnformatted(root));
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
    // BLINKER_LOG_ALL(TAG, "autoFormat: %d", autoFormat);
    if (autoFormat)
    {
        if ((millis() - autoFormatFreshTime) >= BLINKER_MSG_AUTOFORMAT_TIMEOUT)
        {
            BLINKER_LOG_ALL(TAG, "check_auto_format: %s", send_buf);
            if (strlen(send_buf))
            {
                uint8_t need_check = 1;
                if (strstr(send_buf, BLINKER_CMD_ONLINE)) need_check = 0;
                blinker_print(send_buf, need_check);
            }
            // free(send_buf);
            // send_buf = NULL;
            memset(send_buf, '\0', BLINKER_MAX_SEND_SIZE);
            autoFormat = 0;
            BLINKER_LOG_FreeHeap_ALL(TAG);
        }
    }
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

    if (_data == NULL) 
    {
        heart_beat(root);
        widget_parse(root);

        if (!_parsed)
        {
            if (data_availablie_func)
            {
                data_availablie_func(root);
            }
        }
    }
    else
    {
        heart_beat(_data);
        widget_parse(_data);

        if (!_parsed)
        {
            if (data_availablie_func)
            {
                data_availablie_func(_data);
            }
        }
    }

    BLINKER_LOG_ALL(TAG, "parsed data");

    // cJSON_Delete(_data);
    cJSON_Delete(root);

    // BLINKER_LOG_ALL(TAG, "parsed data cJSON_Delete");

    // flush();
}

xTaskHandle pvCreatedTask_ToggleLed4;

void blinker_run(void* pv)
{
    while(1)
    {
        check_auto_format();

        vTaskDelay(50 / portTICK_RATE_MS);
    }
}

void run(void)
{
    xTaskCreate(&blinker_run,
                "blinker_run",
                4096,
                NULL,
                4,
                pvCreatedTask_ToggleLed4);
}
