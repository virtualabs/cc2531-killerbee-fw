#include "radio.h"

volatile radio_state_t g_radio_state;

static int gb_radio_started = 0;

void radio_init(void)
{
  /* Configure radio state. */
  g_radio_state.channel = RADIO_DEFAULT_CHANNEL;

  if (!gb_radio_started)
  {
    /* Initialize CC2531 Radio peripheral. */
    NETSTACK_RADIO.init();
    radio_set_channel(g_radio_state.channel);

    gb_radio_started = 1;
  }

  /* Disable radio. */
  radio_disable_sniffer();
}

int radio_set_channel(int channel)
{
  /* Save channel into radio state. */
  g_radio_state.channel = channel;

  /* Configure channel and restart Radio if necessary. */
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channel);

  return channel;
}


void radio_enable_sniffer(void)
{
  /* Enable sniffing. */
  NETSTACK_RADIO.off();
  NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, 0);
  NETSTACK_RADIO.on();

  /* Save sniffer state. */
  g_radio_state.sniffer_enabled = SNIFFER_ON;
  
}


void radio_disable_sniffer(void)
{
  /* Save sniffer state. */
  g_radio_state.sniffer_enabled = SNIFFER_OFF;
}

int radio_send_packet(void *p_packet_buffer, int packet_length)
{
  /* Send packet through CC2531 RF driver. */
  return NETSTACK_RADIO.send(p_packet_buffer, packet_length);
}


int radio_got_packet(void)
{
  return NETSTACK_RADIO.pending_packet();
}


int radio_read_packet(void *p_packet_buffer, int max_packet_length)
{
  return NETSTACK_RADIO.read(p_packet_buffer, max_packet_length);
}

int radio_is_sniffer_enabled(void)
{
  return g_radio_state.sniffer_enabled;
}