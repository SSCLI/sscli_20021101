/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    async.c

Abstract:

    Worker thread responsible for asynchronous socket I/O operations.
    
    For each socket, we associate a socket structure in which is stored
    asynchronous operations for that socket. We also associate a pollfd
    structure for each socket. The thread will only poll on sockets that
    are considered as asynchronous. A socket is considered asynchronous when
    its associated pollfd structure file descriptor isn't -1.

    Asynchronous operations currently supported are:
    
    - event object notification of network events;
    - overlapped send operations;
    - overlapped write operations.

    Overlapped operations are queued by WSASend/WSARecv. The worker thread
    is notified of pending overlapped operations by reading WS2_OP_SENDTO
    and WS2_OP_RECVFROM opcode in the interthread pipe.

    If the poll (found in the thread loop) returns POLLIN/POLLOUT events
    for that asynchronous socket, we start processing the queued operations.
    Operations are removed from the queue by the worker thread. If poll returns
    exceptional events (POLLHUP/POLLERR/POLLNVAL) then all pending asynchronous 
    operations are cancelled and the socket is restored to its synchronous mode.
    
     
Remarks:
    Since the queues are updated in two different threads, we need a critical
    section - different than the socket critical section - to prevent race 
    condition between the readers/writers of the queue. 
    
    The writers to the queues are :
    
    - WSASend(To), WSARecv(From), by calling QUEUEPushBackAsyncOperation, to
      add overlapped operations;
      
    - SOCKProcessOverlappedSend/Recv, by calling QUEUEDequeAsyncOperations, 
      to cleanup processed overlapped operations.

    - SOCKDeleteAsyncSocket, by calling QUEUEDequeueAsyncOperations, to 
      cancel all pending operations.
     
    SOCKDeleteAsyncSocket is called by SOCKProcessAsyncOperations in response
    of a closesocket operation or a terminate thread operation. It is also
    called by SOCKWorkerThreadMain if an exceptional event is returned from 
    the poll call on that socket. Finally, it can be called if refreshing the
    monitored network events for a socket finds that there isn't any pending 
    overlapped operations nor network events associated with an event object
    for that socket.
--*/

#include "pal/palinternal.h"
#include "pal/critsect.h"
#include "pal/dbgmsg.h"
#include "pal/socket2.h"

#include <sys/types.h>
#if HAVE_POLL
#include <poll.h>
#else
#include "pal/fakepoll.h"
#include <sys/stat.h>
#endif  // HAVE_POLL
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
#if HAVE_STROPTS_H
#include <stropts.h>
#endif  // HAVE_STROPTS_H

SET_DEFAULT_DEBUG_CHANNEL(SOCKET);

extern CRITICAL_SECTION ASYNCSOCKETS_crit_section; /* in socket2.c */
extern int SOCK_pipe_fd[2]; /* in socket2.c */

#define SOCK_PIPE_READ SOCK_pipe_fd[0]

static void SOCKProcessAsyncOperations();
static void SOCKRefreshPollfdEvents( int fd );
static void SOCKProcessOverlappedSend( int fd );
static void SOCKProcessOverlappedRecv( int fd );
static short SOCKNetworkEventsToPollEvents( long network_events );
static BOOL SOCKAreAsyncOperationsEnabled( int fd );

/* the list of asynchronous sockets */
static async_list socket_list;

static ws2_sock      sockets[FD_SETSIZE];
static struct pollfd pollfds[FD_SETSIZE];

/* functions to access the list  */
static void SOCKAddAsyncSocket( int fd );
static BOOL SOCKDeleteAsyncSocket( int fd, BOOL threadStop);
static struct pollfd *SOCKGetPollfdFromAsyncSocket( int fd );
static ws2_sock *SOCKGetDataFromAsyncSocket( int fd );
static ws2_op_list *SOCKGetQueueFromSocket(int fd, enum ws2_opcode which);
static void SOCKRemoveSocketFromPollList( int fd );

static
ws2_op_list*
SOCKGetQueueFromSocket(
    int fd,
    enum ws2_opcode which)
{
    ws2_sock    *sock = NULL;
    ws2_op_list *lpQ  = NULL;

    sock = SOCKGetDataFromAsyncSocket(fd);

    if (sock)
    {
        if (which == WS2_OP_SENDTO)
        {
            lpQ = & (sock->send_list);
        }
        else if (which == WS2_OP_RECVFROM)
        {
            lpQ = & (sock->recv_list);
        }
    }
    return lpQ;
}

/*++
Function:
  SOCKIsQueueEmpty

Test whether specified overlapped operation queue is empty or not

Return value:
  FALSE if not empty or invalid socket or invalid operation type.
--*/
BOOL 
SOCKIsQueueEmpty(
    int fd, 
    enum ws2_opcode which)
{
    BOOL bQueueEmpty  = FALSE;
    ws2_op_list *lpQ  = NULL;
 
    TRACE("Entering SOCKIsQueueEmpty\n");

    lpQ = SOCKGetQueueFromSocket(fd, which);
   
    if (lpQ)
    {
        bQueueEmpty = (BOOL) (lpQ->head == NULL);
    }

    TRACE("Leaving SOCKIsQueueEmpty %d\n", bQueueEmpty);
    return bQueueEmpty;
}

/*++
Function:
  SOCKQueueRemoveOperation
 
 This function assumes the caller will hold the ASYNCSOCKETS_crit_section before
 calling this function 
--*/
BOOL
SOCKQueueRemoveOperation(
    int fd,
    enum ws2_opcode whichQueue,
    LPVOID pOperationToRemove)
{
    ws2_op_list      *lpQ           = NULL;
    ws2_op_list_node *pPreviousNode = NULL;
    ws2_op_list_node *pCurrentNode  = NULL;

    PVOID             ptrToCompare  = NULL;
    
    TRACE("Removing queued overlapped operation on socket %d\n", fd);

    if (!pOperationToRemove)
    {
        ERROR("No operation to remove!\n");
        return FALSE;
    }

    lpQ = SOCKGetQueueFromSocket(fd, whichQueue);

    if (!lpQ) 
    {
        ERROR("No queue for specified socket or operation\n");
        return FALSE;
    }
 
    pCurrentNode = lpQ->head;
  
    if (!pCurrentNode)
    {
        ERROR("Empty queue!\n");
        return FALSE;
    }

    do
    {
        if (whichQueue == WS2_OP_SENDTO)
        {
            ptrToCompare = (PVOID) pCurrentNode->op.p_sendto;
        }
        else if (whichQueue == WS2_OP_RECVFROM)
        {
            ptrToCompare = (PVOID) pCurrentNode->op.p_recvfrom;
        }
        else
        {
            ASSERT("Unsupported operation type\n");
            return FALSE;
        }

        if (ptrToCompare == pOperationToRemove)
        {
            break;
        }

        pPreviousNode = pCurrentNode;
        pCurrentNode  = pCurrentNode->next;

    } while (pCurrentNode);

   
    /* end of queue reached */
    if ( !pCurrentNode )
    {
        ERROR("Couldn't find operation in queue\n");
        return FALSE;
    }
    else 
    {
        if (pPreviousNode)
        {
            pPreviousNode->next = pCurrentNode->next;
    
            if (!pPreviousNode->next)
            {
                lpQ->tail = pPreviousNode;
            }
        }
        else
        {
            lpQ->head = pCurrentNode->next;

            /* if there's now 0 or 1 node in the list, set the tail */
            if ((lpQ->head == NULL) || (lpQ->head->next == NULL))
            {
                lpQ->tail = lpQ->head;
            }
        }
    }

    if (whichQueue == WS2_OP_SENDTO)
    {
        ws2_op_sendto *ptr = (ws2_op_sendto*) pOperationToRemove;

        free( ptr->lpBuffers );
        free( ptr );
    }
    else if (whichQueue == WS2_OP_RECVFROM)
    {
        ws2_op_recvfrom *ptr = (ws2_op_recvfrom*) pOperationToRemove;
        
        free( ptr->lpBuffers );
        free( ptr );
    }
    free(pCurrentNode);

    return TRUE;
}   

/*++
Function:
 SOCKAssignEventObjectToNetworkEvents  
    
 This function assumes the caller will hold the ASYNCSOCKETS_crit_section before
 calling this function 
--*/
BOOL
SOCKAssignEventObjectToNetworkEvents(
    int fd,
    HANDLE hEventObject,
    LONG lNetworkEvents)
{ 
    ws2_sock    *sock        = NULL;
    DWORD        dwLastError = 0;

    TRACE("Assigning an event object %p to network events %d"
          "for socket %d.\n", hEventObject, lNetworkEvents, fd);

    sock = SOCKGetDataFromAsyncSocket(fd);

    if (!sock) 
    {
        dwLastError = WSAENOTSOCK;
        ERROR("Socket %d is not a valid socket\n", fd);
        goto error;
    }
    
    if (!sock->eventselect)
    {
        /* this memory is freed in SOCKDeleteAsyncSocket */
        sock->eventselect =
            (ws2_op_eventselect*)malloc(sizeof(ws2_op_eventselect));

        if ( !sock->eventselect )
        {
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            ERROR("malloc failed for eventselect structure\n");
            goto error;
        }
    }
    sock->eventselect->hEventObject   = hEventObject;
    sock->eventselect->lNetworkEvents = lNetworkEvents;
    sock->eventselect->disabled_events = 0;
        
    return TRUE;
    
error:
    if (sock && sock->eventselect)
    {
        free(sock->eventselect);
        sock->eventselect = NULL;
    }

    SetLastError(dwLastError);
  
    return FALSE;
}

/*++
Function:
 SOCKResetNetworkEvents  
    
--*/
VOID
SOCKResetNetworkEvents(
    int fd,
    LONG lNetworkEvents)
{ 
    ws2_sock    *sock        = NULL;

    TRACE("Resetting network events 0x%x "
          "for socket %d.\n", lNetworkEvents, fd);

    SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

    sock = SOCKGetDataFromAsyncSocket(fd);

    if (!sock) 
    {
        /* should never happened, since this function is always called
           for valid sockets */
        ASSERT("Socket %d is not a valid socket\n", fd);
        goto error;
    }
    
    if (sock->eventselect)
    {
        /* clear the disable_events bits */
        sock->eventselect->disabled_events &= ~lNetworkEvents;
    }
    
    SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

error:
    return;
}

/*++
Function:
  SOCKQueuePushBackOperation
 
 This function assumes the caller will hold the ASYNCSOCKETS_crit_section before
 calling this function 
--*/
BOOL
SOCKQueuePushBackOperation(
    int fd,
    enum ws2_opcode whichQueue, 
    LPVOID *operation)
{ 
    ws2_op_list      *lpQ         = NULL;
    ws2_op_list_node *curr        = NULL;
    DWORD             dwLastError = 0;

    TRACE("Queueing overlapped operation on socket %d\n", fd);
    
    lpQ = SOCKGetQueueFromSocket(fd, whichQueue);

    if (!lpQ) 
    {
        return FALSE;
    }

    /* this memory is freed either in
    
    (1) SOCKProcessOverlappedSend/Recv, after the send/recv operations
        are attempted

    (2) SOCKDeleteAsyncSocket, which is called from closesocket 
        or WSACleanUp 
    */
    curr = (ws2_op_list_node*) malloc( sizeof(ws2_op_list_node) );

    if ( !curr )
    {
        ERROR("Couldn't allocate list node. "
              "Unable to queue overlapped operation for socket %d\n", fd);
        dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    curr->next = NULL;

    if (whichQueue == WS2_OP_SENDTO)
    {
        curr->op.p_sendto = (ws2_op_sendto*) operation;
    }
    else if (whichQueue == WS2_OP_RECVFROM)
    {
        curr->op.p_recvfrom = (ws2_op_recvfrom*) operation;
    }
   
    if ( lpQ->head == NULL )
    {
        lpQ->head = curr;
    }
    else
    {
        lpQ->tail->next = curr;
    }
    lpQ->tail = curr;
  
    return TRUE;
    
error:
    if (curr)
    {
        free(curr);
    }
   
    return FALSE;
}

BOOL 
SOCKFlushOverlappedOperationQueues(int fd, BOOL bSignalEvents)
{
    ws2_sock* sock;
    ws2_op_list_node *curr, *garbage;
    LPWSAOVERLAPPED lpOverlapped;
   
    sock = SOCKGetDataFromAsyncSocket(fd);

    if (!sock) 
    {
        SetLastError(WSAENOTSOCK);
        ERROR("Socket %d is not a valid socket\n", fd);
        return FALSE;
    }

    SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
    
    curr = sock->send_list.head;

    sock->send_list.head = NULL;
    sock->send_list.tail = NULL;

    while( curr )
    {
        garbage = curr;
        curr = curr->next;
        lpOverlapped = garbage->op.p_sendto->lpOverlapped;

        lpOverlapped->Internal     = WSA_OPERATION_ABORTED;
        lpOverlapped->InternalHigh = 0;
        
        if (bSignalEvents && lpOverlapped->hEvent)
        {
            if ( SetEvent(lpOverlapped->hEvent) == FALSE )
            {
                ERROR("Could not signal event!\n");
            }
        }
        
        
        free(garbage->op.p_sendto->lpBuffers);
        free(garbage->op.p_sendto); /* malloc'ed in WSASend */
        free(garbage); /* malloc'ed in SOCKQueuePushBackOperation */
    }


    curr = sock->recv_list.head;
    
    sock->recv_list.head = NULL;
    sock->recv_list.tail = NULL;

    while( curr )
    {
        garbage = curr;
        curr = curr->next;

        lpOverlapped = garbage->op.p_recvfrom->lpOverlapped;
        lpOverlapped->Internal     = WSA_OPERATION_ABORTED;
        lpOverlapped->InternalHigh = 0;
        
        if (bSignalEvents && lpOverlapped->hEvent)
        {
            if ( SetEvent(lpOverlapped->hEvent) == FALSE )
            {
                ERROR("Could not signal event!\n");
            }
        }

        free(garbage->op.p_recvfrom->lpBuffers);
        free(garbage->op.p_recvfrom); /* malloc'ed in WSARecv */
        free(garbage); /* malloc'ed in SOCKQueuePushBackOperation */
    }

    SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
    return TRUE;
}

/*++
Function:
  SOCKIsAsyncSocket

Test whether specified overlapped operation queue is empty or not

Return value:
  FALSE if not an asynchronous socket
--*/
BOOL 
SOCKIsAsyncSocket(
    int fd) 
{
    BOOL      bIsAsyncSocket = FALSE;
    ws2_sock *sock           = NULL;
 
    TRACE("Entering SOCKIsAsyncSocket\n");

    
    if (!SOCKIsQueueEmpty(fd, WS2_OP_SENDTO)   ||
        !SOCKIsQueueEmpty(fd, WS2_OP_RECVFROM))
    {
        bIsAsyncSocket = TRUE;
    }
    else
    {
        sock = SOCKGetDataFromAsyncSocket(fd);

        if (sock && sock->eventselect)
        {
            bIsAsyncSocket = TRUE;
        }
    }

        
    TRACE("Leaving SOCKIsAsyncSocket %d\n", bIsAsyncSocket);
    return bIsAsyncSocket;
}


/*++
Function:
  SOCKWorkerThreadMain

Entry point for the worker thread which will poll() asynchronous sockets
and complete any overlapped operations.

--*/
DWORD SOCKWorkerThreadMain( PVOID phWorkerThreadInitCompleted )
{
    int res;
    int i;


    socket_list.max_fd = SOCK_PIPE_READ;
    socket_list.pollfds = pollfds;
    socket_list.sockets = sockets;

    /* initialize the polled file descriptors arrays so that
       we won't initially poll on any file descriptor BUT the interthread pipe.

       Descriptors are added/removed through SOCKAddAsyncSocket and
       SOCKDeleteAsyncSocket and tested through SOCKAreAsyncOperationsEnabled.
    */
    for ( i = 0; i < FD_SETSIZE; ++i )
    {
        pollfds[i].fd = -1;
    }

    pollfds[SOCK_PIPE_READ].fd = SOCK_PIPE_READ;
    pollfds[SOCK_PIPE_READ].events = POLLIN;

    /* no more direct reference to the arrays below here */

    /* thread initialization completed. Notify WSAStartup. */
    if (!phWorkerThreadInitCompleted ||
        !SetEvent(* (HANDLE*) phWorkerThreadInitCompleted))
    {
        ERROR("Unable to signal thread initialization completion\n");
        return FALSE;
    }

    while(1)
    {
        TRACE("polling %d file descriptors\n", socket_list.max_fd + 1);

        res = poll( SOCKGetPollfdFromAsyncSocket(0),
                    socket_list.max_fd + 1, INFTIM );

        if ( res == -1 )
        {
            if(EINTR == errno)
            {
                TRACE("poll() failed with EINTR; re-polling\n");
                continue;
            }
#if !HAVE_POLL
            // If we're emulating poll on top of select, poll will fail if
            // it's given a socket that has been closed.  In that case, we
            // need to delete the socket and try again.
            else if (errno == EBADF)
            {
                TRACE("poll() failed with EBADF; re-polling\n");
                struct stat sockStat;
                struct pollfd *pollfd;

                for(i = 0; i <= socket_list.max_fd; i++)
                {
                    if (!SOCKAreAsyncOperationsEnabled(i) || i == SOCK_PIPE_READ)
                    {
                        continue;
                    }
                    pollfd = SOCKGetPollfdFromAsyncSocket(i);
                    if (fstat(pollfd->fd, &sockStat) != 0)
                    {
                        // Invalid socket.  Delete it and try to call
                        // poll again.
                        SOCKDeleteAsyncSocket(pollfd->fd, FALSE);
                        break;
                    }
                }
                continue;
            }
#endif  // !HAVE_POLL
            ASSERT("poll() returned -1, errno = %d (%s)\n", 
                   errno, strerror(errno));
        }
        TRACE("poll() returned %d\n", res);

        for ( i = 0; i <= socket_list.max_fd; ++i )
        {

            ws2_sock *sock;
            struct pollfd *pollfd;

            if ( SOCKAreAsyncOperationsEnabled(i) == FALSE || i == SOCK_PIPE_READ )
            {
                continue;
            }

            sock = SOCKGetDataFromAsyncSocket(i);
            pollfd = SOCKGetPollfdFromAsyncSocket(i);

            if(pollfd->revents&POLLNVAL)
            {
                ERROR("pollfd->revents for socket %d contains POLLNVAL!\n",
                       pollfd->fd);
                /* the socket is invalid. if we leave it in the list, poll()
                   will always return immediately with POLLNVAL, so remove it.
                   this shouldn't happen, the socket should have been removed
                   from the list by whoever made it invalid */
                SOCKDeleteAsyncSocket(pollfd->fd, FALSE);
            }

            else if ( pollfd->revents & POLLHUP )
            {
                /* the socket is disconnected. if we leave it in the list, poll()
                   will always return immediately with POLLHUP, so remove it.*/
                SOCKDeleteAsyncSocket(pollfd->fd, FALSE);
            }
            else if ( pollfd->revents & POLLERR )
            {
                /* An exceptional condition has occurred on the socket .
                   if we leave it in the list, poll() will always
                   return immediately with POLLERR, so remove it.*/
                SOCKDeleteAsyncSocket(pollfd->fd, FALSE);
            }

            else if ( pollfd->revents & POLLIN)
            {
                SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

                if ( sock->eventselect &&
                    sock->eventselect->lNetworkEvents & FD_ACCEPT)
                {
                    TRACE("Socket %d is ready to ACCEPT. Event Signalled.\n", i);
                    if ( SetEvent(sock->eventselect->hEventObject) == FALSE )
                    {
                        ERROR("Could not signal event!\n");
                    }
                    /* disable accept events for now */
                    sock->eventselect->disabled_events |= FD_ACCEPT;
                    TRACE("disabled_events = %#lx\n",
                          sock->eventselect->disabled_events);
                }

                SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

                SOCKProcessOverlappedRecv(i);
            }

            else if (pollfd->revents & POLLOUT)
            {

                SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

                /*check if the given event is a connect and signal it*/
                if ( sock->eventselect &&
                    sock->eventselect->lNetworkEvents & FD_CONNECT)
                {
                    TRACE("Socket %d is ready to connect. Event Signalled.\n", i);
                    if ( SetEvent(sock->eventselect->hEventObject) == FALSE )
                    {
                        ERROR("Could not signal event!\n");
                    }
                    /* disable connect events for now */
                    sock->eventselect->disabled_events |= FD_CONNECT;
                    TRACE("disabled_events = %#lx\n",
                          sock->eventselect->disabled_events);
                }

                SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

                SOCKProcessOverlappedSend(i);
            }
    
            SOCKRefreshPollfdEvents(i);
        }

        /* read data from the pipe if there is some */
        if ( pollfds[SOCK_PIPE_READ].revents & POLLIN )
        {
            SOCKProcessAsyncOperations();
        }
    }
}


/*++
Function:
  SOCKRefreshPollfdEvents

In the pollfd structure associated with the given socket, reset the 'events'
field to the appropriate value. This depends on any pending overlapped
operations, as well as any preceeding calls to WSAEventSelect on this
socket. If there are no more pending overlapped operations on the socket,
SOCKDeleteAsyncSocket will be called to clean it up.
--*/
static void SOCKRefreshPollfdEvents( int fd )
{
    short new_events = 0;
    ws2_sock *sock;
    BOOL alive = FALSE;
    

    sock = SOCKGetDataFromAsyncSocket(fd);

    SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

    if ( sock->eventselect )
    {
        /* XOR desired events with disabled events */
        new_events |= 
            SOCKNetworkEventsToPollEvents(sock->eventselect->lNetworkEvents ^ 
                                          sock->eventselect->disabled_events);

        TRACE("socket: %d, eventselect->lNetworkEvents = %#lx, "
              "disabled_events = %#lx\n",
              fd,
              sock->eventselect->lNetworkEvents, 
              sock->eventselect->disabled_events);

        if (new_events)
            alive = TRUE;
    }
    
    /* check for pending send operations */
    if ( sock->send_list.head )
    {
        alive = TRUE;
        TRACE("there are pending send operations\n");
        new_events |= POLLOUT;
    }
    
    /* check for pending recv operations */
    if ( sock->recv_list.head )
    {
        alive = TRUE;
        TRACE("there are pending recv operations\n");
        new_events |= POLLIN;
    }
   
    if ( alive )
    {
        /* make sure the socket is in the poll list */
        SOCKAddAsyncSocket(fd);
        TRACE("new events on socket %d are 0x%hx\n", fd, new_events);
        SOCKGetPollfdFromAsyncSocket(fd)->events = new_events;

    }
    else
    {
        TRACE("No more asynchronous operations on socket %d\n", fd);

        /* 
           we cannot destroy the socket if events are associated with it;
           it's possible that we must reenable the events for that socket
           in the future. In that case, just remove it from the polllist,
           otherwise it is safe to delete it.
        */
        if (sock->eventselect)
        {
           SOCKRemoveSocketFromPollList(fd);
        }
        else
        {
           SOCKDeleteAsyncSocket(fd, FALSE);
        }
    }
    
    SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
}

/*++
Function:
  SOCKProcessAsyncOperations

Used by the worker thread to read any pending asynchronous operations
from the pipe.
--*/
static void SOCKProcessAsyncOperations()
{
    int      fd;
    int      res;
    ws2_op   sock_op;
    
    while ( (res = read(SOCK_PIPE_READ, &sock_op, sizeof(sock_op)))
            == sizeof(sock_op) )
    {
        TRACE("read returned %d\n", res);

        TRACE("Operation read from pipe -> s = %d, opcode = %d, "
              "hEvent = 0x%p\n", (int)sock_op.s, 
              sock_op.opcode, sock_op.event);

        fd = (int)sock_op.s;

        switch( sock_op.opcode )
        {
        case WS2_OP_EVENTSELECT:
            /* fall through */
        case WS2_OP_SENDTO:
            /* fall through */
        case WS2_OP_RECVFROM:
        {
            /* some operation was queued by WSASendTo/RecvFrom/EventSelect
               Enable processing of these operations. 
            */
            SOCKAddAsyncSocket(fd);
            break;
        }
        case WS2_OP_CLOSESOCKET:
            if ( SOCKDeleteAsyncSocket(fd, FALSE) == FALSE )
            {
                WARN("Unable to clean up properly, probable memory leak\n");
            }

            if ( SetEvent(sock_op.event) == FALSE )
            {
                ERROR("Unable to signal event\n");
            }
            break;
        case WS2_OP_STOPTHREAD:
        {
            TRACE("Got WS2_OP_STOPTHREAD; worker thread terminating.\n");
            while (socket_list.max_fd != -1) 
            {
                SOCKDeleteAsyncSocket(socket_list.max_fd, TRUE);
            }
            DeleteCriticalSection( &ASYNCSOCKETS_crit_section );

            /* don't call ExitThread, we don't want to call DllMain */
            TerminateCurrentThread(0);
            break;
        }
        default:
            WARN("Unknown operation, ignoring.\n");
        }

        /* don't call this on WS2_OP_CLOSESOCKET because the socket has
           already been cleaned up */
        if ( sock_op.opcode != WS2_OP_CLOSESOCKET && SOCKAreAsyncOperationsEnabled(fd) )
        {
            SOCKRefreshPollfdEvents(fd);
        }
    }
}


/*++
Function:
  SOCKProcessOverlappedSend
--*/
static void SOCKProcessOverlappedSend( int fd )
{
    int    res;
    fd_set write_set;
  
    ws2_sock *sock;
    ws2_op_list_node *curr;
    ws2_op_list_node *next;
    ws2_op_list_node *cleanup;

    ws2_op_sendto *op_sendto;

  
    FD_ZERO(&write_set);
    if (FD_SET_BOOL(fd, &write_set) == FALSE)
    {
        ERROR("Invalid socket value\n");
        return;
    }
    
    sock = SOCKGetDataFromAsyncSocket(fd);

    SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
    
    if ( !sock->send_list.head )
    {
        TRACE("There are no overlapped send operations pending\n");
        SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
        return;
    } 
    
    /* A quick look at the BCL code shows that lpOverlapped->Internal 
       should store the last error for the current overlapped operation.
      
       Also InternalHigh should hold the number of bytes processed so far
       for that overalapped operation.
 
       See System.Net.Win32::HackedGetOverlappedResult
    */
    curr = sock->send_list.head;

    while (curr)
    {
        DWORD bytesSent = 0;
        WSAOVERLAPPED *lpOverlapped;
        WSABUF *lpBuffers;
        
        op_sendto    = curr->op.p_sendto;
        lpOverlapped = op_sendto->lpOverlapped; 
        lpBuffers    = op_sendto->lpBuffers;
        
        /* Here we need to call WSASend/WSASendTo and get in the 
           non-overlapped I/O code paths. 
           That's why the last two parameters are NULL.
              
           This will set the CURRENT (worker) thread's last error.
           How the last error is reported to the application is done below.
        */
        if ( op_sendto->lpTo )
        {
            res = WSASendTo( fd, 
                             lpBuffers, 
                             1, 
                             &(bytesSent),
                             op_sendto->dwFlags, 
                             op_sendto->lpTo,
                             op_sendto->iToLen, 
                             (LPWSAOVERLAPPED) NULL, 
                             (LPWSAOVERLAPPED_COMPLETION_ROUTINE) NULL );
        }
        else
        {
            res = WSASend( fd, 
                           lpBuffers, 
                           1,  
                           &(bytesSent),
                           op_sendto->dwFlags,
                           (LPWSAOVERLAPPED) NULL,
                           (LPWSAOVERLAPPED_COMPLETION_ROUTINE) NULL);
        }
        
        /* couldn't send the buffer */
        if (SOCKET_ERROR == res)
        {
            if (GetLastError() == WSAEWOULDBLOCK) 
            {
                /* get out of this loop. 
                   Current operation is now the head in the overlapped 
                   operations queue */
                break;
            }
            else
            {
                /* any other kind of problem cause the overlapped operation to
                   be considered as a failure. Update the overlapped
                   structure with error info */

                /* save the last error in the overlapped structure. */
                lpOverlapped->Internal = GetLastError();
            }
        }
        else
        { 
            /* update InternalHigh field with the amount of bytes we sent so
              far */
            lpOverlapped->InternalHigh = bytesSent;

            /* report 'no error' */
            lpOverlapped->Internal = 0;
        }
        
     
        /* overlapped structure is in its final state - operation completed.
           Set the event! */
        if ( SetEvent(lpOverlapped->hEvent) == FALSE )
        {
            ERROR("Unable to set event %p\n", lpOverlapped->hEvent);
        }
  
        /* don't try to send next buffer if last operation produced an error. */ 
        curr = curr->next;
        if (lpOverlapped->Internal)
        {
             break;
        }
    }
 

    /* now free all the memory that was allocated for the send operations
       that got processed.

       first step is to set the local variable "cleanup" to the start of the
       queue and move the head to the current (the new head) operation. 
       This way we can release the critical section faster.
    */
    cleanup = sock->send_list.head;
    sock->send_list.head = curr;

    if (curr == NULL)
    {
        /* we processed all pending send operations.
           send_list tail ptr must be set to NULL as well */
        sock->send_list.tail = NULL;
    }

    SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

    while (cleanup != curr)
    {
        /* this memory is malloc'ed in WSASend */
        free( cleanup->op.p_sendto->lpBuffers);
        free( cleanup->op.p_sendto );

        /* this memory is malloc'ed in SOCKQueuePushBackOperation
           after it receives the operation from the pipe */
        next = cleanup->next;
        free(cleanup);
        cleanup = next;
    }

}

/*++
Function:
  SOCKProcessOverlappedRecv
--*/
static void SOCKProcessOverlappedRecv( int fd )
{
    ws2_sock *sock ;
    int    res;
    fd_set read_set;

    ws2_op_list_node *curr;
    ws2_op_list_node *next;
    ws2_op_list_node *cleanup;
    ws2_op_recvfrom  *op_recvfrom;
    
    SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

    FD_ZERO(&read_set);
    if (FD_SET_BOOL(fd, &read_set) == FALSE)
    {
        ERROR("Invalid socket value\n");
        SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
        return;
    }

    sock = SOCKGetDataFromAsyncSocket(fd);

    if ( !sock->recv_list.head )
    {
        TRACE("There are no overlapped recv operations pending\n");
        SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
        return;
    }
    
    
    /* A quick look at the BCL code shows that lpOverlapped->Internal 
       should store the last error for the current overlapped operation.
      
       Also InternalHigh should hold the number of bytes processed so far
       for that overalapped operation.

       See System.Net.Win32::HackedGetOverlappedResult
    */
    curr = sock->recv_list.head;

    while(curr)
    {
        DWORD bytesRecvd = 0;
        WSAOVERLAPPED *lpOverlapped;
        WSABUF *lpBuffers;
        
        op_recvfrom  = curr->op.p_recvfrom;
        lpOverlapped = op_recvfrom->lpOverlapped; 
        lpBuffers    = op_recvfrom->lpBuffers;

        /* Here we need to call WSARecv/WSARecvFrom and get in the 
           non-overlapped I/O code paths. 
           That's why the last two parameters are NULL.
              
           This will set the CURRENT (worker) thread's last error.
           How the last error is reported to the application is done below.
        */
        if ( op_recvfrom->lpFrom )
        {
            res = WSARecvFrom( fd, 
                               lpBuffers, 
                               1, 
                               &(bytesRecvd) ,
                               &(op_recvfrom->dwFlags), 
                               op_recvfrom->lpFrom, 
                               op_recvfrom->lpFromLen, 
                               (LPWSAOVERLAPPED) NULL, 
                               (LPWSAOVERLAPPED_COMPLETION_ROUTINE) NULL );
        }
        else
        {
            res = WSARecv( fd, 
                           lpBuffers, 
                           1,  
                           &(bytesRecvd) ,
                           &(op_recvfrom->dwFlags),
                           (LPWSAOVERLAPPED) NULL, 
                           (LPWSAOVERLAPPED_COMPLETION_ROUTINE) NULL);
        }

        /* couldn't recv the buffer */
        if (res == SOCKET_ERROR)
        {
            if (GetLastError() == WSAEWOULDBLOCK) 
            {               
                /* get out of this loop. 
                   Current operation is now the head in the overlapped 
                   operations queue */
                break;
            }
            else
            {
                /* save the last error in the overlapped structure */
                lpOverlapped->Internal = GetLastError();
            }
        }
        else
        { 
            /* update InternalHigh field with the amount of bytes we received so
               far */
            lpOverlapped->InternalHigh = bytesRecvd;

            /* report 'no error' */
            lpOverlapped->Internal = 0;
        }
     
        /* overlapped structure is in its final state - operation completed.
           Set the event! */
        if ( SetEvent(lpOverlapped->hEvent) == FALSE )
        {
            ERROR("Unable to set event 0x%x\n", (DWORD) lpOverlapped->hEvent);
        }

        /* don't try to recv next buffer if last operation produced an error. */ 
        curr = curr->next;
        if (lpOverlapped->Internal)
        {
             break;
        }
    }
    
    /* now free all the memory that was allocated for the recv operations
       that got processed.

       first step is to set the local variable "cleanup" to the start of the
       queue and move the head to the current (the new head) operation. 
       This way we can release the critical section faster.
    */
    cleanup = sock->recv_list.head;
    
    sock->recv_list.head = curr;
    
    if (curr == NULL)
    {
        /* we processed all pending recv operations.
           recv_list tail ptr must be set to NULL as well */
        sock->recv_list.tail = NULL;
    }
    
    SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

    /* now free all the memory that was allocated for the recv operations
       that got processed */
    while (cleanup != curr)
    {
        /* this memory is malloc'ed in WSARecv */
        free( cleanup->op.p_recvfrom->lpBuffers);
        free( cleanup->op.p_recvfrom );
        
        /* this memory is malloc'ed in SOCKQueuePushBackOperation,
           after it receives the operation from the pipe */
        next = cleanup->next;
        free(cleanup);
        cleanup = next;
    }
}


/*++
Function:
  SOCKNetworkEventsToPollEvents

Convert a network events bitmask (e.g. provided as the third argument to
WSAEventSelect) to a file descriptor events bitmask as used by poll(2).
--*/
static short SOCKNetworkEventsToPollEvents( long network_events )
{
    short ret = 0;

    TRACE("network_events = %#lx\n", network_events);

    if ( network_events & FD_ACCEPT)
    {
        ret |= POLLIN;
    }
    if ( network_events & FD_CONNECT)
    {
        ret |= POLLOUT;
    }

    TRACE("resulting poll events = 0x%hx\n", ret);

    return ret;
}


/*++
Function:
  SOCKAddAsyncSocket
--*/
static void SOCKAddAsyncSocket( int fd )
{
    TRACE("Adding socket %d to the async list\n", fd);

    SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

    if (SOCKAreAsyncOperationsEnabled(fd))
    {
        SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
        return;
    }
    
    socket_list.pollfds[fd].fd = fd;

    if ( fd > socket_list.max_fd )
    {
        socket_list.max_fd = fd;
    }

    SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
}

/*++
Function:
  SOCKDeleteAsyncSocket
--*/
static BOOL SOCKDeleteAsyncSocket( int fd, BOOL bThreadCleanup )
{
    ws2_sock* sock;
    BOOL bRet;
    
    TRACE("removing socket %d from the async list\n", fd);

    if ( !SOCKAreAsyncOperationsEnabled(fd) )
    {
        ERROR("Cannot cleanup up asynchronous socket %d, "
               "it's not in the list\n", fd);
        return FALSE;
    }
   
    sock = SOCKGetDataFromAsyncSocket(fd);

    if (!sock) 
    {
        SetLastError(WSAENOTSOCK);
        ERROR("Socket %d is not a valid socket\n", fd);
        return FALSE;
    }
    
    SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
    
    SOCKRemoveSocketFromPollList(fd);
   
    /* malloc'ed in SOCKAssignEventObjectToNetworkEvents */
    free( sock->eventselect );     
    sock->eventselect = NULL;
    
    /* don't signal events if we are doing thread cleanup */
    bRet = SOCKFlushOverlappedOperationQueues( fd, 
                                               bThreadCleanup ? FALSE : TRUE);
 
    SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
    
    return bRet;
}



static void 
SOCKRemoveSocketFromPollList( int fd )
{
    /* find the new highest file descriptor */
    socket_list.pollfds[fd].fd = -1;
    if (fd == socket_list.max_fd)
    {
        while ( fd >= 0 && !SOCKAreAsyncOperationsEnabled(fd))
        {
            --fd;
        }

        socket_list.max_fd = fd;
    }
}

/*++
Function:
  SOCKAreAsyncOperationsEnabled

Worker thread differentiates active (with operations) vs non-active async 
socket by testing whether the associated socket's pollfd structure 
has its file descriptor set to -1.

That file descriptor is set in SOCKAddAsyncSocket and cleared (set to -1) in 
SOCKDeleteAsyncSocket

Return values:
    TRUE  if socket is an active async socket
    FALSE otherwise
--*/
static BOOL SOCKAreAsyncOperationsEnabled( int fd )
{
    BOOL bAsyncSocket;

    bAsyncSocket = ! ( socket_list.pollfds[fd].fd == -1 );

    return bAsyncSocket;
}


/*++
Function:
  SOCKGetPollfdFromAsyncSocket
--*/
static struct pollfd *SOCKGetPollfdFromAsyncSocket( int fd )
{
    if (fd >= FD_SETSIZE)
    {
        return NULL;
    }

    return &(socket_list.pollfds[fd]);
}

/*++
Function:
  SOCKGetDataFromAsyncSocket
--*/
static ws2_sock *SOCKGetDataFromAsyncSocket( int fd )
{
    if (fd >= FD_SETSIZE)
    {
        return NULL;
    }

    return &(socket_list.sockets[fd]);
}
