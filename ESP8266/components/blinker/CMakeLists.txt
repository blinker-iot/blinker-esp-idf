set(include_dirs include)
set(srcs 
    "src/api/blinker_api.c"
    "src/mqtt/blinker_mqtt.c"
    "src/provisioning/blinker_prov_apconfig.c"
    "src/provisioning/blinker_prov_smartconfig.c"
    "src/provisioning/blinker_wifi.c"
    "src/utils/blinker_button.c"
    "src/utils/blinker_http.c"
    "src/utils/blinker_mdns.c"
    "src/utils/blinker_storage.c"
    "src/utils/blinker_timesync.c"
    "src/utils/blinker_utils.c")

set(COMPONENT_REQUIRES "mdns")

register_component()