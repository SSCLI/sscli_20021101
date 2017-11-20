/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    socket2.c

Abstract:

    Implementation of the Winsock 2 API required for the PAL

Notes:

Important! The SOCKET (PAL_SOCKET) type is NOT managed by the handle
manager, therefore CloseHandle cannot be called on a socket, unlike
in win32. See socket.c.

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include "pal/file.h"
#include "pal/socket2.h"
#include "pal/critsect.h"
#include "pal/thread.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#if HAVE_POLL
#include <poll.h>
#else
#include "pal/fakepoll.h"
#endif  // HAVE_POLL
#include <sys/uio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#if HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif  // HAVE_SYS_FILIO_H
#if HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif  // HAVE_SYS_SOCKIO_H


SET_DEFAULT_DEBUG_CHANNEL(SOCKET);

CRITICAL_SECTION SOCK_crit_section;
CRITICAL_SECTION ASYNCSOCKETS_crit_section;

static HANDLE SOCK_worker_thread_handle = INVALID_HANDLE_VALUE;

#define INIT_TIMEOUT 5000

int     SOCK_startup_count = 0;
pthread_key_t SOCK_tlsHostentKey = 0;

int SOCK_pipe_fd[2] = { -1, -1 };
#define SOCK_PIPE_READ  SOCK_pipe_fd[0]
#define SOCK_PIPE_WRITE SOCK_pipe_fd[1]

static void  SOCKCommonCleanup();
static BOOL  IsValidSocket( PAL_SOCKET s );


/* This is needed for WSAIoctl */
#define IOC_TYPE_MASK           0x3
#define IOC_TYPE_UNIX           0x0
#define IOC_TYPE_WINSOCK2       0x1
#define IOC_TYPE_SPECIFICADDR   0x2
#define IOC_TYPE_VENDORDEFINED  0x3


/*++
Function:
  WSAStartup

See MSDN doc.
--*/
int
PALAPI
WSAStartup(
       IN WORD wVersionRequired,
       OUT LPWSADATA lpWSAData)
{
    DWORD thread_id;
    int Ret = 0;

    ENTRY("WSAStartup (wVersionRequired=0x%x, lpWSAData=0x%p)\n",
          wVersionRequired, lpWSAData);


    if ( wVersionRequired != 0x202 )
    {
        ERROR("wVersionRequired is not 2.2\n");
        Ret = WSAVERNOTSUPPORTED;
        goto done;
    }

    if (lpWSAData)
    {
        WARN("lpWSAData is non-NULL, but this implementation ignores it\n");

        lpWSAData->wVersion = 0x202;
    }

    SYNCEnterCriticalSection( &SOCK_crit_section , TRUE);
    if ( SOCK_startup_count == 0 )
    {
        DWORD WaitResult;
        HANDLE hWorkerThreadInitCompleted = NULL;

        if ( pipe(SOCK_pipe_fd) != 0 )
        {
            ERROR("Could not create pipe for communication "
                  "with worker thread\n");
            Ret = WSASYSNOTREADY;
            goto error_cleanup;
        }
        TRACE("Created pipe, file descriptors are %d (read) and %d (write)\n",
              SOCK_PIPE_READ, SOCK_PIPE_WRITE);

        /* make the read end of the pipe non-blocking */
        if ( fcntl(SOCK_PIPE_READ, F_SETFL, O_NONBLOCK) == -1 )
        {
            ERROR("Could not make file descriptor %d non-blocking\n", 
                  SOCK_PIPE_READ);
            Ret = WSASYSNOTREADY;
            goto error_cleanup;
        }

        /* event on which the worker thread will notify us when ready */
        hWorkerThreadInitCompleted = CreateEvent( 
                                            NULL,  /* security descriptor */
                                            TRUE,  /* manual reset */
                                            FALSE, /* initial state */
                                            NULL); /* name */
       
        if (NULL == hWorkerThreadInitCompleted)
        {
            ERROR("Could not create thread init notification event object\n");
            Ret = WSASYSNOTREADY;
            goto error_cleanup;
        }

        /* start the worker thread */
        SOCK_worker_thread_handle = 
            CreateThread( NULL, 0, 
                          (LPTHREAD_START_ROUTINE)SOCKWorkerThreadMain,
                          (PVOID) &hWorkerThreadInitCompleted, 0, &thread_id );
        if ( SOCK_worker_thread_handle == NULL )
        {
            ERROR("Could not create worker thread\n");
            if(CloseHandle(hWorkerThreadInitCompleted)== FALSE)
            {
                WARN("Unable to close worker thread handle\n");
            }
            hWorkerThreadInitCompleted = NULL;
            Ret = WSASYSNOTREADY;
            goto error_cleanup;
        }

        /* give it some setup time... */
        WaitResult = WaitForSingleObject(
                        hWorkerThreadInitCompleted,
                        INIT_TIMEOUT);

        if (WaitResult != WAIT_OBJECT_0)
        {
            ERROR("Unexpected failure while waiting for worker thread "
                  "initialization\n");

            if(CloseHandle(hWorkerThreadInitCompleted)== FALSE)
            {
                WARN("Unable to close worker thread handle\n");
            }
            hWorkerThreadInitCompleted = NULL;
            Ret = WSASYSNOTREADY;
            goto error_cleanup;
        }

        if(CloseHandle(hWorkerThreadInitCompleted)== FALSE)
        {
            WARN("Unable to close worker thread handle\n");
        }
        hWorkerThreadInitCompleted = NULL;
    }

    ++SOCK_startup_count;
    TRACE("startup count is now %d\n", SOCK_startup_count);

    SYNCLeaveCriticalSection( &SOCK_crit_section , TRUE);
    goto done;

error_cleanup:
    if (SOCK_worker_thread_handle != INVALID_HANDLE_VALUE)
    {
        SOCKStopWorkerThread();
    
        if ( CloseHandle( SOCK_worker_thread_handle ) == FALSE )
        {
            WARN("Unable to reclaim worker thread handle\n");
        }
    }

    {
        int i;
        /* close the pipe if it was opened */
        for( i = 0; i <= 1; ++i )
        {
            if ( SOCK_pipe_fd[i] != -1 )
            {
                if ( close(SOCK_pipe_fd[i]) != 0 )
                {
                    ERROR("Could not close file descriptor %d\n",
                          SOCK_pipe_fd[i]);
                }
            }
            SOCK_pipe_fd[i] = -1;
        }
    }

    SYNCLeaveCriticalSection( &SOCK_crit_section , TRUE);

done:
    LOGEXIT("WSAStartup returns int %d\n", Ret);
    return Ret;
}


/*++
Function:
  WSACleanup


See MSDN doc.
--*/
int
PALAPI
WSACleanup(
       void)
{
    ENTRY("WSACleanup(void)\n");

    SYNCEnterCriticalSection(&SOCK_crit_section, TRUE);

    if ( SOCK_startup_count == 0 )
    {
        LeaveCriticalSection(&SOCK_crit_section);
        SetLastError(WSANOTINITIALISED);
        ERROR("No preceding successful call to WSAStartup\n");
        LOGEXIT("WSACleanup returns int %d\n", SOCKET_ERROR);
        return SOCKET_ERROR;
    }

    --SOCK_startup_count;
    TRACE("startup count is now %d\n", SOCK_startup_count);
    if ( SOCK_startup_count == 0 )
    {
        SOCKCommonCleanup();
    }

    LeaveCriticalSection(&SOCK_crit_section);

    LOGEXIT("WSACleanup returns int 0\n");
    return 0;
}


/*++
Function:
  WSASend

See MSDN doc.
--*/
int
PALAPI
WSASend(
    IN PAL_SOCKET s,
    IN LPWSABUF lpBuffers,
    IN DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN DWORD dwFlags,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int ret;

    ENTRY("WSASend (s=%#x, lpBuffers=%p, dwBufferCount=%u, lpNumberOfBytesSent=%p, "
          "dwFlags=%#x, lpOverlapped=%p, lpCompletionRoutine=%#x)\n", 
          s, lpBuffers, dwBufferCount, lpNumberOfBytesSent,
          dwFlags, lpOverlapped, lpCompletionRoutine);

    ret = WSASendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent,
                     dwFlags, NULL, 0, lpOverlapped, lpCompletionRoutine);

    LOGEXIT("WSASend returns int %d\n", ret);
    return ret;
}


/*++
Function:
  WSASendTo

See MSDN doc.
--*/
int
PALAPI
WSASendTo(
      IN PAL_SOCKET s,
      IN LPWSABUF lpBuffers,
      IN DWORD dwBufferCount,
      OUT LPDWORD lpNumberOfBytesSent,
      IN DWORD dwFlags,
      IN const struct PAL_sockaddr FAR *lpTo,
      IN int iToLen,
      IN LPWSAOVERLAPPED lpOverlapped,
      IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int    ret = SOCKET_ERROR;
    DWORD  dwLastError = 0;

    ENTRY("WSASendTo (s=%#x, lpBuffers=%p, dwBufferCount=%u, "
          "lpNumberOfBytesSent=%p, dwFlags=%#x, lpTo=%p, iTolen=%d, "
          "lpOverlapped=%p, lpCompletionRoutine=%#x)\n", 
          s, lpBuffers, dwBufferCount, lpNumberOfBytesSent,
          dwFlags, lpTo, iToLen, lpOverlapped, lpCompletionRoutine);

    if ( SOCK_startup_count == 0 )
    {
        ERROR("No successful WSAStartup call\n");
        dwLastError = WSANOTINITIALISED;
        goto done;
    }

    if ( lpCompletionRoutine )
    {
        ASSERT("lpCompletionRoutine is not NULL, as it should be\n");
        dwLastError = WSAEINVAL;
        goto done;
    }

    if (IsBadWritePtr(lpNumberOfBytesSent, sizeof(DWORD)))
    {
        ERROR("lpNumberOfBytesSent is not totally contained in valid "
              "user memory space\n");
        dwLastError = WSAEFAULT;
        goto done;
    }
    if (IsBadReadPtr(lpBuffers, sizeof(WSABUF)) ||
        IsBadReadPtr(lpBuffers[0].buf, lpBuffers[0].len))
    {
        ERROR("lpBuffers is not totally contained in valid "
              "user memory space\n");
        dwLastError = WSAEFAULT;
        goto done;
    }

    if ( IsValidSocket(s) == FALSE )
    {
        ERROR("Socket %d is not a valid socket\n", s);
        dwLastError = WSAENOTSOCK;
        goto done;
    }

    /* MSG_OOB is not supported for UDP and FreeBSD ignores the flags
       and proceeds with the operation instead of reporting an unsupported
       operation
    */
    if (dwFlags & PAL_MSG_OOB)
    {
        socklen_t sizeof_int  = sizeof(int);
        int       socket_type = 0;
    
        /*
           First, we need to get the socket type...
        */
        if ( (getsockopt(s, SOL_SOCKET, SO_TYPE, &socket_type, &sizeof_int )) < 0)
        {
            ERROR("getsockopt failed for socket %d\n", s);
            dwLastError = SOCKErrorReturn(errno);
            goto done;
        }
       
        if ((socket_type == SOCK_DGRAM) || (socket_type == SOCK_RAW))
        {
            ERROR("Unsupported operation for non-stream socket\n");
            dwLastError = WSAEOPNOTSUPP;
            goto done;
        }
    }
    
    /* non-overlapped mode OR overlapped mode but the overlapped send queue 
       is empty - e.g. maybe we can process this operation right away.

       Here we access the queue without the queues critical section.
       There is no need to hold the critical section here since we are
       certain the worker thread won't add anything to the queue - all
       the operations are queued in the application threads (see 
       SOCKQueuePushBackOperation call below.

       There is a special case where two threads might be trying to 
       queue an operation simultaneously and that is effectively a race...
       but it's first and foremost an application design issue!
    */
    if (!lpOverlapped || 
         (lpOverlapped && SOCKIsQueueEmpty (s, WS2_OP_SENDTO) ) )
    {
        /* for overlapped, we need to initialize the WSAOVERLAPPED structure
           Internal fields */
        if (lpOverlapped)
        {
            struct pollfd fds;

            fds.fd = s;
            fds.events = POLLOUT;
           
            lpOverlapped->InternalHigh = 0;
            lpOverlapped->Internal     = 0;
            
            /* We are possibly dealing with blocking sockets.
               Because this is an overlapped socket (lpOverlapped != NULL),  
               we don't want to block but rather add the send operation 
               to the queue if the operation cannot be carried right away. 
               To verify whether we can write on the socket without blocking,
               we use poll in POLLOUT mode
            */
again:
            if (-1 == poll(&fds, 1, 0))
            {
                if (errno == EINTR)
                {
                    TRACE("poll() failed with EINTR; re-polling\n");
                    goto again;
                }
                dwLastError = SOCKErrorReturn(errno);
                goto done;
            }
            else if (fds.revents & POLLERR)
            {
                ERROR("Exceptional condition has occured on the socket\n");
                dwLastError = WSAENOTCONN;
                goto done;
            }
            else if (fds.revents & POLLHUP)
            {
                ERROR("Socket was disconnected\n");
                dwLastError = WSAENOTCONN;
                goto done;
            }
            else if (fds.revents & POLLNVAL)
            {
                ERROR("Invalid socket\n");
                dwLastError = WSAENOTSOCK;
                goto done;
            }
            else if (!(fds.revents & POLLOUT))
            {
                goto overlapped;
            }
        }

        if (lpTo)
        {
            /* sendto */
            struct sockaddr UnixSockaddr;

            // Copy the PAL structure for sockaddr to the native structure.
#if HAVE_SOCKADDR_SA_LEN
            UnixSockaddr.sa_len = sizeof(UnixSockaddr);
#endif  // HAVE_SOCKADDR_SA_LEN
            UnixSockaddr.sa_family = lpTo->sa_family;
            memcpy(UnixSockaddr.sa_data, lpTo->sa_data, sizeof(lpTo->sa_data));
    
            
            ret = sendto((int)s, 
                          lpBuffers[0].buf, lpBuffers[0].len, 
                          (int) dwFlags,
                          &UnixSockaddr, iToLen);

        }
        else
        {
            /* send */
            ret = send((int)s,
                        lpBuffers[0].buf, lpBuffers[0].len,
                        (int) dwFlags); 
        }

        /* we could not process this request right now, queue it */
        if (ret < 0)
        {
            if (errno == EAGAIN && lpOverlapped)
            {
                TRACE("Send returned EAGAIN, queuing send operation\n");
                goto overlapped;
            }
            else if (errno == EPIPE)
            {
                /* we should be getting a SIGPIPE signal here, but since
                   the default behavior for SIGPIPE is process termination,
                   we preferred to tell the system that we want to ignore 
                   SIGPIPE signals and be notified through EPIPE error message
                   instead. (see exception/signal.c::SEHInitializeSignals) 
                   
                   In the context of sockets, SIGPIPE is issued when the
                   connection was dropped.
                */
                TRACE("Connection has been dropped.\n");
                dwLastError = WSAESHUTDOWN;
            }
            else
            {
                TRACE("Error happened (%d) and it wasn't EAGAIN.", errno);
                dwLastError = SOCKErrorReturn(errno);
            }
        }
        else 
        {
            TRACE("Send operation completed succesfully. Bytes sent %d\n", ret);
            *lpNumberOfBytesSent = ret;

            if (lpOverlapped)
            {
                lpOverlapped->InternalHigh = ret;
        
                if ( SetEvent(lpOverlapped->hEvent) == FALSE )
                {
                    ERROR("Could not signal event!\n");
                }
            }
            ret = 0;
        }
    }
    else
overlapped:
    {
        ws2_op_sendto *sendto_op = NULL;
        
        /* We should never attempt to overlap an operation if no peer is 
           associated with the stream socket */
        if ( SOCKIsSocketConnected(s) <= 0)
        {
            dwLastError = GetLastError();
            goto done;
        }

        sendto_op = malloc( sizeof(ws2_op_sendto) );
        if ( !sendto_op ) 
        {   
            ERROR("Couldn't allocate sendto operation\n");
            
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }


        /* 
        This memory is freed in SOCKQUEURemoveBack, called by 
        SOCKProcessOverlappedSend or SOCKDeleteAsyncSocket

        Also, there's ALWAYS only 1 WSABUF structure (see rotor_pal.doc) 
        */
        sendto_op->lpBuffers = (WSABUF*) malloc( sizeof(WSABUF) );
        if ( !sendto_op->lpBuffers)
        {
            ERROR("Couldn't allocate internal copy of WSABUF structure\n");
            
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            free(sendto_op);
            goto done;
        }

        /* initialize Internal and InternalHigh fields of
           WSAOVERLAPPED */
        lpOverlapped->Internal     = 0;
        lpOverlapped->InternalHigh = 0;
    
        /* NB: we don't need to keep dwBufferCount, it's always 1 */
        sendto_op->lpBuffers[0].buf    = lpBuffers[0].buf;
        sendto_op->lpBuffers[0].len    = lpBuffers[0].len;
        sendto_op->dwFlags             = dwFlags;
        sendto_op->lpTo                = lpTo;
        sendto_op->iToLen              = iToLen;
        sendto_op->lpOverlapped        = lpOverlapped;
        
        SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

        if (FALSE == 
            SOCKQueuePushBackOperation((int)s, WS2_OP_SENDTO, (LPVOID) sendto_op))
        {
            ERROR("couldn't push back operation in overlapped send queue\n");
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            free(sendto_op->lpBuffers);
            free(sendto_op);
            SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
            goto done;
        }

        dwLastError = SOCKSendRequestForAsyncOp(s, WS2_OP_SENDTO, NULL);

        if (dwLastError)
        {
            ret = SOCKET_ERROR;
            if (FALSE == 
                SOCKQueueRemoveOperation((int)s, WS2_OP_SENDTO, sendto_op))
            {
                ERROR("couldn't remove operation from queue (memory leak)\n");
            }
            SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
            goto done;
        }

        SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
        dwLastError = WSA_IO_PENDING;
        ret = SOCKET_ERROR;
    }

done:
    if (dwLastError)
    {
        if (lpOverlapped && dwLastError != WSA_IO_PENDING)
        {
            lpOverlapped->Internal     = dwLastError;
            lpOverlapped->InternalHigh = 0;
            if ( SetEvent(lpOverlapped->hEvent) == FALSE )
            {
                ERROR("Could not signal event!\n");
            }
        }
        SetLastError(dwLastError);
    }

    LOGEXIT("WSASendTo returns int %d\n", ret);
    return ret;
}


/*++
Function:
  WSARecv

See MSDN doc.
--*/
int
PALAPI
WSARecv(
    IN PAL_SOCKET s,
    IN OUT LPWSABUF lpBuffers,
    IN DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesRecvd,
    IN OUT LPDWORD lpFlags,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int ret;

    ENTRY("WSARecv (s=%#x, lpBuffers=%p, dwBufferCount=%u, lpNumberOfBytesRecvd=%p,"
          "lpFlags=%#x, lpOverlapped=%p, lpCompletionRoutine=%#x)\n", 
          s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd,
          *lpFlags, lpOverlapped, lpCompletionRoutine);

    ret = WSARecvFrom( s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd,
                       lpFlags, NULL, 0, lpOverlapped, lpCompletionRoutine );

    LOGEXIT("WSARecv returns int %d\n", ret);
    return ret;
}


/*++
Function:
  WSARecvFrom

See MSDN doc.
--*/
int
PALAPI
WSARecvFrom(
        IN PAL_SOCKET s,
        IN OUT LPWSABUF lpBuffers,
        IN DWORD dwBufferCount,
        OUT LPDWORD lpNumberOfBytesRecvd,
        IN OUT LPDWORD lpFlags,
        OUT struct PAL_sockaddr FAR *lpFrom,
        IN OUT LPINT lpFromLen,
        IN LPWSAOVERLAPPED lpOverlapped,
        IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int    ret = SOCKET_ERROR;
    DWORD  dwLastError = 0;

    struct sockaddr UnixSockaddr;
    char sa_data[sizeof(UnixSockaddr.sa_data)];
    int sockaddr_len;


    ENTRY("WSARecvFrom (s=%#x, lpBuffers=%p, dwBufferCount=%u, "
          "lpNumberOfBytesRecvd=%p, lpFlags=%#x, lpFrom=%p, lpFromLen=%d, "
          "lpOverlapped=%p, lpCompletionRoutine=%#x)\n", 
          s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd,
          *lpFlags, lpFrom, lpFrom?*lpFromLen:0, lpOverlapped, 
          lpCompletionRoutine);

    if ( SOCK_startup_count == 0 )
    {
        ERROR("No successful WSAStartup call\n");
        dwLastError = WSANOTINITIALISED;
        goto done;
    }

    if ( lpCompletionRoutine )
    {
        ASSERT("lpCompletionRoutine is not NULL, as it should be\n");
        dwLastError = WSAEINVAL;
        goto done;
    }

    if ( IsValidSocket(s) == FALSE )
    {
        ERROR("Socket %d is not a valid socket\n", s);
        dwLastError = WSAENOTSOCK;
        goto done;
    }
   
    if (IsBadWritePtr(lpNumberOfBytesRecvd, sizeof(DWORD)))
    {
        ERROR("lpNumberOfBytesRecvd is not totally contained in valid "
              "user memory space\n");
        dwLastError = WSAEFAULT;
        goto done;
    }

    if (IsBadWritePtr(lpBuffers, sizeof(WSABUF)))
    {
        ERROR("lpBuffers is not totally contained in valid user "
              "memory space\n");
        dwLastError = WSAEFAULT;
        goto done;
    }
    
    if (lpBuffers[0].len == -1)
    {
        ERROR("Recv buffer length is invalid (-1)");
        dwLastError = WSAEINVAL;
        goto done;
    }
    
    if (IsBadWritePtr(lpBuffers[0].buf, lpBuffers[0].len))
    {
        ERROR("lpBuffers is not totally contained in valid "
              "user memory space\n");
        dwLastError = WSAEFAULT;
        goto done;
    }
    
    /* We need to be sure that the socket is already connect()ed.
       (for connection-oriented sockets only)
       This check needs to be done so that the correct error code
       is set. */
    if (SOCKIsSocketConnected(s) <= 0)
    {
        /* LastError set in SOCKIsSocketConnected */
        dwLastError = GetLastError();
        goto done;
    }

    /* MSG_OOB is not supported for UDP and FreeBSD ignores the flags
       and proceeds with the operation instead of reporting an unsupported
       operation
    */
    if (*lpFlags & PAL_MSG_OOB)
    {
        socklen_t sizeof_int  = sizeof(int);
        int       socket_type = 0;
    
        /*
           First, we need to get the socket type...
        */
        if ( (getsockopt(s, SOL_SOCKET, SO_TYPE, &socket_type, &sizeof_int )) < 0)
        {
            SetLastError(SOCKErrorReturn(errno));
            ERROR("getsockopt failed for socket %d\n", s);
            return(-1);
        }
       
        if ((socket_type == SOCK_DGRAM) || (socket_type == SOCK_RAW))
        {
            SetLastError(WSAEOPNOTSUPP);
            ERROR("Unsupported operation for non-stream socket\n");
            return(-1);
        }
    }

    /* ensure that this socket is bind()ed */
    memset(sa_data, 0, sizeof(UnixSockaddr.sa_data));

    sockaddr_len = sizeof UnixSockaddr;
    if ( getsockname(s, &UnixSockaddr, &sockaddr_len) < 0 )
    {
        ERROR("Failed to call getsockname()! error is %d (%s)\n", 
              errno, strerror(errno));
        dwLastError = WSAEINVAL;
        goto done;
    }
    
    /* If there is no data in sa_data, we aren't bind()ed yet -
       this is an error case in win32 */
    if (!memcmp(UnixSockaddr.sa_data, sa_data, sizeof(UnixSockaddr.sa_data)))
    {
        ERROR("Socket was not previously bound!\n");
        dwLastError = WSAEINVAL;
        goto done;
    }

    /* non-overlapped mode OR overlapped mode but 
       the overlapped recv queue is empty. 

       Here we access the queue without the queues critical section.
       There is no need to hold the critical section here since we are
       certain the worker thread won't add anything to the queue - all
       the operations are queued in the application threads (see 
       SOCKQueuePushBackOperation call below.

       There is a special case where two threads might be trying to 
       queue an operation simultaneously and that is effectively a race...
       but it's first and foremost an application design issue!
    */
    if ( (!lpOverlapped) ||
          (lpOverlapped && SOCKIsQueueEmpty(s, WS2_OP_RECVFROM) ) )
    {
        /* for overlapped, we need to initialize the WSAOVERLAPPED structure
           Internal fields */
        if (lpOverlapped)
        {
            struct pollfd fds;

            fds.fd = s;
            fds.events = POLLRDNORM;
            
            lpOverlapped->InternalHigh = 0;
            lpOverlapped->Internal     = 0;

            /* We are possibly dealing with blocking sockets.
               Because this is an overlapped socket (lpOverlapped != NULL),  
               we don't want to block but rather add the recv operation 
               to the queue if the operation cannot be carried right away. 
               To verify whether we can write on the socket without blocking,
               we use poll in POLLOUT mode
            */
again:
            if (-1 == poll(&fds, 1, 0))
            {
                if (errno == EINTR)
                {
                    TRACE("poll() failed with EINTR; re-polling\n");
                    goto again;
                }

                dwLastError = SOCKErrorReturn(errno);
                goto done;
            }
            else if (fds.revents & POLLERR)
            {
                ERROR("Exceptional condition has occured on the socket\n");
                dwLastError = WSAENOTCONN;
                goto done;
            }
            else if (fds.revents & POLLHUP)
            {
                ERROR("Socket was disconnected\n");
                dwLastError = WSAENOTCONN;
                goto done;
            }
            else if (fds.revents & POLLNVAL)
            {
                ERROR("Invalid socket\n");
                dwLastError = WSAENOTSOCK;
                goto done;
            }
            else if (!(fds.revents & POLLRDNORM))
            {
                goto overlapped;
            }
        }


        if (lpFrom)
        {
            /* recvfrom */
            ret = recvfrom((int)s, lpBuffers[0].buf, lpBuffers[0].len, 
                           (int) *lpFlags, &UnixSockaddr, lpFromLen);
            
            /*Copy BSD structure to Windows structure*/
            lpFrom->sa_family = UnixSockaddr.sa_family;
            memcpy(lpFrom->sa_data, UnixSockaddr.sa_data, sizeof(UnixSockaddr.sa_data));
        }
        else
        {
            /* recv */
            ret = recv((int)s,
                        lpBuffers[0].buf, lpBuffers[0].len,
                        (int) *lpFlags);
        }

        if (ret < 0)
        {
            /* we could not process this request in one pass, queue the
               remaining of that request */
            if (errno == EAGAIN && lpOverlapped)
            {
                TRACE("Recv returned EAGAIN, queuing recv operation\n");
                goto overlapped;
            }

            TRACE("Error happened and it wasn't EAGAIN!\n");
            *lpNumberOfBytesRecvd = 0;
            dwLastError = SOCKErrorReturn(errno);
        
            if (lpOverlapped)
            {
                lpOverlapped->Internal = dwLastError;

                if ( SetEvent(lpOverlapped->hEvent) == FALSE )
                {
                    ERROR("Could not signal event!\n");
                }
            }
        }
        else
        {
            TRACE("Recv operation completed succesfully. Bytes recvd %d\n", ret);
            if (lpOverlapped)
            {
                lpOverlapped->InternalHigh = ret;

                if ( SetEvent(lpOverlapped->hEvent) == FALSE )
                {
                    ERROR("Could not signal event!\n");
                }
            }
            *lpNumberOfBytesRecvd = ret;
            ret = 0;
        }
    }
    else
overlapped:
    {
        ws2_op_recvfrom *recvfrom_op = NULL;

        /* We should never attempt to overlap an operation if no peer is 
           associated with the stream socket */
        if ( SOCKIsSocketConnected(s) <= 0)
        {
            dwLastError = GetLastError();
            goto done;
        }

        recvfrom_op = malloc( sizeof(ws2_op_recvfrom) );
        if ( !recvfrom_op ) 
        {   
            ERROR("Couldn't allocate sendto operation");
            
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        /* 
        This memory is freed in SOCKQUEURemoveBack, called by 
        SOCKProcessOverlappedSend or SOCKDeleteAsyncSocket

        Also, there's ALWAYS only 1 WSABUF structure (see rotor_pal.doc) 
        */
        recvfrom_op->lpBuffers = (WSABUF*) malloc( sizeof(WSABUF) );
        if ( !recvfrom_op->lpBuffers)
        {
            ERROR("Couldn't allocate internal copy of WSABUF structure\n");
            
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            free(recvfrom_op);
            goto done;
        }

        /* initialize Internal and InternalHigh fields of
           WSAOVERLAPPED */
        lpOverlapped->Internal     = 0;
        lpOverlapped->InternalHigh = 0;
    
        /* NB: we don't need to keep dwBufferCount, it's always 1 */
        recvfrom_op->lpBuffers[0].buf     = lpBuffers[0].buf;
        recvfrom_op->lpBuffers[0].len     = lpBuffers[0].len;
        recvfrom_op->dwFlags              = *lpFlags;
        recvfrom_op->lpFrom               = lpFrom;
        recvfrom_op->lpFromLen            = lpFromLen;
        recvfrom_op->lpOverlapped         = lpOverlapped;

        SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

        if (FALSE == 
            SOCKQueuePushBackOperation((int)s, WS2_OP_RECVFROM,
                                        (LPVOID) recvfrom_op ))
        {
            ERROR("couldn't push back operation in overlapped recv queue\n");
           
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            free(recvfrom_op->lpBuffers);
            free(recvfrom_op);
            SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
            goto done;
        }
        
        dwLastError = SOCKSendRequestForAsyncOp(s, WS2_OP_RECVFROM, NULL);

        if (dwLastError)
        {
            ret = SOCKET_ERROR;
            if (FALSE == 
                SOCKQueueRemoveOperation((int)s, WS2_OP_RECVFROM, recvfrom_op))
            {
                ERROR("couldn't remove operation from queue (memory leak)\n");
            }
            SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
            goto done;
        }

        SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
        dwLastError = WSA_IO_PENDING;
        ret = SOCKET_ERROR;
    }

done:
    if (dwLastError) 
    {
        if (lpOverlapped && dwLastError != WSA_IO_PENDING)
        {
            lpOverlapped->Internal     = dwLastError;
            lpOverlapped->InternalHigh = 0;
            if ( SetEvent(lpOverlapped->hEvent) == FALSE )
            {
                ERROR("Could not signal event!\n");
            }
        }
        SetLastError(dwLastError); 
    }
    LOGEXIT("WSARecvFrom returns int %d\n", ret);
    return ret;
}


/*++
Function:
  WSAEventSelect

See MSDN doc.
--*/
int
PALAPI
WSAEventSelect(
           IN PAL_SOCKET s,
           IN WSAEVENT hEventObject,
           IN int lNetworkEvents)
{
    int ret = SOCKET_ERROR;
    DWORD dwLastError = 0;
    

    ENTRY("WSAEventSelect (s=%#x, hEventObject=%p, lNetworkEvent=%#x)\n",
          s, hEventObject, lNetworkEvents);

    if ( SOCK_startup_count == 0 )
    {
        ERROR("No successful WSAStartup call\n");
        SetLastError(WSANOTINITIALISED);
        goto done;
    }

    if(lNetworkEvents && (hEventObject == NULL))
    {
        ERROR("The event object is NULL\n");
        SetLastError(WSAEINVAL);
        goto done;
    }

    /* make the socket non-blocking */
    if ( fcntl( (int)s, F_SETFL, O_NONBLOCK ) == -1 )
    {
        ERROR("Failed to make file descriptor %d non-blocking\n", (int)s);
        if ( errno == EBADF )
        {
            ERROR("%d is a bad file descriptor\n", (int)s);
        }
        SetLastError( WSAENOTSOCK);
        goto done;
    }


    SYNCEnterCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
    
    if (FALSE == 
        SOCKAssignEventObjectToNetworkEvents(
            (int)s, hEventObject, lNetworkEvents))
    {
        SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);
        
        ERROR("Could not assign an event object for network events!\n");
        goto done;
    }

    TRACE("writing to file descriptor %d to wake up worker thread\n",
          SOCK_PIPE_WRITE);

    dwLastError = SOCKSendRequestForAsyncOp(s, WS2_OP_EVENTSELECT, NULL);

    SYNCLeaveCriticalSection(&ASYNCSOCKETS_crit_section, TRUE);

    if (dwLastError)
    {
        SetLastError(dwLastError);
        goto done;
    }

    ret = 0;

done:
    LOGEXIT("WSAEventSelect returns int %d\n", ret);
    return ret;
}

/*++
Function:
  WSAEnumNetworkEvents

See MSDN doc.
--*/
int
PALAPI
WSAEnumNetworkEvents(
        IN PAL_SOCKET s,
        IN WSAEVENT hEventObject,
        OUT LPWSANETWORKEVENTS lpNetworkEvents)
{
    DWORD dwLastError = 0;
    struct pollfd fds;
    int    *lpNetEventError;
    int nPollCount;

    ENTRY("WSAEnumNetworkEvents (s=%#x, hEventObject=%p,"
          "lpNetworkEvent=%p)\n",
          s, hEventObject, lpNetworkEvents);

    if ( SOCK_startup_count == 0 )
    {
        ERROR("No successful WSAStartup call\n");
        dwLastError = WSANOTINITIALISED;
        goto done;
    }

    if ( !IsValidSocket(s))
    {
        dwLastError = WSAENOTSOCK;
        goto done;
    }

    if ( IsBadWritePtr(lpNetworkEvents, sizeof(WSANETWORKEVENTS)))
    {
        ERROR("The network event structure is not in a valid part of the user"
              "address space\n");

        if(lpNetworkEvents == NULL)
        {
            dwLastError = WSAEINVAL;
        }
        else
        {
            dwLastError = WSAEFAULT;
        }
        goto done;
    }

    if ( hEventObject != NULL )
    {
        if (FALSE == ResetEvent(hEventObject))
        {
            WARN("Could not reset event %p\n", hEventObject);
        }
    }

    fds.fd      = s;
    fds.events  = POLLOUT;

again:
    nPollCount = poll(&fds, 1, 0);
    lpNetEventError = & (lpNetworkEvents->iErrorCode[FD_CONNECT_BIT]);

    TRACE("poll returned %d fds.revents %d\n", nPollCount, fds.revents);
    if (nPollCount < 0)
    {
        if ( EINTR == errno)
        {
            TRACE("poll() failed with EINTR; re-polling\n");
            goto again;
        }
        dwLastError = SOCKErrorReturn(errno);
        goto done;
    }

    memset(lpNetworkEvents, 0,
           sizeof(WSANETWORKEVENTS));

    if (fds.revents & POLLERR)
    {
        TRACE("Exceptional condition has occured on the socket\n");
        *lpNetEventError = WSAECONNREFUSED;
        lpNetworkEvents->lNetworkEvents |= FD_CONNECT;
        goto done;
    }
    else if (fds.revents & POLLHUP)
    {
        TRACE("Socket was disconnected\n");
        *lpNetEventError = WSAENETUNREACH;
        lpNetworkEvents->lNetworkEvents |= FD_CONNECT;
        goto done;
    }
    else if (fds.revents & POLLNVAL)
    {
        ERROR("Invalid socket. This should not happen since we"
               "checked the validity of the socket before polling\n");
        dwLastError = WSAENOTSOCK;
        goto done;
    }
    else if (fds.revents & POLLOUT)
    {
        if ( SOCKIsSocketConnected(s) > 0)
        {
            /* socket is connected */
            *lpNetEventError = 0;
        }
        else
        {
            /* socket is NOT connected */
            *lpNetEventError = WSAECONNREFUSED;
        }

        lpNetworkEvents->lNetworkEvents |= FD_CONNECT;
    }

done:
    if (dwLastError)
    {
        SetLastError(dwLastError);

        LOGEXIT("WSAEnumNetworkEvents returns int %d\n", SOCKET_ERROR);
        return SOCKET_ERROR;
    }

    LOGEXIT("WSAEnumNetworkEvents returns int 0\n");
    return 0;
}

/*++
Function:
  WSASocketA

See MSDN doc.
--*/
PAL_SOCKET
PALAPI
WSASocketA(
       IN int af,
       IN int type,
       IN int protocol,
       LPWSAPROTOCOL_INFOA lpProtocolInfo,
       IN GROUP g,
       IN DWORD dwFlags)
{
    PAL_SOCKET ret = INVALID_SOCKET;

    ENTRY("WSASocketA (af=%#x, type=%#x, protocol=%#x, lpProtocolInfo=%p, "
          "g=%#x, dwFlags=%#x)\n",
          af, type, protocol, lpProtocolInfo, g, dwFlags);

    if ( SOCK_startup_count == 0 )
    {
        ERROR("No successful WSAStartup call\n");
        SetLastError(WSANOTINITIALISED);
        goto done;
    }

    if ( dwFlags != WSA_FLAG_OVERLAPPED )
    {
        ASSERT("dwFlags is 0x%x, it should be 0x%x\n", dwFlags,
               WSA_FLAG_OVERLAPPED);
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if ( lpProtocolInfo )
    {
        ASSERT("lpProtocolInfo is not NULL as it should be\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    if ( g )
    {
        ASSERT("GROUP argument g should be 0\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    ret = PAL_socket(af, type, protocol);

done:
    LOGEXIT("WSASocketA returns SOCKET %d\n", ret);
    return ret;
}


/*++
Function:
  WSAIoctl

See MSDN documentation.

Note: This function accepts only the Unix ioctl codes (FIONBIO, FIONREAD,
and SIOCATMARK). Currently none of the winsock2 codes are exposed through
the BCL.
--*/
int
PALAPI
WSAIoctl(
     IN PAL_SOCKET s,
     IN DWORD dwIoControlCode,
     IN LPVOID lpvInBuffer,
     IN DWORD cbInBuffer,
     OUT LPVOID lpvOutBuffer,
     IN DWORD cbOutBuffer,
     OUT LPDWORD lpvbBytesReturned,
     IN LPWSAOVERLAPPED lpOverlapped,
     IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int ret = SOCKET_ERROR;
    BOOL bBehaveLikeNonOverlapped;
    
    ENTRY("WSAIoctl (s=%#x, dwIoControlCode=%#x, lpvInBuffer=%p, cbInBuffer=%u,"
          "lpvOutBuffer=%p, cbOutBuffer=%u, lpvbBytesReturned=%p, "
          "lpOverlapped=%p, lpCompletionRoutine=%#x)\n",
          s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, 
          lpvbBytesReturned, lpOverlapped, lpCompletionRoutine);

    if ( SOCK_startup_count == 0 )
    {
        ERROR("No successful WSAStartup call\n");
        SetLastError(WSANOTINITIALISED);
        goto done;
    }
   
    /* Notice that sanity tests are always done on every parameter,
       regardless of operation requested.
    */

    /* test input buffer */
    if (IsBadReadPtr(lpvInBuffer, cbInBuffer))
    {
        ERROR("lpvInBuffer is not totally contained in a valid part of the "
               "user address space.\n");
        SetLastError(WSAEFAULT);
        goto done;
    }
  
    /* test output buffer */
    if (IsBadWritePtr(lpvOutBuffer, cbOutBuffer))
    {
       ERROR("lpvOutBuffer is not totally contained in a valid part of the "
              "user address space.\n");
       SetLastError(WSAEFAULT);
       goto done;
    }
    
    /* test output bytes returned */
    if (IsBadWritePtr(lpvbBytesReturned, sizeof(DWORD))) 
    {
       ERROR("lpvbBytesReturned is not totally contained in a valid part "
              "of the user address space.\n");
       SetLastError(WSAEFAULT);
       goto done;
    }
    
    /* test if lpOverlapped argument is valid */
    if (lpOverlapped)
    {
        if (IsBadReadPtr(lpOverlapped, sizeof(WSAOVERLAPPED)))
        {
            ERROR("lpOverlapped parameter is not totally contained in a valid "
                   "part of the user address space\n");
            SetLastError(WSAEFAULT);
            goto done;
        }
    }
    
    /* behave like nonoverlapped when there no notification mechanism 
       available */
    if (!lpCompletionRoutine && 
        (!lpOverlapped || !lpOverlapped->hEvent))
    {
        bBehaveLikeNonOverlapped = TRUE;
    }
    else if (IsBadCodePtr((FARPROC) lpCompletionRoutine))
    {
        ERROR("lpCompletionRoutine parameter is not totally contained "
               "in a valid part of the user address space\n");
        SetLastError(WSAEINVAL);
        goto done;
    }
    else
    {
        bBehaveLikeNonOverlapped = FALSE;
    }
     
    /* test if IOC_VOID is set ( code has no parameters ) */ 
    if ( dwIoControlCode & PAL_IOC_VOID )
    {
        if (dwIoControlCode & PAL_IOC_IN)
        {
            ERROR("code can't be of type IOC_VOID and IOC_IN\n");
            SetLastError(WSAEINVAL);
            goto done;
        }
        
        if (dwIoControlCode & PAL_IOC_OUT )
        {
            ERROR("code can't be of type IOC_VOID and IOC_OUT\n");
            SetLastError(WSAEINVAL);
            goto done;
        }
    }


    /* Check to see if we are receiving a BSD ioctl request, if so,
       it can be handled by ioctlsocket() */
    if ( ((dwIoControlCode >> 27) & IOC_TYPE_MASK) == IOC_TYPE_UNIX)
    {
        /* make sure we pass the good buffer to ioctlsocket.

           FreeBSD ioctl() man page isn't clear whether an operation
           can be both in and out. However, the enumerated operations
           are not in and out. However, this could change in the future */
        if (dwIoControlCode & PAL_IOC_IN)
        {
            ret = ioctlsocket( (int)s, 
                               (unsigned long) dwIoControlCode, 
                               lpvInBuffer );
        }
        else if (dwIoControlCode & PAL_IOC_OUT)
        {
            ret = ioctlsocket( (int)s,
                               (unsigned long) dwIoControlCode,
                               lpvOutBuffer );
        }
        else
        {
            ERROR("UNIX ioctl socket operations of type IOC_VOID not "
                  "supported.\n");
            SetLastError(WSAEINVAL);
            goto done;
        }
    }
    /* If we get here, the IOCTL is of type IOC_WS2, IOC_PROTOCOL or
       IOC_VENDOR. These need to each be handled in a special way - and
       only a small subset is currently supported at all. */
    else if (dwIoControlCode == SIO_MULTIPOINT_LOOPBACK)
    {
        /* This is a request to set the loopback status. We will use 
           setsockopt(IP_MULTICAST_LOOPBACK) */
        if (cbInBuffer < sizeof(BOOL))
        {
            /* If we don't have a properly sized In buffer,
               it's an error! */
            ERROR ("Invalid sized cbInBuffer for SIO_MULTIPOINT_LOOPBACK\n");
            SetLastError(WSAEINVAL);
            goto done;
        }
            
        /* Implement SIO_MULTIPOINT_LOOPBACK using 
           setsockopt(IP_MULTICAST_LOOP) */
        ret = setsockopt (s, IPPROTO_IP, IP_MULTICAST_LOOP, 
                          (u_char*) lpvInBuffer, cbInBuffer);
        if (ret == SOCKET_ERROR)
        {
            /* Last error will be set by setsockopt() */
            ERROR("Error calling setsockopt for SIO_MULTIPOINT_LOOPBACK\n");
            goto done;
        }
    }
    else if(dwIoControlCode == SIO_MULTICAST_SCOPE)
    {
        /* This is a request to set the scope for a broadcast message. 
           This is the same as sock option IP_MULTICAST_TTL */

        if (cbInBuffer < sizeof(int))
        {
            /* If we don't have a valid In buffer or it is not properly 
               sized, it's an error! */
            ERROR ("Invalid or invalid sized lpvInBuffer for SIO_MULTICAST_SCOPE\n");
            SetLastError(WSAEINVAL);
            goto done;
        }
            
        /* Implement SIO_MULTICAST_SCOPE using 
           setsockopt(IP_MULTICAST_TTL) */
        ret = setsockopt (s, IPPROTO_IP, IP_MULTICAST_TTL, 
                          (u_char*) lpvInBuffer, cbInBuffer);
        if (ret == SOCKET_ERROR)
        {
            /* Last error will be set by setsockopt() */
            ERROR("Error calling setsockopt for SIO_MULTICAST_SCOPE errno=%d <%s> %d\n", errno, strerror(errno),
                  *((int*)lpvInBuffer));
            goto done;
        }
        *lpvbBytesReturned = cbInBuffer;
    }
    else if (dwIoControlCode == SIO_GET_BROADCAST_ADDRESS)
    {
        struct ifreq tmpIfReq;
        struct sockaddr *UnixSockaddr;

        if (!lpvOutBuffer || cbOutBuffer < sizeof(struct PAL_sockaddr))
        {
            /* If we don't have a valid Out buffer or it is not properly 
               sized, it's an error! */
            ERROR ("Invalid or invalid sized lpvOutBuffer for SIO_GET_BROADCAST_ADDRESS\n");
            SetLastError(WSAEINVAL);
            goto done;
        }

        ret = ioctl(s, SIOCGIFBRDADDR, &tmpIfReq, sizeof(tmpIfReq));
        if (ret == SOCKET_ERROR)
        {
            /* Last error will be set by setsockopt() */
            ERROR("Error calling ioctl for SIO_BROADCAST_ADDRESS\n");
            goto done;
        }

        /* Convert from the Unix sockaddr type returned to a PAL sockaddr */
        UnixSockaddr = &tmpIfReq.ifr_broadaddr;
        ((struct PAL_sockaddr*)lpvOutBuffer)->sa_family = UnixSockaddr->sa_family;
        memcpy(((struct PAL_sockaddr*)lpvOutBuffer)->sa_data, 
               UnixSockaddr->sa_data, sizeof(((struct PAL_sockaddr*)lpvOutBuffer)->sa_data));

    }
    else if (dwIoControlCode == SIO_KEEPALIVE_VALS)
    {
        /* NOTE: This implementation of SIO_KEEPALIVE_VALS is only approximate -
           it only sets Keepalive for a socket on or off, it doesn't modify
           the keepalivetime or keepaliveinterval */
        BOOL KeepAliveStatus;
        int BoolSize;
        BoolSize = sizeof(BOOL);

        if (cbInBuffer < sizeof(struct tcp_keepalive))
        {
            /* If we don't have a properly sized In buffer,
               it's an error! */
            ERROR ("Invalid sized cbInBuffer for SO_KEEPALIVE\n");
            SetLastError(WSAEINVAL);
            goto done;
        }
            
        /* Implement SIO_KEEPALIVE_VALS using 
           setsockopt(SO_KEEPALIVE) */
        KeepAliveStatus = ((struct tcp_keepalive*)lpvInBuffer)->onoff;   
        ret = setsockopt (s, SOL_SOCKET, SO_KEEPALIVE, 
                          &KeepAliveStatus, BoolSize);
        if (ret == SOCKET_ERROR)
        {
            /* Last error will be set by setsockopt() */
            ERROR("Error calling setsockopt for SO_KEEPALIVE\n");
            goto done;
        }
    }
    else if (dwIoControlCode == SIO_FLUSH)
    {
        if (FALSE == SOCKFlushOverlappedOperationQueues(s, TRUE)) 
        {
           ret = SOCKET_ERROR;
           ERROR("Error flushing pending overlapped operations on socket\n");
           goto done;
        }
    }
    else
    {
        /* This is a request for an IOCTL that we don't support - return
           an error */
        ERROR("dwIoControlCode of %#x is not supported.\n", dwIoControlCode);
        SetLastError(WSAEINVAL);
        goto done;
    }
    
    
done:
    LOGEXIT("WSAIoctl returns int %d\n", ret);
    return ret;
}


/*++
Function:
  ioctlsocket

See MSDN documentation.
--*/
int
PALAPI
ioctlsocket(
        IN PAL_SOCKET s,
        IN int cmd,
        IN OUT PAL_u_long FAR *argp)
{
    int ret = SOCKET_ERROR;

    ENTRY("ioctlsocket (s=%#x, cmd=%#x, argp=%p)\n",
          s, cmd, argp);

    if ( SOCK_startup_count == 0 )
    {
        ERROR("No successful WSAStartup call\n");
        SetLastError(WSANOTINITIALISED);
        goto done;
    }

    switch (cmd)
    {
        case PAL_FIONREAD:
            cmd = FIONREAD;
            break;
        case PAL_FIONBIO:
            cmd = FIONBIO;
            break;
        default:
            ASSERT("cmd %d invalid\n", cmd);
            SetLastError(WSAEFAULT);
            goto done;
            break;
    }

    /*check if the argp param is a valid part
      of user address space*/
    if (IsBadWritePtr(argp,sizeof(PAL_u_long)))
    {
        ERROR("arg pointer is not totally contained in a valid"
               "part of the user address space.\n");
        SetLastError(WSAEFAULT);
        goto done;
    }

    /* MSDN docs don't say that ioctlsocket will SetLastError the
       same as WSAIoctl, but tests show that it will */
    ret = ioctl( s, (DWORD)cmd, (LPVOID)argp);
    if ( ret == -1 )
    {
        ERROR("ioctl failed errno %d\n", errno);
        ret = SOCKET_ERROR;

        switch(errno)
        {
        case EBADF:
            SetLastError(WSAENOTSOCK);
            break;
        case EFAULT:
            SetLastError(WSAEFAULT);
            break;
        case EINVAL:
            /* fall through */
        default:
            SetLastError(WSAEINVAL);
            break;
        }
    }

done:
    LOGEXIT("ioctlsocket returns int %d\n", ret);
    return ret;
}


/*++
Function:
  SOCKInitWinSock

Initialization function called from PAL_Initialize.
Initializes the global critical section and allocates a TLS index.
--*/
BOOL
SOCKInitWinSock(void)
{
    int ret;

    TRACE("Initializing WinSock2...\n");
    if (0 != SYNCInitializeCriticalSection( &SOCK_crit_section))
    {
        ERROR("sockets critical section initialization failed\n");
        return FALSE;
    }
    
    if (0 != SYNCInitializeCriticalSection(&ASYNCSOCKETS_crit_section))
    {
        ERROR("async sockets queues critical section initialization failed\n");
        return FALSE;
    }
    

    /* allocate the TLS index for hostent structures */
    if ((ret = pthread_key_create(&SOCK_tlsHostentKey, NULL)) != 0)
    {
       ERROR("pthread_key_create() failed error:%d (%s)\n",
              ret, strerror(ret));
       return FALSE;
    }

    return TRUE;
}


/*++
Function:
  SOCKTerminateWinSock

Termination function called from PAL_Terminate to delete the global
critical section.
--*/
void SOCKTerminateWinSock(void)
{
    int ret;

    TRACE("Terminating WinSock2...\n");
    SYNCEnterCriticalSection( &SOCK_crit_section , TRUE);
    if ( SOCK_startup_count > 0 )
    {
        WARN("Not enough calls to WSACleanup\n");
        SOCKCommonCleanup();
    }

    /* free the hostent structure associated with the main thread */
    SOCKFreeTlsHostent();

    /* free the index in TLS for hostent structures */
    if((ret = pthread_key_delete(SOCK_tlsHostentKey)) != 0)
    {
        ERROR("pthread_key_delete() failed errno:%d (%s)\n",
               ret, strerror(ret));
    }

    SYNCLeaveCriticalSection( &SOCK_crit_section , TRUE);
    DeleteCriticalSection( &SOCK_crit_section );
}



/*++
Function:
  SOCKCommonCleanup

Utility function to cleanup worker thread, etc., when the startup count
reaches 0. Called from WSACleanup and SOCKTerminateWinSock. The calling
function should lock the global critical section before calling this
function and unlock it afterwards.
--*/
static void SOCKCommonCleanup()
{
    int i;
    BOOL trouble = FALSE;
    
    SOCKStopWorkerThread();
    
    if ( CloseHandle( SOCK_worker_thread_handle ) == FALSE )
    {
        WARN("Unable to reclaim worker thread handle\n");
        trouble = TRUE;
        SOCK_worker_thread_handle = INVALID_HANDLE_VALUE;
    }
    
    /* close the pipe */
    for( i = 0; i <= 1; ++i )
    {
        if ( close(SOCK_pipe_fd[i]) != 0 )
        {
            WARN("Could not close file descriptor %d\n", SOCK_pipe_fd[i]);
            trouble = TRUE;
        }
        SOCK_pipe_fd[i] = -1;
    }

    if ( trouble )
    {
        WARN("Socket cleanup was messy\n");
    }
}


/*++
Function:
  SOCKCancelAsyncOperations

Send a message down the pipe to cancel all asynchronous operations on
the given socket. Return value is the same as closesocket -- 0 if
successful, SOCKET_ERROR otherwise.
--*/
int SOCKCancelAsyncOperations( PAL_SOCKET s )
{
    int ret = 0;
    HANDLE hEvent = NULL;
    
    /* allocate an event which will be signalled when all operations
       have been cancelled */
    hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( hEvent == NULL )
    {
        ERROR("CreateEvent failed, unable to cancel any overlapped or "
              "asynchronous operations on socket %d\n", s);
        ret = SOCKET_ERROR;
        goto done;
    }
    
    if (SOCKSendRequestForAsyncOp(s, WS2_OP_CLOSESOCKET, hEvent))
    {
        ret = SOCKET_ERROR;
        goto done;
    }

    if ( WaitForSingleObject( hEvent, 10000 ) != WAIT_OBJECT_0 )
    {
        ASSERT("Event was not signalled, unable to cancel any overlapped "
               "or asynchronous operations on socket %d\n", s);
        ret = SOCKET_ERROR;
    }

done:
    
    if ( hEvent && CloseHandle(hEvent) == FALSE )
    {
        WARN("Could not free event\n");
    }

    return ret;
}

/*++
Function:
  SOCKStopWorkerThread

Send a message down the pipe to tell the worker thread to terminate.

(no parameters, no return value)
--*/
void SOCKStopWorkerThread( void )
{
    if (SOCKSendRequestForAsyncOp(0, WS2_OP_STOPTHREAD, NULL))
    {
        WARN("Could not stop worker thread\n");
    }
    else 
    {
        /* write successful; now wait for thread to terminate itself. */
        if ( WAIT_OBJECT_0 !=
              WaitForSingleObject( SOCK_worker_thread_handle, INFINITE ) )
        {
            ASSERT("Failed to wait on worker thread's handle (%p)\n",
                   SOCK_worker_thread_handle);
        }
    }
    
    /* SOCK_worker_thread_handle is closed in SOCKCommonCleanup */
}


/*++
Function:
  IsValidSocket

Return true if the given socket is valid, false if not.
--*/
static BOOL IsValidSocket( PAL_SOCKET s )
{
    fd_set test_set;
    int    res;
    struct timeval time = { 0, 0 };

    if (INVALID_SOCKET == s)
    {
        return FALSE;
    }

    FD_ZERO(&test_set);
    if (FD_SET_BOOL((int)s, &test_set) == FALSE)
    {
        ERROR("Invalid socket value\n");
        return FALSE;
    }

again:
    res = select( (int)s + 1, NULL, &test_set, NULL, &time );
    if ( res == -1 )
    {
        if (errno == EINTR)
        {
            TRACE("select() failed with EINTR; re-selecting\n");
            goto again;
        }
        else if ( errno == EBADF )
        {
            ERROR("Socket %d is an invalid file descriptor\n", s);
        }
        else
        {
            ASSERT("Unknown error from select(errno=%d)\n", errno);
        }
        return FALSE;
    }
    return TRUE;
}

/*++
Function:
  SOCKAllocTlsHostent

Allocate memory for the calling thread's hostent structure, and put a
pointer to it in thread-local storage. The TLS index is allocated in
SOCKInitWinSock and freed in SOCKTerminateWinSock.
--*/
BOOL SOCKAllocTlsHostent( void )
{
    struct PAL_hostent *phostent;
    int ret;

    phostent = (struct PAL_hostent*)malloc(sizeof(struct PAL_hostent));
    if ( !phostent )
    {
        ERROR("malloc failed\n");
        return FALSE;
    }
    memset(phostent, 0, sizeof(struct PAL_hostent));

    if ((ret = pthread_setspecific(SOCK_tlsHostentKey, phostent)) != 0)
    {
        ERROR("pthread_setspecific() failed error:%d (%s)\n",
               ret, strerror(ret));
    }
    return TRUE;
}

/*++
Function:
  SOCKFreeTlsHostent

Free the memory for the calling thread's hostent structure.
--*/
void SOCKFreeTlsHostent( void )
{
    struct PAL_hostent* ret;
    
    if (NULL != (ret = pthread_getspecific(SOCK_tlsHostentKey)))
    {
        SOCKFreePALHostentFields(ret);
        free(ret);
    }
}

/*++
Function:
  SOCKFreePALHostentFields

Free the fields of a PAL_hostent.  This does not free the hostent itself.
--*/
void SOCKFreePALHostentFields(struct PAL_hostent *hostent)
{
    int i;
    
    free(hostent->h_name);
    hostent->h_name = NULL;
    if (hostent->h_aliases != NULL)
    {
        for(i = 0; hostent->h_aliases[i] != NULL; i++)
        {
            free(hostent->h_aliases[i]);
        }
        free(hostent->h_aliases);
    }
    hostent->h_aliases = NULL;
    if (hostent->h_addr_list != NULL)
    {
        for(i = 0; hostent->h_addr_list[i] != NULL; i++)
        {
            free(hostent->h_addr_list[i]);
        }
        free(hostent->h_addr_list);
    }
    hostent->h_addr_list = NULL;
}

/*++
Function:
  SOCKIsSocketConnected

Detects if socket s is actually connect()ed or not,
by using an 'innocent' function.

1  = connect()ed or N/A
0  = Not
-1 = Error
--*/
int SOCKIsSocketConnected( PAL_SOCKET s )
{
    struct sockaddr sock;
#if HAVE_SOCKLEN_T
    socklen_t sockaddr_size;
#else   /* HAVE_SOCKLEN_T */
    int sockaddr_size;
#endif  /* HAVE_SOCKLEN_T */
    int socket_type;
    int sizeof_int;
    
    sockaddr_size = sizeof(struct sockaddr);
    sizeof_int = sizeof(int);

    /*
       First, we need to get the socket type...
    */
    if ( (getsockopt(s, SOL_SOCKET, SO_TYPE, &socket_type, &sizeof_int )) < 0)
    {
        SetLastError(SOCKErrorReturn(errno));
        ERROR("getsockopt failed for socket %d\n", s);
        return(-1);
    }
    
    /* We only need to check if the socket is connected first if
       we AREN'T either SOCK_RAW or SOCK_DGRAM 
       
       NOTE: This list could change
    */
    if (socket_type != SOCK_RAW && socket_type != SOCK_DGRAM)
    {
        /* Finally, our innocent function. This one should work ONLY
           if we are connected, otherwise error. */
        if ( (getpeername(s, &sock, &sockaddr_size)) < 0)
        {
            SetLastError(SOCKErrorReturn(errno));
            TRACE("Socket %d is not connect()ed\n", s);
            return(0);
        }
    }
    
    
    /* Either we are connect()ed, or are an inappropriate socket type.
       Return 1. (for connected / N/A) */
    return (1); 
}



/*++
Function:
  SOCKSendRequestForAsyncOp

Send a request to the worker thread.


Return value:
    if error: the "LastError" is returned
    otherwise: 0 is returned.

--*/
DWORD SOCKSendRequestForAsyncOp( PAL_SOCKET s, enum ws2_opcode opcode, 
                                 HANDLE hEvent)
{
    ws2_op sock_op;

    DWORD dwLastError = 0;

    sock_op.opcode = opcode;
    sock_op.s      = s;
    sock_op.event  = hEvent;
    
    /* write the request to the pipe */
    if ( write(SOCK_PIPE_WRITE, &sock_op, sizeof(sock_op)) != 
         sizeof(sock_op) )
    {
        ERROR("failed to write %d bytes to pipe to wake up worker "
              "thread\n", sizeof(sock_op));
        dwLastError = ERROR_BROKEN_PIPE;
        goto done;
    }

done:
    return dwLastError;
}
