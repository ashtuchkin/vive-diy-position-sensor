/* Simple compatibility headers for AVR code used with ARM chips
 * Copyright (c) 2015 Paul Stoffregen <paul@pjrc.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _AVR_SLEEP_H_
#define _AVR_SLEEP_H_ 1

#define SLEEP_MODE_IDLE         0
#define SLEEP_MODE_ADC		0
#define SLEEP_MODE_PWR_DOWN	1
#define SLEEP_MODE_PWR_SAVE	1
#define SLEEP_MODE_STANDBY	1
#define SLEEP_MODE_EXT_STANDBY	1

#define set_sleep_mode(mode)	// TODO: actually set the mode...
#define sleep_enable()
#define sleep_disable()
#define sleep_cpu()		(asm("wfi"))
#define sleep_bod_disable()
#define sleep_mode()		sleep_cpu()

// workaround for early versions of Nordic's BLE library
// EIMSK moved to a dummy byte in avr_emulation...
//#if defined(HAL_ACI_TL_H__) && defined(PLATFORM_H__)
//#define EIMSK uint8_t EIMSKworkaround=0; EIMSKworkaround
//#endif

#endif
