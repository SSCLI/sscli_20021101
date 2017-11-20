/*=============================================================
**
** Source: setsockopt_neg.c
**
** Purpose: Negatively test the setsockopt function in SOL_SOCKET.
**          Call getsockopt by passing an invalid socket descriptor.
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
    int err;
    int INVALID_SOCKET_ID = -1;
    int size;
    int nValue = 10;

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
            Trace("\nFailed to call WSACleanup API, error code=%d!\n",
                GetLastError());
        }
        Fail("");
    }

    size =sizeof(int);
    
    /*pass an invalid socket descriptor*/
    err = setsockopt(INVALID_SOCKET_ID, SOL_SOCKET, SO_SNDTIMEO,
                (char *)&nValue, size);
    if(WSAENOTSOCK != GetLastError() || SOCKET_ERROR != err)
    {  
      /*no error occured*/
        Trace("\nFailed to call Setsockopt API for negative test, "
                "call setsockopt by passing an invalid socket descriptor, "
                "an error is expected, but no error or no correct error "
                "code returns!\n");

        err = WSACleanup();
        if(SOCKET_ERROR == err)
        {
            Trace("\nFailed to call WSACleanup API, error code=%d!\n",
                GetLastError());
        }
        Fail("");
    }

    err = WSACleanup();
    if(SOCKET_ERROR == err)
    {
        Fail("\nFailed to call WSACleanup API, error code=%d!\n",
                GetLastError());
    }

    PAL_Terminate();
    return PASS;
}
