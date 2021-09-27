/**
  ******************************************************************************
  * @file    ESP8266_Interface.c
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

#include "ESP8266_Interface.h"

circular_buffer ESP1_cb_Rx;
circular_buffer ESP1_cb_Tx;

void ESP_Enable(uint8_t HuartNumber)
{
    switch(HuartNumber)
    {
    case ESP1_HUART_NUM:
        HAL_GPIO_WritePin(ESP_CH_EN_GPIO_Port, ESP_CH_EN_Pin, GPIO_PIN_SET);  //  Enable module
        break;

    default:

        break;
    }
}

void ESP_Disable(uint8_t HuartNumber)
{
    switch(HuartNumber)
    {
    case ESP1_HUART_NUM:
        HAL_GPIO_WritePin(ESP_CH_EN_GPIO_Port, ESP_CH_EN_Pin, GPIO_PIN_RESET);  //  Disable module
        break;

    default:

        break;
    }
}

STATUS ESP_HuartInit(uint8_t HuartNumber)
{
    switch(HuartNumber)
    {
    case ESP1_HUART_NUM:
        // Configure UART for ESP8266 module
        if(SUCCESS != Init_USART_CB(&ESP1_HUART, &ESP1_cb_Rx, &ESP1_cb_Tx, ESP8266_CB_RX_SIZE, ESP8266_CB_TX_SIZE, sizeof(U8))) //  TODO
        {
            //BlockingFaultHandler();
            return ERROR;
        }
        Start_USART_Rx_IT(&ESP1_HUART);
        return SUCCESS;
        break;

    default:
        return ERROR;
        break;
    }
}

STATUS ESP_HuartSend(uint8_t HuartNumber, char* pData, size_t Size)
{
    circular_buffer* cb_Tx;
    char * data = pData;

    switch(HuartNumber)
    {
    case ESP1_HUART_NUM:


        cb_Tx = (circular_buffer*)(ESP1_HUART.pTxBuffPtr);

        if(cb_space_left(cb_Tx) >= Size)
        {
            __HAL_UART_DISABLE_IT(&ESP1_HUART, UART_IT_TXE);
            while(Size)
            {
               if(SUCCESS != cb_push_back(cb_Tx, data))
               {
                   __HAL_UART_ENABLE_IT(&ESP1_HUART, UART_IT_TXE);
                   return ERROR;
               }
               else
               {
                   data++;
                   Size--;
               }
            }
            __HAL_UART_ENABLE_IT(&ESP1_HUART, UART_IT_TXE);
            return SUCCESS;
        }
        else
        {
            return ERROR;
        }
        break;

    default:
        return ERROR;
        break;
    }
};

STATUS ESP_GetChar(uint8_t HuartNumber, uint8_t* sym)
{
circular_buffer* cb_Rx;

    switch(HuartNumber)
    {
    case ESP1_HUART_NUM:
        cb_Rx = (circular_buffer*)(ESP1_HUART.pRxBuffPtr);
        return cb_pop_front(cb_Rx, sym);
        break;

    default:
        return ERROR;
        break;
    }
}

//  returns number of bytes received from Rx stream of HUART defined by HuartNumber
size_t ESP_NumOfDataReceived(uint8_t HuartNumber)
{
circular_buffer* cb_Rx;

    switch(HuartNumber)
        {
        case ESP1_HUART_NUM:
            cb_Rx = (circular_buffer*)(ESP1_HUART.pRxBuffPtr);
            return cb_space_occupied(cb_Rx);
            break;

        default:
            return ERROR;
            break;
        }
}

// returns number of bytes of space left in transmit buffer
size_t ESP_TransmitBufferSpaceLeft(uint8_t HuartNumber)
{
circular_buffer* cb_Tx;

    switch(HuartNumber)
        {
        case ESP1_HUART_NUM:
            cb_Tx = (circular_buffer*)(ESP1_HUART.pTxBuffPtr);
            return cb_space_left(cb_Tx);
            break;

        default:
            return ERROR;
            break;
        }
}

/* Activates reset pin of the ESP8266 (make Reset of the module) */
void ESP_ActivateResetPin(uint8_t HuartNumber)
{
    switch(HuartNumber)
        {
        case ESP1_HUART_NUM:
            HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_RESET);
            break;

        default:

            break;
        }
    return;
}

/* Releases reset pin of the ESP8266 so module can start */
void ESP_ReleaseResetPin(uint8_t HuartNumber)
{
    switch(HuartNumber)
        {
        case ESP1_HUART_NUM:
            HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_SET);
            break;

        default:

            break;
        }
    return;
}

STATUS ESP_HuartRxOverflow(uint8_t HuartNumber)
{
    switch(HuartNumber)
        {
        case ESP1_HUART_NUM:
            if(UART_RxDataOverflow(&ESP1_HUART)) { return (-1); }
            break;

        default:

            break;
        }
    return 0;
}

/* Sets HUART baudrate */
void ESP_SetBaudRate(uint8_t HuartNumber, uint32_t Baud)
{
    switch(HuartNumber)
    {
    case ESP1_HUART_NUM:
        ESP1_HUART.Init.BaudRate = Baud;
        ESP1_HUART.Init.WordLength = UART_WORDLENGTH_8B;
        ESP1_HUART.Init.StopBits = UART_STOPBITS_1;
        ESP1_HUART.Init.Parity = UART_PARITY_NONE;
        ESP1_HUART.Init.Mode = UART_MODE_TX_RX;
        ESP1_HUART.Init.HwFlowCtl = UART_HWCONTROL_NONE;
        ESP1_HUART.Init.OverSampling = UART_OVERSAMPLING_16;

        HAL_UART_Init(&ESP1_HUART);
        break;

    default:

        break;
    }
}

