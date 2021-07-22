#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "lwip/apps/sntp.h"
#include "blinker_utils.h"

#define REF_TIME        1577808000 /* 2020-01-01 00:00:00 */
#define DEFAULT_TICKS   (2000 / portTICK_PERIOD_MS) /* 2 seconds in ticks */
#define CONFIG_BLINKER_SNTP_SERVER_NAME "pool.ntp.org"

static const char *TAG        = "blinker_timesync";
// static const int NTP_DONE_BIT = BIT0;
static bool g_init_done       = false;
// static EventGroupHandle_t b_ntp_event_group = NULL;

time_t blinker_time(void)
{
    time_t now;
    time(&now);

    return now;
}

static bool blinker_timesync_check(void)
{
    time_t now;
    time(&now);

    if (now > REF_TIME) {
        return true;
    }

    return false;
}

static void blinker_timesync_task(void *arg)
{
    if (!g_init_done) {
        ESP_LOGW(TAG, "Time sync not initialised using 'esp_qcloud_timesync_start'");
    }

    for(;;) {
        if (blinker_timesync_check() == true) {
            break;
        }

        // ESP_LOGI(TAG, "Time not synchronized yet. Retrying...");
        vTaskDelay(DEFAULT_TICKS);
    }

    struct tm timeinfo;
    char strftime_buf[64];
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current UTC time is: %s", strftime_buf);
    // xEventGroupSetBits(b_ntp_event_group, NTP_DONE_BIT);

    vTaskDelete(NULL);
}

esp_err_t blinker_timesync_start(void)
{
    if (sntp_enabled()) {
        ESP_LOGI(TAG, "SNTP already initialized.");
        g_init_done = true;
        return ESP_OK;
    }

    // b_ntp_event_group = xEventGroupCreate();

    char *sntp_server_name = CONFIG_BLINKER_SNTP_SERVER_NAME;

    ESP_LOGI(TAG, "Initializing SNTP. Using the SNTP server: %s", sntp_server_name);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, sntp_server_name);
    setenv("TZ", "CST-8", 1);
    tzset();
    sntp_init();
    g_init_done = true;

    xTaskCreate(blinker_timesync_task, 
                "blinker_timesync",
                2 * 1024,
                NULL,
                6,
                NULL);

    // xEventGroupWaitBits(b_ntp_event_group, NTP_DONE_BIT,
    //                     pdFALSE,
    //                     pdFALSE,
    //                     portMAX_DELAY);

    return ESP_OK;
}