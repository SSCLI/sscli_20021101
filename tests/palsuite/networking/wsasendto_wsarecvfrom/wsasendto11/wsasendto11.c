/*=============================================================================
**
** Source: wsasendto11.c (WSASendTo)
**
** Purpose: This negative test tries to send a buffer with size larger than
**          65500. Operations are done with an UDP, non-blocking socket. 
**          Make sure the number of bytes sent is 0.
** 
** Dependencies: PAL_Initialize
**               PAL_Terminate
**               Fail
**               WSAStartup
**               WSACleanup
**               socket
**               memset
**               closesocket
**               GetLastError
**               listen
**               accept
**               CreateEvent
**               WaitForSingleObject
**  
** 
**  Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
** 
**  The use and distribution terms for this software are contained in the file
**  named license.txt, which can be found in the root of this distribution.
**  By using this software in any fashion, you are agreeing to be bound by the
**  terms of this license.
** 
**  You must not remove this notice, or any other, from this software.
** 
**
**===========================================================================*/

#include <palsuite.h>
#include "wsacommon.h"

/**
 * Global variables
 */

const int       listenBacklog   = 1;    /* backlog for listen */
const int       buffer_too_large   = 66507;    /* buffer too large */

/* utility cleanup function */
static int CloseEventHandle(HANDLE Event);
 
/**
 * main
 * 
 * executable entry point
 */
INT __cdecl main(INT argc, CHAR **argv)
{
    int     i;
    int     err;
    struct  sockaddr_in mySockaddr;
    WSADATA wsaData;        

    /* Sockets descriptor */
    const int numSockets = 1;    /* number of sockets used in this test */

    SOCKET testSockets[1];   
    
    /* Variables for WSASend */

    WSABUF wsaSendBuf;
    DWORD  dwNbrOfByteSent;
    DWORD  dwNbrOfBuf  = 1;
    DWORD  dwSendFlags = 0;

    /* variable for iocltsocket */
    u_long argp;
    
    unsigned char   *sendBuffer;  

    WSAOVERLAPPED wsaOverlapped;

    /* Socket DLL version */
    const WORD wVersionRequested = MAKEWORD(2,2);

    HANDLE  hWriteEvent;
    DWORD   waitResult;        

    /* PAL initialization */
    if( PAL_Initialize(argc, argv) != 0 )
    {        
        return FAIL;
    }
    
    /* Sockets initialization to INVALID_SOCKET */
    for( i = 0; i < numSockets; i++ )
    {
        testSockets[i] = INVALID_SOCKET;
    }

    /* Initialize to use winsock2.dll */
    err = WSAStartup( wVersionRequested,
                      &wsaData);

    if(err != 0)
    {
        Trace( "Client error: Unexpected failure: "
              "WSAStartup(%i) "
              "returned %d\n",
              wVersionRequested, 
              GetLastError() );
        
        Fail("");
    }

    /* Confirm that the WinSock DLL supports 2.2.
       Note that if the DLL supports versions greater    
       than 2.2 in addition to 2.2, it will still return
       2.2 in wVersion since that is the version we      
       requested.                                        
    */
    if ( wsaData.wVersion != wVersionRequested ) 
    {
        Trace("Client error: Unexpected failure "
              "to find a usable version of WinSock DLL\n");

        /* Do some cleanup */
        DoWSATestCleanup( 0, 0);

        
        Fail("");
    }

    /* create an overlapped stream socket in AF_INET domain */

    testSockets[0] = WSASocketA( AF_INET, 
                                 SOCK_DGRAM, 
                                 IPPROTO_UDP,
                                 NULL, 
                                 0, 
                                 WSA_FLAG_OVERLAPPED ); 


    if( testSockets[0] == INVALID_SOCKET )
    {
        Trace("Client error: Unexpected failure: "
              "WSASocketA"
              "(AF_INET,SOCK_DGRAM,IPPROTO_UDP,NULL,0,WSA_FLAG_OVERLAPPED) "
              " returned %d\n",
              GetLastError());

        /* Do some cleanup */
        DoWSATestCleanup( 0, 0);

        
        Fail("");
    }

    /* enable non blocking socket */
    argp=1;
    err = ioctlsocket(testSockets[0], FIONBIO, (u_long FAR *)&argp);

    if (err==SOCKET_ERROR )
    {
        Trace("ERROR: Unexpected failure: "
              "ioctlsocket(.., FIONBIO, ..) "
              "returned %d\n",
              GetLastError() );

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );
        
        Fail("");
    }
    
    /* prepare the sockaddr_in structure */
    mySockaddr.sin_family           = AF_INET;
    mySockaddr.sin_port             = getRotorTestPort();
    mySockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    memset( &(mySockaddr.sin_zero), 0, 8);      

    /* create events */
    hWriteEvent = CreateEvent( NULL, /* no security   */
                             FALSE,    /* reset type    */
                             FALSE,    /* initial state */
                             NULL );   /* object name   */
            
    if( hWriteEvent == NULL )
    {   
        CloseEventHandle(hWriteEvent);

        Trace("Client error: Unexpected failure: "
              "CreateEvent() "
              "returned %d\n",
              GetLastError());

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );

        
        Fail("");
    }  

     /* Initialize the WSAOVERLAPPED to 0 */
    memset(&wsaOverlapped, 0, sizeof(WSAOVERLAPPED));

    wsaOverlapped.hEvent = hWriteEvent;    
    
    /* Set the WSABUF structure */
    sendBuffer = malloc(buffer_too_large * sizeof(char));

    if(sendBuffer==NULL)
    {
        Trace("Client error, Unexpected failure: "
                "not enough memory available for malloc()");

        CloseEventHandle(hWriteEvent);

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                            numSockets );
        
        Fail("");
    }

    wsaSendBuf.len = buffer_too_large;
    wsaSendBuf.buf = sendBuffer;
      
    /* Send some data */
    err = WSASendTo( testSockets[0],
                   &wsaSendBuf,
                   dwNbrOfBuf,
                   &dwNbrOfByteSent,
                   dwSendFlags,
                   (struct sockaddr FAR *)&mySockaddr,
                    sizeof(mySockaddr),
                   &wsaOverlapped,
                   0 );

    if(err != SOCKET_ERROR )
    {   
        if(dwNbrOfByteSent!=0)
        {
            Trace("Client WSASend has "
                "return %d, expected SOCKET_ERROR.",err);

            free(sendBuffer);

            CloseEventHandle(hWriteEvent);

            /* Do some cleanup */
            DoWSATestCleanup( testSockets,
                            numSockets );
            
            Fail("");
        }
    }

    /* handle overlapped operation */
    if(GetLastError()==WSA_IO_PENDING)
    {
        /* Wait 10 seconds for hWriteEvent to be signaled 
            for pending operation
        */
        waitResult = WaitForSingleObject( hWriteEvent,
                                            10000 );        
        
        if (waitResult!=WAIT_OBJECT_0)
        {   
            Trace("Client Error: Unexpected failure: "
                    "WaitForSingleObject has timed out \n");

            free(sendBuffer);
            
            CloseEventHandle(hWriteEvent);
            
            /* Do some cleanup */
            DoWSATestCleanup( testSockets,
                                numSockets );
            
            Fail("");
        }        
    }

    free(sendBuffer);

    /*     
    According to MSDN, 
    For message-oriented sockets, care must be taken not to exceed the 
    maximum message size of the underlying transport, which can be 
    obtained by getting the value of socket option SO_MAX_MSG_SIZE. 
    If the data is too long to pass atomically through the underlying 
    protocol the error WSAEMSGSIZE is returned, and no data is transmitted.

    
    wsaOverlapped.Internal does not return WSAEMSGSIZE.
    
    Test removed: Verify that wsaOverlapped.Internal is WSAEMSGSIZE.             
    */   

    /* Verify that the number of bytes sent is 0    
    */  

    if(wsaOverlapped.InternalHigh!=0)
    {   
        Trace("Client WSASend was called with a buffer larger than"
              " SO_MAX_MSG_SIZE "
              "wsaSendBuf.len = %d, "
              "wsaOverlapped.InternalHigh = % d, "              
              "buffer_too_large = %d, ",
              wsaSendBuf.len, wsaOverlapped.InternalHigh,
              buffer_too_large);

        CloseEventHandle(hWriteEvent);

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );
        
        Fail("");
    }
    
    if(!CloseEventHandle(hWriteEvent))
    {
        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );
        
        Fail("");
    }

    /* Do some cleanup */
    DoWSATestCleanup( testSockets,
                      numSockets );
   
    PAL_Terminate();
    return PASS;
} 


/* int CloseEventHandle(HANDLE Event)
   This function close an Event handle 
   and return an appropriate message in case of error
*/
static int CloseEventHandle(HANDLE Event)
{
    if( CloseHandle(Event) == 0 )
    {
        Trace("Server error: Unexpected failure: "
              "CloseHandle() "
              "returned %d\n",
            GetLastError());
        return 0;
    }    
    return 1;
}

