#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "blinker_utils.h"

#define BLINKER_FORMAT_QUEUE_TIME       200

#define BLINKER_FORMAT_DATA_SIZE        1024

#define BLINKER_MIN_TO_S                60

#define BLINKER_MIN_TO_MS               BLINKER_MIN_TO_S * 1000UL

#define BLINKER_REGISTER_INTERVAL       20000U

#define BLINKER_WEBSOCKET_SERVER_PORT   81

#define BLINKER_MAX_TIME_INTERVAL       600

#define BLINKER_MIN_TIME_INTERVAL       30

#define BLINKER_MAX_TIMES_COUNT         10

#define BLINKER_MIN_TIMES_COUNT         2

#if defined CONFIG_BLINKER_CUSTOM_ESP

#define BLINKER_REGISTER_TASK_DEEP      3

#elif defined CONFIG_BLINKER_PRO_ESP

#define BLINKER_REGISTER_TASK_DEEP      3
#endif

#define BLINKER_CMD_DETAIL              "detail"

#define BLINKER_CMD_AUTHKEY             "authKey"

#define BLINKER_CMD_REGISTER            "register"

#define BLINKER_CMD_MESSAGE             "message"

#define BLINKER_CMD_SUCCESS             "success"

#define BLINKER_CMD_REGISTER_SUCCESS    "Registration successful"

#define BLINKER_CMD_WIFI_CONFIG         "wifi_config"

#define BLINKER_CMD_DEVICE_NAME         "deviceName"

#define BLINKER_CMD_TO_DEVICE           "toDevice"

#define BLINKER_CMD_FROM_DEVICE         "fromDevice"

#define BLINKER_CMD_FROM                "from"

#define BLINKER_CMD_DEVICE_TYPE         "deviceType"

#define BLINKER_CMD_OWN_APP             "OwnApp"

#define BLINKER_CMD_VASSISTANT          "vAssistant"

#define BLINKER_CMD_SERVER_SENDER       "ServerSender"

#define BLINKER_CMD_SERVER_RECEIVER     "ServerReceiver"

#define BLINKER_CMD_MESSAGE_ID          "messageId"

#define BLINKER_CMD_USER_NAME           "iotId"

#define BLINKER_CMD_PASS_WORD           "iotToken"

#define BLINKER_CMD_PRODUCT_KEY         "productKey"

#define BLINKER_CMD_BROKER              "broker"

#define BLINKER_CMD_ALIYUN              "aliyun"

#define BLINKER_CMD_BLINKER             "blinker"

#define BLINKER_CMD_UUID                "uuid"

#define BLINKER_CMD_HOST                "host"

#define BLINKER_CMD_PORT                "port"

#define BLINKER_CMD_DATA                "data"

#define BLINKER_CMD_GET                 "get"

#define BLINKER_CMD_SET                 "set"

#define BLINKER_CMD_STATE               "state"

#define BLINKER_CMD_ONLINE              "online"

#define BLINKER_CMD_VERSION             "version"

#define BLINKER_CMD_BUILTIN_SWITCH      "switch"

#define BLINKER_CMD_SWITCH              "swi"

#define BLINKER_CMD_ICON                "ico"

#define BLINKER_CMD_COLOR               "clr"

#define BLINKER_CMD_COLOR_              "col"

#define BLINKER_CMD_TEXT                "tex"

#define BLINKER_CMD_TEXT1               "tex1"

#define BLINKER_CMD_TEXT_COLOR          "tco"

#define BLINKER_CMD_VALUE               "val"

#define BLINKER_CMD_IMAGE               "img"

#define BLINKER_CMD_UNIT                "uni"

#define BLINKER_CMD_ALIGENIE            "AliGenie"

#define BLINKER_CMD_DUEROS              "DuerOS"

#define BLINKER_CMD_MIOT                "MIOT"

#define BLINKER_CMD_POWER_STATE         "pState"

#define BLINKER_CMD_TRUE                "true"

#define BLINKER_CMD_FALSE               "false"

#define BLINKER_CMD_ON                  "on"

#define BLINKER_CMD_OFF                 "off"

#define BLINKER_CMD_MODE                "mode"

#define BLINKER_CMD_COLOR_TEMP          "colTemp"

#define BLINKER_CMD_COLOR_TEMP_UP       "upColTemp"

#define BLINKER_CMD_COLOR_TEMP_DOWN     "downColTemp"

#define BLINKER_CMD_BRIGHTNESS          "bright"

#define BLINKER_CMD_BRIGHTNESS_UP       "upBright"

#define BLINKER_CMD_BRIGHTNESS_DOWN     "downBright"

#define BLINKER_CMD_TEMP                "temp"

#define BLINKER_CMD_TEMP_UP             "upTemp"

#define BLINKER_CMD_TEMP_DOWN           "downTemp"

#define BLINKER_CMD_HUMI                "humi"

#define BLINKER_CMD_LEVEL               "level"

#define BLINKER_CMD_LEVEL_UP            "upLevel"

#define BLINKER_CMD_LEVEL_DOWN          "downLevel"

#define BLINKER_CMD_HSTATE              "hsState"

#define BLINKER_CMD_VSTATE              "vsState"

#define BLINKER_CMD_PM25                "pm25"

#define BLINKER_CMD_PM10                "pm10"

#define BLINKER_CMD_CO2                 "co2"

#define BLINKER_CMD_AQI                 "aqi"

#define BLINKER_CMD_ECO                 "eco"

#define BLINKER_CMD_ANION               "anion"

#define BLINKER_CMD_HEATER              "heater"

#define BLINKER_CMD_DRYER               "dryer"

#define BLINKER_CMD_SOFT                "soft"

#define BLINKER_CMD_UV                  "uv"

#define BLINKER_CMD_UNSB                "unsb"

#define BLINKER_CMD_NUM                 "num"

#define BLINKER_CMD_TIME                "time"

#define BLINKER_CMD_SSID                "ssid"

#define BLINKER_CMD_PASSWORD            "pswd"

#if defined(CONFIG_BLINKER_WITH_SSL)

#define BLINKER_PROTOCOL_MQTT           "mqtts"

#define BLINKER_PROTPCOL_HTTP           "https"

#else

#define BLINKER_PROTOCOL_MQTT           "mqtt"

#define BLINKER_PROTPCOL_HTTP           "http"

#endif

#ifdef __cplusplus
}
#endif