
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

#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "lwip/api.h"

// the different codes for the callbacks
typedef enum {
  WEBSOCKET_CONNECT,
  WEBSOCKET_DISCONNECT_EXTERNAL, // the other side disconnected
  WEBSOCKET_DISCONNECT_INTERNAL, // the esp32 disconnected
  WEBSOCKET_DISCONNECT_ERROR, // disconnect due to error
  WEBSOCKET_TEXT,
  WEBSOCKET_BIN,
  WEBSOCKET_PING,
  WEBSOCKET_PONG
} WEBSOCKET_TYPE_t;

// websocket operation codes
typedef enum {
  WEBSOCKET_OPCODE_CONT  = 0x0,
  WEBSOCKET_OPCODE_TEXT  = 0x1,
  WEBSOCKET_OPCODE_BIN   = 0x2,
  WEBSOCKET_OPCODE_CLOSE = 0x8,
  WEBSOCKET_OPCODE_PING  = 0x9,
  WEBSOCKET_OPCODE_PONG  = 0xA
} WEBSOCKET_OPCODES_t;

// the header, useful for creating and quickly passing to functions
typedef struct {
  union {
    struct {
      uint16_t LEN:7;     // bits 0..  6
      uint16_t MASK:1;    // bit  7
      uint16_t OPCODE:4;  // bits 8..  11
      uint16_t :3;        // bits 12.. 14 reserved
      uint16_t FIN:1;     // bit  15
    } bit;
    struct {
      uint16_t ONE:8;     // bits 0..  7
      uint16_t ZERO:8;    // bits 8..  15
    } pos;
  } param; // the initial parameters of the header
  uint64_t length; // actual message length
  union {
    char part[4]; // the mask, array
    uint32_t full; // the mask, all 32 bits
  } key; // masking key
  bool received; // was a message successfully received?
} ws_header_t;

// a client, with space for a server callback or a client callback (depending on use)
typedef struct {
  struct netconn* conn; // the connection
  char* url;            // the associated url,  null terminated
  char* protocol;		// the associated protocol, null terminated
  bool ping;            // did we send a ping?
  WEBSOCKET_OPCODES_t last_opcode; // the previous opcode
  char* contin;         // any continuation piece
  bool contin_text;     // is the continue a binary or text?
  uint64_t len;         // length of continuation
  uint32_t unfinished;      // sometimes netconn doesn't read a full frame, treated similarly to a continuation frame
  void (*ccallback)(WEBSOCKET_TYPE_t type,char* msg,uint64_t len); // client callback
  void (*scallback)(uint8_t num,WEBSOCKET_TYPE_t type,char* msg,uint64_t len); // server callback
} ws_client_t;

// returns the populated client struct
// does not send any header, assumes the proper handshake has already occurred
// ccallback = callback for client (userspace)
// scallback = callback for server (userspace)
ws_client_t ws_connect_client(struct netconn* conn,
                              char* url,
                              void (*ccallback)(WEBSOCKET_TYPE_t type,char* msg,uint64_t len),
                              void (*scallback)(uint8_t num,WEBSOCKET_TYPE_t type,char* msg,uint64_t len)
                             );
void ws_disconnect_client(ws_client_t* client,bool mask);
bool ws_is_connected(ws_client_t client); // returns 1 if connected, status updates after send/read/connect/disconnect
int ws_send(ws_client_t* client,WEBSOCKET_OPCODES_t opcode,char* msg,uint64_t len,bool mask); // sends message. this function performs the masking
char* ws_read(ws_client_t* client,ws_header_t* header); // unmasks and returns message. populates header.
char* ws_hash_handshake(char* key,uint8_t len); // returns string of output

#endif // ifndef WEBSOCKET_H

#ifdef __cplusplus
}
#endif
