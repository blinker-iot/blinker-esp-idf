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

int counter = 0;

uint8_t colorR, colorG, colorB, colorW;
uint8_t wsState = 0;
uint8_t wsMode = BLINKER_CMD_MIOT_DAY;
uint32_t colorTemp;

void miot_power_state(const char *state)
{
    BLINKER_LOG(TAG, "need set power state: %s", state);

    blinker_miot_config_t config = {
        .power_state = state,
    };

    if (strcmp("True", state) == 0) wsState = 1;
    else wsState = 0;

    blinker_miot_print(&config);
}

uint32_t getColor()
{
    return colorR << 16 | colorG << 8 | colorB;
}

void miot_color(uint32_t color)
{
    BLINKER_LOG(TAG, "need set color: %d", color);

    colorR = color >> 16 & 0xFF;
    colorG = color >>  8 & 0xFF;
    colorB = color       & 0xFF;

    BLINKER_LOG(TAG, "colorR: %d, colorG: %d, colorB: %d", colorR, colorG, colorB);

    char _color[10];

    sprintf(_color, "%d", color);

    blinker_miot_config_t config = {
        .color = _color,
    };    

    blinker_miot_print(&config);
}

void miot_bright(uint32_t bright)
{
    BLINKER_LOG(TAG, "need set brightness: %d", bright);

    colorW = bright;

    char _colorW[10];

    sprintf(_colorW, "%d", colorW);

    blinker_miot_config_t config = {
        .brightness = _colorW,
    };    

    blinker_miot_print(&config);
}

void miot_color_temp(int32_t cTemp)
{
    BLINKER_LOG(TAG, "need set colorTemperature: %d", cTemp);

    colorTemp = cTemp;

    char _colorTemp[10];

    sprintf(_colorTemp, "%d", colorTemp);

    blinker_miot_config_t config = {
        .color_temp = _colorTemp,
    };    

    blinker_miot_print(&config);
}

void miot_mode(uint8_t mode)
{
    BLINKER_LOG(TAG, "need set mode: %d", mode);

    if (mode == BLINKER_CMD_MIOT_DAY) {
        // Your mode function
    }
    else if (mode == BLINKER_CMD_MIOT_NIGHT) {
        // Your mode function
    }
    else if (mode == BLINKER_CMD_MIOT_COLOR) {
        // Your mode function
    }
    else if (mode == BLINKER_CMD_MIOT_WARMTH) {
        // Your mode function
    }
    else if (mode == BLINKER_CMD_MIOT_TV) {
        // Your mode function
    }
    else if (mode == BLINKER_CMD_MIOT_READING) {
        // Your mode function
    }
    else if (mode == BLINKER_CMD_MIOT_COMPUTER) {
        // Your mode function
    }

    wsMode = mode;

    char _mode[10];

    sprintf(_mode, "%d", wsMode);

    blinker_miot_config_t config = {
        .mode = _mode,
    };    

    blinker_miot_print(&config);
}

void miot_query(int32_t queryCode)
{
    BLINKER_LOG(TAG, "MIOT Query codes: %d", queryCode);
    
    char _color[10];

    sprintf(_color, "%d", getColor());

    char _mode[10];

    sprintf(_mode, "%d", wsMode);

    char _colorW[10];

    sprintf(_colorW, "%d", colorW);

    char _colorTemp[10];

    sprintf(_colorTemp, "%d", colorTemp);

    blinker_miot_config_t config = {
        .power_state = wsState ? "True" : "False",
        .color = _color,
        .mode = _mode,
        .color_temp = _colorTemp,
        .brightness = _colorW,
    };

    switch (queryCode)
    {
        case BLINKER_CMD_QUERY_ALL_NUMBER :
            BLINKER_LOG(TAG, "MIOT Query All");
            break;
        case BLINKER_CMD_QUERY_POWERSTATE_NUMBER :
            BLINKER_LOG(TAG, "MIOT Query Power State");
            break;
        case BLINKER_CMD_QUERY_COLOR_NUMBER :
            BLINKER_LOG(TAG, "MIOT Query Color");
            break;
        case BLINKER_CMD_QUERY_MODE_NUMBER :
            BLINKER_LOG(TAG, "MIOT Query Mode");
            break;
        case BLINKER_CMD_QUERY_COLORTEMP_NUMBER :
            BLINKER_LOG(TAG, "MIOT Query ColorTemperature");
            break;
        case BLINKER_CMD_QUERY_BRIGHTNESS_NUMBER :
            BLINKER_LOG(TAG, "MIOT Query Brightness");
            break;
        default :
            break;
    }

    blinker_miot_print(&config);
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
    
    blinker_button_init(&button1, button1_callback);
    blinker_attach_data(data_callback);

    blinker_miot_power_state_init(miot_power_state);
    blinker_miot_color_init(miot_color);
    blinker_miot_brightness_init(miot_bright);
    blinker_miot_color_temperature_init(miot_color_temp);
    blinker_miot_mode_init(miot_mode);
    blinker_miot_query_init(miot_query);

    blinker_init();
}
