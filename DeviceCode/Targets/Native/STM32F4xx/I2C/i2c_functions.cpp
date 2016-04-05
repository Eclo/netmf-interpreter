////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  Implementation for CMSIS: Copyright (c) Eclo Solutions Lda. Based in original code from Oberon microsystems, Inc.
//
//  *** I2C Driver ***
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <tinyhal.h> 
//////////////////////////////////////
// to simplify, becasue NETMF only use a single I2C1 port
//////////////////////////////////////
// #if I2C_PORT == 2
//     #define I2Cx                   I2C2 
//     #define I2Cx_EV_IRQn           I2C2_EV_IRQn
//     #define I2Cx_ER_IRQn           I2C2_ER_IRQn
//     #define RCC_APB1ENR_I2CxEN     RCC_APB1ENR_I2C2EN
//     #define RCC_APB1RSTR_I2CxRST   RCC_APB1RSTR_I2C2RST
//     #if !defined(I2C_SCL_PIN)
//         #define I2C_SCL_PIN  26 // PB10
//     #endif
//     #if !defined(I2C_SDA_PIN)
//         #define I2C_SDA_PIN  27 // PB11
//     #endif
// #elif I2C_PORT == 3
//     #define I2Cx                   I2C3 
//     #define I2Cx_EV_IRQn           I2C3_EV_IRQn
//     #define I2Cx_ER_IRQn           I2C3_ER_IRQn
//     #define RCC_APB1ENR_I2CxEN     RCC_APB1ENR_I2C3EN
//     #define RCC_APB1RSTR_I2CxRST   RCC_APB1RSTR_I2C3RST
//     #if !defined(I2C_SCL_PIN)
//         #define I2C_SCL_PIN  8 // PA8
//     #endif
//     #if !defined(I2C_SDA_PIN)
//         #define I2C_SDA_PIN  41 // PC9
//     #endif
// #else // use I2C1 by default
        #define I2Cx                   I2C1 
    #define I2Cx_EV_IRQn           I2C1_EV_IRQn
    #define I2Cx_ER_IRQn           I2C1_ER_IRQn
    #define RCC_APB1ENR_I2CxEN     RCC_APB1ENR_I2C1EN
    #define RCC_APB1RSTR_I2CxRST   RCC_APB1RSTR_I2C1RST
    #if !defined(I2C_SCL_PIN)
        #define I2C_SCL_PIN  22 // PB6
    #endif
    #if !defined(I2C_SDA_PIN)
        #define I2C_SDA_PIN  23 // PB7
    #endif
// #endif

static I2C_HAL_XACTION* currentI2CXAction;
static I2C_HAL_XACTION_UNIT* currentI2CUnit;

/**
  * @brief  This function handles I2C event interrupt request.  
  * @param  None
  * @retval None
  * @Note   This function is redefined in "main.h" and related to I2C data transmission     
  */
void I2C1_EV_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(& I2cHandle);
}

/**
  * @brief  This function handles I2C error interrupt request.
  * @param  None
  * @retval None
  * @Note   This function is redefined in "main.h" and related to I2C error
  */
void I2C1_ER_IRQHandler(void)
{
    HAL_I2C_ER_IRQHandler(& I2cHandle);
}

/**
  * @brief  Master Tx Transfer completed callbacks.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *         the configuration information for I2C module
  * @retval None
  */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
}

/**
  * @brief  Master Rx Transfer completed callbacks.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *         the configuration information for I2C module
  * @retval None
  */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
}

/**
  * @brief  I2C error callbacks.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *         the configuration information for I2C module
  * @retval None
  */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
  GPIO_InitTypeDef  GPIO_InitStruct;
  
  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  /* Enable I2C1 clock */
  __HAL_RCC_I2C1_CLK_ENABLE(); 
  
  /*##-2- Configure peripheral GPIO ##########################################*/  
  /* I2C TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = I2C_SCL_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
  /* I2C RX GPIO pin configuration  */
  GPIO_InitStruct.Pin = I2C_SDA_PIN;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
  /*##-3- Configure the NVIC for I2C #########################################*/   
  /* NVIC for I2C1 */
  HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
  HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 2);
  HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
  /*##-1- Reset peripherals ##################################################*/
  __HAL_RCC_I2C1_FORCE_RESET();
  __HAL_RCC_I2C1_RELEASE_RESET();

  /*##-2- Disable peripherals and GPIO Clocks ################################*/
  /* Configure I2C Tx as alternate function  */
  HAL_GPIO_DeInit(GPIOB, I2C_SCL_PIN);
  /* Configure I2C Rx as alternate function  */
  HAL_GPIO_DeInit(GPIOB, I2C_SDA_PIN);
  
  /*##-3- Disable the NVIC for I2C ###########################################*/
  HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
  HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
}

void I2C_ER_Interrupt (void* param) // Error Interrupt Handler
{
    INTERRUPT_START
    
    // pre:
    // I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF | I2C_SR1_TIMEOUT
    
    I2C_HAL_XACTION* xAction = currentI2CXAction;
    
    I2Cx->SR1 = 0; // reset errors
    xAction->Signal(I2C_HAL_XACTION::c_Status_Aborted); // calls XActionStop()
    
    INTERRUPT_END
}
    
void I2C_EV_Interrupt (void* param) // Event Interrupt Handler
{
    INTERRUPT_START
    
    // pre:
    // I2C_SR1_SB | I2C_SR1_ADDR | I2C_SR1_BTF | I2C_CR2_ITBUFEN & (I2C_SR1_RXNE | I2C_SR1_TXE)
    
    I2C_HAL_XACTION* xAction = currentI2CXAction;
    I2C_HAL_XACTION_UNIT* unit = currentI2CUnit;
    
    int todo = unit->m_bytesToTransfer;
    int sr1 = I2Cx->SR1;  // read status register
    int sr2 = I2Cx->SR2;  // clear ADDR bit
    int cr1 = I2Cx->CR1;  // initial control register
    
    if (unit->IsReadXActionUnit()) { // read transaction
        if (sr1 & I2C_SR1_SB) { // start bit
            if (todo == 1) {
                I2Cx->CR1 = (cr1 &= ~I2C_CR1_ACK); // last byte nack
            } else if (todo == 2) {
                I2Cx->CR1 = (cr1 |= I2C_CR1_POS); // prepare 2nd byte nack
            }
            UINT8 addr = xAction->m_address << 1; // address bits
            I2Cx->DR = addr + 1; // send header byte with read bit;
        } else {
            if (sr1 & I2C_SR1_ADDR) { // address sent
                if (todo == 1) {
                    I2Cx->CR1 = (cr1 |= I2C_CR1_STOP); // send stop after single byte
                } else if (todo == 2) {
                    I2Cx->CR1 = (cr1 &= ~I2C_CR1_ACK); // last byte nack
                }
            } else {
                while (sr1 & I2C_SR1_RXNE) { // data available
                    if (todo == 2) { // 2 bytes remaining
                        I2Cx->CR1 = (cr1 |= I2C_CR1_STOP); // stop after last byte
                    } else if (todo == 3) { // 3 bytes remaining
                        if (!(sr1 & I2C_SR1_BTF)) break; // assure 2 bytes are received
                        I2Cx->CR1 = (cr1 &= ~I2C_CR1_ACK); // last byte nack
                    }
                    UINT8 data = I2Cx->DR; // read data
                    *(unit->m_dataQueue.Push()) = data; // save data
                    unit->m_bytesTransferred++;
                    unit->m_bytesToTransfer = --todo; // update todo
                    sr1 = I2Cx->SR1;  // update status register copy
                }
            }
            if (todo == 1) {
                I2Cx->CR2 |= I2C_CR2_ITBUFEN; // enable I2C_SR1_RXNE interrupt
            }
        }
    } else { // write transaction
        if (sr1 & I2C_SR1_SB) { // start bit
            UINT8 addr = xAction->m_address << 1; // address bits
            I2Cx->DR = addr; // send header byte with write bit;
        } else {
            while (todo && (sr1 & I2C_SR1_TXE)) {
                I2Cx->DR = *(unit->m_dataQueue.Pop()); // next data byte;
                unit->m_bytesTransferred++;
                unit->m_bytesToTransfer = --todo; // update todo
                sr1 = I2Cx->SR1;  // update status register copy
            }
            if (!(sr1 & I2C_SR1_BTF)) todo++; // last byte not yet sent
        }
        
    }
    
    if (todo == 0) { // all received or all sent
        if (!xAction->ProcessingLastUnit()) { // start next unit
            I2Cx->CR2 &= ~I2C_CR2_ITBUFEN; // disable I2C_SR1_RXNE interrupt
            currentI2CUnit = xAction->m_xActionUnits[ xAction->m_current++ ];
            I2Cx->CR1 = I2C_CR1_PE | I2C_CR1_START | I2C_CR1_ACK; // send restart
        } else {
            xAction->Signal(I2C_HAL_XACTION::c_Status_Completed); // calls XActionStop()
        }
    }
    
    INTERRUPT_END
}


BOOL I2C_Internal_Initialize()
{
    // NATIVE_PROFILE_HAL_PROCESSOR_I2C();
    
    // if (!(RCC->APB1ENR & RCC_APB1ENR_I2CxEN)) { // only once
    //     currentI2CXAction = NULL;
    //     currentI2CUnit = NULL;
        
    //     // set pins to AF4 and open drain
    //     CPU_GPIO_DisablePin( I2C_SDA_PIN, RESISTOR_PULLUP, 0, (GPIO_ALT_MODE)0x43 );
    //     CPU_GPIO_DisablePin( I2C_SCL_PIN, RESISTOR_PULLUP, 0, (GPIO_ALT_MODE)0x43 );
        
    //     RCC->APB1ENR |= RCC_APB1ENR_I2CxEN; // enable I2C clock
    //     RCC->APB1RSTR = RCC_APB1RSTR_I2CxRST; // reset I2C peripheral
    //     RCC->APB1RSTR = 0;
        
    //     I2Cx->CR2 = SYSTEM_APB1_CLOCK_HZ / 1000000; // APB1 clock in MHz
    //     I2Cx->CCR = (SYSTEM_APB1_CLOCK_HZ / 1000 / 2 - 1) / 100 + 1; // 100KHz
    //     I2Cx->TRISE = SYSTEM_APB1_CLOCK_HZ / (1000 * 1000) + 1; // 1ns;
    //     I2Cx->OAR1 = 0x4000; // init address register
        
    //     I2Cx->CR1 = I2C_CR1_PE; // enable peripheral
        
    //     CPU_INTC_ActivateInterrupt(I2Cx_EV_IRQn, I2C_EV_Interrupt, 0);
    //     CPU_INTC_ActivateInterrupt(I2Cx_ER_IRQn, I2C_ER_Interrupt, 0);
    // }
    
    return TRUE;
}

BOOL I2C_Internal_Uninitialize()
{
    // NATIVE_PROFILE_HAL_PROCESSOR_I2C();
    
    // CPU_INTC_DeactivateInterrupt(I2Cx_EV_IRQn);
    // CPU_INTC_DeactivateInterrupt(I2Cx_ER_IRQn);
    // I2Cx->CR1 = 0; // disable peripheral
    // RCC->APB1ENR &= ~RCC_APB1ENR_I2CxEN; // disable I2C clock
    
    return TRUE;
}

void I2C_Internal_XActionStart( I2C_HAL_XACTION* xAction, bool repeatedStart )
{
    NATIVE_PROFILE_HAL_PROCESSOR_I2C();

    // I2C handler declaration
    I2C_HandleTypeDef I2cHandle;

    /*##-1- Configure the I2C peripheral ######################################*/
    I2cHandle.Instance             = I2Cx;

    I2cHandle.Init.AddressingMode  = I2C_ADDRESSINGMODE_10BIT;
    I2cHandle.Init.ClockSpeed      = xAction->m_clockRate + (xAction->m_clockRate2 << 8);//400000;
    I2cHandle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    I2cHandle.Init.DutyCycle       = I2C_DUTYCYCLE_16_9;
    I2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    I2cHandle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    I2cHandle.Init.OwnAddress1     = xAction->m_address;//I2C_ADDRESS;
    I2cHandle.Init.OwnAddress2     = 0xFE;

    if(HAL_I2C_Init(&I2cHandle) != HAL_OK)
    {
        // Initialization Error
        return FALSE;    
    }
    

    currentI2CXAction = xAction;
    currentI2CUnit = xAction->m_xActionUnits[ xAction->m_current++ ];
    
    UINT32 ccr = xAction->m_clockRate + (xAction->m_clockRate2 << 8);
    if (I2Cx->CCR != ccr) { // set clock rate and rise time
        UINT32 trise;
        if (ccr & I2C_CCR_FS) { // fast => 0.3ns rise time
            trise = SYSTEM_APB1_CLOCK_HZ / (1000 * 3333) + 1; // PCLK1 / 3333kHz
        } else { // slow => 1.0ns rise time
            trise = SYSTEM_APB1_CLOCK_HZ / (1000 * 1000) + 1; // PCLK1 / 1000kHz
        }
        I2Cx->CR1 = 0; // disable peripheral
        I2Cx->CCR = ccr;
        I2Cx->TRISE = trise;
    }
    
    I2Cx->CR1 = I2C_CR1_PE; // enable and reset special flags
    I2Cx->SR1 = 0; // reset error flags
    I2Cx->CR2 |= I2C_CR2_ITEVTEN | I2C_CR2_ITERREN; // enable interrupts
    I2Cx->CR1 = I2C_CR1_PE | I2C_CR1_START | I2C_CR1_ACK; // send start
}

void I2C_Internal_XActionStop()
{
    NATIVE_PROFILE_HAL_PROCESSOR_I2C();
    if (I2Cx->SR2 & I2C_SR2_BUSY && !(I2Cx->CR1 & I2C_CR1_STOP)) {
        I2Cx->CR1 |= I2C_CR1_STOP; // send stop
    }
    I2Cx->CR2 &= ~(I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN); // disable interrupts
    currentI2CXAction = NULL;
    currentI2CUnit = NULL;
}

void I2C_Internal_GetClockRate( UINT32 rateKhz, UINT8& clockRate, UINT8& clockRate2)
{
    NATIVE_PROFILE_HAL_PROCESSOR_I2C();
    if (rateKhz > 400) rateKhz = 400; // upper limit
    UINT32 ccr;
    if (rateKhz <= 100) { // slow clock
        ccr = (SYSTEM_APB1_CLOCK_HZ / 1000 / 2 - 1) / rateKhz + 1; // round up
        if (ccr > 0xFFF) ccr = 0xFFF; // max divider
    } else { // fast clock
        ccr = (SYSTEM_APB1_CLOCK_HZ / 1000 / 3 - 1) / rateKhz + 1; // round up
        ccr |= 0x8000; // set fast mode (duty cycle 1:2)
    }
    clockRate = (UINT8)ccr; // low byte
    clockRate2 = (UINT8)(ccr >> 8); // high byte
}

void I2C_Internal_GetPins(GPIO_PIN& scl, GPIO_PIN& sda)
{
    scl = I2C_SCL_PIN;
    sda = I2C_SDA_PIN;
}
