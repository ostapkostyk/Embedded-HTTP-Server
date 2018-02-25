/**
  ******************************************************************************
  * @file    ESP8266_def.hpp
  * @author  Ostap Kostyk
  * @version V1.0.0
  * @brief   This file contains common defines, enumeration, macros and
  *          structures definitions.
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
#ifndef ESP8266_DEF_HPP_
#define ESP8266_DEF_HPP_

enum class eAT : unsigned int
{
NO_COMMAND_RECEIVED = 0,

ECHO_AT,
ECHO_ECHO_OFF,
ECHO_ECHO_ON,
ECHO_RST,
ECHO_CWMODE,
ECHO_CWJAP,
ECHO_CWLAP,
ECHO_CWQAP,
ECHO_CWSAP,
ECHO_CIPSTATUS,
ECHO_CIPSTART,
ECHO_CIPCLOSE,
ECHO_CIFSR,
ECHO_CIPMUX,
ECHO_CIPSERVER,
ECHO_CIOBAUD_QM,
ECHO_UART_CUR,

ECHO_UNKNOWN,

READY,
OK,
CIOBAUD,
CIOBAUD_RANGE,
BAUDRATE_CONFIRMATION,
NOCHANGE,
IP,
FAIL,
ALREADY_CONNECT,
SEND_OK,
CIPSTATUS,
WRONG_SYNTAX,
AT_ERROR,
LINKED,
LINK,
LINKISBUILDED,
UNLINK,
BUSY,
BUSY_P,
BUSY_S,
LINKISNOT,
NOIP,
NO_THIS_FUNCTION,
CWJAP,
CWJAP_FAULT,
CWSAP_CUR,
CWMODE,
CWLAP,
CIFSR_APIP,
CIFSR_APMAC,
SOCKET_CONNECT,
SOCKET_CLOSED,
SOCKET_CONNECT_FAIL,
REBOOT_DETECTED,
WDT_RESET,

BAD_STRUCTURE,
UNKNOWN,
INTERNAL_ERROR
};

#endif /* ESP8266_DEF_HPP_ */
