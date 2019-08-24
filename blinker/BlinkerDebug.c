#include "BlinkerDebug.h"
// #include "esp_log.h"
// #include "esp_system.h"
// #include "esp_wifi.h"

enum blinker_debug_level_t
{
    _debug_none,
    _debug_default,
    _debug_all
};

enum blinker_debug_level_t debug_level = _debug_default;

void BLINKER_DEBUG_DISABLE(void) { debug_level = _debug_none; }

void BLINKER_DEBUG_ALL(void) { debug_level = _debug_all; }

int8_t isDebug(void) { return debug_level != _debug_none; }

int8_t isDebugAll(void) { return debug_level == _debug_all; }