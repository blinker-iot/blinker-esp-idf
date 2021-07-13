#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "mbedtls/base64.h"

#include "blinker_api.h"
#include "blinker_button.h"
#include "blinker_config.h"
#include "blinker_http.h"
#include "blinker_mqtt.h"
#include "blinker_storage.h"
#include "blinker_wifi.h"

typedef enum {
    BLINKER_DATA_FROM_USER = 0,
    BLINKER_DATA_FROM_ALIGENIE,
    BLINKER_DATA_FROM_DUEROS,
    BLINKER_DATA_FROM_MIOT,
    BLINKER_DATA_FROM_SERVER,
} blinker_data_from_type_t;

typedef enum {
    BLINKER_EVENT_WIFI_INIT = 0,
} blinker_event_t;

typedef enum {
    BLINKER_HTTP_HEART_BEAT = 0,
} blinker_device_http_type_t;

typedef struct {
    char *key;
    char *value;
    bool is_raw;
} blinker_auto_format_queue_t;

typedef struct {
    char *uuid;
    char *message_id;
    blinker_data_from_type_t type;
} blinker_data_from_param_t;

static const char* TAG                      = "blinker_api";
static xQueueHandle auto_format_queue       = NULL;
static SemaphoreHandle_t data_parse_mutex   = NULL;
static blinker_data_from_param_t *data_from = NULL;
static blinker_va_data_t *va_ali            = NULL;
static blinker_va_data_t *va_duer           = NULL;
static blinker_va_data_t *va_miot           = NULL;

static SLIST_HEAD(widget_list_, blinker_widget_data) blinker_widget_list;

static void blinker_device_print(const char *key, const char *value, bool is_raw)
{
    blinker_auto_format_queue_t *print_queue = NULL;

    print_queue         = calloc(1, sizeof(blinker_auto_format_queue_t));
    print_queue->key    = strdup(key);
    print_queue->value  = strdup(value);
    print_queue->is_raw = is_raw;

    if (xQueueSend(auto_format_queue, &print_queue, 0) == pdFALSE) {
        BLINKER_FREE(print_queue->key);
        BLINKER_FREE(print_queue->value);
        BLINKER_FREE(print_queue);
    }
}

static void blinker_device_print_number(cJSON *root, const char *key, const double value)
{
    if (cJSON_HasObjectItem(root, key)) {
        cJSON_DeleteItemFromObject(root, key);
    }

    cJSON_AddNumberToObject(root, key, value);
}

static void blinker_device_print_string(cJSON *root, const char *key, const char *value)
{
    if (cJSON_HasObjectItem(root, key)) {
        cJSON_DeleteItemFromObject(root, key);
    }

    cJSON_AddStringToObject(root, key, value);
}

static void blinker_device_print_raw(cJSON *root, const char *key, const char *value)
{
    if (cJSON_HasObjectItem(root, key)) {
        cJSON_DeleteItemFromObject(root, key);
    }

    cJSON_AddRawToObject(root, key, value);
}

static void blinker_device_http(const char *url, const char *msg, char *payload, const int payload_len, blinker_device_http_type_t type)
{
    esp_http_client_config_t config = {
        .url = "http://iot.diandeng.tech/",
    };

    esp_http_client_handle_t client = blinker_http_init(&config);

    ESP_LOGI(TAG, "http connect to: %s", url);

    switch (type)
    {
        case BLINKER_HTTP_HEART_BEAT:
            blinker_http_set_url(client, url);
            blinker_http_get(client);
            blinker_http_read_response(client, payload, payload_len);
            blinker_http_close(client);
            break;
        
        default:
            break;
    }

    ESP_LOGI(TAG, "read response: %s", payload);
}

/////////////////////////////////////////////////////////////////////////

static esp_err_t blinker_va_print(cJSON *param, blinker_data_from_param_t *from_param)
{
    esp_err_t err    = ESP_FAIL;

    cJSON *root      = NULL;
    char *print_data = NULL;
    char *pub_topic  = NULL;

    if (blinker_mqtt_broker() == BLINKER_BROKER_ALIYUN) {
        root =cJSON_CreateObject();
        cJSON_AddStringToObject(root, BLINKER_CMD_TO_DEVICE, strcat(from_param->uuid, "_r"));
        cJSON_AddStringToObject(root, BLINKER_CMD_FROM_DEVICE, blinker_mqtt_devicename());
        cJSON_AddStringToObject(root, BLINKER_CMD_DEVICE_TYPE, BLINKER_CMD_VASSISTANT);
        cJSON_AddItemToObject(root, BLINKER_CMD_DATA, param);

        print_data = calloc(BLINKER_FORMAT_DATA_SIZE, sizeof(char));
        
        asprintf(&pub_topic, "/sys/%s/%s/rrpc/response%s", 
                blinker_mqtt_product_key(),
                blinker_mqtt_devicename(),
                from_param->message_id);

        if (!print_data || !pub_topic) {
            BLINKER_FREE(print_data);
            BLINKER_FREE(pub_topic);
            cJSON_Delete(root);
            return err;
        }

        char *format_data = cJSON_PrintUnformatted(root);
        size_t olen = 0;
        mbedtls_base64_encode((unsigned char*)print_data, BLINKER_FORMAT_DATA_SIZE, &olen, (unsigned char*)format_data, strlen(format_data));
        BLINKER_FREE(format_data);
        cJSON_Delete(root);

        err = blinker_mqtt_publish(pub_topic, print_data, strlen(print_data));

        BLINKER_FREE(print_data);
        BLINKER_FREE(pub_topic);
    } else if (blinker_mqtt_broker() == BLINKER_BROKER_BLINKER) {
        root =cJSON_CreateObject();
        cJSON_AddStringToObject(param, BLINKER_CMD_MESSAGE_ID, from_param->message_id);
        cJSON_AddStringToObject(root, BLINKER_CMD_TO_DEVICE,   BLINKER_CMD_SERVER_RECEIVER);
        cJSON_AddStringToObject(root, BLINKER_CMD_FROM_DEVICE, blinker_mqtt_devicename());
        cJSON_AddStringToObject(root, BLINKER_CMD_DEVICE_TYPE, BLINKER_CMD_VASSISTANT);
        cJSON_AddItemToObject(root, BLINKER_CMD_DATA, param);

        print_data = calloc(BLINKER_FORMAT_DATA_SIZE, sizeof(char));
        
        cJSON_PrintPreallocated(root, print_data, BLINKER_FORMAT_DATA_SIZE, false);

        asprintf(&pub_topic, "/device/%s/s",
                blinker_mqtt_devicename());

        if (!print_data || !pub_topic) {
            BLINKER_FREE(print_data);
            BLINKER_FREE(pub_topic);
            cJSON_Delete(root);
            return err;
        }
        cJSON_Delete(root);
        
        err = blinker_mqtt_publish(pub_topic, print_data, strlen(print_data));

        BLINKER_FREE(print_data);
        BLINKER_FREE(pub_topic);
    }

    blinker_log_print_heap();

    return err;
}

static void blinker_va_param_add(blinker_va_data_t *va, const char *param, blinker_va_param_type_t param_type, blinker_va_param_val_type_t val_type)
{
    if (va == NULL) {
        return;
    }

    blinker_va_param_t *va_param = calloc(1, sizeof(blinker_va_param_t));

    if (!va_param) {
        BLINKER_FREE(va_param);
        return;
    }

    va_param->key       = strdup(param);
    va_param->type      = param_type;
    va_param->val_type  = val_type;

    blinker_va_param_t *last = SLIST_FIRST(&va->va_param_list);

    if (last == NULL) {
        SLIST_INSERT_HEAD(&va->va_param_list, va_param, next);
    } else {
        SLIST_INSERT_AFTER(last, va_param, next);
    }
}

static void blinker_va_parse(cJSON *data, blinker_va_data_t *va)
{
    if (va == NULL) {
        return;
    }

    if (cJSON_HasObjectItem(data, BLINKER_CMD_GET)) {
        blinker_va_param_cb_t val = {0};

        if (va->cb) {
            val.type = BLINKER_PARAM_GET_STATE;
            va->cb(&val);
        }
    } else if (cJSON_HasObjectItem(data, BLINKER_CMD_SET)) {
        cJSON *set_data = cJSON_GetObjectItem(data, BLINKER_CMD_SET);

        if (cJSON_IsNull(set_data)) {
            cJSON_Delete(set_data);
            return;
        }

        blinker_va_param_t *va_param;
        SLIST_FOREACH(va_param, &va->va_param_list, next) {
            if (cJSON_HasObjectItem(set_data, va_param->key)) {
                blinker_va_param_cb_t param_val = {0};
                cJSON *param_set = cJSON_GetObjectItem(set_data, va_param->key);

                param_val.type = va_param->type;

                switch (va_param->type)
                {
                    case BLINKER_PARAM_POWER_STATE:
                        if (va_param->val_type == BLINKER_VAL_TYPE_STRING && cJSON_IsString(param_set)) {
                            if (strcmp(BLINKER_CMD_TRUE, param_set->valuestring) == 0) {
                                param_val.s = BLINKER_CMD_ON;
                            } else if (strcmp(BLINKER_CMD_FALSE, param_set->valuestring) == 0) {
                                param_val.s = BLINKER_CMD_OFF;
                            } else {
                                param_val.s = param_set->valuestring;
                            }
                        }
                        if (cJSON_HasObjectItem(set_data, BLINKER_CMD_NUM)) {
                            param_val.num = cJSON_GetObjectItem(set_data, BLINKER_CMD_NUM)->valueint;
                        }
                        va->cb(&param_val);
                        break;

                    case BLINKER_PARAM_MODE_STATE:
                        param_val.s = va_param->key;

                        if (va_param->val_type == BLINKER_VAL_TYPE_STRING && cJSON_IsString(param_set)) {
                            if (strcmp(BLINKER_CMD_TRUE, param_set->valuestring) == 0) {
                                param_val.state = BLINKER_CMD_ON;
                            } else if (strcmp(BLINKER_CMD_FALSE, param_set->valuestring) == 0) {
                                param_val.state = BLINKER_CMD_OFF;
                            } else {
                                param_val.state = param_set->valuestring;
                            }
                        }
                        va->cb(&param_val);
                        break;
                    
                    default:
                        if (va_param->val_type == BLINKER_VAL_TYPE_STRING && cJSON_IsString(param_set)) {
                            param_val.s = param_set->valuestring;
                        } else if (va_param->val_type == BLINKER_VAL_TYPE_INT) {
                            if (cJSON_IsString(param_set)) {
                                param_val.i = atoi(param_set->valuestring);
                            } else {
                                param_val.i = param_set->valueint;
                            }
                        }
                        va->cb(&param_val);
                        break;
                }
                
                cJSON_Delete(param_set);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////

esp_err_t blinker_va_multi_num(cJSON *param, const int num)
{
    blinker_device_print_number(param, BLINKER_CMD_NUM, num);

    return ESP_OK;
}

esp_err_t blinker_aligenie_power_state(cJSON *param, const char *state)
{
    blinker_device_print_string(param, BLINKER_CMD_POWER_STATE, state);

    return ESP_OK;
}

esp_err_t blinker_aligenie_color(cJSON *param, const char *color)
{
    blinker_device_print_string(param, BLINKER_CMD_COLOR_, color);

    return ESP_OK;
}

esp_err_t blinker_aligenie_mode(cJSON *param, const char *mode)
{
    blinker_device_print_string(param, BLINKER_CMD_MODE, mode);

    return ESP_OK;
}

esp_err_t blinker_aligenie_color_temp(cJSON *param, const int color_temp)
{
    blinker_device_print_number(param, BLINKER_CMD_COLOR_TEMP, color_temp);

    return ESP_OK;
}

esp_err_t blinker_aligenie_brightness(cJSON *param, const int bright)
{
    blinker_device_print_number(param, BLINKER_CMD_BRIGHTNESS, bright);

    return ESP_OK;
}

esp_err_t blinker_aligenie_temp(cJSON *param, const int temp)
{
    blinker_device_print_number(param, BLINKER_CMD_TEMP, temp);

    return ESP_OK;
}

esp_err_t blinker_aligenie_humi(cJSON *param, const int humi)
{
    blinker_device_print_number(param, BLINKER_CMD_TEMP, humi);

    return ESP_OK;
}

esp_err_t blinker_aligenie_pm25(cJSON *param, const int pm25)
{
    blinker_device_print_number(param, BLINKER_CMD_PM25, pm25);

    return ESP_OK;
}

esp_err_t blinker_aligenie_level(cJSON *param, const int level)
{
    blinker_device_print_number(param, BLINKER_CMD_LEVEL, level);

    return ESP_OK;
}

esp_err_t blinker_aligenie_hswing(cJSON *param, const char *state)
{
    blinker_device_print_string(param, BLINKER_CMD_HSTATE, state);

    return ESP_OK;
}

esp_err_t blinker_aligenie_vswing(cJSON *param, const char *state)
{
    blinker_device_print_string(param, BLINKER_CMD_VSTATE, state);

    return ESP_OK;
}

esp_err_t blinker_aligenie_print(cJSON* param)
{
    esp_err_t err = ESP_FAIL;

    if (!data_from || data_from->type != BLINKER_DATA_FROM_ALIGENIE) {
        return err;
    }

    err = blinker_va_print(param, data_from);

    return err;
}

esp_err_t blinker_aligenie_handler_register(blinker_va_cb_t cb)
{
    if (va_ali == NULL) {
        return ESP_FAIL;
    }

    va_ali->cb = cb;

    return ESP_OK;
}

static void blinker_aligenie_init(void)
{
    if (va_ali == NULL) {
        va_ali = calloc(1, sizeof(blinker_va_data_t));

        if (!va_ali) {
            BLINKER_FREE(va_ali);
            return;
        }
    }

    blinker_va_param_add(va_ali, BLINKER_CMD_POWER_STATE,       BLINKER_PARAM_POWER_STATE,     BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_ali, BLINKER_CMD_COLOR_,            BLINKER_PARAM_COLOR,           BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_ali, BLINKER_CMD_MODE,              BLINKER_PARAM_MODE,            BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_ali, BLINKER_CMD_COLOR_TEMP,        BLINKER_PARAM_COLOR_TEMP,      BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_COLOR_TEMP_UP,     BLINKER_PARAM_COLOR_TEMP_UP,   BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_COLOR_TEMP_DOWN,   BLINKER_PARAM_COLOR_TEMP_DOWN, BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_BRIGHTNESS,        BLINKER_PARAM_BRIGHTNESS,      BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_BRIGHTNESS_UP,     BLINKER_PARAM_BRIGHTNESS_UP,   BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_BRIGHTNESS_DOWN,   BLINKER_PARAM_BRIGHTNESS_DOWN, BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_HSTATE,            BLINKER_PARAM_HSWING,          BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_ali, BLINKER_CMD_VSTATE,            BLINKER_PARAM_VSWING,          BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_ali, BLINKER_CMD_TEMP,              BLINKER_PARAM_TEMP,            BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_TEMP_UP,           BLINKER_PARAM_TEMP_UP,         BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_TEMP_DOWN,         BLINKER_PARAM_TEMP_DOWN,       BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_HUMI,              BLINKER_PARAM_HUMI,            BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_LEVEL,             BLINKER_PARAM_LEVEL,           BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_LEVEL_UP,          BLINKER_PARAM_LEVEL_UP,        BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_LEVEL_DOWN,        BLINKER_PARAM_LEVEL_DOWN,      BLINKER_VAL_TYPE_INT);
}

/////////////////////////////////////////////////////////////////////////

esp_err_t blinker_dueros_power_state(cJSON *param, const char *state)
{
    blinker_device_print_string(param, BLINKER_CMD_POWER_STATE, state);

    return ESP_OK;
}

esp_err_t blinker_dueros_color(cJSON *param, const char *color)
{
    blinker_device_print_string(param, BLINKER_CMD_COLOR_, color);

    return ESP_OK;
}

esp_err_t blinker_dueros_mode(cJSON *param, const char *mode)
{
    char *mode_raw = NULL;
    asprintf(&mode_raw, "[\"\",\"%s\"]", mode);
    if (mode_raw) {
        blinker_device_print_raw(param, BLINKER_CMD_MODE, mode_raw);
    }
    BLINKER_FREE(mode_raw);

    return ESP_OK;
}

esp_err_t blinker_dueros_color_temp(cJSON *param, const int color_temp)
{
    char *color_temp_raw = NULL;
    asprintf(&color_temp_raw, "[\"\",%d]", color_temp);
    if (color_temp_raw) {
        blinker_device_print_raw(param, BLINKER_CMD_COLOR_TEMP, color_temp_raw);
    }
    BLINKER_FREE(color_temp_raw);

    return ESP_OK;
}

esp_err_t blinker_dueros_brightness(cJSON *param, const int bright)
{
    blinker_device_print_number(param, BLINKER_CMD_BRIGHTNESS, bright);

    return ESP_OK;
}

esp_err_t blinker_dueros_temp(cJSON *param, const int temp)
{
    blinker_device_print_number(param, BLINKER_CMD_TEMP, temp);

    return ESP_OK;
}

esp_err_t blinker_dueros_humi(cJSON *param, const int humi)
{
    blinker_device_print_number(param, BLINKER_CMD_TEMP, humi);

    return ESP_OK;
}

esp_err_t blinker_dueros_pm25(cJSON *param, const int pm25)
{
    blinker_device_print_number(param, BLINKER_CMD_PM25, pm25);

    return ESP_OK;
}

esp_err_t blinker_dueros_pm10(cJSON *param, const int pm10)
{
    blinker_device_print_number(param, BLINKER_CMD_PM10, pm10);

    return ESP_OK;
}

esp_err_t blinker_dueros_co2(cJSON *param, const int co2)
{
    blinker_device_print_number(param, BLINKER_CMD_CO2, co2);

    return ESP_OK;
}

esp_err_t blinker_dueros_aqi(cJSON *param, const int aqi)
{
    blinker_device_print_number(param, BLINKER_CMD_AQI, aqi);

    return ESP_OK;
}

esp_err_t blinker_dueros_level(cJSON *param, const int level)
{
    blinker_device_print_number(param, BLINKER_CMD_LEVEL, level);

    return ESP_OK;
}

esp_err_t blinker_dueros_time(cJSON *param, const int time)
{
    blinker_device_print_number(param, BLINKER_CMD_TIME, time);

    return ESP_OK;
}

esp_err_t blinker_dueros_print(cJSON* param)
{
    esp_err_t err = ESP_FAIL;

    if (!data_from || data_from->type != BLINKER_DATA_FROM_DUEROS) {
        return err;
    }

    err = blinker_va_print(param, data_from);

    return err;
}

esp_err_t blinker_dueros_handler_register(blinker_va_cb_t cb)
{
    if (va_duer == NULL) {
        return ESP_FAIL;
    }

    va_duer->cb = cb;

    return ESP_OK;
}

static void blinker_dueros_init(void)
{
    if (va_duer == NULL) {
        va_duer = calloc(1, sizeof(blinker_va_data_t));

        if (!va_duer) {
            BLINKER_FREE(va_duer);
            return;
        }
    }

    blinker_va_param_add(va_duer, BLINKER_CMD_POWER_STATE,       BLINKER_PARAM_POWER_STATE,     BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_duer, BLINKER_CMD_COLOR_,            BLINKER_PARAM_COLOR,           BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_duer, BLINKER_CMD_MODE,              BLINKER_PARAM_MODE,            BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_duer, BLINKER_CMD_COLOR_TEMP,        BLINKER_PARAM_COLOR_TEMP,      BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_COLOR_TEMP_UP,     BLINKER_PARAM_COLOR_TEMP_UP,   BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_COLOR_TEMP_DOWN,   BLINKER_PARAM_COLOR_TEMP_DOWN, BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_BRIGHTNESS,        BLINKER_PARAM_BRIGHTNESS,      BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_BRIGHTNESS_UP,     BLINKER_PARAM_BRIGHTNESS_UP,   BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_BRIGHTNESS_DOWN,   BLINKER_PARAM_BRIGHTNESS_DOWN, BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_TEMP,              BLINKER_PARAM_TEMP,            BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_TEMP_UP,           BLINKER_PARAM_TEMP_UP,         BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_TEMP_DOWN,         BLINKER_PARAM_TEMP_DOWN,       BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_HUMI,              BLINKER_PARAM_HUMI,            BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_LEVEL,             BLINKER_PARAM_LEVEL,           BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_LEVEL_UP,          BLINKER_PARAM_LEVEL_UP,        BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_LEVEL_DOWN,        BLINKER_PARAM_LEVEL_DOWN,      BLINKER_VAL_TYPE_INT);
}

/////////////////////////////////////////////////////////////////////////

esp_err_t blinker_miot_power_state(cJSON *param, const char *state)
{
    blinker_device_print_string(param, BLINKER_CMD_POWER_STATE, state);

    return ESP_OK;
}

esp_err_t blinker_miot_color(cJSON *param, const int color)
{
    blinker_device_print_number(param, BLINKER_CMD_COLOR_, color);

    return ESP_OK;
}

esp_err_t blinker_miot_mode(cJSON *param, const int mode)
{
    blinker_device_print_number(param, BLINKER_CMD_MODE, mode);

    return ESP_OK;
}

esp_err_t blinker_miot_mode_state(cJSON *param, const char *mode, const char *state)
{
    blinker_device_print_string(param, mode, state);

    return ESP_OK;
}

esp_err_t blinker_miot_color_temp(cJSON *param, const int color_temp)
{
    blinker_device_print_number(param, BLINKER_CMD_COLOR_, color_temp);

    return ESP_OK;
}

esp_err_t blinker_miot_brightness(cJSON *param, const int bright)
{
    blinker_device_print_number(param, BLINKER_CMD_BRIGHTNESS, bright);

    return ESP_OK;
}

esp_err_t blinker_miot_temp(cJSON *param, const int temp)
{
    blinker_device_print_number(param, BLINKER_CMD_TEMP, temp);

    return ESP_OK;
}

esp_err_t blinker_miot_humi(cJSON *param, const int humi)
{
    blinker_device_print_number(param, BLINKER_CMD_TEMP, humi);

    return ESP_OK;
}

esp_err_t blinker_miot_pm25(cJSON *param, const int pm25)
{
    blinker_device_print_number(param, BLINKER_CMD_PM25, pm25);

    return ESP_OK;
}

esp_err_t blinker_miot_co2(cJSON *param, const int co2)
{
    blinker_device_print_number(param, BLINKER_CMD_CO2, co2);

    return ESP_OK;
}

esp_err_t blinker_miot_level(cJSON *param, const int level)
{
    blinker_device_print_number(param, BLINKER_CMD_LEVEL, level);

    return ESP_OK;
}

esp_err_t blinker_miot_hswing(cJSON *param, const char *state)
{
    blinker_device_print_string(param, BLINKER_CMD_HSTATE, state);

    return ESP_OK;
}

esp_err_t blinker_miot_vswing(cJSON *param, const char *state)
{
    blinker_device_print_string(param, BLINKER_CMD_VSTATE, state);

    return ESP_OK;
}

esp_err_t blinker_miot_print(cJSON* param)
{
    esp_err_t err = ESP_FAIL;

    if (!data_from || data_from->type != BLINKER_DATA_FROM_MIOT) {
        return err;
    }

    err = blinker_va_print(param, data_from);

    return err;
}

esp_err_t blinker_miot_handler_register(blinker_va_cb_t cb)
{
    if (va_miot == NULL) {
        return ESP_FAIL;
    }

    va_miot->cb = cb;

    return ESP_OK;
}

static void blinker_miot_init(void)
{
    if (va_miot == NULL) {
        va_miot = calloc(1, sizeof(blinker_va_data_t));

        if (!va_miot) {
            BLINKER_FREE(va_miot);
            return;
        }
    }

    blinker_va_param_add(va_miot, BLINKER_CMD_POWER_STATE, BLINKER_PARAM_POWER_STATE, BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_COLOR_,      BLINKER_PARAM_COLOR,       BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_miot, BLINKER_CMD_MODE,        BLINKER_PARAM_MODE,        BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_miot, BLINKER_CMD_COLOR_TEMP,  BLINKER_PARAM_COLOR_TEMP,  BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_miot, BLINKER_CMD_BRIGHTNESS,  BLINKER_PARAM_BRIGHTNESS,  BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_miot, BLINKER_CMD_ECO,         BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_ANION,       BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_HEATER,      BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_DRYER,       BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_ANION,       BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_SOFT,        BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_UV,          BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_UNSB,        BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_HSTATE,      BLINKER_PARAM_HSWING,      BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_VSTATE,      BLINKER_PARAM_VSWING,      BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_TEMP,        BLINKER_PARAM_TEMP,        BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_miot, BLINKER_CMD_HUMI,        BLINKER_PARAM_HUMI,        BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_miot, BLINKER_CMD_LEVEL,       BLINKER_PARAM_LEVEL,       BLINKER_VAL_TYPE_INT);
}

/////////////////////////////////////////////////////////////////////////

esp_err_t blinker_widget_switch(cJSON *param, const char *state)
{
    blinker_device_print_string(param, BLINKER_CMD_SWITCH, state);

    return ESP_OK;
}

esp_err_t blinker_widget_icon(cJSON *param, const char *icon)
{
    blinker_device_print_string(param, BLINKER_CMD_ICON, icon);

    return ESP_OK;
}

esp_err_t blinker_widget_color(cJSON *param, const char *color)
{
    blinker_device_print_string(param, BLINKER_CMD_COLOR, color);

    return ESP_OK;
}

esp_err_t blinker_widget_text(cJSON *param, const char *text)
{
    blinker_device_print_string(param, BLINKER_CMD_TEXT, text);

    return ESP_OK;
}

esp_err_t blinker_widget_text1(cJSON *param, const char *text)
{
    blinker_device_print_string(param, BLINKER_CMD_TEXT1, text);

    return ESP_OK;
}

esp_err_t blinker_widget_text_color(cJSON *param, const char *color)
{
    blinker_device_print_string(param, BLINKER_CMD_TEXT_COLOR, color);

    return ESP_OK;
}

esp_err_t blinker_widget_value_number(cJSON *param, const double value)
{
    blinker_device_print_number(param, BLINKER_CMD_VALUE, value);

    return ESP_OK;
}

esp_err_t blinker_widget_value_string(cJSON *param, const char *value)
{
    blinker_device_print_string(param, BLINKER_CMD_VALUE, value);

    return ESP_OK;
}

esp_err_t blinker_widget_unit(cJSON *param, const char *uint)
{
    blinker_device_print_string(param, BLINKER_CMD_UNIT, uint);

    return ESP_OK;
}

esp_err_t blinker_widget_image(cJSON *param, const int num)
{
    blinker_device_print_number(param, BLINKER_CMD_IMAGE, num);

    return ESP_OK;
}

esp_err_t blinker_widget_rgb_print(const char *key, const int r, const int g, const int b, const int w)
{
    char rgb_raw[24] = {0};
    sprintf(rgb_raw, "[%d,%d,%d,%d]", r, g, b, w);
    blinker_device_print(key, rgb_raw, true);

    return ESP_OK;
}

esp_err_t blinker_widget_tab_print(const char *key, const int tab0, const int tab1, const int tab2, const int tab3, const int tab4)
{
    char tab_data[6] = {0};
    sprintf(tab_data, "%d%d%d%d%d", tab0, tab1, tab2, tab3, tab4);
    blinker_device_print(key, tab_data, false);

    return ESP_OK;
}

esp_err_t blinker_widget_print(const char *key, cJSON *param)
{
    char *raw_data = cJSON_PrintUnformatted(param);

    blinker_device_print(key, raw_data, true);
    BLINKER_FREE(raw_data);
    
    return ESP_OK;
}

esp_err_t blinker_widget_add(const char *key, blinker_widget_type_t type, blinker_widget_cb_t cb)
{
    blinker_widget_data_t *widget = calloc(1, sizeof(blinker_widget_data_t));

    if (!widget) {
        BLINKER_FREE(widget);
        return ESP_FAIL;
    }

    widget->key  = strdup(key);
    widget->type = type;
    widget->cb   = cb;

    blinker_widget_data_t *last = SLIST_FIRST(&blinker_widget_list);

    if (last == NULL) {
        SLIST_INSERT_HEAD(&blinker_widget_list, widget, next);
    } else {
        SLIST_INSERT_AFTER(last, widget, next);
    }

    return ESP_OK;
}

static void blinker_widget_parse(cJSON *data)
{
    blinker_widget_data_t *widget;
    SLIST_FOREACH(widget, &blinker_widget_list, next) {
        if (cJSON_HasObjectItem(data, widget->key)) {
            blinker_widget_param_val_t param_val = {0};
            cJSON *widget_param = cJSON_GetObjectItem(data, widget->key);

            switch (widget->type)
            {
                case BLINKER_BUTTON:
                    if (cJSON_IsString(widget_param)) {
                        param_val.s = widget_param->valuestring;
                        widget->cb(&param_val);
                    }
                    break;

                case BLINKER_IMAGE:
                    if (cJSON_IsString(widget_param)) {
                        param_val.i = atoi(widget_param->valuestring);
                    } else {
                        param_val.i = widget_param->valueint;
                    }
                    widget->cb(&param_val);
                    break;

                case BLINKER_JOYSTICK:
                    if (cJSON_IsArray(widget_param)) {
                        param_val.array[0] = cJSON_GetArrayItem(widget_param, 0)->valueint;
                        param_val.array[1] = cJSON_GetArrayItem(widget_param, 1)->valueint;
                        widget->cb(&param_val);
                    }
                    break;

                case BLINKER_RGB:
                    if (cJSON_IsArray(widget_param)) {
                        param_val.array[0] = cJSON_GetArrayItem(widget_param, 0)->valueint;
                        param_val.array[1] = cJSON_GetArrayItem(widget_param, 1)->valueint;
                        param_val.array[2] = cJSON_GetArrayItem(widget_param, 2)->valueint;
                        param_val.array[3] = cJSON_GetArrayItem(widget_param, 3)->valueint;
                        widget->cb(&param_val);
                    }
                    break;

                case BLINKER_SLIDER:
                    if (cJSON_IsNumber(widget_param)) {
                        param_val.i = widget_param->valueint;
                        widget->cb(&param_val);
                    }
                    break;

                case BLINKER_TAB:
                    if (cJSON_IsString(widget_param) && strlen(widget_param->valuestring) == 5) {
                        param_val.i = (widget_param->valuestring[4] - '0') << 4 | \
                                      (widget_param->valuestring[3] - '0') << 3 | \
                                      (widget_param->valuestring[2] - '0') << 2 | \
                                      (widget_param->valuestring[1] - '0') << 1 | \
                                      (widget_param->valuestring[0] - '0');
                        widget->cb(&param_val);
                    }
                    break;

                default:
                    break;
            }
            cJSON_Delete(widget_param);
        }
    }
}

/////////////////////////////////////////////////////////////////////////

static void blinker_device_print_to_user(const char *data)
{
    char *pub_topic = NULL;
    if (blinker_mqtt_broker() == BLINKER_BROKER_ALIYUN) {
        asprintf(&pub_topic, "/%s/%s/s",
                blinker_mqtt_product_key(),
                blinker_mqtt_devicename());
    } else if (blinker_mqtt_broker() == BLINKER_BROKER_BLINKER) {
        asprintf(&pub_topic, "/device/%s/s",
                blinker_mqtt_devicename());
    }

    blinker_mqtt_publish(pub_topic, data, strlen(data));

    BLINKER_FREE(pub_topic);
}

static void blinker_device_auto_format_task(void *arg)
{
    blinker_auto_format_queue_t *format_queue = NULL;
    char *format_data = NULL;

    for(;;) {
        if (xQueueReceive(auto_format_queue, &format_queue, pdMS_TO_TICKS(BLINKER_FORMAT_QUEUE_TIME)) == pdFALSE) {
            if (format_data != NULL) {
                cJSON *format_print = cJSON_CreateObject();
                
                if (data_from && data_from->type == BLINKER_DATA_FROM_USER) {
                    cJSON_AddStringToObject(format_print, BLINKER_CMD_TO_DEVICE, data_from->uuid);
                } else {
                    cJSON_AddStringToObject(format_print, BLINKER_CMD_TO_DEVICE, blinker_mqtt_uuid());
                }
                cJSON_AddStringToObject(format_print, BLINKER_CMD_FROM_DEVICE, blinker_mqtt_devicename());
                cJSON_AddStringToObject(format_print, BLINKER_CMD_DEVICE_TYPE, BLINKER_CMD_OWN_APP);
                cJSON_AddRawToObject(format_print, BLINKER_CMD_DATA, format_data);
                cJSON_PrintPreallocated(format_print, format_data, 1024, false);
                cJSON_Delete(format_print);

                blinker_device_print_to_user(format_data);

                BLINKER_FREE(format_data);

                blinker_log_print_heap();
            }
            continue;
        }

        cJSON *format_root = NULL;
        
        if (format_data == NULL) {
            format_root = cJSON_CreateObject();
            format_data = calloc(BLINKER_FORMAT_DATA_SIZE, sizeof(char));
        } else {
            format_root = cJSON_Parse(format_data);
        }

        if (format_data != NULL) {
            if (cJSON_HasObjectItem(format_root, format_queue->key)) {
                cJSON_DeleteItemFromObject(format_root, format_queue->key);
            }

            if (format_queue->is_raw) {
                cJSON_AddRawToObject(format_root, format_queue->key, format_queue->value);
            } else {
                cJSON_AddStringToObject(format_root, format_queue->key, format_queue->value);
            }

            cJSON_PrintPreallocated(format_root, format_data, BLINKER_FORMAT_DATA_SIZE, false);
            cJSON_Delete(format_root);
        } else {
            BLINKER_FREE(format_data);
            cJSON_Delete(format_root);
        }

        BLINKER_FREE(format_queue->key);
        BLINKER_FREE(format_queue->value);
        BLINKER_FREE(format_queue);
    }
}

static void blinker_device_heart_beat(cJSON *data)
{
    if (strncmp(BLINKER_CMD_STATE, 
        cJSON_GetObjectItem(data, BLINKER_CMD_GET)->valuestring, 
        strlen(BLINKER_CMD_STATE)) == 0) {
        blinker_device_print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE, false);
        blinker_device_print(BLINKER_CMD_VERSION, CONFIG_BLINKER_FIRMWARE_VERSION, false);
    }
}

static esp_err_t blinker_device_register(blinker_mqtt_config_t *mqtt_cfg)
{
    esp_err_t err = ESP_FAIL;

    esp_http_client_config_t config = {
        .url = "http://iot.diandeng.tech/",
    };

    esp_http_client_handle_t client = blinker_http_init(&config);
    
    char *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE, sizeof(char));
    char *url = NULL;

    asprintf(&url, "http://iot.diandeng.tech/api/v1/user/device/diy/auth?authKey=%s&protocol=%s&version=%s%s%s%s",
            CONFIG_BLINKER_AUTH_KEY,
            BLINKER_PROTOCOL_MQTT,
            CONFIG_BLINKER_FIRMWARE_VERSION,
            CONFIG_BLINKER_ALIGENIE_TYPE,
            CONFIG_BLINKER_DUEROS_TYPE,
            CONFIG_BLINKER_MIOT_TYPE);

    ESP_LOGI(TAG, "http connect to: %s", url);

    err = blinker_http_set_url(client, url);
    err = blinker_http_get(client);
    err = blinker_http_read_response(client, payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE);
    err = blinker_http_close(client);

    BLINKER_FREE(url);

    ESP_LOGI(TAG, "read response: %s", payload);

    cJSON *root = cJSON_Parse(payload);

    if (!root) {
        BLINKER_FREE(payload);
        cJSON_Delete(root);
        return ESP_FAIL;
    } else {
        cJSON *detail = cJSON_GetObjectItem(root, BLINKER_CMD_DETAIL);
        if (detail != NULL && cJSON_HasObjectItem(detail, BLINKER_CMD_DEVICE_NAME)) {
            mqtt_cfg->client_id  = strdup(cJSON_GetObjectItem(detail, BLINKER_CMD_DEVICE_NAME)->valuestring);
            mqtt_cfg->username   = strdup(cJSON_GetObjectItem(detail, BLINKER_CMD_USER_NAME)->valuestring);
            mqtt_cfg->password   = strdup(cJSON_GetObjectItem(detail, BLINKER_CMD_PASS_WORD)->valuestring);
            mqtt_cfg->productkey = strdup(cJSON_GetObjectItem(detail, BLINKER_CMD_PRODUCT_KEY)->valuestring);
            mqtt_cfg->uuid       = strdup(cJSON_GetObjectItem(detail, BLINKER_CMD_UUID)->valuestring);
            mqtt_cfg->uri        = strdup(cJSON_GetObjectItem(detail, BLINKER_CMD_HOST)->valuestring);
            mqtt_cfg->authkey    = strdup(CONFIG_BLINKER_AUTH_KEY);
            mqtt_cfg->port       = atoi(cJSON_GetObjectItem(detail, BLINKER_CMD_PORT)->valuestring);

            if (strncmp(cJSON_GetObjectItem(detail, BLINKER_CMD_BROKER)->valuestring, BLINKER_CMD_ALIYUN, strlen(BLINKER_CMD_ALIYUN)) == 0) {
                mqtt_cfg->broker = BLINKER_BROKER_ALIYUN;
            } else {
                mqtt_cfg->broker = BLINKER_BROKER_BLINKER;
            }

#if defined(CONFIG_BLINKER_WITH_SSL)
            mqtt_cfg->devicename = strdup(cJSON_GetObjectItem(detail, BLINKER_CMD_DEVICE_NAME)->valuestring);
#else
            if (mqtt_cfg->broker == BLINKER_BROKER_ALIYUN) {
                mqtt_cfg->devicename = strdup(cJSON_GetObjectItem(detail, BLINKER_CMD_USER_NAME)->valuestring);
                mqtt_cfg->devicename = strtok(mqtt_cfg->devicename, "&");
            } else {
                mqtt_cfg->devicename = strdup(cJSON_GetObjectItem(detail, BLINKER_CMD_DEVICE_NAME)->valuestring);
            }
#endif

            ESP_LOGI(TAG, "devicename: %s", mqtt_cfg->devicename);
            ESP_LOGI(TAG, "client_id: %s",  mqtt_cfg->client_id);
            ESP_LOGI(TAG, "username: %s",   mqtt_cfg->username);
            ESP_LOGI(TAG, "password: %s",   mqtt_cfg->password);
            ESP_LOGI(TAG, "productkey: %s", mqtt_cfg->productkey);
            ESP_LOGI(TAG, "uuid: %s",       mqtt_cfg->uuid);
            ESP_LOGI(TAG, "uri: %s",        mqtt_cfg->uri);
            ESP_LOGI(TAG, "port: %d",       mqtt_cfg->port);

            err = ESP_OK;
        }
        cJSON_Delete(detail);
    }
    BLINKER_FREE(payload);
    cJSON_Delete(root);

    return err;
}

static blinker_data_from_type_t blinker_device_user_check(const char *user_name)
{
    ESP_LOGI(TAG, "user_name: %s, len: %d", user_name, strlen(user_name));

    if (strncmp(blinker_mqtt_uuid(), user_name, strlen(user_name)) == 0) {
        return BLINKER_DATA_FROM_USER;
    } else if (strncmp(BLINKER_CMD_ALIGENIE, user_name, strlen(user_name)) == 0) {
        return BLINKER_DATA_FROM_ALIGENIE;
    } else if (strncmp(BLINKER_CMD_DUEROS, user_name, strlen(user_name)) == 0) {
        return BLINKER_DATA_FROM_DUEROS;
    } else if (strncmp(BLINKER_CMD_MIOT, user_name, strlen(user_name)) == 0) {
        return BLINKER_DATA_FROM_MIOT;
    } else if (strncmp(BLINKER_CMD_SERVER_SENDER, user_name, strlen(user_name)) == 0) {
        return BLINKER_DATA_FROM_SERVER;
    } else {
        return BLINKER_DATA_FROM_USER;
    }
}

static void blinker_device_data_callback(const char *topic, size_t topic_len, void *payload, size_t payload_len)
{
    xSemaphoreTake(data_parse_mutex, portMAX_DELAY);

    cJSON *sub_data = cJSON_Parse(payload);

    if (sub_data == NULL) {
        cJSON_Delete(sub_data);
        return;
    }

    if (cJSON_HasObjectItem(sub_data, BLINKER_CMD_FROM_DEVICE))
    {
        if (data_from) {
            if (data_from->uuid) {
                BLINKER_FREE(data_from->uuid);
            }
            if (data_from->message_id) {
                BLINKER_FREE(data_from->message_id);
            }
            BLINKER_FREE(data_from);
        }

        data_from = calloc(1, sizeof(blinker_data_from_param_t));

        if (data_from) {
            data_from->uuid = strdup(cJSON_GetObjectItem(sub_data, BLINKER_CMD_FROM_DEVICE)->valuestring);

            cJSON *main_data = cJSON_GetObjectItem(sub_data, BLINKER_CMD_DATA);
            if (main_data == NULL) {
                cJSON_Delete(main_data);
                cJSON_Delete(sub_data);
                return;
            }

            data_from->type = blinker_device_user_check(data_from->uuid);

            if (data_from->type > BLINKER_DATA_FROM_USER) {
                if (blinker_mqtt_broker() == BLINKER_BROKER_ALIYUN) {
                    uint8_t id_len = topic_len - strlen("/sys///rrpc/request") - \
                                    strlen(blinker_mqtt_product_key()) - \
                                    strlen(blinker_mqtt_devicename());
                    data_from->message_id = calloc(id_len + 1, sizeof(char));
                    memcpy(data_from->message_id, topic + topic_len - id_len, id_len);
                } else if (blinker_mqtt_broker() == BLINKER_BROKER_BLINKER) {
                    if (cJSON_HasObjectItem(main_data, BLINKER_CMD_FROM)) {
                        BLINKER_FREE(data_from->uuid);
                        data_from->uuid = strdup(cJSON_GetObjectItem(main_data, BLINKER_CMD_FROM)->valuestring);
                        data_from->type = blinker_device_user_check(data_from->uuid);
                        data_from->message_id = strdup(cJSON_GetObjectItem(main_data, BLINKER_CMD_MESSAGE_ID)->valuestring);
                    }
                }
                
                ESP_LOGI(TAG, "message_id: %s", data_from->message_id);
            }

            switch (data_from->type)
            {
                case BLINKER_DATA_FROM_USER:
                    if (cJSON_HasObjectItem(main_data, BLINKER_CMD_GET)) {
                        blinker_device_heart_beat(main_data);
                    } else {
                        blinker_widget_parse(main_data);
                    }
                    break;

                case BLINKER_DATA_FROM_ALIGENIE:
                    if (va_ali != NULL) {
                        blinker_va_parse(main_data, va_ali);
                    }
                    break;

                case BLINKER_DATA_FROM_DUEROS:
                    if (va_duer != NULL) {
                        blinker_va_parse(main_data, va_duer);
                    }
                    break;

                case BLINKER_DATA_FROM_MIOT:
                    if (va_miot != NULL) {
                        blinker_va_parse(main_data, va_miot);
                    }
                    break;

                default:
                    break;
            }
        
            cJSON_Delete(main_data);
        }
    }

    cJSON_Delete(sub_data);

    xSemaphoreGive(data_parse_mutex);
}

static void blinker_http_heart_beat(void *timer)
{
    char *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, sizeof(char));
    char *url = NULL;

    asprintf(&url, "http://iot.diandeng.tech/api/v1/user/device/heartbeat?deviceName=%s&key=%s&heartbeat=%d",
            blinker_mqtt_devicename(),
            blinker_mqtt_authkey(),
            CONFIG_BLINKER_HTTP_HEART_BEAT_TIME_INTERVAL * BLINKER_MIN_TO_S);

    if (!url || !payload) {
        BLINKER_FREE(payload);
        BLINKER_FREE(url);
        return;
    }

    blinker_device_http(url, "", payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, BLINKER_HTTP_HEART_BEAT);

    BLINKER_FREE(payload);
    BLINKER_FREE(url);
}

static void blinker_button_reset_cb(void *arg)
{
    blinker_storage_erase(CONFIG_BLINKER_NVS_NAMESPACE);
    esp_restart();
}

static esp_err_t blinker_button_reset_init(void)
{
    button_handle_t btn_handle = iot_button_create(CONFIG_BUTTON_IO_NUM, CONFIG_BUTTON_ACTIVE_LEVEL);
    
    esp_err_t err = iot_button_add_custom_cb(btn_handle, CONFIG_BUTTON_RESET_INTERVAL_TIME, blinker_button_reset_cb, NULL);

    return err;
}

static esp_err_t blinker_reboot_reset_check(void)
{
    if (blinker_reboot_unbroken_count() >= CONFIG_BLINKER_REBOOT_UNBROKEN_FALLBACK_COUNT_RESET) {
        blinker_storage_erase(CONFIG_BLINKER_NVS_NAMESPACE);
    }

    return ESP_OK;
}

// static esp_err_t blinker_device_mqtt_init(blinker_mqtt_config_t *config)
// static esp_err_t blinker_device_mqtt_init(void)
// {
//     esp_err_t err = ESP_FAIL;

//     // err = blinker_mqtt_init(config);
//     err = blinker_mqtt_connect();

//     char *sub_topic = NULL;
//     if (blinker_mqtt_broker() == BLINKER_BROKER_ALIYUN) {
//         asprintf(&sub_topic, "/%s/%s/r",
//                 blinker_mqtt_product_key(),
//                 blinker_mqtt_devicename());
//     } else if (blinker_mqtt_broker() == BLINKER_BROKER_BLINKER) {
//         asprintf(&sub_topic, "/device/%s/r",
//                 blinker_mqtt_devicename());
//     }

//     if (sub_topic != NULL) {
//         blinker_mqtt_subscribe(sub_topic, blinker_device_data_callback);
//     }
//     BLINKER_FREE(sub_topic);

//     if (blinker_mqtt_broker() == BLINKER_BROKER_ALIYUN) {
//         asprintf(&sub_topic, "/sys/%s/%s/rrpc/request/+",
//                 blinker_mqtt_product_key(),
//                 blinker_mqtt_devicename());

//         if (sub_topic != NULL) {
//             blinker_mqtt_subscribe(sub_topic, blinker_device_data_callback);
//         }
//         BLINKER_FREE(sub_topic);
//     }

//     return err;
// }

// static void event_handler(void *arg, esp_event_base_t event_base,
//                         int32_t event_id, void *event_data)
// {
//     switch (event_id)
//     {
//         case BLINKER_EVENT_WIFI_INIT:
//             /* code */
//             break;
        
//         default:
//             break;
//     }
// }

static const int REGISTERED_BIT = BIT0;
static EventGroupHandle_t b_register_group = NULL;

static void blinker_device_register_task(void *arg)
{
    esp_err_t err = ESP_FAIL;
    blinker_mqtt_config_t mqtt_config = {0};

    for(;;) {
        err = blinker_device_register(&mqtt_config);

        if (err != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(BLINKER_REGISTER_INTERVAL));
            continue;
        }

        err = blinker_mdns_init(mqtt_config.devicename);
        err = blinker_mqtt_init(&mqtt_config);

        if (err == ESP_OK) {
            break;
        }
    }

    xEventGroupSetBits(b_register_group, REGISTERED_BIT);

    vTaskDelete(NULL);
}

static esp_err_t blinker_device_mqtt_init(void)
{
    esp_err_t err = ESP_FAIL;

    b_register_group = xEventGroupCreate();

    xTaskCreate(blinker_device_register_task, 
                "device_register",
                2 * 1024,
                NULL,
                3,
                NULL);

    xEventGroupWaitBits(b_register_group, REGISTERED_BIT,
                        pdFALSE,
                        pdFALSE,
                        portMAX_DELAY);

    err = blinker_mqtt_connect();

    char *sub_topic = NULL;
    if (blinker_mqtt_broker() == BLINKER_BROKER_ALIYUN) {
        asprintf(&sub_topic, "/%s/%s/r",
                blinker_mqtt_product_key(),
                blinker_mqtt_devicename());
    } else if (blinker_mqtt_broker() == BLINKER_BROKER_BLINKER) {
        asprintf(&sub_topic, "/device/%s/r",
                blinker_mqtt_devicename());
    }

    if (sub_topic != NULL) {
        blinker_mqtt_subscribe(sub_topic, blinker_device_data_callback);
    }
    BLINKER_FREE(sub_topic);

    if (blinker_mqtt_broker() == BLINKER_BROKER_ALIYUN) {
        asprintf(&sub_topic, "/sys/%s/%s/rrpc/request/+",
                blinker_mqtt_product_key(),
                blinker_mqtt_devicename());

        if (sub_topic != NULL) {
            blinker_mqtt_subscribe(sub_topic, blinker_device_data_callback);
        }
        BLINKER_FREE(sub_topic);
    }

    return err;
}

esp_err_t blinker_init(void)
{
    esp_err_t err = ESP_FAIL;

    // blinker_mqtt_config_t mqtt_config = {0};

    data_parse_mutex = xSemaphoreCreateMutex();

#if defined(CONFIG_REBOOT_RESET_TYPE)
    err = blinker_reboot_reset_check();
#elif defined(CONFIG_BUTTON_RESET_TYPE)
    err = blinker_button_reset_init();
#endif
    err = blinker_wifi_init();

    blinker_timesync_start();
    
    err = blinker_device_mqtt_init();
    
#ifndef CONFIG_BLINKER_ALIGENIE_NONE
    blinker_aligenie_init();
#endif
#ifndef CONFIG_BLINKER_DUEROS_NONE
    blinker_dueros_init();
#endif
#ifndef CONFIG_BLINKER_MIOT_NONE
    blinker_miot_init();
#endif

    blinker_websocker_server_init();

    auto_format_queue = xQueueCreate(10, sizeof(blinker_auto_format_queue_t));
    xTaskCreate(blinker_device_auto_format_task, 
                "device_auto_format",
                2 * 1024,
                NULL,
                3,
                NULL);

    TimerHandle_t b_http_heart_beat_timer = xTimerCreate("http_heart_beat",
                                                        pdMS_TO_TICKS(CONFIG_BLINKER_HTTP_HEART_BEAT_TIME_INTERVAL*BLINKER_MIN_TO_MS),
                                                        true, 
                                                        NULL, 
                                                        blinker_http_heart_beat);
    xTimerStart(b_http_heart_beat_timer, 0);

    return err;
}
