#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C"
{
#endif

esp_err_t blinker_storage_init(void);

esp_err_t blinker_storage_set(const char *key, const void *value, size_t length);

esp_err_t blinker_storage_get(const char *key, void *value, size_t length);

esp_err_t blinker_storage_erase(const char *key);

#ifdef __cplusplus
}
#endif
