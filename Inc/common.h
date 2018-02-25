#ifndef _COMMON_H_
#define _COMMON_H_

#include "config.h"
#include "User_typedefs.h"
#include "memmgr.h"
#include "debug.h"

// HW dependent definitions:
#define PORT_DEF         GPIO_TypeDef*      //  Port type definition
#define PIN_NUM_DEF      uint16_t           //  Pin number type definition
#define PIN_STATE_DEF    GPIO_PinState      //  Pin state type definition (Active or Passive, Set or Reset etc)
#define PIN_STATE_HIGH   GPIO_PIN_SET       //  definition to set the pin to high level
#define PIN_STATE_LOW    GPIO_PIN_RESET     //  definition to set the pin to low level
// End Of HW dependent definitions

#endif
