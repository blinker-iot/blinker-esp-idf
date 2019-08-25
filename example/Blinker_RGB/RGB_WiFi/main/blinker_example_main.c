/* *****************************************************************
 *
 * Download latest Blinker library here:
 * https://github.com/blinker-iot/blinker-freertos/archive/master.zip
 * 
 * 
 * Blinker is a cross-hardware, cross-platform solution for the IoT. 
 * It provides APP, device and server support, 
 * and uses public cloud services for data transmission and storage.
 * It can be used in smart home, data monitoring and other fields 
 * to help users build Internet of Things projects better and faster.
 * 
 * Docs: https://doc.blinker.app/
 *       https://github.com/blinker-iot/blinker-doc/wiki
 * 
 * *****************************************************************
 * 
 * Blinker 库下载地址:
 * https://github.com/blinker-iot/blinker-freertos/archive/master.zip
 * 
 * Blinker 是一套跨硬件、跨平台的物联网解决方案，提供APP端、设备端、
 * 服务器端支持，使用公有云服务进行数据传输存储。可用于智能家居、
 * 数据监测等领域，可以帮助用户更好更快地搭建物联网项目。
 * 
 * 文档: https://doc.blinker.app/
 *       https://github.com/blinker-iot/blinker-doc/wiki
 * 
 * *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Blinker.h"

static const char *TAG = "blinker";

char auth[] = "Your Device Secret Key";
char ssid[] = "Your WiFi network SSID or name";
char pswd[] = "Your WiFi network WPA password or WEP key";

BlinkerButton button1 = {.name = "btn-abc"};
BlinkerNumber number1 = {.name = "num-abc"};
BlinkerRGB rgb1 = {.name = "rgbkey"};

int counter = 0;

void rgb1_callback(uint8_t r_value, uint8_t g_value, uint8_t b_value, uint8_t bright_value)
{
    BLINKER_LOG(TAG, "R value: %d", r_value);
    BLINKER_LOG(TAG, "G value: %d", g_value);
    BLINKER_LOG(TAG, "B value: %d", b_value);
    BLINKER_LOG(TAG, "Rrightness value: %d", bright_value);

    blinker_rgb_config_t config = {
        .rgbw = {r_value, g_value, b_value, bright_value},
    };

    blinker_rgb_print(&rgb1, &config);
}

void button1_callback(const char *data)
{
    BLINKER_LOG(TAG, "get button data: %s", data);

    blinker_button_config_t config = {
        .icon = "fas fa-alicorn",
        .color = "0xFF",
        .text1 = "test",
    };

    blinker_button_print(&button1, &config);
}

void data_callback(const cJSON *data)
{
    BLINKER_LOG(TAG, "get json data");

    counter++;

    char count[10];
    sprintf(count, "%d", counter);

    blinker_number_config_t config = {
        .value = count,
    };

    blinker_number_print(&number1, &config);
}

void app_main()
{
    BLINKER_DEBUG_ALL();
    blinker_config_t init_conf = {
        .type = BLINKER_WIFI,
        .wifi = BLINKER_DEFAULT_CONFIG,
    };
    blinker_init(&init_conf);
    blinker_button_init(&button1, button1_callback);
    blinker_rgb_init(&rgb1, rgb1_callback);
    blinker_attach_data(data_callback);

    Blinker.begin(auth, ssid, pswd);
}
