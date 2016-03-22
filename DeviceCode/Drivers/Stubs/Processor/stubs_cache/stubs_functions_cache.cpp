////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Microsoft Corporation.  All rights reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

//--//

__weak void CPU_FlushCaches()
{
}

__weak void CPU_DrainWriteBuffers()
{
}

__weak void CPU_InvalidateCaches()
{
}

__weak void CPU_EnableCaches()
{
}

__weak void CPU_DisableCaches()
{
}

//--//

__weak template <typename T> void CPU_InvalidateAddress( T* address )
{
}

__weak template <typename T> void CPU_InvalidateAddress( volatile T* address )
{
}

//--//

__weak size_t CPU_GetCachableAddress( size_t address )
{
    return address;
}

//--//

__weak size_t CPU_GetUncachableAddress( size_t address )
{
    return address;
}
