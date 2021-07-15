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

void blinker_log_print_heap(void);

esp_err_t blinker_timesync_start(void);

esp_err_t blinker_mdns_init(const char *host_name);

int blinker_reboot_unbroken_count(void);

int blinker_reboot_total_count(void);

#ifdef __cplusplus
}
#endif
