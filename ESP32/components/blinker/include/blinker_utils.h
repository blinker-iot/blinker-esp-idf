/*
 * @Author: your name
 * @Date: 2021-07-02 13:32:25
 * @LastEditTime: 2021-07-22 12:22:52
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ESP8266_RTOS_SDK\0524\smart_config\examples\components\blinker\include\blinker_utils.h
 */
#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLINKER_FREE(ptr) { \
        if (ptr) { \
            free(ptr); \
            ptr = NULL; \
        } \
    }

#define BLINKER_TS_NUM(num) data##num

void blinker_log_print_heap(void);

void blinker_mac_device_name(char *name);

time_t blinker_time(void);

esp_err_t blinker_timesync_start(void);

esp_err_t blinker_mdns_init(const char *host_name);

esp_err_t blinker_mdns_free(void);

int blinker_reboot_unbroken_count(void);

int blinker_reboot_total_count(void);

#ifdef __cplusplus
}
#endif
