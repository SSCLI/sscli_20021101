/*=============================================================
**
** Source: listen_neg.c
**
** Purpose: Negatively test the listen API to setup the backlog 
**          number for a stream socket.
**          Call listen API before calling WSAStartup or after
**          calling WSACleanup.
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
    int socketID = INVALID_SOCKET;
    struct sockaddr_in mySockaddr;

    /*set the backlog number*/
    const int BacklogNumber = 10;

    /*Initialize the PAL environment*/
    err = PAL_Initialize(argc, argv);
    if(0 != err)
    {
        return FAIL;
    }

    /*
    to setup the backlog number 
    before calling the WSAStartup()
    */
    err = listen(socketID,BacklogNumber);
    if(WSANOTINITIALISED != GetLastError() || SOCKET_ERROR != err)
    {
        Fail("\nFailed to call listen API for a negative test,"
            "setup backlog number before calling WSAStartup, "
            "an error is expected, but no error or no expected error "
            "is detected, error code = %u\n", GetLastError());
    }


    /*initialize to use winsock2.dll*/
    err = WSAStartup(VersionRequested,&WsaData);

    if(err != 0)
    {
        Fail("\nFailed to find a usable WinSock DLL!\n");
    }

    /*Confirm that the WinSock DLL supports 2.2.*/  
    if(LOBYTE( WsaData.wVersion ) != 2 ||
            HIBYTE( WsaData.wVersion ) != 2)
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
    socketID = socket(AF_INET,SOCK_STREAM,0);
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

    /*prepare the sockaddr structure*/
    mySockaddr.sin_family = AF_INET;
    mySockaddr.sin_port = 0;
    mySockaddr.sin_addr.S_un.S_addr = INADDR_ANY;
    memset( &(mySockaddr.sin_zero), 0, 8);

    /*bind local address to a socket*/
    err = bind(socketID,(struct sockaddr *)&mySockaddr, 
            sizeof(struct sockaddr));

    if(SOCKET_ERROR == err)
    {
        Trace("\nFailed to call bind API to bind a socket with "
                "local address!\n");
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

    /*
      terminate use of WinSock DLL
      let listen generate error
    */
    err = WSACleanup();
    if(SOCKET_ERROR == err)
    {
        Fail("\nFailed to call WSACleanup API!\n");
    }

    /*
    to setup the backlog number for a created socket
    after calling the WSACleanup()
    */
    err = listen(socketID,BacklogNumber);
    if(WSANOTINITIALISED != GetLastError() || SOCKET_ERROR != err)
    {
        Fail("\nFailed to call listen API for a negative test,"
            "setup backlog number after calling WSACleanup, "
            "an error is expected, but no error or no expect error "
            "is detected, error code = %u\n", GetLastError());
    }

    PAL_Terminate();
    return PASS;
}

