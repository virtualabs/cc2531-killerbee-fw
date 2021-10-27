#ifndef __INC_SERIAL_PROTO_H
#define __INC_SERIAL_PROTO_H

#include "kb-usb.h"
#include "utils.h"

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
  CMD_GOT_PKT,
  CMD_ERR = 0xFF
} command_t;

typedef struct {
  uint32_t size;
  uint8_t payload[256];
} packet_t;

//void proto_send_ack(command_t command);
//void proto_send_packet(uint8_t *p_packet, int packet_len);
void proto_send_packet(packet_t *p_pkt);
void proto_send(command_t command, uint8_t *payload, int len);

#define proto_send_ack(x) proto_send(x, NULL, 0)

#endif /* __INC_SERIAL_PROTO_H */