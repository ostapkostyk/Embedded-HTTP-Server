#ifndef _MY_HAL_UART_H_
#define _MY_HAL_UART_H_

#include "stm32f1xx_hal.h"
#include "CircularBuffer.h"
//#include <stdlib.h>
#include "User_typedefs.h"

/**
  * @brief HAL UART State structures definition
  */
typedef enum
{
  MY_HAL_UART_STATE_TX_ACTIVE         = (1<<0),
  MY_HAL_UART_STATE_RX_OVERFLOW       = (1<<1),
  MY_HAL_UART_STATE_ERROR             = (1<<7)     /*!< Error                                              */
}My_HAL_UART_StateTypeDef;

typedef struct
{
uint8_t         *pRxBuff;         /* Buffer Begin         */
uint8_t         *pRxHead;         /* Head of data         */
uint8_t         *pRxTail;         /* Tail of data         */
uint16_t         RxBuffSize;      /* Buffer size          */
uint16_t         RxBuffReceived;  /* How much data received */

uint8_t         *pTxBuff;         /* Buffer Begin         */
uint8_t         *pTxHead;         /* Head of data         */
uint8_t         *pTxTail;         /* Tail of data         */
uint16_t         TxBuffSize;      /* Buffer size          */
uint16_t         TxBuffLeft;      /* How much space left in buffer */
} USART_IO_buffer;

#define HAL_UART_STATE_RX_OVERFUL 0x01

STATUS Init_USART_CB(UART_HandleTypeDef *huart, circular_buffer *cb_Rx, circular_buffer *cb_Tx, size_t cb_size_rx, size_t cb_size_tx, size_t data_size);
void Start_USART_Rx_IT(UART_HandleTypeDef *huart);
HAL_StatusTypeDef My_HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
void My_HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);

/**
 * @brief Checks Rx overflow flag and clears it if set
 * @param huart: uart handle
 * @retval zero if overflow flag is not set, otherwise (-1)
 */
int UART_RxDataOverflow(UART_HandleTypeDef *huart);

#endif
