#ifndef BLINKER_MQTT_H
#define BLINKER_MQTT_H

#include "BlinkerDebug.h"

// void blinker_spiffs_auth_check(void);

uint8_t blinker_auth_check(void);

void weather_data(blinker_callback_with_json_arg_t func);

void aqi_data(blinker_callback_with_json_arg_t func);

void blinker_set_auth(const char *_key, blinker_callback_with_string_arg_t _func);

void blinker_set_type_auth(const char *_type, const char *_key, blinker_callback_with_string_arg_t _func);

void wifi_init_sta(const char * _ssid, const char * _pswd);

void wifi_init_smart();

void wifi_init_ap();

void device_register(void);

void blinker_mqtt_init(void);

void websocket_init(void);

// void initialise_wifi(void);

char * mqtt_device_name(void);

char * mqtt_auth_key(void);

void ali_type(const char *type, blinker_callback_with_string_arg_t func);

void duer_type(const char *type, blinker_callback_with_string_arg_t func);

void mi_type(const char *type, blinker_callback_with_string_arg_t func);

int available(void);

char *last_read(void);

void flush(void);

void blinker_https_get(const char * _host, const char * _url);

void blinker_https_post(const char * _host, const char * _url, const char * _msg);

void blinker_server(uint8_t type);

int8_t blinker_print(char *data, uint8_t need_check);

int8_t blinker_ws_print(char *data);

int8_t blinker_mqtt_print(char *_data, uint8_t need_check);

int8_t blinker_aligenie_mqtt_print(char *data);

int8_t blinker_dueros_mqtt_print(char *data);

int8_t blinker_miot_mqtt_print(char *data);

void blinker_reset(void);

void device_get_auth(void);

#endif
