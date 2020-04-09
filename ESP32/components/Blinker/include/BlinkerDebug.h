#ifndef BLINKER_DEBUG_H
#define BLINKER_DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
// #include "ArduinoJson.h"
#include "esp_log.h"
#include "esp_system.h"

#include "BlinkerConfig.h"
#include "BlinkerUtility.h"

// char * macDeviceName();

// uint32_t millis();

void BLINKER_DEBUG_DISABLE(void);

void BLINKER_DEBUG_ALL(void);

int8_t isDebug(void);

int8_t isDebugAll(void);

// #define BLINKER_LOG( tag, format, ... ) if (isDebug()) { printf( "%s %s", tag, format ); }

// #define BLINKER_ERR_LOG( tag, format, ... ) if (isDebug()) { printf( "%s %s",tag, format ); }

// #define BLINKER_LOG_FreeHeap(tag) if (isDebug()) { printf( "%s Free memory: %d bytes", tag, esp_get_free_heap_size()); }

// #define BLINKER_LOG_ALL( tag, format, ... ) if (isDebugAll()) { BLINKER_LOG( tag, format ); }

// #define BLINKER_LOG_FreeHeap_ALL(tag) if (isDebugAll()) { printf( "%s Free memory: %d bytes", tag, esp_get_free_heap_size()); }

#define BLINKER_LOG( tag, format, ... ) if (isDebug()) { ESP_LOGI( tag, format, ##__VA_ARGS__ ); }

#define BLINKER_ERR_LOG( tag, format, ... ) if (isDebug()) { ESP_LOGE( tag, format, ##__VA_ARGS__ ); }

#define BLINKER_LOG_FreeHeap(tag) if (isDebug()) { ESP_LOGI( tag, "Free memory: %d bytes", esp_get_free_heap_size()); }

#define BLINKER_LOG_ALL( tag, format, ... ) if (isDebugAll()) { ESP_LOGI( tag, format, ##__VA_ARGS__ ); }

#define BLINKER_LOG_FreeHeap_ALL(tag) if (isDebugAll()) { ESP_LOGI( tag, "Free memory: %d bytes", esp_get_free_heap_size()); }

#endif
