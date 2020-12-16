#include "proto.h"

void proto_send_ack(command_t command)
{
  kb_event_t event;

  /* Set ACK for command. */
  event.command = command + 1;
  event.payload = NULL;
  event.payload_size = 0;
  kb_usb_send(&event);
}

void proto_send_packet(uint8_t *p_packet, int packet_len)
{
  kb_event_t event;

  event.command = CMD_GOT_PKT;
  event.payload = p_packet;
  event.payload_size = packet_len;
  kb_usb_send(&event);
}