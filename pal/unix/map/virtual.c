/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    virtual.c

Abstract:

    Implementation of virtual memory management functions.

--*/

#include "config.h"
#include "pal/palinternal.h"
#include "pal/critsect.h"
#include "pal/dbgmsg.h"
#include "pal/virtual.h"
#include "pal/map.h"
#include "pal/thread.h"
#include "pal/seh.h"
#include "pal/init.h"
#include "common.h"

#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#if HAVE_VM_ALLOCATE
#include <mach/vm_map.h>
#include <mach/mach_init.h>
#endif // HAVE_VM_ALLOCATE

SET_DEFAULT_DEBUG_CHANNEL(VIRTUAL);

CRITICAL_SECTION virtual_critsec;

#if MMAP_IGNORES_HINT
typedef struct FREE_BLOCK {
    char *startBoundary;
    UINT memSize;
    struct FREE_BLOCK *next;
} FREE_BLOCK;
#endif  // MMAP_IGNORES_HINT

// The first node in our list of allocated blocks.
static PCMI pVirtualMemory;

#if MMAP_IGNORES_HINT
// The first node in our list of freed blocks.
static FREE_BLOCK *pFreeMemory;

// The amount of memory that we'll try to reserve on our file.
// Currently 1GB.
static const int BACKING_FILE_SIZE = 1024 * 1024 * 1024;

static void *VIRTUALReserveFromBackingFile(UINT addr, size_t length);
static BOOL VIRTUALAddToFreeList(const PCMI pMemoryToBeReleased);

// The base address of the pages mapped onto our backing file.
static void *gBackingBaseAddress = MAP_FAILED;

// Separate the subset of the feature for experiments
#define RESERVE_FROM_BACKING_FILE 1
#else
// static const void *gBackingBaseAddress = MAP_FAILED;
// #define RESERVE_FROM_BACKING_FILE 1
#endif

#if RESERVE_FROM_BACKING_FILE
static BOOL VIRTUALGetBackingFile(void);

// The file that we're using to back our pages.
static int gBackingFile = -1;
#endif // RESERVE_FROM_BACKING_FILE

/*++
Function:
    VIRTUALInitialize()
    
    Initializes this section's critical section.

Return value:
    TRUE  if initialization succeeded
    FALSE otherwise.

--*/
BOOL
VIRTUALInitialize()
{
    TRACE( "Initializing the Virtual Critical Sections. \n" );

    if (0 != SYNCInitializeCriticalSection( &virtual_critsec ))
    {
        ERROR( "Unable to initialize virtual critical section\n");
        return FALSE;
    }   

    pVirtualMemory = NULL;
    return TRUE;
}

/***
 *
 * VIRTUALCleanup()
 *      Deletes this section's critical section.
 *
 */
void VIRTUALCleanup()
{
    PCMI pEntry;
    PCMI pTempEntry;
#if MMAP_IGNORES_HINT
    FREE_BLOCK *pFreeBlock;
    FREE_BLOCK *pTempFreeBlock;
#endif  // MMAP_IGNORES_HINT

    SYNCEnterCriticalSection( &virtual_critsec , TRUE);

    // Clean up the allocated memory.
    pEntry = pVirtualMemory;
    while ( pEntry )
    {
        WARN( "The memory at %d was not freed through a call to VirtualFree.\n",
              pEntry->startBoundary );
        free( pEntry->pAllocState );
        free( pEntry->pProtectionState );
        pTempEntry = pEntry;
        pEntry = pEntry->pNext;
        free( pTempEntry );
    }
    pVirtualMemory = NULL;
    
#if MMAP_IGNORES_HINT
    // Clean up the free list.
    pFreeBlock = pFreeMemory;
    while (pFreeBlock != NULL)
    {
        // Ignore errors from munmap. There's nothing we'd really want to
        // do about them.
        munmap(pFreeBlock->startBoundary, pFreeBlock->memSize);
        pTempFreeBlock = pFreeBlock;
        pFreeBlock = pFreeBlock->next;
        free(pTempFreeBlock);
    }
    pFreeMemory = NULL;
    gBackingBaseAddress = MAP_FAILED;   
#endif  // MMAP_IGNORES_HINT

#if RESERVE_FROM_BACKING_FILE
    if (gBackingFile != -1)
    {
        close(gBackingFile);
        gBackingFile = -1;
    }
#endif  // RESERVE_FROM_BACKING_FILE

    SYNCLeaveCriticalSection( &virtual_critsec , TRUE);

    TRACE( "Deleting the Virtual Critical Sections. \n" );
    DeleteCriticalSection( &virtual_critsec );
}

/***
 *
 *  VIRTUALContainsInvalidProtectionFlags()
 *          Returns TRUE if an invalid flag is specified. FALSE otherwise.
 */
static BOOL VIRTUALContainsInvalidProtectionFlags( IN DWORD flProtect )
{
    if ( ( flProtect & ~( PAGE_NOACCESS | PAGE_READONLY |
                          PAGE_READWRITE | PAGE_EXECUTE | PAGE_EXECUTE_READ |
                          PAGE_EXECUTE_READWRITE ) ) != 0 )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/****
 *
 * VIRTUALIsPageCommitted
 *
 *  UINT nBitToRetrieve - Which page to check.
 *
 *  Returns TRUE if committed, FALSE otherwise.
 *
 */
static BOOL VIRTUALIsPageCommitted( UINT nBitToRetrieve, CONST PCMI pInformation )
{
    UINT nByteOffset = 0;
    UINT nBitOffset = 0;
    UINT byteMask = 0;

    if ( !pInformation )
    {
        ERROR( "pInformation was NULL!\n" );
        return FALSE;
    }
    
    nByteOffset = nBitToRetrieve / CHAR_BIT;
    nBitOffset = nBitToRetrieve % CHAR_BIT;

    byteMask = 1 << nBitOffset;
    
    if ( pInformation->pAllocState[ nByteOffset ] & byteMask )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*********
 *
 *  VIRTUALGetAllocationType
 *
 *      IN UINT Index - The page within the range to retrieve 
 *                      the state for.
 *
 *      IN pInformation - The virtual memory object.
 *
 */
static INT VIRTUALGetAllocationType( UINT Index, CONST PCMI pInformation )
{
    if ( VIRTUALIsPageCommitted( Index, pInformation ) )
    {
        return MEM_COMMIT;
    }
    else
    {
        return MEM_RESERVE;
    }
}

/****
 *
 * VIRTUALSetAllocState
 *
 *  IN UINT nAction - Which action to perform.
 *  IN UINT nStartingBit - The bit to set.
 *
 *  IN UINT nNumberOfBits - The range of bits to set.
 *  IN PCMI pStateArray - A pointer the array to be manipulated.
 *
 *  Returns TRUE on success, FALSE otherwise.
 *  Turn bit on to indicate committed, turn bit off to indicate reserved.
 *         
 */
static BOOL VIRTUALSetAllocState( UINT nAction, INT nStartingBit, 
                           UINT nNumberOfBits, CONST PCMI pInformation )
{
    /* byte masks for optimized modification of partial bytes (changing less 
       than 8 bits in a single byte). note that bits are treated in little 
       endian order : value 1 is bit 0; value 128 is bit 7. in the binary 
       representations below, bit 0 is on the right */

    /* start masks : for modifying bits >= n while preserving bits < n. 
       example : if nStartignBit%8 is 3, then bits 0, 1, 2 remain unchanged 
       while bits 3..7 are changed; startmasks[3] can be used for this.  */
    static const BYTE startmasks[8] = {
        0xff, /* start at 0 : 1111 1111 */
        0xfe, /* start at 1 : 1111 1110 */
        0xfc, /* start at 2 : 1111 1100 */
        0xf8, /* start at 3 : 1111 1000 */
        0xf0, /* start at 4 : 1111 0000 */
        0xe0, /* start at 5 : 1110 0000 */
        0xc0, /* start at 6 : 1100 0000 */
        0x80  /* start at 7 : 1000 0000 */
    };

    /* end masks : for modifying bits <= n while preserving bits > n. 
       example : if the last bit to change is 5, then bits 6 & 7 stay unchanged 
       while bits 1..5 are changed; endmasks[5] can be used for this.  */
    static const BYTE endmasks[8] = {
        0x01, /* end at 0 : 0000 0001 */
        0x03, /* end at 1 : 0000 0011 */
        0x07, /* end at 2 : 0000 0111 */
        0x0f, /* end at 3 : 0000 1111 */
        0x1f, /* end at 4 : 0001 1111 */
        0x3f, /* end at 5 : 0011 1111 */
        0x7f, /* end at 6 : 0111 1111 */
        0xff  /* end at 7 : 1111 1111 */
    };
    /* last example : if only the middle of a byte must be changed, both start 
       and end masks can be combined (bitwise AND) to obtain the correct mask. 
       if we want to change bits 2 to 4 : 
       startmasks[2] : 0xfc   1111 1100  (change 2,3,4,5,6,7)
       endmasks[4]:    0x1f   0001 1111  (change 0,1,2,3,4)
       bitwise AND :   0x1c   0001 1100  (change 2,3,4)
     */
    
    BYTE byte_mask;
    UINT nLastBit;
    UINT nFirstByte;
    UINT nLastByte;
    UINT nFullBytes;

    TRACE( "VIRTUALSetAllocState( nAction = %d, nStartingBit = %d, "
           "nNumberOfBits = %d, pStateArray = 0x%p )\n", 
           nAction, nStartingBit, nNumberOfBits, pInformation ); 

    if ( !pInformation )
    {
        ERROR( "pInformation was invalid!\n" );
        return FALSE;
    }

    if ( 0 == nNumberOfBits )
    {
        ERROR( "nNumberOfBits was 0!\n" );
        return FALSE;
    }

    nLastBit = nStartingBit+nNumberOfBits-1;
    nFirstByte = nStartingBit / 8;
    nLastByte = nLastBit / 8;

    /* handle partial first byte (if any) */
    if(0 != (nStartingBit % 8))
    {
        byte_mask = startmasks[nStartingBit % 8];

        /* if 1st byte is the only changing byte, combine endmask to preserve 
           trailing bits (see 3rd example above) */
        if( nLastByte == nFirstByte)
        {
            byte_mask &= endmasks[nLastBit % 8];
        }

        /* byte_mask contains 1 for bits to change, 0 for bits to leave alone */
        if(MEM_COMMIT == nAction)
        {
            /* bits to change must be set to 1 : use bitwise OR */
            pInformation->pAllocState[ nFirstByte ] |= byte_mask;
        }
        else
        {
            /* bits to change must be set to 0 : invert byte_mask (giving 0 for 
               bits to change), use bitwise AND */
            byte_mask = ~byte_mask;
            pInformation->pAllocState[ nFirstByte ] &= byte_mask;
        }

        /* stop right away if only 1 byte is being modified */
        if(nLastByte == nFirstByte)
        {
            return TRUE;
        }

        /* we're done with the 1st byte; skip over it */
        nFirstByte++;
    }

    /* number of bytes to change, excluding the last byte (handled separately)*/
    nFullBytes = nLastByte - nFirstByte;
    
    if(0 != nFullBytes)
    {
        /* set bits to 0 or 1, as requested */
        if ( MEM_COMMIT == nAction )
        {
            memset( &(pInformation->pAllocState[ nFirstByte ]), 
                    0xFF, nFullBytes );
        }
        else
        {
            memset( &(pInformation->pAllocState[ nFirstByte ]),
                    0, nFullBytes );
        }    
    }

    /* handle last (possibly partial) byte */
    byte_mask = endmasks[nLastBit % 8];

    /* byte_mask contains 1 for bits to change, 0 for bits to leave alone */
    if(MEM_COMMIT == nAction)
    {
        /* bits to change must be set to 1 : use bitwise OR */
        pInformation->pAllocState[ nLastByte ] |= byte_mask;
    }
    else
    {
        /* bits to change must be set to 0 : invert byte_mask (giving 0 for 
           bits to change), use bitwise AND */
        byte_mask = ~byte_mask;
        pInformation->pAllocState[ nLastByte ] &= byte_mask;
    }

    return TRUE;
}

/****
 *
 * VIRTUALFindRegionInformation( )
 *
 *          IN UINT address - The address to look for.
 *
 *          Returns the PCMI if found, NULL otherwise.
 */
static PCMI VIRTUALFindRegionInformation( IN UINT address ) 
{
    PCMI pEntry = NULL;
    
    TRACE( "VIRTUALFindRegionInformation( %#x )\n", address );

    pEntry = pVirtualMemory;
    
    while( pEntry )
    {
        if ( pEntry->startBoundary > address )
        {
            /* Gone past the possible location in the list. */
            pEntry = NULL;
            break;
        }
        if ( pEntry->startBoundary + pEntry->memSize > address ) 
        {
            break;
        }
        
        pEntry = pEntry->pNext;
    }
    return pEntry;
}

/*++
Function :

    VIRTUALReleaseMemory
    
    Removes a PCMI entry from the list.
    
    Returns true on success. FALSE otherwise.
--*/
static BOOL VIRTUALReleaseMemory( PCMI pMemoryToBeReleased )
{
    BOOL bRetVal = TRUE;
    
    if ( !pMemoryToBeReleased )
    {
        ASSERT( "Invalid pointer.\n" );
        return FALSE;
    }

    
    if ( pMemoryToBeReleased == pVirtualMemory )
    {
        /* This is either the first entry, or the only entry. */
        pVirtualMemory = pMemoryToBeReleased->pNext;
        if ( pMemoryToBeReleased->pNext )
        {
            pMemoryToBeReleased->pNext->pLast = NULL;
        }
    }
    else /* Could be anywhere in the list. */
    {
        /* Delete the entry from the linked list. */
        if ( pMemoryToBeReleased->pLast )
        {
            pMemoryToBeReleased->pLast->pNext = pMemoryToBeReleased->pNext;
        }
        
        if ( pMemoryToBeReleased->pNext )
        {
            pMemoryToBeReleased->pNext->pLast = pMemoryToBeReleased->pLast;
        }
    }

#if MMAP_IGNORES_HINT
    // We've removed the block from our allocated list. Add it to the
    // free list.
    bRetVal = VIRTUALAddToFreeList(pMemoryToBeReleased);
#endif  // MMAP_IGNORES_HINT

    free( pMemoryToBeReleased->pAllocState );
    pMemoryToBeReleased->pAllocState = NULL;
    
    free( pMemoryToBeReleased->pProtectionState );
    pMemoryToBeReleased->pProtectionState = NULL;
    
    free( pMemoryToBeReleased );
    pMemoryToBeReleased = NULL;

    return bRetVal;
}

/****
 *  VIRTUALConvertWinFlags() - 
 *          Converts win32 protection flags to
 *          internal VIRTUAL flags.
 *
 */
static BYTE VIRTUALConvertWinFlags( IN DWORD flProtect )
{
    BYTE MemAccessControl = 0;

    switch ( flProtect & 0xff )
    {
    case PAGE_NOACCESS :
        MemAccessControl = VIRTUAL_NOACCESS;
        break;
    case PAGE_READONLY :
        MemAccessControl = VIRTUAL_READONLY;
        break;
    case PAGE_READWRITE :
        MemAccessControl = VIRTUAL_READWRITE;
        break;
    case PAGE_EXECUTE :
        MemAccessControl = VIRTUAL_EXECUTE;
        break;
    case PAGE_EXECUTE_READ :
        MemAccessControl = VIRTUAL_EXECUTE_READ;
        break;
    case PAGE_EXECUTE_READWRITE:
        MemAccessControl = VIRTUAL_EXECUTE_READWRITE;
        break;
    
    default :
        MemAccessControl = 0;
        ERROR( "Incorrect or no protection flags specified.\n" );
        break;
    }
    return MemAccessControl;
}
/****
 *  VIRTUALConvertVirtualFlags() - 
 *              Converts internal virtual protection
 *              flags to their win32 counterparts.
 */
static DWORD VIRTUALConvertVirtualFlags( IN BYTE VirtualProtect )
{
    DWORD MemAccessControl = 0;

    if ( VirtualProtect == VIRTUAL_READONLY )
    {
        MemAccessControl = PAGE_READONLY;
    }
    else if ( VirtualProtect == VIRTUAL_READWRITE )
    {
        MemAccessControl = PAGE_READWRITE;
    }
    else if ( VirtualProtect == VIRTUAL_EXECUTE_READWRITE )
    {
        MemAccessControl = PAGE_EXECUTE_READWRITE;
    }
    else if ( VirtualProtect == VIRTUAL_EXECUTE_READ )
    {
        MemAccessControl = PAGE_EXECUTE_READ;
    }
    else if ( VirtualProtect == VIRTUAL_EXECUTE )
    {
        MemAccessControl = PAGE_EXECUTE;
    }
    else if ( VirtualProtect == VIRTUAL_NOACCESS )
    {
        MemAccessControl = PAGE_NOACCESS;
    }

    else
    {
        MemAccessControl = 0;
        ERROR( "Incorrect or no protection flags specified.\n" );
    }
    return MemAccessControl;
}


/***
 *  Displays the linked list.
 *
 */
#if defined _DEBUG
static void VIRTUALDisplayList( void  )
{
    PCMI p;
    UINT count;
    UINT index;

    SYNCEnterCriticalSection( &virtual_critsec , TRUE);

    p = pVirtualMemory;
    count = 0;
    while ( p ) {

        DBGOUT( "Entry %d : \n", count );
        DBGOUT( "\t startBoundary %#x \n", p->startBoundary );
        DBGOUT( "\t memSize %d \n", p->memSize );

        DBGOUT( "\t pAllocState " );
        for ( index = 0; index < p->memSize / PAGE_SIZE; index++)
        {
            DBGOUT( "[%d] ", VIRTUALGetAllocationType( index, p ) );
        }
        DBGOUT( "\t pProtectionState " );
        for ( index = 0; index < p->memSize / PAGE_SIZE; index++ )
        {
            DBGOUT( "[%d] ", (UINT)p->pProtectionState[ index ] );
        }
        DBGOUT( "\n" );
        DBGOUT( "\t accessProtection %d \n", p->accessProtection );
        DBGOUT( "\t allocationType %d \n", p->allocationType );
        DBGOUT( "\t pNext %p \n", p->pNext );
        DBGOUT( "\t pLast %p \n", p->pLast );

        count++;
        p = p->pNext;
    }
    SYNCLeaveCriticalSection( &virtual_critsec , TRUE);
}
#endif

/****
 *  VIRTUALStoreAllocationInfo()
 *
 *      Stores the allocation information in the linked list.
 *      NOTE: The caller must own the critical section.
 */
static BOOL VIRTUALStoreAllocationInfo( 
            IN UINT startBoundary,      /* Start of the region. */
            IN UINT memSize,            /* Size of the region. */
            IN DWORD flAllocationType,  /* Allocation Types. */
            IN DWORD flProtection )     /* Protections flags on the memory. */
{
    PCMI pNewEntry   = NULL;
    PCMI pMemInfo    = NULL;
    BOOL bRetVal     = TRUE;
    UINT nBufferSize = 0;

    if ( ( memSize & PAGE_MASK ) != 0 )
    {
        ERROR( "The memory size was not in multiples of the page size. \n" );
        bRetVal =  FALSE;
        goto done;
    }
    if ( !(pNewEntry = ( PCMI )malloc( sizeof( *pNewEntry ) ) ) )
    {
        ERROR( "Unable to allocate memory for the structure.\n");
        bRetVal =  FALSE;
        goto done;
    }
    
    pNewEntry->startBoundary    = startBoundary;
    pNewEntry->memSize          = memSize;
    pNewEntry->allocationType   = flAllocationType;
    pNewEntry->accessProtection = flProtection;
    
    nBufferSize = memSize / PAGE_SIZE / CHAR_BIT;
    if ( ( memSize / PAGE_SIZE ) % CHAR_BIT != 0 )
    {
        nBufferSize++;
    }
    
    pNewEntry->pAllocState      = (BYTE*)malloc( nBufferSize );
    pNewEntry->pProtectionState = (BYTE*)malloc( memSize / PAGE_SIZE );

    if ( pNewEntry->pAllocState && pNewEntry->pProtectionState )
    {
        /* Set the intial allocation state, and initial allocation protection. */
        VIRTUALSetAllocState( MEM_RESERVE, 0, nBufferSize * CHAR_BIT, pNewEntry );

        memset( pNewEntry->pProtectionState,
            VIRTUALConvertWinFlags( flProtection ),
            memSize / PAGE_SIZE );
    }
    else
    {
        ERROR( "Unable to allocate memory for the structure.\n");
        bRetVal =  FALSE;
        
        free( pNewEntry->pProtectionState );
        pNewEntry->pProtectionState = NULL;
        
        free( pNewEntry->pAllocState );
        pNewEntry->pAllocState = NULL;
        
        free( pNewEntry );
        pNewEntry = NULL;
        
        goto done;
    }
    
    pMemInfo = pVirtualMemory;

    if ( pMemInfo && pMemInfo->startBoundary < startBoundary )
    {
        /* Look for the correct insert point */
        TRACE( "Looking for the correct insert location.\n");
        while ( pMemInfo->pNext && ( pMemInfo->pNext->startBoundary < startBoundary ) ) 
        {
            pMemInfo = pMemInfo->pNext;
        }
        
        pNewEntry->pNext = pMemInfo->pNext;
        pNewEntry->pLast = pMemInfo;
        
        if ( pNewEntry->pNext ) 
        {
            pNewEntry->pNext->pLast = pNewEntry;
        }
        
        pMemInfo->pNext = pNewEntry;
    }
    else
    {
        TRACE( "Inserting a new element into the linked list\n" );
        /* This is the first entry in the list. */
        pNewEntry->pNext = pMemInfo;
        pNewEntry->pLast = NULL;
        
        if ( pNewEntry->pNext ) 
        {
            pNewEntry->pNext->pLast = pNewEntry;
        }
        
        pVirtualMemory = pNewEntry ;
    }
done:
    TRACE( "Exiting StoreAllocationInformation. \n" );
    return bRetVal;
}

/******
 *
 *  VIRTUALReserveMemory() - Helper function that actually reserves the memory.
 *
 *      NOTE: I call SetLastError in here, because many different error states
 *              exists, and that would be very complicated to work around.
 *
 */
static LPVOID VIRTUALReserveMemory(
                IN LPVOID lpAddress,       /* Region to reserve or commit */
                IN SIZE_T dwSize,          /* Size of Region */
                IN DWORD flAllocationType, /* Type of allocation */
                IN DWORD flProtect)        /* Type of access protection */
{
    LPVOID pRetVal      = NULL;
    UINT StartBoundary;
    UINT MemSize;
#if HAVE_VM_ALLOCATE
    int result;
#endif  // HAVE_VM_ALLOCATE
    int mmapFlags = 0;
    int mmapFile = -1;
    off_t mmapOffset = 0;


    TRACE( "Reserving the memory now..\n");

    // First, figure out where we're trying to reserve the memory and
    // how much we need. On most systems, requests to mmap must be
    // page-aligned and at multiples of the page size.
    StartBoundary = (UINT)lpAddress & ~BOUNDARY_64K;
    /* Add the sizes, and round down to the nearest page boundary. */
    MemSize = ( ((UINT)lpAddress + dwSize + PAGE_MASK) & ~PAGE_MASK ) - 
               StartBoundary;

    SYNCEnterCriticalSection( &virtual_critsec , TRUE);

#if MMAP_IGNORES_HINT
    pRetVal = VIRTUALReserveFromBackingFile(StartBoundary, MemSize);
#else   // MMAP_IGNORES_HINT
    // Most platforms will only commit the memory if it is dirtied,
    // so this should not consume too much swap space.

#if HAVE_VM_ALLOCATE
    // Allocate with vm_allocate first, then map at the fixed address.
    result = vm_allocate(mach_task_self(), &StartBoundary, MemSize,
                          ((LPVOID) StartBoundary != NULL) ? FALSE : TRUE);
    if (result != KERN_SUCCESS) {
        ERROR("vm_allocate failed to allocated the requested region!\n");
        SetLastError(ERROR_INVALID_ADDRESS);
        pRetVal = NULL;
        goto done;
    }
    mmapFlags |= MAP_FIXED;
#endif // HAVE_VM_ALLOCATE

#if RESERVE_FROM_BACKING_FILE
    mmapFile = gBackingFile;
    mmapOffset = (char *) StartBoundary - (char *) gBackingBaseAddress;
    mmapFlags |= MAP_PRIVATE;
#else // RESERVE_FROM_BACKING_FILE
    mmapFlags |= MAP_ANON | MAP_PRIVATE;
#endif // RESERVE_FROM_BACKING_FILE

    pRetVal = mmap((LPVOID) StartBoundary, MemSize, PROT_NONE,
                   mmapFlags, mmapFile, mmapOffset);

    /* Check to see if the region is what we asked for. */
    if (pRetVal != MAP_FAILED && lpAddress != NULL &&
        StartBoundary != (UINT) pRetVal)
    {
        ERROR("We did not get the region we asked for!\n");
        SetLastError(ERROR_INVALID_ADDRESS);
        munmap(pRetVal, MemSize);
        pRetVal = NULL;
        goto done;
    }
#endif  // MMAP_IGNORES_HINT

    if ( pRetVal != MAP_FAILED)
    {
#if MMAP_ANON_IGNORES_PROTECTION
        if (mprotect(pRetVal, MemSize, PROT_NONE) != 0)
        {
            ERROR("mprotect failed to protect the region!\n");
            SetLastError(ERROR_INVALID_ADDRESS);
            munmap(pRetVal, MemSize);
            pRetVal = NULL;
            goto done;
        }
#endif  // MMAP_ANON_IGNORES_PROTECTION
#if !MMAP_IGNORES_HINT
        if ( !lpAddress )
        {
#endif  // MMAP_IGNORES_HINT
            /* Compute the real values instead of the null values. */
            StartBoundary = (UINT)pRetVal & ~PAGE_MASK;
            MemSize = ( ((UINT)pRetVal + dwSize + PAGE_MASK) & ~PAGE_MASK ) - 
                      StartBoundary;
#if !MMAP_IGNORES_HINT
        }
#endif  // MMAP_IGNORES_HINT
        if ( !VIRTUALStoreAllocationInfo( StartBoundary, MemSize, 
                                   flAllocationType, flProtect ) )
        {
            ASSERT( "Unable to store the structure in the list.\n");
            SetLastError( ERROR_INTERNAL_ERROR );
            munmap( pRetVal, MemSize );
            pRetVal = NULL;
        }
    }
    else
    {
        ERROR( "Failed due to insufficent memory.\n" );
#if HAVE_VM_ALLOCATE
        vm_deallocate(mach_task_self(), StartBoundary, MemSize);
#endif  // HAVE_VM_ALLOCATE
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        pRetVal = NULL;
        goto done;
    }

done:
    SYNCLeaveCriticalSection( &virtual_critsec , TRUE);
    return pRetVal;
}

/******
 *
 *  VIRTUALCommitMemory() - Helper function that actually commits the memory.
 *
 *      NOTE: I call SetLastError in here, because many different error states
 *              exists, and that would be very complicated to work around.
 *
 */
static LPVOID VIRTUALCommitMemory(
                IN LPVOID lpAddress,       /* Region to reserve or commit */
                IN SIZE_T dwSize,          /* Size of Region */
                IN DWORD flAllocationType, /* Type of allocation */
                IN DWORD flProtect)        /* Type of access protection */
{
    UINT StartBoundary          = 0;
    UINT MemSize                = 0;
    PCMI pInformation           = 0;
    LPVOID pRetVal              = NULL;
    BOOL IsLocallyReserved      = FALSE;
    UINT totalPages;
    INT allocationType, curAllocationType;
    INT protectionState, curProtectionState;
    UINT initialRunStart;
    UINT runStart;
    UINT runLength;
    UINT index;
    INT nProtect;
    INT vProtect;

    if ( lpAddress )
    {
        StartBoundary = (UINT)lpAddress & ~PAGE_MASK;
        /* Add the sizes, and round down to the nearest page boundary. */
        MemSize = ( ((UINT)lpAddress + dwSize + PAGE_MASK) & ~PAGE_MASK ) - 
                  StartBoundary;
    }
    else
    {
        MemSize = ( dwSize + PAGE_MASK ) & ~PAGE_MASK;
    }

    /* See if we have already reserved this memory. */
    pInformation = VIRTUALFindRegionInformation( StartBoundary );
    
    if ( !pInformation )
    {
        /* According to the new MSDN docs, if MEM_COMMIT is specified,
        and the memory is not reserved, you reserve and then commit.
        */
        LPVOID pReservedMemory = 
                VIRTUALReserveMemory( lpAddress, dwSize, 
                                      flAllocationType, flProtect );
        
        TRACE( "Reserve and commit the memory!\n " );

        if ( pReservedMemory )
        {
            /* Re-align the addresses and try again to find the memory. */
            StartBoundary = (UINT)pReservedMemory & ~PAGE_MASK;
            MemSize = ( ((UINT)pReservedMemory + dwSize + PAGE_MASK) 
                        & ~PAGE_MASK ) - StartBoundary;
            
            pInformation = VIRTUALFindRegionInformation( StartBoundary );

            if ( !pInformation )
            {
                ASSERT( "Unable to locate the region information.\n" );
                SetLastError( ERROR_INTERNAL_ERROR );
                pRetVal = NULL;
                goto done;
            }
            IsLocallyReserved = TRUE;
        }
        else
        {
            ERROR( "Unable to reserve the memory.\n" );
            /* Don't set last error here, it will already be set. */
            pRetVal = NULL;
            goto done;
        }
    }
               
    TRACE( "Committing the memory now..\n");
    
    // Pages that aren't already committed need to be committed. Pages that
    // are committed don't need to be committed, but they might need to have
    // their permissions changed.
    // To get this right, we find runs of pages with similar states and
    // permissions. If a run is not committed, we commit it by writing a zero
    // to each page, and then set its permissions. If a run is committed but
    // has different permissions from what we're trying to set, we set its
    // permissions. Finally, if a run is already committed and has the right
    // permissions, we don't need to do anything to it.
    
    totalPages = MemSize / PAGE_SIZE;
    runStart = (StartBoundary - pInformation->startBoundary) /
                PAGE_SIZE;   // Page index
    initialRunStart = runStart;
    allocationType = VIRTUALGetAllocationType(runStart, pInformation);
    protectionState = pInformation->pProtectionState[runStart];
    curAllocationType = allocationType;
    curProtectionState = protectionState;
    runLength = 1;
    nProtect = W32toUnixAccessControl(flProtect);
    vProtect = VIRTUALConvertWinFlags(flProtect);

    if (totalPages > pInformation->memSize / PAGE_SIZE - runStart)
    {
        ERROR("Trying to commit beyond the end of the region!\n");
        goto error;
    }

    while(runStart < initialRunStart + totalPages)
    {
        // Find the next run of pages
        for(index = runStart + 1; index < initialRunStart + totalPages;
            index++)
        {
            curAllocationType = VIRTUALGetAllocationType(index, pInformation);
            curProtectionState = pInformation->pProtectionState[index];
            if (curAllocationType != allocationType ||
                curProtectionState != protectionState)
            {
                break;
            }
            runLength++;
        }

        StartBoundary = pInformation->startBoundary + runStart * PAGE_SIZE;
        MemSize = runLength * PAGE_SIZE;
        if (allocationType != MEM_COMMIT)
        {
            // Commit the pages
            if (mmap((void *) StartBoundary, MemSize, PROT_WRITE | PROT_READ,
                     MAP_ANON | MAP_FIXED | MAP_PRIVATE, -1, 0) != MAP_FAILED)
            {
                // Write a zero to the start of each page to commit it.
                int i;
                char *temp = (char *) StartBoundary;
                for(i = 0; i < runLength; i++)
                {
                    *temp = 0;
                    temp += PAGE_SIZE;
                }
            }
            else
            {
                ERROR("mmap() failed! Error(%d)=%s\n", errno, strerror(errno));
                goto error;
            }
            VIRTUALSetAllocState(MEM_COMMIT, runStart, runLength, pInformation);
            if (nProtect == (PROT_WRITE | PROT_READ))
            {
                // Handle this case specially so we don't bother
                // mprotect'ing the region.
                memset(pInformation->pProtectionState + runStart,
                       vProtect, runLength);
            }
            protectionState = VIRTUAL_READWRITE;
        }
        if (protectionState != vProtect)
        {
            // Change permissions.
            if (mprotect((void *) StartBoundary, MemSize, nProtect) != -1)
            {
                memset(pInformation->pProtectionState + runStart,
                       vProtect, runLength);
            }
            else
            {
                ERROR("mprotect() failed! Error(%d)=%s\n",
                      errno, strerror(errno));
                goto error;
            }
        }
        
        runStart = index;
        runLength = 1;
        allocationType = curAllocationType;
        protectionState = curProtectionState;
    }
    pRetVal = (void *) (pInformation->startBoundary +
                        initialRunStart * PAGE_SIZE);
    goto done;

error:
    if ( flAllocationType & MEM_RESERVE || IsLocallyReserved )
    {
#if MMAP_IGNORES_HINT
        mmap(pRetVal, MemSize, PROT_NONE, MAP_FIXED | MAP_PRIVATE,
             gBackingFile, (char *) pRetVal - (char *) gBackingBaseAddress);
#else   // MMAP_IGNORES_HINT
        munmap( pRetVal, MemSize );
#endif  // MMAP_IGNORES_HINT
        if ( VIRTUALReleaseMemory( pInformation ) == FALSE )
        {
            ASSERT( "Unable to remove the PCMI entry from the list.\n" );
            SetLastError( ERROR_INTERNAL_ERROR );
            pRetVal = NULL;
            goto done;
        }
        pInformation = NULL;
        pRetVal = NULL;
    }

done:

    return pRetVal;
}

#if MMAP_IGNORES_HINT
/*++
Function:
    VIRTUALReserveFromBackingFile

    Locates a reserved but unallocated block of memory in the free list.
    
    If addr is not zero, this will only find a block that starts at addr
    and is at least large enough to hold the requested size.
    
    If addr is zero, this finds the first block of memory in the free list
    of the right size.
    
    Once the block is located, it is split if necessary to allocate only
    the requested size. The function then calls mmap() with MAP_FIXED to
    map the located block at its address on an anonymous fd.
    
    This function requires that length be a multiple of the page size. If
    length is not a multiple of the page size, subsequently allocated blocks
    may be allocated on addresses that are not page-size-aligned, which is
    invalid.
    
    Returns the base address of the mapped block, or MAP_FAILED if no
    suitable block exists or mapping fails.
--*/
static void *VIRTUALReserveFromBackingFile(UINT addr, size_t length)
{
    FREE_BLOCK *block;
    FREE_BLOCK *prev;
    FREE_BLOCK *temp;
    void *returnAddress;
    
    block = NULL;
    prev = NULL;
    for(temp = pFreeMemory; temp != NULL; temp = temp->next)
    {
        if (addr != 0)
        {
            if (addr < (UINT) temp->startBoundary)
            {
                // Not up to a block containing addr yet.
                prev = temp;
                continue;
            }
        }
        if ((addr == 0 && temp->memSize >= length) ||
            (addr >= (UINT) temp->startBoundary &&
             addr + length <= (UINT) temp->startBoundary + temp->memSize))
        {
            block = temp;
            break;
        }
        prev = temp;
    }
    if (block == NULL)
    {
        // No acceptable page exists.
        return MAP_FAILED;
    }
    
    // Grab the return address before we adjust the free list.
    if (addr == 0)
    {
        returnAddress = block->startBoundary;
    }
    else
    {
        returnAddress = (void *) addr;
    }

    // Adjust the free list to account for the block we're returning.
    if (block->memSize == length && returnAddress == block->startBoundary)
    {
        // We're going to remove this free block altogether.
        if (prev == NULL)
        {
            // block is the first in our free list.
            pFreeMemory = block->next;
        }
        else
        {
            prev->next = block->next;
        }
        free(block);
    }
    else
    {
        // We have to divide this block. Map in the new block.
        if (returnAddress == (void *) block->startBoundary)
        {
            // The address is right at the beginning of the block.
            // We can make the block smaller.
            block->memSize -= length;
            block->startBoundary += length;
        }
        else if ((UINT) returnAddress + length ==
                 (UINT) block->startBoundary + block->memSize)
        {
            // The allocation is at the end of the block. Make the
            // block smaller from the end.
            block->memSize -= length;
        }
        else
        {
            // Splitting the block. We'll need a new block for the free list.
            temp = (FREE_BLOCK *) malloc(sizeof(FREE_BLOCK));
            if (temp == NULL)
            {
                ERROR("Failed to allocate memory for a new free block!");
                return MAP_FAILED;
            }
            temp->startBoundary = (void *) ((UINT) returnAddress + length);
            temp->memSize = (UINT) block->startBoundary + block->memSize -
                            ((UINT) returnAddress + length);
            temp->next = block->next;
            block->memSize -= (length + temp->memSize);
            block->next = temp;
        }
    }
    return returnAddress;
}

/*++
Function:
    VIRTUALAddToFreeList

    Adds the given block to our free list. Coalesces the list if necessary.
    The block should already have been mapped back onto the backing file.
    
    Returns TRUE if the block was added to the free list.
--*/
static BOOL VIRTUALAddToFreeList(const PCMI pMemoryToBeReleased)
{
    FREE_BLOCK *temp;
    FREE_BLOCK *lastBlock;
    FREE_BLOCK *newBlock;
    BOOL coalesced;
    
    lastBlock = NULL;
    for(temp = pFreeMemory; temp != NULL; temp = temp->next)
    {
        if ((UINT) temp->startBoundary > pMemoryToBeReleased->startBoundary)
        {
            // This check isn't necessary unless the PAL is fundamentally
            // broken elsewhere.
            if (pMemoryToBeReleased->startBoundary +
                pMemoryToBeReleased->memSize > (UINT) temp->startBoundary)
            {
                ASSERT("Free and allocated memory blocks overlap!");
                return FALSE;
            }
            break;
        }
        lastBlock = temp;
    }
    
    // Check to see if we're going to coalesce blocks before we
    // allocate anything.
    coalesced = FALSE;
    
    // First, are we coalescing with the next block?
    if (temp != NULL)
    {
        if ((UINT) temp->startBoundary == pMemoryToBeReleased->startBoundary +
            pMemoryToBeReleased->memSize)
        {
            temp->startBoundary = (char *) pMemoryToBeReleased->startBoundary;
            temp->memSize += pMemoryToBeReleased->memSize;
            coalesced = TRUE;
        }
    }
    
    // Are we coalescing with the previous block? If so, check to see
    // if we can free one of the blocks.
    if (lastBlock != NULL)
    {
        if ((UINT) lastBlock->startBoundary + lastBlock->memSize ==
            pMemoryToBeReleased->startBoundary)
        {
            if (lastBlock->next != NULL &&
                lastBlock->startBoundary + lastBlock->memSize ==
                lastBlock->next->startBoundary)
            {
                lastBlock->memSize += lastBlock->next->memSize;
                temp = lastBlock->next;
                lastBlock->next = lastBlock->next->next;
                free(temp);
            }
            else
            {
                lastBlock->memSize += pMemoryToBeReleased->memSize;
            }
            coalesced = TRUE;
        }
    }
    
    // If we coalesced anything, we're done.
    if (coalesced)
    {
        return TRUE;
    }
    
    // At this point we know we're not coalescing anything and we need
    // a new block.
    newBlock = (FREE_BLOCK *) malloc(sizeof(FREE_BLOCK));
    if (newBlock == NULL)
    {
        ERROR("Failed to allocate memory for a new free block!");
        return FALSE;
    }
    newBlock->startBoundary = (char *) pMemoryToBeReleased->startBoundary;
    newBlock->memSize = pMemoryToBeReleased->memSize;
    if (lastBlock == NULL)
    {
        newBlock->next = temp;
        pFreeMemory = newBlock;
    }
    else
    {
        newBlock->next = lastBlock->next;
        lastBlock->next = newBlock;
    }
    return TRUE;
}
#endif  // MMAP_IGNORES_HINT

#if RESERVE_FROM_BACKING_FILE
/*++
Function:
    VIRTUALGetBackingFile

    Ensures that we have a set of pages that correspond to a backing file.
    We use the PAL as the backing file merely because we're pretty confident
    it exists.
    
    When the backing file hasn't been created, we create it, mmap pages
    onto it, and create the free list.
    
    Returns TRUE if we could locate our backing file, open it, mmap
    pages onto it, and create the free list. Does nothing if we already
    have a mapping.
--*/
static BOOL VIRTUALGetBackingFile(void)
{
    BOOL result = FALSE;
    const char *palName;
    
    SYNCEnterCriticalSection(&virtual_critsec, TRUE);
    if (gBackingFile != -1)
    {
        result = TRUE;
        goto done;
    }

#if MMAP_IGNORES_HINT
    if (pFreeMemory != NULL)
    {
        // Sanity check. Our free list should always be NULL if we
        // haven't allocated our pages.
        ASSERT("Free list is unexpectedly non-NULL without a backing file!");
        goto done;
    }
#endif

    palName = LOADGetLibRotorPalSoFileName();
    gBackingFile = open(palName, O_RDONLY);
    if (gBackingFile == -1)
    {
        ASSERT("Failed to open %s as a backing file: errno=%d\n",
                palName, errno);
        goto done;
    }

#if MMAP_IGNORES_HINT
    gBackingBaseAddress = mmap(0, BACKING_FILE_SIZE, PROT_NONE,
                                MAP_PRIVATE, gBackingFile, 0);
    if (gBackingBaseAddress == MAP_FAILED)
    {
        ERROR("Failed to map onto the backing file: errno=%d\n", errno);
        // Hmph. This is bad.
        close(gBackingFile);
        gBackingFile = -1;
        goto done;
    }

    // Create our free list.
    pFreeMemory = (FREE_BLOCK *) malloc(sizeof(FREE_BLOCK));
    if (pFreeMemory == NULL)
    {
        // Not good.
        ERROR("Failed to allocate memory for the free list!");
        munmap(gBackingBaseAddress, BACKING_FILE_SIZE);
        close(gBackingFile);
        gBackingBaseAddress = (void *) -1;
        gBackingFile = -1;
        goto done;
    }
    pFreeMemory->startBoundary = gBackingBaseAddress;
    pFreeMemory->memSize = BACKING_FILE_SIZE;
    pFreeMemory->next = NULL;
    result = TRUE;
#endif // MMAP_IGNORES_HINT

done:
    SYNCLeaveCriticalSection(&virtual_critsec, TRUE);
    return result;
}
#endif // RESERVE_FROM_BACKING_FILE

/*++
Function:
  VirtualAlloc

Note:
  MEM_TOP_DOWN, MEM_PHYSICAL, MEM_WRITE_WATCH are not supported.
  Unsupported flags are ignored.
  
  Page size on i386 is set to 4k.

See MSDN doc.
--*/
LPVOID
PALAPI
VirtualAlloc(
         IN LPVOID lpAddress,       /* Region to reserve or commit */
         IN SIZE_T dwSize,          /* Size of Region */
         IN DWORD flAllocationType, /* Type of allocation */
         IN DWORD flProtect)        /* Type of access protection */
{
    LPVOID  pRetVal       = NULL;

    ENTRY("VirtualAlloc(lpAddress=%p, dwSize=%u, flAllocationType=%#x, \
          flProtect=%#x)\n", lpAddress, dwSize, flAllocationType, flProtect);
    
    /* Test for un-supported flags. */
    if ( ( flAllocationType & ~( MEM_COMMIT | MEM_RESERVE | MEM_TOP_DOWN ) ) != 0 )
    {
        ASSERT( "flAllocationType can be one, or any combination of MEM_COMMIT, \
               MEM_RESERVE, or MEM_TOP_DOWN.\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto done;
    }
    if ( VIRTUALContainsInvalidProtectionFlags( flProtect ) )
    {
        ASSERT( "flProtect can be one of PAGE_READONLY, PAGE_READWRITE, or \
               PAGE_EXECUTE_READWRITE || PAGE_NOACCESS. \n" );

        SetLastError( ERROR_INVALID_PARAMETER );
        goto done;
    }
    if ( flAllocationType & MEM_TOP_DOWN )
    {
        WARN( "Ignoring the allocation flag MEM_TOP_DOWN.\n" );
    }
    
#if RESERVE_FROM_BACKING_FILE
    // Make sure we have memory to map before we try to use it.
    VIRTUALGetBackingFile();
#endif  // RESERVE_FROM_BACKING_FILE

    if ( flAllocationType & MEM_RESERVE ) 
    {
        SYNCEnterCriticalSection( &virtual_critsec , TRUE);
        pRetVal = VIRTUALReserveMemory( lpAddress, dwSize, flAllocationType, flProtect );
        SYNCLeaveCriticalSection( &virtual_critsec , TRUE);
        if ( !pRetVal )
        {
            /* Error messages are already displayed, just leave. */
            goto done;
        }
    }

    if ( flAllocationType & MEM_COMMIT )
    {
        SYNCEnterCriticalSection( &virtual_critsec , TRUE);
        if ( pRetVal != NULL )
        {
            /* We are reserving and committing. */
            pRetVal = VIRTUALCommitMemory( pRetVal, dwSize, 
                                    flAllocationType, flProtect );    
        }
        else
        {
            /* Just a commit. */
            pRetVal = VIRTUALCommitMemory( lpAddress, dwSize, 
                                    flAllocationType, flProtect );
        }
        SYNCLeaveCriticalSection( &virtual_critsec , TRUE);
    }                      
    
done:
#if defined _DEBUG
    VIRTUALDisplayList();
#endif
    LOGEXIT("VirtualAlloc returning %p\n ", pRetVal  );
    return pRetVal;
}


/*++
Function:
  VirtualFree

See MSDN doc.
--*/
BOOL
PALAPI
VirtualFree(
        IN LPVOID lpAddress,    /* Address of region. */
        IN SIZE_T dwSize,       /* Size of region. */
        IN DWORD dwFreeType )   /* Operation type. */
{
    BOOL bRetVal = TRUE;

    ENTRY("VirtualFree(lpAddress=%p, dwSize=%u, dwFreeType=%#x)\n",
          lpAddress, dwSize, dwFreeType);

    SYNCEnterCriticalSection( &virtual_critsec , TRUE);

    /* Sanity Checks. */
    if ( !lpAddress )
    {
        ERROR( "lpAddress cannot be NULL. You must specify the base address of\
               regions to be de-committed. \n" );
        SetLastError( ERROR_INVALID_ADDRESS );
        bRetVal = FALSE;
        goto VirtualFreeExit;
    }

    if ( !( dwFreeType & MEM_RELEASE ) && !(dwFreeType & MEM_DECOMMIT ) )
    {
        ERROR( "dwFreeType must contain one of the following: \
               MEM_RELEASE or MEM_DECOMMIT\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        bRetVal = FALSE;
        goto VirtualFreeExit;
    }
    /* You cannot release and decommit in one call.*/
    if ( dwFreeType & MEM_RELEASE && dwFreeType & MEM_DECOMMIT )
    {
        ERROR( "MEM_RELEASE cannot be combined with MEM_DECOMMIT.\n" );
        bRetVal = FALSE;
        goto VirtualFreeExit;
    }

    if ( dwFreeType & MEM_DECOMMIT )
    {
        UINT StartBoundary  = 0;
        UINT MemSize        = 0;

        if ( dwSize == 0 )
        {
            ERROR( "dwSize cannot be 0. \n" );
            SetLastError( ERROR_INVALID_PARAMETER );
            bRetVal = FALSE;
            goto VirtualFreeExit;
        }
        /* 
         * A two byte range straddling 2 pages caues both pages to be either
         * released or decommitted. So round the dwSize up to the next page 
         * boundary and round the lpAddress down to the next page boundary.
         */
        MemSize = (((UINT)(dwSize) + ((UINT)(lpAddress) & PAGE_MASK) 
                    + PAGE_MASK) & ~PAGE_MASK);

        StartBoundary = (UINT)lpAddress & ~PAGE_MASK;

        TRACE( "Un-committing the following page(s) %d to %d.\n", 
               StartBoundary, MemSize );
        // Explicitly calling mmap instead of mprotect here makes it
        // that much more clear to the operating system that we no
        // longer need these pages.
#if RESERVE_FROM_BACKING_FILE
        if ( mmap( (LPVOID)StartBoundary, MemSize, PROT_NONE,
                   MAP_FIXED | MAP_PRIVATE, gBackingFile,
                   (char *) StartBoundary - (char *) gBackingBaseAddress ) !=
             MAP_FAILED )
#else   // RESERVE_FROM_BACKING_FILE
        if ( mmap( (LPVOID)StartBoundary, MemSize, PROT_NONE,
                   MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0 ) != MAP_FAILED )
#endif  // RESERVE_FROM_BACKING_FILE
        {
            PCMI pUnCommittedMem;
            
#if MMAP_ANON_IGNORES_PROTECTION
            if (mprotect((LPVOID) StartBoundary, MemSize, PROT_NONE) != 0)
            {
                ASSERT("mprotect failed to protect the region!\n");
                SetLastError(ERROR_INTERNAL_ERROR);
                munmap((LPVOID) StartBoundary, MemSize);
                bRetVal = FALSE;
                goto VirtualFreeExit;
            }
#endif  // MMAP_ANON_IGNORES_PROTECTION

            pUnCommittedMem = VIRTUALFindRegionInformation( StartBoundary );
            if ( pUnCommittedMem )
            {
                UINT index = 0;
                UINT nNumOfPagesToChange = 0;

                /* We can now commit this memory by calling VirtualAlloc().*/
                index = StartBoundary - pUnCommittedMem->startBoundary == 0 ? 
                0 : ( StartBoundary - pUnCommittedMem->startBoundary ) / PAGE_MASK;
            
                nNumOfPagesToChange = MemSize / PAGE_SIZE;
                VIRTUALSetAllocState( MEM_RESERVE, index, 
                                      nNumOfPagesToChange, pUnCommittedMem ); 

                goto VirtualFreeExit;    
            }
            else
            {
                ASSERT( "Unable to locate the region information.\n" );
                SetLastError( ERROR_INTERNAL_ERROR );
                bRetVal = FALSE;
                goto VirtualFreeExit;
            }
        }
        else
        {
            ASSERT( "mmap() returned an abnormal value.\n" );
            bRetVal = FALSE;
            SetLastError( ERROR_INTERNAL_ERROR );
            goto VirtualFreeExit;
        }
    }
    
    if ( dwFreeType & MEM_RELEASE )
    {
        PCMI pMemoryToBeReleased = 
            VIRTUALFindRegionInformation( (UINT)lpAddress );
        
        if ( !pMemoryToBeReleased )
        {
            ERROR( "lpAddress must be the base address returned by VirtualAlloc.\n" );
            SetLastError( ERROR_INVALID_ADDRESS );
            bRetVal = FALSE;
            goto VirtualFreeExit;
        }
        if ( dwSize != 0 )
        {
            ERROR( "dwSize must be 0 if you are releasing the memory.\n" );
            SetLastError( ERROR_INVALID_PARAMETER );
            bRetVal = FALSE;
            goto VirtualFreeExit;
        }

        TRACE( "Releasing the following memory %d to %d.\n", 
               pMemoryToBeReleased->startBoundary, pMemoryToBeReleased->memSize );
        
#if MMAP_IGNORES_HINT
        if (mmap((void *) pMemoryToBeReleased->startBoundary,
                 pMemoryToBeReleased->memSize, PROT_NONE,
                 MAP_FIXED | MAP_PRIVATE, gBackingFile,
                 (char *) pMemoryToBeReleased->startBoundary -
                 (char *) gBackingBaseAddress) != MAP_FAILED)
#else   // MMAP_IGNORES_HINT
        if ( munmap( (LPVOID)pMemoryToBeReleased->startBoundary, 
                     pMemoryToBeReleased->memSize ) == 0 )
#endif  // MMAP_IGNORES_HINT
        {
            if ( VIRTUALReleaseMemory( pMemoryToBeReleased ) == FALSE )
            {
                ASSERT( "Unable to remove the PCMI entry from the list.\n" );
                SetLastError( ERROR_INTERNAL_ERROR );
                bRetVal = FALSE;
                goto VirtualFreeExit;
            }
            pMemoryToBeReleased = NULL;
        }
        else
        {
#if MMAP_IGNORES_HINT
            ASSERT("Unable to remap the memory onto the backing file; "
                   "error is %d.\n", errno);
#else   // MMAP_IGNORES_HINT
            ASSERT( "Unable to unmap the memory, munmap() returned "
                   "an abnormal value.\n" );
#endif  // MMAP_IGNORES_HINT
            SetLastError( ERROR_INTERNAL_ERROR );
            bRetVal = FALSE;
            goto VirtualFreeExit;
        }
    }

VirtualFreeExit:
    SYNCLeaveCriticalSection( &virtual_critsec, TRUE );
    LOGEXIT( "VirtualFree returning %s.\n", bRetVal == TRUE ? "TRUE" : "FALSE" );
    return bRetVal;
}


/*++
Function:
  VirtualProtect

See MSDN doc.
--*/
BOOL
PALAPI
VirtualProtect(
           IN LPVOID lpAddress,
           IN SIZE_T dwSize,
           IN DWORD flNewProtect,
           OUT PDWORD lpflOldProtect)
{
    BOOL bRetVal = FALSE;
    PCMI pEntry = NULL;
    UINT MemSize = 0;
    UINT StartBoundary = 0;
    UINT Index = 0;
    UINT NumberOfPagesToChange = 0;
    UINT OffSet = 0;

    ENTRY("VirtualProtect(lpAddress=%p, dwSize=%u, flNewProtect=%#x, "
          "flOldProtect=%p)\n",
          lpAddress, dwSize, flNewProtect, lpflOldProtect);

    SYNCEnterCriticalSection( &virtual_critsec , TRUE);
    
    StartBoundary = (UINT)lpAddress & ~PAGE_MASK;
    MemSize = (((UINT)(dwSize) + ((UINT)(lpAddress) & PAGE_MASK)
                + PAGE_MASK) & ~PAGE_MASK);

    if ( VIRTUALContainsInvalidProtectionFlags( flNewProtect ) )
    {
        ASSERT( "flProtect can be one of PAGE_NOACCESS, PAGE_READONLY, "
               "PAGE_READWRITE, PAGE_EXECUTE, PAGE_EXECUTE_READ "
               ", or PAGE_EXECUTE_READWRITE. \n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto ExitVirtualProtect;
    }

    if ( !lpflOldProtect ||
         IsBadWritePtr( lpflOldProtect, sizeof( *lpflOldProtect ) ) )
    {
        ERROR( "lpflOldProtect was invalid.\n" );
        SetLastError( ERROR_NOACCESS );
        goto ExitVirtualProtect;
    }

    pEntry = VIRTUALFindRegionInformation( StartBoundary );
    if ( NULL != pEntry )
    {
        /* See if the pages are committed. */
        Index = OffSet = StartBoundary - pEntry->startBoundary == 0 ?
             0 : ( StartBoundary - pEntry->startBoundary ) / PAGE_MASK;
        NumberOfPagesToChange = MemSize / PAGE_SIZE;
        
        TRACE( "Number of pages to check %d, starting page %d \n",
               NumberOfPagesToChange, Index );
    
        for ( ; Index < NumberOfPagesToChange; Index++  )
        {
            if ( !VIRTUALIsPageCommitted( Index, pEntry ) )
            {     
                ERROR( "You can only change the protection attributes"
                       " on committed memory.\n" )
                SetLastError( ERROR_INVALID_ADDRESS );
                goto ExitVirtualProtect;
            }
        }
    }

    if ( 0 == mprotect( (LPVOID)StartBoundary, MemSize, 
                   W32toUnixAccessControl( flNewProtect ) ) )
    {
        /* Reset the access protection. */
        TRACE( "Number of pages to change %d, starting page %d \n",
               NumberOfPagesToChange, OffSet );
        /*
         * Set the old protection flags. We only use the first flag, so
         * if there were several regions with each with different flags only the
         * first region's protection flag will be returned.
         */
        if ( pEntry )
        {
            *lpflOldProtect =
                VIRTUALConvertVirtualFlags( pEntry->pProtectionState[ OffSet ] );
            
            memset( pEntry->pProtectionState + OffSet, 
                    VIRTUALConvertWinFlags( flNewProtect ),
                    NumberOfPagesToChange );
        }
        else
        {
            *lpflOldProtect = PAGE_EXECUTE_READWRITE;
        }
        bRetVal = TRUE;
    }
    else
    {
        ERROR( "%s\n", strerror( errno ) );
        if ( errno == EINVAL )
        {
            SetLastError( ERROR_INVALID_ADDRESS );
        }
        else if ( errno == EACCES )
        {
            SetLastError( ERROR_INVALID_ACCESS );
        }
    }
ExitVirtualProtect:
    SYNCLeaveCriticalSection( &virtual_critsec , TRUE);

#if defined _DEBUG
    VIRTUALDisplayList();
#endif
    LOGEXIT( "VirtualProtect returning %s.\n", bRetVal == TRUE ? "TRUE" : "FALSE" );
    return bRetVal;
}

#if HAVE_VM_ALLOCATE
static void VM_ALLOCATE_VirtualQuery(LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer)
{
    kern_return_t MachRet;
    vm_address_t vm_address;
    vm_size_t vm_size;
    struct vm_region_basic_info info;
    mach_msg_type_number_t infoCnt;
    mach_port_t object_name;

    vm_address = (vm_address_t)lpAddress;
    infoCnt = VM_REGION_BASIC_INFO_COUNT;
    MachRet = vm_region(mach_task_self(),
                        &vm_address,
                        &vm_size,
                        VM_REGION_BASIC_INFO,
                        (vm_region_info_t)&info,
                        &infoCnt,
                        &object_name);
    if (MachRet != KERN_SUCCESS) {
        return;
    }

    if (vm_address > (vm_address_t)lpAddress) {
        /* lpAddress was pointing into a free region */
        return;
    }

    lpBuffer->BaseAddress = (PVOID)vm_address;
    lpBuffer->AllocationProtect = VM_PROT_NONE;
    lpBuffer->RegionSize = (SIZE_T)vm_size;
    lpBuffer->State = MEM_RESERVE;
    lpBuffer->Protect = lpBuffer->AllocationProtect;

    /* Note that if a mapped region and a private region are adjacent, this
        will return MEM_PRIVATE but the region size will span
        both the mapped and private regions. */
    lpBuffer->Type = MEM_PRIVATE;
}
#endif // HAVE_VM_ALLOCATE

/*++
Function:
  VirtualQuery

See MSDN doc.
--*/
SIZE_T
PALAPI
VirtualQuery(
         IN LPCVOID lpAddress,
         OUT PMEMORY_BASIC_INFORMATION lpBuffer,
         IN SIZE_T dwLength)
{
    PCMI pEntry = NULL;
    UINT StartBoundary = 0;

    ENTRY("VirtualQuery(lpAddress=%p, lpBuffer=%p, dwLength=%u)\n",
          lpAddress, lpBuffer, dwLength);

    SYNCEnterCriticalSection( &virtual_critsec , TRUE);

    if ( !lpBuffer || IsBadWritePtr( lpBuffer, sizeof( *lpBuffer ) ) )
    {
        ERROR( "lpBuffer has to be a valid pointer.\n" );
        SetLastError( ERROR_NOACCESS );
        goto ExitVirtualQuery;
    }
    if ( dwLength < sizeof( *lpBuffer ) )
    {
        ERROR( "dwLength cannot be smaller then the size of *lpBuffer.\n" );
        SetLastError( ERROR_BAD_LENGTH );
        goto ExitVirtualQuery;
    }

    StartBoundary = (UINT)lpAddress & ~PAGE_MASK;

#if MMAP_IGNORES_HINT
    // Make sure we have memory to map before we try to query it.
    VIRTUALGetBackingFile();

    // If we're suballocating, claim that any memory that isn't in our
    // suballocated block is already allocated. This keeps callers from
    // using these results to try to allocate those blocks and failing.
    if (StartBoundary < (UINT) gBackingBaseAddress ||
        StartBoundary >= (UINT) gBackingBaseAddress + BACKING_FILE_SIZE)
    {
        if (StartBoundary < (UINT) gBackingBaseAddress)
        {
            lpBuffer->RegionSize = (UINT) gBackingBaseAddress - StartBoundary;
        }
        else
        {
            lpBuffer->RegionSize = -StartBoundary;
        }
        lpBuffer->BaseAddress = (void *) StartBoundary;
        lpBuffer->State = MEM_COMMIT;
        lpBuffer->Type = MEM_MAPPED;
        lpBuffer->AllocationProtect = 0;
        lpBuffer->Protect = 0;
        goto ExitVirtualQuery;
    }
#endif  // MMAP_IGNORES_HINT

    /* Find the entry. */
    pEntry = VIRTUALFindRegionInformation( StartBoundary );

    if ( !pEntry )
    {
        /* Can't find a match, or no list present. */
        /* Next, looking for this region in file maps */
        lpBuffer->RegionSize = 
                MAPGetRegionSize((LPVOID)StartBoundary);
        lpBuffer->BaseAddress = (LPVOID)StartBoundary;
        lpBuffer->AllocationProtect = 0;
        lpBuffer->Protect = 0;
        if(lpBuffer->RegionSize>0)
        {
            lpBuffer->State = MEM_COMMIT;
            lpBuffer->Type = MEM_MAPPED;
        }
        else
        { 
            lpBuffer->State = MEM_FREE;
            lpBuffer->Type = -1;
#if HAVE_VM_ALLOCATE
            VM_ALLOCATE_VirtualQuery(lpAddress, lpBuffer);
#endif
        }
    }
    else
    {
        /* Starting page. */
        UINT Index = ( StartBoundary - pEntry->startBoundary ) / PAGE_SIZE;

        /* Attributes to check for. */
        BYTE AccessProtection = pEntry->pProtectionState[ Index ];
        UINT AllocationType = VIRTUALGetAllocationType( Index, pEntry );
        UINT RegionSize = 0;

        TRACE( "Index = %d, Number of Pages = %d. \n",
               Index, pEntry->memSize / PAGE_SIZE );

        while ( VIRTUALGetAllocationType( Index, pEntry ) == AllocationType &&
                pEntry->pProtectionState[ Index ] == AccessProtection &&
                Index < pEntry->memSize / PAGE_SIZE )
        {
            RegionSize += PAGE_SIZE;
            Index++;
        }

        TRACE( "RegionSize = %d.\n", RegionSize );

        /* Fill the structure.*/
        lpBuffer->AllocationProtect = pEntry->accessProtection;
        lpBuffer->BaseAddress = (LPVOID)StartBoundary;

        lpBuffer->Protect = AllocationType == MEM_COMMIT ?
            VIRTUALConvertVirtualFlags( AccessProtection ) : 0;

        lpBuffer->RegionSize = RegionSize;
        lpBuffer->State =
            ( AllocationType == MEM_COMMIT ? MEM_COMMIT : MEM_RESERVE );
        WARN( "Ignoring lpBuffer->Type. \n" );
    }

ExitVirtualQuery:

    SYNCLeaveCriticalSection( &virtual_critsec , TRUE);
    LOGEXIT( "VirtualQuery returning %d.\n", sizeof( *lpBuffer ) );
    return sizeof( *lpBuffer );
}

/*++
Function:
  IsBadReadPtr

(see MSDN)
--*/
BOOL
PALAPI
IsBadReadPtr(
         IN CONST VOID *lp,
         IN UINT_PTR ucb)
{
    volatile BOOL retval = FALSE;
    volatile LPSTR ptr = (LPSTR) lp;
    LPSTR end;

    ENTRY("IsBadReadPtr(lp=%p, ucb=%u)\n", lp, ucb);

    /* If block size is 0, return FALSE */
    if(ucb == 0)
    {
        TRACE("Block size is zero.\n");
        goto done;
    }

    /* If block extends beyond 0xFFFFFFFF (or 64bit -1), return TRUE */
    if( (((LPVOID)-1) - lp + 1) < ucb )
    {
        TRACE("Block size extends beyond highest possible address!\n");
        retval = TRUE;
        goto done;
    }

    if(!lp)
    {
        TRACE("NULL pointer is not readable.\n");
        retval = TRUE;
        goto done;
    }

    /* Get address of last byte in the range */
    end = (LPSTR)(lp + ucb - 1);

    /* Page-align the starting address in order to check the first byte of each
       page in the range (otherwise we need a special case for the last page) */
    ptr = (LPSTR)((DWORD)lp & (~PAGE_MASK));

    if(!ptr)
    {
        TRACE("First page in range starts at 0x0\n");
        retval = TRUE;
        goto done;
    }

    PAL_TRY
    {
        char c;
        /* Check one byte per page */
        while( ptr<=end )
        {
            c = *(volatile CHAR *)ptr;

            /* Avoid wrap-around */
            if(((LPSTR)-1)-PAGE_SIZE < ptr)
            {
                break;
            }

            ptr += PAGE_SIZE;
        }
    }
    PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        TRACE("Caught exception : address %p is not readable.\n", ptr);
        retval = TRUE;
    }
    PAL_ENDTRY

    if(!retval)
    {
        TRACE("Block %p[%u] is fully readable\n", lp, ucb);
    }
done:
    LOGEXIT("IsBadReadPtr returning BOOL %d\n", retval);
    return retval;
}


/*++
Function:
  IsBadWritePtr

(see MSDN)
--*/
BOOL
PALAPI
IsBadWritePtr(
          IN LPVOID lp,
          IN UINT_PTR ucb)
{
    volatile BOOL retval = FALSE;
    LPSTR end;
    volatile LPSTR ptr;

    ENTRY("IsBadWritePtr(lp=%p, ucb=%u)\n", lp, ucb);

    /* If block size is 0, return FALSE */
    if(ucb == 0)
    {
        TRACE("Block size is zero.\n");
        goto done;
    }

    /* If block extends beyond 0xFFFFFFFF (or 64bit -1), return TRUE */
    if( (((LPVOID)-1) - lp + 1) < ucb )
    {
        TRACE("Block size extends beyond highest possible address!\n");
        retval = TRUE;
        goto done;
    }

    if(!lp)
    {
        TRACE("NULL pointer is not writable.\n");
        retval = TRUE;
        goto done;
    }

    /* Get address of last byte in the range */
    end = (LPSTR)(lp + ucb - 1 );
    /* Page-align the starting address in order to check the first byte of each
       page in the range (otherwise we need a special case for the last page) */
    ptr = (LPSTR)((DWORD)lp & (~PAGE_MASK));
    
    if(!ptr)
    {
        TRACE("First page in range starts at 0x0\n");
        retval = TRUE;
        goto done;
    }

    PAL_TRY
    {
        volatile char c;
        volatile LPSTR ptr = (LPSTR)lp;

        /* By ORing the pointer's contents with zero, we can write to it
           without changing it, so we don't need read access to it. in order to
           preserve its value. (c is volatile to prevent the compiler from
           optimizing out the operation) */
        c = '\0';

        /* Check one byte per page */
        while( ptr<=end )
        {
            *ptr |= c;
            /* Avoid wrap-around */
            if(((LPSTR)-1)-PAGE_SIZE < ptr)
            {
                break;
            }

            ptr += PAGE_SIZE;
        }
    }
    PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        TRACE("Caught exception : address %p is not writable.\n", ptr);
        retval = TRUE;
    }
    PAL_ENDTRY

    if(!retval)
    {
        TRACE("Block %p[%u] is fully writable\n", lp, ucb);
    }
done:
    LOGEXIT("IsBadWritePtr returning BOOL %d\n", retval);
    return retval;
}


/*++
Function:
  IsBadCodePtr

(see MSDN)
--*/
BOOL
PALAPI
IsBadCodePtr(
         FARPROC lpfn)
{
    volatile BOOL retval = FALSE;
    volatile LPSTR ptr;

    ENTRY("IsBadCodePtr(lpfn=%p)\n", lpfn);

    if(!lpfn)
    {
        TRACE("NULL pointer is not readable.\n");
        retval = TRUE;
        goto done;
    }

    ptr = (LPSTR)lpfn;
    /*As per MSDN we are checking if the given memory
     * location has read access.*/
    PAL_TRY
    {
        char c;
        c = *ptr;
    }
    PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        TRACE("Caught exception : address %p is not readable.\n", ptr);
        retval = TRUE;
    }
    PAL_ENDTRY

done:
    LOGEXIT("IsBadCodePtr returning BOOL %d\n", retval);
    return retval;

}

