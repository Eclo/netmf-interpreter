////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  Implementation for STM32F4: Copyright (c) Oberon microsystems, Inc.
//
//  *** AD Conversion ***
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <tinyhal.h>

//--//


#define AD_SAMPLE_TIME 2   // sample time = 28 cycles

#if USER_ADC == 1
    #define ADCx ADC1
    #define RCC_APB2ENR_ADCxEN RCC_APB2ENR_ADC1EN
    // ADC1 pins plus two internally connected channels thus the 0 for 'no pin'
    // Vsense for temperature sensor @ ADC1_IN16
    // Vrefubt for internal voltage reference (1.21V) @ ADC1_IN17
    // to access the internal channels need to include '16' and/or '17' at the AD_CHANNELS array in 'platform_selector.h' 
    #define ADC_PINS {0,1,2,3,4,5,6,7,16,17,32,33,34,35,36,37,0,0} 
#elif USER_ADC == 3
    #define ADCx ADC3
    #define RCC_APB2ENR_ADCxEN RCC_APB2ENR_ADC3EN
    #define ADC_PINS {0,1,2,3,86,87,88,89,90,83,32,33,34,35,84,85} // ADC3 pins
#else
    #error "wrong USER_ADC value (1 or 3). Please go to platform_selector.h (in solution folder) and re-define USER_ADC"
#endif

// Channels
static const BYTE g_AD_Channel[] = AD_CHANNELS;
static const uint32_t g_ADC_Channel[] = ADC_CHANNELS;
static const BYTE g_AD_Pins[] = ADC_PINS;
#define AD_NUM ARRAYSIZE_CONST_EXPR(g_AD_Channel)  // number of channels

// ADC handler declaration
ADC_HandleTypeDef AdcHandle;


//--//

/**
  * @brief ADC MSP Initialization 
  *        This function configures the hardware resources used in this example: 
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration  
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  GPIO_InitTypeDef          GPIO_InitStruct;
  static DMA_HandleTypeDef  hdma_adc;
  
  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO clock */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  /* ADC3 Periph clock enable */
  __HAL_RCC_ADC1_CLK_ENABLE();
  /* Enable DMA2 clock */
  __HAL_RCC_DMA2_CLK_ENABLE(); 
  
  /*##-2- Configure peripheral GPIO ##########################################*/ 
  /* ADC3 Channel8 GPIO pin configuration */
  GPIO_InitStruct.Pin = GPIO_PIN_0; // FIXME
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /*##-3- Configure the DMA streams ##########################################*/
  /* Set the parameters to be configured */
  hdma_adc.Instance = DMA2_Stream0;
  
  hdma_adc.Init.Channel  = DMA_CHANNEL_0; // FIXME
  hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc.Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_adc.Init.Mode = DMA_CIRCULAR;
  hdma_adc.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_adc.Init.FIFOMode = DMA_FIFOMODE_DISABLE;         
  hdma_adc.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
  hdma_adc.Init.MemBurst = DMA_MBURST_SINGLE;
  hdma_adc.Init.PeriphBurst = DMA_PBURST_SINGLE; 

  HAL_DMA_Init(&hdma_adc);
    
  /* Associate the initialized DMA handle to the the ADC handle */
  __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc);

  /*##-4- Configure the NVIC for DMA #########################################*/
  /* NVIC configuration for DMA transfer complete interrupt */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);   
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}
  
/**
  * @brief ADC MSP De-Initialization 
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO to their default state
  * @param hadc: ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
  static DMA_HandleTypeDef  hdma_adc;
  
  /*##-1- Reset peripherals ##################################################*/
  __HAL_RCC_ADC_FORCE_RESET();
  __HAL_RCC_ADC_RELEASE_RESET();

  /*##-2- Disable peripherals and GPIO Clocks ################################*/
  /* De-initialize the ADC3 Channel8 GPIO pin */
  HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0);
  
  /*##-3- Disable the DMA Streams ############################################*/
  /* De-Initialize the DMA Stream associate to transmission process */
  HAL_DMA_DeInit(&hdma_adc); 
    
  /*##-4- Disable the NVIC for DMA ###########################################*/
  HAL_NVIC_DisableIRQ(DMA2_Stream0_IRQn);
}

BOOL AD_Initialize(ANALOG_CHANNEL channel, INT32 precisionInBits)
{
    ADC_ChannelConfTypeDef sConfig;
    
    int chNum = g_AD_Channel[channel];

    // init this channel if it's listed in the AD_CHANNELS array
    // FIXME
    /*##-1- Configure the ADC peripheral #######################################*/
    AdcHandle.Instance = ADCx;

    AdcHandle.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
    AdcHandle.Init.Resolution = ADC_RESOLUTION_12B;
    AdcHandle.Init.ScanConvMode = DISABLE;
    AdcHandle.Init.ContinuousConvMode = ENABLE;
    AdcHandle.Init.DiscontinuousConvMode = DISABLE;
    AdcHandle.Init.NbrOfDiscConversion = 0;
    AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    AdcHandle.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
    AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    AdcHandle.Init.NbrOfConversion = 1;
    AdcHandle.Init.DMAContinuousRequests = ENABLE;
    AdcHandle.Init.EOCSelection = DISABLE;
        
    if(HAL_ADC_Init(&AdcHandle) != HAL_OK)
    {
        // channel not available
        return FALSE;
    }
    
    /*##-2- Configure ADC regular channel ######################################*/
    /* Note: Considering IT occurring after each number of size of              */
    /*       "uhADCxConvertedValue"  ADC conversions (IT by DMA end             */
    /*       of transfer), select sampling time and ADC clock with sufficient   */
    /*       duration to not create an overhead situation in IRQHandler.        */  
    sConfig.Channel = g_ADC_Channel[chNum];//ADCx_CHANNEL;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
    sConfig.Offset = 0;

    if(HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
    {
        // channel not available
        return FALSE;
    }
    return TRUE;

    // FIXME
    // /*##-3- Start the conversion process and enable interrupt ##################*/
    // /* Note: Considering IT occurring after each number of ADC conversions      */
    // /*       (IT by DMA end of transfer), select sampling time and ADC clock    */
    // /*       with sufficient duration to not create an overhead situation in    */
    // /*        IRQHandler. */ 
    // if(HAL_ADC_Start_DMA(&AdcHandle, (uint32_t*)&uhADCxConvertedValue, 1) != HAL_OK)
    // {
    //     // channel not available
    //     return FALSE;
    // }

    // for (int i = 0; i < AD_NUM ; i++) 
    // {
    //     if (g_AD_Channel[i] == chNum) 
    //     {
    //         // valid channel
    //         if (!(RCC->APB2ENR & RCC_APB2ENR_ADCxEN)) { // not yet initialized
    //             RCC->APB2ENR |= RCC_APB2ENR_ADCxEN; // enable AD clock
    //             ADC->CCR = 0; // ADCCLK = PB2CLK / 2;
    //             ADCx->SQR1 = 0; // 1 conversion
    //             ADCx->CR1 = 0;
    //             ADCx->CR2 = ADC_CR2_ADON; // AD on
    //             ADCx->SMPR1 = 0x01249249 * AD_SAMPLE_TIME;
    //             ADCx->SMPR2 = 0x09249249 * AD_SAMPLE_TIME;
    //         }
            
    //         // set pin as analog input if channel is not one of the internally connected
    //         if(chNum <= 15) {
    //             CPU_GPIO_DisablePin(AD_GetPinForChannel(channel), RESISTOR_DISABLED, 0, GPIO_ALT_MODE_1);
    //             return TRUE;        
    //         }
    //     }
    // }
	
    // // channel not available
    // return FALSE;
}

void AD_Uninitialize(ANALOG_CHANNEL channel)
{
    // FIXME
    HAL_ADC_DeInit(&AdcHandle);
    
    // int chNum = g_AD_Channel[channel];

    // // free GPIO pin if this channel is listed in the AD_CHANNELS array 
    // // and if it's not one of the internally connected ones as these channels don't take any GPIO pins
    // if(chNum <= 15) {
    //     CPU_GPIO_DisablePin(AD_GetPinForChannel(channel), RESISTOR_DISABLED, 0, GPIO_ALT_PRIMARY);
    // }
}

INT32 AD_Read(ANALOG_CHANNEL channel)
{
    int chNum = g_AD_Channel[channel];
  
    // check if this channel is listed in the AD_CHANNELS array
    for (int i = 0; i < AD_NUM ; i++) {
        if (g_AD_Channel[i] == chNum) {
            // valid channel
            int x = ADCx->DR; // clear EOC flag

            ADCx->SQR3 = chNum; // select channel
        
            // need to enable internal reference at ADC->CCR register to work with internally connected channels 
            if(chNum == 16 || chNum == 17) 
            {
                ADC->CCR |= ADC_CCR_TSVREFE; // Enable internal reference to work with temperature sensor and VREFINT channels
            }
    
            ADCx->CR2 |= ADC_CR2_SWSTART; // start AD
            while (!(ADCx->SR & ADC_SR_EOC)); // wait for completion
    
            // disable internally reference
            if(chNum == 16 || chNum == 17) {
                ADC->CCR &= ~ADC_CCR_TSVREFE; 
            }
    
            return ADCx->DR; // read result
        }
    }

    // channel not available
    return 0;
}

UINT32 AD_ADChannels()
{
    return AD_NUM;
}

GPIO_PIN AD_GetPinForChannel(ANALOG_CHANNEL channel)
{
    // return GPIO pin
    // for internally connected channels this is GPIO_PIN_NONE as these don't take any GPIO pins
    int chNum = g_AD_Channel[channel];

    for (int i = 0; i < AD_NUM ; i++) {
        if (g_AD_Channel[i] == chNum) {
            return (GPIO_PIN)g_AD_Pins[chNum];
        }
    }

    // channel not available
    return GPIO_PIN_NONE;
}

BOOL AD_GetAvailablePrecisionsForChannel(ANALOG_CHANNEL channel, INT32* precisions, UINT32& size)
{
    int chNum = g_AD_Channel[channel];

    // check if this channel is listed in the AD_CHANNELS array
    for (int i = 0; i < AD_NUM ; i++) {
        if (g_AD_Channel[i] == chNum) {
            precisions[0] = 12;
            size = 1;
            return TRUE;
        }
    }

    // channel not available
    size = 0;
    return FALSE;
}
