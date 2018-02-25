/**
  ******************************************************************************
  * @file    ESP8266_Interface.h
  * @author  Ostap Kostyk
  * @brief   ESP8266_Interface implements hardware- and implementation-depended
  *          interface to ESP8266 class in C language to be compatible with
  *          Low-level implementation of the hardware or HAL
  *
  ******************************************************************************
  * Copyright (C) 2018  Ostap Kostyk
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version provided that the redistributions
  * of source code must retain the above copyright notice.
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

#ifndef ESP8266_INTERFACE_H_
#define ESP8266_INTERFACE_H_

#include "common.h"
#include "CircularBuffer.h"

/* ==== ESP 1 Configuration ==== */
#define ESP1_HUART_NUM  1
#define ESP1_HUART      huart2
#define ESP8266_CB_RX_SIZE  200
#define ESP8266_CB_TX_SIZE  200
/*  END of ESP 1 Configuration      */

extern UART_HandleTypeDef ESP1_HUART;
extern int UART_RxDataOverflow(UART_HandleTypeDef *huart);
extern STATUS Init_USART_CB(UART_HandleTypeDef *huart, circular_buffer *cb_Rx, circular_buffer *cb_Tx, size_t cb_size_rx, size_t cb_size_tx, size_t data_size);
extern void Start_USART_Rx_IT(UART_HandleTypeDef *huart);

/* Initialize streams to/from serial port. Any other required initialization can be implemented here */
STATUS ESP_HuartInit(uint8_t HuartNumber);

/* Enables ESP module by providing High level on "Enable" pin */
void ESP_Enable(uint8_t HuartNumber);

/* Disables ESP module by providing Low level on "Enable" pin */
void ESP_Disable(uint8_t HuartNumber);

/* Sends data to HUART defined by number HuartNumber. Returns SUCCESS if Success OR ERROR in case of error. Return type STATUS is uint32_t type */
STATUS ESP_HuartSend(uint8_t HuartNumber, char* pData, size_t Size);

// Gets next char from input stream from UART defined by number HuartNumber, returns SUCCESS if symbol extracted or ERROR if there is no more symbols from the stream
STATUS ESP_GetChar(uint8_t HuartNumber, uint8_t* sym);

//  returns number of bytes received from Rx stream of HUART defined by HuartNumber
size_t ESP_NumOfDataReceived(uint8_t HuartNumber);

// returns number of bytes of space left in transmit buffer
size_t ESP_TransmitBufferSpaceLeft(uint8_t HuartNumber);

/* Activates reset pin of the ESP8266 (make Reset of the module) */
void ESP_ActivateResetPin(uint8_t HuartNumber);

/* Releases reset pin of the ESP8266 so module can start */
void ESP_ReleaseResetPin(uint8_t HuartNumber);

/* Check if Overflow occured for Rx HUART buffer */
STATUS ESP_HuartRxOverflow(uint8_t HuartNumber);

/* Sets HUART baudrate */
void ESP_SetBaudRate(uint8_t HuartNumber, uint32_t Baud);

#endif /* ESP8266_INTERFACE_H_ */
