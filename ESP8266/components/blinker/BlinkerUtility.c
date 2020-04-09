#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "BlinkerUtility.h"

char macStr[13];

char * macDeviceName(void)
{
    uint8_t mac[6];
    // char macStr;

    // esp_base_mac_addr_get(mac);
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    // ESP_LOGI(TAG, "%d", mac);
    sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return macStr;
}

uint32_t millis(void) { return esp_log_early_timestamp(); }

char * trim(char *data)
{
    for (uint16_t num = 0; num < strlen(data); num++)
    {
        if (data[num] == '\n' || data[num] == '\r' || data[num] == '\t')
        {
            for (uint16_t _num = num; _num < (strlen(data) -1); _num++)
            {
                data[_num] = data[_num + 1];
            }
            data[strlen(data) - 1] = '\0';
            num--;
        }
    }

    return data;
}

int8_t isJson(const char *data)
{
    cJSON *root = cJSON_Parse(data);

    if (root == NULL)
    {
        cJSON_Delete(root);
        return 0;
    }
    else
    {
        cJSON_Delete(root);
        return 1;
    }
    
}
