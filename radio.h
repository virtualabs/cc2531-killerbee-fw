#ifndef __INC_RADIO_H
#define __INC_RADIO_H

#include "contiki.h"
#include "net/netstack.h"

#define RADIO_DEFAULT_CHANNEL   11

typedef enum {
  SNIFFER_OFF,
  SNIFFER_ON
} sniffer_status;

typedef struct {

  /* RF channel. */
  int channel;

  /* Sniffer status. */
  sniffer_status sniffer_enabled;

} radio_state_t;


void radio_init(void);
int radio_set_channel(int channel);
void radio_enable_sniffer(void);
void radio_disable_sniffer(void);
int radio_send_packet(void *p_packet_buffer, int packet_length);
int radio_got_packet(void);
int radio_read_packet(void *p_packet_buffer, int max_packet_length);
int radio_is_sniffer_enabled(void);



#endif /* __INC_RADIO_H */