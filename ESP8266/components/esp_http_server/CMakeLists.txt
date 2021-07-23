set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_PRIV_INCLUDEDIRS src/port/esp8266 src/util)
set(COMPONENT_SRCS "src/httpd_main.c"
                   "src/httpd_parse.c"
                   "src/httpd_sess.c"
                   "src/httpd_txrx.c"
                   "src/httpd_uri.c"
                   "src/httpd_ws.c"
                   "src/util/ctrl_sock.c")

set(COMPONENT_PRIV_REQUIRES lwip)
set(COMPONENT_REQUIRES http_parser)

register_component()
