#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"

#include "blinker_wifi.h"
#include "blinker_storage.h"
#include "blinker_prov_smartconfig.h"

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
        // ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s",
                ip4addr_ntoa(&event->ip_info.ip));
        xEventGroupSetBits(b_wifi_event_group, CONNECTED_BIT);
    }
}

static esp_err_t blinker_wifi_start(const wifi_config_t *wifi_config)
{
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, (wifi_config_t *)wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());
    
    xEventGroupWaitBits(b_wifi_event_group, CONNECTED_BIT,
                        pdFALSE,
                        pdFALSE,
                        portMAX_DELAY);
    return ESP_OK;
}

static esp_err_t blinker_wifi_get_config(wifi_config_t *wifi_config)
{
    if (blinker_storage_get("wifi_config", wifi_config, sizeof(wifi_config_t)) == ESP_OK) {
#if defined CONFIG_BLINKER_DEFAULT_CONFIG
        if (!strcmp((char *)wifi_config->sta.ssid, CONFIG_BLINKER_WIFI_SSID)) {
            return ESP_OK;
        }
#else
        return ESP_OK;
#endif
    }

    esp_wifi_restore();

#if defined CONFIG_BLINKER_DEFAULT_CONFIG
    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = CONFIG_BLINKER_WIFI_SSID,
            .password = CONFIG_BLINKER_WIFI_PASSWORD
        },
    };

    blinker_wifi_start(&wifi_cfg);
#elif defined CONFIG_BLINKER_SMART_CONFIG
    blinker_prov_smartconfig_init();
#endif

    esp_wifi_get_config(ESP_IF_WIFI_STA, wifi_config);
    blinker_storage_set("wifi_config", wifi_config, sizeof(wifi_config_t));

    return ESP_OK;
}

esp_err_t blinker_wifi_reset(void)
{
    esp_err_t err = ESP_FAIL;

    err = esp_wifi_restore();

    err = blinker_storage_erase("wifi_config");

    return err;
}

esp_err_t blinker_wifi_init(void)
{
    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    b_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );

    wifi_config_t wifi_cfg = {0};
    blinker_wifi_get_config(&wifi_cfg);
    blinker_wifi_start(&wifi_cfg);

    return ESP_OK;
}