#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_tls.h"

#include "blinker_http.h"

static const char *TAG = "blinker_http";

esp_http_client_handle_t blinker_http_init(esp_http_client_config_t *config)
{
    return esp_http_client_init(config);
}

esp_err_t blinker_http_set_url(esp_http_client_handle_t client, const char *url)
{
    return esp_http_client_set_url(client, url);
}

esp_err_t blinker_http_set_header(esp_http_client_handle_t client, const char *key, const char *value)
{
    return esp_http_client_set_header(client, key, value);
}

esp_err_t blinker_http_get(esp_http_client_handle_t client)
{
    esp_err_t err = ESP_FAIL;
    
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    err = esp_http_client_open(client, 0);

    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    } else {
        int content_length = esp_http_client_fetch_headers(client);
        if (content_length < 0) {
            ESP_LOGI(TAG, "HTTP client fetch headers failed");
            err = ESP_FAIL;
        } else {
            err = ESP_OK;
        }
    }

    return err;
}

esp_err_t blinker_http_post(esp_http_client_handle_t client, const char *post_data)
{
    esp_http_client_set_method(client, HTTP_METHOD_POST);

    ESP_LOGI(TAG, "HTTP POST: %s", post_data);

    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t blinker_http_read_response(esp_http_client_handle_t client, char *buffer, int len)
{
    int data_read = esp_http_client_read_response(client, buffer, len);
    
    if (data_read >= 0) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
        esp_http_client_get_status_code(client),
        esp_http_client_get_content_length(client));
        // ESP_LOG_BUFFER_HEX(TAG, buffer, strlen(buffer));

        return ESP_OK;
    } else {
        ESP_LOGI(TAG, "Failed to read response");

        return ESP_FAIL;
    }
}

esp_err_t blinker_http_close(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    return ESP_OK;
}