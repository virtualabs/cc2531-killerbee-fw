#include "kb-usb.h"

static USBBuffer data_rx_urb;
static USBBuffer data_tx_urb;
static USBBuffer data_tx_urb2;
static USBBuffer data_tx_urb3;
static uint8_t usb_rx_data[BUFFER_SIZE];
static uint8_t enabled = 0;

static uint8_t buf[BUFSIZE];
static int ptr;


static kb_usb_state g_state;
static kb_event_t kb_event;
volatile uint8_t g_pkt_len;

/* Custom event message. */
process_event_t kb_event_message;


void kb_usb_send_bytes(uint8_t *bytes, int length)
{
  if(!enabled) {
    return;
  }

  data_tx_urb.flags = USB_BUFFER_IN;
  data_tx_urb.flags |= USB_BUFFER_NOTIFY;
  data_tx_urb.next = NULL;
  data_tx_urb.data = bytes;
  data_tx_urb.left = (length>64)?64:length;
  
  if (length > 64)
  {
    data_tx_urb2.flags = USB_BUFFER_IN;
    data_tx_urb2.flags |= USB_BUFFER_NOTIFY;
    data_tx_urb2.next = NULL;
    data_tx_urb2.data = &bytes[64];
    data_tx_urb2.left = (length>128)?64:length-64;
    data_tx_urb.next = &data_tx_urb2;

    if (length > 128)
    {
      data_tx_urb3.flags = USB_BUFFER_IN;
      data_tx_urb3.flags |= USB_BUFFER_NOTIFY;
      data_tx_urb3.next = NULL;
      data_tx_urb3.data = &bytes[128];
      data_tx_urb3.left = length-128;
      data_tx_urb2.next = &data_tx_urb3;
    }
  }
  usb_submit_xmit_buffer(EPIN, &data_tx_urb);
}


void kb_usb_send(kb_event_t *p_event)
{
  uint32_t i;
  uint32_t len;
  uint8_t checksum;
  static uint8_t buf[132];

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
  kb_usb_send_bytes(buf, len);
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
    0,
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
      /*USB_EP2_SIZE*/64,
      0
    },
    {
      sizeof(configuration_block.ep_out),
      ENDPOINT,
      0x03,
      0x02,
      /*USB_EP3_SIZE*/64,
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
  data_rx_urb.left = BUFFER_SIZE;
  data_rx_urb.next = NULL;
  usb_submit_recv_buffer(EPOUT, &data_rx_urb);
}

static void
do_work(void)
{
  unsigned int events;

  events = usb_get_global_events();
  if(events & USB_EVENT_CONFIG) {

    /* Force state once the device is configured. */
    g_state = KBS_IDLE;
    ptr = 0;

    /* Enable endpoints */
    enabled = 1;
    usb_setup_bulk_endpoint(EPIN);
    usb_setup_bulk_endpoint(EPOUT);

    queue_rx_urb();
  }
  if(events & USB_EVENT_RESET) {
    enabled = 0;
  }

  if(!enabled) {
    return;
  }

  events = usb_get_ep_events(EPOUT);
  if((events & USB_EP_EVENT_NOTIFICATION)
     && !(data_rx_urb.flags & USB_BUFFER_SUBMITTED)) {
    if(!(data_rx_urb.flags & USB_BUFFER_FAILED)) {
      uint8_t len;
      uint8_t i;

      len = BUFFER_SIZE - data_rx_urb.left;
      for(i = 0; i < len; i++) {
        input_handler(usb_rx_data[i]);
      }
    }
    queue_rx_urb();
  }
}

PROCESS(kb_usb_process, "Killerbee USB driver");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(kb_usb_process, ev, data)
{

  PROCESS_BEGIN();

  kb_event_message = process_alloc_event();
  ptr = 0;

  //usb_setup();
  usb_set_global_event_process(&kb_usb_process);
  usb_set_ep_event_process(EPIN, &kb_usb_process);
  usb_set_ep_event_process(EPOUT, &kb_usb_process);

  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_EXIT) {
      break;
    }
    if(ev == PROCESS_EVENT_POLL) {
      do_work();
    }
  }

  PROCESS_END();
}


void kb_usb_init(void)
{
  /* Initialize state. */
  g_state = KBS_IDLE;
  ptr = 0;

  leds_off(LEDS_RED | LEDS_GREEN);
  process_start(&kb_usb_process, NULL);
}
