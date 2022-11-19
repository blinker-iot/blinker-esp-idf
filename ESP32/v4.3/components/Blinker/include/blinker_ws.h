#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <esp_http_server.h>

typedef struct {
    httpd_handle_t hd;
    int fd;
} async_resp_arg_t;

typedef void (*blinker_websocket_data_cb_t)(async_resp_arg_t *req, const char *payload);

esp_err_t blinker_websocket_async_print(async_resp_arg_t *arg, const char *payload);

esp_err_t blinker_websocket_print(httpd_req_t *req, const char *payload);

esp_err_t blinker_websocket_server_init(blinker_websocket_data_cb_t cb);

#ifdef __cplusplus
}
#endif
