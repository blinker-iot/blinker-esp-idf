#pragma once

#include <esp_err.h>

#include "esp_http_client.h"

#ifdef __cplusplus
extern "C"
{
#endif

esp_http_client_handle_t blinker_http_init(esp_http_client_config_t *config);

esp_err_t blinker_http_set_url(esp_http_client_handle_t client, const char *url);

esp_err_t blinker_http_set_header(esp_http_client_handle_t client, const char *key, const char *value);

esp_err_t blinker_http_get(esp_http_client_handle_t client);

esp_err_t blinker_http_post(esp_http_client_handle_t client, const char *post_data);

esp_err_t blinker_http_read_response(esp_http_client_handle_t client, char *buffer, int len);

esp_err_t blinker_http_close(esp_http_client_handle_t client);

#ifdef __cplusplus
}
#endif
