
/*
esp32-websocket - a websocket component on esp-idf
Copyright (C) 2019 Blake Felt - blake.w.felt@gmail.com

This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include "websocket.h"

#define WEBSOCKET_SERVER_MAX_CLIENTS 5//CONFIG_WEBSOCKET_SERVER_MAX_CLIENTS
#define WEBSOCKET_SERVER_QUEUE_SIZE 4//CONFIG_WEBSOCKET_SERVER_QUEUE_SIZE
#define WEBSOCKET_SERVER_QUEUE_TIMEOUT 30//CONFIG_WEBSOCKET_SERVER_QUEUE_TIMEOUT
#define WEBSOCKET_SERVER_TASK_STACK_DEPTH 1536//CONFIG_WEBSOCKET_SERVER_TASK_STACK_DEPTH
#define WEBSOCKET_SERVER_TASK_PRIORITY 5//CONFIG_WEBSOCKET_SERVER_TASK_PRIORITY
#define WEBSOCKET_SERVER_PINNED false//CONFIG_WEBSOCKET_SERVER_PINNED
#if WEBSOCKET_SERVER_PINNED
#define WEBSOCKET_SERVER_PINNED_CORE 0//CONFIG_WEBSOCKET_SERVER_PINNED_CORE
#endif

// starts the server
int ws_server_start();

// ends the server
int ws_server_stop();

// adds a client, returns the client's number in the server
int ws_server_add_client(struct netconn* conn,
                         char* msg,
                         uint16_t len,
                         char* url,
                         void (*callback)(uint8_t num,
                                          WEBSOCKET_TYPE_t type,
                                          char* msg,
                                          uint64_t len));

int ws_server_add_client_protocol(struct netconn* conn,
                                  char* msg,
                                  uint16_t len,
                                  char* url,
                                  char* protocol,
                                  void (*callback)(uint8_t num,
                                                   WEBSOCKET_TYPE_t type,
                                                   char* msg,
                                                   uint64_t len));

int ws_server_len_url(char* url); // returns the number of connected clients to url
int ws_server_len_all(); // returns the total number of connected clients

int ws_server_remove_client(int num); // removes the client with the set number
int ws_server_remove_clients(char* url); // removes all clients connected to the specified url
int ws_server_remove_all(); // removes all clients from the server

int ws_server_send_text_client(int num,char* msg,uint64_t len); // send text to client with the set number
int ws_server_send_text_clients(char* url,char* msg,uint64_t len); // sends text to all clients with the set number
int ws_server_send_text_all(char* msg,uint64_t len); // sends text to all clients

int ws_server_send_bin_client(int num,char* msg,uint64_t len);
int ws_server_send_bin_clients(char* url,char* msg,uint64_t len);
int ws_server_send_bin_all(char* msg,uint64_t len);

// these versions can be sent from the callback ONLY

int ws_server_send_text_client_from_callback(int num,char* msg,uint64_t len); // send text to client with the set number
int ws_server_send_text_clients_from_callback(char* url,char* msg,uint64_t len); // sends text to all clients with the set number
int ws_server_send_text_all_from_callback(char* msg,uint64_t len); // sends text to all clients

int ws_server_ping(); // sends a ping to all connected clients

#endif

#ifdef __cplusplus
}
#endif
