#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
// #include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"

#include "BlinkerMQTT.h"

static const char *TAG = "BlinkerMQTT";

static EventGroupHandle_t wifi_event_group;
static EventGroupHandle_t smart_event_group;
static EventGroupHandle_t http_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

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

char* blinker_authkey;
char* blinker_auth;
char* blinker_type;
char* https_request_data;
int32_t https_request_bytes = 0;
uint8_t blinker_https_type = BLINKER_CMD_DEFAULT_NUMBER;

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

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "www.howsmyssl.com"
#define WEB_PORT "443"
#define WEB_URL "https://www.howsmyssl.com/a/check"

// static const char *TAG = "example";

// static const char *REQUEST = "GET " WEB_URL " HTTP/1.0\r\n"
//     "Host: "WEB_SERVER"\r\n"
//     "User-Agent: esp-idf/1.0 esp32\r\n"
//     "\r\n";

// static const int CONNECTED_BIT = BIT0;
// static const int ESPTOUCH_DONE_BIT = BIT1;
static int AUTH_BIT = BIT0;
// static const int REGISTER_BIT = BIT0;
int8_t spiffs_start = 0;

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

enum smartconfig_step_t
{
    sconf_ap_init,
    sconf_ap_connect,
    sconf_ap_connected,
    sconf_ap_disconnect,
    sconf_begin
};

enum smartconfig_step_t sconf_step = sconf_ap_init;

static int s_retry_num = 0;

#define EXAMPLE_ESP_MAXIMUM_RETRY 5

static void smartconfig_example_task(void * parm);
void initialise_wifi(void);
void mbedtls_https_client(void);

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

                        aliKaTime = millis();
                        isAliAlive = 1;
                        isAliAvail = 1;

                        if (aligenie_parse_func) aligenie_parse_func(event->data);
                    }
                    else if (strncmp(BLINKER_CMD_DUEROS, _uuid->valuestring, strlen(_uuid->valuestring)) == 0)
                    {
                        BLINKER_LOG_ALL(TAG, "form DuerOS");

                        cJSON_Delete(root);

                        duerKaTime = millis();
                        isDuerAlive = 1;
                        isDuerAvail = 1;

                        if (dueros_parse_func) dueros_parse_func(event->data);
                    }
                    else if (strncmp(BLINKER_CMD_MIOT, _uuid->valuestring, strlen(_uuid->valuestring)) == 0)
                    {
                        BLINKER_LOG_ALL(TAG, "form MIOT");

                        cJSON_Delete(root);

                        miKaTime = millis();
                        isMIOTAlive = 1;
                        isMIOTAvail = 1;

                        if (miot_parse_func) miot_parse_func(event->data);
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
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
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
        .task_stack = 3072,
        // .cert_pem = (const char *)iot_eclipse_org_pem_start,
    };

    BLINKER_LOG_ALL(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);

    // initialise_mdns();
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

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if (sconf_step == sconf_begin)
        {
            xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
        }
        else
        {
            esp_wifi_connect();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (sconf_step == sconf_ap_connect)
        {
            sconf_step = sconf_ap_disconnect;

            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            initialise_wifi();
            xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
        }
        else
        {
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            } else {
                xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            }
            ESP_LOGI(TAG,"connect to the AP fail");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        sconf_step = sconf_ap_connected;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        #if defined (CONFIG_BLINKER_SMART_CONFIG)
            xEventGroupSetBits(smart_event_group, CONNECTED_BIT);
        #elif defined (CONFIG_BLINKER_AP_CONFIG)
            xEventGroupSetBits(ap_event_group, CONNECTED_BIT);
        #endif
    }else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
        ESP_ERROR_CHECK( esp_wifi_connect() );
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(smart_event_group, ESPTOUCH_DONE_BIT);
    }
}

void initialise_wifi(void)
{
    sconf_step = sconf_begin;

    BLINKER_LOG_ALL(TAG, "initialise_wifi");

    ESP_ERROR_CHECK(esp_netif_init());
    smart_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1) {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY); 
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

void blinker_set_auth(const char * _key, blinker_callback_with_string_arg_t _func)
{
    uint8_t _len = strlen(_key) + 1;
    blinker_authkey = (char *)malloc(_len*sizeof(char));
    strcpy(blinker_authkey, _key);

    data_parse_func = _func;
}

void wifi_init_smart(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );

    wifi_event_group = xEventGroupCreate();

    smart_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    // tcpip_adapter_init();
    // ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // EXAMPLE_ESP_WIFI_SSID = _ssid;
    // EXAMPLE_ESP_WIFI_PASS = _pswd;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    sconf_step = sconf_begin;

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

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

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(wifi_event_group);
    vEventGroupDelete(smart_event_group);

    BLINKER_LOG_FreeHeap(TAG);
}

void wifi_init_sta(const char * _ssid, const char * _pswd)
{
    // blinker_authkey = (char *)malloc(strlen(_key)*sizeof(char));
    // strcpy(blinker_authkey, _key);
    // // blinker_authkey = _key;

    // data_parse_func = _func;

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA %s, %s", _ssid, _pswd);

    ESP_ERROR_CHECK( nvs_flash_init() );

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    // tcpip_adapter_init();
    // ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // EXAMPLE_ESP_WIFI_SSID = _ssid;
    // EXAMPLE_ESP_WIFI_PASS = _pswd;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = ""
        },
    };
    // wifi_config_t wifi_config;
    strcpy((char *)wifi_config.sta.ssid, _ssid);
    strcpy((char *)wifi_config.sta.password, _pswd);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA %s, %s", _ssid, _pswd);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 (char *)wifi_config.sta.ssid, (char *)wifi_config.sta.password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 (char *)wifi_config.sta.ssid, (char *)wifi_config.sta.password);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    BLINKER_LOG(TAG, "wifi_init_sta finished.");
    BLINKER_LOG(TAG, "connect to ap SSID:%s password:%s",
            (char *)wifi_config.sta.ssid, (char *)wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(wifi_event_group);

    BLINKER_LOG_FreeHeap(TAG);
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

    https_request_bytes = strlen(_data) + 1;

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
    uint16_t _len = strlen(_msg);
    sprintf(_num, "%d", _len);
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
}

void blinker_https_task(void* pv)
{
    // wolfssl_https_client();
    mbedtls_https_client();
}

void blinker_server(uint8_t type)
{
    blinker_https_type = type;

    // ESP_ERROR_CHECK( nvs_flash_init() );
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    xTaskCreate(&blinker_https_task,
                "blinker_https_task",
                8192,
                NULL,
                5,
                NULL);

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    // ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "blinker_server!");
    // xTaskCreate(&https_get_task, "https_get_task", 8192, NULL, 5, NULL);

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
            vEventGroupDelete(http_event_group);
            break;
        default:
            break;
    }

}

void mbedtls_https_client(void)
{
    // char recv_data[1024] = {0};
    char payload[1024] = {0};
    uint8_t need_read = 0;
    uint16_t check_num = 0;

    char buf[1024];
    int ret, flags, len;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    BLINKER_LOG_ALL(TAG, "Seeding the random number generator");

    mbedtls_ssl_config_init(&conf);

    mbedtls_entropy_init(&entropy);
    if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    NULL, 0)) != 0)
    {
        BLINKER_LOG_ALL(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        abort();
    }

    BLINKER_LOG_ALL(TAG, "Loading the CA root certificate...");

    // ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start,
    //                              server_root_cert_pem_end-server_root_cert_pem_start);

    // if(ret < 0)
    // {
    //     ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
    //     abort();
    // }

    BLINKER_LOG_ALL(TAG, "Setting hostname for TLS session...");

     /* Hostname set here should match CN in server certificate */
    if((ret = mbedtls_ssl_set_hostname(&ssl, BLINKER_SERVER_HOST)) != 0)
    {
        BLINKER_LOG_ALL(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
        abort();
    }

    BLINKER_LOG_ALL(TAG, "Setting up the SSL/TLS structure...");

    if((ret = mbedtls_ssl_config_defaults(&conf,
                                          MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        BLINKER_LOG_ALL(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }

    /* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
       a warning if CA verification fails but it will continue to connect.

       You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
    */
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef CONFIG_MBEDTLS_DEBUG
    mbedtls_esp_enable_debug_log(&conf, 4);
#endif

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
    {
        BLINKER_LOG_ALL(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }

    while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
        // xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
        //                     false, true, portMAX_DELAY);
        // BLINKER_LOG_ALL(TAG, "Connected to AP");

        mbedtls_net_init(&server_fd);

        BLINKER_LOG(TAG, "Connecting to %s:%s...", BLINKER_SERVER_HOST, BLINKER_SERVER_PORT);

        if ((ret = mbedtls_net_connect(&server_fd, BLINKER_SERVER_HOST,
                                      BLINKER_SERVER_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
        {
            BLINKER_LOG_ALL(TAG, "mbedtls_net_connect returned -%x", -ret);
            goto exit;
        }

        BLINKER_LOG_ALL(TAG, "Connected.");

        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

        BLINKER_LOG_ALL(TAG, "Performing the SSL/TLS handshake...");

        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
        {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                BLINKER_LOG_ALL(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
                goto exit;
            }
        }

        BLINKER_LOG_ALL(TAG, "Verifying peer X.509 certificate...");

        if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
        {
            /* In real life, we probably want to close connection if ret != 0 */
            ESP_LOGW(TAG, "Failed to verify peer certificate!");
            bzero(buf, sizeof(buf));
            mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
            ESP_LOGW(TAG, "verification info: %s", buf);
        }
        else {
            BLINKER_LOG_ALL(TAG, "Certificate verified.");
        }

        BLINKER_LOG_ALL(TAG, "Cipher suite is %s", mbedtls_ssl_get_ciphersuite(&ssl));

        BLINKER_LOG_ALL(TAG, "Writing HTTP request...");

        size_t written_bytes = 0;
        do {
            ret = mbedtls_ssl_write(&ssl,
                                    (const unsigned char *)https_request_data + written_bytes,
                                    strlen(https_request_data) - written_bytes);
            if (ret >= 0) {
                BLINKER_LOG_ALL(TAG, "%d bytes written", ret);
                written_bytes += ret;
            } else if (ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_WANT_READ) {
                BLINKER_LOG_ALL(TAG, "mbedtls_ssl_write returned -0x%x", -ret);
                goto exit;
            }
        } while(written_bytes < strlen(https_request_data));

        BLINKER_LOG_ALL(TAG, "Reading HTTP response...");

        do
        {
            len = sizeof(buf) - 1;
            bzero(buf, sizeof(buf));
            ret = mbedtls_ssl_read(&ssl, (unsigned char *)buf, len);

            if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
                continue;

            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                ret = 0;
                break;
            }

            if(ret < 0)
            {
                BLINKER_LOG_ALL(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
                break;
            }

            if(ret == 0)
            {
                need_read = 0;
                BLINKER_LOG_ALL(TAG, "connection closed");
                break;
            }

            len = ret;
            BLINKER_LOG_ALL(TAG, "%d bytes read", len);
            // BLINKER_LOG_ALL(TAG, "need_read:%d", need_read);
            /* Print response directly to stdout as it is read */
            for(int i = 0; i < len; i++) {
                putchar(buf[i]);
                if (need_read) payload[check_num] = buf[i];
                check_num++;
                if (buf[i] == '\n')
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
        } while(1);

        mbedtls_ssl_close_notify(&ssl);

    exit:
        mbedtls_ssl_session_reset(&ssl);
        mbedtls_net_free(&server_fd);

        if(ret != 0)
        {
            mbedtls_strerror(ret, buf, 100);
            BLINKER_LOG_ALL(TAG, "Last error was: -0x%x - %s", -ret, buf);
        }

        putchar('\n'); // JSON output doesn't have a newline at end

        // static int request_count;
        // BLINKER_LOG_ALL(TAG, "Completed %d requests", ++request_count);

        // for(int countdown = 10; countdown >= 0; countdown--) {
        //     BLINKER_LOG_ALL(TAG, "%d...", countdown);
        //     vTaskDelay(1000 / portTICK_PERIOD_MS);
        // }
        // BLINKER_LOG_ALL(TAG, "Starting again!");

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

                        // mdns_free();

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
                    
                    isMQTTinit = 1;

                    xEventGroupSetBits(http_event_group, isMQTTinit);
                    
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
                        // mdns_free(); 
                        uint8_t _len = strlen(_auth->valuestring) + 1;
                        blinker_authkey = (char *)malloc(_len*sizeof(char));
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
            BLINKER_LOG_ALL(TAG, "check_duer_print_span failed");
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

int8_t blinker_print(char *data, uint8_t need_check)
{
    // if (dataFrom_MQTT == BLINKER_MSG_FROM_WS)
    // {
    //     dataFrom_MQTT = BLINKER_MSG_FROM_MQTT;

    //     if (need_check)
    //     {
    //         if (!check_print_span())
    //         {                
    //             respTime = millis();
    //             return 0;
    //         }
    //     }

    //     respTime = millis();

    //     char *_data;

    //     _data = (char*)malloc((strlen(data)+2)*sizeof(char));
    //     strcpy(_data, data);
    //     strcat(_data, "\n");

    //     return blinker_ws_print(_data);
    // }
    // else
    // {
        return blinker_mqtt_print(data, need_check);
    // }    
}

