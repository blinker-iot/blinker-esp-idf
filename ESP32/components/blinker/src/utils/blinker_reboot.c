#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include <esp_timer.h>
#include <esp_system.h>
#include "esp_sntp.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"

#include "blinker_utils.h"
#include "blinker_storage.h"

#if CONFIG_IDF_TARGET_ESP8266
#include "esp8266/rtc_register.h"
#elif CONFIG_IDF_TARGET_ESP32
#include "esp32/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/rtc.h"
#endif

#define RTC_RESET_SW_CAUSE_REG              RTC_STORE0
#define RTC_RESET_HW_CAUSE_REG              RTC_STATE1
#define RTC_WAKEUP_HW_CAUSE_REG             RTC_STATE2

#define RTC_RESET_HW_CAUSE_LSB              0
#define RTC_RESET_HW_CAUSE_MSB              3

#define REBOOT_RECORD_KEY                   "reboot_record"
#define CONFIG_UNBROKEN_RECORD_TASK_DEFAULT_PRIOTY (ESP_TASK_MAIN_PRIO + 1)

typedef struct  {
    size_t total_count;
    size_t unbroken_count;
    RESET_REASON reason;
} blinker_reboot_record_t;

static const char *TAG = "blinker_reboot";
static blinker_reboot_record_t g_reboot_record = {0};

static uint32_t esp_rtc_get_reset_reason(void)
{
#if CONFIG_IDF_TARGET_ESP8266
    return GET_PERI_REG_BITS(RTC_RESET_HW_CAUSE_REG, RTC_RESET_HW_CAUSE_MSB, RTC_RESET_HW_CAUSE_LSB);
#else
    return rtc_get_reset_reason(0);
#endif
}

static void esp_reboot_count_erase_timercb(void *priv)
{
    g_reboot_record.unbroken_count = 0;
    blinker_storage_set(REBOOT_RECORD_KEY, &g_reboot_record, sizeof(blinker_reboot_record_t));

    ESP_LOGI(TAG, "Erase reboot count");
}

static esp_err_t blinker_reboot_unbroken_init(void)
{
    esp_err_t err          = ESP_OK;
    g_reboot_record.reason = esp_rtc_get_reset_reason();

    blinker_storage_init();
    blinker_storage_get(REBOOT_RECORD_KEY, &g_reboot_record, sizeof(blinker_reboot_record_t));

    g_reboot_record.total_count++;

    /**< If the device reboots within the instruction time,
         the event_mdoe value will be incremented by one */
#if CONFIG_IDF_TARGET_ESP8266
    if (g_reboot_record.reason != DEEPSLEEP_RESET) {
#else
    if (g_reboot_record.reason != DEEPSLEEP_RESET && g_reboot_record.reason != RTCWDT_BROWN_OUT_RESET) {
#endif
        g_reboot_record.unbroken_count++;
        ESP_LOGI(TAG, "reboot unbroken count: %d", g_reboot_record.unbroken_count);
    } else {
        g_reboot_record.unbroken_count = 1;
        ESP_LOGI(TAG, "reboot reason: %d", g_reboot_record.reason);
    }

    err = blinker_storage_set(REBOOT_RECORD_KEY, &g_reboot_record, sizeof(blinker_reboot_record_t));

    esp_timer_handle_t time_handle   = NULL;
    esp_timer_create_args_t timer_cfg = {
        .name = "reboot_count_erase",
        .callback = esp_reboot_count_erase_timercb,
        .dispatch_method = ESP_TIMER_TASK,
    };

    err = esp_timer_create(&timer_cfg, &time_handle);
    err = esp_timer_start_once(time_handle, CONFIG_BLINKER_REBOOT_UNBROKEN_INTERVAL_TIMEOUT * 1000UL);

    return err;
}

static void reboot_unbroken_record_task(void *arg)
{
    blinker_reboot_unbroken_init();

    if (CONFIG_BLINKER_REBOOT_UNBROKEN_FALLBACK_COUNT &&
            blinker_reboot_unbroken_count() >= CONFIG_BLINKER_REBOOT_UNBROKEN_FALLBACK_COUNT) {
        // esp_ota_mark_app_invalid_rollback_and_reboot();
    }

    if (blinker_reboot_unbroken_count() >= CONFIG_BLINKER_REBOOT_UNBROKEN_FALLBACK_COUNT_RESET) {
        blinker_storage_erase(CONFIG_BLINKER_NVS_NAMESPACE);
    }

    ESP_LOGI(TAG, "version_fallback_task exit");

    vTaskDelete(NULL);
}

static void __attribute__((constructor)) blinker_reboot_unbroken_record(void)
{
    xTaskCreate(reboot_unbroken_record_task, 
                "reboot_unbroken_record", 
                4 * 1024,
                NULL, 
                CONFIG_UNBROKEN_RECORD_TASK_DEFAULT_PRIOTY, 
                NULL);
}

int blinker_reboot_unbroken_count(void)
{
    ESP_LOGI(TAG, "blinker_reboot_unbroken_count: %d", g_reboot_record.unbroken_count);
    return g_reboot_record.unbroken_count;
}

int blinker_reboot_total_count(void)
{
    return g_reboot_record.total_count;
}

// bool esp_qcloud_reboot_is_exception(bool erase_coredump)
// {
//     esp_err_t ret        = ESP_OK;
//     ssize_t coredump_len = 0;

//     const esp_partition_t *coredump_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
//                                            ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
//     ESP_QCLOUD_ERROR_CHECK(!coredump_part, false, "<%s> esp_partition_get fail", esp_err_to_name(ret));

//     ret = esp_partition_read(coredump_part, 0, &coredump_len, sizeof(ssize_t));
//     ESP_QCLOUD_ERROR_CHECK(ret, false, "<%s> esp_partition_read fail", esp_err_to_name(ret));

//     if (coredump_len <= 0) {
//         return false;
//     }

//     if (erase_coredump) {
//         /**< erase all coredump partition */
//         ret = esp_partition_erase_range(coredump_part, 0, coredump_part->size);
//         ESP_QCLOUD_ERROR_CHECK(ret, false, "<%s> esp_partition_erase_range fail", esp_err_to_name(ret));
//     }

//     return true;
// }