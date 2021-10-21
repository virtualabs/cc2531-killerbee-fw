#include "proto.h"
#include <stdlib.h>

extern process_event_t kb_sendpkt_message;

void proto_send_ack(command_t command)
{
  kb_event_t *p_kb_evt;
  uint8_t checksum;

  /* Allocate memory for kb_event_t structure. */
  p_kb_evt = (kb_event_t *)malloc(sizeof(kb_event_t));
  if (p_kb_evt != NULL)
  {
    /* Allocate memory for our payload. */
    p_kb_evt->payload = (uint8_t *)malloc(3);
    if (p_kb_evt->payload != NULL)
    {
      /* Build our event. */
      p_kb_evt->payload[0] = 3;
      p_kb_evt->payload[1] = command;
      checksum = packet_compute_checksum(p_kb_evt->payload, 2);
      p_kb_evt->payload[2] = checksum;
      p_kb_evt->payload_size = 3;

      /* Send packet to USB process. */
      process_post(PROCESS_BROADCAST, kb_sendpkt_message, (void *)p_kb_evt);
    }
  }
}

void proto_send_packet(packet_t *p_pkt)
{
  kb_event_t *p_kb_evt;
  int i;
  uint8_t checksum;

  /* Allocate memory for kb_event_t structure. */
  p_kb_evt = (kb_event_t *)malloc(sizeof(kb_event_t));
  if (p_kb_evt != NULL)
  {
    /* Allocate memory for our payload. */
    p_kb_evt->payload = (uint8_t *)malloc(p_pkt->size + 3);
    if (p_kb_evt->payload != NULL)
    {
      /* Build our event. */
      p_kb_evt->payload[0] = (uint8_t)(p_pkt->size + 3);
      p_kb_evt->payload[1] = CMD_GOT_PKT;
      for (i=0; i<p_pkt->size; i++)
      {
        p_kb_evt->payload[2+i] = p_pkt->payload[i];
      }
      checksum = packet_compute_checksum(p_kb_evt->payload, p_pkt->size+2);
      p_kb_evt->payload[2+i] = checksum;
      p_kb_evt->payload_size = p_pkt->size + 3;

      /* Send packet to USB process. */
      process_post(PROCESS_BROADCAST, kb_sendpkt_message, (void *)p_kb_evt);

      /* Free packet. */
      free(p_pkt);
    }
  }
}