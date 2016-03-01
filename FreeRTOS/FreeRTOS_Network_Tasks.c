/*
*/

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

// prototypes
static void prvServerConnectionInstance( void *pvParameters );
static void prvConnectionListeningTask( void *pvParameters );

/*-----------------------------------------------------------*/

/* The maximum time to wait for a closing socket to close. */
#define tcpechoSHUTDOWN_DELAY	( pdMS_TO_TICKS( 5000 ) )
/* The standard echo port number. */
#define tcpechoPORT_NUMBER		7
/* If ipconfigUSE_TCP_WIN is 1 then the Tx sockets will use a buffer size set by
ipconfigTCP_TX_BUF_LEN, and the Tx window size will be
configECHO_SERVER_TX_WINDOW_SIZE times the buffer size.  Note
ipconfigTCP_TX_BUF_LEN is set in FreeRTOSIPConfig.h as it is a standard TCP/IP
stack constant, whereas configECHO_SERVER_TX_WINDOW_SIZE is set in
FreeRTOSConfig.h as it is a demo application constant. */
#ifndef configECHO_SERVER_TX_WINDOW_SIZE
	#define configECHO_SERVER_TX_WINDOW_SIZE	2
#endif
/* If ipconfigUSE_TCP_WIN is 1 then the Rx sockets will use a buffer size set by
ipconfigTCP_RX_BUF_LEN, and the Rx window size will be
configECHO_SERVER_RX_WINDOW_SIZE times the buffer size.  Note
ipconfigTCP_RX_BUF_LEN is set in FreeRTOSIPConfig.h as it is a standard TCP/IP
stack constant, whereas configECHO_SERVER_RX_WINDOW_SIZE is set in
FreeRTOSConfig.h as it is a demo application constant. */
#ifndef configECHO_SERVER_RX_WINDOW_SIZE
	#define configECHO_SERVER_RX_WINDOW_SIZE	2
#endif

/* Stores the stack size passed into vStartSimpleTCPServerTasks() so it can be
reused when the server listening task creates tasks to handle connections. */
static uint16_t usUsedStackSize = 0;

/* Use by the pseudo random number generator. */
static UBaseType_t ulNextRand;

/*-----------------------------------------------------------*/

extern void vStartSimpleTCPServerTasks( uint16_t usStackSize, UBaseType_t uxPriority )
{
	/* Create the TCP echo server. */
	xTaskCreate( prvConnectionListeningTask, "ServerListener", usStackSize, NULL, uxPriority + 1, NULL );

	/* Remember the requested stack size so it can be re-used by the server
	listening task when it creates tasks to handle connections. */
	usUsedStackSize = usStackSize;
}

/*-----------------------------------------------------------*/
static void prvConnectionListeningTask( void *pvParameters )
{
    struct freertos_sockaddr xClient, xBindAddress;
    Socket_t xListeningSocket, xConnectedSocket;
    socklen_t xSize = sizeof( xClient );
    static const TickType_t xReceiveTimeOut = portMAX_DELAY;
    const BaseType_t xBacklog = 20;

#if( ipconfigUSE_TCP_WIN == 1 )
	WinProperties_t xWinProps;

	/* Fill in the buffer and window sizes that will be used by the socket. */
	xWinProps.lTxBufSize = ipconfigTCP_TX_BUF_LEN;
	xWinProps.lTxWinSize = configECHO_SERVER_TX_WINDOW_SIZE;
	xWinProps.lRxBufSize = ipconfigTCP_RX_BUF_LEN;
	xWinProps.lRxWinSize = configECHO_SERVER_RX_WINDOW_SIZE;
#endif /* ipconfigUSE_TCP_WIN */

	/* Just to prevent compiler warnings. */
	( void ) pvParameters;

	/* Attempt to open the socket. */
	xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
	configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

	/* Set a time out so accept() will just wait for a connection. */
	FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );

	/* Set the window and buffer sizes. */
	#if( ipconfigUSE_TCP_WIN == 1 )
	{
		FreeRTOS_setsockopt( xListeningSocket, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProps, sizeof( xWinProps ) );
	}
	#endif /* ipconfigUSE_TCP_WIN */

	/* Bind the socket to the port that the client task will send to, then
	listen for incoming connections. */
	xBindAddress.sin_port = tcpechoPORT_NUMBER;
	xBindAddress.sin_port = FreeRTOS_htons( xBindAddress.sin_port );
	FreeRTOS_bind( xListeningSocket, &xBindAddress, sizeof( xBindAddress ) );
	FreeRTOS_listen( xListeningSocket, xBacklog );

	for( ;; )
	{
		/* Wait for a client to connect. */
		xConnectedSocket = FreeRTOS_accept( xListeningSocket, &xClient, &xSize );
		configASSERT( xConnectedSocket != FREERTOS_INVALID_SOCKET );

		/* Spawn a task to handle the connection. */
		xTaskCreate( prvServerConnectionInstance, "EchoServer", usUsedStackSize, ( void * ) xConnectedSocket, tskIDLE_PRIORITY, NULL );
	}
}
/*-----------------------------------------------------------*/

static void prvServerConnectionInstance( void *pvParameters )
{
    int32_t lBytes, lSent, lTotalSent;
    Socket_t xConnectedSocket;
    static const TickType_t xReceiveTimeOut = pdMS_TO_TICKS( 5000 );
    static const TickType_t xSendTimeOut = pdMS_TO_TICKS( 5000 );
    TickType_t xTimeOnShutdown;
    uint8_t *pucRxBuffer;

	xConnectedSocket = ( Socket_t ) pvParameters;

	/* Attempt to create the buffer used to receive the string to be echoed
	back.  This could be avoided using a zero copy interface that just returned
	the same buffer. */
	pucRxBuffer = ( uint8_t * ) pvPortMalloc( ipconfigTCP_MSS );

	if( pucRxBuffer != NULL )
	{
		FreeRTOS_setsockopt( xConnectedSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof( xReceiveTimeOut ) );
		FreeRTOS_setsockopt( xConnectedSocket, 0, FREERTOS_SO_SNDTIMEO, &xSendTimeOut, sizeof( xReceiveTimeOut ) );

		for( ;; )
		{
			/* Zero out the receive array so there is NULL at the end of the string
			when it is printed out. */
			memset( pucRxBuffer, 0x00, ipconfigTCP_MSS );

			/* Receive data on the socket. */
			lBytes = FreeRTOS_recv( xConnectedSocket, pucRxBuffer, ipconfigTCP_MSS, 0 );

			/* If data was received, echo it back. */
			if( lBytes >= 0 )
			{
				lSent = 0;
				lTotalSent = 0;

				/* Call send() until all the data has been sent. */
				while( ( lSent >= 0 ) && ( lTotalSent < lBytes ) )
				{
					lSent = FreeRTOS_send( xConnectedSocket, pucRxBuffer, lBytes - lTotalSent, 0 );
					lTotalSent += lSent;
				}

				if( lSent < 0 )
				{
					/* Socket closed? */
					break;
				}
			}
			else
			{
				/* Socket closed? */
				break;
			}
		}
	}

	/* Initiate a shutdown in case it has not already been initiated. */
	FreeRTOS_shutdown( xConnectedSocket, FREERTOS_SHUT_RDWR );

	/* Wait for the shutdown to take effect, indicated by FreeRTOS_recv()
	returning an error. */
	xTimeOnShutdown = xTaskGetTickCount();
	do
	{
		if( FreeRTOS_recv( xConnectedSocket, pucRxBuffer, ipconfigTCP_MSS, 0 ) < 0 )
		{
			break;
		}
	} while( ( xTaskGetTickCount() - xTimeOnShutdown ) < tcpechoSHUTDOWN_DELAY );

	/* Finished with the socket, buffer, the task. */
	vPortFree( pucRxBuffer );
	FreeRTOS_closesocket( xConnectedSocket );

	vTaskDelete( NULL );
}

/*-----------------------------------------------------------*/

static void prvSRand( UBaseType_t ulSeed )
{
	/* Utility function to seed the pseudo random number generator. */
	ulNextRand = ulSeed;
}
/*-----------------------------------------------------------*/

UBaseType_t uxRand( void )
{
    const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL;
    static BaseType_t xInitialised = pdFALSE;

	/* Don't initialise until the scheduler is running, as the timeout in the
	random number generator uses the tick count. */
	if( xInitialised == pdFALSE )
	{
		if( xTaskGetSchedulerState() !=  taskSCHEDULER_NOT_STARTED )
		{
// 		RNG_HandleTypeDef xRND;
// 		uint32_t ulSeed;
// 
// 			/* Generate a random number with which to seed the local pseudo random
// 			number generating function. */
// 			HAL_RNG_Init( &xRND );
// 			HAL_RNG_GenerateRandomNumber( &xRND, &ulSeed );
// 			prvSRand( ulSeed );
// 			xInitialised = pdTRUE;
		}
	}

	/* Utility function to generate a pseudo random number. */

	ulNextRand = ( ulMultiplier * ulNextRand ) + ulIncrement;
	return( ( int ) ( ulNextRand >> 16UL ) & 0x7fffUL );
}
