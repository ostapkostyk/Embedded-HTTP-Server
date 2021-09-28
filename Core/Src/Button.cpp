/**
  ******************************************************************************
  * @file    Button.cpp
  * @author  Ostap Kostyk
  * @brief   Button class
  *          This code handles hardware buttons
  *          Constructor of the class creates linked list of buttons
  *          Main handler handles all buttons from the list ones called
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

#include "Button.h"

Button* Button::pFirst = 0;

#ifndef RTOS_USED
Timer ButtonCtrlTimer{Timer::Type::Down, ButtonCtrlTime, true};
#endif

Button::Button(PORT_DEF Port, PIN_NUM_DEF Pin, ePressedLevel Level)
{
Button* pButton;

    Prev = 0;
    Next = 0;
    State = eState::Released;
    NewValue = eState::Released;
    OldValue = eState::Released;
    TestValue = eState::Released;
    PressedEventFlag = false;
    ReleasedEventFlag = false;
    PressedTimeCounter = 0;
    ReleasedTimeCounter = 0;
    Step = 0;

    this->Port  = Port;
    this->Pin   = Pin;
    this->Level = Level;

    pButton = Button::pFirst;

    if(pButton == 0)     //  Creating first instance of the Button class
    {
        Button::pFirst = this;
    }
    else
    {
        while(pButton->Next != 0)
        {
            pButton = pButton->Next;
        }
        pButton->Next = this;
        Prev = pButton;
    }
}

void Button::ClearPressedEvent(void)
{
    PressedEventFlag = false;
}

void Button::ClearReleasedEvent(void)
{
    ReleasedEventFlag = false;
}

unsigned long Button::GetPressedTime(void)
{
    return (PressedTimeCounter * ButtonCtrlTime);
}

unsigned long Button::GetReleasedTime(void)
{
    return (ReleasedTimeCounter * ButtonCtrlTime);
}

void Button::Ctrl(void)
{
Button* pButton;

    if(Button::pFirst == 0)   //  No buttons exist
    {
        return;
    }

    pButton = Button::pFirst;

#ifndef RTOS_USED
    if(ButtonCtrlTimer.Elapsed())
#endif
    {
        while(1)
        {
            if(ButtonReadPin(pButton->Port, pButton->Pin) == 0)
            {
                if(pButton->Level == ePressedLevel::Low) { pButton->NewValue = eState::Pressed; }
                else                                     { pButton->NewValue = eState::Released; }
            }
            else
            {
                if(pButton->Level == ePressedLevel::Low) { pButton->NewValue = eState::Released; }
                else                                     { pButton->NewValue = eState::Pressed; }
            }

            switch(pButton->Step)
            {
            case 0:
                if(pButton->NewValue != pButton->OldValue)
                {
                    pButton->TestValue = pButton->NewValue;
                    pButton->Step = 1;
                }
                break;

            case 1:
                if(pButton->NewValue != pButton->OldValue && pButton->NewValue == pButton->TestValue)   //  new and verified state
                {
                    pButton->OldValue = pButton->NewValue;
                    pButton->Step = 0;
                    pButton->State = pButton->NewValue;
                    if(pButton->NewValue == eState::Pressed)
                    {
                        pButton->PressedEventFlag = true;
                        pButton->PressedTimeCounter = 0;
                    }
                    else
                    {
                        pButton->ReleasedEventFlag = true;
                        pButton->ReleasedTimeCounter = 0;
                    }
                }
                else
                {
                    pButton->TestValue = pButton->NewValue;
                    pButton->Step = 2;
                }
                break;

            case 2:
                if(pButton->NewValue != pButton->OldValue && pButton->NewValue == pButton->TestValue) { pButton->Step = 1; }   //  verified state
                else
                {
                    pButton->OldValue = pButton->NewValue;
                    pButton->TestValue = pButton->NewValue;
                    pButton->Step = 0;
                }
                break;

            default:
                pButton->Step = 0;
                break;
            }

            if(pButton->State == eState::Pressed) { pButton->PressedTimeCounter++; }
            else                                  { pButton->ReleasedTimeCounter++; }

            if(pButton->Next == 0) { return; }   //  all Buttons done

            pButton = pButton->Next;

        }   //  END of while(1)
    }   //  END of if(ButtonCtrlTimer.Elapsed())
}
