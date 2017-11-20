// ==++==
// 
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==
//
// util.hpp
//
// Miscellaneous useful functions
//
#ifndef _H_UTIL
#define _H_UTIL

void FatalOutOfMemoryError(void);
#define ALLOC_FAILURE_ACTION FatalOutOfMemoryError();

#include "utilcode.h"


//========================================================================
// More convenient names for integer types of a guaranteed size.
//========================================================================


typedef __int8              I1;
typedef unsigned __int8     U1;
typedef __int16             I2;
typedef unsigned __int16    U2;
typedef __int32             I4;
typedef unsigned __int32    U4;
typedef __int64             I8;
typedef unsigned __int64    U8;
typedef float               R4;
typedef double              R8;


// Based on whether we are running on a Uniprocessor or Multiprocessor machine,
// set up a bunch of services that are correct / efficient.  These are initialized
// in InitFastInterlockOps().

typedef void   (__fastcall *BitFieldOps) (DWORD volatile *Target, const int Bits);
typedef LONG   (__fastcall *XchgOps)     (LONG volatile *Target, LONG Value);
typedef PVOID   (__fastcall *XchgOpsPtr)    (PVOID volatile *Target, PVOID Value);
typedef LONG    (__fastcall *CmpXchgOps)    (LONG volatile *Destination, LONG Exchange, LONG Comperand);
typedef PVOID   (__fastcall *CmpXchgOpsPtr) (PVOID volatile *Destination, PVOID Exchange, PVOID Comperand);
typedef LONG   (__fastcall *XchngAddOps) (LONG volatile *Target, LONG Value);
typedef LONG   (__fastcall *IncDecOps)   (LONG volatile *Target);
typedef UINT64  (__fastcall *IncDecLongOps) (UINT64 volatile *Target);


extern BitFieldOps FastInterlockOr;
extern BitFieldOps FastInterlockAnd;

extern XchgOps     FastInterlockExchange;
extern XchgOpsPtr       FastInterlockExchangePointer;
extern CmpXchgOps  FastInterlockCompareExchange;
extern CmpXchgOpsPtr    FastInterlockCompareExchangePointer;
extern XchngAddOps FastInterlockExchangeAdd;

// So that we can run on Win95 386, which lacks the xadd instruction, the following
// services return zero or a positive or negative number only.  Do not rely on
// values -- only on the sign of the return.
extern IncDecOps   FastInterlockIncrement;
extern IncDecOps   FastInterlockDecrement;
extern IncDecLongOps FastInterlockIncrementLong;
extern IncDecLongOps FastInterlockDecrementLong;



// Copied from malloc.h: don't want to bring in the whole header file.
void * __cdecl _alloca(size_t);

// Function to parse apart a command line and return the 
// arguments just like argv and argc
LPWSTR* CommandLineToArgvW(LPWSTR lpCmdLine, DWORD *pNumArgs);
#define ISWWHITE(x) ((x)==L' ' || (x)==L'\t' || (x)==L'\n' || (x)==L'\r' )


BOOL inline FitsInI1(__int64 val)
{
    return val == (__int64)(__int8)val;
}

BOOL inline FitsInI2(__int64 val)
{
    return val == (__int64)(__int16)val;
}

BOOL inline FitsInI4(__int64 val)
{
    return val == (__int64)(__int32)val;
}

//************************************************************************
// EEQuickBytes
//
// A wrapper of CQuickBytes that fails fast if we don't get our memory.
//
// GetLastError() can be used to determine the failure reason.  In some
// cases, a stack overflow is the cause.
//
//
class EEQuickBytes : public CQuickBytes {
public:
    void* Alloc(SIZE_T iItems);
};


// returns FALSE if overflow: otherwise, (*pa) is incremented by b
BOOL inline SafeAddUINT16(UINT16 *pa, ULONG b)
{
    UINT16 a = *pa;
    if ( ((UINT16)b) != b )
    {
        return FALSE;
    }
    if ( ((UINT16)(a + ((UINT16)b))) < a)
    {
        return FALSE;
    }
    (*pa) += (UINT16)b;
    return TRUE;
}


// returns FALSE if overflow: otherwise, (*pa) is incremented by b
BOOL inline SafeAddUINT32(UINT32 *pa, UINT32 b)
{
    UINT32 a = *pa;
    if ( ((UINT32)(a + b)) < a)
    {
        return FALSE;
    }
    (*pa) += b;
    return TRUE;
}

// returns FALSE if overflow: otherwise, (*pa) is multiplied by b
BOOL inline SafeMulSIZE_T(SIZE_T *pa, SIZE_T b)
{
#ifdef _DEBUG
    {
        //Make sure SIZE_T is unsigned
        SIZE_T m = ((SIZE_T)(-1));
        SIZE_T z = 0;
        _ASSERTE(m > z);
    }
#endif


    SIZE_T a = *pa;
    const SIZE_T m = ((SIZE_T)(-1));
    if ( (m / b) < a )
    {
        return FALSE;
    }
    (*pa) *= b;
    return TRUE;
}



//************************************************************************
// CQuickHeap
//
// A fast non-multithread-safe heap for short term use.
// Destroying the heap frees all blocks allocated from the heap.
// Blocks cannot be freed individually.
//
// The heap uses COM+ exceptions to report errors.
//
// The heap does not use any internal synchronization so it is not
// multithreadsafe.
//************************************************************************
class CQuickHeap
{
    public:
        CQuickHeap(); 
        ~CQuickHeap();

        //---------------------------------------------------------------
        // Allocates a block of "sz" bytes. If there's not enough
        // memory, throws an OutOfMemoryError.
        //---------------------------------------------------------------
        LPVOID Alloc(UINT sz);


    private:
        enum {
#ifdef _DEBUG
            kBlockSize = 24
#else
            kBlockSize = 1024
#endif
        };

        // The QuickHeap allocates QuickBlock's as needed and chains
        // them in a single-linked list. Most QuickBlocks have a size
        // of kBlockSize bytes (not counting m_next), and individual
        // allocation requests are suballocated from them.
        // Allocation requests of greater than kBlockSize are satisfied
        // by allocating a special big QuickBlock of the right size.
        struct QuickBlock
        {
            QuickBlock  *m_next;
            BYTE         m_bytes[1];
        };


        // Linked list of QuickBlock's.
        QuickBlock      *m_pFirstQuickBlock;

        // Offset to next available byte in m_pFirstQuickBlock.
        LPBYTE           m_pNextFree;

        // Linked list of big QuickBlock's
        QuickBlock      *m_pFirstBigQuickBlock;

};

//======================================================================
// String Helpers
//
//
//
ULONG StringHashValueW(LPWSTR wzString);
ULONG StringHashValueA(LPCSTR szString);


LPCVOID ReserveAlignedMemory(DWORD dwAlign, DWORD dwSize);

void PrintToStdOutA(const char *pszString);
void PrintToStdOutW(const WCHAR *pwzString);
void PrintToStdErrA(const char *pszString);
void PrintToStdErrW(const WCHAR *pwzString);
void NPrintToStdOutA(const char *pszString, size_t nbytes);
void NPrintToStdOutW(const WCHAR *pwzString, size_t nchars);
void NPrintToStdErrA(const char *pszString, size_t nbytes);
void NPrintToStdErrW(const WCHAR *pwzString, size_t nchars);


//=====================================================================
// Function for formatted text output to the debugger
//
//
void __cdecl VMDebugOutputA(LPSTR format, ...);
void __cdecl VMDebugOutputW(LPWSTR format, ...);


//=====================================================================
// Displays the messaage box or logs the message, corresponding to the last COM+ error occured
void VMDumpCOMErrors(HRESULT hrErr);

//=====================================================================
// Switches on different code paths in checked builds for code coverage.
#ifdef _DEBUG

inline WORD GetDayOfWeek()
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    return st.wDayOfWeek;
}

#define DAYOFWEEKDEBUGHACKSENABLED TRUE

#define MonDebugHacksOn() (DAYOFWEEKDEBUGHACKSENABLED && 1 == GetDayOfWeek())
#define TueDebugHacksOn() (DAYOFWEEKDEBUGHACKSENABLED && 2 == GetDayOfWeek())
#define WedDebugHacksOn() (DAYOFWEEKDEBUGHACKSENABLED && 3 == GetDayOfWeek())
#define ThuDebugHacksOn() (DAYOFWEEKDEBUGHACKSENABLED && 4 == GetDayOfWeek())
#define FriDebugHacksOn() (DAYOFWEEKDEBUGHACKSENABLED && 5 == GetDayOfWeek())


#else

#define MonDebugHacksOn() FALSE
#define TueDebugHacksOn() FALSE
#define WedDebugHacksOn() FALSE
#define ThuDebugHacksOn() FALSE
#define FriDebugHacksOn() FALSE


#endif

#include "nativevaraccessors.h"

#ifdef _DEBUG
#define INDEBUG(x)          x
#define INDEBUG_COMMA(x)    x,
#define COMMA_INDEBUG(x)    ,x
#else
#define INDEBUG(x)
#define INDEBUG_COMMA(x)
#define COMMA_INDEBUG(x)
#endif



LPWSTR *SegmentCommandLine(LPCWSTR lpCmdLine, DWORD *pNumArgs);


#define AUTO_COOPERATIVE_GC() AutoCooperativeGC __autoGC;
#define MAYBE_AUTO_COOPERATIVE_GC(__flag) AutoCooperativeGC __autoGC(__flag);

#define AUTO_PREEMPTIVE_GC() AutoPreemptiveGC __autoGC;
#define MAYBE_AUTO_PREEMPTIVE_GC(__flag) AutoPreemptiveGC __autoGC(__flag);

typedef BOOL (*FnLockOwner)(LPVOID);
struct LockOwner
{
    LPVOID lock;
    FnLockOwner lockOwnerFunc;
};

#endif /* _H_UTIL */

