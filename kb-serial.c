#include "kb-serial.h"

#include <string.h>
#include "debug.h"
#include "lib/ringbuf.h"
#include "dev/io-arch.h"

static struct ringbuf rxbuf;
static uint8_t rxbuf_data[BUFSIZE];
static kb_serial_state g_state;
static kb_event_t kb_event;
volatile uint8_t g_pkt_len;

static uint8_t buf[256];

/* Custom event message. */
process_event_t kb_event_message;

/**
 * @brief: compute serial packet checksum
 * @param p_packet: pointer to some packet bytes
 * @param packet_len: number of bytes in packet
 **/

uint8_t packet_compute_checksum(uint8_t *p_packet, int packet_len)
{
  int i;
  uint8_t checksum = 0xFF;
  
  for (i=0; i<packet_len; i++)
    checksum ^= p_packet[i];
  return checksum;
}


/**
 * @brief: check if packet checksum is valid
 * @param p_packet: packet bytes
 * @param packet_len: number of bytes in packet
 * @return 1 if packet is valid, 0 otherwise
 **/

int packet_is_valid(uint8_t *p_packet, int packet_len)
{
  return (packet_compute_checksum(p_packet, packet_len-1) == p_packet[packet_len-1]);
}


PROCESS(kb_serial_process, "Killerbee driver");


/**
 * @brief: UART input callback
 * @param c: incoming character to process
 * @return always 1
 **/

int kb_serial_input_byte(unsigned char c)
{
  static uint8_t overflow = 0; /* Buffer overflow: ignore until END */
  
  if(!overflow) {
    /* Add character */
    if(ringbuf_put(&rxbuf, c) == 0) {
      /* Buffer overflow: ignore the rest of the line */
      overflow = 1;
    }
  } 

  /* Wake up consumer process */
  process_poll(&kb_serial_process);
  return 1;
}


/**
 * @brief: Killerbee serial handling process
 * @param ev: event
 * @param data: event data
 **/

PROCESS_THREAD(kb_serial_process, ev, data)
{
  static uint8_t buf[BUFSIZE];
  static int ptr;

  PROCESS_BEGIN();

  kb_event_message = process_alloc_event();
  ptr = 0;

  while(1) {
    /* Fill application buffer until newline or empty */
    int c = ringbuf_get(&rxbuf);
    
    if(c == -1) {
      /* Buffer empty, wait for poll */
      PROCESS_YIELD();
    } else {

      switch (g_state)
      {
        case KBS_IDLE:
          {
            /* Packet length must be >= 3. */
            if (c < 3)
              break;

            /* Parse length byte. */
            g_pkt_len = c;
            ptr = 0;
            buf[ptr++] = (uint8_t)c;
            g_state = KBS_WAIT_PAYLOAD;
          }
          break;

        case KBS_WAIT_PAYLOAD:
          {
            if(ptr < (g_pkt_len-1))
            {
              buf[ptr++] = (uint8_t)c;
            }
            else
            {
              buf[ptr++] = (uint8_t)c;
              g_state = KBS_PACKET_RECEIVED;

              /* Check CRC. */
              if (packet_is_valid(buf, g_pkt_len))
              {
                /* Command at offset 1. */
                kb_event.command = buf[1];
                kb_event.payload_size = ptr-3;
                kb_event.payload = &buf[2];

                /* Broadcast event */
                process_post(PROCESS_BROADCAST, kb_event_message, (void *)&kb_event);
              }

              /* Wait for another packet. */
              g_state = KBS_IDLE;
            }
          }
          break;

        default:
          g_state = KBS_IDLE;
          break;

      }
    }
  }

  PROCESS_END();
}


/**
 * @brief: Initialize Killerbee serial protocol
 **/

void kb_serial_init(void)
{
  ringbuf_init(&rxbuf, rxbuf_data, sizeof(rxbuf_data));
  io_arch_set_input(kb_serial_input_byte);

  g_state = KBS_IDLE;
  process_start(&kb_serial_process, NULL);
}

void putdw(uint32_t v)
{
  puthex((uint8_t)(v>>24));
  puthex((uint8_t)((v&0x00FF0000)>>16));
  puthex((uint8_t)((v&0x0000FF00)>>8));
  puthex((uint8_t)((v&0x000000FF)));
}

void kb_serial_send(kb_event_t *p_event)
{
  uint32_t i;
  uint32_t len;
  uint8_t checksum;

  /* Prepare buffer. */
  len = p_event->payload_size + 3;
  buf[0] = len;
  buf[1] = p_event->command;
  
  for (i=0; i<(p_event->payload_size); i++)
  {
    buf[2+i] = p_event->payload[i];
  }
  checksum = packet_compute_checksum(buf, len-1);
  buf[2+i] = checksum;

  /* Send buffer. */
  for (i=0; i<len; i++)
    usb_serial_writeb(buf[i]);
  usb_serial_flush();
}
