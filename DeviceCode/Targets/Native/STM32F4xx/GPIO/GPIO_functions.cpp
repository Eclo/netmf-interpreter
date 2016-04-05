////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  Implementation for CMSIS: Copyright (c) Eclo Solutions Lda. Based in original code from Oberon microsystems, Inc.
//
//  *** GPIO Driver ***
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <tinyhal.h>

#define Gpio_MaxPins (TOTAL_GPIO_PORT * 16)
#define Gpio_MaxInt 16

// indexed port configuration access
#define Port(port) ((GPIO_TypeDef *) (GPIOA_BASE + (port << 10)))

struct Int_State
{
    HAL_COMPLETION completion; // debounce completion
    BYTE packed_port_pin;      // encoded number with port and pin number
    BYTE mode;     // edge mode
    BYTE debounce; // debounce flag
    BYTE expected; // expected pin state
    GPIO_INTERRUPT_SERVICE_ROUTINE ISR; // interrupt handler
    void* param;   // interrupt handler parameter
    UINT32 debounceTicks;
};

static Int_State g_int_state[Gpio_MaxInt]; // interrupt state

static UINT32 g_debounceTicks;
static UINT16 g_pinReserved[TOTAL_GPIO_PORT]; //  1 bit per pin

/*
 * Debounce Completion Handler
 */
void GPIO_DebounceHandler(void* arg)
{
    Int_State* state = (Int_State*)arg;
    if(state->ISR)
    {
        UINT32 actual = CPU_GPIO_GetPinState(state->packed_port_pin); // get actual pin state
        if(actual == state->expected)
        {
            state->ISR(state->packed_port_pin, actual, state->param);
            if(state->mode == GPIO_INT_EDGE_BOTH)
            { // both edges
                state->expected ^= 1; // update expected state
            }
        }
    }
}

/*
 * Interrupt Handler
 */
void GlobalGPIOHandler(int num)  // 0 <= num <= 15
{
    Int_State* state = &g_int_state[num];
    state->completion.Abort();
    UINT32 gpio_pin = 1 << num;
    UINT32 actual;
    
    HAL_GPIO_EXTI_IRQHandler(gpio_pin);
    
    actual = CPU_GPIO_GetPinState(state->packed_port_pin); // get actual pin state

    if(state->ISR)
    {
        if(state->debounce)
        {   // debounce enabled
            // for back compat treat state.debounceTicks == 0 as indication to use global debounce setting
            UINT32 debounceDeltaTicks = state->debounceTicks == 0 ? g_debounceTicks : state->debounceTicks;
            state->completion.EnqueueTicks(HAL_Time_CurrentTicks() + debounceDeltaTicks);
        }
        else
        {
            state->ISR(state->packed_port_pin, state->expected, state->param);
            if(state->mode == GPIO_INT_EDGE_BOTH)
            { // both edges
                if(actual != state->expected)
                { // fire another isr to keep in synch
                    state->ISR(state->packed_port_pin, actual, state->param);
                }
                else
                {
                    state->expected ^= 1; // update expected state
                }
            }
        }
    }

}



void EXTI0_IRQHandler(void) // EXTI0
{
    GlobalGPIOHandler(0);
}

void EXTI1_IRQHandler(void) // EXTI1
{
    GlobalGPIOHandler(1);
}

void EXTI2_IRQHandler(void) // EXTI2
{
    GlobalGPIOHandler(2);
}

void EXTI3_IRQHandler(void) // EXTI3
{
    GlobalGPIOHandler(3);
}

void EXTI4_IRQHandler(void) // EXTI4
{
    GlobalGPIOHandler(4);
}

void EXTI5_IRQHandler(void) // EXTI5 - EXTI9
{
    UINT32 pending = EXTI->PR & EXTI->IMR & 0x03E0; // pending bits 5..9
    int num = 5; 
    pending >>= 5;
    do
    {
        if(pending & 1) 
        {
            GlobalGPIOHandler(num);
        }
        num++;
        pending >>= 1;
    } 
    while(pending);
}

void EXTI10_IRQHandler(void) // EXTI10 - EXTI15
{
    UINT32 pending = EXTI->PR & EXTI->IMR & 0xFC00; // pending bits 10..15
    int num = 10; 
    pending >>= 10;
    do
    {
        if(pending & 1) 
        {
            GlobalGPIOHandler(num);
        }
        num++; 
        pending >>= 1;
    } 
    while(pending);
}

BOOL GPIO_Set_Interrupt(UINT32 pin
                               , GPIO_INTERRUPT_SERVICE_ROUTINE ISR
                               , void* ISR_Param
                               , GPIO_INT_EDGE mode
                               , BOOL GlitchFilterEnable
                              )
{
    UINT32 num = pin & 0x0F;
    UINT32 bit = 1 << num;
    UINT32 shift = (num & 0x3) << 2; // 4 bit fields
    UINT32 idx = num >> 2;
    UINT32 mask = 0xF << shift;
    UINT32 config = (pin >> 4) << shift; // port number configuration

    Int_State* state = &g_int_state[num];

    GLOBAL_LOCK(irq);

    if(ISR)
    {
        if((SYSCFG->EXTICR[idx] & mask) != config)
        {
            if(EXTI->IMR & bit)
                return FALSE; // interrupt in use

            SYSCFG->EXTICR[idx] = SYSCFG->EXTICR[idx] & ~mask | config;
        }
        state->packed_port_pin = (BYTE)pin;
        state->mode = (BYTE)mode;
        state->debounce = (BYTE)GlitchFilterEnable;
        state->param = ISR_Param;
        state->ISR = ISR;
        state->completion.Abort();
        state->completion.SetArgument(state);

        EXTI->RTSR &= ~bit;
        EXTI->FTSR &= ~bit;
        switch(mode)
        {
        case GPIO_INT_EDGE_LOW:
        case GPIO_INT_LEVEL_LOW:
            EXTI->FTSR |= bit;
            state->expected = FALSE;
            break;

        case GPIO_INT_EDGE_HIGH:
        case GPIO_INT_LEVEL_HIGH:
            EXTI->RTSR |= bit;
            state->expected = TRUE;
            break;

        case GPIO_INT_EDGE_BOTH:
            EXTI->FTSR |= bit;
            EXTI->RTSR |= bit;
            UINT32 actual;
            do
            {
                EXTI->PR = bit; // remove pending interrupt
                actual = CPU_GPIO_GetPinState(pin); // get actual pin state
            } while(EXTI->PR & bit); // repeat if pending again
            state->expected = (BYTE)(actual ^ 1);
        }

        EXTI->IMR |= bit; // enable interrupt
        // check for level interrupts
        if(mode == GPIO_INT_LEVEL_HIGH && CPU_GPIO_GetPinState(pin)
            || mode == GPIO_INT_LEVEL_LOW && !CPU_GPIO_GetPinState(pin))
        {
            EXTI->SWIER = bit; // force interrupt
        }
    }
    else if((SYSCFG->EXTICR[idx] & mask) == config)
    {
        EXTI->IMR &= ~bit; // disable interrupt
        state->ISR = NULL;
        state->completion.Abort();
    }
    return TRUE;
}

void EnableIRQ(uint16_t gpio_pin)
{
    if(gpio_pin == GPIO_PIN_0)
    {
        HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_1)
    {
        HAL_NVIC_EnableIRQ(EXTI1_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_2)
    {
        HAL_NVIC_EnableIRQ(EXTI2_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_3)
    {
        HAL_NVIC_EnableIRQ(EXTI3_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_4)
    {
        HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_5)
    {
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_6)
    {
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_7)
    {
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_8)
    {
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_9)
    {
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_10)
    {
        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_11)
    {
        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_12)
    {
        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_13)
    {
        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_14)
    {
        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    }
    else if(gpio_pin == GPIO_PIN_15)
    {
        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    }
}

// mode:  0: input,  1: output,  2: alternate, 3: analog
// alternate: od | AF << 4 | speed << 8
void GPIO_Pin_Config(GPIO_PIN packed_port_pin, UINT32 mode, GPIO_RESISTOR resistor, UINT32 alternate)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_TypeDef* port = Port(packed_port_pin >> 4); // pointer to the actual port registers
    packed_port_pin &= 0x0F; // bit number
    UINT32 gpio_pin = 1 << packed_port_pin;

    // Configure PA0 pin as input floating
    GPIO_InitStructure.Mode = mode;
    if(resistor == RESISTOR_PULLUP)
        GPIO_InitStructure.Pull = GPIO_PUPDR_PUPDR0_0;
    if(resistor == RESISTOR_PULLDOWN)
        GPIO_InitStructure.Pull = GPIO_PUPDR_PUPDR0_1;
    GPIO_InitStructure.Pin = gpio_pin;
    
    // Enable GPIO clock
    #if defined (RCC_AHB1ENR_GPIOAEN)
    if(port == GPIOA)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
        // Enable and set EXTI Line Interrupt
        EnableIRQ(gpio_pin);
    }
    #endif
    #if defined (RCC_AHB1ENR_GPIOBEN)
    if(port == GPIOB)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
        // Enable and set EXTI Line Interrupt
        EnableIRQ(gpio_pin);
    }
    #endif
    #if defined (RCC_AHB1ENR_GPIOCEN)
    if(port == GPIOC)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);
        // Enable and set EXTI Line Interrupt
        EnableIRQ(gpio_pin);
    }
    #endif
    #if defined (RCC_AHB1ENR_GPIODEN)
    if(port == GPIOD)
    {
        __HAL_RCC_GPIOD_CLK_ENABLE();
        HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);
        // Enable and set EXTI Line Interrupt
        EnableIRQ(gpio_pin);
    }
    #endif
    #if defined (RCC_AHB1ENR_GPIOEEN)
    if(port == GPIOE)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
        HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);
        // Enable and set EXTI Line Interrupt
        EnableIRQ(gpio_pin);
    }
    #endif
    #if defined (RCC_AHB1ENR_GPIOFEN)
    if(port == GPIOF)
    {
        __HAL_RCC_GPIOF_CLK_ENABLE();
        HAL_GPIO_Init(GPIOF, &GPIO_InitStructure);
        // Enable and set EXTI Line Interrupt
        EnableIRQ(gpio_pin);
    }
    #endif
    #if defined (RCC_AHB1ENR_GPIOGEN)
    if(port == GPIOG)
    {
        __HAL_RCC_GPIOG_CLK_ENABLE();
        HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);
        // Enable and set EXTI Line Interrupt
        EnableIRQ(gpio_pin);
    }
    #endif
    #if defined (RCC_AHB1ENR_GPIOHEN)
    if(port == GPIOH)
    {
        __HAL_RCC_GPIOH_CLK_ENABLE();
        HAL_GPIO_Init(GPIOH, &GPIO_InitStructure);
        // Enable and set EXTI Line Interrupt
        EnableIRQ(gpio_pin);
    }
    #endif
    #if defined (RCC_AHB1ENR_GPIOIEN)
    if(port == GPIOI)
    {
        __HAL_RCC_GPIOI_CLK_ENABLE();
        HAL_GPIO_Init(GPIOI, &GPIO_InitStructure);
        // Enable and set EXTI Line Interrupt
        EnableIRQ(gpio_pin);
    }
    #endif
    #if defined (RCC_AHB1ENR_GPIOJEN)
    if(port == GPIOJ)
    {
        __HAL_RCC_GPIOJ_CLK_ENABLE();
        HAL_GPIO_Init(GPIOJ, &GPIO_InitStructure);
        // Enable and set EXTI Line Interrupt
        EnableIRQ(gpio_pin);
    }
    #endif
}

BOOL CPU_GPIO_Initialize()
{
    GPIO_InitTypeDef GPIO_InitStruct;

    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();

    CPU_GPIO_SetDebounce(20); // ???

    for(int i = 0; i < TOTAL_GPIO_PORT; i++)
    {
        g_pinReserved[i] = 0;
    }

    for(int i = 0; i < Gpio_MaxInt; i++)
    {
        g_int_state[i].completion.InitializeForISR(&GPIO_DebounceHandler);
    }

    // Configure all GPIO as analog to reduce current consumption on non used IOs
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Pin = GPIO_PIN_All;

    #if defined (RCC_AHB1ENR_GPIOAEN)
    #if !defined(BUILD_RTM)
    // don't change PA13 and PA14 as they maybe used in JTAG
    GPIO_InitStruct.Pin = GPIO_PIN_All & (~GPIO_PIN_13) & (~GPIO_PIN_14);
    #endif 
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    #if defined(BUILD_RTM)
    __HAL_RCC_GPIOA_CLK_DISABLE();
    #endif
    #endif
    #if defined (RCC_AHB1ENR_GPIOBEN)
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    __HAL_RCC_GPIOB_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOCEN)
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    __HAL_RCC_GPIOC_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIODEN)
    __HAL_RCC_GPIOD_CLK_ENABLE();
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    __HAL_RCC_GPIOD_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOEEN)
    __HAL_RCC_GPIOE_CLK_ENABLE();
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    __HAL_RCC_GPIOE_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOFEN)
    __HAL_RCC_GPIOF_CLK_ENABLE();
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    __HAL_RCC_GPIOF_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOGEN)
    __HAL_RCC_GPIOG_CLK_ENABLE(); 
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
    __HAL_RCC_GPIOG_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOHEN)
    __HAL_RCC_GPIOH_CLK_ENABLE(); 
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct); 
    __HAL_RCC_GPIOH_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOIEN)
    __HAL_RCC_GPIOI_CLK_ENABLE(); 
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
    __HAL_RCC_GPIOI_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOJEN)
    __HAL_RCC_GPIOJ_CLK_ENABLE(); 
    HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);
    __HAL_RCC_GPIOJ_CLK_DISABLE();
    #endif

    return TRUE;
}

BOOL CPU_GPIO_Uninitialize()
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();

    for(int i = 0; i < Gpio_MaxInt; i++)
    {
        g_int_state[i].completion.Abort();
    }

    // disable all GPIO external interrups and clock;
    #if defined (RCC_AHB1ENR_GPIOAEN)
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_All);
    __HAL_RCC_GPIOA_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOBEN)
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_All);
    __HAL_RCC_GPIOB_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOCEN)
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_All);
    __HAL_RCC_GPIOC_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIODEN)
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_All);
    __HAL_RCC_GPIOD_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOEEN)
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_All);
    __HAL_RCC_GPIOE_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOFEN)
    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_All);
    __HAL_RCC_GPIOF_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOGEN)
    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_All);
    __HAL_RCC_GPIOG_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOHEN)
    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_All);
    __HAL_RCC_GPIOH_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOIEN)
    HAL_GPIO_DeInit(GPIOI, GPIO_PIN_All);
    __HAL_RCC_GPIOI_CLK_DISABLE();
    #endif
    #if defined (RCC_AHB1ENR_GPIOJEN)
    HAL_GPIO_DeInit(GPIOJ, GPIO_PIN_All);
    __HAL_RCC_GPIOJ_CLK_DISABLE();
    #endif

    return TRUE;
}

UINT32 CPU_GPIO_Attributes(GPIO_PIN pin)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if(pin < Gpio_MaxPins)
    {
        return GPIO_ATTRIBUTE_INPUT | GPIO_ATTRIBUTE_OUTPUT;
    }
    return GPIO_ATTRIBUTE_NONE;
}

/*
 * alternate:
 * GPIO_ALT_PRIMARY: GPIO
 * GPIO_ALT_MODE_1: Analog
 * GPIO_ALT_MODE_2 | AF << 4 | speed << 8: Alternate Function
 * GPIO_ALT_MODE_3 | AF << 4 | speed << 8: Alternate Function with open drain
 * speed: 0: 2MHZ, 1: 25MHz, 2: 50MHz, 3: 100MHz
 */
void CPU_GPIO_DisablePin(GPIO_PIN pin, GPIO_RESISTOR resistor, UINT32 output, GPIO_ALT_MODE alternate)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if(pin < Gpio_MaxPins)
    {
        UINT32 mode = output;
        UINT32 altMode = (UINT32)alternate & 0x0F;
        
        if(altMode == 1)
            mode = 3; // analog
        else if(altMode)
            mode = 2; // alternate pin function

        GPIO_Pin_Config(pin, mode, resistor, (UINT32)alternate);
        GPIO_Set_Interrupt(pin, NULL, 0, GPIO_INT_NONE, FALSE); // disable interrupt
    }
}

void CPU_GPIO_EnableOutputPin(GPIO_PIN pin, BOOL initialState)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if(pin < Gpio_MaxPins)
    {
        CPU_GPIO_SetPinState(pin, initialState);
        GPIO_Pin_Config(pin, 1, RESISTOR_DISABLED, 0); // general purpose output
        GPIO_Set_Interrupt(pin, NULL, 0, GPIO_INT_NONE, FALSE); // disable interrupt
    }
}

BOOL CPU_GPIO_EnableInputPin(GPIO_PIN pin
                            , BOOL GlitchFilterEnable
                            , GPIO_INTERRUPT_SERVICE_ROUTINE ISR
                            , GPIO_INT_EDGE edge
                            , GPIO_RESISTOR resistor
                           )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    return CPU_GPIO_EnableInputPin2(pin, GlitchFilterEnable, ISR, 0, edge, resistor);
}

BOOL CPU_GPIO_EnableInputPin2(GPIO_PIN pin
                             , BOOL GlitchFilterEnable
                             , GPIO_INTERRUPT_SERVICE_ROUTINE ISR
                             , void* ISR_Param
                             , GPIO_INT_EDGE edge
                             , GPIO_RESISTOR resistor
                            )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if(pin >= Gpio_MaxPins)
        return FALSE;

    GPIO_Pin_Config(pin, 0, resistor, 0); // input
    return GPIO_Set_Interrupt(pin, ISR, ISR_Param, edge, GlitchFilterEnable);
}

BOOL CPU_GPIO_GetPinState(GPIO_PIN pin)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if(pin >= Gpio_MaxPins)
        return FALSE;

    GPIO_TypeDef* port = Port(pin >> 4); // pointer to the actual port registers 
    return (port->IDR >> (pin & 0xF)) & 1;
}

void CPU_GPIO_SetPinState(GPIO_PIN pin, BOOL pinState)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if(pin < Gpio_MaxPins)
    {
        GPIO_TypeDef* port = Port(pin >> 4); // pointer to the actual port registers 
        UINT16 bit = 1 << (pin & 0x0F);
        if(pinState)
            port->BSRR = bit; // set bit
        else
            port->BSRR = (uint32_t)bit << 16U; // reset bit
    }
}

BOOL CPU_GPIO_PinIsBusy(GPIO_PIN pin)  // busy == reserved
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if(pin >= Gpio_MaxPins)
        return FALSE;

    int port = pin >> 4, sh = pin & 0x0F;
    return (g_pinReserved[port] >> sh) & 1;
}

BOOL CPU_GPIO_ReservePin(GPIO_PIN pin, BOOL fReserve)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if(pin >= Gpio_MaxPins)
        return FALSE;

    int port = pin >> 4, bit = 1 << (pin & 0x0F);
    GLOBAL_LOCK(irq);
    if(fReserve)
    {
        if(g_pinReserved[port] & bit)
            return FALSE; // already reserved

        g_pinReserved[port] |= bit;
    }
    else
    {
        g_pinReserved[port] &= ~bit;
    }
    return TRUE;
}

UINT32 CPU_GPIO_GetDebounce()
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    return g_debounceTicks / (SLOW_CLOCKS_PER_SECOND / 1000); // ticks -> ms
}

BOOL CPU_GPIO_SetDebounce(INT64 debounceTimeMilliseconds)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if(debounceTimeMilliseconds > 0 && debounceTimeMilliseconds < 10000)
    {
        g_debounceTicks = CPU_MillisecondsToTicks((UINT32)debounceTimeMilliseconds);
        return TRUE;
    }
    return FALSE;
}

INT32 CPU_GPIO_GetPinCount()
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    return Gpio_MaxPins;
}

void CPU_GPIO_GetPinsMap(UINT8* pins, size_t size)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    for(int i = 0; i < size && i < Gpio_MaxPins; i++)
    {
        pins[i] = GPIO_ATTRIBUTE_INPUT | GPIO_ATTRIBUTE_OUTPUT;
    }
}

UINT8 CPU_GPIO_GetSupportedResistorModes(GPIO_PIN pin)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    return (1 << RESISTOR_DISABLED) | (1 << RESISTOR_PULLUP) | (1 << RESISTOR_PULLDOWN);
}

UINT8 CPU_GPIO_GetSupportedInterruptModes(GPIO_PIN pin)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    return (1 << GPIO_INT_EDGE_LOW) | (1 << GPIO_INT_EDGE_HIGH) | (1 << GPIO_INT_EDGE_BOTH)
        | (1 << GPIO_INT_LEVEL_LOW) | (1 << GPIO_INT_LEVEL_HIGH);
}

UINT32 CPU_GPIO_GetPinDebounce(GPIO_PIN pin)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    UINT32 num = pin & 0x0F;
    Int_State& state = g_int_state[num];

    return state.debounceTicks / (SLOW_CLOCKS_PER_SECOND / 1000); // ticks -> ms
}

BOOL CPU_GPIO_SetPinDebounce(GPIO_PIN pin, INT64 debounceTimeMilliseconds)
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    UINT32 num = pin & 0x0F;
    Int_State& state = g_int_state[num];

    if(debounceTimeMilliseconds > 0 && debounceTimeMilliseconds < 10000)
    {
        state.debounceTicks = CPU_MillisecondsToTicks((UINT32)debounceTimeMilliseconds);
        return TRUE;
    }
    return FALSE;
}
