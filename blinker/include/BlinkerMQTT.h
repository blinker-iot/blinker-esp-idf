#ifndef BLINKER_MQTT_H
#define BLINKER_MQTT_H

#include "BlinkerDebug.h"

void wifi_init_sta(const char * _key, const char * _ssid, const char * _pswd, blinker_callback_with_string_arg_t _func);

void wifi_init_smart(const char * _key);

void https_test(void);

void blinker_mqtt_init(void);

// void initialise_wifi();

int available(void);

char *last_read(void);

void flush(void);

int8_t blinker_mqtt_print(char *_data);

#endif
