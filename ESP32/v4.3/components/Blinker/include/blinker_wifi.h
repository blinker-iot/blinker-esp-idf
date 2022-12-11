#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C"
{
#endif

esp_err_t blinker_wifi_start(const wifi_config_t *wifi_config);

esp_err_t blinker_wifi_reset(void);

esp_err_t blinker_wifi_init(void);

#ifdef __cplusplus
}
#endif
