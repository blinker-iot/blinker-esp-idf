#include <string.h>
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "rom/ets_sys.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_system.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"

#include "BlinkerMQTT.h"

#include <sys/socket.h>
#include <netdb.h>

#include "lwip/apps/sntp.h"
#include "wolfssl/ssl.h"

// #include "mbedtls/platform.h"
// #include "mbedtls/net_sockets.h"
// #include "mbedtls/esp_debug.h"
// #include "mbedtls/ssl.h"
// #include "mbedtls/entropy.h"
// #include "mbedtls/ctr_drbg.h"
// #include "mbedtls/error.h"
// #include "mbedtls/certs.h"

#include "esp_http_client.h"
#include "mqtt_client.h"

#include "mdns.h"

// #include <sys/socket.h>
// #include <netdb.h>

#include "lwip/api.h"
#include "websocket_server.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_spiffs.h"

static QueueHandle_t client_queue;
const static int client_queue_size = 10;

static const char *TAG = "BlinkerMQTT";

static EventGroupHandle_t wifi_event_group;
static EventGroupHandle_t smart_event_group;
static EventGroupHandle_t ap_event_group;
// static EventGroupHandle_t register_event_group;
static EventGroupHandle_t http_event_group;

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static int AUTH_BIT = BIT0;
static const int REGISTER_BIT = BIT0;
int8_t spiffs_start = 0;

enum smartconfig_step_t
{
    sconf_ap_init,
    sconf_ap_connect,
    sconf_ap_connected,
    sconf_ap_disconnect,
    sconf_begin
};

enum smartconfig_step_t sconf_step = sconf_ap_init;

// #define EXAMPLE_ESP_WIFI_SSID      "MF"
// #define EXAMPLE_ESP_WIFI_PASS      "cd85586651"

#define WEB_SERVER "iot.diandeng.tech"
#define WEB_PORT "443"
#define WEB_URL "https://iot.diandeng.tech"

#define REQUEST "GET " WEB_URL " HTTP/1.0\r\n" \
    "Host: "WEB_SERVER"\r\n" \
    "User-Agent: esp-idf/1.0 espressif\r\n" \
    "\r\n"

#define WOLFSSL_DEMO_THREAD_NAME        "https_client"
#define WOLFSSL_DEMO_THREAD_STACK_WORDS 8192
#define WOLFSSL_DEMO_THREAD_PRORIOTY    6

#define WOLFSSL_DEMO_SNTP_SERVERS       "pool.ntp.org"

// const char send_data[] = REQUEST;
// const int32_t send_bytes = sizeof(send_data);

char* blinker_authkey;
char* blinker_auth;
char* blinker_type;
char* https_request_data;
int32_t https_request_bytes = 0;
uint8_t blinker_https_type = BLINKER_CMD_DEFAULT_NUMBER;
// char recv_data[1024] = {0};

char*       MQTT_HOST_MQTT;
char*       MQTT_ID_MQTT;
char*       MQTT_NAME_MQTT;
char*       MQTT_KEY_MQTT;
char*       MQTT_PRODUCTINFO_MQTT;
char*       UUID_MQTT;
char*       DEVICE_NAME_MQTT;
char*       BLINKER_PUB_TOPIC_MQTT;
char*       BLINKER_SUB_TOPIC_MQTT;
uint16_t    MQTT_PORT_MQTT;

char*       msgBuf_MQTT;
int8_t      isFresh_MQTT = 0;
int8_t      isAlive = 0;
int8_t      isConnect_MQTT = 0;
int8_t      isAvail_MQTT = 0;
uint8_t     ws_num_MQTT = 0;
uint8_t     respTimes = 0;
uint32_t    respTime = 0;
uint32_t    printTime = 0;
uint32_t    _print_time = 0;
uint8_t     _print_times = 0;
uint8_t     dataFrom_MQTT = BLINKER_MSG_FROM_MQTT;

int8_t      isMQTTinit = 0;
uint32_t    kaTime = 0;
const char* _aliType;
const char* _duerType;
const char* _miType;

uint32_t    aliKaTime = 0;
uint8_t     isAliAlive = 0;
uint8_t     isAliAvail = 0;
uint32_t    duerKaTime = 0;
uint8_t     isDuerAlive = 0;
uint8_t     isDuerAvail = 0;
uint32_t    miKaTime = 0;
uint8_t     isMIOTAlive = 0;
uint8_t     isMIOTAvail = 0;
uint8_t     respAliTimes = 0;
uint32_t    respAliTime = 0;
uint8_t     respDuerTimes = 0;
uint32_t    respDuerTime = 0;
uint8_t     respMIOTTimes = 0;
uint32_t    respMIOTTime = 0;

uint8_t isHello = 0;

#define MQTT_CLIENT_THREAD_NAME         "mqtt_client_thread"
#define MQTT_CLIENT_THREAD_STACK_WORDS  3072
#define MQTT_CLIENT_THREAD_PRIO         8

void smartconfig_task(void * parm);
void initialise_wifi();
void blinker_https_get(const char * _host, const char * _url);
blinker_callback_with_string_arg_t  data_parse_func = NULL;
blinker_callback_with_string_arg_t  aligenie_parse_func = NULL;
blinker_callback_with_string_arg_t  dueros_parse_func = NULL;
blinker_callback_with_string_arg_t  miot_parse_func = NULL;

blinker_callback_with_json_arg_t    _weather_func = NULL;
blinker_callback_with_json_arg_t    _aqi_func = NULL;

typedef struct
{
    const char *uuid;
} blinker_share_t;

blinker_share_t _sharers[BLINKER_MQTT_MAX_SHARERS_NUM];
uint8_t         _sharerCount = 0;
uint8_t         _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

#define CONFIG_MQTT_PAYLOAD_BUFFER 1460
// #define CONFIG_MQTT_BROKER 
// #define CONFIG_MQTT_PORT

uint8_t blinker_auth_check(void)
{
    static const char *TAGs = "blinker_auth_check";

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            BLINKER_LOG(TAGs, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            BLINKER_LOG(TAGs, "Failed to find SPIFFS partition");
        } else {
            BLINKER_LOG(TAGs, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return 0;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        BLINKER_LOG(TAGs, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        BLINKER_LOG(TAGs, "Partition size: total: %d, used: %d", total, used);
    }

    FILE* f;

    struct stat st;

    BLINKER_LOG(TAGs, "check auth");

    if (stat("/spiffs/auth.txt", &st) != 0)
    {
        BLINKER_LOG(TAGs, "not auth");

        // f = fopen("/spiffs/auth.txt", "w");
        // if (f == NULL) {
        //     ESP_LOGE(TAG, "Failed to open file for writing");
        //     return 0;
        // }
        // fclose(f);
        esp_vfs_spiffs_unregister(NULL);
        return 0;
    }
    else
    {
        BLINKER_LOG(TAGs, "authed, check auth user");

        f = fopen("/spiffs/auth.txt", "r");
        if (f == NULL) {
            BLINKER_LOG(TAGs, "Failed to open file for reading");
            esp_vfs_spiffs_unregister(NULL);
            return 0;
        }

        char line[64];
        fgets(line, sizeof(line), f);
        fclose(f);
        // strip newline
        char* pos = strchr(line, '\n');
        if (pos) {
            *pos = '\0';
        }
        BLINKER_LOG(TAGs, "Read from file: '%s'", line);

        esp_vfs_spiffs_unregister(NULL);
        return 1;
    }
}

// void auth_check_task(void* pv)
// {
//     blinker_auth_check();
// }

// void blinker_spiffs_auth_check(void)
// {
//     // xTaskCreate(&auth_check_task,
//     //             "auth_check_task",
//     //             4096,
//     //             NULL,
//     //             9,
//     //             NULL);
//     spiffs_start = 1;

//     // blinker_auth_check();

//     ESP_LOGI(TAG, "Initializing SPIFFS");

//     ESP_LOGI( TAG, "Free memory: %d bytes", esp_get_free_heap_size());
    
//     esp_vfs_spiffs_conf_t conf = {
//       .base_path = "/spiffs",
//       .partition_label = NULL,
//       .max_files = 5,
//       .format_if_mount_failed = true
//     };
    
//     // Use settings defined above to initialize and mount SPIFFS filesystem.
//     // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
//     esp_err_t ret = esp_vfs_spiffs_register(&conf);

//     ESP_LOGI( TAG, "Free memory: %d bytes", esp_get_free_heap_size());

//     if (ret != ESP_OK) {
//         if (ret == ESP_FAIL) {
//             ESP_LOGE(TAG, "Failed to mount or format filesystem");
//         } else if (ret == ESP_ERR_NOT_FOUND) {
//             ESP_LOGE(TAG, "Failed to find SPIFFS partition");
//         } else {
//             ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
//         }
//         return;
//     }
    
//     size_t total = 0, used = 0;
//     ret = esp_spiffs_info(NULL, &total, &used);
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
//     } else {
//         ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
//     }

//     // Use POSIX and C standard library functions to work with files.
//     // First create a file.
//     ESP_LOGI(TAG, "Opening file");
//     FILE* f = fopen("/spiffs/hello.txt", "w");
//     if (f == NULL) {
//         ESP_LOGE(TAG, "Failed to open file for writing");
//         return;
//     }
//     fprintf(f, "Hello World!\n");
//     fclose(f);
//     ESP_LOGI(TAG, "File written");

//     // Check if destination file exists before renaming
//     struct stat st;
//     if (stat("/spiffs/foo.txt", &st) == 0) {
//         // Delete it if it exists
//         unlink("/spiffs/foo.txt");
//     }

//     // Rename original file
//     ESP_LOGI(TAG, "Renaming file");
//     if (rename("/spiffs/hello.txt", "/spiffs/foo.txt") != 0) {
//         ESP_LOGE(TAG, "Rename failed");
//         return;
//     }

//     // Open renamed file for reading
//     ESP_LOGI(TAG, "Reading file");
//     f = fopen("/spiffs/foo.txt", "r");
//     if (f == NULL) {
//         ESP_LOGE(TAG, "Failed to open file for reading");
//         return;
//     }
//     char line[64];
//     fgets(line, sizeof(line), f);
//     fclose(f);
//     // strip newline
//     char* pos = strchr(line, '\n');
//     if (pos) {
//         *pos = '\0';
//     }
//     ESP_LOGI(TAG, "Read from file: '%s'", line);

//     // All done, unmount partition and disable SPIFFS
//     ESP_LOGI( TAG, "Free memory: %d bytes", esp_get_free_heap_size());

//     esp_vfs_spiffs_unregister(NULL);
//     ESP_LOGI(TAG, "SPIFFS unmounted");

//     for (;;)
//     {
//         if (spiffs_start == 0)
//             break;
        
//         vTaskDelay(10 / portTICK_RATE_MS);
//     }
// }

void blinker_auth_save(void)
{
    static const char *TAGs = "blinker_auth_check";

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            BLINKER_LOG(TAGs, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            BLINKER_LOG(TAGs, "Failed to find SPIFFS partition");
        } else {
            BLINKER_LOG(TAGs, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        BLINKER_LOG(TAGs, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        BLINKER_LOG(TAGs, "Partition size: total: %d, used: %d", total, used);
    }

    FILE* f;

    struct stat st;

    if (stat("/spiffs/auth.txt", &st) == 0) {
        return;
    }

    BLINKER_LOG(TAGs, "blinker auth need save uuid: %s", UUID_MQTT);

   
    BLINKER_LOG(TAGs, "store auth uuid");

    unlink("/spiffs/auth.txt");

    f = fopen("/spiffs/auth.txt", "w");
    if (f == NULL) {
        BLINKER_LOG(TAGs, "Failed to open file for writing");
        return;
    }
    fprintf(f, UUID_MQTT);
    fclose(f);
    esp_vfs_spiffs_unregister(NULL);
}

void blinker_spiffs_delete(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            BLINKER_LOG(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            BLINKER_LOG(TAG, "Failed to find SPIFFS partition");
        } else {
            BLINKER_LOG(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        BLINKER_LOG(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        BLINKER_LOG(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // FILE* f;

    struct stat st;

    if (stat("/spiffs/auth.txt", &st) == 0) {
        // Delete it if it exists
        unlink("/spiffs/auth.txt");
    }
    
    esp_vfs_spiffs_unregister(NULL);
}

void blinker_auth_task(void* pv)
{
    BLINKER_LOG(TAG, "blinker_auth_get");

    blinker_ws_print("{\"message\":\"success\"}\n");

    device_get_auth();
    // xEventGroupSetBits(register_event_group, REGISTER_BIT);

    vTaskDelete(NULL);
}

void blinker_auth_get(void)
{
    xTaskCreate(blinker_auth_task,
                "blinker_auth_task",
                2048,
                NULL,
                6,
                NULL);
}

void websocket_callback(uint8_t num,WEBSOCKET_TYPE_t type,char* msg,uint64_t len) {
    const static char* TAG = "websocket_callback";
    // int value;

    switch(type) {
        case WEBSOCKET_CONNECT:
            ESP_LOGI(TAG,"client %i connected!",num);
            // blinker_ws_print("{\"state\":\"connected\"}\n");
            break;
            case WEBSOCKET_DISCONNECT_EXTERNAL:
            ESP_LOGI(TAG,"client %i sent a disconnect message",num);
            //   led_duty(0);
            break;
        case WEBSOCKET_DISCONNECT_INTERNAL:
            ESP_LOGI(TAG,"client %i was disconnected",num);
            break;
        case WEBSOCKET_DISCONNECT_ERROR:
            ESP_LOGI(TAG,"client %i was disconnected due to an error",num);
            //   led_duty(0);
            break;
        case WEBSOCKET_TEXT:
            ESP_LOGI(TAG,"client %i sent text, len: %u, msg: %s", num, (uint32_t)len, (char *)msg);

            // blinker_ws_print("{\"state\":\"connected\"}\n");

            if (isFresh_MQTT) free(msgBuf_MQTT);

            // if (strncmp(BLINKER_SUB_TOPIC_MQTT, event->topic, event->topic_len) == 0)
            // {
            cJSON *root = cJSON_Parse((char *)msg);

            if (root != NULL)
            {

                // msgBuf_MQTT = (char *)malloc(((uint32_t)len + 1)*sizeof(char));
                // strcpy(msgBuf_MQTT, event->data);

                if (sconf_step == sconf_ap_connected)
                {
                    cJSON *_type = cJSON_GetObjectItemCaseSensitive(root, "register");

                    if (cJSON_IsString(_type) && (_type->valuestring != NULL))
                    {
                        #if defined (CONFIG_BLINKER_MODE_PRO)
                            if (strcmp(_type->valuestring, blinker_type) == 0)
                            {
                                BLINKER_LOG_ALL(TAG, "check register success.");

                                cJSON_Delete(_type);
                                cJSON_Delete(root);

                                blinker_auth_get();

                                return;

                                // xEventGroupSetBits(register_event_group, REGISTER_BIT);
                            }
                            else
                            {
                                cJSON_Delete(_type);
                                cJSON_Delete(root);
                                BLINKER_LOG_ALL(TAG, "check register failed.");
                            }
                        #else
                            cJSON_Delete(_type);
                            cJSON_Delete(root);
                        #endif
                    }
                    else
                    {
                        cJSON_Delete(root);

                        dataFrom_MQTT = BLINKER_MSG_FROM_WS;
                        
                        if (data_parse_func) data_parse_func((char *)msg);
                        kaTime = millis();
                        isAvail_MQTT = 1;
                        // isFresh_MQTT = 1;
                        isAlive = 1;
                    }
                }
                else
                {
                    cJSON *_ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
                    cJSON *_pswd = cJSON_GetObjectItemCaseSensitive(root, "pswd");

                    if (cJSON_IsString(_ssid) && (_ssid->valuestring != NULL))
                    {
                        mdns_free();

                        wifi_config_t wifi_config = {
                            .sta = {
                                .ssid = "",
                                .password = ""
                            },
                        };

                        strcpy((char *)wifi_config.sta.ssid, _ssid->valuestring);
                        
                        if (_pswd->valuestring != NULL)
                        {
                            strcpy((char *)wifi_config.sta.password, _pswd->valuestring);
                        }

                        sconf_step = sconf_ap_connect;

                        esp_wifi_deinit();

                        tcpip_adapter_init();

                        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

                        ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
                        // ESP_ERROR_CHECK( esp_wifi_disconnect() );
                        ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
                        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
                        ESP_ERROR_CHECK( esp_wifi_connect() );
                    }
                    cJSON_Delete(_ssid);
                    cJSON_Delete(_pswd);
                    cJSON_Delete(root);
                }
            }
            else
            {
                cJSON_Delete(root);
            }                
            
            // }
            // else
            // {
            //     BLINKER_ERR_LOG(TAG, "not from sub topic!");
            // }
            //   if(len) {
            //     switch(msg[0]) {
            //       case 'L':
            //         if(sscanf(msg,"L%i",&value)) {
            //           ESP_LOGI(TAG,"LED value: %i",value);
            //         //   led_duty(value);
            //           ws_server_send_text_all_from_callback(msg,len); // broadcast it!
            //         }
            //     }
            //   }
            break;
        case WEBSOCKET_BIN:
            ESP_LOGI(TAG,"client %i sent binary message of size %i:\n%s",num,(uint32_t)len,msg);
            break;
        case WEBSOCKET_PING:
            ESP_LOGI(TAG,"client %i pinged us with message of size %i:\n%s",num,(uint32_t)len,msg);
            break;
        case WEBSOCKET_PONG:
            ESP_LOGI(TAG,"client %i responded to the ping",num);
            break;
    }
}

// serves any clients
static void http_serve(struct netconn *conn) {
    const static char* TAG = "http_server";
    // const static char HTML_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
    // const static char ERROR_HEADER[] = "HTTP/1.1 404 Not Found\nContent-type: text/html\n\n";
    // const static char JS_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/javascript\n\n";
    // const static char CSS_HEADER[] = "HTTP/1.1 200 OK\nContent-type: text/css\n\n";
    //const static char PNG_HEADER[] = "HTTP/1.1 200 OK\nContent-type: image/png\n\n";
    // const static char ICO_HEADER[] = "HTTP/1.1 200 OK\nContent-type: image/x-icon\n\n";
    //const static char PDF_HEADER[] = "HTTP/1.1 200 OK\nContent-type: application/pdf\n\n";
    //const static char EVENT_HEADER[] = "HTTP/1.1 200 OK\nContent-Type: text/event-stream\nCache-Control: no-cache\nretry: 3000\n\n";
    struct netbuf* inbuf;
    static char* buf;
    static uint16_t buflen;
    static err_t err;

    // default page
    //   extern const uint8_t root_html_start[] asm("_binary_root_html_start");
    //   extern const uint8_t root_html_end[] asm("_binary_root_html_end");
    //   const uint32_t root_html_len = root_html_end - root_html_start;

    //   // test.js
    //   extern const uint8_t test_js_start[] asm("_binary_test_js_start");
    //   extern const uint8_t test_js_end[] asm("_binary_test_js_end");
    //   const uint32_t test_js_len = test_js_end - test_js_start;

    //   // test.css
    //   extern const uint8_t test_css_start[] asm("_binary_test_css_start");
    //   extern const uint8_t test_css_end[] asm("_binary_test_css_end");
    //   const uint32_t test_css_len = test_css_end - test_css_start;

    //   // favicon.ico
    //   extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
    //   extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");
    //   const uint32_t favicon_ico_len = favicon_ico_end - favicon_ico_start;

    //   // error page
    //   extern const uint8_t error_html_start[] asm("_binary_error_html_start");
    //   extern const uint8_t error_html_end[] asm("_binary_error_html_end");
    //   const uint32_t error_html_len = error_html_end - error_html_start;

    netconn_set_recvtimeout(conn,5000); // allow a connection timeout of 1 second
    ESP_LOGI(TAG,"reading from client...");
    err = netconn_recv(conn, &inbuf);
    ESP_LOGI(TAG,"read from client");
    if(err==ERR_OK) {
        netbuf_data(inbuf, (void**)&buf, &buflen);
        if(buf) {

        // default page
        if     (strstr(buf,"GET / ")
            && !strstr(buf,"Upgrade: websocket")) {
            ESP_LOGI(TAG,"Sending /");
            // netconn_write(conn, HTML_HEADER, sizeof(HTML_HEADER)-1,NETCONN_NOCOPY);
            // netconn_write(conn, root_html_start,root_html_len,NETCONN_NOCOPY);
            netconn_close(conn);
            netconn_delete(conn);
            netbuf_delete(inbuf);
        }

        // default page websocket
        else if(strstr(buf,"GET / ")
            && strstr(buf,"Upgrade: websocket")) {
            ESP_LOGI(TAG,"Requesting websocket on /");
            ws_server_add_client(conn,buf,buflen,"/",websocket_callback);

            blinker_ws_print("{\"state\":\"connected\"}\n");

            netbuf_delete(inbuf);            
        }

        else if(strstr(buf,"GET /test.js ")) {
            ESP_LOGI(TAG,"Sending /test.js");
            // netconn_write(conn, JS_HEADER, sizeof(JS_HEADER)-1,NETCONN_NOCOPY);
            // netconn_write(conn, test_js_start, test_js_len,NETCONN_NOCOPY);
            netconn_close(conn);
            netconn_delete(conn);
            netbuf_delete(inbuf);
        }

        else if(strstr(buf,"GET /test.css ")) {
            ESP_LOGI(TAG,"Sending /test.css");
            // netconn_write(conn, CSS_HEADER, sizeof(CSS_HEADER)-1,NETCONN_NOCOPY);
            // netconn_write(conn, test_css_start, test_css_len,NETCONN_NOCOPY);
            netconn_close(conn);
            netconn_delete(conn);
            netbuf_delete(inbuf);
        }

        else if(strstr(buf,"GET /favicon.ico ")) {
            ESP_LOGI(TAG,"Sending favicon.ico");
            // netconn_write(conn,ICO_HEADER,sizeof(ICO_HEADER)-1,NETCONN_NOCOPY);
            // netconn_write(conn,favicon_ico_start,favicon_ico_len,NETCONN_NOCOPY);
            netconn_close(conn);
            netconn_delete(conn);
            netbuf_delete(inbuf);
        }

        else if(strstr(buf,"GET /")) {
            // ESP_LOGI(TAG,"Unknown request, sending error page: %s",buf);
            // netconn_write(conn, ERROR_HEADER, sizeof(ERROR_HEADER)-1,NETCONN_NOCOPY);
            // netconn_write(conn, error_html_start, error_html_len,NETCONN_NOCOPY);
            netconn_close(conn);
            netconn_delete(conn);
            netbuf_delete(inbuf);
        }

        else {
            ESP_LOGI(TAG,"Unknown request");
            netconn_close(conn);
            netconn_delete(conn);
            netbuf_delete(inbuf);
        }
        }
        else {
        ESP_LOGI(TAG,"Unknown request (empty?...)");
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
        }
    }
    else { // if err==ERR_OK
        ESP_LOGI(TAG,"error on read, closing connection");
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
    }
}

// handles clients when they first connect. passes to a queue
static void server_task(void* pvParameters) {
    const static char* TAG = "server_task";
    struct netconn *conn, *newconn;
    static err_t err;
    client_queue = xQueueCreate(client_queue_size,sizeof(struct netconn*));

    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn,NULL,81);
    netconn_listen(conn);
    ESP_LOGI(TAG,"server listening");
    do {
        err = netconn_accept(conn, &newconn);
        ESP_LOGI(TAG,"new client");
        if(err == ERR_OK) {
            xQueueSendToBack(client_queue,&newconn,portMAX_DELAY);
            // http_serve(newconn);
        }
    } while(err == ERR_OK);
    netconn_close(conn);
    netconn_delete(conn);
    BLINKER_LOG_ALL(TAG,"task ending, rebooting board");
    esp_restart();
}

// receives clients from queue, handles them
static void server_handle_task(void* pvParameters) {
    const static char* TAG = "server_handle_task";
    struct netconn* conn;
    ESP_LOGI(TAG,"task starting");
    for(;;) {
        xQueueReceive(client_queue,&conn,portMAX_DELAY);
        if(!conn) continue;
        http_serve(conn);
    }
    vTaskDelete(NULL);
}

// static void count_task(void* pvParameters) {
//     const static char* TAG = "count_task";
//     char out[20];
//     int len;
//     int clients;
//     const static char* word = "%i";
//     uint8_t n = 0;
//     const int DELAY = 1000 / portTICK_PERIOD_MS; // 1 second

//     ESP_LOGI(TAG,"starting task");
//     for(;;) {
//         len = sprintf(out,word,n);
//         clients = ws_server_send_text_all(out,len);
//         if(clients > 0) {
//         //ESP_LOGI(TAG,"sent: \"%s\" to %i clients",out,clients);
//         }
//         n++;
//         vTaskDelay(DELAY);
//     }
// }

void weather_data(blinker_callback_with_json_arg_t func)
{
    _weather_func = func;
}

void aqi_data(blinker_callback_with_json_arg_t func)
{
    _aqi_func = func;
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;
    
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            if (sconf_step == sconf_begin)
            {
                BLINKER_LOG(TAG, "xTaskCreate smartconfig_task.");

                xTaskCreate(smartconfig_task, "smartconfig_task", 1024, NULL, 3, NULL);
            }
            else
            {
                BLINKER_LOG(TAG, "esp_wifi_connect.");

                esp_wifi_connect();
            }
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            BLINKER_LOG(TAG, "got ip:%s",
                    ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            #if defined (CONFIG_BLINKER_SMART_CONFIG)
                xEventGroupSetBits(smart_event_group, CONNECTED_BIT);
            #elif defined (CONFIG_BLINKER_AP_CONFIG)
                xEventGroupSetBits(ap_event_group, CONNECTED_BIT);
            #endif
            sconf_step = sconf_ap_connected;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            BLINKER_LOG_ALL(TAG, "Disconnect reason : %d", info->disconnected.reason);
            if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
                /*Switch to 802.11 bgn mode */
                esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
            }
            if (sconf_step == sconf_ap_connect)
            {
                sconf_step = sconf_ap_disconnect;

                xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
                initialise_wifi();
                xTaskCreate(smartconfig_task, "smartconfig_task", 1024, NULL, 3, NULL);
            }
            else
            {
                esp_wifi_connect();
                xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            }
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                    MAC2STR(event->event_info.sta_connected.mac),
                    event->event_info.sta_connected.aid);
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                    MAC2STR(event->event_info.sta_disconnected.mac),
                    event->event_info.sta_disconnected.aid);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void blinker_set_auth(const char * _key, blinker_callback_with_string_arg_t _func)
{
    blinker_authkey = (char *)malloc(strlen(_key)*sizeof(char));
    strcpy(blinker_authkey, _key);

    data_parse_func = _func;
}

void blinker_set_type_auth(const char * _type, const char * _key, blinker_callback_with_string_arg_t _func)
{
    blinker_type = (char *)malloc(strlen(_type)*sizeof(char));
    strcpy(blinker_type, _type);

    blinker_auth = (char *)malloc(strlen(_key)*sizeof(char));
    strcpy(blinker_auth, _key);

    data_parse_func = _func;
}

// void wifi_init_sta(const char * _key, const char * _ssid, const char * _pswd, blinker_callback_with_string_arg_t _func)
void wifi_init_sta(const char * _ssid, const char * _pswd)
{
    // blinker_authkey = (char *)malloc(strlen(_key)*sizeof(char));
    // strcpy(blinker_authkey, _key);
    // // blinker_authkey = _key;

    // data_parse_func = _func;

    ESP_ERROR_CHECK( nvs_flash_init() );

    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    // EXAMPLE_ESP_WIFI_SSID = _ssid;
    // EXAMPLE_ESP_WIFI_PASS = _pswd;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = ""
        },
    };
    // wifi_config_t wifi_config;
    strcpy((char *)wifi_config.sta.ssid, _ssid);
    strcpy((char *)wifi_config.sta.password, _pswd);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, true, false, portMAX_DELAY); 

    BLINKER_LOG(TAG, "wifi_init_sta finished.");
    BLINKER_LOG(TAG, "connect to ap SSID:%s password:%s",
            (char *)wifi_config.sta.ssid, (char *)wifi_config.sta.password);
}

// void wifi_init_smart(const char * _key, blinker_callback_with_string_arg_t _func)
void wifi_init_smart()
{
    // blinker_authkey = (char *)malloc(strlen(_key)*sizeof(char));
    // strcpy(blinker_authkey, _key);
    // // blinker_authkey = _key;

    // data_parse_func = _func;

    ESP_ERROR_CHECK( nvs_flash_init() );

    wifi_event_group = xEventGroupCreate();

    smart_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    sconf_step = sconf_begin;

    wifi_config_t wifi_config;
    esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);

    BLINKER_LOG_ALL(TAG, "wifi_config: %s", (char *)wifi_config.sta.ssid);
    if (strlen((char *)wifi_config.sta.ssid) > 1)
    {
        sconf_step = sconf_ap_connect;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    xEventGroupWaitBits(smart_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    BLINKER_LOG_ALL(TAG, "wifi_init_smart finished.");

    #if defined (CONFIG_BLINKER_MODE_PRO)
        BLINKER_LOG_ALL(TAG, "mdns_init. %s", macDeviceName());

        ESP_ERROR_CHECK( mdns_init() );
        // set mDNS hostname (required if you want to advertise services)
        ESP_ERROR_CHECK( mdns_hostname_set(macDeviceName()) );
        // set default mDNS instance name
        ESP_ERROR_CHECK( mdns_instance_name_set(macDeviceName()) );

        // structure with TXT records
        mdns_txt_item_t serviceTxtData[1] = {
            {"deviceName", macDeviceName()}
        };

        //initialize service
        ESP_ERROR_CHECK( mdns_service_add(macDeviceName(), "_blinker", "_tcp", 81, serviceTxtData, 1) );
    #endif
    // BLINKER_LOG_ALL(TAG, "connect to ap SSID:%s password:%s",
    //          EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

// void wifi_init_ap(const char * _key, blinker_callback_with_string_arg_t _func)
void wifi_init_ap()
{
    // blinker_authkey = (char *)malloc(strlen(_key)*sizeof(char));
    // strcpy(blinker_authkey, _key);

    // data_parse_func = _func;

    wifi_event_group = xEventGroupCreate();

    ap_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    sconf_step = sconf_begin;    

    wifi_config_t _wifi_config;
    esp_wifi_get_config(ESP_IF_WIFI_STA, &_wifi_config);

    BLINKER_LOG_ALL(TAG, "wifi_config: %s", (char *)_wifi_config.sta.ssid);
    if (strlen((char *)_wifi_config.sta.ssid) > 1)
    {
        sconf_step = sconf_ap_connect;
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK(esp_wifi_start() );

        websocket_init();
    }
    else
    {
        char ap_ssid[30];
        strcpy(ap_ssid, "DiyArduino_");
        strcat(ap_ssid, macDeviceName());
        wifi_config_t wifi_config = {
            .ap = {
                .ssid = "",
                .ssid_len = 0,
                .password = "",
                .max_connection = 4,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK
            },
        };    
        strcpy((char *)wifi_config.ap.ssid, ap_ssid);
        wifi_config.ap.ssid_len = strlen(ap_ssid);
        // if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        // }

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "wifi_init_softap SSID:%s ", ap_ssid);
        
        BLINKER_LOG_ALL(TAG, "mdns_init. %s", ap_ssid);
        // initialize mDNS
        ESP_ERROR_CHECK( mdns_init() );
        // set mDNS hostname (required if you want to advertise services)
        ESP_ERROR_CHECK( mdns_hostname_set(ap_ssid) );
        // set default mDNS instance name
        ESP_ERROR_CHECK( mdns_instance_name_set(ap_ssid) );

        // structure with TXT records
        mdns_txt_item_t serviceTxtData[1] = {
            {"deviceName", ap_ssid}
        };

        //initialize service
        ESP_ERROR_CHECK( mdns_service_add(DEVICE_NAME_MQTT, "_blinker", "_tcp", 81, serviceTxtData, 1) );

        websocket_init();
    }

    ESP_LOGI(TAG, "wifi_init_softap");
    
    xEventGroupWaitBits(ap_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    
    ESP_LOGI(TAG, "wifi_init_softap finished.");
}

void sc_callback(smartconfig_status_t status, void *pdata)
{
    switch (status) {
        case SC_STATUS_WAIT:
            BLINKER_LOG_ALL(TAG, "SC_STATUS_WAIT");
            break;
        case SC_STATUS_FIND_CHANNEL:
            BLINKER_LOG_ALL(TAG, "SC_STATUS_FINDING_CHANNEL");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            BLINKER_LOG_ALL(TAG, "SC_STATUS_GETTING_SSID_PSWD");
            break;
        case SC_STATUS_LINK:
            BLINKER_LOG_ALL(TAG, "SC_STATUS_LINK");
            wifi_config_t *wifi_config = pdata;
            BLINKER_LOG_ALL(TAG, "SSID:%s", wifi_config->sta.ssid);
            BLINKER_LOG_ALL(TAG, "PASSWORD:%s", wifi_config->sta.password);
            ESP_ERROR_CHECK( esp_wifi_disconnect() );
            ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config) );
            ESP_ERROR_CHECK( esp_wifi_connect() );
            break;
        case SC_STATUS_LINK_OVER:
            BLINKER_LOG_ALL(TAG, "SC_STATUS_LINK_OVER");
            if (pdata != NULL) {
                sc_callback_data_t *sc_callback_data = (sc_callback_data_t *)pdata;
                switch (sc_callback_data->type) {
                    case SC_ACK_TYPE_ESPTOUCH:
                        BLINKER_LOG_ALL(TAG, "Phone ip: %d.%d.%d.%d", sc_callback_data->ip[0], sc_callback_data->ip[1], sc_callback_data->ip[2], sc_callback_data->ip[3]);
                        BLINKER_LOG_ALL(TAG, "TYPE: ESPTOUCH");
                        break;
                    case SC_ACK_TYPE_AIRKISS:
                        BLINKER_LOG_ALL(TAG, "TYPE: AIRKISS");
                        break;
                    default:
                        BLINKER_LOG_ALL(TAG, "TYPE: ERROR");
                        break;
                }
            }
            xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
            break;
        default:
            break;
    }
}

void smartconfig_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS) );
    ESP_ERROR_CHECK( esp_smartconfig_start(sc_callback) );
    while (1) {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY); 
        if(uxBits & CONNECTED_BIT) {
            BLINKER_LOG_ALL(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            BLINKER_LOG_ALL(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

void initialise_wifi(void)
{
    esp_wifi_deinit();

    sconf_step = sconf_begin;

    BLINKER_LOG_ALL(TAG, "initialise_wifi");

    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    // ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void initialise_mdns(void)
{    
    BLINKER_LOG_ALL(TAG, "mdns_init. %s", DEVICE_NAME_MQTT);
    // initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    // set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(DEVICE_NAME_MQTT) );
    // set default mDNS instance name
    ESP_ERROR_CHECK( mdns_instance_name_set(DEVICE_NAME_MQTT) );

    // structure with TXT records
    mdns_txt_item_t serviceTxtData[1] = {
        {"deviceName", DEVICE_NAME_MQTT}
    };

    //initialize service
    ESP_ERROR_CHECK( mdns_service_add(DEVICE_NAME_MQTT, "_blinker", "_tcp", 81, serviceTxtData, 1) );
    //add another TXT item
    // ESP_ERROR_CHECK( mdns_service_txt_item_set("_blinker", "_tcp", "deviceName", DEVICE_NAME_MQTT) );
    //change TXT item value
    // ESP_ERROR_CHECK( mdns_service_txt_item_set("_http", "_tcp", "u", "admin") );

    // websocket_init();
    // vTaskDelete(wstask);

    // ws_server_start();
    // xTaskCreate(&server_task,"server_task",1536,NULL,9,NULL);
    // xTaskCreate(&server_handle_task,"server_handle_task",2048,NULL,6,NULL);
    // xTaskCreate(&blinker_websocket_server,"blinker_websocket_server",6000,NULL,2,NULL);
    // vTaskDelete(wstask);
}

void websocket_init(void)
{
    ws_server_start();
    xTaskCreate(&server_task,"server_task",1536,NULL,9,NULL);
    xTaskCreate(&server_handle_task,"server_handle_task",1536,NULL,6,NULL);

    BLINKER_LOG_FreeHeap(TAG);

    // #if defined (CONFIG_BLINKER_AP_CONFIG)
    //     xEventGroupWaitBits(ap_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    // #endif
    // xTaskCreate(&blinker_websocket_server,"blinker_websocket_server",8192,NULL,2,NULL);
}

void get_time(void)
{
    struct timeval now;
    int sntp_retry_cnt = 0;
    int sntp_retry_time = 0;

    sntp_setoperatingmode(0);
    sntp_setservername(0, "ntp1.aliyun.com");
    sntp_setservername(1, "120.25.108.11");
    sntp_setservername(2, "time.pool.aliyun.com");
    sntp_init();

    while (1) {
        for (int32_t i = 0; (i < (SNTP_RECV_TIMEOUT / 100)) && now.tv_sec < 1525952900; i++) {
            vTaskDelay(100 / portTICK_RATE_MS);
            gettimeofday(&now, NULL);
        }

        if (now.tv_sec < 1525952900) {
            sntp_retry_time = SNTP_RECV_TIMEOUT << sntp_retry_cnt;

            if (SNTP_RECV_TIMEOUT << (sntp_retry_cnt + 1) < SNTP_RETRY_TIMEOUT_MAX) {
                sntp_retry_cnt ++;
            }

            BLINKER_LOG_ALL(TAG, "SNTP get time failed, retry after %d ms\n", sntp_retry_time);
            vTaskDelay(sntp_retry_time / portTICK_RATE_MS);
        } else {
            BLINKER_LOG_ALL(TAG, "SNTP get time success\n");
            break;
        }
    }
}

void register_warn()
{
    BLINKER_ERR_LOG(TAG, "Maybe you have put in the wrong AuthKey!");
    BLINKER_ERR_LOG(TAG, "Or maybe your request is too frequently!");
    BLINKER_ERR_LOG(TAG, "Or maybe your network is disconnected!");
}

void https_delay(uint8_t _seconds)
{
    for (int countdown = _seconds; countdown >= 0; countdown--) {
        BLINKER_LOG_ALL(TAG, "%d...\n", countdown);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    BLINKER_LOG_ALL(TAG, "Starting again!\n");
}

int8_t check_register_data(const char * _data)
{
    BLINKER_LOG_ALL(TAG, "check_register_data");

    cJSON *root = cJSON_Parse(_data);

    if (root == NULL) 
    {
        cJSON_Delete(root);
        return 0;
    }

    cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);

    if (detail == NULL)
    {
        cJSON_Delete(root);
        return 0;
    }

    cJSON *_userID = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_DEVICENAME);
    
    if (_userID == NULL) 
    {
        cJSON_Delete(root);
        return 0;
    }

    cJSON_Delete(root);
    return 1;
}

    // char payload[1024] = {0};
    // uint8_t need_read = 0;
    // uint16_t check_num = 0;

// void mbedtls_https_client(void)
// {
//     char recv_data[1024] = {0};
//     char payload[1024] = {0};
//     uint8_t need_read = 0;
//     uint16_t check_num = 0;

//     char buf[1024];
//     int ret, flags, len;

//     mbedtls_entropy_context entropy;
//     mbedtls_ctr_drbg_context ctr_drbg;
//     mbedtls_ssl_context ssl;
//     mbedtls_x509_crt cacert;
//     mbedtls_ssl_config conf;
//     mbedtls_net_context server_fd;

//     mbedtls_ssl_init(&ssl);
//     mbedtls_x509_crt_init(&cacert);
//     mbedtls_ctr_drbg_init(&ctr_drbg);
//     BLINKER_LOG_ALL(TAG, "Seeding the random number generator");

//     mbedtls_ssl_config_init(&conf);

//     mbedtls_entropy_init(&entropy);
//     if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
//                                     NULL, 0)) != 0)
//     {
//         BLINKER_LOG_ALL(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
//         abort();
//     }

//     BLINKER_LOG_ALL(TAG, "Loading the CA root certificate...");

//     // ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start,
//     //                              server_root_cert_pem_end-server_root_cert_pem_start);

//     // if(ret < 0)
//     // {
//     //     ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
//     //     abort();
//     // }

//     BLINKER_LOG_ALL(TAG, "Setting hostname for TLS session...");

//      /* Hostname set here should match CN in server certificate */
//     if((ret = mbedtls_ssl_set_hostname(&ssl, BLINKER_SERVER_HOST)) != 0)
//     {
//         BLINKER_LOG_ALL(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
//         abort();
//     }

//     BLINKER_LOG_ALL(TAG, "Setting up the SSL/TLS structure...");

//     if((ret = mbedtls_ssl_config_defaults(&conf,
//                                           MBEDTLS_SSL_IS_CLIENT,
//                                           MBEDTLS_SSL_TRANSPORT_STREAM,
//                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
//     {
//         BLINKER_LOG_ALL(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
//         goto exit;
//     }

//     /* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
//        a warning if CA verification fails but it will continue to connect.

//        You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
//     */
//     mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
//     mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
//     mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
// #ifdef CONFIG_MBEDTLS_DEBUG
//     mbedtls_esp_enable_debug_log(&conf, 4);
// #endif

//     if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
//     {
//         BLINKER_LOG_ALL(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
//         goto exit;
//     }

//     while(1) {
//         /* Wait for the callback to set the CONNECTED_BIT in the
//            event group.
//         */
//         // xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
//         //                     false, true, portMAX_DELAY);
//         // BLINKER_LOG_ALL(TAG, "Connected to AP");

//         mbedtls_net_init(&server_fd);

//         BLINKER_LOG_ALL(TAG, "Connecting to %s:%s...", BLINKER_SERVER_HOST, BLINKER_SERVER_PORT);

//         if ((ret = mbedtls_net_connect(&server_fd, BLINKER_SERVER_HOST,
//                                       BLINKER_SERVER_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
//         {
//             BLINKER_LOG_ALL(TAG, "mbedtls_net_connect returned -%x", -ret);
//             goto exit;
//         }

//         BLINKER_LOG_ALL(TAG, "Connected.");

//         mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

//         BLINKER_LOG_ALL(TAG, "Performing the SSL/TLS handshake...");

//         while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
//         {
//             if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
//             {
//                 BLINKER_LOG_ALL(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
//                 goto exit;
//             }
//         }

//         BLINKER_LOG_ALL(TAG, "Verifying peer X.509 certificate...");

//         if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
//         {
//             /* In real life, we probably want to close connection if ret != 0 */
//             ESP_LOGW(TAG, "Failed to verify peer certificate!");
//             bzero(buf, sizeof(buf));
//             mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
//             ESP_LOGW(TAG, "verification info: %s", buf);
//         }
//         else {
//             BLINKER_LOG_ALL(TAG, "Certificate verified.");
//         }

//         BLINKER_LOG_ALL(TAG, "Cipher suite is %s", mbedtls_ssl_get_ciphersuite(&ssl));

//         BLINKER_LOG_ALL(TAG, "Writing HTTP request...");

//         size_t written_bytes = 0;
//         do {
//             ret = mbedtls_ssl_write(&ssl,
//                                     (const unsigned char *)https_request_data + written_bytes,
//                                     strlen(https_request_data) - written_bytes);
//             if (ret >= 0) {
//                 BLINKER_LOG_ALL(TAG, "%d bytes written", ret);
//                 written_bytes += ret;
//             } else if (ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_WANT_READ) {
//                 BLINKER_LOG_ALL(TAG, "mbedtls_ssl_write returned -0x%x", -ret);
//                 goto exit;
//             }
//         } while(written_bytes < strlen(https_request_data));

//         BLINKER_LOG_ALL(TAG, "Reading HTTP response...");

//         do
//         {
//             len = sizeof(buf) - 1;
//             bzero(buf, sizeof(buf));
//             ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);

//             if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
//                 continue;

//             if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
//                 ret = 0;
//                 break;
//             }

//             if(ret < 0)
//             {
//                 BLINKER_LOG_ALL(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
//                 break;
//             }

//             if(ret == 0)
//             {
//                 need_read = 0;
//                 BLINKER_LOG_ALL(TAG, "connection closed");
//                 break;
//             }

//             len = ret;
//             BLINKER_LOG_ALL(TAG, "%d bytes read", len);
//             // BLINKER_LOG_ALL(TAG, "need_read:%d", need_read);
//             /* Print response directly to stdout as it is read */
//             for(int i = 0; i < len; i++) {
//                 putchar(buf[i]);
//                 if (need_read) payload[check_num] = buf[i];
//                 check_num++;
//                 if (buf[i] == '\n')
//                 {
//                     if (check_num == 2)
//                     {
//                         BLINKER_LOG_ALL(TAG, "headers received");
//                         need_read = 1;
//                     }
//                     // BLINKER_LOG_ALL(TAG, "check_num: %d", check_num);
//                     check_num = 0;
//                 }
//             }
//         } while(1);

//         mbedtls_ssl_close_notify(&ssl);

//     exit:
//         mbedtls_ssl_session_reset(&ssl);
//         mbedtls_net_free(&server_fd);

//         if(ret != 0)
//         {
//             mbedtls_strerror(ret, buf, 100);
//             BLINKER_LOG_ALL(TAG, "Last error was: -0x%x - %s", -ret, buf);
//         }

//         putchar('\n'); // JSON output doesn't have a newline at end

//         // static int request_count;
//         // BLINKER_LOG_ALL(TAG, "Completed %d requests", ++request_count);

//         // for(int countdown = 10; countdown >= 0; countdown--) {
//         //     BLINKER_LOG_ALL(TAG, "%d...", countdown);
//         //     vTaskDelay(1000 / portTICK_PERIOD_MS);
//         // }
//         // BLINKER_LOG_ALL(TAG, "Starting again!");

//         BLINKER_LOG_ALL(TAG, "reply was:");
//         BLINKER_LOG_ALL(TAG, "==============================");
//         BLINKER_LOG_ALL(TAG, "payload: %s", payload);
//         BLINKER_LOG_ALL(TAG, "==============================");

//         switch (blinker_https_type)
//         {
//             case BLINKER_CMD_WEATHER_NUMBER :
//                 if (https_request_bytes != 0)
//                 {
//                     free(https_request_data);
//                     https_request_bytes = 0;
//                 }

//                 BLINKER_LOG_ALL(TAG, "BLINKER_CMD_WEATHER_NUMBER");

//                 if (isJson(payload))
//                 {
//                     cJSON *root = cJSON_Parse(payload);

//                     cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);

//                     if (detail != NULL && cJSON_IsObject(detail))
//                     {
//                         if (_weather_func) _weather_func(detail);
//                     }

//                     cJSON_Delete(root);
//                 }

//                 vTaskDelete(NULL);
//                 return;
//             case BLINKER_CMD_AQI_NUMBER :
//                 if (https_request_bytes != 0)
//                 {
//                     free(https_request_data);
//                     https_request_bytes = 0;
//                 }

//                 BLINKER_LOG_ALL(TAG, "BLINKER_CMD_AQI_NUMBER");

//                 if (isJson(payload))
//                 {
//                     cJSON *root = cJSON_Parse(payload);

//                     cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);

//                     if (detail != NULL && cJSON_IsObject(detail))
//                     {
//                         if (_aqi_func) _aqi_func(detail);
//                     }

//                     cJSON_Delete(root);
//                 }

//                 vTaskDelete(NULL);
//                 return;
//             case BLINKER_CMD_FRESH_SHARERS_NUMBER :
//                 if (https_request_bytes != 0)
//                 {
//                     free(https_request_data);
//                     https_request_bytes = 0;
//                 }

//                 BLINKER_LOG_ALL(TAG, "BLINKER_CMD_FRESH_SHARERS_NUMBER");

//                 if (isJson(payload))
//                 {
//                     cJSON *root = cJSON_Parse(payload);

//                     cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);

//                     if (detail != NULL && cJSON_IsObject(detail))
//                     {
//                         cJSON *users = cJSON_GetObjectItemCaseSensitive(detail, "users");

//                         if (users != NULL && cJSON_IsArray(users))
//                         {
//                             uint8_t arry_size = cJSON_GetArraySize(users);
                            
//                             _sharerCount = arry_size;

//                             BLINKER_LOG_ALL(TAG, "sharers arry size: %d", arry_size);
//                         }
//                     }

//                     cJSON_Delete(root);
//                 }
                
//                 vTaskDelete(NULL);
//                 return;
//             case BLINKER_CMD_DEVICE_REGISTER_NUMBER :
//                 BLINKER_LOG_ALL(TAG, "BLINKER_CMD_DEVICE_REGISTER_NUMBER");

//                 if (check_register_data(payload))
//                 {
//                     cJSON *root = cJSON_Parse(payload);
                
//                     cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);
//                     cJSON *_userID = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_DEVICENAME);
//                     cJSON *_userName = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_IOTID);
//                     cJSON *_key = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_IOTTOKEN);
//                     cJSON *_productInfo = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_PRODUCTKEY);
//                     cJSON *_broker = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_BROKER);
//                     cJSON *_uuid = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_UUID);

//                     // if (cJSON_IsString(_userID) && (_userID->valuestring != NULL))
//                     // {
//                     //     BLINKER_LOG_ALL(TAG, "_userId: %s", _userID->valuestring);
//                     // }

//                     if (https_request_bytes != 0)
//                     {
//                         free(https_request_data);
//                         https_request_bytes = 0;
//                     }

//                     if (isMQTTinit)
//                     {
//                         free(MQTT_HOST_MQTT);
//                         free(MQTT_ID_MQTT);
//                         free(MQTT_NAME_MQTT);
//                         free(MQTT_KEY_MQTT);
//                         free(MQTT_PRODUCTINFO_MQTT);
//                         free(UUID_MQTT);
//                         free(DEVICE_NAME_MQTT);
//                         free(BLINKER_PUB_TOPIC_MQTT);
//                         free(BLINKER_SUB_TOPIC_MQTT);

//                         mdns_free();

//                         isMQTTinit = 0;
//                     }

//                     if (strcmp(_broker->valuestring, BLINKER_MQTT_BORKER_ALIYUN) == 0)
//                     {
//                         BLINKER_LOG_ALL(TAG, "broker is aliyun");

//                         DEVICE_NAME_MQTT = (char*)malloc((strlen(_userID->valuestring)+1)*sizeof(char));
//                         strcpy(DEVICE_NAME_MQTT, _userID->valuestring);
//                         MQTT_ID_MQTT = (char*)malloc((strlen(_userID->valuestring)+1)*sizeof(char));
//                         strcpy(MQTT_ID_MQTT, _userID->valuestring);
//                         MQTT_NAME_MQTT = (char*)malloc((strlen(_userName->valuestring)+1)*sizeof(char));
//                         strcpy(MQTT_NAME_MQTT, _userName->valuestring);
//                         MQTT_KEY_MQTT = (char*)malloc((strlen(_key->valuestring)+1)*sizeof(char));
//                         strcpy(MQTT_KEY_MQTT, _key->valuestring);
//                         MQTT_PRODUCTINFO_MQTT = (char*)malloc((strlen(_productInfo->valuestring)+1)*sizeof(char));
//                         strcpy(MQTT_PRODUCTINFO_MQTT, _productInfo->valuestring);
//                         MQTT_HOST_MQTT = (char*)malloc((strlen(BLINKER_MQTT_ALIYUN_HOST)+1)*sizeof(char));
//                         strcpy(MQTT_HOST_MQTT, BLINKER_MQTT_ALIYUN_HOST);
//                         MQTT_PORT_MQTT = BLINKER_MQTT_ALIYUN_PORT;

//                         BLINKER_SUB_TOPIC_MQTT = (char*)malloc((1 + strlen(MQTT_PRODUCTINFO_MQTT) + 
//                                             1 + strlen(MQTT_ID_MQTT) + 3)*sizeof(char));
//                         strcpy(BLINKER_SUB_TOPIC_MQTT, "/");
//                         strcat(BLINKER_SUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT);
//                         strcat(BLINKER_SUB_TOPIC_MQTT, "/");
//                         strcat(BLINKER_SUB_TOPIC_MQTT, MQTT_ID_MQTT);                
//                         strcat(BLINKER_SUB_TOPIC_MQTT, "/r");

//                         BLINKER_PUB_TOPIC_MQTT = (char*)malloc((1 + strlen(MQTT_PRODUCTINFO_MQTT) + 
//                                             1 + strlen(MQTT_ID_MQTT) + 3)*sizeof(char));
//                         strcpy(BLINKER_PUB_TOPIC_MQTT, "/");
//                         strcat(BLINKER_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT);
//                         strcat(BLINKER_PUB_TOPIC_MQTT, "/");
//                         strcat(BLINKER_PUB_TOPIC_MQTT, MQTT_ID_MQTT);                
//                         strcat(BLINKER_PUB_TOPIC_MQTT, "/s");
//                     }

//                     UUID_MQTT = (char*)malloc((strlen(_uuid->valuestring)+1)*sizeof(char));
//                     strcpy(UUID_MQTT, _uuid->valuestring);

//                     BLINKER_LOG_ALL(TAG, "====================");
//                     BLINKER_LOG_ALL(TAG, "DEVICE_NAME_MQTT: %s", DEVICE_NAME_MQTT);
//                     BLINKER_LOG_ALL(TAG, "MQTT_PRODUCTINFO_MQTT: %s", MQTT_PRODUCTINFO_MQTT);
//                     BLINKER_LOG_ALL(TAG, "MQTT_ID_MQTT: %s", MQTT_ID_MQTT);
//                     BLINKER_LOG_ALL(TAG, "MQTT_NAME_MQTT: %s", MQTT_NAME_MQTT);
//                     BLINKER_LOG_ALL(TAG, "MQTT_KEY_MQTT: %s", MQTT_KEY_MQTT);
//                     BLINKER_LOG_ALL(TAG, "MQTT_BROKER: %s", _broker->valuestring);
//                     BLINKER_LOG_ALL(TAG, "HOST: %s", MQTT_HOST_MQTT);
//                     BLINKER_LOG_ALL(TAG, "PORT: %d", MQTT_PORT_MQTT);
//                     BLINKER_LOG_ALL(TAG, "UUID_MQTT: %s", UUID_MQTT);
//                     BLINKER_LOG_ALL(TAG, "BLINKER_SUB_TOPIC_MQTT: %s", BLINKER_SUB_TOPIC_MQTT);
//                     BLINKER_LOG_ALL(TAG, "BLINKER_PUB_TOPIC_MQTT: %s", BLINKER_PUB_TOPIC_MQTT);
//                     BLINKER_LOG_ALL(TAG, "====================");
                    
//                     isMQTTinit = 1;

//                     xEventGroupSetBits(http_event_group, isMQTTinit);
                    
//                     cJSON_Delete(root);

//                     BLINKER_LOG_FreeHeap(TAG);

//                     // wolfSSL_shutdown(ssl);

//                     // // wolfSSL_free(ssl);

//                     // close(socket);

//                     // wolfSSL_CTX_free(ctx);

//                     // wolfSSL_Cleanup();

//                     // return;
                    
//                     vTaskDelete(NULL);
//                     return;
//                 }
//                 break;
//             case BLINKER_CMD_DEVICE_AUTH_NUMBER :
//                 BLINKER_LOG_ALL(TAG, "BLINKER_CMD_DEVICE_AUTH_NUMBER");

//                 // if (check_register_data(payload))
//                 // {
//                     cJSON *root = cJSON_Parse(payload);
                
//                     cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);
//                     cJSON *_auth = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_AUTHKEY);

//                     if (https_request_bytes != 0)
//                     {
//                         free(https_request_data);
//                         https_request_bytes = 0;
//                     }

//                     if (cJSON_IsString(_auth) && (_auth->valuestring != NULL))
//                     {
//                         mdns_free(); 

//                         blinker_authkey = (char *)malloc(strlen(_auth->valuestring)*sizeof(char));
//                         strcpy(blinker_authkey, _auth->valuestring);

//                         AUTH_BIT = BIT1;
//                         // xEventGroupSetBits(http_event_group, AUTH_BIT);
                        
//                         cJSON_Delete(root);

//                         BLINKER_LOG_FreeHeap(TAG);

//                         // wolfSSL_shutdown(ssl);

//                         // // wolfSSL_free(ssl);

//                         // close(socket);

//                         // wolfSSL_CTX_free(ctx);

//                         // wolfSSL_Cleanup();

//                         // return;
                        
//                         vTaskDelete(NULL);
//                         return;
//                     }
//                 // }
//                 break;
//             default :
//                 if (https_request_bytes != 0)
//                 {
//                     free(https_request_data);
//                     https_request_bytes = 0;
//                 }

//                 BLINKER_LOG_ALL(TAG, "default");

//                 if (isJson(payload))
//                 {
//                     cJSON *root = cJSON_Parse(payload);

//                     cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);

//                     if (detail != NULL && cJSON_IsString(detail))
//                     {
//                         BLINKER_ERR_LOG(TAG, "%s", detail->valuestring);
//                     }

//                     cJSON_Delete(root);
//                 }
                
//                 vTaskDelete(NULL);
//                 return;
//         }
//         https_delay(20);
//     }
// }

void wolfssl_https_client(void)
{
    int32_t ret = 0;

    const portTickType xDelay = 500 / portTICK_RATE_MS;
    WOLFSSL_CTX* ctx = NULL;
    WOLFSSL* ssl = NULL;

    int32_t socket = -1;
    struct sockaddr_in sock_addr;
    struct hostent* entry = NULL;

    /* CA date verification need system time */
    get_time();

    char recv_data[1024] = {0};
    char payload[1024] = {0};
    uint8_t need_read = 0;
    uint16_t check_num = 0;

    while (1) {

        BLINKER_LOG_ALL(TAG, "Setting hostname for TLS session...\n");

        /*get addr info for hostname*/
        do {
            entry = gethostbyname(BLINKER_SERVER_HOST);
            vTaskDelay(xDelay);
        } while (entry == NULL);

        BLINKER_LOG_ALL(TAG, "Init wolfSSL...\n");
        ret = wolfSSL_Init();

        if (ret != WOLFSSL_SUCCESS) {
            BLINKER_LOG_ALL(TAG, "Init wolfSSL failed:%d...\n", ret);
            goto failed1;
        }

        BLINKER_LOG_ALL(TAG, "Set wolfSSL ctx ...\n");
        ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());

        if (!ctx) {
            BLINKER_LOG_ALL(TAG, "Set wolfSSL ctx failed...\n");
            goto failed1;
        }

        BLINKER_LOG_ALL(TAG, "Creat socket ...\n");
        socket = socket(AF_INET, SOCK_STREAM, 0);

        if (socket < 0) {
            BLINKER_LOG_ALL(TAG, "Creat socket failed...\n");
            goto failed2;
        }

#if CONFIG_CERT_AUTH
        BLINKER_LOG_ALL(TAG, "Loading the CA root certificate...\n");
        ret = wolfSSL_CTX_load_verify_buffer(ctx, server_root_cert_pem_start, server_root_cert_pem_end - server_root_cert_pem_start, WOLFSSL_FILETYPE_PEM);

        if (WOLFSSL_SUCCESS != ret) {
            BLINKER_LOG_ALL(TAG, "Loading the CA root certificate failed...\n");
            goto failed3;
        }

        wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER, NULL);
#else
        wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_NONE, NULL);
#endif

        memset(&sock_addr, 0, sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(BLINKER_SERVER_PORT);
        sock_addr.sin_addr.s_addr = ((struct in_addr*)(entry->h_addr))->s_addr;

        BLINKER_LOG_ALL(TAG, "Connecting to %s:%d...\n", BLINKER_SERVER_HOST, BLINKER_SERVER_PORT);
        ret = connect(socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr));

        if (ret) {
            BLINKER_LOG_ALL(TAG, "Connecting to %s:%d failed: %d\n", BLINKER_SERVER_HOST, BLINKER_SERVER_PORT, ret);
            goto failed3;
        }

        BLINKER_LOG_ALL(TAG, "Create wolfSSL...\n");
        ssl = wolfSSL_new(ctx);

        if (!ssl) {
            BLINKER_LOG_ALL(TAG, "Create wolfSSL failed...\n");
            goto failed3;
        }

        wolfSSL_set_fd(ssl, socket);

        BLINKER_LOG_ALL(TAG, "Performing the SSL/TLS handshake...\n");
        ret = wolfSSL_connect(ssl);

        if (WOLFSSL_SUCCESS != ret) {
            BLINKER_LOG_ALL(TAG, "Performing the SSL/TLS handshake failed:%d\n", ret);
            goto failed4;
        }

        BLINKER_LOG_ALL(TAG, "Writing HTTPS request...\n");
        ret = wolfSSL_write(ssl, https_request_data, https_request_bytes);

        if (ret <= 0) {
            BLINKER_LOG_ALL(TAG, "Writing HTTPS request failed:%d\n", ret);
            goto failed5;
        }

        BLINKER_LOG_ALL(TAG, "Reading HTTPS response...\n");

        do {
            ret = wolfSSL_read(ssl, recv_data, sizeof(recv_data));


            if (ret <= 0) {
                BLINKER_LOG_ALL(TAG, "\nConnection closed\n");

                // BLINKER_LOG_ALL(TAG, "payload: %s", payload);
                // BLINKER_LOG_FreeHeap(TAG);

                need_read = 0;

                break;
            }

            BLINKER_LOG_ALL(TAG, "ret: %d", ret);

            /* Print response directly to stdout as it is read */
            BLINKER_LOG_ALL(TAG, "%s", recv_data);
            for (int i = 0; i < ret; i++) {
                // BLINKER_LOG_ALL(TAG, "%c", recv_data[i]);
                if (need_read) payload[check_num] = recv_data[i];
                check_num++;
                if (recv_data[i] == '\n')
                {
                    if (check_num == 2)
                    {
                        BLINKER_LOG_ALL(TAG, "headers received");
                        need_read = 1;

                        wolfSSL_shutdown(ssl);
                        close(socket);
                    }
                    // BLINKER_LOG_ALL(TAG, "check_num: %d", check_num);
                    check_num = 0;
                }
            }
        } while (1);

failed5:
        wolfSSL_shutdown(ssl);
failed4:
        wolfSSL_free(ssl);
failed3:
        close(socket);
failed2:
        wolfSSL_CTX_free(ctx);
failed1:
        wolfSSL_Cleanup();

        BLINKER_LOG_ALL(TAG, "reply was:");
        BLINKER_LOG_ALL(TAG, "==============================");
        BLINKER_LOG_ALL(TAG, "payload: %s", payload);
        BLINKER_LOG_ALL(TAG, "==============================");

        switch (blinker_https_type)
        {
            case BLINKER_CMD_WEATHER_NUMBER :
                if (https_request_bytes != 0)
                {
                    free(https_request_data);
                    https_request_bytes = 0;
                }

                BLINKER_LOG_ALL(TAG, "BLINKER_CMD_WEATHER_NUMBER");

                if (isJson(payload))
                {
                    cJSON *root = cJSON_Parse(payload);

                    cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);

                    if (detail != NULL && cJSON_IsObject(detail))
                    {
                        if (_weather_func) _weather_func(detail);
                    }

                    cJSON_Delete(root);
                }

                vTaskDelete(NULL);
                return;
            case BLINKER_CMD_AQI_NUMBER :
                if (https_request_bytes != 0)
                {
                    free(https_request_data);
                    https_request_bytes = 0;
                }

                BLINKER_LOG_ALL(TAG, "BLINKER_CMD_AQI_NUMBER");

                if (isJson(payload))
                {
                    cJSON *root = cJSON_Parse(payload);

                    cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);

                    if (detail != NULL && cJSON_IsObject(detail))
                    {
                        if (_aqi_func) _aqi_func(detail);
                    }

                    cJSON_Delete(root);
                }

                vTaskDelete(NULL);
                return;
            case BLINKER_CMD_FRESH_SHARERS_NUMBER :
                if (https_request_bytes != 0)
                {
                    free(https_request_data);
                    https_request_bytes = 0;
                }

                BLINKER_LOG_ALL(TAG, "BLINKER_CMD_FRESH_SHARERS_NUMBER");

                if (isJson(payload))
                {
                    cJSON *root = cJSON_Parse(payload);

                    cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);

                    if (detail != NULL && cJSON_IsObject(detail))
                    {
                        cJSON *users = cJSON_GetObjectItemCaseSensitive(detail, "users");

                        if (users != NULL && cJSON_IsArray(users))
                        {
                            uint8_t arry_size = cJSON_GetArraySize(users);
                            
                            _sharerCount = arry_size;

                            BLINKER_LOG_ALL(TAG, "sharers arry size: %d", arry_size);
                        }
                    }

                    cJSON_Delete(root);
                }
                
                vTaskDelete(NULL);
                return;
            case BLINKER_CMD_DEVICE_REGISTER_NUMBER :
                BLINKER_LOG_ALL(TAG, "BLINKER_CMD_DEVICE_REGISTER_NUMBER");

                if (check_register_data(payload))
                {
                    cJSON *root = cJSON_Parse(payload);
                
                    cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);
                    cJSON *_userID = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_DEVICENAME);
                    cJSON *_userName = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_IOTID);
                    cJSON *_key = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_IOTTOKEN);
                    cJSON *_productInfo = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_PRODUCTKEY);
                    cJSON *_broker = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_BROKER);
                    cJSON *_uuid = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_UUID);

                    // if (cJSON_IsString(_userID) && (_userID->valuestring != NULL))
                    // {
                    //     BLINKER_LOG_ALL(TAG, "_userId: %s", _userID->valuestring);
                    // }

                    if (https_request_bytes != 0)
                    {
                        free(https_request_data);
                        https_request_bytes = 0;
                    }

                    if (isMQTTinit)
                    {
                        free(MQTT_HOST_MQTT);
                        free(MQTT_ID_MQTT);
                        free(MQTT_NAME_MQTT);
                        free(MQTT_KEY_MQTT);
                        free(MQTT_PRODUCTINFO_MQTT);
                        free(UUID_MQTT);
                        free(DEVICE_NAME_MQTT);
                        free(BLINKER_PUB_TOPIC_MQTT);
                        free(BLINKER_SUB_TOPIC_MQTT);

                        mdns_free();

                        isMQTTinit = 0;
                    }

                    if (strcmp(_broker->valuestring, BLINKER_MQTT_BORKER_ALIYUN) == 0)
                    {
                        BLINKER_LOG_ALL(TAG, "broker is aliyun");

                        DEVICE_NAME_MQTT = (char*)malloc((strlen(_userID->valuestring)+1)*sizeof(char));
                        strcpy(DEVICE_NAME_MQTT, _userID->valuestring);
                        MQTT_ID_MQTT = (char*)malloc((strlen(_userID->valuestring)+1)*sizeof(char));
                        strcpy(MQTT_ID_MQTT, _userID->valuestring);
                        MQTT_NAME_MQTT = (char*)malloc((strlen(_userName->valuestring)+1)*sizeof(char));
                        strcpy(MQTT_NAME_MQTT, _userName->valuestring);
                        MQTT_KEY_MQTT = (char*)malloc((strlen(_key->valuestring)+1)*sizeof(char));
                        strcpy(MQTT_KEY_MQTT, _key->valuestring);
                        MQTT_PRODUCTINFO_MQTT = (char*)malloc((strlen(_productInfo->valuestring)+1)*sizeof(char));
                        strcpy(MQTT_PRODUCTINFO_MQTT, _productInfo->valuestring);
                        MQTT_HOST_MQTT = (char*)malloc((strlen(BLINKER_MQTT_ALIYUN_HOST)+1)*sizeof(char));
                        strcpy(MQTT_HOST_MQTT, BLINKER_MQTT_ALIYUN_HOST);
                        MQTT_PORT_MQTT = BLINKER_MQTT_ALIYUN_PORT;

                        BLINKER_SUB_TOPIC_MQTT = (char*)malloc((1 + strlen(MQTT_PRODUCTINFO_MQTT) + 
                                            1 + strlen(MQTT_ID_MQTT) + 3)*sizeof(char));
                        strcpy(BLINKER_SUB_TOPIC_MQTT, "/");
                        strcat(BLINKER_SUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT);
                        strcat(BLINKER_SUB_TOPIC_MQTT, "/");
                        strcat(BLINKER_SUB_TOPIC_MQTT, MQTT_ID_MQTT);                
                        strcat(BLINKER_SUB_TOPIC_MQTT, "/r");

                        BLINKER_PUB_TOPIC_MQTT = (char*)malloc((1 + strlen(MQTT_PRODUCTINFO_MQTT) + 
                                            1 + strlen(MQTT_ID_MQTT) + 3)*sizeof(char));
                        strcpy(BLINKER_PUB_TOPIC_MQTT, "/");
                        strcat(BLINKER_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT);
                        strcat(BLINKER_PUB_TOPIC_MQTT, "/");
                        strcat(BLINKER_PUB_TOPIC_MQTT, MQTT_ID_MQTT);                
                        strcat(BLINKER_PUB_TOPIC_MQTT, "/s");
                    }

                    UUID_MQTT = (char*)malloc((strlen(_uuid->valuestring)+1)*sizeof(char));
                    strcpy(UUID_MQTT, _uuid->valuestring);

                    BLINKER_LOG_ALL(TAG, "====================");
                    BLINKER_LOG_ALL(TAG, "DEVICE_NAME_MQTT: %s", DEVICE_NAME_MQTT);
                    BLINKER_LOG_ALL(TAG, "MQTT_PRODUCTINFO_MQTT: %s", MQTT_PRODUCTINFO_MQTT);
                    BLINKER_LOG_ALL(TAG, "MQTT_ID_MQTT: %s", MQTT_ID_MQTT);
                    BLINKER_LOG_ALL(TAG, "MQTT_NAME_MQTT: %s", MQTT_NAME_MQTT);
                    BLINKER_LOG_ALL(TAG, "MQTT_KEY_MQTT: %s", MQTT_KEY_MQTT);
                    BLINKER_LOG_ALL(TAG, "MQTT_BROKER: %s", _broker->valuestring);
                    BLINKER_LOG_ALL(TAG, "HOST: %s", MQTT_HOST_MQTT);
                    BLINKER_LOG_ALL(TAG, "PORT: %d", MQTT_PORT_MQTT);
                    BLINKER_LOG_ALL(TAG, "UUID_MQTT: %s", UUID_MQTT);
                    BLINKER_LOG_ALL(TAG, "BLINKER_SUB_TOPIC_MQTT: %s", BLINKER_SUB_TOPIC_MQTT);
                    BLINKER_LOG_ALL(TAG, "BLINKER_PUB_TOPIC_MQTT: %s", BLINKER_PUB_TOPIC_MQTT);
                    BLINKER_LOG_ALL(TAG, "====================");

                    #if defined (CONFIG_BLINKER_MODE_PRO)
                        blinker_auth_save();

                        vTaskDelay(1000 / portTICK_RATE_MS);
                    #endif
                    
                    isMQTTinit = 1;
                    
                    cJSON_Delete(root);

                    BLINKER_LOG_FreeHeap(TAG);

                    initialise_mdns();

                    vTaskDelay(1000 / portTICK_RATE_MS);

                    xEventGroupSetBits(http_event_group, isMQTTinit);

                    // wolfSSL_shutdown(ssl);

                    // // wolfSSL_free(ssl);

                    // close(socket);

                    // wolfSSL_CTX_free(ctx);

                    // wolfSSL_Cleanup();

                    // return;
                    
                    vTaskDelete(NULL);
                    return;
                }
                break;
            case BLINKER_CMD_DEVICE_AUTH_NUMBER :
                BLINKER_LOG_ALL(TAG, "BLINKER_CMD_DEVICE_AUTH_NUMBER");

                // if (check_register_data(payload))
                // {
                    cJSON *root = cJSON_Parse(payload);
                
                    cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);
                    cJSON *_auth = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_AUTHKEY);

                    if (https_request_bytes != 0)
                    {
                        free(https_request_data);
                        https_request_bytes = 0;
                    }

                    if (cJSON_IsString(_auth) && (_auth->valuestring != NULL))
                    {
                        mdns_free(); 

                        blinker_authkey = (char *)malloc(strlen(_auth->valuestring)*sizeof(char));
                        strcpy(blinker_authkey, _auth->valuestring);

                        AUTH_BIT = BIT1;
                        // xEventGroupSetBits(http_event_group, AUTH_BIT);
                        
                        cJSON_Delete(root);

                        BLINKER_LOG_FreeHeap(TAG);

                        // wolfSSL_shutdown(ssl);

                        // // wolfSSL_free(ssl);

                        // close(socket);

                        // wolfSSL_CTX_free(ctx);

                        // wolfSSL_Cleanup();

                        // return;
                        
                        vTaskDelete(NULL);
                        return;
                    }
                // }
                break;
            default :
                if (https_request_bytes != 0)
                {
                    free(https_request_data);
                    https_request_bytes = 0;
                }

                BLINKER_LOG_ALL(TAG, "default");

                if (isJson(payload))
                {
                    cJSON *root = cJSON_Parse(payload);

                    cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);

                    if (detail != NULL && cJSON_IsString(detail))
                    {
                        BLINKER_ERR_LOG(TAG, "%s", detail->valuestring);
                    }

                    cJSON_Delete(root);
                }
                
                vTaskDelete(NULL);
                return;
        }

        https_delay(20);
    }
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                // BLINKER_LOG_ALL(TAG, "%.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

void blinker_https_get(const char * _host, const char * _url)
{
    BLINKER_LOG_FreeHeap(TAG);

    if (https_request_bytes != 0) free(https_request_data);

    char _data[256];

    strcpy(_data, "GET https://");
    strcat(_data, _host);
    strcat(_data, _url);
    strcat(_data, " HTTP/1.0\r\n");
    strcat(_data, "Host: ");
    strcat(_data, _host);
    strcat(_data, "\r\nUser-Agent: blinker\r\n\r\n");

    https_request_bytes = strlen(_data);

    https_request_data = (char *)malloc(https_request_bytes);
    strcpy(https_request_data, _data);

    BLINKER_LOG_ALL(TAG, "http data: %s, len: %d", https_request_data, https_request_bytes);
}

void blinker_https_post(const char * _host, const char * _url, const char * _msg)
{
    BLINKER_LOG_FreeHeap(TAG);

    if (https_request_bytes != 0) free(https_request_data);

    char _data[256];

    strcpy(_data, "POST https://");
    strcat(_data, _host);
    strcat(_data, _url);
    strcat(_data, " HTTP/1.0\r\n");
    strcat(_data, "Host: ");
    strcat(_data, _host);
    strcat(_data, "\r\nUser-Agent: blinker");    
    strcat(_data, "\r\nContent-Type: application/json;charset=utf-8\r\n");
    
    char _num[6] = {0};
    sprintf(_num, "%d", strlen(_msg));
    strcat(_data, "Content-Length: ");
    strcat(_data, _num);
    strcat(_data, "\r\nConnection: Keep-Alive\r\n\r\n");

    https_request_bytes = strlen(_data) + strlen(_msg);

    https_request_data = (char *)malloc(https_request_bytes);
    strcpy(https_request_data, _data);
    strcat(https_request_data, _msg);

    BLINKER_LOG_ALL(TAG, "blinker_https_post");

    BLINKER_LOG_ALL(TAG, "http data: %s, len: %d", https_request_data, https_request_bytes);

    // BLINKER_LOG_FreeHeap(TAG);
}

void blinker_https_task(void* pv)
{
    wolfssl_https_client();
    // mbedtls_https_client();
}

void blinker_server(uint8_t type)
{
    blinker_https_type = type;

    xTaskCreate(&blinker_https_task,
                "blinker_https_task",
                4096,
                NULL,
                5,
                NULL);

    switch (type)
    {
        // case BLINKER_CMD_SMS_NUMBER:
        //     break;
        // case BLINKER_CMD_FRESH_SHARERS_NUMBER:
        //     xEventGroupWaitBits(http_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
        //     break;
        // case BLINKER_CMD_DEVICE_AUTH_NUMBER:
        //     xEventGroupWaitBits(http_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
        //     break;
        case BLINKER_CMD_DEVICE_REGISTER_NUMBER:
            xEventGroupWaitBits(http_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
            break;
        default:
            break;
    }
}

void device_get_auth(void)
{
    // blinker_ws_print("{\"message\",\"success\"}");

    BLINKER_LOG(TAG, "device_get_auth");

    char test_url[100];

    strcpy(test_url, "/api/v1/user/device/auth/get?deviceType=");
    strcat(test_url, blinker_type);
    strcat(test_url, "&typeKey=");
    strcat(test_url, blinker_auth);
    strcat(test_url, "&deviceName=");
    strcat(test_url, macDeviceName());
    if (_aliType) strcat(test_url, _aliType);
    if (_duerType) strcat(test_url, _duerType);
    if (_miType) strcat(test_url, _miType);

    blinker_https_get(BLINKER_SERVER_HOST, test_url);

    blinker_server(BLINKER_CMD_DEVICE_AUTH_NUMBER);
}

void device_register(void)
{
    BLINKER_LOG(TAG, "device_register");

    http_event_group = xEventGroupCreate();

    #if defined (CONFIG_BLINKER_MODE_PRO)
        if (blinker_auth_check())
        {
            device_get_auth();

            isHello = 1;
        }

        for (;;)
        {
            vTaskDelay(10 / portTICK_RATE_MS);
            if (AUTH_BIT == BIT1)
            break;
        }


        // xEventGroupWaitBits(http_event_group, AUTH_BIT, false, true, portMAX_DELAY);
    #endif

    char test_url[100];

    #if defined (CONFIG_BLINKER_MODE_WIFI)
        strcpy(test_url, "/api/v1/user/device/diy/auth?authKey=");
    #elif defined (CONFIG_BLINKER_MODE_PRO)
        strcpy(test_url, "/api/v1/user/device/auth?authKey=");
    #endif
        strcat(test_url, blinker_authkey);
        if (_aliType) strcat(test_url, _aliType);
        if (_duerType) strcat(test_url, _duerType);
        if (_miType) strcat(test_url, _miType);
    // #elif defined (CONFIG_BLINKER_MODE_PRO)
    //     strcpy(test_url, "/api/v1/user/device/auth/get?deviceType=");
    //     strcat(test_url, blinker_type);
    //     strcat(test_url, "&typeKey=");
    //     strcat(test_url, blinker_auth);
    //     strcat(test_url, "&deviceName=");
    //     strcat(test_url, macDeviceName());
    //     if (_aliType) strcat(test_url, _aliType);
    //     if (_duerType) strcat(test_url, _duerType);
    //     if (_miType) strcat(test_url, _miType);
    // #endif
    blinker_https_get(BLINKER_SERVER_HOST, test_url);

    blinker_server(BLINKER_CMD_DEVICE_REGISTER_NUMBER);

    // xTaskCreate(blinker_https_task,
    //             WOLFSSL_DEMO_THREAD_NAME,
    //             WOLFSSL_DEMO_THREAD_STACK_WORDS,
    //             NULL,
    //             WOLFSSL_DEMO_THREAD_PRORIOTY,
    //             NULL);

    // xEventGroupWaitBits(http_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    // while (!isMQTTinit) { vTaskDelay(1000 / portTICK_RATE_MS); }
}

// 请求方法 | 空格 | URL | 空格 | 协议版本 | 回车 | 换行
// 头部字段 | : | 值 | 回车 | 换行
// ...
// 头部字段 | : | 值 | 回车 | 换行
// 回车 | 换行
// 数据

esp_mqtt_client_handle_t blinker_mqtt_client;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    blinker_mqtt_client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            BLINKER_LOG(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, BLINKER_SUB_TOPIC_MQTT, 0);
            BLINKER_LOG_ALL(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            BLINKER_LOG_FreeHeap(TAG);

            #if defined (CONFIG_BLINKER_MODE_PRO)
                if (isHello == 0)
                {
                    isHello = 1;

                    char stateJsonStr[256] = "{\"message\":\"Registration successful\"}";

                    blinker_mqtt_print(stateJsonStr, 0);
                }
            #endif

            // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            // BLINKER_LOG_ALL(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            // BLINKER_LOG_ALL(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            BLINKER_LOG(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            BLINKER_LOG_ALL(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            // msg_id = esp_mqtt_client_publish(client, BLINKER_PUB_TOPIC_MQTT, "{\"data\":{\"state\":\"online\"},\"fromDevice\":\"FC03CAC2HQFPY94881XL7XLD\",\"toDevice\":\"73c7b5a4b2f221c0a72d7b4128e40237\",\"deviceType\":\"OwnApp\"}", 0, 0, 0);
            // BLINKER_LOG_ALL(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            BLINKER_LOG_ALL(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            BLINKER_LOG_ALL(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            BLINKER_LOG_FreeHeap(TAG);
            BLINKER_LOG_ALL(TAG, "MQTT_EVENT_DATA");
            BLINKER_LOG_ALL(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
            BLINKER_LOG_ALL(TAG, "DATA=%.*s", event->data_len, event->data);

            if (isFresh_MQTT) free(msgBuf_MQTT);

            if (strncmp(BLINKER_SUB_TOPIC_MQTT, event->topic, event->topic_len) == 0)
            {
                cJSON *root = cJSON_Parse(event->data);

                if (root != NULL)
                {
                    cJSON *_uuid = cJSON_GetObjectItemCaseSensitive(root, "fromDevice");
                    // cJSON *dataGet = cJSON_GetObjectItemCaseSensitive(root, "data");

                    BLINKER_LOG_ALL(TAG, "from device: %s", _uuid->valuestring);
                    // BLINKER_LOG_ALL(TAG, "data: %s", cJSON_PrintUnformatted(dataGet));

                    // cJSON *ttest = cJSON_CreateObject();
                    // cJSON_AddItemToObject(ttest, "data", dataGet);
                    // BLINKER_LOG_ALL(TAG, "data: %s", cJSON_PrintUnformatted(ttest));

                    if (strncmp(UUID_MQTT, _uuid->valuestring, strlen(_uuid->valuestring)) == 0)
                    {
                        BLINKER_LOG_ALL(TAG, "Authority uuid");

                        BLINKER_LOG_FreeHeap(TAG);

                        cJSON_Delete(root);

                        // msgBuf_MQTT = (char *)malloc((event->data_len + 1)*sizeof(char));
                        // strcpy(msgBuf_MQTT, event->data);

                        if (data_parse_func) data_parse_func(event->data);
                        kaTime = millis();
                        isAvail_MQTT = 1;
                        // isFresh_MQTT = 1;
                        isAlive = 1;
                    }
                    else if (strncmp(BLINKER_CMD_ALIGENIE, _uuid->valuestring, strlen(_uuid->valuestring)) == 0)
                    {
                        BLINKER_LOG_ALL(TAG, "form AliGenie");

                        cJSON_Delete(root);

                        if (aligenie_parse_func) aligenie_parse_func(event->data);
                        aliKaTime = millis();
                        isAliAlive = 1;
                        isAliAvail = 1;
                    }
                    else if (strncmp(BLINKER_CMD_DUEROS, _uuid->valuestring, strlen(_uuid->valuestring)) == 0)
                    {
                        BLINKER_LOG_ALL(TAG, "form DuerOS");

                        cJSON_Delete(root);

                        if (dueros_parse_func) dueros_parse_func(event->data);
                        duerKaTime = millis();
                        isDuerAlive = 1;
                        isDuerAvail = 1;
                    }
                    else if (strncmp(BLINKER_CMD_MIOT, _uuid->valuestring, strlen(_uuid->valuestring)) == 0)
                    {
                        BLINKER_LOG_ALL(TAG, "form MIOT");

                        cJSON_Delete(root);

                        if (miot_parse_func) miot_parse_func(event->data);
                        miKaTime = millis();
                        isMIOTAlive = 1;
                        isMIOTAvail = 1;
                    }
                    else
                    {
                        cJSON_Delete(root);

                        BLINKER_LOG_ALL(TAG, "not from UUID!");
                    }
                }
                else
                {
                    cJSON_Delete(root);
                }                
                
            }
            else
            {
                BLINKER_ERR_LOG(TAG, "not from sub topic!");
            }

            // msg_id = esp_mqtt_client_publish(client, BLINKER_PUB_TOPIC_MQTT, "{\"data\":{\"state\":\"online\"},\"fromDevice\":\"FC03CAC2HQFPY94881XL7XLD\",\"toDevice\":\"73c7b5a4b2f221c0a72d7b4128e40237\",\"deviceType\":\"OwnApp\"}", 0, 0, 0);
            // BLINKER_LOG_ALL(TAG, "sent publish successful, msg_id=%d", msg_id);
            // free(blinker_mqtt_client.mqtt_state);
            break;
        case MQTT_EVENT_ERROR:
            BLINKER_LOG_ALL(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

void blinker_mqtt_init(void)
{
    // esp_err_t ret = xTaskCreate(&mqtt_client_thread,
    //                             MQTT_CLIENT_THREAD_NAME,
    //                             MQTT_CLIENT_THREAD_STACK_WORDS,
    //                             NULL,
    //                             MQTT_CLIENT_THREAD_PRIO,
    //                             NULL);

    // if (ret != pdPASS)  {
    //     ESP_LOGE(TAG, "mqtt create client thread %s failed", MQTT_CLIENT_THREAD_NAME);
    // } 

    const esp_mqtt_client_config_t mqtt_cfg = {
        .host = BLINKER_MQTT_ALIYUN_HOST,
        .event_handle = mqtt_event_handler,
        .client_id = MQTT_ID_MQTT,
        .username = MQTT_NAME_MQTT,
        .password = MQTT_KEY_MQTT,
        .port = BLINKER_MQTT_ALIYUN_PORT,
        .transport = MQTT_TRANSPORT_OVER_SSL,
        .task_stack = 2048,
        // .cert_pem = (const char *)iot_eclipse_org_pem_start,
    };

    BLINKER_LOG_ALL(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);

    // initialise_mdns();
}

char * mqtt_device_name(void)
{
    return DEVICE_NAME_MQTT;
}

char * mqtt_auth_key(void)
{
    return blinker_authkey;
}

void ali_type(const char *type, blinker_callback_with_string_arg_t func)
{
    _aliType = type;
    aligenie_parse_func = func;
}

void duer_type(const char *type, blinker_callback_with_string_arg_t func)
{
    _duerType = type;
    dueros_parse_func = func;
}

void mi_type(const char *type, blinker_callback_with_string_arg_t func)
{
    _miType = type;
    miot_parse_func = func;
}

int available(void)
{
    if (isAvail_MQTT)
    {
        isAvail_MQTT = 0;
        return 1;
    }
    else {
        return 0;
    }
}

char *last_read(void)
{
    if (isFresh_MQTT) return msgBuf_MQTT;
    else return "";
}

void flush(void)
{
    if (isFresh_MQTT)
    {
        BLINKER_LOG_ALL(TAG, "flush");

        free(msgBuf_MQTT); 
        isFresh_MQTT = 0;
        isAvail_MQTT = 0;
    }
}

void check_ka(void) {
    if (millis() - kaTime >= BLINKER_MQTT_KEEPALIVE)
        isAlive = 0;
}

int check_ali_ka(void) {
    if (millis() - aliKaTime >= 10000)
        return 0;
    else
        return 1;
}

int check_duer_ka(void) {
    if (millis() - duerKaTime >= 10000)
        return 0;
    else
        return 1;
}

int check_miot_ka(void) {
    if (millis() - miKaTime >= 10000)
        return 0;
    else
        return 1;
}

int8_t check_can_print(void) {
    check_ka();

    if ((millis() - printTime >= BLINKER_MQTT_MSG_LIMIT && isAlive) || printTime == 0) {
        return 1;
    }
    else {
        BLINKER_ERR_LOG(TAG, "MQTT NOT ALIVE OR MSG LIMIT");
        return 0;
    }
}

int8_t check_print_span(void) {
    if (millis() - respTime < BLINKER_PRINT_MSG_LIMIT) {
        if (respTimes > BLINKER_PRINT_MSG_LIMIT) {
            BLINKER_ERR_LOG(TAG, "WEBSOCKETS CLIENT NOT ALIVE OR MSG LIMIT");

            return 0;
        }
        else {
            respTimes++;
            return 1;
        }
    }
    else {
        respTimes = 0;
        return 1;
    }
}

int check_ali_print_span(void)
{
    if (millis() - respAliTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respAliTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(TAG, "ALIGENIE NOT ALIVE OR MSG LIMIT");

            return false;
        }
        else
        {
            respAliTimes++;
            return true;
        }
    }
    else
    {
        respAliTimes = 0;
        return true;
    }
}

int check_duer_print_span(void)
{
    if (millis() - respDuerTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respDuerTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(TAG, "DUEROS NOT ALIVE OR MSG LIMIT");

            return false;
        }
        else
        {
            respDuerTimes++;
            return true;
        }
    }
    else
    {
        respDuerTimes = 0;
        return true;
    }
}

int check_miot_print_span(void)
{
    if (millis() - respMIOTTime < BLINKER_PRINT_MSG_LIMIT/2)
    {
        if (respMIOTTimes > BLINKER_PRINT_MSG_LIMIT/2)
        {
            BLINKER_ERR_LOG(TAG, "DUEROS NOT ALIVE OR MSG LIMIT");

            return false;
        }
        else
        {
            respMIOTTimes++;
            return true;
        }
    }
    else
    {
        respMIOTTimes = 0;
        return true;
    }
}

int8_t check_print_limit(void)
{
    if ((millis() - _print_time) < 60000)
    {
        if (_print_times < 10) return 1;
        else 
        {
            BLINKER_ERR_LOG(TAG, "MQTT MSG LIMIT");
            return 0;
        }
    }
    else
    {
        _print_time = millis();
        _print_times = 0;
        return 1;
    }
}

int8_t blinker_print(char *data, uint8_t need_check)
{
    if (dataFrom_MQTT == BLINKER_MSG_FROM_WS)
    {
        dataFrom_MQTT = BLINKER_MSG_FROM_MQTT;

        if (need_check)
        {
            if (!check_print_span())
            {                
                respTime = millis();
                return 0;
            }
        }

        respTime = millis();

        char *_data;

        _data = (char*)malloc((strlen(data)+2)*sizeof(char));
        strcpy(_data, data);
        strcat(_data, "\n");

        return blinker_ws_print(_data);
    }
    else
    {
        return blinker_mqtt_print(data, need_check);
    }    
}

int8_t blinker_ws_print(char *data)
{
    BLINKER_ERR_LOG(TAG, "ws response: %s ", data);

    BLINKER_LOG_FreeHeap(TAG);

    dataFrom_MQTT = BLINKER_MSG_FROM_MQTT;

    return (ws_server_send_text_all(data, strlen(data)) > 0) ? 1 : 0;
}

int8_t blinker_mqtt_print(char *data, uint8_t need_check)
{
    if (isMQTTinit)
    {
        uint16_t num = strlen(data);

        for(uint16_t c_num = num; c_num > 0; c_num--)
        {
            data[c_num+7] = data[c_num-1];
        }

        data[num+8] = '\0';        

        char data_add[20] = "{\"data\":";
        for(uint8_t c_num = 0; c_num < 8; c_num++)
        {
            data[c_num] = data_add[c_num];
        }
        strcat(data, ",\"fromDevice\":\"");
        strcat(data, MQTT_ID_MQTT);
        strcat(data, "\",\"toDevice\":\"");
        
        if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
        {
            strcat(data, _sharers[_sharerFrom].uuid);
        }
        else
        {
            strcat(data, UUID_MQTT);
        }

        strcat(data, "\",\"deviceType\":\"OwnApp\"}");

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;        

        BLINKER_LOG_ALL(TAG, "publish: %s", data);

        int8_t _alive = isAlive;

        if (!isJson(data))
        {
            BLINKER_ERR_LOG(TAG, "Print data is not Json!");
            return 0;
        }

        if (need_check)
        {
            if (!check_print_span())
            {
                return 0;
            }
            respTime = millis();

            if (!check_can_print())
            {
                if (!_alive)
                {
                    isAlive = 0;
                }
                return 0;
            }

            if (!check_print_limit())
            {
                return 0;
            }

            _print_times++;
        }

        int msg_id = esp_mqtt_client_publish(blinker_mqtt_client, BLINKER_PUB_TOPIC_MQTT, data, 0, 0, 0);
        BLINKER_LOG_ALL(TAG, "sent publish successful, msg_id=%d", msg_id);

        BLINKER_LOG_FreeHeap(TAG);

        return 1;
    }

    return 0;
}

int8_t blinker_aligenie_mqtt_print(char *data)
{
    if (isMQTTinit)
    {
        uint16_t num = strlen(data);

        for(uint16_t c_num = num; c_num > 0; c_num--)
        {
            data[c_num+7] = data[c_num-1];
        }

        data[num+8] = '\0';        

        char data_add[20] = "{\"data\":";
        for(uint8_t c_num = 0; c_num < 8; c_num++)
        {
            data[c_num] = data_add[c_num];
        }
        strcat(data, ",\"fromDevice\":\"");
        strcat(data, MQTT_ID_MQTT);
        strcat(data, "\",\"toDevice\":\"AliGenie_r\"");
        strcat(data, ",\"deviceType\":\"vAssistant\"}");

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;       

        BLINKER_LOG_ALL(TAG, "publish: %s", data);

        if (!isJson(data))
        {
            BLINKER_ERR_LOG(TAG, "Print data is not Json!");
            return 0;
        }

        // int8_t _alive = isAlive;

        if (!check_ali_ka())
        {
            return 0;
        }

        if (!check_ali_print_span())
        {
            respAliTime = millis();
            return 0;
        }
        respAliTime = millis();

        int msg_id = esp_mqtt_client_publish(blinker_mqtt_client, BLINKER_PUB_TOPIC_MQTT, data, 0, 0, 0);
        BLINKER_LOG_ALL(TAG, "sent publish successful, msg_id=%d", msg_id);

        isAliAlive = 0;
        return 1;
    }

    return 0;
}

int8_t blinker_dueros_mqtt_print(char *data)
{
    if (isMQTTinit)
    {
        uint16_t num = strlen(data);

        for(uint16_t c_num = num; c_num > 0; c_num--)
        {
            data[c_num+7] = data[c_num-1];
        }

        data[num+8] = '\0';        

        char data_add[20] = "{\"data\":";
        for(uint8_t c_num = 0; c_num < 8; c_num++)
        {
            data[c_num] = data_add[c_num];
        }
        strcat(data, ",\"fromDevice\":\"");
        strcat(data, MQTT_ID_MQTT);
        strcat(data, "\",\"toDevice\":\"DuerOS_r\"");
        strcat(data, ",\"deviceType\":\"vAssistant\"}");

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

        BLINKER_LOG_ALL(TAG, "publish: %s", data);

        if (!isJson(data))
        {
            BLINKER_ERR_LOG(TAG, "Print data is not Json!");
            return 0;
        }

        // int8_t _alive = isAlive;

        if (!check_duer_ka())
        {
            return 0;
        }

        if (!check_duer_print_span())
        {
            respDuerTime = millis();
            return 0;
        }
        respDuerTime = millis();

        int msg_id = esp_mqtt_client_publish(blinker_mqtt_client, BLINKER_PUB_TOPIC_MQTT, data, 0, 0, 0);
        BLINKER_LOG_ALL(TAG, "sent publish successful, msg_id=%d", msg_id);

        isDuerAlive = 0;
        return 1;
    }

    return 0;
}

int8_t blinker_miot_mqtt_print(char *data)
{
    if (isMQTTinit)
    {
        uint16_t num = strlen(data);

        for(uint16_t c_num = num; c_num > 0; c_num--)
        {
            data[c_num+7] = data[c_num-1];
        }

        data[num+8] = '\0';        

        char data_add[20] = "{\"data\":";
        for(uint8_t c_num = 0; c_num < 8; c_num++)
        {
            data[c_num] = data_add[c_num];
        }
        strcat(data, ",\"fromDevice\":\"");
        strcat(data, MQTT_ID_MQTT);
        strcat(data, "\",\"toDevice\":\"MIOT_r\"");
        strcat(data, ",\"deviceType\":\"vAssistant\"}");

        _sharerFrom = BLINKER_MQTT_FROM_AUTHER;

        BLINKER_LOG_ALL(TAG, "publish: %s", data);

        if (!isJson(data))
        {
            BLINKER_ERR_LOG(TAG, "Print data is not Json!");
            return 0;
        }

        // int8_t _alive = isAlive;

        if (!check_miot_ka())
        {
            return 0;
        }

        if (!check_miot_print_span())
        {
            respMIOTTime = millis();
            return 0;
        }
        respMIOTTime = millis();

        int msg_id = esp_mqtt_client_publish(blinker_mqtt_client, BLINKER_PUB_TOPIC_MQTT, data, 0, 0, 0);
        BLINKER_LOG_ALL(TAG, "sent publish successful, msg_id=%d", msg_id);

        isMIOTAlive = 0;
        return 1;
    }

    return 0;
}

void blinker_reset(void)
{
    BLINKER_LOG(TAG, "Blinker Reset!!!");
    blinker_spiffs_delete();
    esp_wifi_restore();
    vTaskDelay(1000 / portTICK_RATE_MS);
    esp_restart();
}
