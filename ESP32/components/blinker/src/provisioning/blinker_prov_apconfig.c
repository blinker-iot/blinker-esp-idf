#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "blinker_prov_apconfig.h"

static const char *TAG = "blinker_prov_apconfig";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

esp_err_t blinker_prov_apconfig_init(void)
{
#ifndef CONFIG_IDF_TARGET_ESP8266
    esp_netif_create_default_wifi_ap();
#endif
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    uint8_t mac[6] = {0};
    char mac_str[31] = { 0 };
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
#if defined CONFIG_BLINKER_CUSTOM_ESP
    sprintf(mac_str, "DiyArduino_%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#elif defined CONFIG_BLINKER_PRO_ESP
    sprintf(mac_str, "%s_%02X%02X%02X%02X%02X%02X", CONFIG_BLINKER_TYPE_KEY,mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#endif

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(mac_str),
            .max_connection = 4,
            .channel = 1,
            .authmode = WIFI_AUTH_OPEN
        },
    };
    strcpy((char *)wifi_config.ap.ssid, mac_str);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "apconfig start");
    
    return ESP_OK;
}