/*============================================================================
**
** Source:  wsaenumnetworkevents_client.c
**
** Purpose: Checks that _alloca allocates memory, and that the memory is
**          readable and writeable.Checks that _alloca allocates memory, 
**          and that the memory is readable and writeable.
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
**==========================================================================*/

#include <palsuite.h>

int __cdecl main(int argc, char *argv[])
{
    WORD VersionRequested = MAKEWORD(2,2);
    WSADATA WsaData;
    int err;
    int socketID;
    struct sockaddr_in mySockaddr;

    HANDLE myEvent;
    WSANETWORKEVENTS myEventStruct;
    int PORTNUMBER = getRotorTestPort();


    /*Initialize the PAL environment*/
    err = PAL_Initialize(argc, argv);
    if(0 != err)
    {
        return FAIL;
    }

    /*initialize to use winsock2.dll*/
    err = WSAStartup(VersionRequested, &WsaData);
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

    /*create an event*/
    myEvent = CreateEvent( NULL,   /* no security */
                           FALSE,  /* system automatically reset state */
                           FALSE,  /* nonsignaled */
                           NULL ); /* event object name */

    if(ERROR_INVALID_HANDLE == err)
    {
        Trace("\nFailed to call CreateEvent API to create an event "
            "error code = %u\n", GetLastError());

        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API ,"
                "error code = %u\n", GetLastError());
        }
        Fail("");
    }

    /*create a stream socket in AF_INET domain*/
    socketID = socket(AF_INET, SOCK_STREAM, 0);

    if(INVALID_SOCKET == socketID)
    {
        Trace("\nFailed to call socket API to create a steam socket; "
                "error code = %u\n", GetLastError());

        err = CloseHandle(myEvent);
        if(0 == err)
        {
            Trace("\nFailed to call CloseHandle API!\n");
        }

        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API!\n");
        }
        Fail("");
    }

    /*associate this event with FD_CONNECT event*/
    err = WSAEventSelect(socketID, myEvent, FD_CONNECT);
    if(SOCKET_ERROR == err)
    {
        Trace("\nFailed to call WSAEvenvSelect API; "
            "error code = %u\n", GetLastError());

        err = closesocket(socketID);
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call closesocket API!\n");
        }

        err = CloseHandle(myEvent);
        if(0 == err)
        {
            Trace("\nFailed to call CloseHandle API!\n");
        }

        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API!\n");
        }
        Fail("");
    }


    /*prepare the sockaddr_in structure*/
    mySockaddr.sin_family = AF_INET;
    mySockaddr.sin_port = PORTNUMBER;
    mySockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    memset( &(mySockaddr.sin_zero), 0, 8);

    /*connect to a stream server*/
    err = connect(socketID, (struct sockaddr *)&mySockaddr, 
                sizeof(struct sockaddr));

    if(SOCKET_ERROR == err && WSAEWOULDBLOCK != GetLastError())
    {
        Trace("\nFailed to call connect API to connect a stream server!\n");
        err = closesocket(socketID);
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call closesocket API!\n");
        }

        err = CloseHandle(myEvent);
        if(0 == err)
        {
            Trace("\nFailed to call CloseHandle API!\n");
        }
        
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API!\n");
        }
        Fail("");
    }
    
    Sleep(5000);
    /*
    *to discover the connect events and check the  
    *event ID and related event error code
    */
    err = WSAEnumNetworkEvents(socketID, (WSAEVENT)myEvent, &myEventStruct);
    if(SOCKET_ERROR == err ||
        FD_CONNECT != myEventStruct.lNetworkEvents ||
        0 != myEventStruct.iErrorCode[FD_CONNECT_BIT])
    {
        Trace("\nFailed to call WSAEnumNetworkEvenvs API, "
            "error code = %u\n", GetLastError());

        err = closesocket(socketID);
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call closesocket API!\n");
        }

        err = CloseHandle(myEvent);
        if(0 == err)
        {
            Trace("\nFailed to call CloseHandle API!\n");
        }

        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API!\n");
        }
        Fail("");
    }

    err = closesocket(socketID);
    if(SOCKET_ERROR == err)
    {
        Trace("\nFailed to call closesocket API!\n");

        err = CloseHandle(myEvent);
        if(0 == err)
        {
            Trace("\nFailed to call CloseHandle API!\n");
        }

        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API!\n");
        }
        Fail("");
    }

    err = CloseHandle(myEvent);
    if(0 == err)
    {
        Trace("\nFailed to call CloseHandle API!\n");
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API!\n");
        }
        Fail("");
    }

    err = WSACleanup();
    if(SOCKET_ERROR == err)
    {
        Fail("\nFailed to call WSACleanup API!\n");
    }

    PAL_Terminate();
    return PASS;
}

