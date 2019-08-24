#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"

#include "BlinkerApi.h"
#include "BlinkerMQTT.h"

static const char *TAG = "BlinkerApi";

blinker_api_t Blinker;

int8_t _parsed = 0;
int8_t autoFormat = 0;
char *send_buf = NULL;
uint32_t autoFormatFreshTime = 0;

void run(void);
void blinker_run(void* pv);
void parse(const char *data);

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
            // BLINKER_LOG(TAG, "check_string_num, name: %s, num: %d", name, cNum);
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
            // BLINKER_LOG(TAG, "check_rgb_num, name: %s, num: %d", name, cNum);
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
            // BLINKER_LOG(TAG, "check_rgb_num, name: %s, num: %d", name, cNum);
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
            // BLINKER_LOG(TAG, "check_rgb_num, name: %s, num: %d", name, cNum);
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

    // BLINKER_LOG(TAG, "%s", cJSON_PrintUnformatted(pValue));

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
    cJSON *pValue = cJSON_CreateObject();

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

    // BLINKER_LOG(TAG, "%s", cJSON_PrintUnformatted(pValue));
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
    BLINKER_LOG(TAG, "_Widgets_str _wName: %s", _wName);

    int8_t num = check_string_num(_wName, _Widgets_str, _wCount_str);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    cJSON *state = cJSON_GetObjectItemCaseSensitive(data, _wName);

    BLINKER_LOG(TAG, "_Widgets_str num: %d", num);

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
    BLINKER_LOG(TAG, "_Widgets_rgb _wName: %s", _wName);

    int8_t num = check_rgb_num(_wName, _Widgets_rgb, _wCount_rgb);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    cJSON *state = cJSON_GetObjectItemCaseSensitive(data, _wName);

    BLINKER_LOG(TAG, "_Widgets_rgb num: %d", num);

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
    BLINKER_LOG(TAG, "_Widgets_int _wName: %s", _wName);

    int8_t num = check_int_num(_wName, _Widgets_int, _wCount_int);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    cJSON *state = cJSON_GetObjectItemCaseSensitive(data, _wName);

    BLINKER_LOG(TAG, "_Widgets_int num: %d", num);

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
    BLINKER_LOG(TAG, "_Widgets_tab _wName: %s", _wName);

    int8_t num = check_tab_num(_wName, _Widgets_tab, _wCount_tab);

    if (num == BLINKER_OBJECT_NOT_AVAIL) return;

    cJSON *state = cJSON_GetObjectItemCaseSensitive(data, _wName);

    BLINKER_LOG(TAG, "_Widgets_tab num: %d", num);

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
    BLINKER_LOG(TAG, "auto_format_data key: %s, value: %s", key, value);

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
                BLINKER_LOG(TAG, "cJSON_AddRawToObject");
            }
            else 
            {
                cJSON_AddStringToObject(root, key, value);
                BLINKER_LOG(TAG, "cJSON_AddStringToObject");
            }
        }

        cJSON_PrintPreallocated(root, send_buf, BLINKER_MAX_SEND_SIZE, 0);
        BLINKER_LOG(TAG, "auto_format_data2: %s", send_buf);
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
                BLINKER_LOG(TAG, "cJSON_AddRawToObject");
            }
            else 
            {
                cJSON_AddStringToObject(root, key, value);
                BLINKER_LOG(TAG, "cJSON_AddStringToObject");
            }
        }

        cJSON_PrintPreallocated(root, send_buf, BLINKER_MAX_SEND_SIZE, 0);
        BLINKER_LOG(TAG, "auto_format_data2: %s", send_buf);
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
                blinker_mqtt_print(send_buf);
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
    BLINKER_LOG(TAG, "KEY: %s, SSID: %s, PSWD: %s", key, ssid, pswd);
    wifi_init_sta(key, ssid, pswd, parse);
    https_test();
    run();
}

void blinker_init(const blinker_config_t *_conf)
{
    Blinker.begin = default_init;
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

void parse(const char * data)
{
    BLINKER_LOG(TAG, "parse get data: %s", data);

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

void blinker_run(void* pv)
{
    while(1)
    {
        // if (available())
        // {
        //     BLINKER_LOG_FreeHeap(TAG);

        //     // char data[1024];
        //     // strcpy(data, last_read());

        //     BLINKER_LOG(TAG, "get data: %d", strlen(last_read()));

        //     // TaskHandle_t TaskHandle;    
        //     // TaskStatus_t TaskStatus;

        //     // TaskHandle=xTaskGetHandle("blinker_run");         //根据任务名获取任务句柄

        //     // //获取LED0_Task的任务信息
        //     // vTaskGetInfo((TaskHandle_t  )TaskHandle,        //任务句柄
        //     //             (TaskStatus_t* )&TaskStatus,       //任务信息结构体
        //     //             (BaseType_t    )pdTRUE,            //允许统计任务堆栈历史最小剩余大小
        //     //             (eTaskState    )eInvalid);         //函数自己获取任务运行壮态

        //     // BLINKER_LOG(TAG, "任务名:                %s\r\n",TaskStatus.pcTaskName);
        //     // BLINKER_LOG(TAG, "任务编号:              %d\r\n",(int)TaskStatus.xTaskNumber);
        //     // BLINKER_LOG(TAG, "任务壮态:              %d\r\n",TaskStatus.eCurrentState);
        //     // BLINKER_LOG(TAG, "任务当前优先级:        %d\r\n",(int)TaskStatus.uxCurrentPriority);
        //     // BLINKER_LOG(TAG, "任务基优先级:          %d\r\n",(int)TaskStatus.uxBasePriority);
        //     // BLINKER_LOG(TAG, "任务堆栈基地址:        %#x\r\n",(int)TaskStatus.pxStackBase);
        //     // BLINKER_LOG(TAG, "任务堆栈历史剩余最小值: %d\r\n",TaskStatus.usStackHighWaterMark);
        //     unsigned portBASE_TYPE xHighWaterMark = uxTaskGetStackHighWaterMark( pvCreatedTask_ToggleLed4 );
        //     BLINKER_LOG(TAG, "ToggleLed4: %ld", xHighWaterMark);

        //     parse(last_read());
        // }
        check_auto_format();

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}