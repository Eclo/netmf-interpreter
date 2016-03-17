////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  Implementation for STM32F4: Copyright (c) Oberon microsystems, Inc.
//
//  *** Serial Driver ***
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <tinyhal.h>

#if (!defined(USART_PORTS) || !defined(USART_PORTS_TX_RX)) 
#error "No USART ports defined! Please go to platform_selector.h (in solution folder) and uncomment the define that matches the USART ports available in the device datasheet"
#endif

// IO addresses
static const USART_TypeDef* g_Uart_Ports[] = USART_PORTS;
static const GPIO_TypeDef* g_Uart_Ports_Tx_Rx[] = USART_PORTS_TX_RX;

// Pins
static BYTE g_Uart_RxD_Pins[ARRAYSIZE(g_Uart_Ports)];
static BYTE g_Uart_TxD_Pins[ARRAYSIZE(g_Uart_Ports)];
#ifdef USART_PORTS_FLOW_CONTROL
    static const GPIO_TypeDef* g_Uart_Ports_Flow_Control[] = USART_PORTS_FLOW_CONTROL;
    static BYTE g_Uart_CTS_Pins[ARRAYSIZE(g_Uart_Ports)];
    static BYTE g_Uart_RTS_Pins[ARRAYSIZE(g_Uart_Ports)];
#endif

void USART_Handle_RX_IRQ (int ComPortNum, USART_TypeDef* uart)
{
    INTERRUPT_START;

    char c = (char)(uart->DR); // read RX data
    USART_AddCharToRxBuffer(ComPortNum, c);
    Events_Set(SYSTEM_EVENT_FLAG_COM_IN);

    INTERRUPT_END;
}

void USART_Handle_TX_IRQ (int ComPortNum, USART_TypeDef* uart)
{
    INTERRUPT_START;

    char c;
    if (USART_RemoveCharFromTxBuffer(ComPortNum, c)) {
        uart->DR = c;  // write TX data
    } else {
        uart->CR1 &= ~USART_CR1_TXEIE; // TX int disable
    }
    Events_Set(SYSTEM_EVENT_FLAG_COM_OUT);

    INTERRUPT_END;
}

void USART_Interrupt0 (void* param)
{
    UINT16 sr = USART1->SR;
    if (sr & USART_SR_RXNE) USART_Handle_RX_IRQ(0, USART1);
    if (sr & USART_SR_TXE)  USART_Handle_TX_IRQ(0, USART1);
}

void USART_Interrupt1 (void* param)
{
    UINT16 sr = USART2->SR;
    if (sr & USART_SR_RXNE) USART_Handle_RX_IRQ(1, USART2);
    if (sr & USART_SR_TXE)  USART_Handle_TX_IRQ(1, USART2);
}

void USART_Interrupt2 (void* param)
{
    UINT16 sr = USART3->SR;
    if (sr & USART_SR_RXNE) USART_Handle_RX_IRQ(2, USART3);
    if (sr & USART_SR_TXE)  USART_Handle_TX_IRQ(2, USART3);
}

void USART_Interrupt3 (void* param)
{
    UINT16 sr = UART4->SR;
    if (sr & USART_SR_RXNE) USART_Handle_RX_IRQ(3, UART4);
    if (sr & USART_SR_TXE)  USART_Handle_TX_IRQ(3, UART4);
}

void USART_Interrupt4 (void* param)
{
    UINT16 sr = UART5->SR;
    if (sr & USART_SR_RXNE) USART_Handle_RX_IRQ(4, UART5);
    if (sr & USART_SR_TXE)  USART_Handle_TX_IRQ(4, UART5);
}

void USART_Interrupt5 (void* param)
{
    UINT16 sr = USART6->SR;
    if (sr & USART_SR_RXNE)
        USART_Handle_RX_IRQ(5, USART6);

    if (sr & USART_SR_TXE)
        USART_Handle_TX_IRQ(5, USART6);
}

BOOL ComputeUSARTPins()
{
    // sanity check
    if(ARRAYSIZE(g_Uart_Ports) != ARRAYSIZE(g_Uart_Ports_Tx_Rx))
    {
        return FALSE;
    }
    #ifdef USART_PORTS_FLOW_CONTROL
        if(ARRAYSIZE(g_Uart_Ports) != ARRAYSIZE(g_Uart_Ports_Flow_Control))
        {
            return FALSE;
        }
    #endif

    // fill arrays with user USART ports definitions
    for(int i = 0; i < ARRAYSIZE(g_Uart_Ports); i++)
    {
        if(g_Uart_Ports[i] == USART1)
        {
            if(g_Uart_Ports_Tx_Rx[i] == GPIOA)
            {
                g_Uart_RxD_Pins[i] = 10; // PA10
                g_Uart_TxD_Pins[i] = 9;  // PA9
            }
            else if(g_Uart_Ports_Tx_Rx[i] == GPIOB)
            {
                g_Uart_RxD_Pins[i] = 23;  // PB7
                g_Uart_TxD_Pins[i] = 22;  // PB6
            }
            else
            {
                // invalid port selection
                return FALSE;
            }
                
            #ifdef USART_PORTS_FLOW_CONTROL
                if(g_Uart_Ports_Flow_Control[i] != GPIO_NO_PORT && g_Uart_Ports_Flow_Control[i] == GPIOA)
                {
                    g_Uart_CTS_Pins[i] = 11; // PA11
                    g_Uart_RTS_Pins[i] = 12; // PA12
                }
                else if(g_Uart_Ports_Flow_Control[i] != GPIO_NO_PORT)
                {
                    // invalid port selection
                    return FALSE;
                } 
            #endif
        }
        #ifdef USART2
        else if(g_Uart_Ports[i] == USART2)
        {
            if(g_Uart_Ports_Tx_Rx[i] == GPIOA)
            {
                g_Uart_RxD_Pins[i] = 3;  // PA3
                g_Uart_TxD_Pins[i] = 2;  // PA2
            }
            else if(g_Uart_Ports_Tx_Rx[i] == GPIOD)
            {
                g_Uart_RxD_Pins[i] = 54;  // PD6
                g_Uart_TxD_Pins[i] = 53;  // PD5
            }
            else
            {
                // invalid port selection
                return FALSE;
            }
                
            #ifdef USART_PORTS_FLOW_CONTROL
                if(g_Uart_Ports_Flow_Control[i] != GPIO_NO_PORT)
                {
                    if(g_Uart_Ports_Flow_Control[i] == GPIOA)
                    {
                        g_Uart_CTS_Pins[i] = 1; // PA1
                        g_Uart_RTS_Pins[i] = 0; // PA0
                    }
                    else if(g_Uart_Ports_Flow_Control[i] == GPIOA)
                    {
                        g_Uart_CTS_Pins[i] = 52; // PD4
                        g_Uart_RTS_Pins[i] = 51; // PD3
                    }
                    else
                    {
                        // invalid port selection
                        return FALSE;
                    }
                }
            #endif
        }
        #endif
        #ifdef USART3
        else if(g_Uart_Ports[i] == USART3)
        {
            if(g_Uart_Ports_Tx_Rx[i] == GPIOB)
            {
                g_Uart_RxD_Pins[i] = 27;  // PB11
                g_Uart_TxD_Pins[i] = 26;  // PB10
            }
            else if(g_Uart_Ports_Tx_Rx[i] == GPIOC)
            {
                g_Uart_RxD_Pins[i] = 43;  // PC11
                g_Uart_TxD_Pins[i] = 42;  // PC10
            }
            else if(g_Uart_Ports_Tx_Rx[i] == GPIOD)
            {
                g_Uart_RxD_Pins[i] = 57;  // PD9
                g_Uart_TxD_Pins[i] = 56;  // PD8
            }
            else
                // shouldn't get here!
                return FALSE;
                
            #ifdef USART_PORTS_FLOW_CONTROL
                if(g_Uart_Ports_Flow_Control[i] != GPIO_NO_PORT)
                {
                    if(g_Uart_Ports_Flow_Control[i] == GPIOB)
                    {
                        g_Uart_CTS_Pins[i] = 30; // PB14
                        g_Uart_RTS_Pins[i] = 29; // PB13
                    }
                    else if(g_Uart_Ports_Flow_Control[i] == GPIOD)
                    {
                        g_Uart_CTS_Pins[i] = 60; // PD12
                        g_Uart_RTS_Pins[i] = 59; // PD11
                    }
                    else
                    {
                        // invalid port selection
                        return FALSE;
                    }
                }
            #endif
        }
        #endif
        #ifdef UART4
        else if(g_Uart_Ports[i] == UART4)
        {
            if(g_Uart_Ports_Tx_Rx[i] == GPIOA)
            {
                g_Uart_RxD_Pins[i] = 1;  // PA1
                g_Uart_TxD_Pins[i] = 0;  // PA0
            }
            else if(g_Uart_Ports_Tx_Rx[i] == GPIOC)
            {
                g_Uart_RxD_Pins[i] = 43;  // PC11
                g_Uart_TxD_Pins[i] = 42;  // PC10
            }
            else
            {
                // invalid port selection
                return FALSE;
            }
        }
        #endif
        #ifdef USART6
        else if(g_Uart_Ports[i] == USART6)
        {
            if(g_Uart_Ports_Tx_Rx[i] == GPIOC)
            {
                g_Uart_RxD_Pins[i] = 39;  // PC7
                g_Uart_TxD_Pins[i] = 38;  // PC6
            }
            else if(g_Uart_Ports_Tx_Rx[i] == GPIOG)
            {
                g_Uart_RxD_Pins[i] = 105;  // PG9
                g_Uart_TxD_Pins[i] = 110;  // PG14
            }
            else
            {
                // invalid port selection
                return FALSE;
            }
            #ifdef USART_PORTS_FLOW_CONTROL
                if(g_Uart_Ports_Flow_Control[i] != GPIO_NO_PORT && g_Uart_Ports_Flow_Control[i] == GPIOG)
                {
                        g_Uart_CTS_Pins[i] = 109; // PG13
                        g_Uart_RTS_Pins[i] = 108; // PG12
                }
                else if(g_Uart_Ports_Flow_Control[i] != GPIO_NO_PORT)
                {
                    // invalid port selection
                    return FALSE;
                }
            #endif
        }
        #endif
        #ifdef UART7
        else if(g_Uart_Ports[i] == UART7)
        {
            if(g_Uart_Ports_Tx_Rx[i] == GPIOE)
            {
                g_Uart_RxD_Pins[i] = 71;  // PE7
                g_Uart_TxD_Pins[i] = 72;  // PE8
            }
            else if(g_Uart_Ports_Tx_Rx[i] == GPIOF)
            {
                g_Uart_RxD_Pins[i] = 86;  // PF6
                g_Uart_TxD_Pins[i] = 87;  // PF7
            }
            else
            {
                // invalid port selection
                return FALSE;
            }
        }
        #endif
        #ifdef UART8
        else if(g_Uart_Ports[i] == UART8)
        {
            if(g_Uart_Ports_Tx_Rx[i] == GPIOE)
            {
                g_Uart_RxD_Pins[i] = 64;  // PE0
                g_Uart_TxD_Pins[i] = 65;  // PE1
            }
            else
            {
                // invalid port selection
                return FALSE;
            }
        }
        #endif
    }
}

void EnableUSARTGPIOClocks(int ComPortNum)
{
    // Enable GPIO TX/RX clock
    if(g_Uart_Ports_Tx_Rx[ComPortNum] == GPIOA)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    }
    else if(g_Uart_Ports_Tx_Rx[ComPortNum] == GPIOB)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
    else if(g_Uart_Ports_Tx_Rx[ComPortNum] == GPIOC)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
    else if(g_Uart_Ports_Tx_Rx[ComPortNum] == GPIOD)
    {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }
    else if(g_Uart_Ports_Tx_Rx[ComPortNum] == GPIOE)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
    else if(g_Uart_Ports_Tx_Rx[ComPortNum] == GPIOF)
    {
        __HAL_RCC_GPIOF_CLK_ENABLE();
    }
    else if(g_Uart_Ports_Tx_Rx[ComPortNum] == GPIOG)
    {
        __HAL_RCC_GPIOG_CLK_ENABLE();
    }
    else if(g_Uart_Ports_Tx_Rx[ComPortNum] == GPIOH)
    {
        __HAL_RCC_GPIOH_CLK_ENABLE();
    }
    else if(g_Uart_Ports_Tx_Rx[ComPortNum] == GPIOI)
    {
        __HAL_RCC_GPIOI_CLK_ENABLE();
    }
    #ifdef GPIOJ
    else if(g_Uart_Ports_Tx_Rx[ComPortNum] == GPIOJ)
    {
        __HAL_RCC_GPIOJ_CLK_ENABLE();
    }
    #endif
    #ifdef GPIOK
    else if(g_Uart_Ports_Tx_Rx[ComPortNum] == GPIOK)
    {
        __HAL_RCC_GPIOK_CLK_ENABLE();
    }
    #endif
    
    // Enable USART1 clock
    if(g_Uart_Ports[ComPortNum] == USART1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
    }
    #ifdef USART2
    else if(g_Uart_Ports[ComPortNum] == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
    }
    #endif
    #ifdef USART3
    else if(g_Uart_Ports[ComPortNum] == USART3)
    {
        __HAL_RCC_USART3_CLK_ENABLE();
    }
    #endif
    #ifdef UART4
    else if(g_Uart_Ports[ComPortNum] == UART4)
    {
        __HAL_RCC_UART4_CLK_ENABLE();
    }
    #endif
    #ifdef USART6
    else if(g_Uart_Ports[ComPortNum] == USART6)
    {
        __HAL_RCC_USART6_CLK_ENABLE();
    }
    #endif
    #ifdef UART7
    else if(g_Uart_Ports[ComPortNum] == UART7)
    {
        __HAL_RCC_UART7_CLK_ENABLE();
    }
    #endif
    #ifdef UART8
    else if(g_Uart_Ports[ComPortNum] == UART8)
    {
        __HAL_RCC_UART8_CLK_ENABLE();
    }
    #endif
}

void ConfigureUSARTGPIOPin(int ComPortNum)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    
    // USART/UART alternate function
    if(g_Uart_Ports[ComPortNum] == USART1)
    {
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    }
    #ifdef USART2
    else if(g_Uart_Ports[ComPortNum] == USART2)
    {
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    }
    #endif
    #ifdef USART3
    else if(g_Uart_Ports[ComPortNum] == USART3)
    {
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    }
    #endif
    #ifdef UART4
    else if(g_Uart_Ports[ComPortNum] == UART4)
    {
        GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
    }
    #endif
    #ifdef USART6
    else if(g_Uart_Ports[ComPortNum] == USART6)
    {
        GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    }
    #endif
    #ifdef UART7
    else if(g_Uart_Ports[ComPortNum] == UART7)
    {
        GPIO_InitStruct.Alternate = GPIO_AF8_UART7;
    }
    #endif
    #ifdef UART8
    else if(g_Uart_Ports[ComPortNum] == UART8)
    {
        GPIO_InitStruct.Alternate = GPIO_AF8_UART8;
    }
    #endif

    // UART TX GPIO pin configuration
    GPIO_InitStruct.Pin       = g_Uart_TxD_Pins[ComPortNum];
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;

    HAL_GPIO_Init((GPIO_TypeDef*)g_Uart_Ports_Tx_Rx[ComPortNum], &GPIO_InitStruct);

    // UART RX GPIO pin configuration
    GPIO_InitStruct.Pin = g_Uart_RxD_Pins[ComPortNum];

    HAL_GPIO_Init((GPIO_TypeDef*)g_Uart_Ports_Tx_Rx[ComPortNum], &GPIO_InitStruct);
}

/**
  * @brief UART MSP Initialization 
  *        This function configures the hardware resources used:
  *           - Compute peripherals pin arrays (only once)
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration  
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    if(g_Uart_RxD_Pins[0] == 0)
    {
        //  need to fill USART pin arrays
        ComputeUSARTPins();
    }
    
    for(int i = 0; i < ARRAYSIZE(g_Uart_Ports); i++)
    {
        if(g_Uart_Ports[i] == huart->Instance)
        {
            // Enable GPIO TX/RX clock and USART clock
            EnableUSARTGPIOClocks(i);
            // UART TX/RX GPIO pin configuration
            ConfigureUSARTGPIOPin(i);
            // exit loop
            break;
        }
    }
}

/**
  * @brief UART MSP De-Initialization 
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO configuration to their default state
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    /*##-1- Reset peripherals ##################################################*/
    for(int i = 0; i < ARRAYSIZE(g_Uart_Ports); i++)
    {
        if(g_Uart_Ports[i] == huart->Instance)
        {
            if(g_Uart_Ports[i] == USART1)
            {
                __HAL_RCC_USART1_FORCE_RESET();
                __HAL_RCC_USART1_RELEASE_RESET();
            }
            #ifdef USART2
            else if(g_Uart_Ports[i] == USART2)
            {
                __HAL_RCC_USART2_FORCE_RESET();
                __HAL_RCC_USART2_RELEASE_RESET();
            }
            #endif
            #ifdef USART3
            else if(g_Uart_Ports[i] == USART3)
            {
                __HAL_RCC_USART3_FORCE_RESET();
                __HAL_RCC_USART3_RELEASE_RESET();
            }
            #endif
            #ifdef UART4
            else if(g_Uart_Ports[i] == UART4)
            {
                __HAL_RCC_UART4_FORCE_RESET();
                __HAL_RCC_UART4_RELEASE_RESET();
            }
            #endif
            #ifdef USART6
            else if(g_Uart_Ports[i] == USART6)
            {
                __HAL_RCC_USART6_FORCE_RESET();
                __HAL_RCC_USART6_RELEASE_RESET();
            }
            #endif
            #ifdef UART7
            else if(g_Uart_Ports[i] == UART7)
            {
                __HAL_RCC_UART7_FORCE_RESET();
                __HAL_RCC_UART7_RELEASE_RESET();
            }
            #endif
            #ifdef UART8
            else if(g_Uart_Ports[i] == UART8)
            {
                __HAL_RCC_UART8_FORCE_RESET();
                __HAL_RCC_UART8_RELEASE_RESET();
            }
            #endif

            /*##-2- Disable peripherals and GPIO Clocks ################################*/
            /* Configure UART Tx as alternate function */
            HAL_GPIO_DeInit((GPIO_TypeDef*)g_Uart_Ports_Tx_Rx[i], g_Uart_TxD_Pins[i]);
            /* Configure UART Rx as alternate function */
            HAL_GPIO_DeInit((GPIO_TypeDef*)g_Uart_Ports_Tx_Rx[i], g_Uart_RxD_Pins[i]);
        }
    }
}

BOOL CPU_USART_Initialize( int ComPortNum, int BaudRate, int Parity, int DataBits, int StopBits, int FlowValue )
{
    UART_HandleTypeDef UartHandle;

    if (ComPortNum >= TOTAL_USART_PORT)
        return FALSE;

    if (Parity >= USART_PARITY_MARK)
        return FALSE;
    
    GLOBAL_LOCK(irq);
    
    // USART_TypeDef* uart = g_Uart_Ports[ComPortNum];

    // UINT32 clk;
    
    // // enable UART clock
    // if (ComPortNum == 5)
    // { // COM6 on APB2
    //     RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
    //     clk = SYSTEM_APB2_CLOCK_HZ;
    // }
    // else if (ComPortNum == 0)
    // { // COM1 on APB2
    //     RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    //     clk = SYSTEM_APB2_CLOCK_HZ;
    // }
    // else if (ComPortNum < 5)
    // { // COM2-5 on APB1
    //     RCC->APB1ENR |= RCC_APB1ENR_USART2EN >> 1 << ComPortNum;
    //     clk = SYSTEM_APB1_CLOCK_HZ;
    // }
    // else
    // { // COM7-8 on APB1
    //     RCC->APB1ENR |= RCC_APB1ENR_UART7EN >> 6 << ComPortNum;
    //     clk = SYSTEM_APB1_CLOCK_HZ;
    // }
    
    //  baudrate
    // UINT16 div = (UINT16)((clk + (BaudRate >> 1)) / BaudRate); // rounded
    // uart->BRR = div;
    
    UartHandle.Instance          = (USART_TypeDef*)g_Uart_Ports[ComPortNum];
    UartHandle.Init.BaudRate     = BaudRate;
    // UART Number of Stop Bits
    if (StopBits == USART_STOP_BITS_TWO)
    {
        UartHandle.Init.StopBits = USART_CR2_STOP_1;    
    }
    else
    {
        UartHandle.Init.StopBits = 0;
    }
    // UART Parity
    if(Parity == UART_PARITY_ODD)
    {
        UartHandle.Init.Parity   = USART_CR1_PCE | USART_CR1_PS;
    }
    else if(Parity == UART_PARITY_EVEN)
    {
        UartHandle.Init.Parity   = USART_CR1_PCE;
        DataBits++;
    }
    else
    {
        UartHandle.Init.Parity = 0;
    }
    // UART Word Length
    if (DataBits == 9)
        UartHandle.Init.WordLength = USART_CR1_M;
    else
    {
        if (DataBits != 8)
            return FALSE;
        UartHandle.Init.WordLength   = 0;
    }
    // UART Hardware Flow Control
    if (FlowValue & USART_FLOW_NONE)
    {
        UartHandle.Init.HwFlowCtl |= 0;
    }
    if (FlowValue & USART_FLOW_HW_OUT_EN)
    {
        UartHandle.Init.HwFlowCtl |= USART_CR3_CTSE;
    }
    if (FlowValue & USART_FLOW_HW_IN_EN)
    {
        UartHandle.Init.HwFlowCtl |= USART_CR3_RTSE;
    }   

    UartHandle.Init.Mode         = UART_MODE_TX_RX;
    UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
      
    // // control
    // UINT16 ctrl = USART_CR1_TE | USART_CR1_RE;
    // if (Parity) {
    //     ctrl |= USART_CR1_PCE;
    //     DataBits++;
    // }

    // if (Parity == USART_PARITY_ODD)
    //     ctrl |= USART_CR1_PS;

    // if (DataBits == 9)
    //     ctrl |= USART_CR1_M;
    // else
    // {
    //     if (DataBits != 8)
    //         return FALSE;
    // }
    // uart->CR1 = ctrl;
    
    // if (StopBits == USART_STOP_BITS_ONE)
    //     StopBits = 0;

    // uart->CR2 = (UINT16)(StopBits << 12);
    
    // ctrl = 0;
    // if (FlowValue & USART_FLOW_HW_OUT_EN)
    //     ctrl |= USART_CR3_CTSE;

    // if (FlowValue & USART_FLOW_HW_IN_EN)
    //     ctrl |= USART_CR3_RTSE;

    // uart->CR3 = ctrl;

    // GPIO_PIN rxPin, txPin, ctsPin, rtsPin;
    // CPU_USART_GetPins(ComPortNum, rxPin, txPin, ctsPin, rtsPin);
    // UINT32 alternate = 0x72; // AF7 = USART1-3
    // if (ComPortNum >= 3)
    //     alternate = 0x82; // AF8 = UART4-8

    // CPU_GPIO_DisablePin(rxPin, RESISTOR_PULLUP, 0, (GPIO_ALT_MODE)alternate);
    // CPU_GPIO_DisablePin(txPin, RESISTOR_DISABLED, 1, (GPIO_ALT_MODE)alternate);
    // if (FlowValue & USART_FLOW_HW_OUT_EN)
    // {
    //     if (ctsPin == GPIO_PIN_NONE)
    //         return FALSE;

    //     CPU_GPIO_DisablePin(ctsPin, RESISTOR_DISABLED, 0, (GPIO_ALT_MODE)alternate);
    // }

    // if (FlowValue & USART_FLOW_HW_IN_EN)
    // {
    //     if (rtsPin == GPIO_PIN_NONE)
    //         return FALSE;

    //     CPU_GPIO_DisablePin(rtsPin, RESISTOR_DISABLED, 1, (GPIO_ALT_MODE)alternate);
    // }
    
    // CPU_USART_ProtectPins(ComPortNum, FALSE);
    
//     switch (ComPortNum)
//     {
//     case 0:
//         CPU_INTC_ActivateInterrupt(USART1_IRQn, USART_Interrupt0, 0);
//         break;

//     case 1:
//         CPU_INTC_ActivateInterrupt(USART2_IRQn, USART_Interrupt1, 0);
//         break;

//     case 2:
//         CPU_INTC_ActivateInterrupt(USART3_IRQn, USART_Interrupt2, 0);
//         break;

//     case 3:
//         CPU_INTC_ActivateInterrupt(UART4_IRQn, USART_Interrupt3, 0);
//         break;

//     case 4:
//         CPU_INTC_ActivateInterrupt(UART5_IRQn, USART_Interrupt4, 0);
//         break;

//     case 5:
//         CPU_INTC_ActivateInterrupt(USART6_IRQn, USART_Interrupt5, 0);
//         break;

// // some SoCS have more UARTs (default is 6 )
// #if TOTAL_USART_PORT > 6 
//     case 6:
//         CPU_INTC_ActivateInterrupt(UART7_IRQn, USART_Interrupt4, 0);
//         break;

//     case 7:
//         CPU_INTC_ActivateInterrupt(UART8_IRQn, USART_Interrupt5, 0);
//         break;
// #endif
//     }

//     uart->CR1 |= USART_CR1_UE; // start uart

    if(HAL_UART_Init(&UartHandle) != HAL_OK)
    {
        return FALSE;
    }
    return TRUE;
}

BOOL CPU_USART_Uninitialize( int ComPortNum )
{
    GLOBAL_LOCK(irq);
    
    ((USART_TypeDef*)g_Uart_Ports[ComPortNum])->CR1 = 0; // stop uart

//     switch (ComPortNum)
//     {
//     case 0:
//         CPU_INTC_DeactivateInterrupt(USART1_IRQn);
//         break;

//     case 1:
//         CPU_INTC_DeactivateInterrupt(USART2_IRQn);
//         break;

//     case 2:
//         CPU_INTC_DeactivateInterrupt(USART3_IRQn);
//         break;

//     case 3:
//         CPU_INTC_DeactivateInterrupt(UART4_IRQn);
//         break;

//     case 4:
//         CPU_INTC_DeactivateInterrupt(UART5_IRQn);
//         break;

//     case 5:
//         CPU_INTC_DeactivateInterrupt(USART6_IRQn);
//         break;

// // some SoCS have more UARTs (default is 6 )
// #if TOTAL_USART_PORT > 6 
//     case 6:
//         CPU_INTC_DeactivateInterrupt(UART7_IRQn);
//         break;

//     case 7:
//         CPU_INTC_DeactivateInterrupt(UART8_IRQn);
//         break;
// #endif
//     }

    CPU_USART_ProtectPins(ComPortNum, TRUE);
    
    // // disable UART clock
    // if (ComPortNum == 5)
    // { // COM6 on APB2
    //     RCC->APB2ENR &= ~RCC_APB2ENR_USART6EN;
    // }
    // else if (ComPortNum == 0)
    // { // COM1 on APB2
    //     RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN;
    // }
    // else if (ComPortNum < 5) 
    // { // COM2-5 on APB1
    //     RCC->APB1ENR &= ~(RCC_APB1ENR_USART2EN >> 1 << ComPortNum);
    // } 
    // else 
    // { // COM7-8 on APB1
    //     RCC->APB1ENR &= ~(RCC_APB1ENR_UART7EN >> 6 << ComPortNum);
    // }

    return TRUE;
}

BOOL CPU_USART_TxBufferEmpty( int ComPortNum )
{
    if (g_Uart_Ports[ComPortNum]->SR & USART_SR_TXE)
        return TRUE;

    return FALSE;
}

BOOL CPU_USART_TxShiftRegisterEmpty( int ComPortNum )
{
    if (g_Uart_Ports[ComPortNum]->SR & USART_SR_TC)
        return TRUE;

    return FALSE;
}

void CPU_USART_WriteCharToTxBuffer( int ComPortNum, UINT8 c )
{   
#ifdef DEBUG
    ASSERT(CPU_USART_TxBufferEmpty(ComPortNum));
#endif
    ((USART_TypeDef*)g_Uart_Ports[ComPortNum])->DR = c;
}

void CPU_USART_TxBufferEmptyInterruptEnable( int ComPortNum, BOOL Enable )
{
    USART_TypeDef* uart = (USART_TypeDef*)g_Uart_Ports[ComPortNum];
    if (Enable) 
    {
        uart->CR1 |= USART_CR1_TXEIE;  // tx int enable
    } 
    else
    {
        uart->CR1 &= ~USART_CR1_TXEIE; // tx int disable
    }
}

BOOL CPU_USART_TxBufferEmptyInterruptState( int ComPortNum )
{
    if (g_Uart_Ports[ComPortNum]->CR1 & USART_CR1_TXEIE)
        return TRUE;

    return FALSE;
}

void CPU_USART_RxBufferFullInterruptEnable( int ComPortNum, BOOL Enable )
{
    USART_TypeDef* uart = (USART_TypeDef*)g_Uart_Ports[ComPortNum];
    if (Enable) 
    {
        uart->CR1 |= USART_CR1_RXNEIE;  // rx int enable
    } 
    else 
    {
        uart->CR1 &= ~USART_CR1_RXNEIE; // rx int disable
    }
}

BOOL CPU_USART_RxBufferFullInterruptState( int ComPortNum )
{
    if (g_Uart_Ports[ComPortNum]->CR1 & USART_CR1_RXNEIE)
        return TRUE;

    return FALSE;
}

BOOL CPU_USART_TxHandshakeEnabledState( int ComPortNum )
{
    // The state of the CTS input only matters if Flow Control is enabled
#ifdef USART_PORTS_FLOW_CONTROL
    if( (UINT32)ComPortNum < ARRAYSIZE_CONST_EXPR(g_Uart_CTS_Pins)
     && g_Uart_Ports[ComPortNum]->CR3 & USART_CR3_CTSE
      )
    {
        return !CPU_GPIO_GetPinState(g_Uart_CTS_Pins[ComPortNum]); // CTS active
    }
#endif

    return TRUE; // If this handshake input is not being used, it is assumed to be good
}

void CPU_USART_ProtectPins( int ComPortNum, BOOL On )  // idempotent
{
    if (On)
    {
        CPU_USART_RxBufferFullInterruptEnable(ComPortNum, FALSE);
        CPU_USART_TxBufferEmptyInterruptEnable(ComPortNum, FALSE);
    }
    else
    {
        CPU_USART_TxBufferEmptyInterruptEnable(ComPortNum, TRUE);
        CPU_USART_RxBufferFullInterruptEnable(ComPortNum, TRUE);
    }
}

UINT32 CPU_USART_PortsCount()
{
    return TOTAL_USART_PORT;
}

void CPU_USART_GetPins( int ComPortNum, GPIO_PIN& rxPin, GPIO_PIN& txPin,GPIO_PIN& ctsPin, GPIO_PIN& rtsPin )
{
    rxPin = txPin = ctsPin = rtsPin = GPIO_PIN_NONE;
    if ((UINT32)ComPortNum >= ARRAYSIZE_CONST_EXPR(g_Uart_RxD_Pins))
        return;

    rxPin = g_Uart_RxD_Pins[ComPortNum];
    txPin = g_Uart_TxD_Pins[ComPortNum];

#if defined(UART_CTS_PINS) && defined(UART_RTS_PINS)
    if ((UINT32)ComPortNum >= ARRAYSIZE_CONST_EXPR(g_Uart_CTS_Pins))
        return; // no CTS/RTS

    ctsPin = g_Uart_CTS_Pins[ComPortNum];
    rtsPin = g_Uart_RTS_Pins[ComPortNum];
#endif
}

void CPU_USART_GetBaudrateBoundary( int ComPortNum, UINT32 & maxBaudrateHz, UINT32 & minBaudrateHz )
{
    UINT32 clk = SYSTEM_APB2_CLOCK_HZ;
    if (ComPortNum && ComPortNum != 5)
        clk = SYSTEM_APB1_CLOCK_HZ;

    maxBaudrateHz = clk >> 4;
    minBaudrateHz = clk >> 16;
}

BOOL CPU_USART_SupportNonStandardBaudRate( int ComPortNum )
{
    return TRUE;
}

BOOL CPU_USART_IsBaudrateSupported( int ComPortNum, UINT32& BaudrateHz )
{
    UINT32 max = SYSTEM_APB2_CLOCK_HZ >> 4;
    if (ComPortNum && ComPortNum != 5)
        max = SYSTEM_APB1_CLOCK_HZ >> 4;

    if (BaudrateHz <= max)
        return TRUE;

    BaudrateHz = max;
    return FALSE;
}


