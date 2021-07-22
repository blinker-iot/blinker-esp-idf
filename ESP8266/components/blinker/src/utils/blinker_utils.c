#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_system.h>
#include "esp_wifi.h"

#include "esp_heap_caps.h"

#include "blinker_utils.h"

static const char *TAG = "blinker_utils";

void blinker_log_print_heap(void)
{
    ESP_LOGI(TAG, "Free heap, current: %d", esp_get_free_heap_size());
}

void blinker_mac_device_name(char *name)
{
    uint8_t mac[6] = { 0 };
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    sprintf(name, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}