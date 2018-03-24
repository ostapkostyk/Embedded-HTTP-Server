/**
  ******************************************************************************
  * @file    LED.h
  * @author  Ostap Kostyk
  * @brief   LED class
  *          This code handles LED's (Light emitting diodes) operation
  *          Constructor of the class creates linked list of LEDs
  *          Main handler handles all LEDs from the list ones called
  *          Main handler must be called periodically (e.g. in main cycle)
  *
  ******************************************************************************
  * Copyright (C) 2018  Ostap Kostyk
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  * Author contact information: Ostap Kostyk, email: ostap.kostyk@gmail.com
  ******************************************************************************
 */

#ifndef LED_H_
#define LED_H_

#include "common.h"

#ifndef RTOS_USED
#include <Timer.h>

using namespace mTimer;
#endif

// LEDCtrlTime LED mode is recalculated every LEDCtrlTime to save CPU time
#define LEDCtrlTime     (_100ms_)

class LED
{
public:
/* Describes which logical level turns LED on */
enum class eActiveLevel{Low = 0, High};

/* Constructor */
LED(PORT_DEF Port_, PIN_NUM_DEF Pin_, eActiveLevel Level_);

/* recalculates all existing LEDs, should be called in main() */
static void Ctrl(void);

/* Turns LED constantly On */
void On();

/* Turns LED constantly Off */
void Off();

/* Blinks LED continuously with On time TimeOn, Off time TimeOff. Starts with On state if StartsWithOn is true otherwise starts with Off state */
void Blink(unsigned long OnTime, unsigned long OffTime, bool StartsWithOn);

/* Blink N times with On/Off time equal to TimeOn/TimeOff and then turns off constantly */
void BlinkNtimes(unsigned long OnTime, unsigned long OffTime, unsigned int N);

private:
/* LEDMode enumeration
 * Describes LED working Mode.
 *
 * Off: constantly Off;
 * On: constantly On;
 * BlinkOn: constantly blinking, starts with On;
 * BlinkOff: constantly blinking, starts with Off state;
 * NFlashes: flashes N times and then goes Off, starts with On state
*/
enum class eMode{OFF = 0, ON, BlinkOn, BlinkOff, NFlashes};

enum class eState{OFF = 0, ON};

/* Linked list of LED instances */
static LED* pFirst;
LED* Prev;
LED* Next;

/* HW resources */
PORT_DEF Port;
PIN_NUM_DEF Pin;
eActiveLevel Level;

/* Miscellaneous */
eMode Mode;
eMode ModePrev;
eState State;
unsigned long OnTime;
unsigned long OffTime;
unsigned long TimeCounter;
unsigned int  FlashesCounter;
};

extern void LEDWritePin (PORT_DEF Port, PIN_NUM_DEF Pin, PIN_STATE_DEF PinState);

#endif /* LED_H_ */
