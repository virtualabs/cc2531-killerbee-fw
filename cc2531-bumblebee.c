/*
 * Copyright (c) 2012, George Oikonomou (oikonomou@users.sourceforge.net)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */
/**
 * \file
 *         A simple demo project which demonstrates the cc2531 USB dongle
 *         USB (CDC_ACM) functionality.
 *
 *         It will print out periodically. Anything you type in the dongle's
 *         serial console will be echoed back after a newline.
 *
 * \author
 *         George Oikonomou - <oikonomou@users.sourceforge.net>
 */

#include <stdlib.h>
#include "contiki.h"
#include "net/packetbuf.h"
#include "dev/leds.h"
#include "debug.h"
#include "radio.h"
#include "kb-usb.h"
#include "proto.h"

/* Default ZigBee channel 0 on 2.4GHz page. */
#define CC2530_RF_CHANNEL 11

/* Packet buffer size: 256 - 3 bytes (length, command, crc). */
#define PACKET_BUFFER_SIZE  (256-3)

/* RF packet buffer. */
static unsigned char packet_buf[PACKET_BUFFER_SIZE];

/* Contiki local events. */
static process_event_t event_packet_received;
extern process_event_t kb_event_message;

/**
 * dispatch_command()
 * 
 * @brief: Dispatch a command buffer received through USB.
 * @param p_event: pointer to a `kb_event_t` structure filled by USB driver.
 **/

void dispatch_command(kb_event_t *p_event)
{
  uint8_t channel;

  /* Switch on command. */
  switch(p_event->command)
  {
    /* Initialize. */
    case CMD_INIT:
      {

        /* Init radio. */
        radio_init();
        radio_disable_sniffer();

        /* Send ACK. */
        proto_send_ack(CMD_INIT);
      }
      break;

    /* Change RF channel. */
    case CMD_SET_CHANNEL:
      {
        /* Check parameter size (must be 1 byte) */
        if (p_event->payload_size == 1)
        {
          /* Change channel. */
          channel = p_event->payload[0];
          radio_set_channel(channel);

          /* Send ACK. */
          proto_send_ack(CMD_SET_CHANNEL);
        }
      }
      break;

    /* Send RF packet. */
    case CMD_SEND_PKT:
      {
        /* Packet is in payload, check size first. */
        if (p_event->payload_size > 0)
        {
          /* Show activity. */
          leds_on(LEDS_GREEN);

          /* Send packet. */
          radio_send_packet(p_event->payload, p_event->payload_size);

          /* Send ACK. */
          proto_send_ack(CMD_SEND_PKT);

          /* Shut off LED. */
          leds_off(LEDS_GREEN);
        }
      }
      break;

    /* Start sniffer. */
    case CMD_SNIFF_ON:
      {
        /* Enable sniffer. */
        radio_enable_sniffer();

        /* Send ACK. */
        proto_send_ack(CMD_SNIFF_ON_ACK);
      }
      break;


    /* Stop sniffer. */
    case CMD_SNIFF_OFF:
      {
        /* Enable sniffer. */
        radio_disable_sniffer();

        /* Send ACK. */
        proto_send_ack(CMD_SNIFF_OFF_ACK);
      }
      break;

    default:
      break;

  }
}

/* Defines processes. */
PROCESS(cc2531_rf_sniffer, "Sniffer");
PROCESS(cc2531_bumlblebee_process, "cc2531 USB Demo process");

/**
 * cc2531_rf_sniffer()
 * 
 * @brief: RF sniffer thread, sniffs packet and notify cc2531_bumblebee_process when received.
 * @param ev: event type
 * @param data: event data
 **/
PROCESS_THREAD(cc2531_rf_sniffer, ev, data)
{
  int pkt_size = 0;
  packet_t *p_pkt;

  PROCESS_BEGIN();

  /* Init netstack */
  radio_init();
  radio_set_channel(11);

  while(1)
  {
    if (radio_got_packet())
    {
      pkt_size = radio_read_packet(&packet_buf[2], PACKET_BUFFER_SIZE);
      if (pkt_size > 0)
      {
        /* Insert RSSI and LQI. */
        packet_buf[0] = packetbuf_attr(PACKETBUF_ATTR_RSSI);
        packet_buf[1] = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);

        /* Report packet. */
        p_pkt = (packet_t *)malloc(sizeof(packet_t));
        if (p_pkt != NULL)
        {
          memset(p_pkt, 0, sizeof(packet_t));
          memcpy(p_pkt->payload, packet_buf, pkt_size+2);
          p_pkt->size = (uint8_t)pkt_size + 2;
          process_post(&cc2531_bumlblebee_process, event_packet_received, p_pkt);
        }
      }
    }
    PROCESS_PAUSE();
  }

  PROCESS_END();

}


/**
 * cc2531_bumblebee_process()
 * 
 * @brief: Main process thread, handling USB and everything.
 * @param ev: event type
 * @param data: event data
 **/

PROCESS_THREAD(cc2531_bumlblebee_process, ev, data)
{
  kb_event_t *p_event;
  packet_t *p_pkt;
  PROCESS_BEGIN();

  kb_usb_init();

  while(1) {
    PROCESS_WAIT_EVENT();
    if (ev == event_packet_received) {
      /* Show activity. */
      leds_on(LEDS_GREEN);

      p_pkt = (packet_t *)data;
      proto_send_packet(p_pkt->payload, p_pkt->size);
      free(p_pkt);

      /* Shut off LED. */
      leds_off(LEDS_GREEN);
    } else if (ev == kb_event_message) {
      p_event = (kb_event_t *)data;
      dispatch_command(p_event);
    }
  }

  PROCESS_END();
}

/* Start both processes (main routine). */
AUTOSTART_PROCESSES(&cc2531_bumlblebee_process, &cc2531_rf_sniffer);