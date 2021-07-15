#pragma once

#include <stdint.h>

#include <esp_err.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    BLINKER_BROKER_ALIYUN = 0,
    BLINKER_BROKER_BLINKER,
} blinker_mqtt_broker_t;

typedef struct {
    char *devicename;
    char *authkey;
    char *client_id;
    char *username;
    char *password;
    char *productkey;
    char *uuid;
    char *uri;
    int  port;
    blinker_mqtt_broker_t broker;
} blinker_mqtt_config_t;

typedef void (*blinker_mqtt_subscribe_cb_t)(const char *topic, size_t topic_len, void *payload, size_t payload_len);

blinker_mqtt_broker_t blinker_mqtt_broker(void);

const char *blinker_mqtt_devicename(void);

const char *blinker_mqtt_token(void);

const char *blinker_mqtt_uuid(void);

const char *blinker_mqtt_product_key(void);

const char *blinker_mqtt_authkey(void);

esp_err_t blinker_mqtt_subscribe(const char *topic, blinker_mqtt_subscribe_cb_t cb);

esp_err_t blinker_mqtt_unsubscribe(const char *topic);

esp_err_t blinker_mqtt_publish(const char *topic, const char *data, size_t data_len);

esp_err_t blinker_mqtt_connect(void);

esp_err_t blinker_mqtt_disconnect(void);

esp_err_t blinker_mqtt_init(blinker_mqtt_config_t *config);

#ifdef __cplusplus
}
#endif
