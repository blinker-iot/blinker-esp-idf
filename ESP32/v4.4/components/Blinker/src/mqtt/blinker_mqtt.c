#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <mqtt_client.h>

#include "blinker_mqtt.h"
#include "blinker_config.h"

static const char *TAG = "blinker_mqtt";

#define MAX_MQTT_SUBSCRIPTIONS      6

typedef struct {
    char *topic;
    blinker_mqtt_subscribe_cb_t cb;
} blinker_mqtt_subscription_t;

typedef struct {
    esp_mqtt_client_handle_t mqtt_client;
    blinker_mqtt_config_t    *config;
    blinker_mqtt_subscription_t *subscriptions[MAX_MQTT_SUBSCRIPTIONS];
} blinker_mqtt_data_t;

blinker_mqtt_data_t *mqtt_data = NULL;
const int MQTT_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t mqtt_event_group;

blinker_mqtt_broker_t blinker_mqtt_broker(void)
{
    return mqtt_data->config->broker;
}

const char *blinker_mqtt_devicename(void)
{
    if (!mqtt_data) return NULL;

    return mqtt_data->config->devicename;
}

const char *blinker_mqtt_token(void)
{
    if (!mqtt_data) return NULL;

    return mqtt_data->config->password;
}

const char *blinker_mqtt_uuid(void)
{
    if (!mqtt_data) return NULL;

    return mqtt_data->config->uuid;
}

const char *blinker_mqtt_authkey(void)
{
    if (!mqtt_data) return NULL;

    return mqtt_data->config->authkey;
}

const char *blinker_mqtt_product_key(void)
{
    if (!mqtt_data) return NULL;

    return mqtt_data->config->productkey;
}

static void blinker_mqtt_subscribe_callback(const char *topic, int topic_len, const char *data, int data_len)
{
    blinker_mqtt_subscription_t **subscriptions = mqtt_data->subscriptions;

    int i;
    int _topic_len;

    for (i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (subscriptions[i]) {
            _topic_len = strlen(subscriptions[i]->topic);
            if (_topic_len != topic_len) {
                _topic_len = _topic_len - 1;
            } else {
                _topic_len = topic_len;
            }
            if (strncmp(topic, subscriptions[i]->topic, _topic_len) == 0) {
                subscriptions[i]->cb(topic, topic_len, (void *)data, data_len);
            }
        }
    }
}

esp_err_t blinker_mqtt_subscribe(const char *topic, blinker_mqtt_subscribe_cb_t cb)
{
    if (!mqtt_data || !topic || !cb) {
        return ESP_FAIL;
    }

    int i;

    for (i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (!mqtt_data->subscriptions[i]) {
            blinker_mqtt_subscription_t *subscription = calloc(1, sizeof(blinker_mqtt_subscription_t));

            if(!subscription) {
                return ESP_FAIL;
            }

            subscription->topic = strdup(topic);
            
            if (!subscription->topic) {
                free(subscription);
                return ESP_FAIL;
            }

            int ret = esp_mqtt_client_subscribe(mqtt_data->mqtt_client, subscription->topic, 0);

            if (ret < 0) {
                free(subscription->topic);
                free(subscription);
                return ESP_FAIL;
            }

            subscription->cb = cb;
            mqtt_data->subscriptions[i] = subscription;
            ESP_LOGI(TAG, "Subscribed to topic: %s", topic);
            return ESP_OK;
        }
    }

    return ESP_OK;
}

esp_err_t blinker_mqtt_unsubscribe(const char *topic)
{
    if (!mqtt_data || !topic) {
        return ESP_FAIL;
    }

    int ret = esp_mqtt_client_unsubscribe(mqtt_data->mqtt_client, topic);

    if (ret < 0) {
        ESP_LOGI(TAG, "Could not unsubscribe from topic: %s", topic);
    }

    blinker_mqtt_subscription_t **subscriptions = mqtt_data->subscriptions;
    int i;

    for (i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (subscriptions[i]) {
            if (strncmp(topic, subscriptions[i]->topic, strlen(topic)) == 0) {
                BLINKER_FREE(subscriptions[i]->topic);
                BLINKER_FREE(subscriptions[i]);
                return ESP_OK;
            }
        }
    }

    return ESP_FAIL;
}

esp_err_t blinker_mqtt_publish(const char *topic, const char *data, size_t data_len)
{
    if (!mqtt_data || !topic || !data) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Publishing to %s, %.*s", topic, data_len, data);
    int ret = esp_mqtt_client_publish(mqtt_data->mqtt_client, topic, data, data_len, 0, 0);

    if (ret < 0) {
        ESP_LOGI(TAG, "MQTT Publish failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            for (int i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
                if (mqtt_data->subscriptions[i]) {
                    esp_mqtt_client_subscribe(event->client, mqtt_data->subscriptions[i]->topic, 0);
                }
            }

            blinker_log_print_heap();

            xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_EVENT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);

            blinker_mqtt_subscribe_callback(event->topic, event->topic_len, event->data, event->data_len);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

esp_err_t blinker_mqtt_connect(void)
{
    if (!mqtt_data) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Connecting to %s", mqtt_data->config->uri);
    mqtt_event_group = xEventGroupCreate();
    esp_err_t ret = esp_mqtt_client_start(mqtt_data->mqtt_client);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_mqtt_client_start() failed with err = %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Waiting for MQTT connection. This may take time.");
    xEventGroupWaitBits(mqtt_event_group, MQTT_CONNECTED_EVENT, false, true, portMAX_DELAY);
    return ESP_OK;
}

static void blinker_mqtt_unsubscribe_all()
{
    if (!mqtt_data) {
        return;
    }

    int i;

    for (i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (mqtt_data->subscriptions[i]) {
            blinker_mqtt_unsubscribe(mqtt_data->subscriptions[i]->topic);
        }
    }
}

esp_err_t blinker_mqtt_disconnect(void)
{
    if (!mqtt_data) {
        return ESP_FAIL;
    }

    blinker_mqtt_unsubscribe_all();
    esp_err_t err = esp_mqtt_client_stop(mqtt_data->mqtt_client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disconnect from MQTT");
    } else {
        ESP_LOGI(TAG, "MQTT Disconnected.");
    }

    return err;
}

esp_err_t blinker_mqtt_init(blinker_mqtt_config_t *config)
{
    if (mqtt_data) {
        ESP_LOGI(TAG, "MQTT already initialised");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initialising MQTT");
    mqtt_data = calloc(1, sizeof(blinker_mqtt_data_t));

    if (!mqtt_data) {
        // ESP_LOGI(TAG, "Initialising MQTT2");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Initialising MQTT config");

    mqtt_data->config = malloc(sizeof(blinker_mqtt_config_t));
    *mqtt_data->config = *config;

    const esp_mqtt_client_config_t mqtt_client_cfg = {
        .client_id      = config->client_id,
        .username       = config->username,
        .password       = config->password,
        .uri            = config->uri,
        .port           = config->port,
        .keepalive      = 30,
        .event_handle   = mqtt_event_handler_cb,
    };

    mqtt_data->mqtt_client = esp_mqtt_client_init(&mqtt_client_cfg);
    return ESP_OK;
}
