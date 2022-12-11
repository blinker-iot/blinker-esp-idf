#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"

#ifndef CONFIG_IDF_TARGET_ESP8266
#include "esp_netif.h"
#endif

#include "blinker_wifi.h"
#include "blinker_config.h"
#include "blinker_storage.h"

static const char* TAG = "blinker_wifi";
static const int CONNECTED_BIT = BIT0;
static EventGroupHandle_t b_wifi_event_group = NULL;

static void event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        
        wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGE(TAG, "Disconnect reason : %d", disconnected->reason);
        if(disconnected->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT){
            ESP_LOGE(TAG, "wrong password");
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
#if CONFIG_IDF_TARGET_ESP8266
        ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
#else
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
#endif
        xEventGroupSetBits(b_wifi_event_group, CONNECTED_BIT);
    }
}

esp_err_t blinker_wifi_start(const wifi_config_t *wifi_config)
{
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, (wifi_config_t *)wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());
    
    xEventGroupWaitBits(b_wifi_event_group, CONNECTED_BIT,
                        pdFALSE,
                        pdFALSE,
                        portMAX_DELAY);
    return ESP_OK;
}

esp_err_t blinker_wifi_reset(void)
{
    esp_err_t err = ESP_FAIL;

    err = esp_wifi_restore();

    err = blinker_storage_erase(BLINKER_CMD_WIFI_CONFIG);

    return err;
}

esp_err_t blinker_wifi_init(void)
{
#if CONFIG_IDF_TARGET_ESP8266
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
#else
    esp_netif_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
#endif

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
#if CONFIG_IDF_TARGET_ESP8266
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
#endif
    b_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );

    return ESP_OK;
}