#ifndef _UTIL_CRC16_H_
#define _UTIL_CRC16_H_

#include <stdint.h>

static inline uint16_t _crc16_update(uint16_t crc, uint8_t data) __attribute__((always_inline, unused));
static inline uint16_t _crc16_update(uint16_t crc, uint8_t data)
{
	unsigned int i;

	crc ^= data;
	for (i = 0; i < 8; ++i) {
		if (crc & 1) {
			crc = (crc >> 1) ^ 0xA001;
		} else {
			crc = (crc >> 1);
		}
	}
	return crc;
}

static inline uint16_t _crc_xmodem_update(uint16_t crc, uint8_t data) __attribute__((always_inline, unused));
static inline uint16_t _crc_xmodem_update(uint16_t crc, uint8_t data)
{
	unsigned int i;

	crc = crc ^ ((uint16_t)data << 8);
	for (i=0; i<8; i++) {
		if (crc & 0x8000) {
			crc = (crc << 1) ^ 0x1021;
		} else {
			crc <<= 1;
		}
	}
	return crc;
}

static inline uint16_t _crc_ccitt_update (uint16_t crc, uint8_t data) __attribute__((always_inline, unused));
static inline uint16_t _crc_ccitt_update (uint16_t crc, uint8_t data)
{
	data ^= (crc & 255);
	data ^= data << 4;

	return ((((uint16_t)data << 8) | (crc >> 8)) ^ (uint8_t)(data >> 4) 
		^ ((uint16_t)data << 3));
}

static inline uint8_t _crc_ibutton_update(uint8_t crc, uint8_t data) __attribute__((always_inline, unused));
static inline uint8_t _crc_ibutton_update(uint8_t crc, uint8_t data)
{
	unsigned int i;

	crc = crc ^ data;
	for (i = 0; i < 8; i++) {
		if (crc & 0x01) {
			crc = (crc >> 1) ^ 0x8C;
		} else {
			crc >>= 1;
		}
	}
	return crc;
}

#endif

