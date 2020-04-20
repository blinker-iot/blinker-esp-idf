// /* Hello World Example

//    This example code is in the Public Domain (or CC0 licensed, at your option.)

//    Unless required by applicable law or agreed to in writing, this
//    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//    CONDITIONS OF ANY KIND, either express or implied.
// */
// #include <stdio.h>
// #include "sdkconfig.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_system.h"
// #include "esp_spi_flash.h"

// #include "Blinker.h"

// #ifdef CONFIG_IDF_TARGET_ESP32
// #define CHIP_NAME "ESP32"
// #endif

// #ifdef CONFIG_IDF_TARGET_ESP32S2BETA
// #define CHIP_NAME "ESP32-S2 Beta"
// #endif

// void app_main(void)
// {
//     printf("Hello world!\n");

//     /* Print chip information */
//     esp_chip_info_t chip_info;
//     esp_chip_info(&chip_info);
//     printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
//             CHIP_NAME,
//             chip_info.cores,
//             (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
//             (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

//     printf("silicon revision %d, ", chip_info.revision);

//     printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
//             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

//     for (int i = 10; i >= 0; i--) {
//         printf("Restarting in %d seconds...\n", i);
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
//     printf("Restarting now.\n");
//     fflush(stdout);
//     esp_restart();
// }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Blinker.h"

static const char *TAG = "blinker";

BlinkerButton button1 = {.name = "btn-abc"};
BlinkerNumber number1 = {.name = "num-abc"};

int counter = 0;

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
    // sprintf(count, "%d", counter);
    itoa(counter, count, 10);

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

    blinker_init();
}
