/*=============================================================================
**
** Source: wsarecvfrom7_client.c (WSASend, WSARecvFrom)
**
** Purpose: This program tests wsarecvfrom. A Client 
**          sends a simple buffer with a TCP, Blocking and 
**          overlapped socket. The server will receive that buffer
**          by calling WSARecvFrom with a TCP, Blocking and 
**          overlapped socket.
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

    HANDLE  writeEvent;
    DWORD   waitResult;

    /* Sockets descriptor */
    const int numSockets = 1;    /* number of sockets used in this test */

    SOCKET testSockets[1];

    /* Variables for WSASend */
    WSABUF wsaBuf[1];
    DWORD  dwNbrOfByteSent;
    DWORD  dwNbrOfBuf  = 1;
    DWORD  dwSendFlags = 0;
    char   myBuffer[255] = "violets are blue, roses are red";    

    WSAOVERLAPPED wsaOverlapped;

    /* Socket DLL version */
    const WORD wVersionRequested = MAKEWORD(2,2);

    /* Sockets initialization to INVALID_SOCKET */
    for( i = 0; i < numSockets; i++ )
    {
        testSockets[i] = INVALID_SOCKET;
    }

    /* PAL initialization */
    if( PAL_Initialize(argc, argv) != 0 )
    {
        return FAIL;
    }
   
    /* Initialize to use winsock2.dll */
    err = WSAStartup( wVersionRequested,
                      &wsaData);

    if(err != 0)
    {
        Fail( "ERROR: Unexpected failure: "
              "WSAStartup(%i) "
              "returned %d\n",
              wVersionRequested, 
              GetLastError() );
    }

    /* Confirm that the WinSock DLL supports 2.2.
       Note that if the DLL supports versions greater    
       than 2.2 in addition to 2.2, it will still return
       2.2 in wVersion since that is the version we      
       requested.                                        
    */
    if ( wsaData.wVersion != wVersionRequested ) 
    {
         
        Trace("ERROR: Unexpected failure "
              "to find a usable version of WinSock DLL\n");

        /* Do some cleanup */
        DoWSATestCleanup( 0, 0);

        Fail("");
    }

    /* create an overlapped stream socket in AF_INET domain */

    testSockets[0] = WSASocketA( AF_INET, 
                                 SOCK_STREAM, 
                                 IPPROTO_TCP,
                                 NULL, 
                                 0, 
                                 WSA_FLAG_OVERLAPPED ); 


    if( testSockets[0] == INVALID_SOCKET )

    {
        Trace("ERROR: Unexpected failure: "
              "WSASocketA"
              "(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED)) "
              " returned %d\n",
              GetLastError());

        /* Do some cleanup */
        DoWSATestCleanup( 0, 0);

        Fail("");
    }
    
    /* prepare the sockaddr_in structure */
    mySockaddr.sin_family           = AF_INET;
    mySockaddr.sin_port             = getRotorTestPort();
    mySockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    memset( &(mySockaddr.sin_zero), 0, 8);    

    /* connect to a server */
    err = connect( testSockets[0], 
                   (struct sockaddr *)&mySockaddr,
                   sizeof(struct sockaddr));

    if( err == SOCKET_ERROR )
    {
        Trace("ERROR: Unexpected failure: "
              "connect() socket with local server "
              "returned %d\n",
              GetLastError());

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );
        Fail("");
    }

    /* Set the WSABUF structure */
    wsaBuf[0].len = 255;
    wsaBuf[0].buf = myBuffer;

    /* create an event */
    writeEvent = CreateEvent( NULL, /* no security */
                             FALSE, /* reset type */
                             FALSE, /* initial state */
                             NULL );  /* object name */
                
    if( writeEvent == NULL )
    {
        Trace("ERROR: Unexpected failure: "
              "CreateEvent() "
              "returned %d\n",
              GetLastError());

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );

        Fail("");
    }

    /* Set the WSAOVERLAPPED to zero */
    memset(&wsaOverlapped, 0, sizeof(WSAOVERLAPPED));

    /* Specify which event to signal when data is arrived*/
    wsaOverlapped.hEvent = writeEvent;
    
    /* Send some data */
    err = WSASend( testSockets[0],
                   wsaBuf,
                   dwNbrOfBuf,
                   &dwNbrOfByteSent,
                   dwSendFlags,
                   &wsaOverlapped,
                   0 );

    if(err == SOCKET_ERROR)
    {   
        if(GetLastError() != WSA_IO_PENDING)
        {
            Trace("ERROR: Unexpected failure: "
                  "WSASend(...) "
                  "returned %d\n",
                  GetLastError());

            /* Do some cleanup */
            DoWSATestCleanup( testSockets,
                              numSockets );        

            /* close the event */
            if( CloseHandle(writeEvent) == 0 )
            {
                Trace("ERROR: Unexpected failure: "
                      "CloseHandle() "
                      "returned %d\n",
                      GetLastError());
            }

            Fail("");
        }

        /* Wait 10 seconds for writeEvent to be signaled */
        waitResult = WaitForSingleObject( writeEvent,  /* handle to object */
                                      10000 );    /* 10 sec time-out interval*/

        if(waitResult!=WAIT_OBJECT_0)
        {
            Trace("ERROR: Unexpected failure: "
                  "WSASend(...) "
                  "returned %d\n",
                  GetLastError());

            /* Do some cleanup */
            DoWSATestCleanup( testSockets,
                              numSockets );        

            /* close the event */
            if( CloseHandle(writeEvent) == 0 )
            {
                Trace("ERROR: Unexpected failure: "
                      "CloseHandle() "
                      "returned %d\n",
                      GetLastError());
            }

            Fail("");
        }

    }
    else
    {
        /* if no error occured, number of byte sent should 
           be 255.
        */        
        if(dwNbrOfByteSent!=255)
        {
            Trace("client error, Unexpected failure: "
                  "WSASend(...) returned immediately"
                  "but dwNbrOfByteSent(%d) is different from the 255\n",
                  dwNbrOfByteSent);

            /* Do some cleanup */
            DoWSATestCleanup( testSockets,
                              numSockets );        

            /* close the event */
            if( CloseHandle(writeEvent) == 0 )
            {
                Trace("ERROR: Unexpected failure: "
                      "CloseHandle() "
                      "returned %d\n",
                      GetLastError());
            }

            Fail("");
        }
    }


    /* close the event */
    if( CloseHandle(writeEvent) == 0 )
    {
        Trace("ERROR: Unexpected failure: "
              "CloseHandle() "
              "returned %d\n",
              GetLastError());
        
        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );

        Fail("");
    }


    /* Disconnect the socket */
    err = shutdown( testSockets[0], 
                    SD_BOTH);

    if( err == SOCKET_ERROR )
    {
        Trace("ERROR: Unexpected failure: "
              "shutdown() socket "
              "returned %d\n",
              GetLastError());

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
