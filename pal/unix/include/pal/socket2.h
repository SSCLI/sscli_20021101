/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

  include/pal/socket2.h

Abstract:

  Header file for winsock2 functionality (asynchronous notification,
overlapped operations, ...)

Notes:

  Because the winsock2 features require a separate worker thread, there
are some serious synchronization issues. This header file contains the
structures used to communicate between the worker thread and the application
thread.

When an applicaton thread wants to initiate or modify an asynchronous 
operation, it should fill one of the structures below and write it to
the pipe. Thus the worker thread will fulfil requests on a FIFO basis.
Most of the PAL functions consider a succesful write to the pipe to 
be the completion of the given operation. With a few exceptions, this 
design is not inherently succeptable to race conditions, except where 
a race exists in the application itself.

However, there are some exceptions. closesocket, for example, is supposed
to cancel any overlapped operations on that socket, and signal any events
associated with same. No events should be signalled after closesocket
returns. So closesocket needs to receive confirmation from the worker
thread that all events associated with the object have been signalled.
As part of the data that is written to the pipe, closesocket can pass
an event on which it wants to receive completion notification, and wait
on that event before returning.

--*/

#ifndef _PAL_SOCKET_H_
#define _PAL_SOCKET_H_

#include "config.h"
#include <pthread.h>

extern CRITICAL_SECTION SOCK_crit_section;
extern int SOCK_startup_count;
extern pthread_key_t SOCK_tlsHostentKey;

#if !HAVE_SOCKLEN_T
/* Systems that don't have socklen_t seem to use int instead. */
typedef int socklen_t;
#endif  /* HAVE_SOCKLEN_T */

/* opcodes to be used with the ws2_op structure below */
enum ws2_opcode
{
    /* use 'arg.p_xxx' with a pointer to a structure of the
       appropriate type */
    WS2_OP_SENDTO,
    WS2_OP_RECVFROM,
    WS2_OP_EVENTSELECT,

    /* use 'arg.event' with a HANDLE to the event to signal once
       all overlapped operations have been cancelled */
    WS2_OP_CLOSESOCKET,

    /* use 'arg.network_events' to represent the network events 
       (FD_READ, etc.) that should be set for this socket */
    WS2_OP_RESET_EVENTS,

    WS2_OP_STOPTHREAD  /* to tell worker thread to terminate */
};

typedef struct _ws2_op_sendto
{
    LPWSABUF lpBuffers;
    DWORD    dwFlags;
    const struct PAL_sockaddr FAR *lpTo;
    int      iToLen;
    LPWSAOVERLAPPED lpOverlapped;
} ws2_op_sendto;

typedef struct _ws2_op_recvfrom
{
    LPWSABUF lpBuffers;
    DWORD    dwFlags;
    struct PAL_sockaddr FAR *lpFrom;
    LPINT    lpFromLen;
    LPWSAOVERLAPPED lpOverlapped;
} ws2_op_recvfrom;

typedef struct _ws2_op_eventselect
{
    HANDLE   hEventObject;
    long     lNetworkEvents;  /* original events */
    long     disabled_events; /* events currently disabled */
} ws2_op_eventselect;

typedef struct _ws2_op
{
    struct _ws2_op  *self_addr;
    PAL_SOCKET      s;
    enum ws2_opcode opcode; /* one of WS2_OP_XXX. See above. */
    HANDLE          event;  /* only use with WS2_OP_CLOSESOCKET */
} ws2_op;

typedef struct _ws2_op_list_node
{
    struct _ws2_op_list_node *next;
    union
    {
        ws2_op_sendto   *p_sendto;
        ws2_op_recvfrom *p_recvfrom;
    } op;
} ws2_op_list_node;

typedef struct _ws2_op_list
{
    ws2_op_list_node *head;
    ws2_op_list_node *tail;
} ws2_op_list;

typedef struct _ws2_sock
{
    ws2_op_eventselect *eventselect;
    ws2_op_list        send_list;
    ws2_op_list        recv_list;
} ws2_sock;

typedef struct _async_list
{
    int           max_fd;  /* highest fd currently in the list */
    struct pollfd *pollfds; 
    ws2_sock      *sockets;
} async_list;

/*++
Function:
    FD_SET_BOOL
Abstract:
   Calls FD_SET macro.   
Parameters:
    PAL_SOCKET sockId: socket ID
    fd_set* fd: fd_set structure.    
Returns:
   returns FALSE when socket value is invalid, TRUE otherwise.
--*/
BOOL FD_SET_BOOL(PAL_SOCKET sockId, fd_set* fd);

/*++
Function:
    FD_CLR_BOOL
Abstract:
   Calls FD_CLR macro.
Parameters:
    PAL_SOCKET sockId: socket ID
    fd_set* fd: fd_set structure.        
Returns:
    returns FALSE when socket value is invalid, TRUE otherwise.
--*/
BOOL FD_CLR_BOOL(PAL_SOCKET sockId, fd_set* fd);

/* calls FD_ISSET only if socket value < FD_SETSIZE 
  returns TRUE when socket value is valid and the FD is set, FALSE otherwise */
#define FD_ISSET_BOOL(n, p) \
    (((n) <= FD_SETSIZE) && FD_ISSET((n), (p)))

/*++
Function:
  SOCKInitWinSock
--*/
BOOL SOCKInitWinSock(void);

/*++
Function:
  SOCKTerminateWinSock
--*/
void SOCKTerminateWinSock(void);

/*++
Function:
  SOCKCancelAsyncOperations
--*/
int SOCKCancelAsyncOperations( PAL_SOCKET s );

/*++
Function:
  SOCKStopWorkerThread

Send a message down the pipe to tell the worker thread to terminate.

(no parameters, no return value)
--*/
void SOCKStopWorkerThread( void );

/*++
Function:
  SOCKWorkerThreadMain

Entry point for the worker thread which will poll() asynchronous sockets
and complete any overlapped operations.

Parameters:
  PVOID phWorkerThreadInitCompleted: a pointer to a event handle
--*/
DWORD SOCKWorkerThreadMain( PVOID phWorkerThreadInitCompleted );

/*++
Function:
  SOCKAllocTlsHostent
--*/
BOOL SOCKAllocTlsHostent( void );

/*++
Function:
  SOCKFreeTlsHostent
--*/
void SOCKFreeTlsHostent( void );


/*++
Function:
  SOCKFreePALHostentFields

Free the fields of a PAL_hostent.  This does not free the hostent itself.
--*/
void SOCKFreePALHostentFields(struct PAL_hostent *hostent);


/*++
Function:
  SOCKIsSocketConnected
--*/
int SOCKIsSocketConnected( PAL_SOCKET s );


/*++
Function:
  SOCKErrorReturn
--*/  
DWORD SOCKErrorReturn( int errnumber );

/*++
Function:
    SOCKGetNativeSocketLevel
--*/
int SOCKGetNativeSocketLevel(int palLevel);

/*++
Function:
    SOCKGetNativeSocketType
--*/  
int SOCKGetNativeSocketType(int palType);

/*++
Function:
    SOCKGetPALSocketType
--*/  
int SOCKGetPALSocketType(int nativeType);

/*++
Function:
    SOCKGetNativeSocketOption
--*/  
int SOCKGetNativeSocketOption(int level, int palOption);

/*++
Function:
  SOCKIsQueueEmpty

Test whether specified overlapped operation queue is empty or not

Return value:
  FALSE if not empty or invalid socket or invalid operation type.
--*/
BOOL SOCKIsQueueEmpty(int fd, enum ws2_opcode which);

BOOL
SOCKQueueRemoveOperation(
    int fd,
    enum ws2_opcode whichQueue,
    LPVOID pOperationToRemove);
    
BOOL
SOCKQueuePushBackOperation(
    int fd,
    enum ws2_opcode whichQueue, 
    LPVOID *operation);

/**
Function:
    SOCKIsAsyncSocket

Test whether there is any async operation associated with specified socket.

Return value:
    FALSE if no asynchronous operation associated with socket
*/
BOOL
SOCKIsAsyncSocket(
    int fd);

BOOL
SOCKAssignEventObjectToNetworkEvents(
    int fd,
    HANDLE hEventObject,
    LONG lNetworkEvents);

VOID
SOCKResetNetworkEvents(
    int fd,
    LONG lNetworkEvents);


/*++
Function:
  SOCKSendRequestForAsyncOp

Send a request to the worker thread.


Return value:
    if error: the "LastError" is returned
    otherwise: 0 is returned.

--*/
DWORD SOCKSendRequestForAsyncOp( PAL_SOCKET s, enum ws2_opcode opcode, 
                                 HANDLE hEvent);

/*++
Function:
  SOCKFlushOverlappedOperationQueues

Clear all overlapped operation on socket's queues.

Return value:
    TRUE if succeeded
    FALSE otherwise

--*/
BOOL 
SOCKFlushOverlappedOperationQueues(int fd, BOOL bSignalEvents);

#endif
