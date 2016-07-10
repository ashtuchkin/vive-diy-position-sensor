/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2016 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "usb_dev.h"
#include "usb_audio.h"
#include "HardwareSerial.h"
#include <string.h> // for memcpy()

#ifdef AUDIO_INTERFACE // defined by usb_dev.h -> usb_desc.h
#if F_CPU >= 20000000

bool AudioInputUSB::update_responsibility;
audio_block_t * AudioInputUSB::incoming_left;
audio_block_t * AudioInputUSB::incoming_right;
audio_block_t * AudioInputUSB::ready_left;
audio_block_t * AudioInputUSB::ready_right;
uint16_t AudioInputUSB::incoming_count;
uint8_t AudioInputUSB::receive_flag;

#define DMABUFATTR __attribute__ ((section(".dmabuffers"), aligned (4)))
uint16_t usb_audio_receive_buffer[AUDIO_RX_SIZE/2] DMABUFATTR;
uint32_t usb_audio_sync_feedback DMABUFATTR;
uint8_t usb_audio_receive_setting=0;

static uint32_t feedback_accumulator = 185042824;

void AudioInputUSB::begin(void)
{
	incoming_count = 0;
	incoming_left = NULL;
	incoming_right = NULL;
	ready_left = NULL;
	ready_right = NULL;
	receive_flag = 0;
	// update_responsibility = update_setup();
	// TODO: update responsibility is tough, partly because the USB
	// interrupts aren't sychronous to the audio library block size,
	// but also because the PC may stop transmitting data, which
	// means we no longer get receive callbacks from usb_dev.
	update_responsibility = false;
	//usb_audio_sync_feedback = 722824;
	//usb_audio_sync_feedback = 723700; // too fast?
	usb_audio_sync_feedback = 722534; // too slow
}

static void copy_to_buffers(const uint32_t *src, int16_t *left, int16_t *right, unsigned int len)
{
	uint32_t *target = (uint32_t*) src + len; 
	while ((src < target) && (((uintptr_t) left & 0x02) != 0)) {
		uint32_t n = *src++;
		*left++ = n & 0xFFFF;
		*right++ = n >> 16;
	}

	while ((src < target - 2)) {
		uint32_t n = *src++;
		uint32_t n1 = *src++;
		*(uint32_t *)left = n1 & 0xFFFF | ((n & 0xFFFF) << 16);
		left+=2;
		*(uint32_t *)right = (n1 >> 16) | ((n & 0xFFFF0000)) ;
		right+=2;
	}

	while ((src < target)) {
		uint32_t n = *src++;
		*left++ = n & 0xFFFF;
		*right++ = n >> 16;
	}
}

// Called from the USB interrupt when an isochronous packet arrives
// we must completely remove it from the receive buffer before returning
//
void usb_audio_receive_callback(unsigned int len)
{
	unsigned int count, avail;
	audio_block_t *left, *right;
	const uint32_t *data;

	AudioInputUSB::receive_flag = 1;
	len >>= 2; // 1 sample = 4 bytes: 2 left, 2 right
	data = (const uint32_t *)usb_audio_receive_buffer;

	count = AudioInputUSB::incoming_count;
	left = AudioInputUSB::incoming_left;
	right = AudioInputUSB::incoming_right;
	if (left == NULL) {
		left = AudioStream::allocate();
		if (left == NULL) return;
		AudioInputUSB::incoming_left = left;
	}
	if (right == NULL) {
		right = AudioStream::allocate();
		if (right == NULL) return;
		AudioInputUSB::incoming_right = right;
	}
	while (len > 0) {
		avail = AUDIO_BLOCK_SAMPLES - count;
		if (len < avail) {
			copy_to_buffers(data, left->data + count, right->data + count, len);
			AudioInputUSB::incoming_count = count + len;
			return;
		} else if (avail > 0) {
			copy_to_buffers(data, left->data + count, right->data + count, avail);
			data += avail;
			len -= avail;
			if (AudioInputUSB::ready_left || AudioInputUSB::ready_right) {
				// buffer overrun, PC sending too fast
				AudioInputUSB::incoming_count = count + avail;
				//if (len > 0) {
					//serial_print("!");
					//serial_phex(len);
				//}
				return;
			}
			send:
			AudioInputUSB::ready_left = left;
			AudioInputUSB::ready_right = right;
			//if (AudioInputUSB::update_responsibility) AudioStream::update_all();
			left = AudioStream::allocate();
			if (left == NULL) {
				AudioInputUSB::incoming_left = NULL;
				AudioInputUSB::incoming_right = NULL;
				AudioInputUSB::incoming_count = 0;
				return;
			}
			right = AudioStream::allocate();
			if (right == NULL) {
				AudioStream::release(left);
				AudioInputUSB::incoming_left = NULL;
				AudioInputUSB::incoming_right = NULL;
				AudioInputUSB::incoming_count = 0;
				return;
			}
			AudioInputUSB::incoming_left = left;
			AudioInputUSB::incoming_right = right;
			count = 0;
		} else {
			if (AudioInputUSB::ready_left || AudioInputUSB::ready_right) return;
			goto send; // recover from buffer overrun
		}
	}
	AudioInputUSB::incoming_count = count;
}

void AudioInputUSB::update(void)
{
	audio_block_t *left, *right;

	__disable_irq();
	left = ready_left;
	ready_left = NULL;
	right = ready_right;
	ready_right = NULL;
	uint16_t c = incoming_count;
	uint8_t f = receive_flag;
	receive_flag = 0;
	__enable_irq();
	if (f) {
		int diff = AUDIO_BLOCK_SAMPLES/2 - (int)c;
		feedback_accumulator += diff;
		// TODO: min/max sanity check for feedback_accumulator??
		usb_audio_sync_feedback = (feedback_accumulator >> 8) + diff * 3;
		//if (diff > 0) {
			//serial_print(".");
		//} else if (diff < 0) {
			//serial_print("^");
		//}
	}
	//serial_phex(c);
	//serial_print(".");
	if (!left || !right) {
		//serial_print("#"); // buffer underrun - PC sending too slow
		if (f) feedback_accumulator += 10 << 8;
	}
	if (left) {
		transmit(left, 0);
		release(left);
	}
	if (right) {
		transmit(right, 1);
		release(right);
	}
}







bool AudioOutputUSB::update_responsibility;
audio_block_t * AudioOutputUSB::left_1st;
audio_block_t * AudioOutputUSB::left_2nd;
audio_block_t * AudioOutputUSB::right_1st;
audio_block_t * AudioOutputUSB::right_2nd;
uint16_t AudioOutputUSB::offset_1st;


uint16_t usb_audio_transmit_buffer[AUDIO_TX_SIZE/2] DMABUFATTR;
uint8_t usb_audio_transmit_setting=0;

void AudioOutputUSB::begin(void)
{
	update_responsibility = false;
	left_1st = NULL;
	right_1st = NULL;
}

static void copy_from_buffers(uint32_t *dst, int16_t *left, int16_t *right, unsigned int len)
{
	// TODO: optimize...
	while (len > 0) {
		*dst++ = (*right++ << 16) | (*left++ & 0xFFFF);
		len--;
	}
}

void AudioOutputUSB::update(void)
{
	audio_block_t *left, *right;

	left = receiveReadOnly(0); // input 0 = left channel
	right = receiveReadOnly(1); // input 1 = right channel
	if (usb_audio_transmit_setting == 0) {
		if (left) release(left);
		if (right) release(right);
		if (left_1st) { release(left_1st); left_1st = NULL; }
		if (left_2nd) { release(left_2nd); left_2nd = NULL; }
		if (right_1st) { release(right_1st); right_1st = NULL; }
		if (right_2nd) { release(right_2nd); right_2nd = NULL; }
		offset_1st = 0;
		return;
	}
	if (left == NULL) {
		if (right == NULL) return;
		right->ref_count++;
		left = right;
	} else if (right == NULL) {
		left->ref_count++;
		right = left;
	}
	__disable_irq();
	if (left_1st == NULL) {
		left_1st = left;
		right_1st = right;
		offset_1st = 0;
	} else if (left_2nd == NULL) {
		left_2nd = left;
		right_2nd = right;
	} else {
		// buffer overrun - PC is consuming too slowly
		audio_block_t *discard1 = left_1st;
		left_1st = left_2nd;
		left_2nd = left;
		audio_block_t *discard2 = right_1st;
		right_1st = right_2nd;
		right_2nd = right;
		offset_1st = 0; // TODO: discard part of this data?
		//serial_print("*");
		release(discard1);
		release(discard2);
	}
	__enable_irq();
}


// Called from the USB interrupt when ready to transmit another
// isochronous packet.  If we place data into the transmit buffer,
// the return is the number of bytes.  Otherwise, return 0 means
// no data to transmit
unsigned int usb_audio_transmit_callback(void)
{
	static uint32_t count=5;
	uint32_t avail, num, target, offset, len=0;
	audio_block_t *left, *right;

	if (++count < 9) {   // TODO: dynamic adjust to match USB rate
		target = 44;
	} else {
		count = 0;
		target = 45;
	}
	while (len < target) {
		num = target - len;
		left = AudioOutputUSB::left_1st;
		if (left == NULL) {
			// buffer underrun - PC is consuming too quickly
			memset(usb_audio_transmit_buffer + len, 0, num * 4);
			//serial_print("%");
			break;
		}
		right = AudioOutputUSB::right_1st;
		offset = AudioOutputUSB::offset_1st;

		avail = AUDIO_BLOCK_SAMPLES - offset;
		if (num > avail) num = avail;

		copy_from_buffers((uint32_t *)usb_audio_transmit_buffer + len,
			left->data + offset, right->data + offset, num);
		len += num;
		offset += num;
		if (offset >= AUDIO_BLOCK_SAMPLES) {
			AudioStream::release(left);
			AudioStream::release(right);
			AudioOutputUSB::left_1st = AudioOutputUSB::left_2nd;
			AudioOutputUSB::left_2nd = NULL;
			AudioOutputUSB::right_1st = AudioOutputUSB::right_2nd;
			AudioOutputUSB::right_2nd = NULL;
			AudioOutputUSB::offset_1st = 0;
		} else {
			AudioOutputUSB::offset_1st = offset;
		}
	}
	return target * 4;
}


#endif // F_CPU
#endif // AUDIO_INTERFACE
