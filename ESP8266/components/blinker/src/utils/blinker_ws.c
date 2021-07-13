#include <string.h>
#include <stdlib.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>

#include <esp_http_server.h>
#include "blinker_config.h"
#include "blinker_utils.h"

static const char *TAG       = "blinker_ws";

// /*
//  * Structure holding server handle
//  * and internal socket fd in order
//  * to use out of request send
//  */
// struct async_resp_arg {
//     httpd_handle_t hd;
//     int fd;
// };

// /*
//  * async send function, which we put into the httpd work queue
//  */
// static void ws_async_send(void *arg)
// {
//     static const char * data = "Async data";
//     struct async_resp_arg *resp_arg = arg;
//     httpd_handle_t hd = resp_arg->hd;
//     int fd = resp_arg->fd;
//     httpd_ws_frame_t ws_pkt;
//     memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
//     ws_pkt.payload = (uint8_t*)data;
//     ws_pkt.len = strlen(data);
//     ws_pkt.type = HTTPD_WS_TYPE_TEXT;

//     httpd_ws_send_frame_async(hd, fd, &ws_pkt);
//     free(resp_arg);
// }

// static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
// {
//     struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
//     resp_arg->hd = req->handle;
//     resp_arg->fd = httpd_req_to_sockfd(req);
//     return httpd_queue_work(handle, ws_async_send, resp_arg);
// }

static esp_err_t echo_handler(httpd_req_t *req)
{
    uint8_t buf[128] = { 0 };
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = buf;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 128);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "httpd_ws_recv_frame failed with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);
    // if (ws_pkt.type == HTTPD_WS_TYPE_TEXT &&
    //     strcmp((char*)ws_pkt.payload,"Trigger async") == 0) {
    //     return trigger_async_send(req->handle, req);
    // }

    // ret = httpd_ws_send_frame(req, &ws_pkt);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    // }
    return ret;
}

static esp_err_t ws_connect_handler(httpd_req_t *req)
{
    char *buf = "{\"state\":\"connected\"}\n";
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)buf;
    ws_pkt.len     = strlen(buf);
    ws_pkt.type    = HTTPD_WS_TYPE_TEXT;
    httpd_ws_send_frame(req, &ws_pkt);
    return ESP_OK;
}

static const httpd_uri_t ws = {
        .uri        = "/",
        .method     = HTTP_GET,
        .handler    = echo_handler,
        .connect_cb = ws_connect_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port    = BLINKER_WEBSOCKET_SERVER_PORT;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Registering the ws handler
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &ws);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

esp_err_t blinker_websocker_server_init(void)
{
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    server = start_webserver();

    return ESP_OK;
}