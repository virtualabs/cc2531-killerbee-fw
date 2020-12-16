#include "utils.h"

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