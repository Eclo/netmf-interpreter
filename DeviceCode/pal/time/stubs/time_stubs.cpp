////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Microsoft Corporation.  All rights reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <tinyhal.h>

__weak HRESULT Time_Initialize()
{
    return CLR_E_NOTIMPL;
}

__weak HRESULT Time_Uninitialize()
{
    return CLR_E_NOTIMPL;
}

__weak INT64 Time_GetUtcTime()
{
    return 0;
}

__weak INT64 Time_SetUtcTime( INT64 UtcTime, bool calibrate )
{
    return 0;
}

__weak INT64 Time_GetLocalTime()
{
    return 0;
}

__weak INT32 Time_GetTimeZoneOffset()
{
    return 0;
}

__weak INT32 Time_SetTimeZoneOffset(INT32 offset)
{
    return 0;
}

__weak INT64 Time_GetTickCount()
{
    return 0;
}

__weak INT64 Time_GetMachineTime()
{
    return 0;
}

__weak BOOL Time_ToSystemTime(INT64 time, SYSTEMTIME* systemTime)
{
    return FALSE;
}

__weak INT64 Time_FromSystemTime(const SYSTEMTIME* systemTime)
{
    return 0;
}

__weak HRESULT Time_DaysInMonth(INT32 year, INT32 month, INT32* days)
{
    return CLR_E_NOTIMPL;
}

__weak HRESULT Time_AccDaysInMonth(INT32 year, INT32 month, INT32* days)
{
    return CLR_E_NOTIMPL;
}

__weak BOOL Utility_SafeSprintfV( LPSTR& szBuffer, size_t& iBuffer, LPCSTR format, va_list arg )
{
    return FALSE;
}

__weak BOOL Utility_SafeSprintf( LPSTR& szBuffer, size_t& iBuffer, LPCSTR format, ... )
{
    return FALSE;
}

__weak BOOL Time_TimeSpanToStringEx( const INT64& ticks, LPSTR& buf, size_t& len )
{
    return FALSE;
}

__weak LPCSTR Time_TimeSpanToString( const INT64& ticks )
{
    return NULL;
}

__weak BOOL Time_DateTimeToStringEx( const INT64& time, LPSTR& buf, size_t& len )
{   
    return FALSE;
}

__weak LPCSTR Time_DateTimeToString( const INT64& time)
{
    return NULL;
}

__weak LPCSTR Time_CurrentDateTimeToString()
{
    return NULL;
}
