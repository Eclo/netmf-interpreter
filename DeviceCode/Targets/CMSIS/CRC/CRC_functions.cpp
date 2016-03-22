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

/*
    Interface to Cortex-M CRC calculation unit that implements the equivalent to the software implementation in Support\CRC project.
    CRC-32 (Ethernet) polynomial: 0x4C11DB7.
    rgBlock: pointer to the region block to be CRCed
    nLength: lenght of rgBlock to compute CRC
    crc: previous CRC value to start CRC computing
*/
UINT32 SUPPORT_ComputeCRC(const void* rgBlock, int nLength, UINT32 crc)
{
    CRC_HandleTypeDef hcrc;
    uint32_t index = 0U;

    // init CRC unit    
    hcrc.Instance = CRC;
    HAL_CRC_Init(&hcrc);

    // Process Locked
    __HAL_LOCK(&hcrc); 

    // Change CRC peripheral state
    hcrc.State = HAL_CRC_STATE_BUSY;

    // if crc (argument for initial value) is not 0, init the CRC calculation unit with this value
    if(crc != 0)
    {
        // init CRC value
        hcrc.Instance->DR = crc;      
    }

    const UINT8* ptr = (const UINT8*)rgBlock;

    // Enter Data to the CRC calculator
    for(index = 0U; index < nLength; index++)
    {
        hcrc.Instance->DR = (uint32_t)*ptr++;
    }

    // Change CRC peripheral state to ready
    hcrc.State = HAL_CRC_STATE_READY;

    // Process Unlocked
    __HAL_UNLOCK(&hcrc);

    // Return the CRC computed value
    return hcrc.Instance->DR;    
}
