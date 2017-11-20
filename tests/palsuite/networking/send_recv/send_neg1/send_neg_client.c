/*=============================================================
**
** Source: send_neg_client.c
**
** Purpose: Negatively test the send API.
**          This is a client routine. This client routine must be
**          started after the related server is started.
**          This test case is to test sending before calling 
**          WSAStartup or after calling WSACleanup.
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
    int socketID = -1;
    struct sockaddr_in mySockaddr;
    /*defin a non-zero length data*/
    const char *data = "None-zero length data test";
    int nBuffer;


    /*Initialize the PAL environment*/
    err = PAL_Initialize(argc, argv);
    if(0 != err)
    {
        return FAIL;
    }

    /*send non-zero length data to server before calling WSAStartup*/
    err = send(socketID, data, strlen(data), 0);
    if(WSANOTINITIALISED != GetLastError() || SOCKET_ERROR != err)
    {
        Fail("\nFailed to call send API for a negative test "
            "call send before calling WSAStartup, an error is expected, "
            "but no error or no expected error is detected, "
            "error code = %u\n", GetLastError());
    }

    /*initialize to use winsock2.dll*/
    err = WSAStartup(VersionRequested,&WsaData);
    if ( err != 0 )
    {
        Fail("\nFailed to find a usable WinSock DLL!\n");
    }

    /*Confirm that the WinSock DLL supports 2.2.*/
     if ( LOBYTE( WsaData.wVersion ) != 2 ||
            HIBYTE( WsaData.wVersion ) != 2 )
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
        Trace("\nFailed to call socket API to creat a steam socket!\n");
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API!\n");
        }
        Fail("");
    }

    /*prepare the sockaddr_in structure*/
    mySockaddr.sin_family = AF_INET;
    mySockaddr.sin_port = getRotorTestPort();
    mySockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    memset( &(mySockaddr.sin_zero), 0, 8);

    /*connect to a stream server*/
    err = connect(socketID, (struct sockaddr *)&mySockaddr,
            sizeof(struct sockaddr));
    if(SOCKET_ERROR == err)
    {
        Trace("\nFailed to call connect API to connect a stream server!\n");
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

    /*terminate use of WinSock DLL*/
    err = WSACleanup();
    if(SOCKET_ERROR == err)
    {
        Fail("\nFailed to call WSACleanup API!\n");
    }

    nBuffer = strlen(data);

    /*send non-zero length data to server after calling WSACleanup*/
    err = send(socketID,data,nBuffer,0);
    if(WSANOTINITIALISED != GetLastError() || SOCKET_ERROR != err)
    {
        Fail("\nFailed to call send API for a gegative test "
            "call send after calling WSACleanup, an error is expected, "
            "but no error or no expected error is detected, "
            "error code = %u\n", GetLastError());
    }

    PAL_Terminate();
    return PASS;
}
