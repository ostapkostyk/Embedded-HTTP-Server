/**
  ******************************************************************************
  * @file    Timer.cpp
  * @author  Ostap Kostyk
  * @brief   Timer class
  *          This file provides implementation of the software timers
  *          Constructor of the class creates linked list of timers
  *          Main handler handles all timers from the list ones called
  *          Main handler must be called every tick usually provided by HW timer
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

#include <Timer.h>

namespace mTimer
{

Timer* Timer::pFirst = 0;

Timer::Timer(void)
{
Timer* pTimer;

    RanOutFlag = 0;
    Value = 0;
    InitVal = 0;
    Enable = 0;
    Prev = 0;
    Next = 0;
    type = Down;

    pTimer = Timer::pFirst;

    if(pTimer == 0)     //  Creating first instance of the Timer class
    {
        Timer::pFirst = this;
    }
    else
    {
        while(pTimer->Next != 0)
        {
            pTimer = pTimer->Next;
        }
        pTimer->Next = this;
        Prev = pTimer;
    }
}

Timer::Timer(Type tType, unsigned long Time, bool enable)
{
Timer* pTimer;

	RanOutFlag = 0;
	InitVal = Time;
	Enable = enable;
	Prev = 0;
	Next = 0;
	type = tType;
	switch(type)
	{
	case Timer::Type::Up:
		Value = 0;
		break;
	default:
	case Timer::Type::Down:
		Value = Time;
		break;
	}

	pTimer = Timer::pFirst;

	if(pTimer == 0)     //  Creating first instance of the Timer class
	{
		Timer::pFirst = this;
	}
	else
	{
		while(pTimer->Next != 0)
		{
			pTimer = pTimer->Next;
		}
		pTimer->Next = this;
		Prev = pTimer;
	}
}

void Timer::Tick(void)
{
Timer* pTimer;

    if(Timer::pFirst == 0)   //  No timers exist
    {
        return;
    }

    pTimer = Timer::pFirst;

    while(1)
    {
        if(pTimer->Enable == true)
        {
            switch(pTimer->type)
            {
            case Timer::Type::Up:
                pTimer->Value++;
                if(pTimer->InitVal != 0)
                {
                    if(pTimer->Value == pTimer->InitVal) { pTimer->RanOutFlag = true; }
                }
                break;

            default:
            case Timer::Type::Down:
                if(pTimer->Value != 0)
                {
                    pTimer->Value--;
                    if(pTimer->Value == 0)
                    {
                        pTimer->RanOutFlag = true;
                        pTimer->Value = pTimer->InitVal;
                    }
                }
                break;
            }
        }
        else
        {
            // do nothing
        }

        if(pTimer->Next == 0) { return; }   //  all timers done

        pTimer = pTimer->Next;
    }
}

void Timer::Set(unsigned long InitValue)
{
    InitVal = InitValue;
}

void Timer::Reset()
{
    if(type == Up)
    {
    	Value = 0;
    }
    else
    {
    	Value = InitVal;
    }

    RanOutFlag = 0;
    Enable = true;
}

bool Timer::Elapsed()
{
bool tmp;

    tmp = RanOutFlag;
    if(tmp) { RanOutFlag = 0; }
    return tmp;
}

}
