// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#define BUTTON_IO_NUM  0
#define BUTTON_ACTIVE_LEVEL   0
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "unity.h"
#include "iot_button.h"
#include "esp_system.h"
#include "esp_log.h"

static const char* TAG_BTN = "BTN_TEST";

void button_tap_cb(void* arg)
{
    char* pstr = (char*) arg;
    ESP_EARLY_LOGI(TAG_BTN, "tap cb (%s), heap: %d\n", pstr, esp_get_free_heap_size());
}

void button_press_2s_cb(void* arg)
{
    ESP_EARLY_LOGI(TAG_BTN, "press 2s, heap: %d\n", esp_get_free_heap_size());
}

void button_press_5s_cb(void* arg)
{
    ESP_EARLY_LOGI(TAG_BTN, "press 5s, heap: %d\n", esp_get_free_heap_size());
}

void button_test()
{
    printf("before btn init, heap: %d\n", esp_get_free_heap_size());
    button_handle_t btn_handle = iot_button_create(BUTTON_IO_NUM, BUTTON_ACTIVE_LEVEL);
    iot_button_set_evt_cb(btn_handle, BUTTON_CB_PUSH, button_tap_cb, "PUSH");
    iot_button_set_evt_cb(btn_handle, BUTTON_CB_RELEASE, button_tap_cb, "RELEASE");
    iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, button_tap_cb, "TAP");
    iot_button_set_serial_cb(btn_handle, 2, 1000/portTICK_RATE_MS, button_tap_cb, "SERIAL");

    iot_button_add_custom_cb(btn_handle, 2, button_press_2s_cb, NULL);
    iot_button_add_custom_cb(btn_handle, 5, button_press_5s_cb, NULL);
    printf("after btn init, heap: %d\n", esp_get_free_heap_size());

    vTaskDelay(100000 / portTICK_PERIOD_MS);
    printf("free btn: heap:%d\n", esp_get_free_heap_size());
    iot_button_delete(btn_handle);
    printf("after free btn: heap:%d\n", esp_get_free_heap_size());
}

TEST_CASE("Button test", "[button][iot]")
{
    button_test();
}
