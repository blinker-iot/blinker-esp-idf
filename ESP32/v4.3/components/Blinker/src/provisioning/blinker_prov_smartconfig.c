#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_smartconfig.h"
#if CONFIG_IDF_TARGET_ESP8266
#include "tcpip_adapter.h"
#include "smartconfig_ack.h"
#else
#include "esp_netif.h"
#endif
#include "blinker_prov_smartconfig.h"

static EventGroupHandle_t s_wifi_event_group;

static const int CONNECTED_BIT       = BIT0;
static bool b_prov_smartconfig_start = false;
static const char* TAG = "blinker_prov_smartconfig";
static void blinker_prov_smartconfig_start(void);

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
        // blinker_prov_smartconfig_start();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        // xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        // wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t*) event_data;
        // ESP_LOGE(TAG, "Disconnect reason : %d", disconnected->reason);
        // if(disconnected->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT){
        //     ESP_LOGE(TAG, "wrong password, need reset device");
        //     // esp_wifi_disconnect();
        //     // blinker_prov_smartconfig_stop();
        //     // vTaskDelay(10);
        //     // blinker_prov_smartconfig_start();
        //     // return;
        // }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
#if CONFIG_IDF_TARGET_ESP8266
        ESP_LOGI(TAG, "smartconfig connected, got ip:%s",
                ip4addr_ntoa(&event->ip_info.ip));
#else
        ESP_LOGI(TAG, "smartconfig connected, got ip:" IPSTR, IP2STR(&event->ip_info.ip));
#endif
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t* evt = (smartconfig_event_got_ssid_pswd_t*)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };
        uint8_t rvd_data[33] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;

        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);
        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
            ESP_LOGI(TAG, "RVD_DATA:%s", rvd_data);
        }

        // ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_connect());
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        // xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);

        blinker_prov_smartconfig_stop();
        ESP_LOGI(TAG, "blinker_prov_smartconfig_stop");
    }
}

esp_err_t blinker_prov_smartconfig_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    
    ESP_ERROR_CHECK(esp_wifi_start());

    blinker_prov_smartconfig_start();
    
    xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT, true, false, portMAX_DELAY);

    return ESP_OK;
}

static void blinker_prov_smartconfig_start(void)
{
    b_prov_smartconfig_start = true;
#if defined CONFIG_BLINKER_SMART_CONFIG
    ESP_ERROR_CHECK(esp_smartconfig_set_type(CONFIG_BLINKER_PROV_SMARTCONFIG_TYPE));
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
#endif
}

esp_err_t blinker_prov_smartconfig_stop(void)
{
    esp_err_t err = ESP_FAIL;

    if (b_prov_smartconfig_start) {
        err = esp_smartconfig_stop();

        b_prov_smartconfig_start = false;
    }

    return err;
}
