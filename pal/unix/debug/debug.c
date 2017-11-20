/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    debug.c

Abstract:

    Implementation of Win32 debugging API functions.

Revision History:

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include "../thread/process.h"
#include "pal/context.h"
#include "pal/debug.h"
#include "pal/misc.h"

#include <signal.h>
#include <unistd.h>
#if HAVE_PROCFS_CTL
#include <fcntl.h>
#include <unistd.h>
#else   // HAVE_PROCFS_CTL
#include <sys/ptrace.h>
#endif  // HAVE_PROCFS_CTL
#if HAVE_VM_READ
#include <mach/mach.h>
#endif  // HAVE_VM_READ
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

SET_DEFAULT_DEBUG_CHANNEL(DEBUG);

#if HAVE_PROCFS_CTL
#define CTL_ATTACH      "attach"
#define CTL_DETACH      "detach"
#define CTL_WAIT        "wait"
#else   // HAVE_PROCFS_CTL

#ifdef PTRACE_PEEKDATA
#define PT_READ_D   PTRACE_PEEKDATA
#define PT_WRITE_D  PTRACE_POKEDATA
#define PT_ATTACH   PTRACE_ATTACH
#define PT_DETACH   PTRACE_DETACH
#endif  // !defined(PTRACE_PEEKDATA)

#if HAVE_CADDR_T
#define PT_ADDR(x)  ((caddr_t) (x))
#else   // HAVE_CADDR_T
#define PT_ADDR(x)  ((int) (x))
#endif  // HAVE_CADDR_T

#endif  // HAVE_PROCFS_CTL

/* ------------------- Constant definitions ----------------------------------*/

const BOOL DBG_ATTACH       = TRUE;
const BOOL DBG_DETACH       = FALSE;
static const char PAL_OUTPUTDEBUGSTRING[]= "PAL_OUTPUTDEBUGSTRING";

/* ------------------- Static function prototypes ----------------------------*/

#if !HAVE_VM_READ && !HAVE_PROCFS_CTL
static int
DBGWriteProcMem_Int(DWORD processId, int *addr, int data);
static int
DBGWriteProcMem_IntWithMask(DWORD processId, int *addr, int data,
                            unsigned int mask);
#endif  // !HAVE_VM_READ && !HAVE_PROCFS_CTL
static int
DBGSetProcessAttached(DWORD attachedProcId, DWORD attacherProcId, BOOL bAttach);

/*++
Function:
  FlushInstructionCache

The FlushInstructionCache function flushes the instruction cache for
the specified process.

Remarks

This is a no-op for x86 architectures where the instruction and data
caches are coherent in hardware. For non-X86 architectures, this call
usually maps to a kernel API to flush the D-caches on all processors.

--*/
BOOL
PALAPI
FlushInstructionCache(
        IN HANDLE hProcess,
        IN LPCVOID lpBaseAddress,
        IN SIZE_T dwSize)
{
    BOOL Ret;

    ENTRY("FlushInstructionCache (hProcess=%p, lpBaseAddress=%p dwSize=%d)\
          \n", hProcess, lpBaseAddress, dwSize);

    Ret = DBG_FlushInstructionCache(lpBaseAddress, dwSize);

    LOGEXIT("FlushInstructionCache returns BOOL %d\n", Ret);
    return Ret;
}


/*++
Function:
  OutputDebugStringA

See MSDN doc.
--*/
VOID
PALAPI
OutputDebugStringA(
        IN LPCSTR lpOutputString)
{
    char *env_string;

    ENTRY("OutputDebugStringA (lpOutputString=%p (%s))\n",
          lpOutputString?lpOutputString:"NULL",
          lpOutputString?lpOutputString:"NULL");
    
    /* as we don't support debug events, we are going to output the debug string
      to stderr instead of generating OUT_DEBUG_STRING_EVENT */
    if ( (lpOutputString != NULL) && 
         (NULL != (env_string = MiscGetenv(PAL_OUTPUTDEBUGSTRING))))
    {
        fprintf(stderr, "%s", lpOutputString);
    }
        
    LOGEXIT("OutputDebugStringA returns\n");
}

/*++
Function:
  OutputDebugStringW

See MSDN doc.
--*/
VOID
PALAPI
OutputDebugStringW(
        IN LPCWSTR lpOutputString)
{
    CHAR *lpOutputStringA;
    int strLen;

    ENTRY("OutputDebugStringW (lpOutputString=%p (%S))\n",
          lpOutputString ? lpOutputString: W16_NULLSTRING,
          lpOutputString ? lpOutputString: W16_NULLSTRING);
    
    if (lpOutputString == NULL) 
    {
        OutputDebugStringA("");
        goto EXIT;
    }

    if ((strLen = WideCharToMultiByte(CP_ACP, 0, lpOutputString, -1, NULL, 0, 
                                      NULL, NULL)) 
        == 0)
    {
        ASSERT("failed to get wide chars length\n");
        SetLastError(ERROR_INTERNAL_ERROR);
        goto EXIT;
    }
    
    /* strLen includes the null terminator */
    if ((lpOutputStringA = (LPSTR) malloc(strLen * sizeof(CHAR))) == NULL)
    {
        ERROR("Insufficient memory available !\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto EXIT;
    }
    
    if(! WideCharToMultiByte(CP_ACP, 0, lpOutputString, -1, 
                             lpOutputStringA, strLen, NULL, NULL)) 
    {
        ASSERT("failed to convert wide chars to multibytes\n");
        SetLastError(ERROR_INTERNAL_ERROR);
        free(lpOutputStringA);
        goto EXIT;
    }
    
    OutputDebugStringA(lpOutputStringA);
    free(lpOutputStringA);

EXIT:    
    LOGEXIT("OutputDebugStringW returns\n");
}

/*++
Function:
  DebugBreak

See MSDN doc.
--*/
VOID
PALAPI
DebugBreak(
       VOID)
{
    ENTRY("DebugBreak()\n");
    
    DBG_DebugBreak();
    
    LOGEXIT("DebugBreak returns\n");
}

/*++
Function:
  GetThreadContext

See MSDN doc.
--*/
BOOL
PALAPI
GetThreadContext(
           IN HANDLE hThread,
           IN OUT LPCONTEXT lpContext)
{    
    BOOL ret;
    ENTRY("GetThreadContext (hThread=%p, lpContext=%p)\n",hThread,lpContext);

    if(hThread == hPseudoCurrentThread)
    {
        hThread = PROCGetRealCurrentThread();
    } 
    
    ret = CONTEXT_GetThreadContext(hThread, lpContext);
    
    LOGEXIT("GetThreadContext returns ret:%d\n", ret);
    return ret;
}

/*++
Function:
  SetThreadContext

See MSDN doc.
--*/
BOOL
PALAPI
SetThreadContext(
           IN HANDLE hThread,
           IN CONST CONTEXT *lpContext)
{
    BOOL ret;
    ENTRY("SetThreadContext (hThread=%p, lpContext=%p)\n",hThread,lpContext);

    if(hThread == hPseudoCurrentThread)
    {
        hThread = PROCGetRealCurrentThread();
    } 
    
    ret = CONTEXT_SetThreadContext(hThread, lpContext);
    
    LOGEXIT("SetThreadContext returns ret:%d\n", ret);
    return ret;
}

/*++
Function:
  ReadProcessMemory

See MSDN doc.
--*/
BOOL
PALAPI
ReadProcessMemory(
           IN HANDLE hProcess,
           IN LPCVOID lpBaseAddress,
           IN LPVOID lpBuffer,
           IN DWORD  nSize,
           OUT LPDWORD lpNumberOfBytesRead
           )
{
    DWORD processId;
    volatile BOOL ret = FALSE;
    volatile DWORD numberOfBytesRead = 0;
#if HAVE_VM_READ
    kern_return_t result;
    vm_map_t task;
    int bytesToRead;
#elif HAVE_PROCFS_CTL
    int fd;
    char memPath[64];
#else
    unsigned nbInts;
    int* ptrInt;
    int* lpTmpBuffer;
#endif
#if !HAVE_PROCFS_CTL
    LPVOID lpBaseAddressAligned;
    unsigned int offset;
#endif  // !HAVE_PROCFS_CTL

    ENTRY("ReadProcessMemory (hProcess=%p,lpBaseAddress=%p, lpBuffer=%p, "
          "nSize=%u, lpNumberOfBytesRead=%p)\n",hProcess,lpBaseAddress,
          lpBuffer, nSize, lpNumberOfBytesRead);
    
    if (!(processId = PROCGetProcessIDFromHandle(hProcess)))
    {
        ERROR("Invalid process handler hProcess:%p.",hProcess);
        SetLastError(ERROR_INVALID_HANDLE);        
        goto EXIT;
    }
    
    // Check if the read request is for the current process. 
    // We don't need ptrace in that case.
    if (GetCurrentProcessId() == processId) 
    {
        TRACE("We are in the same process, so ptrace is not needed\n");
        
        PAL_TRY {
            memcpy(lpBuffer, lpBaseAddress, nSize);
            numberOfBytesRead = nSize;
            ret = TRUE;
        } PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            SetLastError(ERROR_ACCESS_DENIED);
        } PAL_ENDTRY
        goto EXIT;        
    }

#if HAVE_VM_READ
    result = task_for_pid(mach_task_self(), processId, &task);
    if (result != KERN_SUCCESS)
    {
        ERROR("No Mach task for pid %d: %d\n", processId, ret);
        SetLastError(ERROR_INVALID_HANDLE);
        goto EXIT;
    }
    // vm_read_overwrite usually requires that the address be page-aligned
    // and the size be a multiple of the page size.  We can't differentiate
    // between the cases in which that's required and those in which it
    // isn't, so we do it all the time.
    lpBaseAddressAligned = (LPVOID) ((DWORD) lpBaseAddress & ~PAGE_MASK);
    offset = ((DWORD) lpBaseAddress & PAGE_MASK);
    while (nSize > 0)
    {
        vm_size_t bytesRead;
        char data[PAGE_SIZE];
        
        bytesToRead = PAGE_SIZE - offset;
        if (bytesToRead > nSize)
        {
            bytesToRead = nSize;
        }
        bytesRead = PAGE_SIZE;
        result = vm_read_overwrite(task, (vm_address_t) lpBaseAddressAligned,
                                   PAGE_SIZE, (vm_address_t) data, &bytesRead);
        if (result != KERN_SUCCESS || bytesRead != PAGE_SIZE)
        {
            ERROR("vm_read_overwrite failed for %d bytes from %p in %d: %d\n",
                  PAGE_SIZE, (char *) lpBaseAddressAligned, task, result);
            if (result <= KERN_RETURN_MAX)
            {
                SetLastError(ERROR_INVALID_ACCESS);
            }
            else
            {
                SetLastError(ERROR_INTERNAL_ERROR);
            }
            goto EXIT;
        }
        memcpy(lpBuffer + numberOfBytesRead, data + offset, bytesToRead);
        numberOfBytesRead += bytesToRead;
        lpBaseAddressAligned = (char *) lpBaseAddressAligned + PAGE_SIZE;
        nSize -= bytesToRead;
        offset = 0;
    }
    ret = TRUE;
#else   // HAVE_VM_READ
    // Attach the process before calling ptrace otherwise it fails.
    if (DBGAttachProcess(processId))
    {
#if HAVE_PROCFS_CTL
        snprintf(memPath, sizeof(memPath), "/proc/%u/mem", processId);
        fd = open(memPath, O_RDONLY);
        if (fd == -1)
        {
            ERROR("Failed to open %s\n", memPath);
            SetLastError(ERROR_INVALID_ACCESS);
            goto CLEANUP2;
        }

        if (lseek(fd, (DWORD) lpBaseAddress, SEEK_SET) == -1)
        {
            ERROR("Failed to seek to base address\n");
            SetLastError(ERROR_INVALID_ACCESS);
            goto CLEANUP2;
        }
        
        numberOfBytesRead = read(fd, lpBuffer, nSize);

#else   // HAVE_PROCFS_CTL
        offset = (unsigned int)lpBaseAddress % sizeof(int);
        lpBaseAddressAligned =  (LPVOID) ((char*)lpBaseAddress - offset);    
        nbInts = (nSize + offset)/sizeof(int) + 
                 ((nSize + offset)%sizeof(int) ? 1:0);
        
        /* before transferring any data to lpBuffer we should make sure that all 
           data is accessible for read. so we need to use a temp buffer for that.*/
        if (!(lpTmpBuffer = malloc(nbInts * sizeof(int))))
        {
            ERROR("Insufficient memory available !\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto CLEANUP1;
        }
        
        for (ptrInt = lpTmpBuffer; nbInts; ptrInt++,
             ((int*)lpBaseAddressAligned)++, nbInts--)
        {
            errno = 0;
            *ptrInt = ptrace(PT_READ_D, processId,
                             PT_ADDR(lpBaseAddressAligned), 0);
            if (*ptrInt == -1 && errno) 
            {
                if (errno == EFAULT) 
                {
                    ERROR("ptrace(PT_READ_D, pid:%d, addr:%p, data:0) failed"
                          " errno=%d (%s)\n", processId, lpBaseAddressAligned,
                          errno, strerror(errno));
                    
                    SetLastError(ptrInt == lpTmpBuffer ? ERROR_ACCESS_DENIED : 
                                                         ERROR_PARTIAL_COPY);
                }
                else
                {
                    ASSERT("ptrace(PT_READ_D, pid:%d, addr:%p, data:0) failed"
                          " errno=%d (%s)\n", processId, lpBaseAddressAligned,
                          errno, strerror(errno));
                    SetLastError(ERROR_INTERNAL_ERROR);
                }
                
                goto CLEANUP2;
            }
        }
        
        /* transfer data from temp buffer to lpBuffer */
        memcpy( (char *)lpBuffer, ((char*)lpTmpBuffer) + offset, nSize);
        numberOfBytesRead = nSize;
#endif  // HAVE_PROCFS_CTL
        ret = TRUE;
    }
    else
    {
        /* Failed to attach processId */
        goto EXIT;    
    }

CLEANUP2:
#if HAVE_PROCFS_CTL
    if (fd != -1)
    {
        close(fd);
    }
#else   // HAVE_PROCFS_CTL
    if (lpTmpBuffer) 
    {
        free(lpTmpBuffer);
    }
#endif  // HAVE_PROCFS_CTL

#if !HAVE_PROCFS_CTL
CLEANUP1:
#endif  // HAVE_PROCFS_CTL
    if (!DBGDetachProcess(processId))
    {
        /* Failed to detach processId */
        ret = FALSE;
    }
#endif  // HAVE_VM_READ
EXIT:
    if (lpNumberOfBytesRead)
    {
        *lpNumberOfBytesRead = numberOfBytesRead;
    }
    LOGEXIT("ReadProcessMemory returns BOOL %d\n", ret);
    return ret;
}

/*++
Function:
  WriteProcessMemory

See MSDN doc.
--*/
BOOL
PALAPI
WriteProcessMemory(
           IN HANDLE hProcess,
           IN LPVOID lpBaseAddress,
           IN LPVOID lpBuffer,
           IN DWORD  nSize,
           OUT LPDWORD lpNumberOfBytesWritten
           )

{
    DWORD processId;
    volatile BOOL ret = FALSE;
#if HAVE_VM_READ
    kern_return_t result;
    vm_map_t task;
#elif HAVE_PROCFS_CTL
    int fd;
    char memPath[64];
    DWORD bytesWritten;
#else
    unsigned int FirstIntOffset;
    unsigned int LastIntOffset;
    unsigned int FirstIntMask;
    unsigned int LastIntMask;
    unsigned int nbInts;
    int *lpTmpBuffer = 0, *lpInt;
    LPVOID lpBaseAddressAligned;
#endif

    ENTRY("WriteProcessMemory (hProcess=%p,lpBaseAddress=%p, lpBuffer=%p, "
           "nSize=%u, lpNumberOfBytesWritten=%p)\n",
           hProcess,lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);    
    
    if (!(nSize && (processId = PROCGetProcessIDFromHandle(hProcess))))
    {
        ERROR("Invalid nSize:%u number or invalid process handler "
              "hProcess:%p\n", nSize, hProcess);
        SetLastError(ERROR_INVALID_PARAMETER);
        goto EXIT;
    }
    
    if (lpNumberOfBytesWritten)
    {
        *lpNumberOfBytesWritten = 0;
    }
    
    // Check if the write request is for the current process.
    // In that case we don't need ptrace.
    if (GetCurrentProcessId() == processId) 
    {
        TRACE("We are in the same process so we don't need ptrace\n");
        
	    PAL_TRY {        
            memcpy(lpBaseAddress, lpBuffer, nSize);
        
            if (lpNumberOfBytesWritten)
            {
                *lpNumberOfBytesWritten = nSize;
            }
            ret = TRUE;
        } PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            SetLastError(ERROR_ACCESS_DENIED);
        } PAL_ENDTRY
        goto EXIT;        
    }

#if HAVE_VM_READ
    result = task_for_pid(mach_task_self(), processId, &task);
    if (result != KERN_SUCCESS)
    {
        ERROR("No Mach task for pid %d: %d\n", processId, ret);
        SetLastError(ERROR_INVALID_HANDLE);
        goto EXIT;
    }
    result = vm_write(task, (vm_address_t) lpBaseAddress, 
                      (vm_address_t) lpBuffer, nSize);
    if (result != KERN_SUCCESS)
    {
        ERROR("vm_write failed for %d bytes from %p in %d: %d\n",
              nSize, lpBaseAddress, task, result);
        if (result <= KERN_RETURN_MAX)
        {
            SetLastError(ERROR_ACCESS_DENIED);
        }
        else
        {
            SetLastError(ERROR_INTERNAL_ERROR);
        }
        goto EXIT;
    }
    if (lpNumberOfBytesWritten)
    {
        *lpNumberOfBytesWritten = nSize;
    }
    ret = TRUE;
#else   // HAVE_VM_READ
    /* Attach the process before calling ptrace otherwise it fails */
    if (DBGAttachProcess(processId))
    {
#if HAVE_PROCFS_CTL
        snprintf(memPath, sizeof(memPath), "/proc/%u/mem", processId);
        fd = open(memPath, O_WRONLY);
        if (fd == -1)
        {
            ERROR("Failed to open %s\n", memPath);
            SetLastError(ERROR_INVALID_ACCESS);
            goto CLEANUP2;
        }

        if (lseek(fd, (DWORD) lpBaseAddress, SEEK_SET) == -1)
        {
            ERROR("Failed to seek to base address\n");
            SetLastError(ERROR_INVALID_ACCESS);
            goto CLEANUP2;
        }
        
        bytesWritten = write(fd, lpBuffer, nSize);
        if (lpNumberOfBytesWritten != NULL)
        {
            *lpNumberOfBytesWritten = bytesWritten;
        }

#else   // HAVE_PROCFS_CTL
        FirstIntOffset = (unsigned int)lpBaseAddress % sizeof(int);    
        FirstIntMask = -1;
        FirstIntMask <<= (FirstIntOffset * 8);
        
        nbInts = (nSize + FirstIntOffset) / sizeof(int) + 
                 (((nSize + FirstIntOffset)%sizeof(int)) ? 1:0);
        lpBaseAddressAligned = (LPVOID)((char*)lpBaseAddress - FirstIntOffset);
        
        if ((lpTmpBuffer = malloc(nbInts * sizeof(int))) == NULL)
        {
            ERROR("Insufficient memory available !\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto CLEANUP1;
        }
        
        memcpy( (char *)lpTmpBuffer + FirstIntOffset, (char *)lpBuffer, nSize);    
        lpInt = lpTmpBuffer;

        LastIntOffset = (nSize + FirstIntOffset) % sizeof(int);
        LastIntMask = -1;
        LastIntMask >>= ((sizeof(int) - LastIntOffset) * 8);
        
        if (nbInts == 1)
        {
            if (DBGWriteProcMem_IntWithMask(processId, lpBaseAddressAligned, 
                                            *lpInt,
                                            LastIntMask & FirstIntMask)
                  == 0)
            {
                goto CLEANUP2;
            }
            if (lpNumberOfBytesWritten)
            {
                *lpNumberOfBytesWritten = nSize;
            }
            ret = TRUE;
            goto CLEANUP2;
        }
        
        if (DBGWriteProcMem_IntWithMask(processId,
                                        ((int*)lpBaseAddressAligned)++,
                                        *lpInt++, FirstIntMask) 
            == 0)
        {
            goto CLEANUP2;
        }

        while (--nbInts > 1)
        {      
          if (DBGWriteProcMem_Int(processId, ((int*)lpBaseAddressAligned)++,
                                  *lpInt++) == 0)
          {
              goto CLEANUP2;
          }
        }
        
        if (DBGWriteProcMem_IntWithMask(processId, lpBaseAddressAligned,
                                        *lpInt, LastIntMask ) == 0)
        {
            goto CLEANUP2;
        }

        if (lpNumberOfBytesWritten)
        {
            *lpNumberOfBytesWritten = nSize;
        }
#endif  // HAVE_PROCFS_CTL
        ret = TRUE;
    }
    else
    {
        /* Failed to attach processId */
        goto EXIT;    
    }
    
CLEANUP2:
#if HAVE_PROCFS_CTL
    if (fd != -1)
    {
        close(fd);
    }
#else   // HAVE_PROCFS_CTL
    if (lpTmpBuffer) 
    {
        free(lpTmpBuffer);
    }
#endif  // HAVE_PROCFS_CTL

#if !HAVE_PROCFS_CTL
CLEANUP1:
#endif  // !HAVE_PROCFS_CTL
    if (!DBGDetachProcess(processId))
    {
        /* Failed to detach processId */
        ret = FALSE;
    }
#endif  // HAVE_VM_READ
EXIT:
    LOGEXIT("WriteProcessMemory returns BOOL %d\n", ret);
    return ret;
}

#if !HAVE_VM_READ && !HAVE_PROCFS_CTL
/*++
Function:
  DBGWriteProcMem_Int

Abstract
  write one int to a process memory address

Parameter
  processId : process handle
  addr : memory address where the int should be written
  data : int to be written in addr

Return
  Return 1 if it succeeds, or 0 if it's fails
--*/
static
int
DBGWriteProcMem_Int(IN DWORD processId, 
                    IN int *addr,
                    IN int data)
{
    if (ptrace( PT_WRITE_D, processId, PT_ADDR(addr), data ) == -1)
    {
        if (errno == EFAULT) 
        {
            ERROR("ptrace(PT_WRITE_D, pid:%d caddr_t:%p data:%x) failed "
                  "errno:%d (%s)\n", processId, addr, data, errno, strerror(errno));
            SetLastError(ERROR_INVALID_ADDRESS);
        }
        else
        {
            ASSERT("ptrace(PT_WRITE_D, pid:%d caddr_t:%p data:%x) failed "
                  "errno:%d (%s)\n", processId, addr, data, errno, strerror(errno));
            SetLastError(ERROR_INTERNAL_ERROR);
        }
        return 0;
    }

    return 1;
}

/*++
Function:
  DBGWriteProcMem_IntWithMask

Abstract
  write one int to a process memory address space using mask

Parameter
  processId : process ID
  addr : memory address where the int should be written
  data : int to be written in addr
  mask : the mask used to write only a parts of data

Return
  Return 1 if it succeeds, or 0 if it's fails
--*/
static
int
DBGWriteProcMem_IntWithMask(IN DWORD processId,
                            IN int *addr,
                            IN int data,
                            IN unsigned int mask )
{
    int readInt;

    if (mask != ~0)
    {
        errno = 0;
        if (((readInt = ptrace( PT_READ_D, processId, PT_ADDR(addr), 0 )) == -1)
             && errno)
        {
            if (errno == EFAULT) 
            {
                ERROR("ptrace(PT_READ_D, pid:%d, caddr_t:%p, 0) failed "
                      "errno:%d (%s)\n", processId, addr, errno, strerror(errno));
                SetLastError(ERROR_INVALID_ADDRESS);
            }
            else
            {
                ASSERT("ptrace(PT_READ_D, pid:%d, caddr_t:%p, 0) failed "
                      "errno:%d (%s)\n", processId, addr, errno, strerror(errno));
                SetLastError(ERROR_INTERNAL_ERROR);
            }

            return 0;
        }
        data = (data & mask) | (readInt & ~mask);
    }    
    return DBGWriteProcMem_Int(processId, addr, data);
}
#endif  // !HAVE_VM_READ && !HAVE_PROCFS_CTL

/*++
Function:
  DBGAttachProcess

Abstract  
  
  Attach the indicated process to the current process. 
  
  if the indicated process is already attached by the current process, then 
  increment the number of attachment pending. if ot, attach it to the current 
  process (with PT_ATTACH).

Parameter
  processId : process ID to attach
Return
  Return true if it succeeds, or false if it's fails
--*/
BOOL 
DBGAttachProcess(DWORD processId)
{
    int attchmentCount;
    int savedErrno;
#if HAVE_PROCFS_CTL
    int fd;
    char ctlPath[1024];
#endif  // HAVE_PROCFS_CTL

    attchmentCount = 
        DBGSetProcessAttached(processId, GetCurrentProcessId(), DBG_ATTACH);

    if (attchmentCount == -1)
    {
        /* Failed to set the process as attached */
        goto EXIT;
    }
    
    if (attchmentCount == 1)
    {
#if HAVE_PROCFS_CTL
        struct timespec waitTime;

        // FreeBSD has some trouble when a series of attach/detach sequences
        // occurs too close together.  When this happens, we'll be able to
        // attach to the process, but waiting for the process to stop
        // (either via writing "wait" to /proc/<pid>/ctl or via waitpid)
        // will hang.  If we pause for a very short amount of time before
        // trying to attach, we don't run into the bug.
        // PR 35175 in the FreeBSD bug database is likely related to this
        // problem, which occurs regardless of whether we use ptrace
        // or procfs to communicate with the other process.
        waitTime.tv_sec = 0;
        waitTime.tv_nsec = 50000000;
        nanosleep(&waitTime, NULL);
        
        sprintf(ctlPath, "/proc/%d/ctl", processId);
        fd = open(ctlPath, O_WRONLY);
        if (fd == -1)
        {
            ERROR("Failed to open %s: errno is %d (%s)\n", ctlPath,
                  errno, strerror(errno));
            goto DETACH1;
        }
        
        if (write(fd, CTL_ATTACH, sizeof(CTL_ATTACH)) < sizeof(CTL_ATTACH))
        {
            ERROR("Failed to attach to %s: errno is %d (%s)\n", ctlPath,
                  errno, strerror(errno));
            close(fd);
            goto DETACH1;
        }
        
        if (write(fd, CTL_WAIT, sizeof(CTL_WAIT)) < sizeof(CTL_WAIT))
        {
            ERROR("Failed to wait for %s: errno is %d (%s)\n", ctlPath,
                  errno, strerror(errno));
            goto DETACH2;
        }
        
        close(fd);
#else   // HAVE_PROCFS_CTL
        if (ptrace( PT_ATTACH, processId, 0, 0 ) == -1)
        {
            if (errno != ESRCH)
            {                
                ASSERT("ptrace(PT_ATTACH, pid:%d) failed errno:%d (%s)\n",
                     processId, errno, strerror(errno));
            }
            goto DETACH1;
        }
                    
        if (waitpid(processId, NULL, WUNTRACED) == -1)
        {
            if (errno != ESRCH)
            {
                ASSERT("waitpid(pid:%d, NULL, WUNTRACED) failed.errno:%d"
                       " (%s)\n", processId, errno, strerror(errno));
            }
            goto DETACH2;
        }
#endif  // HAVE_PROCFS_CTL
    }
    
    return TRUE;

DETACH2:
#if HAVE_PROCFS_CTL
    if (write(fd, CTL_DETACH, sizeof(CTL_DETACH)) < sizeof(CTL_DETACH))
    {
        ASSERT("Failed to detach from %s: errno is %d (%s)\n", ctlPath,
               errno, strerror(errno));
    }
    close(fd);
#else   // HAVE_PROCFS_CTL
    if (ptrace(PT_DETACH, processId, 0, 0) == -1)
    {
        ASSERT("ptrace(PT_DETACH, pid:%d) failed. errno:%d (%s)\n", processId, 
              errno, strerror(errno));
    }
#endif  // HAVE_PROCFS_CTL
DETACH1:
    savedErrno = errno;
    DBGSetProcessAttached(processId, GetCurrentProcessId(), DBG_DETACH);
    errno = savedErrno;
EXIT:
    if (errno == ESRCH || errno == ENOENT || errno == EBADF)
    {
        ERROR("Invalid process ID:%d\n", processId);
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        SetLastError(ERROR_INTERNAL_ERROR);
    }
    return FALSE;
}

/*++
Function:
  DBGDetachProcess

Abstract
  Detach the indicated process from the current process.
  
  if the indicated process is already attached by the current process, then 
  decrement the number of attachment pending and detach it from the current 
  process (with PT_DETACH) if there's no more attachment left. 
  
Parameter
  processId : process handle

Return
  Return true if it succeeds, or true if it's fails
--*/
BOOL
DBGDetachProcess(DWORD processId)
{     
    int nbAttachLeft;
#if HAVE_PROCFS_CTL
    int fd;
    char ctlPath[1024];
#endif  // HAVE_PROCFS_CTL

    nbAttachLeft = DBGSetProcessAttached(processId, GetCurrentProcessId(), 
                                         DBG_DETACH);    
    if (nbAttachLeft == -1)
    {
        /* Failed to set the process as detached */
        return FALSE;
    }
    
    /* check if there's no more attachment left on processId */
    if (nbAttachLeft == 0)
    {
#if HAVE_PROCFS_CTL
        sprintf(ctlPath, "/proc/%d/ctl", processId);
        fd = open(ctlPath, O_WRONLY);
        if (fd == -1)
        {
            if (errno == ENOENT)
            {
                ERROR("Invalid process ID: %d\n", processId);
                SetLastError(ERROR_INVALID_PARAMETER);
            }
            else
            {
                ERROR("Failed to open %s: errno is %d (%s)\n", ctlPath,
                      errno, strerror(errno));
                SetLastError(ERROR_INTERNAL_ERROR);
            }
            return FALSE;
        }
        
        if (write(fd, CTL_DETACH, sizeof(CTL_DETACH)) < sizeof(CTL_DETACH))
        {
            ERROR("Failed to detach from %s: errno is %d (%s)\n", ctlPath,
                  errno, strerror(errno));
            close(fd);
            return FALSE;
        }
        close(fd);
        
#else   // HAVE_PROCFS_CTL
        if (ptrace(PT_DETACH, processId, PT_ADDR(1), 0) == -1)
        {            
            if (errno == ESRCH)
            {
                ERROR("Invalid process ID: %d\n", processId);
                SetLastError(ERROR_INVALID_PARAMETER);
            }
            else
            {
                ASSERT("ptrace(PT_DETACH, pid:%d) failed. errno:%d (%s)\n", 
                      processId, errno, strerror(errno));
                SetLastError(ERROR_INTERNAL_ERROR);
            }
            return FALSE;
        }
#endif  // HAVE_PROCFS_CTL
        
        if (kill(processId, SIGCONT) == -1)
        {
            ERROR("Failed to continue the detached process:%d errno:%d (%s)\n",
                  processId, errno, strerror(errno));
            return FALSE;
        }
    }
    return TRUE;
}

/*++
Function:
  DBGSetProcessAttached

Abstract
  saves the current process Id in the attached process structure

Parameter
  attachedProcId : process ID of the attached process
  attacherProcId : process ID of the attacher process
  bAttach : true (false) to set the process as attached (as detached)
Return
 returns the number of attachment left on attachedProcId, or -1 if it fails
--*/
static int
DBGSetProcessAttached(DWORD attachedProcId,
                      DWORD attacherProcId,
                      BOOL  bAttach)
{
    SHMPROCESS *shmprocess;
    int ret = -1;
    
    SHMLock();
    
    if (attachedProcId == GetCurrentProcessId())
    {
        ERROR("The attached process couldn't be the current process\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto EXIT;        
    }
    
    if(!(shmprocess = PROCFindSHMProcess(attachedProcId)))
    {
        ASSERT("Process 0x%08x was not found in shared memory.\n", 
               attachedProcId);
        SetLastError(ERROR_INTERNAL_ERROR);
        goto EXIT;
    }    
    
    if ((shmprocess->attachedByProcId != attacherProcId) && 
        (shmprocess->attachedByProcId !=0))
    {
        ERROR("process ID:%d already attached by an other process ID:%d\n",
              attachedProcId, shmprocess->attachedByProcId);
        SetLastError(ERROR_NOACCESS);
        goto EXIT;
    }
    
    if (bAttach)
    {
        shmprocess->attachedByProcId = attacherProcId;
        shmprocess->attachCount++;
    }
    else
    {
        if (shmprocess->attachCount-- <= 0)
        {
            ASSERT("attachCount <= 0 check for extra PROCSetProcessDetached calls\n");
            SetLastError(ERROR_INTERNAL_ERROR);
            goto EXIT;
        }
        
        if (shmprocess->attachCount == 0)
        {
            shmprocess->attachedByProcId = 0;
        }
    }
    ret = shmprocess->attachCount;

EXIT:    
    SHMRelease();
    return ret;
}

