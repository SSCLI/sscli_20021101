/*=============================================================
**
** Source: select_neg.c
**
** Purpose: Negatively test the select API to monitor readable socket 
**          when waiting time is zero. If no readable socket is ready, 
**          the select call will return 0 immediately.
**          Call select before calling WSAStartup or after calling
**          WSACleanup.
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
**============================================================*/
#include <palsuite.h>

int __cdecl main(int argc, char *argv[])
{
    WORD VersionRequested = MAKEWORD(2,2);
    WSADATA WsaData;
    int err;
    int socketID;
    struct sockaddr_in mySockaddr;
    int nBacklogNumber = 1;
    int socketFds;
    struct timeval tv;
    fd_set readfds;
    
    /*Initialize the PAL environment*/
    err = PAL_Initialize(argc, argv);
    if(0 != err)
    {
        return FAIL;
    }

    /*set the waiting time as zero*/
    tv.tv_sec = 0L;
    tv.tv_usec = 0L;

    /*initialize the readable fd_set*/
    FD_ZERO(&readfds);

    /*monitor the readable socket set before calling WSAStartup*/
    socketFds = select(0, &readfds, NULL, NULL, &tv);

    if(WSANOTINITIALISED != GetLastError() || SOCKET_ERROR != socketFds) 
    {
        Fail("\nFailed to call select API for a negative test, "
            "Call select before calling WSAStartup, an error is "
            "expected, but no error or no expected error is detected, "
            "error code= %u\n", GetLastError());
    }


    /*initialize to use winsock2.dll*/
    err = WSAStartup(VersionRequested, &WsaData);
    if(err != 0)
    {
        Fail("\nFailed to find a usable WinSock DLL!\n");
    }

    /* Confirm that the WinSock DLL supports 2.2.*/
    if(LOBYTE(WsaData.wVersion) != 2 ||
            HIBYTE(WsaData.wVersion) != 2)
    {
        Trace("\nFailed to find a usable WinSock DLL!\n");
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API!\n");
        }
        Fail("");
    }

    /*create a stream socket in AF_INET domain*/
    socketID = socket(AF_INET, SOCK_STREAM, 0);
    if(INVALID_SOCKET == socketID)
    {
        Trace("\nFailed to call socket API to create a stream socket!\n");
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API!\n");
        }
        Fail("");
    }

    /*prepare the sockaddr_in structure*/
    mySockaddr.sin_family = AF_INET;
    mySockaddr.sin_port = 0;
    mySockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    memset( &(mySockaddr.sin_zero), 0, 8);

    /*bind the local address to the created socket*/
    err = bind(socketID,(struct sockaddr *)&mySockaddr,
            sizeof(struct sockaddr));

    if(SOCKET_ERROR == err)
    {
        Trace("\nFailed to call bind API to bind the socket with "
                "a local address!\n");
        err = closesocket(socketID);
        if(SOCKET_ERROR == err)
        {    
            Trace("\nFailed to call closesocket API!\n");
        }
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API!\n");
        }
        Fail("");
    }

    /*to setup the backlog number for a created socket*/
    err = listen(socketID,nBacklogNumber);

    if(SOCKET_ERROR == err)
    {
        Trace("\nFailed to call listen API for positive test!\n");
        err = closesocket(socketID);
        if(SOCKET_ERROR == err)
        {    
            Trace("\nFailed to call closesocket API!\n");
        }
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API!\n");
        }
        Fail("");
    }

    /*add socket to the readable socket set*/
    FD_SET(socketID, &readfds);

    /*terminate use of WinSock DLL*/
    /*and let select generate error*/
    err = WSACleanup();
    if(SOCKET_ERROR == err)
    {
        Fail("\nFailed to call WSACleanup API!\n");
    }

    /*monitor the readable socket set after calling WSACleanup*/
    socketFds = select(0, &readfds, NULL, NULL, &tv);

    if(WSANOTINITIALISED != GetLastError() || SOCKET_ERROR != socketFds) 
    {
        Fail("\nFailed to call select API for a negative test, "
            "Call select after calling WSACleanup, an error is "
            "expected, but no error or no expected error is detected, "
            "error code= %u\n", GetLastError());
    }

    PAL_Terminate();
    return PASS;
}
