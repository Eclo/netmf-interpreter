////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  Implementation for CMSIS: Copyright (c) Eclo Solutions Lda.
//
//  *** USB WinUSB device for debug ***
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <tinyhal.h>
#include <pal\com\usb\USB.h>


#include "usbd_desc.h"
#include "usbd_conf.h"
#include "usbd_core.h"
#include "usbd_winusb.h"

//--//
USB_CONTROLLER_STATE usbControlerState;
static UINT8 ep0Buffer[64];
static UINT16 endpointStatus[USB_MAX_QUEUES];
static UINT16 EP_Type;
UINT8 previousDeviceState;

/* Queues for all data endpoints */
Hal_Queue_KnownSize<USB_PACKET64, USB_QUEUE_PACKET_COUNT> QueueBuffers[4];

USBD_HandleTypeDef  USBD_Device;
extern PCD_HandleTypeDef hpcd;

USB_CONTROLLER_STATE *CPU_USB_GetState(int Controller)
{
    //return NULL;
    return &usbControlerState;
}

HRESULT CPU_USB_Initialize(int Controller)
{
    // setup usb state variables
    usbControlerState.EndpointStatus = endpointStatus;
    
    // FIXME
    usbControlerState.EndpointCount = 2;
    
    // FIXME
    // get max ep0 packet size from actual configuration
    //const USB_DEVICE_DESCRIPTOR* desc = ( USB_DEVICE_DESCRIPTOR* )USB_FindRecord( &usbControlerState, USB_DEVICE_DESCRIPTOR_MARKER, 0 );
    //usbControlerState.PacketSize = desc ? desc->bMaxPacketSize0 : 8;
    usbControlerState.PacketSize = WINUSB_MAX_FS_PACKET;
 
    // config endpoints
    // EP1
    // IN direction 
    usbControlerState.IsTxQueue[0] = TRUE;
    // max packet size
    usbControlerState.MaxPacketSize[0] = WINUSB_MAX_FS_PACKET;
    // assign queues
    // clear queue before use
    QueueBuffers[0].Initialize();
    // Attach queue to endpoint
    usbControlerState.Queues[0] = &QueueBuffers[0];  
    
    // EP2
    // OUT direction
    usbControlerState.IsTxQueue[1] = FALSE;
    // max packet size
    usbControlerState.MaxPacketSize[1] = WINUSB_MAX_FS_PACKET;
    // assign queues
    // clear queue before use
    QueueBuffers[1].Initialize();
    // Attach queue to endpoint
    usbControlerState.Queues[1] = &QueueBuffers[1];  
    
    // ?????????????
    EP_Type = 0xE0;
    
    USBD_Device.pUserData = &usbControlerState;
    
    // Init Device Library
    USBD_Init(&USBD_Device, &WINUSB_Desc, 0);
    
    // Add Supported Class
    USBD_RegisterClass(&USBD_Device, USBD_WINUSB_CLASS);

    /* Add WinUSB callbacks */
    //USBD_WINUSB_RegisterInterface(&USBD_Device, &USBD_WINUSB_fops);
    
    // Start Device Process
    USBD_Start(&USBD_Device);

    return S_OK;
}

HRESULT CPU_USB_Uninitialize(int Controller)
{
    USBD_Stop(&USBD_Device);
    USBD_DeInit(&USBD_Device);

    return S_OK;
}

BOOL CPU_USB_StartOutput(USB_CONTROLLER_STATE* state, int endpoint)
{
    if(state == NULL || endpoint >= state->EndpointCount)
    {
        return FALSE;
    }
        
    //OTG_TypeDef* OTG = STM32F4_USB_REGS( State->ControllerNum );

    //USB_Debug( 't' );

    GLOBAL_LOCK(irq);

    // If endpoint is not an output
    if(state->Queues[endpoint] == NULL || !state->IsTxQueue[endpoint])
    {
        return FALSE;        
    }

    /* if the halt feature for this endpoint is set, then just clear all the characters */
    if(state->EndpointStatus[endpoint] & USB_STATUS_ENDPOINT_HALT)
    {
        while(USB_TxDequeue(state, endpoint, TRUE) != NULL)
        {
        } // clear TX queue

        return TRUE;
    }

    if(irq.WasDisabled())
    { 
        // check all endpoints for pending actions
        //STM32F4_USB_Driver_Interrupt( OTG, state );
    }
    
    // FIXME
    // write first packet if not done yet
    //USBD_WINUSB_DataIn(&USBD_Device, endpoint);

    return TRUE;
}

BOOL CPU_USB_RxEnable(USB_CONTROLLER_STATE* state, int endpoint)
{
    // If this is not a legal Rx queue
    if(state == NULL || state->Queues[endpoint] == NULL || state->IsTxQueue[endpoint])
    {
        return FALSE;        
    }

    //OTG_TypeDef* OTG = STM32F4_USB_REGS( State->ControllerNum );

    //USB_Debug( 'e' );

    GLOBAL_LOCK(irq);

    // enable Rx
    // if( !( OTG->DOEP[ endpoint ].CTL & OTG_DOEPCTL_EPENA ) )
    // {
    //     OTG->DOEP[ endpoint ].TSIZ = OTG_DOEPTSIZ_PKTCNT_1 | State->MaxPacketSize[ endpoint ];
    //     OTG->DOEP[ endpoint ].CTL |= OTG_DOEPCTL_EPENA | OTG_DOEPCTL_CNAK; // enable endpoint
    // }

    return TRUE;
}

BOOL CPU_USB_GetInterruptState()
{
    return FALSE;
}

BOOL CPU_USB_ProtectPins(int controller, BOOL On)
{
    return TRUE;
}

/**
  * @brief  This function handles USB-On-The-Go FS/HS global interrupt request.
  * @param  None
  * @retval None
  */
// must declare this as extern C funtion otherwise it won't be recognized by the linker as a valid interrupt handler  
extern "C" void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd);
}
