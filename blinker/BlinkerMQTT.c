#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"

#include "BlinkerMQTT.h"

#include <sys/socket.h>
#include <netdb.h>

#include "lwip/apps/sntp.h"
#include "wolfssl/ssl.h"

#include "esp_http_client.h"
#include "mqtt_client.h"

static const char *TAG = "BlinkerMQTT";

static EventGroupHandle_t wifi_event_group;
static EventGroupHandle_t http_event_group;

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

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

#define WEB_SERVER "www.howsmyssl.com"
#define WEB_PORT 443
#define WEB_URL "https://www.howsmyssl.com/a/check"

#define REQUEST "GET " WEB_URL " HTTP/1.0\r\n" \
    "Host: "WEB_SERVER"\r\n" \
    "User-Agent: esp-idf/1.0 espressif\r\n" \
    "\r\n"

#define WOLFSSL_DEMO_THREAD_NAME        "wolfssl_client"
#define WOLFSSL_DEMO_THREAD_STACK_WORDS 8192
#define WOLFSSL_DEMO_THREAD_PRORIOTY    6

#define WOLFSSL_DEMO_SNTP_SERVERS       "pool.ntp.org"

// const char send_data[] = REQUEST;
// const int32_t send_bytes = sizeof(send_data);

const char* blinker_authkey;
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

#define MQTT_CLIENT_THREAD_NAME         "mqtt_client_thread"
#define MQTT_CLIENT_THREAD_STACK_WORDS  4096
#define MQTT_CLIENT_THREAD_PRIO         8

void smartconfig_task(void * parm);
void initialise_wifi();
void blinker_https_get(const char * _host, const char * _url);
blinker_callback_with_string_arg_t data_parse_func = NULL;
blinker_callback_with_string_arg_t aligenie_parse_func = NULL;
blinker_callback_with_string_arg_t dueros_parse_func = NULL;
blinker_callback_with_string_arg_t miot_parse_func = NULL;

#define CONFIG_MQTT_PAYLOAD_BUFFER 1460
// #define CONFIG_MQTT_BROKER 
// #define CONFIG_MQTT_PORT

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;
    
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            if (sconf_step == sconf_begin)
            {
                xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
            }
            else
            {
                esp_wifi_connect();
            }
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            BLINKER_LOG(TAG, "got ip:%s",
                    ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

            sconf_step = sconf_ap_connected;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
            if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
                /*Switch to 802.11 bgn mode */
                esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
            }
            if (sconf_step == sconf_ap_connect)
            {
                sconf_step = sconf_ap_disconnect;

                xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
                initialise_wifi();
                xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
            }
            else
            {
                esp_wifi_connect();
                xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

void wifi_init_sta(const char * _key, const char * _ssid, const char * _pswd, blinker_callback_with_string_arg_t _func)
{
    // blinker_authkey = (char *)malloc(strlen(_key)*sizeof(char));
    // strcpy(blinker_authkey, _key);
    blinker_authkey = _key;

    data_parse_func = _func;

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

    BLINKER_LOG(TAG, "wifi_init_sta finished.");
    BLINKER_LOG(TAG, "connect to ap SSID:%s password:%s",
            (char *)wifi_config.sta.ssid, (char *)wifi_config.sta.password);
}

void wifi_init_smart(const char * _key)
{
    blinker_authkey = (char *)malloc(strlen(_key)*sizeof(char));
    strcpy(blinker_authkey, _key);

    ESP_ERROR_CHECK( nvs_flash_init() );

    sconf_step = sconf_ap_connect;

    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // wifi_config_t wifi_config = {
    //     .sta = {
    //         .ssid = EXAMPLE_ESP_WIFI_SSID,
    //         .password = EXAMPLE_ESP_WIFI_PASS
    //     },
    // };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    // ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    BLINKER_LOG_ALL(TAG, "wifi_init_smart finished.");
    // BLINKER_LOG_ALL(TAG, "connect to ap SSID:%s password:%s",
    //          EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
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
                        ESP_LOGE(TAG, "TYPE: ERROR");
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

void initialise_wifi()
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

void get_time(void)
{
    struct timeval now;
    int sntp_retry_cnt = 0;
    int sntp_retry_time = 0;

    sntp_setoperatingmode(0);
    sntp_setservername(0, WOLFSSL_DEMO_SNTP_SERVERS);
    sntp_setservername(1, "210.72.145.44");
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

    char payload[1024] = {0};
    uint8_t need_read = 0;
    uint16_t check_num = 0;

// void https_get_task(void)
// {
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
//         ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
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
//     if((ret = mbedtls_ssl_set_hostname(&ssl, WEB_SERVER)) != 0)
//     {
//         ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
//         abort();
//     }

//     BLINKER_LOG_ALL(TAG, "Setting up the SSL/TLS structure...");

//     if((ret = mbedtls_ssl_config_defaults(&conf,
//                                           MBEDTLS_SSL_IS_CLIENT,
//                                           MBEDTLS_SSL_TRANSPORT_STREAM,
//                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
//     {
//         ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
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
//         ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
//         goto exit;
//     }
    
//     // char recv_data[1024] = {0};
//     // char payload[1024] = {0};
//     // uint8_t need_read = 0;
//     // uint16_t check_num = 0;

//     while(1) {
//         /* Wait for the callback to set the CONNECTED_BIT in the
//            event group.
//         */
//         xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
//                             false, true, portMAX_DELAY);
//         BLINKER_LOG_ALL(TAG, "Connected to AP");

//         mbedtls_net_init(&server_fd);

//         BLINKER_LOG_ALL(TAG, "Connecting to %s:%s...", BLINKER_SERVER, BLINKER_SERVER_PORT);

//         if ((ret = mbedtls_net_connect(&server_fd, BLINKER_SERVER,
//                                       BLINKER_SERVER_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
//         {
//             ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
//             goto exit;
//         }

//         BLINKER_LOG_ALL(TAG, "Connected.");

//         mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

//         BLINKER_LOG_ALL(TAG, "Performing the SSL/TLS handshake...");

//         while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
//         {
//             if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
//             {
//                 ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
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
//                 ESP_LOGE(TAG, "mbedtls_ssl_write returned -0x%x", -ret);
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
//                 ESP_LOGE(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
//                 break;
//             }

//             if(ret == 0)
//             {
//                 BLINKER_LOG_ALL(TAG, "connection closed");
//                 break;
//             }

//             len = ret;
//             ESP_LOGD(TAG, "%d bytes read", len);
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
//             ESP_LOGE(TAG, "Last error was: -0x%x - %s", -ret, buf);
//         }

//         // putchar('\n'); // JSON output doesn't have a newline at end

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

//         // BLINKER_LOG_ALL(TAG, "check isJson");
//         if (check_register_data(payload))
//         {
//             cJSON *root = cJSON_Parse(payload);
        
//             cJSON *detail = cJSON_GetObjectItemCaseSensitive(root, BLINKER_CMD_DETAIL);
//             cJSON *_userID = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_DEVICENAME);
//             cJSON *_userName = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_IOTID);
//             cJSON *_key = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_IOTTOKEN);
//             cJSON *_productInfo = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_PRODUCTKEY);
//             cJSON *_broker = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_BROKER);
//             cJSON *_uuid = cJSON_GetObjectItemCaseSensitive(detail, BLINKER_CMD_UUID);

//             // if (cJSON_IsString(_userID) && (_userID->valuestring != NULL))
//             // {
//             //     BLINKER_LOG_ALL(TAG, "_userId: %s", _userID->valuestring);
//             // }
//             if (isMQTTinit)
//             {
//                 free(MQTT_HOST_MQTT);
//                 free(MQTT_ID_MQTT);
//                 free(MQTT_NAME_MQTT);
//                 free(MQTT_KEY_MQTT);
//                 free(MQTT_PRODUCTINFO_MQTT);
//                 free(UUID_MQTT);
//                 free(DEVICE_NAME_MQTT);
//                 free(BLINKER_PUB_TOPIC_MQTT);
//                 free(BLINKER_SUB_TOPIC_MQTT);

//                 isMQTTinit = 0;
//             }

//             if (strcmp(_broker->valuestring, BLINKER_MQTT_BORKER_ALIYUN) == 0)
//             {
//                 BLINKER_LOG_ALL(TAG, "broker is aliyun");

//                 DEVICE_NAME_MQTT = (char*)malloc((strlen(_userID->valuestring)+1)*sizeof(char));
//                 strcpy(DEVICE_NAME_MQTT, _userID->valuestring);
//                 MQTT_ID_MQTT = (char*)malloc((strlen(_userID->valuestring)+1)*sizeof(char));
//                 strcpy(MQTT_ID_MQTT, _userID->valuestring);
//                 MQTT_NAME_MQTT = (char*)malloc((strlen(_userName->valuestring)+1)*sizeof(char));
//                 strcpy(MQTT_NAME_MQTT, _userName->valuestring);
//                 MQTT_KEY_MQTT = (char*)malloc((strlen(_key->valuestring)+1)*sizeof(char));
//                 strcpy(MQTT_KEY_MQTT, _key->valuestring);
//                 MQTT_PRODUCTINFO_MQTT = (char*)malloc((strlen(_productInfo->valuestring)+1)*sizeof(char));
//                 strcpy(MQTT_PRODUCTINFO_MQTT, _productInfo->valuestring);
//                 MQTT_HOST_MQTT = (char*)malloc((strlen(BLINKER_MQTT_ALIYUN_HOST)+1)*sizeof(char));
//                 strcpy(MQTT_HOST_MQTT, BLINKER_MQTT_ALIYUN_HOST);
//                 MQTT_PORT_MQTT = BLINKER_MQTT_ALIYUN_PORT;

//                 BLINKER_SUB_TOPIC_MQTT = (char*)malloc((1 + strlen(MQTT_PRODUCTINFO_MQTT) + 
//                                     1 + strlen(MQTT_ID_MQTT) + 3)*sizeof(char));
//                 strcpy(BLINKER_SUB_TOPIC_MQTT, "/");
//                 strcat(BLINKER_SUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT);
//                 strcat(BLINKER_SUB_TOPIC_MQTT, "/");
//                 strcat(BLINKER_SUB_TOPIC_MQTT, MQTT_ID_MQTT);                
//                 strcat(BLINKER_SUB_TOPIC_MQTT, "/r");

//                 BLINKER_PUB_TOPIC_MQTT = (char*)malloc((1 + strlen(MQTT_PRODUCTINFO_MQTT) + 
//                                     1 + strlen(MQTT_ID_MQTT) + 3)*sizeof(char));
//                 strcpy(BLINKER_PUB_TOPIC_MQTT, "/");
//                 strcat(BLINKER_PUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT);
//                 strcat(BLINKER_PUB_TOPIC_MQTT, "/");
//                 strcat(BLINKER_PUB_TOPIC_MQTT, MQTT_ID_MQTT);                
//                 strcat(BLINKER_PUB_TOPIC_MQTT, "/s");
//             }

//             UUID_MQTT = (char*)malloc((strlen(_uuid->valuestring)+1)*sizeof(char));
//             strcpy(UUID_MQTT, _uuid->valuestring);

//             BLINKER_LOG_ALL(TAG, "====================");
//             BLINKER_LOG_ALL(TAG, "DEVICE_NAME_MQTT: %s", DEVICE_NAME_MQTT);
//             BLINKER_LOG_ALL(TAG, "MQTT_PRODUCTINFO_MQTT: %s", MQTT_PRODUCTINFO_MQTT);
//             BLINKER_LOG_ALL(TAG, "MQTT_ID_MQTT: %s", MQTT_ID_MQTT);
//             BLINKER_LOG_ALL(TAG, "MQTT_NAME_MQTT: %s", MQTT_NAME_MQTT);
//             BLINKER_LOG_ALL(TAG, "MQTT_KEY_MQTT: %s", MQTT_KEY_MQTT);
//             BLINKER_LOG_ALL(TAG, "MQTT_BROKER: %s", _broker->valuestring);
//             BLINKER_LOG_ALL(TAG, "HOST: %s", MQTT_HOST_MQTT);
//             BLINKER_LOG_ALL(TAG, "PORT: %d", MQTT_PORT_MQTT);
//             BLINKER_LOG_ALL(TAG, "UUID_MQTT: %s", UUID_MQTT);
//             BLINKER_LOG_ALL(TAG, "BLINKER_SUB_TOPIC_MQTT: %s", BLINKER_SUB_TOPIC_MQTT);
//             BLINKER_LOG_ALL(TAG, "BLINKER_PUB_TOPIC_MQTT: %s", BLINKER_PUB_TOPIC_MQTT);
//             BLINKER_LOG_ALL(TAG, "====================");
            
//             isMQTTinit = 1;

//             xEventGroupSetBits(http_event_group, isMQTTinit);
            
//             cJSON_Delete(root);

//             BLINKER_LOG_FreeHeap(TAG);

//             return;
//         }

//         https_delay(20);
//     }
// }

void wolfssl_client(void)
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
            case BLINKER_CMD_DEVICE_REGISTER_NUMBER :
                // BLINKER_LOG_ALL(TAG, "check isJson");
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

                        BLINKER_SUB_TOPIC_MQTT = (char*)malloc((1 + strlen(MQTT_PRODUCTINFO_MQTT) + \
                                            1 + strlen(MQTT_ID_MQTT) + 3)*sizeof(char));
                        strcpy(BLINKER_SUB_TOPIC_MQTT, "/");
                        strcat(BLINKER_SUB_TOPIC_MQTT, MQTT_PRODUCTINFO_MQTT);
                        strcat(BLINKER_SUB_TOPIC_MQTT, "/");
                        strcat(BLINKER_SUB_TOPIC_MQTT, MQTT_ID_MQTT);                
                        strcat(BLINKER_SUB_TOPIC_MQTT, "/r");

                        BLINKER_PUB_TOPIC_MQTT = (char*)malloc((1 + strlen(MQTT_PRODUCTINFO_MQTT) + \
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
                    
                    isMQTTinit = 1;

                    xEventGroupSetBits(http_event_group, isMQTTinit);
                    
                    cJSON_Delete(root);

                    BLINKER_LOG_FreeHeap(TAG);

                    wolfSSL_shutdown(ssl);

                    // wolfSSL_free(ssl);

                    close(socket);

                    // wolfSSL_CTX_free(ctx);

                    // wolfSSL_Cleanup();

                    // return;
                    
                    vTaskDelete(NULL);
                    return;
                }                
            default :
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

    BLINKER_LOG_ALL(TAG, "http data: %s, len: %d", https_request_data, https_request_bytes);
}

void wolfssl_http(void* pv)
{
    wolfssl_client();
}

void blinker_server(void)
{
    xTaskCreate(wolfssl_http,
                WOLFSSL_DEMO_THREAD_NAME,
                WOLFSSL_DEMO_THREAD_STACK_WORDS,
                NULL,
                WOLFSSL_DEMO_THREAD_PRORIOTY,
                NULL);

    switch (blinker_https_type)
    {
        case BLINKER_CMD_DEVICE_REGISTER_NUMBER:
            xEventGroupWaitBits(http_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
            break;
        
        default:
            break;
    }
}

void device_register(void)
{
    http_event_group = xEventGroupCreate();

    char test_url[64] = "/api/v1/user/device/diy/auth?authKey=";
    strcat(test_url, blinker_authkey);
    if (_aliType) strcat(test_url, _aliType);
    if (_duerType) strcat(test_url, _duerType);
    if (_miType) strcat(test_url, _miType);
    blinker_https_get("iotdev.clz.me", test_url);

    blinker_https_type = BLINKER_CMD_DEVICE_REGISTER_NUMBER;

    blinker_server();

    // xTaskCreate(wolfssl_http,
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

                        cJSON_Delete(root);

                        // msgBuf_MQTT = (char *)malloc((event->data_len + 1)*sizeof(char));
                        // strcpy(msgBuf_MQTT, event->data);

                        if (data_parse_func) data_parse_func(event->data);
                        kaTime = millis();
                        isAvail_MQTT = 1;
                        isFresh_MQTT = 1;
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
        // .cert_pem = (const char *)iot_eclipse_org_pem_start,
    };

    BLINKER_LOG_ALL(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
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
        
        // if (_sharerFrom < BLINKER_MQTT_MAX_SHARERS_NUM)
        // {
        //     strcat(data, _sharers[_sharerFrom]->uuid());
        // }
        // else
        // {
            strcat(data, UUID_MQTT);
        // }

        strcat(data, "\",\"deviceType\":\"OwnApp\"}");

        // _sharerFrom = BLINKER_MQTT_FROM_AUTHER;        

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

        // _sharerFrom = BLINKER_MQTT_FROM_AUTHER;        

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
        strcat(data, "\",\"toDevice\":\"AliGenie_r\"");
        strcat(data, ",\"deviceType\":\"vAssistant\"}");

        // _sharerFrom = BLINKER_MQTT_FROM_AUTHER;        

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

        // _sharerFrom = BLINKER_MQTT_FROM_AUTHER;        

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