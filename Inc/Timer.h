/**
  ******************************************************************************
  * @file    Timer.h
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

#ifndef TIMER_H_
#define TIMER_H_

namespace mTimer
{

#define HW_TIMER_TIME   1   //  1ms = Interrupt_timer_time * HW_TIMER_TIME

// ==========  TIME DEFINITIONS  ==========
/* Comment: starting names with underscore is not desirable but
 * it is very unlikely that system will use combination of the
 * number following by underscore so for convenience _<number>ms_
 * format is used */
#define _10ms_  (10UL*HW_TIMER_TIME)
#define _30ms_  (30UL*HW_TIMER_TIME)
#define _50ms_  (50UL*HW_TIMER_TIME)
#define _100ms_ (100UL*HW_TIMER_TIME)
#define _200ms_ (200UL*HW_TIMER_TIME)
#define _300ms_ (300UL*HW_TIMER_TIME)
#define _500ms_ (500UL*HW_TIMER_TIME)
#define _700ms_ (700UL*HW_TIMER_TIME)
#define _800ms_ (800UL*HW_TIMER_TIME)
#define _900ms_ (900UL*HW_TIMER_TIME)
#define _1sec_  (1000UL*HW_TIMER_TIME)
#define _2sec_  (2000UL*HW_TIMER_TIME)
#define _3sec_  (3000UL*HW_TIMER_TIME)
#define _4sec_  (4000UL*HW_TIMER_TIME)
#define _5sec_  (5000UL*HW_TIMER_TIME)
#define _10sec_ (10000UL*HW_TIMER_TIME)
#define _20sec_ (20000UL*HW_TIMER_TIME)
#define _30sec_ (30000UL*HW_TIMER_TIME)
#define _1min_  (60000UL*HW_TIMER_TIME)
#define _2min_  (120000UL*HW_TIMER_TIME)
#define _5min_  (300000UL*HW_TIMER_TIME)
#define _10min_ (600000UL*HW_TIMER_TIME)
#define _30min_ (1800000UL*HW_TIMER_TIME)
#define _1hour_ (3600000UL*HW_TIMER_TIME)
#define _2hour_ (7200000UL*HW_TIMER_TIME)
#define _5hour_ (18000000UL*HW_TIMER_TIME)

class Timer
{
public:
	enum Type{Down = 0, Up};

/* Constructor */
Timer();
Timer(Type tType, unsigned long Time, bool enable);

/* Public Methods */
static void Tick();                                    // re-calculate timer, must be called every timer-atomic time like 1ms ones for all existing timers
void Set(unsigned long Value);                         // sets value for comparison
void Reset();                                          // clear counter, RanOutFlag and enables timer (starts it)
bool Elapsed();                                        // Test if timer ran-out, flag is cleared by calling function (if already set). Returns true if timer ran-out
void Stop()                 { Enable = false; };       // Make pause (stops counting but do not change any values, do not reset or restart)
void Continue()             { Enable = true; };        // continue timer after pause (after Stop() method)
unsigned long Get()   const { return Value; };         // Returns timer value in milliseconds
void SetType(Type tType)    { type = tType; };         // Sets timer type Up or Down

/*===========================*/
private:

static Timer* pFirst;
Timer* Prev;
Timer* Next;

bool RanOutFlag;      //  flag
unsigned long Value;
unsigned long InitVal;
bool Enable;
Type type;

};  //  End of class Timer



};  //  End of namespace mTimer
#endif /* TIMER_H_ */
