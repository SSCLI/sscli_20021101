/*=============================================================================
**
** Source: wsasendto6.c (WSASendTo, WSARecv)
**
** Purpose: This test sends and receives a large amount of buffer. At 400th 
**          WSASendTo, a shutdown is done on the server. Then the client 
**          should immediately stop sending data and close normally. A check 
**          is done to verify that information has been sent correctly. The 
**          operations are TCP, Blocking and Overlapped.
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
const int       THREAD_FAIL     = 0; /* server port number */
const int       THREAD_SUCCESS  = 1;    /* backlog for listen */
const int       THREAD_UNDEFINED = 2;    /* backlog for listen */

/* set the size of the buffer */
const u_long    so_max_msg_size = 0xfffffff;

/* Thread that is used as a client */
void PALAPI Thread_Client(LPVOID lpParam);

/* utility cleanup function */
static int CloseEventHandle(HANDLE Event);
static int CloseThreadHandle(HANDLE thread);
static int WaitForClientThreadToFinish(HANDLE thread);
static int CloseSocket(SOCKET sockList[], int numSockets);

 
/* We use a global variable to keep track of the 
   client thread return value because GetExitCodeThread
   is not supported on PAL
*/
int threadExitCode;

/**
 * main
 * 
 * executable entry point
 */
INT __cdecl main(INT argc, CHAR **argv)
{
    int     i;
    int     err;    
    int     addrlen = sizeof(struct sockaddr);
    struct  sockaddr_in mySockaddr;
    WSADATA wsaData;
    HANDLE  hReadEvent;
    DWORD   waitResult;

    /* Thread variable */
    HANDLE hThreadClient;
    DWORD dwThreadClient;     
    DWORD dwClientParam[2];     

    /* Sockets descriptor */
    const int numSockets = 2;    /* number of sockets used in this test */

    SOCKET testSockets[2];

     /* Variables needed for setsockopt */
    BOOL bReuseAddr = TRUE;    


    /* Variables needed for select */
    struct timeval waitTime;
    fd_set readFds;
    int    socketFds;

    /* Variables needed for WSARecv */
    WSABUF        wsaBuf;
    DWORD         dwNbrOfBuf  = 1;
    DWORD         dwNbrOfByteSent;
    DWORD         dwRecvFlags = 0;
    WSAOVERLAPPED wsaRecvOverlapped;
    
    /* Variable used to store transmitted data */
    unsigned char myBuffer[255];
    unsigned char myData[500][255];
    unsigned char* pMyData;
    
    int bufferCounter;
    
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
        Fail( "Server error: Unexpected failure: "
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
        Trace("Server error: Unexpected failure "
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
        Trace("Server error: Unexpected failure: "
              "WSASocketA"
              "(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED)) "
              " returned %d\n",
              GetLastError());

        /* Do some cleanup */
        DoWSATestCleanup( 0, 0);

        Fail("");
    }

    /* Allows the socket to be bound to an address that is already in use. */
    err = setsockopt( testSockets[0],
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      (const char *)&bReuseAddr,
                      sizeof( BOOL ) );

    if( err == SOCKET_ERROR )
    {
        Trace("Server error: Unexpected failure: "
              "setsockopt(.., SOL_SOCKET,SO_REUSEADDR, ..) "
              "returned %d\n",
              GetLastError() );

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );

        Fail("");
    }

    /* prepare the sockaddr structure */

    mySockaddr.sin_family           = AF_INET;
    mySockaddr.sin_port             = getRotorTestPort();
    mySockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    memset( &(mySockaddr.sin_zero), 0, 8);

    /* bind local address to a socket */
    err = bind( testSockets[0],
                (struct sockaddr *)&mySockaddr,
                sizeof(struct sockaddr) );


    if( err == SOCKET_ERROR )
    {
        Trace("ERROR: Unexpected failure: "
              "bind() socket with local address "
              "returned %d\n",
              GetLastError() );

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );

        Fail("");
    }

    /* listen to the socket */
    err = listen( testSockets[0], 
                  listenBacklog );

    if( err == SOCKET_ERROR )
    {
        Trace("ERROR: Unexpected failure: "
              "listen() to sockets "
              "returned %d\n",
              GetLastError() );

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );

        Fail("");
    }
    
    /* create a client thread */

    hThreadClient = 
        CreateThread( 
                NULL,                        /* no security attributes */
                0,                           /* use default stack size */
                (LPTHREAD_START_ROUTINE)Thread_Client,/* thread function    */
                (LPVOID)&dwClientParam,      /* argument to thread function */
                0,                           /* use default creation flags  */
                &dwThreadClient);            /* returns the thread identifier*/

    if(hThreadClient==NULL)
    {        
        Trace( "Server Error: Unexpected failure: "
              "CreateThread() "
              "returned NULL\n");        

          /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );

        Fail("");
    }

    /* set the server waiting time as 10 seconds */
    waitTime.tv_sec = 10L;
    waitTime.tv_usec = 0L;

    /* initialize the except socket set  */
    FD_ZERO( &readFds );

    /* add socket to readable socket set */
    FD_SET( testSockets[0], 
            &readFds );

    /* monitor the readable socket set   */
    socketFds = select( 0,
                        &readFds,
                        NULL,
                        NULL,
                        &waitTime);   

    if( socketFds == SOCKET_ERROR )
    {
        Trace("ERROR: Unexpected failure "
              "with select\n");

        WaitForClientThreadToFinish(hThreadClient);

        CloseThreadHandle(hThreadClient);        

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                        numSockets );

        Fail("");
    }

    if( socketFds == 0 )
    {
        Trace("ERROR: Unexpected select "
              "timed out\n");

        WaitForClientThreadToFinish(hThreadClient);

        CloseThreadHandle(hThreadClient);        

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                        numSockets );

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );

        Fail("");
    }

    /* accept connection */
    testSockets[1] = accept( testSockets[0],
                             (struct sockaddr *)&mySockaddr,
                             &addrlen );

    if( testSockets[1] == INVALID_SOCKET )
    {
        Trace("ERROR: Unexpected failure: "
              "accept() connection on socket "
              "returned %d\n",
              GetLastError());

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );

        Fail("");
    }
    
    /* create an event */
    hReadEvent = CreateEvent( NULL, /* no security   */
                             FALSE,   /* reset type    */
                             FALSE,   /* initial state */
                             NULL );  /* object name   */
            
    if( hReadEvent == NULL )
    {            
        Trace("Server error: Unexpected failure: "
              "CreateEvent() "
              "returned %d\n",
              GetLastError());

        WaitForClientThreadToFinish(hThreadClient);
        

        CloseThreadHandle(hThreadClient);        

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );

        Fail("");
        
    }

     /* Initialize the WSABUF structure */
    wsaBuf.buf = myBuffer;
    wsaBuf.len = 255;
    
    bufferCounter = 0;
    pMyData = (unsigned char*)myData;

    /* loop that receive data from client 
       It received 400 buffer then proceed to a shutdown 
       on the receiving socket.
    */
    for(i=0;i<400;i++)
    {
        /* Initialize the WSAOVERLAPPED to 0 */
        memset(&wsaRecvOverlapped, 0, sizeof(WSAOVERLAPPED));
        /* Specify which event to signal when data is arrived*/
        wsaRecvOverlapped.hEvent = hReadEvent;

        memset(myBuffer, 0, 255);        

        /* Prepare to receive data */
        err = WSARecv( testSockets[1],
                    &wsaBuf,
                    dwNbrOfBuf,
                    &dwNbrOfByteSent,
                    &dwRecvFlags,
                    &wsaRecvOverlapped,
                    0 );

        if( err != SOCKET_ERROR )
        {           
            if(dwNbrOfByteSent==0)
            {   
                Trace("Server error: WSARecv() "
                    "received %d, expected 0.\n",
                    dwNbrOfByteSent);            
                
                WaitForClientThreadToFinish(hThreadClient);                

                CloseThreadHandle(hThreadClient);

                CloseEventHandle(hReadEvent);                

                /* Do some cleanup */
                DoWSATestCleanup( testSockets,
                                numSockets );

                Fail("");
            }
            /* reset the event */
            ResetEvent(hReadEvent);
        }
        else if(GetLastError()!=WSA_IO_PENDING)
        {
            Trace("Server error: WSARecv() "
                    "returned %d, expected WSA_IO_PENDING\n",
                    GetLastError() );
                
            WaitForClientThreadToFinish(hThreadClient);            
            
            CloseThreadHandle(hThreadClient);

            CloseEventHandle(hReadEvent);
            
            /* Do some cleanup */
            DoWSATestCleanup( testSockets,
                            numSockets );

            Fail("");
        }
        else
        {   
            waitResult = WaitForSingleObject( hReadEvent,
                                            10000 );
            
            if (waitResult!=WAIT_OBJECT_0)
            {                   
                Trace("Server error: Unexpected failure: "
                    "WaitForSingleObject has timed out \n");

                WaitForClientThreadToFinish(hThreadClient);

                CloseThreadHandle(hThreadClient);

                CloseEventHandle(hReadEvent);                
        
                /* Do some cleanup */
                DoWSATestCleanup( testSockets,
                                numSockets );

                Fail("");
            }
        }

         /* test if data can be copied to the current position in the 
           receiving data array. */
        if( pMyData+wsaRecvOverlapped.InternalHigh <&(myData[500][255]) )
        {
            /* copy buffer to data array */
            memcpy(pMyData,wsaBuf.buf,wsaRecvOverlapped.InternalHigh);

            /* increment the position where we can write data on the array*/
            pMyData+=wsaRecvOverlapped.InternalHigh;            
        }
        else
        {
            /* Else the data received exceed buffer capacity */
            Trace("Unexpected, data received exceed buffer capacity.\n");
            WaitForClientThreadToFinish(hThreadClient);

            CloseThreadHandle(hThreadClient);

            CloseEventHandle(hReadEvent);                
    
            /* Do some cleanup */
            DoWSATestCleanup( testSockets,
                            numSockets );

            Fail("");
        }
    }


    if(!WaitForClientThreadToFinish(hThreadClient))
    {        

        CloseThreadHandle(hThreadClient);

        CloseEventHandle(hReadEvent);

        /* Do some cleanup */
        DoWSATestCleanup( testSockets,
                          numSockets );

        Fail("");
    }

    if(!CloseThreadHandle(hThreadClient)||
       !CloseEventHandle(hReadEvent))
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

/* DWORD  PALAPI Thread_Client(LPVOID lpParam)  
   This is a client thread started by the main process.
   It simulate a client connecting to a remote server.   
*/
void  PALAPI Thread_Client(LPVOID lpParam)
{     
    unsigned int     i;
    int     err;
    struct  sockaddr_in mySockaddr;    

    /* Sockets descriptor */
    const int numSockets = 1;    /* number of sockets used in this test */

    SOCKET testSockets[1];    
    
    
    /* Variables for WSASend */

    WSABUF wsaSendBuf;
    DWORD  dwNbrOfByteSent;
    DWORD  dwNbrOfBuf  = 1;
    DWORD  dwSendFlags = 0;
    
    unsigned char sendBuffer[255];    

    WSAOVERLAPPED wsaOverlapped;

    int byteCounter;    

    HANDLE  hWriteEvent;
    DWORD   waitResult;

    threadExitCode=THREAD_UNDEFINED;

    /* Sockets initialization to INVALID_SOCKET */
    for( i = 0; i < (unsigned int)numSockets; i++ )
    {
        testSockets[i] = INVALID_SOCKET;
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
        Trace("Client error: Unexpected failure: "
              "WSASocketA"
              "(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED) "
              " returned %d\n",
              GetLastError());        

        threadExitCode=THREAD_FAIL;

        ExitThread(0);
    }
    
    /* prepare the sockaddr_in structure */
    mySockaddr.sin_family           = AF_INET;
    mySockaddr.sin_port             = getRotorTestPort();
    mySockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    memset( &(mySockaddr.sin_zero), 0, 8);    

    /* Wait for server to call accept */
    Sleep(500);

    /* connect to a server */
    err = connect( testSockets[0], 
                   (struct sockaddr *)&mySockaddr,
                   sizeof(struct sockaddr));

    if( err == SOCKET_ERROR )
    {
        Trace("Client error: Unexpected failure: "
              "connect() socket with local server "
              "returned %d\n",
              GetLastError());

        /* Do some cleanup */
        CloseSocket( testSockets, numSockets );

        threadExitCode=THREAD_FAIL;

        ExitThread(0);
    }


    /* create events */
    hWriteEvent = CreateEvent( NULL, /* no security   */
                             FALSE,    /* reset type    */
                             FALSE,    /* initial state */
                             NULL );   /* object name   */
            
    if( hWriteEvent == NULL )
    {
        Trace("Client error: Unexpected failure: "
              "CreateEvent() "
              "returned %d\n",
              GetLastError());
           
        CloseEventHandle(hWriteEvent);

        /* Do some cleanup */
        CloseSocket( testSockets, numSockets );

        threadExitCode=THREAD_FAIL;

        ExitThread(0);
    }

     /* Initialize the WSAOVERLAPPED to 0 */
    memset(&wsaOverlapped, 0, sizeof(WSAOVERLAPPED));  
    wsaOverlapped.hEvent = hWriteEvent;

     /* fill the sent buffer with value */
    for (i=0;i<255;i++)
    {
        sendBuffer[i]=i;
    }
    
      /* Set the WSABUF structure */
    wsaSendBuf.len = 255;        
    wsaSendBuf.buf = sendBuffer;
    byteCounter = 0;

    /* This loop is intended to send 500 buffer (of 255 bytes).
       The server that receive will shutdown connection
       at the 400th recv. The Client should handle that close
       connection gracefully and close its allocated resources
    */
    for(i=0;i<500;i++)
    {   
        /* Initialize the WSAOVERLAPPED to 0 */
        memset(&wsaOverlapped, 0, sizeof(WSAOVERLAPPED));        
        wsaOverlapped.hEvent = hWriteEvent;

        /* Send some data */
        err = WSASendTo( testSockets[0],
                       &wsaSendBuf,
                       dwNbrOfBuf,
                       &dwNbrOfByteSent,
                       dwSendFlags,
                       (struct sockaddr*)NULL, /* ignored in TCP */
                       (int)NULL, /* ignored in TCP */
                       &wsaOverlapped,
                       0 );

        if(err != SOCKET_ERROR )
        {            
            /* Program continue normally, reset the write event */
            ResetEvent(hWriteEvent);
        }
        else
        {
            /* The server shutdown its socket after 400 wsarecv, it is 
               impossible to have 401 or more successfull send operation */
            if(GetLastError()==WSAECONNABORTED||GetLastError()==WSAESHUTDOWN)
            {
                if (i<400)
                {
                    if(GetLastError()==WSAESHUTDOWN)
                    {
                        Trace("Unexpected WSAESHUTDOWN");
                    }
                    else
                    {
                        Trace("Unexpected WSAECONNABORTED");
                    }

                    CloseEventHandle(hWriteEvent);
                    
                    CloseSocket( testSockets, numSockets );
                    
                    Fail("");
                }
                else
                {
                    /* the program will terminate normally because
                       connection has been closed by the server.
                    */                    
                    break;
                }
            }
            
            /* Handle the overlapped operation */
            if(GetLastError()!=WSA_IO_PENDING)
            {

                Trace("Client error: Unexpected failure: "
                    "WSASend() "
                    "returned %d\n",
                    GetLastError());

               
                CloseEventHandle(hWriteEvent);

                /* Do some cleanup */
                CloseSocket( testSockets, numSockets );

                threadExitCode=THREAD_FAIL;

                ExitThread(0);
            }

            /* Wait 10 seconds for WriteEvent to be signaled 
               by the pending operation
            */
            waitResult = WaitForSingleObject( hWriteEvent,  
                                              10000 );    
 
            if (waitResult!=WAIT_OBJECT_0)
            {   
                Trace("ERROR: Unexpected failure: "
                      "WaitForSingleObject has timed out \n");
                
                CloseEventHandle(hWriteEvent);

                /* Do some cleanup */
                CloseSocket( testSockets, numSockets );

                threadExitCode=THREAD_FAIL;

                ExitThread(0);
            }
        }

        /* keep track of the number of bytes sent */
        byteCounter += wsaOverlapped.InternalHigh; 
        
        /* if wsaOverlapped.InternalHigh is 0, it means 
           that connection has been closed
        */
        if(wsaOverlapped.InternalHigh==0)
        {            
            /* The server shutdown the receiving socket 
               after 400 successfull receive.
            */            
            if (i<400)
            {
                Trace("Unexpected wsaOverlapped.InternalHigh = 0 " 
                     "at WSASend attempt #%d", i);

                
                CloseEventHandle(hWriteEvent);

                /* Do some cleanup */
                CloseSocket( testSockets, numSockets );

                threadExitCode=THREAD_FAIL;

                ExitThread(0);
            }
        }
    } /* end of loop */



    if(!CloseEventHandle(hWriteEvent)||    
       !CloseSocket( testSockets, numSockets ))
    {
        threadExitCode=THREAD_FAIL;

        ExitThread(0);
    }

    threadExitCode=THREAD_SUCCESS;

    ExitThread(0);    
} 



/* int CloseThreadHandle(HANDLE thread)
   This function close a thread handle 
   and return an appropriate message in case of error
*/
static int CloseThreadHandle(HANDLE thread)
{   
    if(thread!=NULL)
    {
        if( CloseHandle(thread) == 0 )
        {
            Trace("Server error: Unexpected failure: "
                  "CloseHandle(thread) "
                  "returned %d\n",
                GetLastError());
            return 0;
        }
    }
    return 1;
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

/* int WaitForThreadToFinish(HANDLE thread)
   This function return 0 if the client thread exit code 
   is clientHasFailed and 1 other is the execution was successfull
*/
static int WaitForClientThreadToFinish(HANDLE thread)
{
    DWORD   waitResult;   
    
    if(thread==NULL)
    {
        return 0;
    }

    /* Wait for the thread to finish execution */
    waitResult = WaitForSingleObject( thread, 10000 );
            
    if (waitResult!=WAIT_OBJECT_0)
    {   
        Trace("Server error: Unexpected failure: "
              "WaitForSingleObject has timed out waiting for" 
              " the client thread to finish.\n");
        return 0;
    }

    if(threadExitCode==THREAD_SUCCESS)
    {
        return 1;
    }    
    else if(threadExitCode==THREAD_FAIL)
    {
        /* return error, error msg has been handled by the client */
        return 0;
    }
    else
    {
        Trace("Client error: Unexpected failure: "
              "No known error code defined on the client thread.");
        return 0;
    }
}

/* int CloseSocket(SOCKET sockList[], int numSockets)
   This function close sockets contained in sockList parameter.

*/
static int CloseSocket(SOCKET sockList[], int numSockets)
{
    int i;
    int err;

    int bResult=1; /* failure indication */

    if( (sockList != NULL) &&
        (numSockets != 0 ) )
    {
        /* Close the open sockets */
        for( i = 0 ; i < numSockets; i++ )
        {
            if( sockList[i] != INVALID_SOCKET )
            {
                err = closesocket(sockList[i]);
                if( err == SOCKET_ERROR )
                {    
                    Trace("ERROR: Unexpected failure: "
                          "closesocket(%i) returned (%d)\n", 
                          sockList[i],
                          GetLastError());

                    bResult = 0;
                }
            }

        }
    }
    else
    {
        bResult = 0;
    }
    return bResult;
}
