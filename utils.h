#ifndef __INC_UTILS_H
#define __INC_UTILS_H

#include "contiki.h"
#include <string.h>

uint8_t packet_compute_checksum(uint8_t *p_packet, int packet_len);
int packet_is_valid(uint8_t *p_packet, int packet_len);

#endif /* __INC_UTILS_H */