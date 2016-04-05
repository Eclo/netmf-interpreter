////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  Implementation for STM32F4: Copyright (c) Oberon microsystems, Inc.
//
//  *** DA Conversion ***
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <tinyhal.h>

///////////////////////////////////////////////////////////////////////////////

#define DA_CHANNELS 2       // number of channels
#define DA_FIRST_PIN 4      // channel 0 pin (A4)

//--//

DAC_HandleTypeDef DacHandle;

/**
  * @brief DAC MSP De-Initialization 
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO to their default state
  * @param hadc: DAC handle pointer
  * @retval None
  */
void HAL_DAC_MspInit(DAC_HandleTypeDef* hdac)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* DAC Periph clock enable */
    __HAL_RCC_DAC_CLK_ENABLE();
    /* Enable GPIO clock ****************************************/
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*##-2- Configure peripheral GPIO ##########################################*/ 
    /* DAC Channel1 GPIO pin configuration */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
  
/**
  * @brief  DeInitializes the DAC MSP.
  * @param  hdac: pointer to a DAC_HandleTypeDef structure that contains
  *         the configuration information for the specified DAC.  
  * @retval None
  */
void HAL_DAC_MspDeInit(DAC_HandleTypeDef* hdac)
{
  static DMA_HandleTypeDef  hdma_dac1;
  
  /*##-1- Reset peripherals ##################################################*/
  __HAL_RCC_DAC_FORCE_RESET();
  __HAL_RCC_DAC_RELEASE_RESET();

  /*##-2- Disable peripherals and GPIO Clocks ################################*/
  /* De-initialize the DAC Channel1 GPIO pin */
  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4);
}

BOOL DA_Initialize(DA_CHANNEL channel, INT32 precisionInBits)
{
    DAC_ChannelConfTypeDef sConfig;
    
    if (precisionInBits != 12) return FALSE;

    // FIXME *##-1- Configure the DAC peripheral #######################################*/
    DacHandle.Instance = DAC;
    /*##-1- Initialize the DAC peripheral ######################################*/
    if(HAL_DAC_Init(&DacHandle) != HAL_OK)
    {
        /* Initialization Error */
        return FALSE;
    }

    /*##-2- Configure DAC channel1 #############################################*/
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;

    if (channel) 
    {
        // enable channel 2
        if(HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DAC_CHANNEL_2) != HAL_OK)
        {
            /* Channel configuration Error */
            return FALSE;
        }
        /*##-3- Set DAC Channel1 DHR register ######################################*/
        if (HAL_DAC_SetValue(&DacHandle, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 0xFF) != HAL_OK)
        {
            /* Setting value Error */
            return FALSE;
        }
        /*##-4- Enable DAC Channel ################################################*/
        if (HAL_DAC_Start(&DacHandle, DAC_CHANNEL_2) != HAL_OK)
        {
            /* Start Error */
            return FALSE;
        }
    } 
    else 
    {
        // enable channel 1
        if(HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DAC_CHANNEL_1) != HAL_OK)
        {
            /* Channel configuration Error */
            return FALSE;
        }
        /*##-3- Set DAC Channel1 DHR register ######################################*/
        if (HAL_DAC_SetValue(&DacHandle, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0xFF) != HAL_OK)
        {
            /* Setting value Error */
            return FALSE;
        }
        /*##-4- Enable DAC Channel ################################################*/
        if (HAL_DAC_Start(&DacHandle, DAC_CHANNEL_1) != HAL_OK)
        {
            /* Start Error */
            return FALSE;
        }
    }

    // // enable DA clock
    // RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    // // set pin as analog
    // CPU_GPIO_DisablePin(DA_FIRST_PIN + channel, RESISTOR_DISABLED, 0, GPIO_ALT_MODE_1);
    // if (channel) {
    //     DAC->CR |= DAC_CR_EN2; // enable channel 2
    // } else {
    //     DAC->CR |= DAC_CR_EN1; // enable channel 1
    // }
    return TRUE;
}

void DA_Uninitialize(DA_CHANNEL channel)
{
    // FIXME
    HAL_DAC_DeInit(&DacHandle);
    
    // if (channel) {
    //     DAC->CR &= ~DAC_CR_EN2; // disable channel 2
    // } else {
    //     DAC->CR &= ~DAC_CR_EN1; // disable channel 1
    // }
    // // free pin
    // CPU_GPIO_DisablePin(DA_FIRST_PIN + channel, RESISTOR_DISABLED, 0, GPIO_ALT_PRIMARY);
    // if ((DAC->CR & (DAC_CR_EN1 | DAC_CR_EN2)) == 0) { // all channels off
    //     // disable DA clock
    //     RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
    // }
}

// level is a 12 bit value
void DA_Write(DA_CHANNEL channel, INT32 level)
{
    if (channel) 
    {
        DAC->DHR12R2 = level;
    } 
    else 
    {
        DAC->DHR12R1 = level;
    }
}

UINT32 DA_DAChannels()
{
    return DA_CHANNELS;
}

GPIO_PIN DA_GetPinForChannel(DA_CHANNEL channel)
{
    if ((UINT32)channel >= DA_CHANNELS) 
    {
        return GPIO_PIN_NONE;
    }
    return DA_FIRST_PIN + channel;
}

BOOL DA_GetAvailablePrecisionsForChannel(DA_CHANNEL channel, INT32* precisions, UINT32& size)
{
    size = 0;
    if (precisions == NULL || (UINT32)channel >= DA_CHANNELS) 
    {
        return FALSE;
    }
    precisions[0] = 12;
    size = 1;
    return TRUE;
}
