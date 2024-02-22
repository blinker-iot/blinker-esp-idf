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
#include <stdbool.h>

#include "blinker_api.h"

static const char *TAG = "blinker";
static cJSON *button_param = NULL;
static cJSON *num_param = NULL;
static int count = 0;

void button1_callback(const blinker_widget_param_val_t *val)
{
    if (val == NULL) {
        ESP_LOGE(TAG, "Invalid parameter in button1_callback");
        return;
    }

    ESP_LOGI(TAG, "button state: %s", val->s);
    count++;

    if (button_param == NULL) {
        button_param = cJSON_CreateObject();
        if (button_param == NULL) {
            ESP_LOGE(TAG, "Failed to create cJSON object for button");
            return;
        }
    }
    blinker_widget_switch(button_param, val->s);
    blinker_widget_print(BUTTON_1, button_param);

    if (num_param == NULL) {
        num_param = cJSON_CreateObject();
        if (num_param == NULL) {
            ESP_LOGE(TAG, "Failed to create cJSON object for number");
            return;
        }
    }
    blinker_widget_color(num_param, "#FF00FF");
    blinker_widget_text(num_param, "按键测试");
    blinker_widget_unit(num_param, "次");
    blinker_widget_value_number(num_param, count);
    blinker_widget_print(NUM_1, num_param);
}

static void data_callback(const char *data)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "Invalid parameter in data_callback");
        return;
    }
    ESP_LOGI(TAG, "data: %s", data);
}

void app_main()
{
    blinker_init();

    blinker_widget_add(BUTTON_1, BLINKER_BUTTON, button1_callback);
    blinker_data_handler(data_callback);
}