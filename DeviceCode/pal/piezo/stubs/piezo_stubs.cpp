////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Microsoft Corporation.  All rights reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "tinyhal.h"

//--//

__weak void Piezo_Initialize()
{
}

__weak void Piezo_Uninitialize()
{
}

__weak BOOL Piezo_Tone( UINT32 Frequency_Hertz, UINT32 Duration_Milliseconds )
{
    return TRUE;
}

__weak BOOL Piezo_IsEnabled()
{
    return FALSE;
}
