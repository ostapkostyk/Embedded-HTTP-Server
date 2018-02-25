/**
  ******************************************************************************
  * @file    EEPROM/EEPROM_Emulation/inc/eeprom.h
  * @author  MCD Application Team
  * @edited by Ostap Kostyk
  * @version V1.0.1
  * @brief   This file contains all the functions prototypes for the EEPROM 
  *          emulation firmware library.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EEPROM_H
#define __EEPROM_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/***********************************************
 *    Memory Configuration
 ***********************************************/
/* Define the size of the sectors to be used */
#define PAGE_SIZE               ((uint32_t)0x0400)  /* Page size = 1KByte */

/* EEPROM start address in Flash */
#define EEPROM_START_ADDRESS  ((uint32_t)0x0800F800) /* EEPROM emulation start address:
                                                  from sector2 : before 2K from the end of
                                                  Flash memory */

/**********************************************
 *    Application Specific Configuration
 **********************************************/

#define EE_STRING1_LEN  (10*2)

/* Size of each element MUST be multiple to uint16_t */
typedef struct
{
  uint16_t MyNum16;
  char MyString[EE_STRING1_LEN];
  uint32_t MyNum32;
}EE_Data_t;

/**********************************************/
/*    END OF CONFIGURATION                    */
/**********************************************/

/* Exported constants --------------------------------------------------------*/
/* EEPROM emulation firmware error codes */
#define EE_OK      (uint32_t)HAL_OK
#define EE_ERROR   (uint32_t)HAL_ERROR
#define EE_BUSY    (uint32_t)HAL_BUSY
#define EE_TIMEOUT (uint32_t)HAL_TIMEOUT

/* Device voltage range supposed to be [2.7V to 3.6V], the operation will
   be done by word  */
//#define VOLTAGE_RANGE           (uint8_t)VOLTAGE_RANGE_3

/* Pages 0 and 1 base and end addresses */
#define PAGE0_BASE_ADDRESS    ((uint32_t)(EEPROM_START_ADDRESS + 0x0000))
#define PAGE0_END_ADDRESS     ((uint32_t)(EEPROM_START_ADDRESS + (PAGE_SIZE - 1)))

#define PAGE1_BASE_ADDRESS    ((uint32_t)(EEPROM_START_ADDRESS + PAGE_SIZE))
#define PAGE1_END_ADDRESS     ((uint32_t)(EEPROM_START_ADDRESS + (2 * PAGE_SIZE - 1)))

/* Used Flash pages for EEPROM emulation */
#define PAGE0                 ((uint16_t)0x0000)
#define PAGE1                 ((uint16_t)0x0001) /* Page nb between PAGE0_BASE_ADDRESS & PAGE1_BASE_ADDRESS*/

/* No valid page define */
#define NO_VALID_PAGE         ((uint16_t)0x00AB)

/* Page status definitions */
#define ERASED                ((uint16_t)0xFFFF)     /* Page is empty */
#define RECEIVE_DATA          ((uint16_t)0xEEEE)     /* Page is marked to receive data */
#define VALID_PAGE            ((uint16_t)0x0000)     /* Page containing valid data */

/* Valid pages in read and write defines */
#define READ_FROM_VALID_PAGE  ((uint8_t)0x00)
#define WRITE_IN_VALID_PAGE   ((uint8_t)0x01)

/* Page full define */
#define PAGE_FULL             ((uint8_t)0x80)

/* Variables' number */
//#define NB_OF_VAR             ((uint8_t)0x03)
#define NB_OF_VAR               (sizeof(EE_Data_t) / sizeof(uint16_t))

/* Exported types ------------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/* To be called first */
void EE_AddrInit(void);

/*
@retval: HAL_StatusTypeDef (HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT)
 * */
uint16_t EE_Init(void);

uint16_t EE_InitData(void);

uint8_t EE_ReadElem(uint16_t* pAddr, size_t Size);

uint8_t EE_WriteElem(uint16_t* pAddr, size_t Size);

/*
@retval: 0 - OK, 1 - Var not found, NO_VALID_PAGE
 * */
uint16_t EE_ReadVariable(uint16_t VirtAddress, uint16_t* Data);

/*
 *
@retval: NO_VALID_PAGE or PAGE_FULL or HAL_StatusTypeDef
 * */
uint16_t EE_WriteVariable(uint16_t VirtAddress, uint16_t Data);

/* External Variables */

/* Virtual address defined by the user: 0xFFFF value is prohibited */
extern uint16_t VirtAddVarTab[NB_OF_VAR];

extern EE_Data_t EE_Data;

/**********************************************************************************************************
 *      EXAMPLE OF USAGE IN APPLICATION
 **********************************************************************************************************/
#if 0
//----------------------------
//  VARIABLES DECLARATION:
//----------------------------
/* EEPROM Emulation */
/* Virtual address defined by the user: 0xFFFF value is prohibited */
EE_Data_t EE_Data;
uint16_t VirtAddVarTab[NB_OF_VAR];
uint16_t EE_Status;

//----------------------------
// ONE_TIME INITIALIZATION
//----------------------------
/* EEPROM EMULATION-RELATED */
EE_AddrInit();
/* Unlock the Flash Program Erase controller */
HAL_FLASH_Unlock();

EE_Status = EE_Init();    //  Init EEPROM Emulation

EE_Status = EE_InitData();
if(EE_Status) { debug_print("EE Data Init ERROR:%x\r\n", EE_Status); }

/* END OF EEPROM EMULATION-RELATED */

//-----------------------------
//  Read-Write Operations
//-----------------------------
/* Example of EEPROM Emulation usage */
EE_Status = EE_ReadElem((uint16_t*)&EE_Data.MyNum32, sizeof(EE_Data.MyNum32));
if(EE_Status) { debug_print("EE Read Num ERROR: %x\r\n", EE_Status); }

EE_Status = EE_ReadElem((uint16_t*)&EE_Data.MyString, sizeof(EE_Data.MyString));
if(EE_Status) { debug_print("EE Read Str ERROR: %x\r\n", EE_Status); }
else          { debug_print("EE String:%s", EE_Data.MyString); }

EE_Data.MyNum32++;
snprintf(EE_Data.MyString, sizeof(EE_Data.MyString)-1, "This is Start#%d", (int)EE_Data.MyNum32);

EE_Status = EE_WriteElem((uint16_t*)&EE_Data.MyNum32, sizeof(EE_Data.MyNum32));
if(EE_Status) { debug_print("EE Write ERROR: %x\r\n", EE_Status); }

EE_Status = EE_WriteElem((uint16_t*)&EE_Data.MyString, sizeof(EE_Data.MyString));
if(EE_Status) { debug_print("EE WriteStr ERROR: %x\r\n", EE_Status); }

#endif



#endif /* __EEPROM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
