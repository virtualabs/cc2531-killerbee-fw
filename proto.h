#ifndef __INC_SERIAL_PROTO_H
#define __INC_SERIAL_PROTO_H

#include "kb-usb.h"

/**************************************************
 * Serial protocol overview
 *
 * Command structure:
 * 
 * [ CMD (1 byte) ][ PARAMS ...]
 *************************************************/


typedef enum {
  CMD_INIT,
  CMD_INIT_ACK,
  CMD_SET_CHANNEL,
  CMD_SET_CHANNEL_ACK,
  CMD_SEND_PKT,
  CMD_SEND_PKT_ACK,
  CMD_SNIFF_ON,
  CMD_SNIFF_ON_ACK,
  CMD_SNIFF_OFF,
  CMD_SNIFF_OFF_ACK,
  CMD_GOT_PKT
} command_t;

typedef struct {
  uint32_t size;
  uint8_t payload[256];
} packet_t;

void proto_send_ack(command_t command);
void proto_send_packet(uint8_t *p_packet, int packet_len);

#endif /* __INC_SERIAL_PROTO_H */