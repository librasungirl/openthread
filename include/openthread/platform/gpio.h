/*
 *  Copyright (c) 2019, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *   This file includes the platform abstraction for GPIO.
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup gpio GPIO
 * @ingroup platform
 *
 * @brief
 *   This module includes the platform abstraction to GPIO.
 *
 * @{
 *
 */

enum
{
    GPIO_LOGIC_HIGH = 1,
    GPIO_LOGIC_LOW  = 0,
};

/**
 * Set logic high for output pin.
 *
 */
void otPlatGpioSet(uint32_t port, uint8_t pin);

/**
 * Set logic low for output pin.
 *
 */
void otPlatGpioClear(uint32_t port, uint8_t pin);

/**
 * Clear all LED
 *
 */
void otPlatGpioClearAll();

/**
 * Toggle output pin.
 *
 */
void otPlatGpioToggle(uint32_t port, uint8_t pin);

/**
 * Read the value of output pin.
 *
 */
uint8_t otPlatGpioGet(uint32_t port, uint8_t pin);

// typedef bool (*otPlatGpioIntHandler)(void *aContext);
//, uint32_t aPort, uint32_t aPin);
/*
typedef struct otPlatGpioIntCallback
{
    struct otPlatGpioIntCallback *mNext;    ///< A pointer to the next UDP receiver (internal use only).
    otPlatGpioIntHandler          mHandler; ///< A function pointer to the receiver callback.
    void *                        mContext; ///< A pointer to application-specific context.
} otPlatGpioIntCallback;
*/

/**
 * Register a callback for GPIO interrupt.
 *
 */
// void otPlatGpioAddCallback(otInstance *aInstance, uint32_t port, uint32_t pin, otPlatGpioIntCallback aCallback);

/**
 * Turn off all the LEDs
 *
 */
void otPlatGpioTurnOffAll();

/**
 * @}
 *
 */

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // GPIO_H_
