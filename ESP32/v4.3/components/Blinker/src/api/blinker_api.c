#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "mbedtls/base64.h"

#include "blinker_api.h"
#include "blinker_button.h"
#include "blinker_http.h"
#include "blinker_mqtt.h"
#include "blinker_prov_apconfig.h"
#include "blinker_prov_smartconfig.h"
#include "blinker_storage.h"
#include "blinker_wifi.h"
#include "blinker_ws.h"

typedef struct blinker_widget_data {
    char *key;
    blinker_widget_type_t type;
    blinker_widget_cb_t   cb;
    SLIST_ENTRY(blinker_widget_data) next;
} blinker_widget_data_t;

typedef enum {
    BLINKER_VAL_TYPE_INT = 0,
    BLINKER_VAL_TYPE_STRING,
    BLINKER_VAL_TYPE_STRING_STRING,
} blinker_va_param_val_type_t;

typedef struct blinker_va_param {
    char *key;
    blinker_va_param_type_t     type;
    blinker_va_param_val_type_t val_type;
    SLIST_ENTRY(blinker_va_param) next;
} blinker_va_param_t;

typedef struct {
    blinker_va_cb_t cb;
    SLIST_HEAD(va_list_, blinker_va_param) va_param_list;
} blinker_va_data_t;

typedef struct blinker_timeslot_param {
//     char    *key;
    char *data;
    SLIST_ENTRY(blinker_timeslot_param) next;
} blinker_timeslot_data_param_t;

typedef struct {
    uint8_t  num;
    uint8_t  times;
    uint8_t  times_count;
    uint16_t interval;
    blinker_json_cb_t cb;
} blinker_timeslot_param_t;

typedef enum {
    BLINKER_DATA_FROM_WS = 0,
    BLINKER_DATA_FROM_USER,
    BLINKER_DATA_FROM_ALIGENIE,
    BLINKER_DATA_FROM_DUEROS,
    BLINKER_DATA_FROM_MIOT,
    BLINKER_DATA_FROM_SERVER,
} blinker_data_from_type_t;

typedef enum {
    BLINKER_EVENT_WIFI_INIT = 0,
} blinker_event_t;

typedef enum {
    BLINKER_HTTP_METHOD_GET = 0,
    BLINKER_HTTP_METHOD_POST,
} blinker_device_http_method_t;


typedef struct {
    char *uuid;
    char *message_id;
    async_resp_arg_t *resp_arg;
    blinker_data_from_type_t type;
} blinker_data_from_param_t;

typedef struct {
    char *key;
    char *value;
    bool is_raw;
} blinker_auto_format_queue_t;

#if defined CONFIG_BLINKER_PRO_ESP
typedef enum {
    // BLINKER_PRO_DEVICE_AUTH = 0,
    BLINKER_PRO_DEVICE_CHECK = 0,
    BLINKER_PRO_DEVICE_WAIT_REGISTER,
    BLINKER_PRO_DEVICE_REGISTER
} blinker_pro_device_status_t;

typedef struct {
    const char *uuid;
} blinker_pro_esp_param_t;

#define BLINKER_PRO_ESP_CHECK_KEY           "pro_check"
#endif

static const char* TAG                      = "blinker_api";
static xQueueHandle auto_format_queue       = NULL;
static SemaphoreHandle_t data_parse_mutex   = NULL;
static blinker_data_from_param_t *data_from = NULL;
static blinker_va_data_t *va_ali            = NULL;
static blinker_va_data_t *va_duer           = NULL;
static blinker_va_data_t *va_miot           = NULL;
static const int REGISTERED_BIT             = BIT0;
static EventGroupHandle_t b_register_group  = NULL;
#if defined CONFIG_BLINKER_AP_CONFIG
static const int APCONFIG_DONE_BIT          = BIT0;
static EventGroupHandle_t b_apconfig_group  = NULL;
#endif
static blinker_void_cb_t heart_beat_cb      = NULL;
static blinker_data_cb_t extra_data_cb      = NULL;
static blinker_timeslot_param_t *timeslot_p = NULL;

#if defined CONFIG_BLINKER_PRO_ESP
static bool pro_device_wait_register        = false;
static bool pro_device_get_register         = false;
#endif

static SLIST_HEAD(widget_list_, blinker_widget_data) blinker_widget_list;
static SLIST_HEAD(timeslot_data_list_, blinker_timeslot_param) blinker_timeslot_list;
static void blinker_device_print(const char *key, const char *value, const bool is_raw);
static esp_err_t blinker_device_http(const char *url, const char *msg, char *payload, const int payload_len, const blinker_device_http_method_t type);

/////////////////////////////////////////////////////////////////////////

static void blinker_timeslot_timer_callback(void *timer)
{
    static bool one_loop = false;
    if (timeslot_p->cb) {
        cJSON *data_param = cJSON_CreateObject();
        cJSON_AddNumberToObject(data_param, "date", blinker_time());
        timeslot_p->cb(data_param);
        timeslot_p->times_count++;

        char *now_data = cJSON_PrintUnformatted(data_param);
        
        if (timeslot_p->times_count == timeslot_p->times) {
            timeslot_p->times_count = 0;

            if (!one_loop) one_loop = true;

            ESP_LOGI(TAG, "times trigged, need format");

            blinker_timeslot_data_param_t *ts_data;
            cJSON *data_array = cJSON_CreateArray();

            cJSON_AddItemToArray(data_array, cJSON_CreateRaw(now_data));
            BLINKER_FREE(now_data);
            cJSON_Delete(data_param);

            SLIST_FOREACH(ts_data, &blinker_timeslot_list, next) {
                if (ts_data->data) {
                    cJSON_AddItemToArray(data_array, cJSON_CreateRaw(ts_data->data));
                    BLINKER_FREE(ts_data->data);
                }
            }
            
            char *format_data = cJSON_PrintUnformatted(data_array);
            ESP_LOGI(TAG, "format data: %s", format_data);

            char *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, sizeof(char));
            char *post_msg = NULL;
            char *url = NULL;

            if (!payload) {
                BLINKER_FREE(format_data);
                cJSON_Delete(data_array);
                return;
            }

            asprintf(&post_msg, "{\"device\":\"%s\",\"key\":\"%s\",\"data\":%s}",
                    blinker_mqtt_devicename(),
                    blinker_mqtt_authkey(),
                    format_data);

            if (!post_msg) {
                BLINKER_FREE(payload);
                BLINKER_FREE(format_data);
                cJSON_Delete(data_array);
                return;
            }

            asprintf(&url, "%s://%s/api/v1/storage/ts",
                    BLINKER_PROTPCOL_HTTP,
                    "storage.diandeng.tech");

            if (!url) {
                BLINKER_FREE(payload);
                BLINKER_FREE(post_msg);
                BLINKER_FREE(format_data);
                cJSON_Delete(data_array);
                return;
            }

            blinker_device_http(url, post_msg, payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, BLINKER_HTTP_METHOD_POST);
            
            BLINKER_FREE(url);
            BLINKER_FREE(post_msg);
            BLINKER_FREE(payload);
            BLINKER_FREE(format_data);
            cJSON_Delete(data_array);
        } else {
            if (!one_loop) {
                blinker_timeslot_data_param_t *new_data = calloc(1, sizeof(blinker_timeslot_data_param_t));

                new_data->data = strdup(now_data);

                blinker_timeslot_data_param_t *last = SLIST_FIRST(&blinker_timeslot_list);
                if (last == NULL) {
                    SLIST_INSERT_HEAD(&blinker_timeslot_list, new_data, next);
                } else {
                    SLIST_INSERT_AFTER(last, new_data, next);
                }
            } else {
                blinker_timeslot_data_param_t *set_data;
                SLIST_FOREACH(set_data, &blinker_timeslot_list, next) {
                    if (!set_data->data) {
                        set_data->data = strdup(now_data);
                    }
                }
            }
            BLINKER_FREE(now_data);
            cJSON_Delete(data_param);
        }

        blinker_log_print_heap();
    }
}

esp_err_t blinker_timeslot_data(cJSON *param, const char *key, const double value)
{
    if (cJSON_HasObjectItem(param, key)) {
        cJSON_DeleteItemFromObject(param, key);
    }

    cJSON_AddNumberToObject(param, key, value);
    // blinker_timeslot_data_param_t *last = SLIST_FIRST(&blinker_timeslot_list);

    // if (last != NULL) {
    //     blinker_timeslot_data_param_t *ts_data;

    //     SLIST_FOREACH(ts_data, &blinker_timeslot_list, next) {
    //         if (!strcmp(key, ts_data->key)) {
    //             ts_data->data[timeslot_p->times_count] = data;
    //             return ESP_OK;
    //         }
    //     }
    // }

    // blinker_timeslot_data_param_t *new_data = calloc(1, sizeof(blinker_timeslot_data_param_t));

    // if (!new_data) {
    //     return ESP_FAIL;
    // }

    // new_data->key  = strdup(key);
    // new_data->data = calloc(timeslot_p->times, sizeof(double));
    // new_data->data[0] = data;

    // if (last == NULL) {
    //     SLIST_INSERT_HEAD(&blinker_timeslot_list, new_data, next);
    // } else {
    //     SLIST_INSERT_AFTER(last, new_data, next);
    // }

    return ESP_OK;
}

esp_err_t blinker_timeslot_data_init(const uint16_t interval, const uint8_t times, const blinker_json_cb_t cb)
{
    esp_err_t err = ESP_FAIL;

    if (timeslot_p == NULL && interval <= BLINKER_MAX_TIME_INTERVAL && \
        interval >= BLINKER_MIN_TIME_INTERVAL && times <= BLINKER_MAX_TIMES_COUNT && \
        times >= BLINKER_MIN_TIMES_COUNT) {
        timeslot_p = calloc(1, sizeof(blinker_timeslot_param_t));

        if (!timeslot_p) {
            return err;
        }

        timeslot_p->interval = interval;
        timeslot_p->times    = times;
        timeslot_p->cb       = cb;

        TimerHandle_t b_timeslot_timer = xTimerCreate("blinker_timeslot",
                                                    pdMS_TO_TICKS(interval * 1000UL),
                                                    true,
                                                    NULL,
                                                    blinker_timeslot_timer_callback);

        xTimerStart(b_timeslot_timer, 0);

        err = ESP_OK;
    }

    return err;
}

esp_err_t blinker_data_handler(const blinker_data_cb_t cb)
{
    if (extra_data_cb == NULL) {
        extra_data_cb = cb;
    }

    return ESP_OK;
}

esp_err_t blinker_heart_beat_handler(const blinker_void_cb_t cb)
{
    if (heart_beat_cb == NULL) {
        heart_beat_cb = cb;
    }

    return ESP_OK;
}

esp_err_t blinker_builtin_switch_handler(const blinker_widget_cb_t cb)
{
    blinker_widget_add(BLINKER_CMD_BUILTIN_SWITCH, BLINKER_SWITCH, cb);

    return ESP_OK;
}

esp_err_t blinker_builtin_switch_state(const char *state)
{
    blinker_device_print(BLINKER_CMD_BUILTIN_SWITCH, state, false);

    return ESP_OK;
}

esp_err_t blinker_weather(char **payload, const int city)
{
    esp_err_t err = ESP_FAIL;

    *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/2, sizeof(char));
    char *url = NULL;
    
    if (!*payload) {
        return err;
    }

    asprintf(&url, "%s://%s/api/v3/weather?deviceName=%s&key=%s&code=%d",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST,
            blinker_mqtt_devicename(),
            blinker_mqtt_token(),
            city);

    if (!url) {
        BLINKER_FREE(*payload);
        return err;
    }

    err = blinker_device_http(url, "", *payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/2, BLINKER_HTTP_METHOD_GET);

    BLINKER_FREE(url);

    return err;
}

esp_err_t blinker_weather_forecast(char **payload, const int city)
{
    esp_err_t err = ESP_FAIL;

    *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/2, sizeof(char));
    char *url = NULL;

    if (!*payload) {
        return err;
    }

    asprintf(&url, "%s://%s/api/v3/forecast?deviceName=%s&key=%s&code=%d",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST,
            blinker_mqtt_devicename(),
            blinker_mqtt_token(),
            city);

    if (!url) {
        BLINKER_FREE(*payload);
        return err;
    }

    err = blinker_device_http(url, "", *payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/2, BLINKER_HTTP_METHOD_GET);

    BLINKER_FREE(url);

    return err;
}

esp_err_t blinker_air(char **payload, const int city)
{
    esp_err_t err = ESP_FAIL;

    *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/2, sizeof(char));
    char *url = NULL;

    if (!*payload) {
        return err;
    }

    asprintf(&url, "%s://%s/api/v3/air?deviceName=%s&key=%s&code=%d",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST,
            blinker_mqtt_devicename(),
            blinker_mqtt_token(),
            city);

    if (!url) {
        BLINKER_FREE(*payload);
        return err;
    }

    err = blinker_device_http(url, "", *payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/2, BLINKER_HTTP_METHOD_GET);

    BLINKER_FREE(url);

    return err;
}

esp_err_t blinker_sms(const char *msg)
{
    esp_err_t err = ESP_FAIL;

    char *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, sizeof(char));
    char *post_msg = NULL;
    char *url = NULL;

    if (!payload) {
        return err;
    }

    asprintf(&post_msg, "{\"deviceName\":\"%s\",\"key\":\"%s\",\"msg\":\"%s\",\"cel\":\"\"}",
            blinker_mqtt_devicename(),
            blinker_mqtt_authkey(),
            msg);

    if (!post_msg) {
        BLINKER_FREE(payload);
        return err;
    }

    asprintf(&url, "%s://%s/api/v1/user/device/sms",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST);

    if (!url) {
        BLINKER_FREE(payload);
        BLINKER_FREE(post_msg);
        return err;
    }

    err = blinker_device_http(url, post_msg, payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, BLINKER_HTTP_METHOD_POST);
    
    BLINKER_FREE(payload);
    BLINKER_FREE(post_msg);
    BLINKER_FREE(url);

    return err;
}

esp_err_t blinker_push(const char *msg)
{
    esp_err_t err = ESP_FAIL;

    char *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, sizeof(char));
    char *post_msg = NULL;
    char *url = NULL;

    if (!payload) {
        return err;
    }

    asprintf(&post_msg, "{\"deviceName\":\"%s\",\"key\":\"%s\",\"msg\":\"%s\",\"receivers\":\"\"}",
            blinker_mqtt_devicename(),
            blinker_mqtt_authkey(),
            msg);

    if (!post_msg) {
        BLINKER_FREE(payload);
        return err;
    }

    asprintf(&url, "%s://%s/api/v1/user/device/push",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST);

    if (!url) {
        BLINKER_FREE(payload);
        BLINKER_FREE(post_msg);
        return err;
    }

    err = blinker_device_http(url, post_msg, payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, BLINKER_HTTP_METHOD_POST);
    
    BLINKER_FREE(payload);
    BLINKER_FREE(post_msg);
    BLINKER_FREE(url);

    return err;
}

esp_err_t blinker_wechat(const char *msg)
{
    esp_err_t err = ESP_FAIL;

    char *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, sizeof(char));
    char *post_msg = NULL;
    char *url = NULL;

    if (!payload) {
        return err;
    }

    asprintf(&post_msg, "{\"deviceName\":\"%s\",\"key\":\"%s\",\"msg\":\"%s\"}",
            blinker_mqtt_devicename(),
            blinker_mqtt_authkey(),
            msg);

    if (!post_msg) {
        BLINKER_FREE(payload);
        return err;
    }

    asprintf(&url, "%s://%s/api/v1/user/device/wxMsg/",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST);

    if (!url) {
        BLINKER_FREE(payload);
        BLINKER_FREE(post_msg);
        return err;
    }

    err = blinker_device_http(url, post_msg, payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, BLINKER_HTTP_METHOD_POST);
    
    BLINKER_FREE(payload);
    BLINKER_FREE(post_msg);
    BLINKER_FREE(url);

    return err;
}

esp_err_t blinker_wechat_template(const char *title, const char *state, const char *msg)
{
    esp_err_t err = ESP_FAIL;

    char *payload = calloc(1, CONFIG_BLINKER_HTTP_BUFFER_SIZE/4);
    char *post_msg = NULL;
    char *url = NULL;

    if (!payload) {
        return err;
    }

    asprintf(&post_msg, "{\"deviceName\":\"%s\",\"key\":\"%s\",\"title\":\"%s\",\"state\":\"%s\",\"msg\":\"%s\",\"receivers\":\"\"}",
            blinker_mqtt_devicename(),
            blinker_mqtt_authkey(),
            title,
            state,
            msg);

    if (!post_msg) {
        BLINKER_FREE(payload);
        return err;
    }

    asprintf(&url, "%s://%s/api/v1/user/device/wxMsg/",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST);

    if (!url) {
        BLINKER_FREE(payload);
        BLINKER_FREE(post_msg);
        return err;
    }

    err = blinker_device_http(url, post_msg, payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, BLINKER_HTTP_METHOD_POST);
    
    BLINKER_FREE(payload);
    BLINKER_FREE(post_msg);
    BLINKER_FREE(url);

    return err;
}

esp_err_t blinker_log(const char *msg)
{
    esp_err_t err = ESP_FAIL;

    char *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, sizeof(char));
    char *post_msg = NULL;
    char *url = NULL;

    if (!payload) {
        return err;
    }

    asprintf(&post_msg, "{\"device\":\"%s\",\"token\":\"%s\",\"data\":[[%ld,\"%s\"]]}",
            blinker_mqtt_devicename(),
            blinker_mqtt_token(),
            blinker_time(),
            msg);

    if (!post_msg) {
        BLINKER_FREE(payload);
        return err;
    }

    asprintf(&url, "%s://%s/api/v1/user/device/cloud_storage/logs",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST);

    if (!url) {
        BLINKER_FREE(payload);
        BLINKER_FREE(post_msg);
        return err;
    }

    err = blinker_device_http(url, post_msg, payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, BLINKER_HTTP_METHOD_POST);
    
    BLINKER_FREE(payload);
    BLINKER_FREE(post_msg);
    BLINKER_FREE(url);

    return err;
}

esp_err_t blinker_config_update(const char *msg)
{
    esp_err_t err = ESP_FAIL;

    char *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, sizeof(char));
    char *post_msg = NULL;
    char *url = NULL;

    if (!payload) {
        return err;
    }

    asprintf(&post_msg, "{\"device\":\"%s\",\"key\":\"%s\",\"data\":\"%s\"}",
            blinker_mqtt_devicename(),
            blinker_mqtt_authkey(),
            msg);

    if (!post_msg) {
        BLINKER_FREE(payload);
        return err;
    }

    asprintf(&url, "%s://%s/api/v1/storage/object",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST);

    if (!url) {
        BLINKER_FREE(payload);
        BLINKER_FREE(post_msg);
        return err;
    }

    err = blinker_device_http(url, post_msg, payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, BLINKER_HTTP_METHOD_POST);
    
    BLINKER_FREE(payload);
    BLINKER_FREE(post_msg);
    BLINKER_FREE(url);

    return err;
}

esp_err_t blinker_config_get(char **payload)
{
    esp_err_t err = ESP_FAIL;

    *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/2, sizeof(char));
    char *url = NULL;

    if (!*payload) {
        return err;
    }

    asprintf(&url, "%s://%s/api/v1/user/device/cloud_storage/object?token=%s",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST,
            blinker_mqtt_token());

    if (!url) {
        BLINKER_FREE(*payload);
        return err;
    }

    err = blinker_device_http(url, "", *payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/2, BLINKER_HTTP_METHOD_GET);

    BLINKER_FREE(url);

    return err;
}

/////////////////////////////////////////////////////////////////////////

static void blinker_device_print(const char *key, const char *value, const bool is_raw)
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

static esp_err_t blinker_device_http(const char *url, const char *msg, char *payload, int payload_len, blinker_device_http_method_t type)
{
    esp_err_t err = ESP_FAIL;

    esp_http_client_config_t config = {
        .url = "http://iot.diandeng.tech/",
    };

    esp_http_client_handle_t client = blinker_http_init(&config);

    ESP_LOGI(TAG, "http connect to: %s", url);

    switch (type)
    {
        case BLINKER_HTTP_METHOD_GET:
            err = blinker_http_set_url(client, url);
            err = blinker_http_get(client);
            err = blinker_http_read_response(client, payload, payload_len);
            err = blinker_http_close(client);
            break;

        case BLINKER_HTTP_METHOD_POST:
            err = blinker_http_set_url(client, url);
            err = blinker_http_set_header(client, "Content-Type", "application/json");
            err = blinker_http_post(client, msg);
            err = blinker_http_read_response(client, payload, payload_len);
            err = blinker_http_close(client);
            break;
        
        default:
            break;
    }

    ESP_LOGI(TAG, "read response: %s", payload);

    return err;
}

/////////////////////////////////////////////////////////////////////////

static esp_err_t blinker_va_print(const char *param, const blinker_data_from_param_t *from_param)
{
    esp_err_t err    = ESP_FAIL;

    cJSON *root      = NULL;
    char *print_data = NULL;
    char *pub_topic  = NULL;

    if (blinker_mqtt_broker() == BLINKER_BROKER_ALIYUN) {
        root =cJSON_CreateObject();
        cJSON_AddStringToObject(root, BLINKER_CMD_TO_DEVICE, from_param->uuid);
        cJSON_AddStringToObject(root, BLINKER_CMD_FROM_DEVICE, blinker_mqtt_devicename());
        cJSON_AddStringToObject(root, BLINKER_CMD_DEVICE_TYPE, BLINKER_CMD_VASSISTANT);
        cJSON_AddRawToObject(root, BLINKER_CMD_DATA, param);

        print_data = calloc(BLINKER_FORMAT_DATA_SIZE, sizeof(char));
        
        if (!print_data) {
            cJSON_Delete(root);
            return err;
        }
        
        asprintf(&pub_topic, "/sys/%s/%s/rrpc/response%s", 
                blinker_mqtt_product_key(),
                blinker_mqtt_devicename(),
                from_param->message_id);

        if (!pub_topic) {
            BLINKER_FREE(print_data);
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
        cJSON_AddRawToObject(root, BLINKER_CMD_DATA, param);

        print_data = calloc(BLINKER_FORMAT_DATA_SIZE, sizeof(char));
        
        if (!print_data) {
            cJSON_Delete(root);
            return err;
        }
        
        cJSON_PrintPreallocated(root, print_data, BLINKER_FORMAT_DATA_SIZE, false);

        asprintf(&pub_topic, "/device/%s/s",
                blinker_mqtt_devicename());

        if (!pub_topic) {
            BLINKER_FREE(print_data);
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

static void blinker_va_param_add(blinker_va_data_t *va, const char *param, const blinker_va_param_type_t param_type, const blinker_va_param_val_type_t val_type)
{
    if (va == NULL) {
        return;
    }

    blinker_va_param_t *va_param = calloc(1, sizeof(blinker_va_param_t));

    if (!va_param) {
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

static bool blinker_va_parse(cJSON *data, const blinker_va_data_t *va)
{
    bool is_parsed = false;

    if (va == NULL) {
        return is_parsed;
    }

    if (cJSON_HasObjectItem(data, BLINKER_CMD_GET)) {
        blinker_va_param_cb_t val = {0};

        if (va->cb) {
            val.type = BLINKER_PARAM_GET_STATE;
            va->cb(&val);
        }

        is_parsed = true;
    } else if (cJSON_HasObjectItem(data, BLINKER_CMD_SET)) {
        cJSON *set_data = cJSON_GetObjectItem(data, BLINKER_CMD_SET);

        if (cJSON_IsNull(set_data)) {
            // cJSON_Delete(set_data);
            return is_parsed;
        }

        blinker_va_param_t *va_param;
        SLIST_FOREACH(va_param, &va->va_param_list, next) {
            if (cJSON_HasObjectItem(set_data, va_param->key)) {
                is_parsed = true;

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
                
                // cJSON_Delete(param_set);
            }
        }
    }

    return is_parsed;
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

    char *param_data = cJSON_PrintUnformatted(param);
    err = blinker_va_print(param_data, data_from);
    BLINKER_FREE(param_data);

    return err;
}

esp_err_t blinker_aligenie_handler_register(const blinker_va_cb_t cb)
{
    if (va_ali == NULL) {
        return ESP_FAIL;
    }

    va_ali->cb = cb;

    return ESP_OK;
}

#ifndef CONFIG_BLINKER_ALIGENIE_NONE
static void blinker_aligenie_init(void)
{
    if (va_ali == NULL) {
        va_ali = calloc(1, sizeof(blinker_va_data_t));

        if (!va_ali) {
            return;
        }
    }

    blinker_va_param_add(va_ali, BLINKER_CMD_POWER_STATE,       BLINKER_PARAM_POWER_STATE,     BLINKER_VAL_TYPE_STRING);
#if defined CONFIG_BLINKER_ALIGENIE_LIGHT
    blinker_va_param_add(va_ali, BLINKER_CMD_COLOR_,            BLINKER_PARAM_COLOR,           BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_ali, BLINKER_CMD_MODE,              BLINKER_PARAM_MODE,            BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_ali, BLINKER_CMD_COLOR_TEMP,        BLINKER_PARAM_COLOR_TEMP,      BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_COLOR_TEMP_UP,     BLINKER_PARAM_COLOR_TEMP_UP,   BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_COLOR_TEMP_DOWN,   BLINKER_PARAM_COLOR_TEMP_DOWN, BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_BRIGHTNESS,        BLINKER_PARAM_BRIGHTNESS,      BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_BRIGHTNESS_UP,     BLINKER_PARAM_BRIGHTNESS_UP,   BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_BRIGHTNESS_DOWN,   BLINKER_PARAM_BRIGHTNESS_DOWN, BLINKER_VAL_TYPE_INT);
#elif defined CONFIG_BLINKER_ALIGENIE_AIR_CONDITION
    blinker_va_param_add(va_ali, BLINKER_CMD_TEMP,              BLINKER_PARAM_TEMP,            BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_TEMP_UP,           BLINKER_PARAM_TEMP_UP,         BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_TEMP_DOWN,         BLINKER_PARAM_TEMP_DOWN,       BLINKER_VAL_TYPE_INT);
    // blinker_va_param_add(va_ali, BLINKER_CMD_HUMI,              BLINKER_PARAM_HUMI,            BLINKER_VAL_TYPE_INT);
#endif

#if defined(CONFIG_BLINKER_ALIGENIE_AIR_CONDITION) || defined(CONFIG_BLINKER_ALIGENIE_FAN)
    blinker_va_param_add(va_ali, BLINKER_CMD_MODE,              BLINKER_PARAM_MODE,            BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_ali, BLINKER_CMD_HSTATE,            BLINKER_PARAM_HSWING,          BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_ali, BLINKER_CMD_VSTATE,            BLINKER_PARAM_VSWING,          BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_ali, BLINKER_CMD_LEVEL,             BLINKER_PARAM_LEVEL,           BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_LEVEL_UP,          BLINKER_PARAM_LEVEL_UP,        BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_ali, BLINKER_CMD_LEVEL_DOWN,        BLINKER_PARAM_LEVEL_DOWN,      BLINKER_VAL_TYPE_INT);
#endif
}
#endif

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

    char *param_data = cJSON_PrintUnformatted(param);
    err = blinker_va_print(param_data, data_from);
    BLINKER_FREE(param_data);

    return err;
}

esp_err_t blinker_dueros_handler_register(const blinker_va_cb_t cb)
{
    if (va_duer == NULL) {
        return ESP_FAIL;
    }

    va_duer->cb = cb;

    return ESP_OK;
}

#ifndef CONFIG_BLINKER_DUEROS_NONE
static void blinker_dueros_init(void)
{
    if (va_duer == NULL) {
        va_duer = calloc(1, sizeof(blinker_va_data_t));

        if (!va_duer) {
            return;
        }
    }

    blinker_va_param_add(va_duer, BLINKER_CMD_POWER_STATE,       BLINKER_PARAM_POWER_STATE,     BLINKER_VAL_TYPE_STRING);
#if defined CONFIG_BLINKER_DUEROS_LIGHT
    blinker_va_param_add(va_duer, BLINKER_CMD_COLOR_,            BLINKER_PARAM_COLOR,           BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_duer, BLINKER_CMD_MODE,              BLINKER_PARAM_MODE,            BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_duer, BLINKER_CMD_COLOR_TEMP,        BLINKER_PARAM_COLOR_TEMP,      BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_COLOR_TEMP_UP,     BLINKER_PARAM_COLOR_TEMP_UP,   BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_COLOR_TEMP_DOWN,   BLINKER_PARAM_COLOR_TEMP_DOWN, BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_BRIGHTNESS,        BLINKER_PARAM_BRIGHTNESS,      BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_BRIGHTNESS_UP,     BLINKER_PARAM_BRIGHTNESS_UP,   BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_BRIGHTNESS_DOWN,   BLINKER_PARAM_BRIGHTNESS_DOWN, BLINKER_VAL_TYPE_INT);
#elif defined CONFIG_BLINKER_DUEROS_AIR_CONDITION
    blinker_va_param_add(va_duer, BLINKER_CMD_TEMP,              BLINKER_PARAM_TEMP,            BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_TEMP_UP,           BLINKER_PARAM_TEMP_UP,         BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_TEMP_DOWN,         BLINKER_PARAM_TEMP_DOWN,       BLINKER_VAL_TYPE_INT);
    // blinker_va_param_add(va_duer, BLINKER_CMD_HUMI,              BLINKER_PARAM_HUMI,            BLINKER_VAL_TYPE_INT);
#endif

#if defined(CONFIG_BLINKER_DUEROS_AIR_CONDITION) || defined(CONFIG_BLINKER_DUEROS_FAN)
    blinker_va_param_add(va_duer, BLINKER_CMD_MODE,              BLINKER_PARAM_MODE,            BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_duer, BLINKER_CMD_LEVEL,             BLINKER_PARAM_LEVEL,           BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_LEVEL_UP,          BLINKER_PARAM_LEVEL_UP,        BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_duer, BLINKER_CMD_LEVEL_DOWN,        BLINKER_PARAM_LEVEL_DOWN,      BLINKER_VAL_TYPE_INT);
#endif
}
#endif

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

    char *param_data = cJSON_PrintUnformatted(param);
    err = blinker_va_print(param_data, data_from);
    BLINKER_FREE(param_data);

    return err;
}

esp_err_t blinker_miot_handler_register(const blinker_va_cb_t cb)
{
    if (va_miot == NULL) {
        return ESP_FAIL;
    }

    va_miot->cb = cb;

    return ESP_OK;
}

#ifndef CONFIG_BLINKER_MIOT_NONE
static void blinker_miot_init(void)
{
    if (va_miot == NULL) {
        va_miot = calloc(1, sizeof(blinker_va_data_t));

        if (!va_miot) {
            return;
        }
    }

    blinker_va_param_add(va_miot, BLINKER_CMD_POWER_STATE, BLINKER_PARAM_POWER_STATE, BLINKER_VAL_TYPE_STRING);
#if defined CONFIG_BLINKER_MIOT_LIGHT
    blinker_va_param_add(va_miot, BLINKER_CMD_COLOR_,      BLINKER_PARAM_COLOR,       BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_miot, BLINKER_CMD_MODE,        BLINKER_PARAM_MODE,        BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_miot, BLINKER_CMD_COLOR_TEMP,  BLINKER_PARAM_COLOR_TEMP,  BLINKER_VAL_TYPE_INT);
    blinker_va_param_add(va_miot, BLINKER_CMD_BRIGHTNESS,  BLINKER_PARAM_BRIGHTNESS,  BLINKER_VAL_TYPE_INT);
#elif defined CONFIG_BLINKER_MIOT_AIR_CONDITION
    blinker_va_param_add(va_miot, BLINKER_CMD_ECO,         BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_ANION,       BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_HEATER,      BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_DRYER,       BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_ANION,       BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_SOFT,        BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_UV,          BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_UNSB,        BLINKER_PARAM_MODE_STATE,  BLINKER_VAL_TYPE_STRING_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_TEMP,        BLINKER_PARAM_TEMP,        BLINKER_VAL_TYPE_INT);
    // blinker_va_param_add(va_miot, BLINKER_CMD_HUMI,        BLINKER_PARAM_HUMI,        BLINKER_VAL_TYPE_INT);
#endif

#if defined(CONFIG_BLINKER_MIOT_AIR_CONDITION) || defined(CONFIG_BLINKER_MIOT_FAN)
    blinker_va_param_add(va_miot, BLINKER_CMD_HSTATE,      BLINKER_PARAM_HSWING,      BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_VSTATE,      BLINKER_PARAM_VSWING,      BLINKER_VAL_TYPE_STRING);
    blinker_va_param_add(va_miot, BLINKER_CMD_LEVEL,       BLINKER_PARAM_LEVEL,       BLINKER_VAL_TYPE_INT);
#endif
}
#endif

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
    cJSON *param = cJSON_CreateObject();
    blinker_widget_value_string(param, tab_data);
    blinker_widget_print(key, param);
    cJSON_Delete(param);

    return ESP_OK;
}

esp_err_t blinker_widget_print(const char *key, cJSON *param)
{
    char *raw_data = cJSON_PrintUnformatted(param);

    blinker_device_print(key, raw_data, true);
    BLINKER_FREE(raw_data);
    
    return ESP_OK;
}

esp_err_t blinker_widget_add(const char *key, const blinker_widget_type_t type, const blinker_widget_cb_t cb)
{
    blinker_widget_data_t *widget = calloc(1, sizeof(blinker_widget_data_t));

    if (!widget) {
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

static bool blinker_widget_parse(const cJSON *data)
{
    bool is_parsed = false;

    blinker_widget_data_t *widget;
    SLIST_FOREACH(widget, &blinker_widget_list, next) {
        if (cJSON_HasObjectItem(data, widget->key)) {
            is_parsed = true;
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

                case BLINKER_SWITCH:
                    if (cJSON_IsString(widget_param)) {
                        param_val.s = widget_param->valuestring;
                        widget->cb(&param_val);
                    }
                    break;

                default:
                    break;
            }
            // cJSON_Delete(widget_param);
        }
    }

    return is_parsed;
}

/////////////////////////////////////////////////////////////////////////

static void blinker_device_print_to_user(char *data)
{

    if (data_from && data_from->type == BLINKER_DATA_FROM_WS) {
        strcat(data, "\n");
        blinker_websocket_async_print(data_from->resp_arg, data);
    } else {
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
}

static void blinker_device_auto_format_task(void *arg)
{
    blinker_auto_format_queue_t *format_queue = NULL;
    char *format_data = NULL;

    for(;;) {
        if (xQueueReceive(auto_format_queue, &format_queue, pdMS_TO_TICKS(BLINKER_FORMAT_QUEUE_TIME)) == pdFALSE) {
            if (format_data != NULL) {
                
                if (data_from && data_from->type == BLINKER_DATA_FROM_WS) {
                    blinker_device_print_to_user(format_data);

                    BLINKER_FREE(format_data);
                }else {
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
                }

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

static bool blinker_device_heart_beat(cJSON *data)
{
    if (strncmp(BLINKER_CMD_STATE, 
                cJSON_GetObjectItem(data, BLINKER_CMD_GET)->valuestring, 
                strlen(BLINKER_CMD_STATE)) == 0) {
        blinker_device_print(BLINKER_CMD_STATE, BLINKER_CMD_ONLINE, false);
        blinker_device_print(BLINKER_CMD_VERSION, CONFIG_BLINKER_FIRMWARE_VERSION, false);

        if (heart_beat_cb) {
            heart_beat_cb();
        }

        return true;
    }

    return false;
}

#if defined CONFIG_BLINKER_PRO_ESP
static esp_err_t blinker_device_auth(blinker_mqtt_config_t *mqtt_cfg)
{
    esp_err_t err = ESP_FAIL;

    char *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE, sizeof(char));
    char *mac_name = calloc(13, sizeof(char));
    char *url = NULL;

    if (!payload) {
        return err;
    }

    if (!mac_name) {
        BLINKER_FREE(payload);
        return err;
    }

    blinker_mac_device_name(mac_name);

    asprintf(&url, "%s://%s/api/v1/user/device/auth/get?deviceType=%s&typeKey=%s&deviceName=%s",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST,
            CONFIG_BLINKER_TYPE_KEY,
            CONFIG_BLINKER_AUTH_KEY,
            mac_name);

    if (!url) {
        BLINKER_FREE(payload);
        BLINKER_FREE(mac_name);
        return err;
    }

    blinker_device_http(url, "", payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE, BLINKER_HTTP_METHOD_GET);

    BLINKER_FREE(url);
    BLINKER_FREE(mac_name);

    ESP_LOGI(TAG, "read response: %s", payload);

    cJSON *root = cJSON_Parse(payload);

    if (!root) {
        BLINKER_FREE(payload);
        cJSON_Delete(root);
        return err;
    } else {
        cJSON *detail = cJSON_GetObjectItem(root, BLINKER_CMD_DETAIL);
        if (detail != NULL && cJSON_HasObjectItem(detail, BLINKER_CMD_AUTHKEY)) {
            mqtt_cfg->authkey = strdup(cJSON_GetObjectItem(detail, BLINKER_CMD_AUTHKEY)->valuestring);

            err = ESP_OK;
        }
        // cJSON_Delete(detail);
    }

    BLINKER_FREE(payload);
    cJSON_Delete(root);

    return err;
}
#endif

static esp_err_t blinker_device_register(blinker_mqtt_config_t *mqtt_cfg)
{
    esp_err_t err = ESP_FAIL;
    
    char *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE, sizeof(char));
    char *url = NULL;

    if (!payload) {
        return err;
    }

#if defined CONFIG_BLINKER_CUSTOM_ESP
    asprintf(&url, "%s://%s/api/v1/user/device/diy/auth?authKey=%s&protocol=%s&version=%s%s%s%s",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST,
            CONFIG_BLINKER_AUTH_KEY,
            BLINKER_PROTOCOL_MQTT,
            CONFIG_BLINKER_FIRMWARE_VERSION,
            CONFIG_BLINKER_ALIGENIE_TYPE,
            CONFIG_BLINKER_DUEROS_TYPE,
            CONFIG_BLINKER_MIOT_TYPE);
#elif defined CONFIG_BLINKER_PRO_ESP
    asprintf(&url, "%s://%s/api/v1/user/device/auth?authKey=%s&protocol=%s&version=%s%s%s%s",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST,
            mqtt_cfg->authkey,
            BLINKER_PROTOCOL_MQTT,
            CONFIG_BLINKER_FIRMWARE_VERSION,
            CONFIG_BLINKER_ALIGENIE_TYPE,
            CONFIG_BLINKER_DUEROS_TYPE,
            CONFIG_BLINKER_MIOT_TYPE);
#endif

    if (!url) {
        BLINKER_FREE(payload);
        return err;
    }

    blinker_device_http(url, "", payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE, BLINKER_HTTP_METHOD_GET);

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
#if defined CONFIG_BLINKER_CUSTOM_ESP
            mqtt_cfg->authkey    = strdup(CONFIG_BLINKER_AUTH_KEY);
#endif
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
        // cJSON_Delete(detail);
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

static void blinker_websocket_data_callback(async_resp_arg_t *req, const char *payload)
{
    xSemaphoreTake(data_parse_mutex, portMAX_DELAY);

    cJSON *ws_data = cJSON_Parse(payload);

    if (ws_data == NULL) {
        cJSON_Delete(ws_data);
        xSemaphoreGive(data_parse_mutex);
        return;
    }

#if defined CONFIG_BLINKER_PRO_ESP
    if (cJSON_HasObjectItem(ws_data, BLINKER_CMD_REGISTER) && pro_device_wait_register) {
        if (strcmp(CONFIG_BLINKER_TYPE_KEY, cJSON_GetObjectItem(ws_data, BLINKER_CMD_REGISTER)->valuestring) == 0) {
            pro_device_get_register = true;

            blinker_websocket_async_print(req, "{\"message\":\"success\"}\n");

            cJSON_Delete(ws_data);
            xSemaphoreGive(data_parse_mutex);

            return;
        }
    }
#endif

#if defined CONFIG_BLINKER_AP_CONFIG
    if (cJSON_HasObjectItem(ws_data, BLINKER_CMD_SSID) && b_apconfig_group) {
        wifi_config_t wifi_cfg = {0};
        strcpy((char *)wifi_cfg.sta.ssid, cJSON_GetObjectItem(ws_data, BLINKER_CMD_SSID)->valuestring);
        strcpy((char *)wifi_cfg.sta.password, cJSON_GetObjectItem(ws_data, BLINKER_CMD_PASSWORD)->valuestring);

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
        ESP_ERROR_CHECK(esp_wifi_connect());

        cJSON_Delete(ws_data);
        xSemaphoreGive(data_parse_mutex);
        xEventGroupSetBits(b_apconfig_group, APCONFIG_DONE_BIT);

        return;
    }
#endif

    if (data_from) {
        if (data_from->uuid) {
            BLINKER_FREE(data_from->uuid);
        }
        if (data_from->message_id) {
            BLINKER_FREE(data_from->message_id);
        }
        if (data_from->resp_arg) {
            BLINKER_FREE(data_from->resp_arg);
        }
        BLINKER_FREE(data_from);
    }

    data_from = calloc(1, sizeof(blinker_data_from_param_t));

    if (data_from) {
        data_from->resp_arg = calloc(1, sizeof(async_resp_arg_t));
        data_from->resp_arg->hd = req->hd;
        data_from->resp_arg->fd = req->fd;
        data_from->type = BLINKER_DATA_FROM_WS;
    } else {
        cJSON_Delete(ws_data);
        xSemaphoreGive(data_parse_mutex);
        return;
    }

    bool is_parsed = false;

    if (cJSON_HasObjectItem(ws_data, BLINKER_CMD_GET)) {
        is_parsed = blinker_device_heart_beat(ws_data);
    } else {
        is_parsed = blinker_widget_parse(ws_data);
    }

    if (!is_parsed) {
        if (extra_data_cb) {
            char *extra_data = cJSON_PrintUnformatted(ws_data);
            extra_data_cb(extra_data);
            BLINKER_FREE(extra_data);
        }
    }
    
    cJSON_Delete(ws_data);

    xSemaphoreGive(data_parse_mutex);
}

static void blinker_mqtt_data_callback(const char *topic, size_t topic_len, void *payload, size_t payload_len)
{
    xSemaphoreTake(data_parse_mutex, portMAX_DELAY);

    cJSON *sub_data = cJSON_Parse(payload);

    if (sub_data == NULL) {
        cJSON_Delete(sub_data);
        xSemaphoreGive(data_parse_mutex);
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
            if (data_from->resp_arg) {
                BLINKER_FREE(data_from->resp_arg);
            }
            BLINKER_FREE(data_from);
        }

        data_from = calloc(1, sizeof(blinker_data_from_param_t));

        if (data_from) {
            data_from->uuid = strdup(cJSON_GetObjectItem(sub_data, BLINKER_CMD_FROM_DEVICE)->valuestring);

            cJSON *main_data = cJSON_GetObjectItem(sub_data, BLINKER_CMD_DATA);
            if (main_data == NULL) {
                BLINKER_FREE(data_from->uuid);
                BLINKER_FREE(data_from);
                // cJSON_Delete(main_data);
                cJSON_Delete(sub_data);
                xSemaphoreGive(data_parse_mutex);
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

            bool is_parsed = false;

            switch (data_from->type)
            {
                case BLINKER_DATA_FROM_USER:
                    if (cJSON_HasObjectItem(main_data, BLINKER_CMD_GET)) {
                        is_parsed = blinker_device_heart_beat(main_data);
                    } else {
                        is_parsed = blinker_widget_parse(main_data);
                    }
                    break;

                case BLINKER_DATA_FROM_ALIGENIE:
                    if (va_ali != NULL) {
                        is_parsed = blinker_va_parse(main_data, va_ali);
                    }
                    break;

                case BLINKER_DATA_FROM_DUEROS:
                    if (va_duer != NULL) {
                        is_parsed = blinker_va_parse(main_data, va_duer);
                    }
                    break;

                case BLINKER_DATA_FROM_MIOT:
                    if (va_miot != NULL) {
                        is_parsed = blinker_va_parse(main_data, va_miot);
                    }
                    break;

                default:
                    break;
            }

            if (!is_parsed) {
                if (extra_data_cb) {
                    char *extra_data = cJSON_PrintUnformatted(main_data);
                    extra_data_cb(extra_data);
                    BLINKER_FREE(extra_data);
                }
            }
        
            // cJSON_Delete(main_data);
        }
    }

    cJSON_Delete(sub_data);

    xSemaphoreGive(data_parse_mutex);
}

static void blinker_http_heart_beat(void *timer)
{
    char *payload = calloc(CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, sizeof(char));
    char *url = NULL;

    if (!payload) {
        return;
    }

    asprintf(&url, "%s://%s/api/v1/user/device/heartbeat?deviceName=%s&key=%s&heartbeat=%d",
            BLINKER_PROTPCOL_HTTP,
            CONFIG_BLINKER_SERVER_HOST,
            blinker_mqtt_devicename(),
            blinker_mqtt_authkey(),
            CONFIG_BLINKER_HTTP_HEART_BEAT_TIME_INTERVAL * BLINKER_MIN_TO_S);

    if (!url) {
        BLINKER_FREE(payload);
        return;
    }

    blinker_device_http(url, "", payload, CONFIG_BLINKER_HTTP_BUFFER_SIZE/4, BLINKER_HTTP_METHOD_GET);

    BLINKER_FREE(payload);
    BLINKER_FREE(url);
}
#if defined(CONFIG_BUTTON_RESET_TYPE)
static void blinker_button_reset_cb(void *arg)
{
    blinker_storage_erase(CONFIG_BLINKER_NVS_NAMESPACE);
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
}

static esp_err_t blinker_button_reset_init(void)
{
    button_handle_t btn_handle = iot_button_create(CONFIG_BUTTON_IO_NUM, CONFIG_BUTTON_ACTIVE_LEVEL);
    
    esp_err_t err = iot_button_add_custom_cb(btn_handle, CONFIG_BUTTON_RESET_INTERVAL_TIME, blinker_button_reset_cb, NULL);

    return err;
}
#elif defined(CONFIG_REBOOT_RESET_TYPE)
static esp_err_t blinker_reboot_reset_check(void)
{
    blinker_reboot_unbroken_count();

    return ESP_OK;
}
#endif

static void blinker_device_register_task(void *arg)
{
#if defined CONFIG_BLINKER_CUSTOM_ESP
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
#elif defined CONFIG_BLINKER_PRO_ESP
    esp_err_t err = ESP_FAIL;
    blinker_mqtt_config_t mqtt_config = {0};
    blinker_pro_esp_param_t pro_auth  = {0};
    blinker_pro_device_status_t pro_status = BLINKER_PRO_DEVICE_CHECK;
    bool task_run = true;
    char mac_name[13] = {0};

    while(task_run) {
        switch (pro_status) {
            case BLINKER_PRO_DEVICE_CHECK:
                if (blinker_storage_get(BLINKER_PRO_ESP_CHECK_KEY, &pro_auth, sizeof(blinker_pro_esp_param_t)) != ESP_OK) {
                    blinker_mac_device_name(mac_name);
                    blinker_mdns_init(mac_name);
                    pro_status = BLINKER_PRO_DEVICE_WAIT_REGISTER;
                    pro_device_wait_register = true;

                    ESP_LOGI(TAG, "BLINKER_PRO_DEVICE_WAIT_REGISTER");
                } else {
                    pro_status = BLINKER_PRO_DEVICE_REGISTER;
                }
                break;
            
            case BLINKER_PRO_DEVICE_WAIT_REGISTER:
                if (pro_device_get_register) {
                    pro_device_wait_register = false;
                    pro_status = BLINKER_PRO_DEVICE_REGISTER;
                    ESP_LOGI(TAG, "BLINKER_PRO_DEVICE_REGISTER");
                } else {
                    vTaskDelay(pdMS_TO_TICKS(BLINKER_REGISTER_INTERVAL/10));
                }
                break;

            case BLINKER_PRO_DEVICE_REGISTER:
                err = blinker_device_auth(&mqtt_config);
                if (err != ESP_OK) {
                    vTaskDelay(pdMS_TO_TICKS(BLINKER_REGISTER_INTERVAL));
                    break;
                }

                err = blinker_device_register(&mqtt_config);

                // blinker_device_print(BLINKER_CMD_MESSAGE, BLINKER_CMD_SUCCESS, false);

                if (err != ESP_OK) {
                    vTaskDelay(pdMS_TO_TICKS(BLINKER_REGISTER_INTERVAL));
                } else {
                    err = blinker_mdns_init(mqtt_config.devicename);
                    err = blinker_mqtt_init(&mqtt_config);

                    task_run = false;
                }
                break;
            
            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    xEventGroupSetBits(b_register_group, REGISTERED_BIT);
    vTaskDelete(NULL);
#endif
}

static esp_err_t blinker_device_mqtt_init(void)
{
    esp_err_t err = ESP_FAIL;

    b_register_group = xEventGroupCreate();

    xTaskCreate(blinker_device_register_task, 
                "device_register",
                BLINKER_REGISTER_TASK_DEEP * 1024,
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

    if (!sub_topic) {
        return err;
    }
    blinker_mqtt_subscribe(sub_topic, blinker_mqtt_data_callback);
    BLINKER_FREE(sub_topic);

    if (blinker_mqtt_broker() == BLINKER_BROKER_ALIYUN) {
        asprintf(&sub_topic, "/sys/%s/%s/rrpc/request/+",
                blinker_mqtt_product_key(),
                blinker_mqtt_devicename());

        if (!sub_topic) {
            return err;
        }
        blinker_mqtt_subscribe(sub_topic, blinker_mqtt_data_callback);
        BLINKER_FREE(sub_topic);
    }

#if defined CONFIG_BLINKER_PRO_ESP
    blinker_pro_esp_param_t pro_auth  = {0};
    
    if (blinker_storage_get(BLINKER_PRO_ESP_CHECK_KEY, &pro_auth, sizeof(blinker_pro_esp_param_t)) != ESP_OK) {
        pro_auth.uuid = blinker_mqtt_uuid();
        ESP_LOGE(TAG, "check storage state: %d", blinker_storage_set(BLINKER_PRO_ESP_CHECK_KEY, &pro_auth, sizeof(blinker_pro_esp_param_t)));

        blinker_device_print(BLINKER_CMD_MESSAGE, BLINKER_CMD_REGISTER_SUCCESS, false);
    }
#endif

    return err;
}

static esp_err_t blinker_wifi_get_config(wifi_config_t *wifi_config)
{
    if (blinker_storage_get("wifi_config", wifi_config, sizeof(wifi_config_t)) == ESP_OK) {
#if defined CONFIG_BLINKER_DEFAULT_CONFIG
        if (!strcmp((char *)wifi_config->sta.ssid, CONFIG_BLINKER_WIFI_SSID)) {
            return ESP_OK;
        }
#else
        return ESP_OK;
#endif
    }

    // esp_wifi_restore();
    // esp_wifi_start();

#if defined CONFIG_BLINKER_DEFAULT_CONFIG
    strcpy((char *)wifi_config->sta.ssid, CONFIG_BLINKER_WIFI_SSID);
    strcpy((char *)wifi_config->sta.password, CONFIG_BLINKER_WIFI_PASSWORD);
#elif defined CONFIG_BLINKER_SMART_CONFIG
    blinker_prov_smartconfig_init();
#elif defined CONFIG_BLINKER_AP_CONFIG
    b_apconfig_group = xEventGroupCreate();
    blinker_prov_apconfig_init();
    blinker_websocket_server_init(blinker_websocket_data_callback);
    xEventGroupWaitBits(b_apconfig_group, APCONFIG_DONE_BIT, true, false, portMAX_DELAY);
#endif

    
#ifndef CONFIG_BLINKER_DEFAULT_CONFIG
    esp_wifi_get_config(ESP_IF_WIFI_STA, wifi_config);
#endif
    blinker_storage_set(BLINKER_CMD_WIFI_CONFIG, wifi_config, sizeof(wifi_config_t));
    
    return ESP_OK;
}

static esp_err_t blinker_device_wifi_init(void)
{
    esp_err_t err = ESP_FAIL;
    wifi_config_t wifi_cfg = { 0 };

    err = blinker_wifi_init();
    err = blinker_wifi_get_config(&wifi_cfg);
    err = blinker_wifi_start(&wifi_cfg);

    return err;
}

esp_err_t blinker_init(void)
{
    esp_err_t err = ESP_FAIL;

    data_parse_mutex = xSemaphoreCreateMutex();

#if defined(CONFIG_REBOOT_RESET_TYPE)
    err = blinker_reboot_reset_check();
#elif defined(CONFIG_BUTTON_RESET_TYPE)
    err = blinker_storage_init();
    err = blinker_button_reset_init();
#endif
    err = blinker_device_wifi_init();

    blinker_timesync_start();

    auto_format_queue = xQueueCreate(10, sizeof(blinker_auto_format_queue_t));
    xTaskCreate(blinker_device_auto_format_task, 
                "device_auto_format",
                3 * 1024,
                NULL,
                3,
                NULL);

    blinker_websocket_server_init(blinker_websocket_data_callback);
    
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

    TimerHandle_t b_http_heart_beat_timer = xTimerCreate("http_heart_beat",
                                                        pdMS_TO_TICKS(CONFIG_BLINKER_HTTP_HEART_BEAT_TIME_INTERVAL*BLINKER_MIN_TO_MS),
                                                        true, 
                                                        NULL, 
                                                        blinker_http_heart_beat);
    xTimerStart(b_http_heart_beat_timer, 0);

    return err;
}
