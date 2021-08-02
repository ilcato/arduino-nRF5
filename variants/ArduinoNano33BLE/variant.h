/*
  Copyright (c) 2014-2015 Arduino LLC.  All right reserved.
  Copyright (c) 2016 Sandeep Mistry All right reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _VARIANT_ARDUINO_NANO33_BLE_
#define _VARIANT_ARDUINO_NANO33_BLE_

/** Master clock frequency */
#define VARIANT_MCK       (64000000ul)

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// Number of pins defined in PinDescription array
#define PINS_COUNT           (34u)
#define NUM_DIGITAL_PINS     (21u)
#define NUM_ANALOG_INPUTS    (8u)
#define NUM_ANALOG_OUTPUTS   (0u)

// LEDs
#define PIN_LED                (13)
#define LED_BUILTIN            PIN_LED

// Buttons

/*
 * Analog pins
 */
#define PIN_A0               (14)
#define PIN_A1               (15)
#define PIN_A2               (16)
#define PIN_A3               (17)
#define PIN_A4               (18)
#define PIN_A5               (19)
#define PIN_A6               (20) /* AIN3 (P0.05)        */
#define PIN_A7               (21) /* AIN0 (P0.02) / AREF */

static const uint8_t A0  = PIN_A0 ; // AIN1
static const uint8_t A1  = PIN_A1 ; // AIN2
static const uint8_t A2  = PIN_A2 ; // AIN4
static const uint8_t A3  = PIN_A3 ; // AIN5
static const uint8_t A4  = PIN_A4 ; // AIN6
static const uint8_t A5  = PIN_A5 ; // AIN7
static const uint8_t A6  = PIN_A6 ; // AIN3 (P0.05)
static const uint8_t A7  = PIN_A7 ; // AIN0 (P0.02) / AREF
#define ADC_RESOLUTION    12

// Other pins
//#define PIN_AREF           (21)
//static const uint8_t AREF = PIN_AREF;

/*
 * Serial interfaces
 */
// Serial
#define PIN_SERIAL_RX       (1ul)
#define PIN_SERIAL_TX       (0ul)

/*
 * SPI Interfaces
 */
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_MISO         (12)
#define PIN_SPI_MOSI         (11)
#define PIN_SPI_SCK          (13)

static const uint8_t SS   = 10 ;
static const uint8_t MOSI = PIN_SPI_MOSI ;
static const uint8_t MISO = PIN_SPI_MISO ;
static const uint8_t SCK  = PIN_SPI_SCK ;

/*
 * Wire Interfaces
 */
#define WIRE_INTERFACES_COUNT 1

#define PIN_WIRE_SDA         (18u)
#define PIN_WIRE_SCL         (19u)

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

#define PIN_WIRE_SDA1       (30u)
#define PIN_WIRE_SCL1       (31u)

#define PIN_ENABLE_I2C_PULLUP      (32u)
#define PIN_ENABLE_SENSORS_3V3     (33u)

#define PIN_INT_APDS (26u)


/*
 * Reset Button at P0.21
 */
#define RESET_PIN            21

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#endif
