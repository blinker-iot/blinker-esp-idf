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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "unity.h"
#include "iot_button.h"
#include "esp_log.h"

#define BUTTON_IO_NUM  GPIO_NUM_0
#define BUTTON_ACTIVE_LEVEL   BUTTON_ACTIVE_LOW
static const char* TAG_BTN = "BTN_TEST";

static void button_tap_cb(void* arg)
{
    char* pstr = (char*) arg;
    ESP_EARLY_LOGI(TAG_BTN, "tap cb (%s), heap: %d\n", pstr, esp_get_free_heap_size());
}

static void button_press_2s_cb(void* arg)
{
    ESP_EARLY_LOGI(TAG_BTN, "press 2s, heap: %d\n", esp_get_free_heap_size());
}

static void button_press_5s_cb(void* arg)
{
    ESP_EARLY_LOGI(TAG_BTN, "press 5s, heap: %d\n", esp_get_free_heap_size());
}

extern "C" void button_obj_test()
{
    const char *push = "PUSH";
    const char *release = "RELEASE";
    const char *tap = "TAP";
    const char *serial = "SERIAL";
    CButton* btn = new CButton(BUTTON_IO_NUM, BUTTON_ACTIVE_LEVEL);
    btn->set_evt_cb(BUTTON_CB_PUSH, button_tap_cb, (void*) push);
    btn->set_evt_cb(BUTTON_CB_RELEASE, button_tap_cb, (void*) release);
    btn->set_evt_cb(BUTTON_CB_TAP, button_tap_cb, (void*) tap);
    btn->set_evt_cb(BUTTON_CB_SERIAL, button_tap_cb, (void*) serial);
    
    btn->add_custom_cb(2, button_press_2s_cb, NULL);
    btn->add_custom_cb(5, button_press_5s_cb, NULL);

    vTaskDelay(10000 / portTICK_PERIOD_MS);
    delete btn;
    ESP_LOGI(TAG_BTN, "button is deleted");
}

TEST_CASE("Button cpp test", "[button_cpp][iot]")
{
    button_obj_test();
}
