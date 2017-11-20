/*=============================================================
**
** Source: getpeername_neg.c
**
** Purpose: Negative test getpeername function to retrieve the 
**          peer name by passing a samller name size than it should 
**          be.
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
    WORD VersionRequested = MAKEWORD(2, 2);
    WSADATA WsaData;

    int SocketID;
    int err;
    struct sockaddr_in server;
    struct hostent *ServerHostent;
    struct sockaddr MySockaddr;
    const char *pHostName = NULL;
    int ServerState;
    int size;

    /*Initialize the PAL environment*/
    err = PAL_Initialize(argc, argv);
    if(0 != err)
    {
        return FAIL;
    }

    /*initialize to use winsock2.dll */
    err = WSAStartup(VersionRequested, &WsaData);
    if(0 != err)
    {
        Fail("\nFailed to find a usable WinSock DLL, "
                "error code=%u\n", GetLastError());
    }

    /*Confirm that the WinSock DLL supports 2.2.*/
    if(LOBYTE( WsaData.wVersion ) != 2 ||
            HIBYTE( WsaData.wVersion ) != 2)
    {
        Trace("\nFailed to find a usable WinSock DLL, "
                "error code=%u\n", GetLastError());
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API, "
                "error code=%u\n", GetLastError());
        }
        Fail("");
    }

    /*create a stream socket in AF_INET domain*/
    SocketID = socket(AF_INET, SOCK_STREAM, 0);
    if(INVALID_SOCKET == SocketID)
    {
        Trace("\nFailed to call socket API to create a stream socket, "
                "error code=%u\n", GetLastError());
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API, "
                "error code=%u\n", GetLastError());
        }
        Fail("");
    }

    /*retrive the host name from external file*/
    pHostName = getRotorTestHost();
    if(NULL == pHostName )
    {
        Trace("\nFailed to read server name!\n");
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API\n");
        }
        Fail("");
    }
    if(strlen(pHostName) == 0)
    {
        Trace("\nFailed to read server name!\n");
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API, "
                "error code=%u\n", GetLastError());
        }
        Fail("");
    }

    /*
     *connect this socket to a server
     *get host info
    */
    ServerHostent = gethostbyname(pHostName);
    if(NULL == ServerHostent)
    {
        Trace("\nFailed to call gethostbynam API, "
                "error code=%u\n", GetLastError());
        err = closesocket(SocketID);
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call closesocket API, "
                "error code=%u\n", GetLastError());
        }
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API, "
                "error code=%u\n", GetLastError());
        }
        Fail("");
    }
    memset( &server, 0, sizeof(server) );
    memcpy( &server.sin_addr, ServerHostent->h_addr, ServerHostent->h_length);
    server.sin_family = ServerHostent->h_addrtype;

    /* Set the port number we want to connect to */
    server.sin_port = th_htons(80);

    ServerState = connect( SocketID, (const struct sockaddr FAR *)&server,
            sizeof(server) );
    if (SOCKET_ERROR == ServerState)
    {
        Trace("\nFailed to call connect API, "
                "error code=%u\n", GetLastError());
        err = closesocket(SocketID);
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call closesocket API, "
                "error code=%u\n", GetLastError());
        }
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API, "
                "error code=%u\n", GetLastError());
        }
        Fail("");
    }

    
    /*make the parameter size smaller than what it should be*/
    size = sizeof(struct sockaddr)-1;

    /*retrieve the peer host info*/
    err = getpeername(SocketID, &MySockaddr, &size);

    if(WSAEFAULT != GetLastError() || SOCKET_ERROR != err)
    {
        Trace("\nFailed to call getpeername API for a negative test!\n");
        err = closesocket(SocketID);
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call closesocket API, "
                "error code=%u\n", GetLastError());
        }

        err= WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API, "
                "error code=%u\n", GetLastError());
        }
        Fail("");
    }

    err = closesocket(SocketID);
    if(SOCKET_ERROR == err)
    {
        Trace("\nFailed to call closesocket API, "
                "error code=%u\n", GetLastError());
        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API, "
                "error code=%u\n", GetLastError());
        }
        Fail("");
    }

    err = WSACleanup();
    if(SOCKET_ERROR == err)
    {
        Fail("\nFailed to call WSACleanup API, "
                "error code=%u\n", GetLastError());
    }

    PAL_Terminate();
    return PASS;
}
