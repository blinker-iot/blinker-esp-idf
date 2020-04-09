#ifndef BLINKER_CONFIG_H
#define BLINKER_CONFIG_H

#include "BlinkerServer.h"

#define BLINKER_VERSION                 "0.1.0"

#define BLINKER_CONNECT_TIMEOUT_MS      10000UL

#define BLINKER_STREAM_TIMEOUT          100

#define BLINKER_NTP_TIMEOUT             1000UL

#define BLINKER_GPS_MSG_LIMIT           30000UL

#define BLINKER_PRINT_MSG_LIMIT         20

#define BLINKER_MQTT_MSG_LIMIT          1000UL

#define BLINKER_PRO_MSG_LIMIT           200UL

#define BLINKER_MQTT_CONNECT_TIMESLOT   5000UL

#define BLINKER_BRIDGE_MSG_LIMIT        10000UL

#define BLINKER_LINK_MSG_LIMIT          10000UL

#define BLINKER_MQTT_KEEPALIVE          30000UL

#define BLINKER_SMS_MSG_LIMIT           60000UL

#define BLINKER_PUSH_MSG_LIMIT          60000UL

#define BLINKER_WECHAT_MSG_LIMIT        60000UL

#define BLINKER_WEATHER_MSG_LIMIT       60000UL

#define BLINKER_AQI_MSG_LIMIT           60000UL

#define BLINKER_CONFIG_UPDATE_LIMIT     10000UL

#define BLINKER_CONFIG_GET_LIMIT        10000UL

#define BLINKER_WIFI_AUTO_INIT_TIMEOUT  20000UL

#define BLINKER_AT_MSG_TIMEOUT          1000UL

#define BLINKER_SERVER_CONNECT_LIMIT    12

#if defined(BLINKER_DATA_HOUR_UPDATE)
    #define BLINKER_DATA_FREQ_TIME          3600UL
#else
    #define BLINKER_DATA_FREQ_TIME          60
#endif

#define BLINKER_DEVICE_HEARTBEAT_TIME   600

#define BLINKER_MDNS_SERVICE_BLINKER    "blinker"

#define BLINKER_ERROR                   0x00

#define BLINKER_SUCCESS                 0x01

#define BLINKER_MSG_FROM_MQTT           0

#define BLINKER_MSG_FROM_WS             1

#define BLINKER_MSG_AUTOFORMAT_TIMEOUT  100

#define BLINKER_CMD_ON                  "on"

#define BLINKER_CMD_OFF                 "off"

#define BLINKER_CMD_TRUE                "true"

#define BLINKER_CMD_FALSE               "false"

#define BLINKER_CMD_JOYSTICK            "joy"

#define BLINKER_CMD_GYRO                "gyro"

#define BLINKER_CMD_AHRS                "ahrs"

#define BLINKER_CMD_GPS                 "gps"

#define BLINKER_CMD_RGB                 "rgb"

#define BLINKER_CMD_VIBRATE             "vibrate"

#define BLINKER_CMD_BUTTON_TAP          "tap"

#define BLINKER_CMD_BUTTON_PRESSED      "press"

#define BLINKER_CMD_BUTTON_RELEASED     "pressup"

#define BLINKER_CMD_BUTTON_PRESS        "press"

#define BLINKER_CMD_BUTTON_PRESSUP      "pressup"

#define BLINKER_CMD_NEWLINE             "\n"

#define BLINKER_CMD_INTERSPACE          " "

#define BLINKER_CMD_DATA                "data"

#define BLINKER_CMD_FREQ                "freq"

#define BLINKER_CMD_GET                 "get"

#define BLINKER_CMD_SET                 "set"

#define BLINKER_CMD_STATE               "state"

#define BLINKER_CMD_ONLINE              "online"

#define BLINKER_CMD_CONNECTED           "connected"

#define BLINKER_CMD_VERSION             "version"

#define BLINKER_CMD_NOTICE              "notice"

#define BLINKER_CMD_BUILTIN_SWITCH      "switch"

#define BLINKER_CMD_FROMDEVICE          "fromDevice"

#define BLINKER_CMD_NOTFOUND            "device not found"

#define BLINKER_CMD_COMMAND             "cmd"

#define BLINKER_CMD_EVENT               "event"

#define BLINKER_CMD_AUTO                "auto"

#define BLINKER_CMD_AUTOID              "autoId"

#define BLINKER_CMD_ID                  "id"

#define BLINKER_CMD_AUTODATA            "autoData"

#define BLINKER_CMD_DELETID             "deletId"

#define BLINKER_CMD_LOGIC               "logic"

#define BLINKER_CMD_LOGICDATA           "logicData"

#define BLINKER_CMD_LOGICTYPE           "logicType"

#define BLINKER_CMD_LESS                "<"//"less"

#define BLINKER_CMD_EQUAL               "="//"equal"

#define BLINKER_CMD_GREATER             ">"//"greater"

#define BLINKER_CMD_NUMBERIC            "numberic"

#define BLINKER_CMD_OR                  "or"

#define BLINKER_CMD_AND                 "and"

#define BLINKER_CMD_COMPARETYPE         "compareType"

#define BLINKER_CMD_TRIGGER             "trigger"

#define BLINKER_CMD_DURATION            "duration"

#define BLINKER_CMD_TARGETKEY           "targetKey"

#define BLINKER_CMD_TARGETSTATE         "targetState"

#define BLINKER_CMD_TARGETDATA          "targetData"

#define BLINKER_CMD_TIMESLOT            "timeSlot"

#define BLINKER_CMD_RANGE               "range"

#define BLINKER_CMD_LINKDEVICE          "linkDevice"

#define BLINKER_CMD_LINKTYPE            "linkType"

#define BLINKER_CMD_LINKDATA            "linkData"

#define BLINKER_CMD_TRIGGEDDATA         "triggedData"

#define BLINKER_CMD_TYPE                "type"

#define BLINKER_CMD_TIMER               "timer"

#define BLINKER_CMD_RUN                 "run"

#define BLINKER_CMD_ENABLE              "ena"

#define BLINKER_CMD_COUNTDOWN           "countdown"

#define BLINKER_CMD_COUNTDOWNDATA       "countdownData"

#define BLINKER_CMD_TOTALTIME           "ttim"

#define BLINKER_CMD_RUNTIME             "rtim"

#define BLINKER_CMD_ACTION              "act"

#define BLINKER_CMD_ACTION1             "act1"

#define BLINKER_CMD_ACTION2             "act2"

#define BLINKER_CMD_LOOP                "loop"

#define BLINKER_CMD_LOOPDATA            "loopData"

#define BLINKER_CMD_TIME                "tim"

#define BLINKER_CMD_TIME_ALL            "time"

#define BLINKER_CMD_TIMES               "tis"

#define BLINKER_CMD_TRIGGED             "tri"

#define BLINKER_CMD_TIME1               "dur1"

#define BLINKER_CMD_TIME2               "dur2"

#define BLINKER_CMD_TIMING              "timing"

#define BLINKER_CMD_TIMINGDATA          "timingData"

#define BLINKER_CMD_DAY                 "day"

#define BLINKER_CMD_TASK                "task"

#define BLINKER_CMD_DELETETASK          "dlt"

#define BLINKER_CMD_DELETE              "dlt"

#define BLINKER_CMD_DETAIL              "detail"

#define BLINKER_CMD_OK                  "OK"

#define BLINKER_CMD_ERROR               "ERROR"

#define BLINKER_CMD_MESSAGE             "message"

#define BLINKER_CMD_DEVICENAME          "deviceName"

#define BLINKER_CMD_AUTHKEY             "authKey"

#define BLINKER_CMD_IOTID               "iotId"

#define BLINKER_CMD_IOTTOKEN            "iotToken"

#define BLINKER_CMD_PRODUCTKEY          "productKey"

#define BLINKER_CMD_BROKER              "broker"

#define BLINKER_CMD_UUID                "uuid"

#define BLINKER_CMD_KEY                 "key"

#define BLINKER_CMD_SMS                 "sms"

#define BLINKER_CMD_PUSH                "push"

#define BLINKER_CMD_WECHAT              "wechat"

#define BLINKER_CMD_WEATHER             "weather"

#define BLINKER_CMD_AQI                 "aqi"

#define BLINKER_CMD_CONFIG              "config"

#define BLINKER_CMD_DEFAULT             "default"

#define BLINKER_CMD_SWITCH              "swi"

#define BLINKER_CMD_VALUE               "val"

#define BLINKER_CMD_ICON                "ico"

#define BLINKER_CMD_COLOR               "clr"

#define BLINKER_CMD_COLOR_              "col"

#define BLINKER_CMD_TITLE               "tit"

#define BLINKER_CMD_CONTENT             "con"

#define BLINKER_CMD_TEXT                "tex"

#define BLINKER_CMD_TEXT1               "tex1"

#define BLINKER_CMD_TEXTCOLOR           "tco"

#define BLINKER_CMD_UNIT                "uni"

#define BLINKER_CMD_SUMMARY             "sum"

#define BLINKER_CMD_POWERSTATE          "pState"

#define BLINKER_CMD_NUM                 "num"

#define BLINKER_CMD_BRIGHTNESS          "bright"

#define BLINKER_CMD_UPBRIGHTNESS        "upBright"

#define BLINKER_CMD_DOWNBRIGHTNESS      "downBright"

#define BLINKER_CMD_COLORTEMP           "colTemp"

#define BLINKER_CMD_UPCOLORTEMP         "upColTemp"

#define BLINKER_CMD_DOWNCOLORTEMP       "downColTemp"

#define BLINKER_CMD_TEMP                "temp"

#define BLINKER_CMD_HUMI                "humi"

#define BLINKER_CMD_PM25                "pm25"

#define BLINKER_CMD_PM10                "pm10"

#define BLINKER_CMD_CO2                 "co2"

#define BLINKER_CMD_MAX                 "max"

#define BLINKER_CMD_MIN                 "min"

#define BLINKER_CMD_MODE                "mode"

#define BLINKER_CMD_CANCELMODE          "cMode"

#define BLINKER_CMD_READING             "reading"

#define BLINKER_CMD_MOVIE               "movie"

#define BLINKER_CMD_SLEEP               "sleep"

#define BLINKER_CMD_HOLIDAY             "holiday"

#define BLINKER_CMD_MUSIC               "music"

#define BLINKER_CMD_COMMON              "common"

#define BLINKER_CMD_ALIGENIE_READING    "reading"

#define BLINKER_CMD_ALIGENIE_MOVIE      "movie"

#define BLINKER_CMD_ALIGENIE_SLEEP      "sleep"

#define BLINKER_CMD_ALIGENIE_HOLIDAY    "holiday"

#define BLINKER_CMD_ALIGENIE_MUSIC      "music"

#define BLINKER_CMD_ALIGENIE_COMMON     "common"

#define BLINKER_CMD_DUEROS_READING      "READING"

#define BLINKER_CMD_DUEROS_SLEEP        "SLEEP"

#define BLINKER_CMD_DUEROS_ALARM        "ALARM"

#define BLINKER_CMD_DUEROS_NIGHT_LIGHT  "NIGHT_LIGHT"

#define BLINKER_CMD_DUEROS_ROMANTIC     "ROMANTIC"

#define BLINKER_CMD_DUEROS_SUNDOWN      "SUNDOWN"

#define BLINKER_CMD_DUEROS_SUNRISE      "SUNRISE"

#define BLINKER_CMD_DUEROS_RELAX        "RELAX"

#define BLINKER_CMD_DUEROS_LIGHTING     "LIGHTING"

#define BLINKER_CMD_DUEROS_SUN          "SUN"

#define BLINKER_CMD_DUEROS_STAR         "STAR"

#define BLINKER_CMD_DUEROS_ENERGY_SAVING "ENERGY_SAVING"

#define BLINKER_CMD_DUEROS_MOON         "MOON"

#define BLINKER_CMD_DUEROS_JUDI         "JUDI"

#define BLINKER_CMD_MIOT_DAY            0

#define BLINKER_CMD_MIOT_NIGHT          1

#define BLINKER_CMD_MIOT_COLOR          2

#define BLINKER_CMD_MIOT_WARMTH         3

#define BLINKER_CMD_MIOT_TV             4

#define BLINKER_CMD_MIOT_READING        5

#define BLINKER_CMD_MIOT_COMPUTER       6

#define BLINKER_CMD_UPGRADE             "upgrade"

#define BLINKER_CMD_SHARE               "share"

#define BLINKER_CMD_AUTO_UPDATE_KEY     "upKey"

#define BLINKER_CMD_CANCEL_UPDATE_KEY   "cKey"

#define BLINKER_CMD_ALIGENIE            "AliGenie"

#define BLINKER_CMD_DUEROS              "DuerOS"

#define BLINKER_CMD_MIOT                "MIOT"

#define BLINKER_CMD_SERVERCLIENT        "serverClient"

#define BLINKER_CMD_HELLO               "hello"

// #define BLINKER_CMD_WHOIS               "whois"

#define BLINKER_CMD_AT                  "AT"

#define BLINKER_CMD_GATE                "gate"

#define BLINKER_CMD_CONTROL             "ctrl"

#define BLINKER_CMD_DEVICEINFO          "dInf"

#define BLINKER_CMD_NEW                 "{\"hello\":\"new\"}"

#define BLINKER_CMD_WHOIS               "{\"hello\":\"whois\"}"

#define BLINKER_CMD_TAB_0               16 // 0x10000

#define BLINKER_CMD_TAB_1               8  // 0x01000

#define BLINKER_CMD_TAB_2               4  // 0x00100

#define BLINKER_CMD_TAB_3               2  // 0x00010

#define BLINKER_CMD_TAB_4               1  // 0x00001

// #define BLINKER_SERVER_HOST             "iot.diandeng.tech"

#define BLINKER_SERVER_PORT             "443"

#define BLINKER_MQTT_BORKER_ALIYUN      "aliyun"

#define BLINKER_MQTT_ALIYUN_URL         "mqtts://public.iot-as-mqtt.cn-shanghai.aliyuncs.com:1883"

#define BLINKER_MQTT_ALIYUN_HOST        "public.iot-as-mqtt.cn-shanghai.aliyuncs.com"

#define BLINKER_MQTT_ALIYUN_PORT        1883

#define BLINKER_MSG_FROM_MQTT           0

#define BLINKER_MSG_FROM_WS             1

#define BLINKER_MAX_READ_SIZE           1024

#define BLINKER_MAX_SEND_SIZE           1024

#define BLINKER_MAX_WIDGET_SIZE         16

#define BLINKER_OBJECT_NOT_AVAIL        -1

#define false                           0

#define true                            1

#define BLINKER_ALIGENIE_LIGHT          "&aliType=light"

#define BLINKER_ALIGENIE_OUTLET         "&aliType=outlet"

#define BLINKER_ALIGENIE_MULTI_OUTLET   "&aliType=multi_outlet"

#define BLINKER_ALIGENIE_SENSOR         "&aliType=sensor"

#define BLINKER_DUEROS_LIGHT            "&duerType=LIGHT"

#define BLINKER_DUEROS_OUTLET           "&duerType=SOCKET"

#define BLINKER_DUEROS_MULTI_OUTLET     "&duerType=MULTI_SOCKET"

#define BLINKER_DUEROS_SENSOR           "&duerType=AIR_MONITOR"

#define BLINKER_MIOT_LIGHT              "&miType=light"

#define BLINKER_MIOT_OUTLET             "&miType=outlet"

#define BLINKER_MIOT_MULTI_OUTLET       "&miType=multi_outlet"

#define BLINKER_MIOT_SENSOR             "&miType=sensor"

#define BLINKER_CMD_MODE_READING_NUMBER         0

#define BLINKER_CMD_MODE_MOVIE_NUMBER           1

#define BLINKER_CMD_SLEEP_NUMBER                2

#define BLINKER_CMD_HOLIDAY_NUMBER              3

#define BLINKER_CMD_MUSIC_NUMBER                4

#define BLINKER_CMD_COMMON_NUMBER               5

#define BLINKER_CMD_QUERY_ALL_NUMBER            0

#define BLINKER_CMD_QUERY_POWERSTATE_NUMBER     1

#define BLINKER_CMD_QUERY_COLOR_NUMBER          2

#define BLINKER_CMD_QUERY_MODE_NUMBER           3

#define BLINKER_CMD_QUERY_COLORTEMP_NUMBER      4

#define BLINKER_CMD_QUERY_BRIGHTNESS_NUMBER     5

#define BLINKER_CMD_QUERY_TEMP_NUMBER           6

#define BLINKER_CMD_QUERY_HUMI_NUMBER           7

#define BLINKER_CMD_QUERY_PM25_NUMBER           8

#define BLINKER_CMD_QUERY_PM10_NUMBER           9

#define BLINKER_CMD_QUERY_CO2_NUMBER            10

#define BLINKER_CMD_QUERY_AQI_NUMBER            11

#define BLINKER_CMD_QUERY_TIME_NUMBER           12

#define BLINKER_CMD_SMS_NUMBER                  1

#define BLINKER_CMD_PUSH_NUMBER                 2

#define BLINKER_CMD_WECHAT_NUMBER               3

#define BLINKER_CMD_WEATHER_NUMBER              4

#define BLINKER_CMD_AQI_NUMBER                  5

#define BLINKER_CMD_BRIDGE_NUMBER               6

#define BLINKER_CMD_CONFIG_UPDATE_NUMBER        7

#define BLINKER_CMD_CONFIG_GET_NUMBER           8

#define BLINKER_CMD_CONFIG_DELETE_NUMBER        9

#define BLINKER_CMD_DATA_STORAGE_NUMBER         10

#define BLINKER_CMD_DATA_GET_NUMBER             11

#define BLINKER_CMD_DATA_DELETE_NUMBER          12

#define BLINKER_CMD_AUTO_PULL_NUMBER            13

#define BLINKER_CMD_OTA_NUMBER                  14

#define BLINKER_CMD_OTA_STATUS_NUMBER           15

#define BLINKER_CMD_FRESH_SHARERS_NUMBER        16

#define BLINKER_CMD_LOWPOWER_FREQ_GET_NUM       17

#define BLINKER_CMD_LOWPOWER_FREQ_UP_NUMBER     18

#define BLINKER_CMD_LOWPOWER_DATA_GET_NUM       19

#define BLINKER_CMD_LOWPOWER_DATA_UP_NUMBER     20

#define BLINKER_CMD_EVENT_DATA_NUMBER           21

#define BLINKER_CMD_GPS_DATA_NUMBER             22

#define BLINKER_CMD_DEVICE_HEARTBEAT_NUMBER     23

#define BLINKER_CMD_EVENT_WARNING_NUMBER        24

#define BLINKER_CMD_EVENT_ERROR_NUMBER          25

#define BLINKER_CMD_EVENT_MSG_NUMBER            26

#define BLINKER_CMD_DEVICE_REGISTER_NUMBER      27

#define BLINKER_CMD_DEVICE_AUTH_NUMBER          28

#define BLINKER_CMD_DEFAULT_NUMBER              0

#define BLINKER_MQTT_MAX_SHARERS_NUM            9

#define BLINKER_MQTT_FROM_AUTHER                BLINKER_MQTT_MAX_SHARERS_NUM

#define BLINKER_MQTT_FORM_SERVER                BLINKER_MQTT_MAX_SHARERS_NUM + 1

#endif
