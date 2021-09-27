/**
  ******************************************************************************
  * @file    LED.cpp
  * @author  Ostap Kostyk
  * @brief   LED class
  *          This code handles LED's (Light emitting diodes) operation
  *          Constructor of the class creates linked list of LEDs
  *          Main handler handles all LEDs from the list ones called
  *          Main handler must be called periodically (e.g. in main cycle or
  *          in thread)
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

#include <LED.h>

LED* LED::pFirst = 0;

#ifndef RTOS_USED
Timer LEDCtrlTimer{Timer::Type::Down, LEDCtrlTime, true};
#endif

LED::LED(PORT_DEF Port, PIN_NUM_DEF Pin, eActiveLevel Level)
{
LED* pLED;

    Prev = 0;
    Next = 0;
    Mode = eMode::OFF;
    ModePrev = eMode::OFF;
    State = eState::OFF;
    OnTime = 0;
    OffTime = 0;
    TimeCounter = 0;
    FlashesCounter = 0;
    this->Port  = Port;
    this->Pin   = Pin;
    this->Level = Level;

    pLED = LED::pFirst;

    if(pLED == 0)     //  Creating first instance of the Timer class
    {
        LED::pFirst = this;
    }
    else
    {
        while(pLED->Next != 0)
        {
            pLED = pLED->Next;
        }
        pLED->Next = this;
        Prev = pLED;
    }
}

void LED::On()
{
    Mode = eMode::ON;
    ModePrev = eMode::ON;
    State = eState::ON;
    if(Level == eActiveLevel::High)
    {
        LEDWritePin(Port, Pin, PIN_STATE_HIGH);
    }
    else
    {
        LEDWritePin(Port, Pin, PIN_STATE_LOW);
    }
}

void LED::Off()
{
    Mode = eMode::OFF;
    ModePrev = eMode::OFF;
    State = eState::OFF;
    if(Level == eActiveLevel::High)
    {
        LEDWritePin(Port, Pin, PIN_STATE_LOW);
    }
    else
    {
        LEDWritePin(Port, Pin, PIN_STATE_HIGH);
    }
}

void LED::Blink(unsigned long OnTime, unsigned long OffTime, bool StartsWithOn)
{
    // convert time into number of LED timer time-outs
    if(OnTime == 0 || OffTime == 0 || LEDCtrlTime == 0)
    {
        LED::Off();
        return;
    }

    if(OnTime < LEDCtrlTime) { OnTime = LEDCtrlTime; }
    if(OffTime < LEDCtrlTime) { OffTime = LEDCtrlTime; }

    this->OnTime = OnTime / LEDCtrlTime;
    this->OffTime = OffTime / LEDCtrlTime;

    // Set LED mode
    if(StartsWithOn) { Mode = eMode::BlinkOn; }
    else             { Mode = eMode::BlinkOff; }
}

void LED::BlinkNtimes(unsigned long OnTime, unsigned long OffTime, unsigned int N)
{
    // convert time into number of LED timer time-outs
    if(OnTime == 0 || OffTime == 0 || LEDCtrlTime == 0)
    {
        LED::Off();
        return;
    }

    if(OnTime  < LEDCtrlTime) { OnTime  = LEDCtrlTime; }
    if(OffTime < LEDCtrlTime) { OffTime = LEDCtrlTime; }

    this->OnTime = OnTime / LEDCtrlTime;
    this->OffTime = OffTime / LEDCtrlTime;

    Mode = eMode::NFlashes;

    if(Mode != ModePrev)
    {
        ModePrev = Mode;
        TimeCounter = 0;
        FlashesCounter = N;
    }
}

void LED::Ctrl(void)
{
LED* pLED;

    if(LED::pFirst == 0)   //  No timers exist
    {
        return;
    }

    pLED = LED::pFirst;

#ifndef RTOS_USED
    if(LEDCtrlTimer.Elapsed())
#endif
    {
        while(1)
        {
            if(pLED->Mode == eMode::BlinkOn || pLED->Mode == eMode::BlinkOff || pLED->Mode == eMode::NFlashes)
            {
                if(pLED->Mode != pLED->ModePrev)    //  this check must be here (not in LED::Blink method) to be synchronized with LEDCtrlTimer tick
                {
                    pLED->ModePrev = pLED->Mode;
                    switch(pLED->Mode)
                    {
                    case eMode::BlinkOn:
                    case eMode::NFlashes:
                        pLED->TimeCounter = pLED->OnTime;
                        pLED->State = eState::ON;
                        if(pLED->Level == eActiveLevel::High) { LEDWritePin(pLED->Port, pLED->Pin, PIN_STATE_HIGH); }
                        else                                  { LEDWritePin(pLED->Port, pLED->Pin, PIN_STATE_LOW);  }
                        break;

                    case eMode::BlinkOff:
                        pLED->TimeCounter = pLED->OffTime;
                        pLED->State = eState::OFF;
                        if(pLED->Level == eActiveLevel::High) { LEDWritePin(pLED->Port, pLED->Pin, PIN_STATE_LOW); }
                        else                                  { LEDWritePin(pLED->Port, pLED->Pin, PIN_STATE_HIGH);  }
                        break;

                    default:                        //  this must never be reached
                        pLED->Mode = eMode::OFF;
                    }
                }
                else
                {
                    if(pLED->Mode == eMode::NFlashes && pLED->FlashesCounter == 0)
                    {
                        // Do nothing
                    }
                    else
                    {
                        if(pLED->TimeCounter) { pLED->TimeCounter--; }
                        if(pLED->TimeCounter == 0)
                        {
                            if(pLED->State == eState::ON)
                            {
                                pLED->TimeCounter = pLED->OffTime;
                                pLED->State = eState::OFF;
                                if(pLED->Level == eActiveLevel::High) { LEDWritePin(pLED->Port, pLED->Pin, PIN_STATE_LOW); }
                                else                                  { LEDWritePin(pLED->Port, pLED->Pin, PIN_STATE_HIGH);  }
                                if(pLED->Mode == eMode::NFlashes && pLED->FlashesCounter != 0) { pLED->FlashesCounter--; }
                            }
                            else
                            {
                                pLED->TimeCounter = pLED->OnTime;
                                pLED->State = eState::ON;
                                if(pLED->Level == eActiveLevel::High) { LEDWritePin(pLED->Port, pLED->Pin, PIN_STATE_HIGH); }
                                else                                  { LEDWritePin(pLED->Port, pLED->Pin, PIN_STATE_LOW);  }
                            }
                        }
                    }
                }
            }

            if(pLED->Next == 0) { return; }   //  all LEDs done

            pLED = pLED->Next;

        }   //  END of while(1)
    }   //  END of if(LEDCtrlTimer.Elapsed())
}













