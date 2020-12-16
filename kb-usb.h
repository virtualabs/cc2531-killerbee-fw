#ifndef __INC_USB_PROTO_H
#define __INC_USB_PROTO_H

#include "contiki.h"

/* 
 * Contiki hack: load models.h before loading usb-arch.h, in this way
 *               all USB structures will take the right values for EP2
 *               and EP3 buffers.
 */

#include "models.h"
#include "usb-arch.h"
#include "dev/leds.h"
#include "dev/leds-arch.h"
#include "descriptors.h"
#include "utils.h"


#define EPIN  0x82
#define EPOUT 0x03

#define BUFSIZE 256
#define BUFFER_SIZE 64

typedef enum {
  KBS_IDLE,
  KBS_WAIT_PAYLOAD,
  KBS_PACKET_RECEIVED
} kb_usb_state;

typedef struct {
  unsigned char command;
  uint32_t payload_size;
  uint8_t *payload;
} kb_event_t;

PROCESS_NAME(kb_serial_process);

void kb_usb_init(void);
void kb_usb_send(kb_event_t *p_event);

#endif /* __INC_USB_PROTO_H */