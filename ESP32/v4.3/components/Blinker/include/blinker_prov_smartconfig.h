#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C"
{
#endif

esp_err_t blinker_prov_smartconfig_init(void);

esp_err_t blinker_prov_smartconfig_stop(void);

#ifdef __cplusplus
}
#endif
