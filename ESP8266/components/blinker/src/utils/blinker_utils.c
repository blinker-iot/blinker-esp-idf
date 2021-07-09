#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_system.h>

#include "esp_heap_caps.h"

#include "blinker_utils.h"

static const char *TAG = "blinker_utils";

void blinker_log_print_heap(void)
{
    ESP_LOGI(TAG, "Free heap, current: %d", esp_get_free_heap_size());
}