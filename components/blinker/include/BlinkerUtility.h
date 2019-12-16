#ifndef BLINKER_UTILITY_H
#define BLINKER_UTILITY_H

#include "BlinkerDebug.h"

typedef void (*blinker_callback_t)(void);
typedef void (*blinker_callback_with_arg_t)(void*);
typedef void (*blinker_callback_with_json_arg_t)(const cJSON *data);
typedef void (*blinker_callback_with_string_arg_t)(const char *data);
typedef void (*blinker_callback_with_string_uint8_arg_t)(const char *data, uint8_t num);
typedef void (*blinker_callback_with_int32_arg_t)(int32_t data);
typedef void (*blinker_callback_with_uint8_arg_t)(uint8_t data);
typedef void (*blinker_callback_with_tab_arg_t)(uint8_t data);
typedef void (*blinker_callback_with_begin_t)(const char * k1, ...);
typedef void (*blinker_callback_with_rgb_arg_t)(uint8_t r_data, uint8_t g_data, uint8_t b_data, uint8_t bright_data);
typedef void (*blinker_callback_with_int32_uint8_arg_t)(int32_t data, uint8_t num);

char * macDeviceName(void);

uint32_t millis(void);

char * trim(char *data);

int8_t isJson(const char *data);

#endif
