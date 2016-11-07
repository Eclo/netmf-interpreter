//////////////////////////////////////////////////////////////////////////////////////////
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
//  Copyright (c) Microsoft Open Technologies. All rights reserved.
//  Implementation for CMSIS: Copyright (c) Eclo Solutions Lda
//
//  *** Interface for Cortex-M CRC Calculation Unit ***
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <tinyhal.h>

#define CRC_BASE            (AHB1PERIPH_BASE + 0x3000U)
#define CRC                 ((CRC_TypeDef *) CRC_BASE)

typedef struct
{
  CRC_TypeDef                 *Instance;  /*!< Register base address   */
}CRC_HandleTypeDef;


/*
    Interface to Cortex-M CRC calculation unit that implements the equivalent to the software implementation at Support\CRC project.
    CRC-32 (Ethernet) polynomial: 0x4C11DB7.
    buffer: pointer to the region block to be CRCed
    size: lenght of buffer to compute CRC
    crc: previous CRC value to start CRC computing
*/

/* NOTES: 
    - should be good for F1, L1, F2 and F4 units series, other have diferent configurations and polynomial coefficients
    - can only handle buffer sizes multiple of 4 otherwise the calculation is incorrect
    - can not handle initial CRC values, only 0 is admited
    - all the above can be fixed with more code so feel free to jump in ;)     
*/


UINT32 SUPPORT_ComputeCRC(const void* buffer, int size, UINT32 initCrc)
{
    CRC_HandleTypeDef hcrc;
    
    uint32_t index = 0U;
    uint32_t arg1;
    UINT32 crc;
  
    // anything to do here?
    if(size == 0)
    {
        return initCrc;
    }
    
    // init CRC unit    
    hcrc.Instance = CRC;

    // Reset CRC Calculation Unit
    (&hcrc)->Instance->CR |= CRC_CR_RESET;
    
    uint8_t* ptr = (uint8_t*)buffer;

    // CRC calculation unit is initialed with 0xFFFFFFFF which is not good for our CRC calculations
    // feeding 0xFFFFFFFF to the calculation unit will set the register to 0x00000000
    
    while(hcrc.Instance->DR != 0x0)
    {
        hcrc.Instance->DR = 0xFFFFFFFF;        
    } 

    // we'll be reading the buffer in steps of 4 bytes, so the size must be recalculated accordingly
    size = size >> 2;

    // Enter Data to the CRC calculator
    for(index = 0U; index < size; index++)
    {
        // take the next 4 bytes as if they were a UINT32
        // because the CRC calculation unit expects the bytes in reverse order, reverse the byte order first 
        arg1 = __REV(*(uint32_t*)(ptr));

        // feed the bytes to the CRC 
        hcrc.Instance->DR = arg1;

        // copy to return value
        crc = (uint32_t)hcrc.Instance->DR;

        // increase pointer by 4 to the next position
        // we are reading UINT32 from a UINT8 pointer 
        ptr +=4;
    }

    // Return the CRC computed value
    return crc;    
}
