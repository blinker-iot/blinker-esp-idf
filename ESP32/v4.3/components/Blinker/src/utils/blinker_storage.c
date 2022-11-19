#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "esp_log.h"
#include "esp_system.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "blinker_storage.h"

static const char *TAG = "blinker_storage";

esp_err_t blinker_storage_init(void)
{
    static bool init_flag = false;

    if (!init_flag) {
        esp_err_t ret = nvs_flash_init();

        if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {// || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }

        ESP_ERROR_CHECK(ret);

        init_flag = true;
    }

    return ESP_OK;
}

esp_err_t blinker_storage_erase(const char *key)
{
    ESP_LOGI(TAG, "blinker_storage_erase: %s", key);

    esp_err_t ret    = ESP_OK;
    nvs_handle handle = 0;

    /**< Open non-volatile storage with a given namespace from the default NVS partition */
    ret = nvs_open(CONFIG_BLINKER_NVS_NAMESPACE, NVS_READWRITE, &handle);

    /**
     * @brief If key is CONFIG_BLINKER_NVS_NAMESPACE, erase all info in CONFIG_BLINKER_NVS_NAMESPACE
     */
    if (!strcmp(key, CONFIG_BLINKER_NVS_NAMESPACE)) {
        ret = nvs_erase_all(handle);
    } else {
        ret = nvs_erase_key(handle, key);
    }

    /**< Write any pending changes to non-volatile storage */
    nvs_commit(handle);

    /**< Close the storage handle and free any allocated resources */
    nvs_close(handle);

    return ret;
}

esp_err_t blinker_storage_set(const char *key, const void *value, size_t length)
{
    esp_err_t ret     = ESP_OK;
    nvs_handle handle = 0;

    /**< Open non-volatile storage with a given namespace from the default NVS partition */
    ret = nvs_open(CONFIG_BLINKER_NVS_NAMESPACE, NVS_READWRITE, &handle);

    /**< set variable length binary value for given key */
    ret = nvs_set_blob(handle, key, value, length);

    /**< Write any pending changes to non-volatile storage */
    nvs_commit(handle);

    /**< Close the storage handle and free any allocated resources */
    nvs_close(handle);

    return ret;
}

esp_err_t blinker_storage_get(const char *key, void *value, size_t length)
{
    esp_err_t ret     = ESP_OK;
    nvs_handle handle = 0;

    /**< Open non-volatile storage with a given namespace from the default NVS partition */
    ret = nvs_open(CONFIG_BLINKER_NVS_NAMESPACE, NVS_READWRITE, &handle);

    /**< get variable length binary value for given key */
    ret = nvs_get_blob(handle, key, value, &length);

    /**< Close the storage handle and free any allocated resources */
    nvs_close(handle);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "<ESP_ERR_NVS_NOT_FOUND> Get value for given key, key: %s", key);
        return ESP_ERR_NVS_NOT_FOUND;
    }

    return ESP_OK;
}
