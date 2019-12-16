
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

#include "websocket.h"
#include "lwip/tcp.h" // for the netconn structure
#include "esp_system.h" // for esp_random
#include "base64.h"
#include "sha1.h"
#include <string.h>

ws_client_t ws_connect_client(struct netconn* conn,
                              char* url,
                              void (*ccallback)(WEBSOCKET_TYPE_t type,char* msg,uint64_t len),
                              void (*scallback)(uint8_t num,WEBSOCKET_TYPE_t type,char* msg,uint64_t len)
                            ) {
  ws_client_t client;
  client.conn = conn;
  client.url  = url;
  client.ping = 0;
  client.last_opcode = 0;
  client.contin = NULL;
  client.len = 0;
  client.unfinished = 0;
  client.ccallback = ccallback;
  client.scallback = scallback;
  return client;
}

void ws_disconnect_client(ws_client_t* client,bool mask) {
  ws_send(client,WEBSOCKET_OPCODE_CLOSE,NULL,0,mask); // tell the client to close
  if(client->conn) {
    client->conn->callback = NULL; // shut off the callback
    netconn_close(client->conn);
    netconn_delete(client->conn);
    client->conn = NULL;
  }
  client->url = NULL;
  client->last_opcode = 0;
  if(client->len) {
    if(client->contin)
      free(client->contin);
    client->len = 0;
  }
  client->ccallback = NULL;
  client->scallback = NULL;
}

bool ws_is_connected(ws_client_t client) {
  if(client.conn)
    return 1;
  return 0;
}

static void ws_generate_mask(ws_header_t* header) {
  header->param.bit.MASK = 1;
  header->key.full = esp_random(); // generate a random 32 bit number
}

static void ws_encrypt_decrypt(char* msg,ws_header_t header) {
  if(header.param.bit.MASK) {
    for(uint64_t i=0; i<header.length; i++) {
      msg[i] ^= header.key.part[i%4];
    }
  }
}

int ws_send(ws_client_t* client,WEBSOCKET_OPCODES_t opcode,char* msg,uint64_t len,bool mask) {
  char* out;
  char* encrypt;
  uint64_t pos;
  uint64_t true_len;
  ws_header_t header;
  int ret;

  header.param.pos.ZERO = 0; // reset the whole header
  header.param.pos.ONE  = 0;

  header.param.bit.FIN = 1; // all pieces are done (you don't need a huge message anyway...)
  header.param.bit.OPCODE = opcode;
  // populate LEN field
  pos = 2;
  header.length = len;
  if(len<=125) {
    header.param.bit.LEN = len;
  }
  else if(len<65536) {
    header.param.bit.LEN = 126;
    pos += 2;
  }
  else {
    header.param.bit.LEN = 127;
    pos += 8;
  }

  if(mask) {
    ws_generate_mask(&header); // get a key
    encrypt = malloc(len); // allocate memory for the encryption
    memcpy(encrypt,msg,len);
    ws_encrypt_decrypt(encrypt,header); // encrypt it!
    pos += 4; // add the position
  }

  true_len = pos+len; // get the length of the entire message
  pos = 2;
  out = malloc(true_len); // allocate dat memory

  out[0] = header.param.pos.ZERO; // save header
  out[1] = header.param.pos.ONE;

  // put in the length, if necessary
  if(header.param.bit.LEN == 126) {
    out[2] = (len >> 8) & 0xFF;
    out[3] = (len     ) & 0xFF;
    pos = 4;
  }
  if(header.param.bit.LEN == 127) {
    //memcpy(&out[2],&len,8);
    out[2] = (len >> 56) & 0xFF;
    out[3] = (len >> 48) & 0xFF;
    out[4] = (len >> 40) & 0xFF;
    out[5] = (len >> 32) & 0xFF;
    out[6] = (len >> 24) & 0xFF;
    out[7] = (len >> 16) & 0xFF;
    out[8] = (len >> 8)  & 0xFF;
    out[9] = (len)       & 0xFF;
    pos = 10;
  }

  if(mask) {
    out[pos] = header.key.part[0]; pos++;
    out[pos] = header.key.part[1]; pos++;
    out[pos] = header.key.part[2]; pos++;
    out[pos] = header.key.part[3]; pos++;
    memcpy(&out[pos],encrypt,len); // put in the encrypted message
    free(encrypt);
  }
  else {
    memcpy(&out[pos],msg,len);
  }

  ret = netconn_write(client->conn,out,true_len,NETCONN_COPY); // finally! send it.
  free(out); // free the entire message
  return ret;
}

char* ws_read(ws_client_t* client,ws_header_t* header) {
  char* ret;
  char* append;
  err_t err;
  struct netbuf* inbuf;
  struct netbuf* inbuf2;
  char* buf;
  char* buf2;
  uint16_t len;
  uint16_t len2;
  uint64_t pos;
  uint64_t cont_len;
  uint64_t cont_pos;

  // if we read from this previously (not cont frames), stop reading
  if(client->unfinished) {
    client->unfinished--;
    return NULL;
  }

  err = netconn_recv(client->conn,&inbuf);
  if(err != ERR_OK) return NULL;
  netbuf_data(inbuf,(void**)&buf, &len);
  if(!buf) return NULL;

  // get the header
  header->param.pos.ZERO = buf[0];
  header->param.pos.ONE  = buf[1];

  // get the message length
  pos = 2;
  if(header->param.bit.LEN <= 125) {
    header->length = header->param.bit.LEN;
  }
  else if(header->param.bit.LEN == 126) {
    header->length = buf[2] << 8 | buf[3];
    pos = 4;
  }
  else { // LEN = 127
    header->length = (uint64_t)buf[2] << 56 | (uint64_t)buf[3] << 48
                   | (uint64_t)buf[4] << 40 | (uint64_t)buf[5] << 32
                   | (uint64_t)buf[6] << 24 | (uint64_t)buf[7] << 16
                   | (uint64_t)buf[8] << 8  | (uint64_t)buf[9];
    pos = 10;
  }

  if(header->param.bit.MASK) {
    memcpy(&(header->key.full),&buf[pos],4); // extract the key
    pos += 4;
  }

  ret = malloc(header->length+1); // allocate memory, plus a byte
  if(!ret) {
    netbuf_delete(inbuf);
    header->received = 0;
    return NULL;
  }

  cont_len = len-pos; // get the actual length
  memcpy(ret,&buf[pos],header->length); // allocate the total memory
  cont_pos = cont_len; // get the initial position
  // netconn gives messages in pieces, so we need to get those (different than OPCODE_CONT)
  while(cont_len < header->length) { // while the actual length is less than the header stated
    err = netconn_recv(client->conn,&inbuf2);
    if(err != ERR_OK) {
      netbuf_delete(inbuf2);
      free(ret);
      client->unfinished = 0;
      header->received = 0;
      return NULL;
    }
    netbuf_data(inbuf2,(void**)&buf2, &len2);
    // Prevent catastrophic failure due to memory leakage
    if(cont_len + len2 > header->length) {
      netbuf_delete(inbuf2);
      free(ret);
      client->unfinished = 0;
      header->received = 0;
      return NULL;
    }
    memcpy(&ret[cont_pos],buf2,len2);
    cont_pos += len2;
    if(!buf2) {
      client->unfinished = 0;
      header->received = 0;
    }
    netbuf_delete(inbuf2);
    client->unfinished++;
    cont_len += len2;
  }

  ret[header->length] = '\0'; // end string
  ws_encrypt_decrypt(ret,*header); // unencrypt, if necessary

  if(header->param.bit.FIN == 0) { // if the message isn't done
    if((header->param.bit.OPCODE == WEBSOCKET_OPCODE_CONT) &&
       ((client->last_opcode==WEBSOCKET_OPCODE_BIN) || (client->last_opcode==WEBSOCKET_OPCODE_TEXT))) {
         cont_len = header->length + client->len;
         append = malloc(cont_len);
         memcpy(append,client->contin,client->len);
         memcpy(&append[client->len],ret,header->length);
         free(client->contin);
         client->contin = malloc(cont_len);
         client->len = cont_len;

         free(append);
         free(ret);
         netbuf_delete(inbuf);
         //free(buf);
         return NULL;
    }
    else if((header->param.bit.OPCODE==WEBSOCKET_OPCODE_BIN) || (header->param.bit.OPCODE==WEBSOCKET_OPCODE_TEXT)) {
      if(client->len) {
        free(client->contin);
      }
      client->contin = malloc(header->length);
      memcpy(client->contin,ret,header->length);
      client->len = header->length;
      client->last_opcode = header->param.bit.OPCODE;

      free(ret);
      netbuf_delete(inbuf);
      //free(buf);
      return NULL;
    }
    else { // there shouldn't be another FIN code....
      free(ret);
      netbuf_delete(inbuf);
      //free(buf);
      return NULL;
    }
  }
  client->last_opcode = header->param.bit.OPCODE;
  if(inbuf) netbuf_delete(inbuf);
  header->received = 1;
  return ret;
}

char* ws_hash_handshake(char* handshake,uint8_t len) {
  const char hash[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  const uint8_t hash_len = sizeof(hash);
  char* ret;
  char key[64];
  unsigned char sha1sum[20];
  unsigned int ret_len;

  if(!len) return NULL;
  ret = malloc(32);

  memcpy(key,handshake,len);
  strlcpy(&key[len],hash,sizeof(key));
  mbedtls_sha1((unsigned char*)key,len+hash_len-1,sha1sum);
  mbedtls_base64_encode(NULL, 0, &ret_len, sha1sum, 20);
  if(!mbedtls_base64_encode((unsigned char*)ret,32,&ret_len,sha1sum,20)) {
    ret[ret_len] = '\0';
    return ret;
  }
  free(ret);
  return NULL;
}
