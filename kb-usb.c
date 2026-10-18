#include "kb-usb.h"
#include "proto.h"
#include <stdlib.h>

static USBBuffer data_rx_urb;
static USBBuffer data_tx_urb;
static uint8_t usb_rx_data[USB_EP3_SIZE];
static uint8_t enabled = 0;

/* EP3 OUT rx buffer. */
static uint8_t rx_buffer[RX_BUFSIZE];
volatile int ptr;

/* EP2 IN tx buffer: one 64-byte packet [chunk_len][chunk][padding]. */
static uint8_t tx_buffer[USB_EP2_SIZE];

/*
 * EP2 IN message FIFO (ring buffer). Only touched from process context; the
 * USB interrupt only ever sees data_tx_urb/tx_buffer.
 */
static uint8_t PKT_FIFO[PKT_FIFO_MAX];
static uint16_t fifo_head = 0;      /* next byte to send */
static uint16_t fifo_count = 0;     /* bytes queued */
static uint8_t fifo_msg_left = 0;   /* bytes left of the message being sent */

volatile kb_usb_state g_state;
volatile kb_event_t *kb_event;
volatile int g_pkt_len;

/* Custom event message. */
process_event_t kb_event_message;

/* Custom event message. */
process_event_t kb_sendpkt_message;


/*
 * Build the next IN packet from the FIFO and submit it, but only when the
 * previous one has left data_tx_urb (USB_BUFFER_SUBMITTED cleared by the
 * USB interrupt). A submitted URB is never modified.
 */
static void kb_usb_tx_kick(void)
{
  uint8_t len, i, c;

  if(!enabled || (data_tx_urb.flags & USB_BUFFER_SUBMITTED) || (fifo_count == 0)) {
    return;
  }

  len = (fifo_count > (USB_EP2_SIZE-1)) ? (USB_EP2_SIZE-1) : (uint8_t)fifo_count;
  for (i=0; i<len; i++)
  {
    c = PKT_FIFO[fifo_head];
    fifo_head = (fifo_head + 1) & (PKT_FIFO_MAX - 1);
    if (fifo_msg_left == 0)
      fifo_msg_left = c;  /* length byte of the next message */
    fifo_msg_left--;
    tx_buffer[1 + i] = c;
  }
  fifo_count -= len;
  tx_buffer[0] = len;
  memset(&tx_buffer[1 + len], 0, (USB_EP2_SIZE-1) - len);

  data_tx_urb.flags = USB_BUFFER_IN | USB_BUFFER_NOTIFY;
  data_tx_urb.data = tx_buffer;
  data_tx_urb.left = USB_EP2_SIZE;
  data_tx_urb.next = NULL;
  usb_submit_xmit_buffer(EPIN, &data_tx_urb);
}

void kb_usb_send_bytes(uint8_t *bytes, int length)
{
  uint16_t tail, limit;
  int i;

  /* Don't send if not enabled. */
  if(!enabled) {
    return;
  }

  /*
   * Append the whole message or drop it. Captured frames leave headroom so
   * that command ACKs still fit when the host drains slowly.
   */
  limit = PKT_FIFO_MAX;
  if ((length > 1) && (bytes[1] == CMD_GOT_PKT))
    limit -= PKT_FIFO_RESERVE;
  if ((length <= 0) || ((fifo_count + length) > limit))
    return;

  tail = (fifo_head + fifo_count) & (PKT_FIFO_MAX - 1);
  for (i=0; i<length; i++)
  {
    PKT_FIFO[tail] = bytes[i];
    tail = (tail + 1) & (PKT_FIFO_MAX - 1);
  }
  fifo_count += length;

  kb_usb_tx_kick();
}

/* Callback to the input handler */
static void input_handler(unsigned char c)
{
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
        rx_buffer[ptr++] = (uint8_t)c;
        g_state = KBS_WAIT_PAYLOAD;
      }
      break;

    case KBS_WAIT_PAYLOAD:
      {
        if((ptr <= g_pkt_len) && (ptr < RX_BUFSIZE))
        {
          rx_buffer[ptr++] = (uint8_t)c;
        }

        if (ptr == g_pkt_len)
        {
          //rx_buffer[ptr++] = (uint8_t)c;
          g_state = KBS_PACKET_RECEIVED;

          /* Check CRC. */
          if (packet_is_valid(rx_buffer, g_pkt_len))
          {
            /* Allocate a KB event. */
            kb_event = (kb_event_t *)malloc(sizeof(kb_event_t));
            if (kb_event != NULL)
            {
              /* Command at offset 1. */
              kb_event->command = rx_buffer[1];
              kb_event->payload_size = g_pkt_len-3;
              kb_event->payload = NULL;

              if (kb_event->payload_size > 0)
              {
                /* Allocate a new payload buffer. */
                kb_event->payload = (uint8_t *)malloc(sizeof(uint8_t) * kb_event->payload_size);
                if (kb_event->payload != NULL)
                {
                  /* Copy payload. */
                  memcpy(kb_event->payload, &rx_buffer[2], kb_event->payload_size);
                }
              }

              /* Broadcast event, or drop the command if we are out of memory. */
              if (((kb_event->payload_size > 0) && (kb_event->payload == NULL)) ||
                  (process_post(PROCESS_BROADCAST, kb_event_message, (void *)kb_event) != PROCESS_ERR_OK))
              {
                if (kb_event->payload != NULL)
                  free(kb_event->payload);
                free((void *)kb_event);
              }
            }
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

const struct usb_st_device_descriptor device_descriptor =
  {
    sizeof(struct usb_st_device_descriptor),
    DEVICE,
    0x0200,
    0, /* Defined at interface level. */
    0,
    0,
    CTRL_EP_SIZE,
    CDC_ACM_CONF_VID,
    CDC_ACM_CONF_PID,
    0x0000,
    1,
    2,
    3, /* iSerialNumber: chip IEEE address (usb-serial.c) */
    1
  };

  const struct configuration_st {
  struct usb_st_configuration_descriptor configuration;
  struct usb_st_interface_descriptor data;
  struct usb_st_endpoint_descriptor ep_in;
  struct usb_st_endpoint_descriptor ep_out;
} BYTE_ALIGNED configuration_block =
  {
    /* Configuration */
    {
      sizeof(configuration_block.configuration),
      CONFIGURATION,
      sizeof(configuration_block),
      1, /* bNumInterfaces = 1 */
      1, /* bConfigurationValue */
      0, /* iConfiguration */
      0x80, /* Bus powered */
      250 /* 250 mA */
    },
    {
      sizeof(configuration_block.data),
      INTERFACE,
      0, /* Interface number = 0 */
      0, /* bAlternate setting = 0 */
      2, /* bNumEndpoints = 2 */
      0xff, /* Interface class = 0xFF (vendor class specific) */
      0, /* bInterface subclass */
      0, /* bInterface protocol */
      0 /* iInterface */
    },
    {
      sizeof(configuration_block.ep_in),
      ENDPOINT,
      0x82,
      0x02,
      USB_EP2_SIZE,
      0
    },
    {
      sizeof(configuration_block.ep_out),
      ENDPOINT,
      0x03,
      0x02,
      USB_EP3_SIZE,
      0
    }
  };

const struct usb_st_configuration_descriptor const *configuration_head =
(struct usb_st_configuration_descriptor const*)&configuration_block;



static void
queue_rx_urb(void)
{
  data_rx_urb.flags = USB_BUFFER_PACKET_END;    /* Make sure we are getting immediately the packet. */
  data_rx_urb.flags |= USB_BUFFER_NOTIFY;
  data_rx_urb.data = usb_rx_data;
  data_rx_urb.left = USB_EP3_SIZE;
  data_rx_urb.next = NULL;
  usb_submit_recv_buffer(EPOUT, &data_rx_urb);
}

static void
do_work(void)
{
  unsigned int events;

  events = usb_get_global_events();
  if(events & USB_EVENT_RESET) {
    enabled = 0;
  }
  if(events & USB_EVENT_CONFIG) {

    /* Force state once the device is configured. */
    g_state = KBS_IDLE;
    ptr = 0;
    fifo_count = 0;
    fifo_msg_left = 0;

    /* Enable endpoints */
    usb_setup_bulk_endpoint(EPIN);
    usb_setup_bulk_endpoint(EPOUT);

    /* Endpoint setup dropped any queued buffer: data_tx_urb is ours again. */
    data_tx_urb.flags = 0;
    enabled = 1;

    queue_rx_urb();
  }

  if(!enabled) {
    return;
  }

  events = usb_get_ep_events(EPOUT);
  if((events & USB_EP_EVENT_NOTIFICATION)
     && !(data_rx_urb.flags & USB_BUFFER_SUBMITTED)) {
    if(!(data_rx_urb.flags & USB_BUFFER_FAILED)) {
      int len;
      int i;

      len = BUFFER_SIZE - data_rx_urb.left;
      for(i = 0; i < len; i++) {
        input_handler(usb_rx_data[i]);
      }
    }
    queue_rx_urb();
  }

  /* EP2 IN packet collected (or EP idle): send the next chunk. */
  usb_get_ep_events(EPIN);
  kb_usb_tx_kick();
}

PROCESS(kb_usb_process, "Killerbee USB driver");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(kb_usb_process, ev, data)
{
  kb_event_t *p_sendpkt;
  PROCESS_BEGIN();

  kb_event_message = process_alloc_event();
  kb_sendpkt_message = process_alloc_event();
  ptr = 0;

  usb_set_global_event_process(&kb_usb_process);
  usb_set_ep_event_process(EPIN, &kb_usb_process);
  usb_set_ep_event_process(EPOUT, &kb_usb_process);

  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_EXIT) {
      break;
    }

    /* Do we have a packet to send ? */
    if (ev == kb_sendpkt_message) {
      leds_on(LEDS_RED);
      
      /* Got packet to send, send packet through USB endpoint 2 */
      p_sendpkt = (kb_event_t *)data;
      kb_usb_send_bytes(p_sendpkt->payload, p_sendpkt->payload_size);

      /* Free packet resources. */
      free(p_sendpkt->payload);
      free(p_sendpkt);
      leds_off(LEDS_RED);
    }

    if(ev == PROCESS_EVENT_POLL) {
      do_work();
    }
  }

  PROCESS_END();
}

void kb_fifo_reset(void)
{
  /* Drop queued messages, but finish the one already partly sent. */
  fifo_count = (fifo_count < fifo_msg_left) ? fifo_count : fifo_msg_left;
}

void kb_usb_reset(void)
{
  /* Initialize state. */
  g_state = KBS_IDLE;
  ptr = 0;

  /* Reset data FIFO. */
  kb_fifo_reset();
}

void kb_usb_init(void)
{
  /* Reset USB processing globals and FIFO. */
  kb_usb_reset();

  /* Switch off LEDs. */
  leds_off(LEDS_RED | LEDS_GREEN);

  /* Start USB task. */
  process_start(&kb_usb_process, NULL);
}
