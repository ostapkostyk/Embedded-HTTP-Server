/**
  ******************************************************************************
  * @file    Button.h
  * @author  Ostap Kostyk
  * @brief   Button class
  *          This code handles hardware buttons
  *          Constructor of the class creates linked list of buttons
  *          Main handler handles all buttons from the list ones called
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

#ifndef BUTTON_H_
#define BUTTON_H_

#include "common.h"

#ifndef RTOS_USED
#include <Timer.h>

using namespace mTimer;
#endif

// ButtonCtrlTime: Button state is recalculated once timer run out to save CPU time
#define ButtonCtrlTime     (_30ms_)

class Button
{
public:
/* Describes which logical level corresponds to pressed button */
enum class ePressedLevel{Low = 0, High};

/* Button states enumeration */
enum class eState{Released = 0, Pressed};

/* Constructor */
Button(PORT_DEF Port, PIN_NUM_DEF Pin, ePressedLevel Level);

/* recalculates all existing Buttons, should be called in main() */
static void Ctrl(void);

/* returns current state of the button (Released or Pressed) */
eState GetState(void) const { return State; }

/* returns true if button has been pressed. This flag stays active until ClearPressedEvent() method called */
bool PressedEvent(void) const { return PressedEventFlag; }

/* returns true if button has been released. This flag stays active until ClearReleasedEvent() method called */
bool ReleasedEvent(void) const { return ReleasedEventFlag; }

void ClearPressedEvent(void);

void ClearReleasedEvent(void);

/* returns time in ms since button has been pressed and counts while button stays pressed.
 * Automatically cleared when button is pressed (when state changes from Released to Pressed).
 * Hold it's value while button is released */
unsigned long GetPressedTime(void);

/* vice versa for above */
unsigned long GetReleasedTime(void);

private:
/* Linked list of Button instances */
static Button* pFirst;
Button* Prev;
Button* Next;

/* Hardware resources */
PORT_DEF Port;
PIN_NUM_DEF Pin;
ePressedLevel Level;

/* Miscellaneous */
eState State, NewValue, OldValue, TestValue;
bool PressedEventFlag, ReleasedEventFlag;
unsigned char Step;
unsigned long PressedTimeCounter, ReleasedTimeCounter;
};

/* External function, implements hardware-dependent reading. Reads Pin state and returns zero when Low level or non-zero when High level */
extern int ButtonReadPin (PORT_DEF Port, PIN_NUM_DEF Pin);

#endif /* BUTTON_H_ */
