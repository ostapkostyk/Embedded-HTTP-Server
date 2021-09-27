/**
  ******************************************************************************
  * @file    stm32f1xx_hal_uart.c
  * @author  MCD Application Team
  * @version V1.0.4
  * @date    29-April-2016
  * @brief   UART HAL module driver.
  *          This file provides firmware functions to manage the following 
  *          functionalities of the Universal Asynchronous Receiver Transmitter (UART) peripheral:
  *           + Initialization and de-initialization functions
  *           + IO operation functions
  *           + Peripheral Control functions 
  *           + Peripheral State and Errors functions  
  @verbatim
  ==============================================================================
                        ##### How to use this driver #####
  ==============================================================================
  [..]
    The UART HAL driver can be used as follows:
    
    (#) Declare a UART_HandleTypeDef handle structure.

    (#) Initialize the UART low level resources by implementing the HAL_UART_MspInit() API:
        (##) Enable the USARTx interface clock.
        (##) UART pins configuration:
            (+++) Enable the clock for the UART GPIOs.
             (+++) Configure the USART pins (TX as alternate function pull-up, RX as alternate function Input).
        (##) NVIC configuration if you need to use interrupt process (HAL_UART_Transmit_IT()
             and HAL_UART_Receive_IT() APIs):
            (+++) Configure the USARTx interrupt priority.
            (+++) Enable the NVIC USART IRQ handle.
        (##) DMA Configuration if you need to use DMA process (HAL_UART_Transmit_DMA()
             and HAL_UART_Receive_DMA() APIs):
            (+++) Declare a DMA handle structure for the Tx/Rx channel.
            (+++) Enable the DMAx interface clock.
            (+++) Configure the declared DMA handle structure with the required 
                  Tx/Rx parameters.                
            (+++) Configure the DMA Tx/Rx channel.
            (+++) Associate the initialized DMA handle to the UART DMA Tx/Rx handle.
            (+++) Configure the priority and enable the NVIC for the transfer complete 
                  interrupt on the DMA Tx/Rx channel.
            (+++) Configure the USARTx interrupt priority and enable the NVIC USART IRQ handle
                  (used for last byte sending completion detection in DMA non circular mode)

    (#) Program the Baud Rate, Word Length, Stop Bit, Parity, Hardware 
        flow control and Mode(Receiver/Transmitter) in the huart Init structure.

    (#) For the UART asynchronous mode, initialize the UART registers by calling
        the HAL_UART_Init() API.

    (#) For the UART Half duplex mode, initialize the UART registers by calling 
        the HAL_HalfDuplex_Init() API.

    (#) For the LIN mode, initialize the UART registers by calling the HAL_LIN_Init() API.

    (#) For the Multi-Processor mode, initialize the UART registers by calling 
        the HAL_MultiProcessor_Init() API.

     [..] 
       (@) The specific UART interrupts (Transmission complete interrupt, 
            RXNE interrupt and Error Interrupts) will be managed using the macros
            __HAL_UART_ENABLE_IT() and __HAL_UART_DISABLE_IT() inside the transmit 
            and receive process.

     [..] 
       (@) These APIs (HAL_UART_Init() and HAL_HalfDuplex_Init()) configure also the 
            low level Hardware GPIO, CLOCK, CORTEX...etc) by calling the customed 
            HAL_UART_MspInit() API.

     [..] 
        Three operation modes are available within this driver :

     *** Polling mode IO operation ***
     =================================
     [..]    
       (+) Send an amount of data in blocking mode using HAL_UART_Transmit() 
       (+) Receive an amount of data in blocking mode using HAL_UART_Receive()
       
     *** Interrupt mode IO operation ***
     ===================================
     [..]
       (+) Send an amount of data in non blocking mode using HAL_UART_Transmit_IT() 
       (+) At transmission end of transfer HAL_UART_TxCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_TxCpltCallback
       (+) Receive an amount of data in non blocking mode using HAL_UART_Receive_IT() 
       (+) At reception end of transfer HAL_UART_RxCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_RxCpltCallback
       (+) In case of transfer Error, HAL_UART_ErrorCallback() function is executed and user can 
            add his own code by customization of function pointer HAL_UART_ErrorCallback

     *** DMA mode IO operation ***
     ==============================
     [..] 
       (+) Send an amount of data in non blocking mode (DMA) using HAL_UART_Transmit_DMA() 
       (+) At transmission end of half transfer HAL_UART_TxHalfCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_TxHalfCpltCallback 
       (+) At transmission end of transfer HAL_UART_TxCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_TxCpltCallback
       (+) Receive an amount of data in non blocking mode (DMA) using HAL_UART_Receive_DMA() 
       (+) At reception end of half transfer HAL_UART_RxHalfCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_RxHalfCpltCallback 
       (+) At reception end of transfer HAL_UART_RxCpltCallback is executed and user can 
            add his own code by customization of function pointer HAL_UART_RxCpltCallback
       (+) In case of transfer Error, HAL_UART_ErrorCallback() function is executed and user can 
            add his own code by customization of function pointer HAL_UART_ErrorCallback
       (+) Pause the DMA Transfer using HAL_UART_DMAPause()
       (+) Resume the DMA Transfer using HAL_UART_DMAResume()
       (+) Stop the DMA Transfer using HAL_UART_DMAStop()

     *** UART HAL driver macros list ***
     =============================================
     [..]
       Below the list of most used macros in UART HAL driver.

      (+) __HAL_UART_ENABLE: Enable the UART peripheral 
      (+) __HAL_UART_DISABLE: Disable the UART peripheral
      (+) __HAL_UART_GET_FLAG : Check whether the specified UART flag is set or not
      (+) __HAL_UART_CLEAR_FLAG : Clear the specified UART pending flag
      (+) __HAL_UART_ENABLE_IT: Enable the specified UART interrupt
      (+) __HAL_UART_DISABLE_IT: Disable the specified UART interrupt
      (+) __HAL_UART_GET_IT_SOURCE: Check whether the specified UART interrupt has occurred or not

     [..]
       (@) You can refer to the UART HAL driver header file for more useful macros 
      
  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "My_hal_uart_stm32f1xx.h"

/** @addtogroup STM32F1xx_HAL_Driver
  * @{
  */

/** @defgroup UART UART
  * @brief HAL UART module driver
  * @{
  */
#ifdef HAL_UART_MODULE_ENABLED
    
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* External variables --------------------------------------------------------*/

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* function prototypes */
void My_HAL_UART_IRQHandler(UART_HandleTypeDef *huart);

/** @addtogroup UART_Private_Functions   UART Private Functions
  * @{
  */

/**
* @brief This function handles USART1 global interrupt.
*/
void MY_USART1_IRQHandler(void)
{
  HAL_NVIC_ClearPendingIRQ(USART1_IRQn);
  My_HAL_UART_IRQHandler(&huart1);
}

/**
* @brief This function handles USART2 global interrupt.
*/
void MY_USART2_IRQHandler(void)
{
  HAL_NVIC_ClearPendingIRQ(USART2_IRQn);
  My_HAL_UART_IRQHandler(&huart2);
}

STATUS Init_USART_CB(UART_HandleTypeDef *huart, circular_buffer *cb_Rx, circular_buffer *cb_Tx, size_t cb_size_rx, size_t cb_size_tx, size_t data_size)
{
   if((data_size != sizeof(uint8_t)) && (data_size != sizeof(uint16_t)))
   {
      return ERROR;
   }

   if(SUCCESS == cb_init(cb_Rx, cb_size_rx, data_size))
   {
      huart->pRxBuffPtr = (uint8_t *)cb_Rx;
      huart->gState &= ~MY_HAL_UART_STATE_RX_OVERFLOW;
      if(SUCCESS == cb_init(cb_Tx, cb_size_tx, data_size))
      {
        huart->pTxBuffPtr = (uint8_t *)cb_Tx;
        return SUCCESS;
      }
   }

   return ERROR;
}

void Start_USART_Rx_IT(UART_HandleTypeDef *huart)
{
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    /* Check if a transmit process is ongoing or not */

     /* Enable the UART Parity Error Interrupt */
    __HAL_UART_ENABLE_IT(huart, UART_IT_PE);

    /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
    __HAL_UART_ENABLE_IT(huart, UART_IT_ERR);

    /* Enable the UART Data Register not empty Interrupt */
    __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);
}

/**
 * @brief Checks Rx overflow flag and clears it if set
 * @param huart: uart handle
 * @retval zero if overflow flag is not set, otherwise (-1)
 */
int UART_RxDataOverflow(UART_HandleTypeDef *huart)
{
    if(huart->gState & MY_HAL_UART_STATE_RX_OVERFLOW)
    {
        huart->gState &= ~MY_HAL_UART_STATE_RX_OVERFLOW;
        return (-1);
    }

    return 0;
}

static HAL_StatusTypeDef My_UART_Transmit_IT(UART_HandleTypeDef *huart);
static HAL_StatusTypeDef My_UART_Receive_IT(UART_HandleTypeDef *huart);
/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/

/**
  * @brief  Sends an amount of data in non blocking mode.
  * @param  huart: Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
HAL_StatusTypeDef My_HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    uint16_t i;
    circular_buffer* cb;

    cb = (circular_buffer*)(huart->pTxBuffPtr);

    if( !(huart->gState & (HAL_UART_StateTypeDef)MY_HAL_UART_STATE_TX_ACTIVE) )    //  Can't change huart->State type but want to use own enum so type conversion used
    {
      if((pData == NULL ) || (Size == 0))
      {
        return HAL_ERROR;
      }

      if(Size > cb_space_left(cb))
      {
        return HAL_ERROR;
      }

      /* Process Locked */
      __HAL_LOCK(huart);

      for (i=0; i < Size; i++)
      {
         if(SUCCESS != cb_push_back(cb, (pData + i)))
         {
            return HAL_ERROR;
         }
      }

      huart->ErrorCode = HAL_UART_ERROR_NONE;

      /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
      __HAL_UART_ENABLE_IT(huart, UART_IT_ERR);

      /* Process Unlocked */
      __HAL_UNLOCK(huart);

      /* Enable the UART Transmit Data Register Empty Interrupt */
      __HAL_UART_ENABLE_IT(huart, UART_IT_TXE);

      return HAL_OK;
    }
    else
    {
      return HAL_BUSY;
    }

    return HAL_OK;
}

/**
  * @brief  This function handles UART interrupt request.
  * @param  huart: Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
void My_HAL_UART_IRQHandler(UART_HandleTypeDef *huart)
{
  uint32_t tmp_flag = 0, tmp_it_source = 0;

  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_PE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_PE);  
  /* UART parity error interrupt occurred ------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  { 
    huart->ErrorCode |= HAL_UART_ERROR_PE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_FE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
  /* UART frame error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  { 
    huart->ErrorCode |= HAL_UART_ERROR_FE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_NE);
  /* UART noise error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  { 
    huart->ErrorCode |= HAL_UART_ERROR_NE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_ORE);
  /* UART Over-Run interrupt occurred ----------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  { 
    huart->ErrorCode |= HAL_UART_ERROR_ORE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE);
  /* UART in mode Receiver ---------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  { 
    My_UART_Receive_IT(huart);
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_TXE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TXE);
  /* UART in mode Transmitter ------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    My_UART_Transmit_IT(huart);
  }

  if(huart->ErrorCode != HAL_UART_ERROR_NONE)
  {
    /* Clear all the error flag at once */
    __HAL_UART_CLEAR_PEFLAG(huart);
    
    /* Set the UART state ready to be able to start again the process */
    huart->gState = HAL_UART_STATE_READY;
    
    HAL_UART_ErrorCallback(huart);
  }  
}

/** @defgroup UART_Exported_Functions_Group4 Peripheral State and Errors functions 
  *  @brief   UART State and Errors functions 
  *
@verbatim   
  ==============================================================================
                 ##### Peripheral State and Errors functions #####
  ==============================================================================  
 [..]
   This subsection provides a set of functions allowing to return the State of 
   UART communication process, return Peripheral Errors occurred during communication 
   process
   (+) HAL_UART_GetState() API can be helpful to check in run-time the state of the UART peripheral.
   (+) HAL_UART_GetError() check in run-time errors that could be occurred during communication. 

@endverbatim
  * @{
  */

/** @defgroup UART_Private_Functions   UART Private Functions
  *  @brief   UART Private functions 
  * @{
  */

/**
  * @brief  Sends an amount of data in non blocking mode.
  * @param  huart: Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
static HAL_StatusTypeDef My_UART_Transmit_IT(UART_HandleTypeDef *huart)
{
  uint8_t  tmp8;
  uint16_t tmp16;
  circular_buffer* cb;

  cb = (circular_buffer*)(huart->pTxBuffPtr);

  if ((huart->Init.WordLength == UART_WORDLENGTH_9B) && (huart->Init.Parity == UART_PARITY_NONE))
  {
    if(SUCCESS == cb_pop_front(cb, &tmp16))       /* there is data to send */
    {
      huart->Instance->DR = (tmp16 & (uint16_t)0x01FF);
      huart->gState |= MY_HAL_UART_STATE_TX_ACTIVE;
    }
    else
    {
      /* Disable the UART Transmit Data Register Empty Interrupt */
      __HAL_UART_DISABLE_IT(huart, UART_IT_TXE);

      huart->gState &= ~MY_HAL_UART_STATE_TX_ACTIVE;
    }
  }
  else
  {
    if(SUCCESS == cb_pop_front(cb, &tmp8))       /* there is data to send */
    {
      huart->Instance->DR = (tmp8 & (uint8_t)0x00FF);
      huart->gState |= MY_HAL_UART_STATE_TX_ACTIVE;
    }
    else
    {
      /* Disable the UART Transmit Data Register Empty Interrupt */
      __HAL_UART_DISABLE_IT(huart, UART_IT_TXE);

      huart->gState &= ~MY_HAL_UART_STATE_TX_ACTIVE;
    }
  }

  huart->ErrorCode = HAL_UART_ERROR_NONE;

  return HAL_OK;
}


/**
  * @brief  Receives an amount of data in non blocking mode 
  * @param  huart: Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */
static HAL_StatusTypeDef My_UART_Receive_IT(UART_HandleTypeDef *huart)
{
  uint8_t  tmp8;
  uint16_t tmp16;
  circular_buffer* cb;

  cb = (circular_buffer*)(huart->pRxBuffPtr);
  
  if ((huart->Init.WordLength == UART_WORDLENGTH_9B) && (huart->Init.Parity == UART_PARITY_NONE))
  {
    tmp16 = (uint16_t)(huart->Instance->DR & 0x1FF);
    if(SUCCESS != cb_push_back(cb, &tmp16))
    {
      huart->gState |= MY_HAL_UART_STATE_RX_OVERFLOW;
    }
  }
  else
  {
    tmp8 = (uint8_t)(huart->Instance->DR & (uint8_t)0xFF);
    if(SUCCESS != cb_push_back(cb, &tmp8))
    {
      huart->gState |= MY_HAL_UART_STATE_RX_OVERFLOW;
    }
  }

  return HAL_OK;
}

#endif /* HAL_UART_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
