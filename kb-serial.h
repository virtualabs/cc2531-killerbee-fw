#ifndef __INC_PROTO_H
#define __INC_PROTO_H

#include "contiki.h"

#define BUFSIZE 256


typedef enum {
  KBS_IDLE,
  KBS_WAIT_PAYLOAD,
  KBS_PACKET_RECEIVED
} kb_serial_state;

typedef struct {
  unsigned char command;
  uint32_t payload_size;
  uint8_t *payload;
} kb_event_t;

PROCESS_NAME(kb_serial_process);

void kb_serial_init(void);
void kb_serial_send(kb_event_t *p_event);

#endif /* __INC_PROTO_H */