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
#include "common.h"

/*******************************************************************/
/* The folowing routines are useful to have around so that they can
   be called from the debugger 
*/
/*******************************************************************/
#include "stdlib.h"

bool isMemoryReadable(const void* start, unsigned len)
{
    void* buff = _alloca(len);
    return(ReadProcessMemory(GetCurrentProcess(), start, buff, len, 0) != 0);
}


/*******************************************************************/
MethodDesc* IP2MD(ULONG_PTR IP) {
    return(IP2MethodDesc((SLOT)IP));
}

/*******************************************************************/
MethodDesc* Entry2MethodDescMD(BYTE* entry) {
    return(Entry2MethodDesc((BYTE*) entry, 0));
}

/*******************************************************************/
/* if addr is a valid method table, return a poitner to it */
MethodTable* AsMethodTable(size_t addr) {
    MethodTable* pMT = (MethodTable*) addr;
    if (!isMemoryReadable(pMT, sizeof(MethodTable)))
        return(0);

    EEClass* cls = pMT->GetClass();
    if (!isMemoryReadable(cls, sizeof(EEClass)))
        return(0);

    if (cls->GetMethodTable() != pMT)
        return(0);

    return(pMT);
}

/*******************************************************************/
/* if addr is a valid method table, return a pointer to it */
MethodDesc* AsMethodDesc(size_t addr) {
    MethodDesc* pMD = (MethodDesc*) addr;

    if (!isMemoryReadable(pMD, sizeof(MethodDesc)))
        return(0);

    MethodDescChunk *chunk = MethodDescChunk::RecoverChunk(pMD);
    if (!isMemoryReadable(chunk, sizeof(MethodDescChunk)))
        return(0);

    MethodTable* pMT = chunk->GetMethodTable();
    if (AsMethodTable((size_t) pMT) == 0)
        return(0);

    return(pMD);
}

/*******************************************************************/
/* check to see if 'retAddr' is a valid return address (it points to
   someplace that has a 'call' right before it), If possible it is 
   it returns the address that was called in whereCalled */

bool isRetAddr(size_t retAddr, size_t* whereCalled) 
{
        // don't waste time values clearly out of range
        if (retAddr < (size_t)BOT_MEMORY || retAddr > (size_t)TOP_MEMORY)   
	    return false;

        BYTE* spot = (BYTE*) retAddr;
        if (!isMemoryReadable(&spot[-7], 7))
            return(false);

            // Note this is possible to be spoofed, but pretty unlikely
        *whereCalled = 0;
            // call XXXXXXXX
        if (spot[-5] == 0xE8) {         
            *whereCalled = *((int*) (retAddr-4)) + retAddr; 
            return(true);
            }

            // call [XXXXXXXX]
        if (spot[-6] == 0xFF && (spot[-5] == 025))  {
            if (isMemoryReadable(*((size_t**)(retAddr-4)),4)) {
                *whereCalled = **((size_t**) (retAddr-4));
                return(true);
            }
        }

            // call [REG+XX]
        if (spot[-3] == 0xFF && (spot[-2] & ~7) == 0120 && (spot[-2] & 7) != 4) 
            return(true);
        if (spot[-4] == 0xFF && spot[-3] == 0124)       // call [ESP+XX]
            return(true);

            // call [REG+XXXX]
        if (spot[-6] == 0xFF && (spot[-5] & ~7) == 0220 && (spot[-5] & 7) != 4) 
            return(true);

        if (spot[-7] == 0xFF && spot[-6] == 0224)       // call [ESP+XXXX]
            return(true);

            // call [REG]
        if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0020 && (spot[-1] & 7) != 4 && (spot[-1] & 7) != 5)
            return(true);

            // call REG
        if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0320 && (spot[-1] & 7) != 4)
            return(true);

            // There are other cases, but I don't believe they are used.
    return(false);
}


/*******************************************************************/
/* LogCurrentStack, pretty printing IL methods if possible. This
   routine is very robust.  It will never cause an access violation
   and it always find return addresses if they are on the stack 
   (it may find some sperious ones however).  */ 

int LogCurrentStack(BYTE* topOfStack, unsigned len)
{
    size_t* top = (size_t*) topOfStack;
    size_t* end = (size_t*) &topOfStack[len];

    size_t* ptr = (size_t*) (((size_t) top) & ~3);    // make certain dword aligned.
    size_t whereCalled;

    while (ptr < end) 
    {
        if (!isMemoryReadable(ptr, sizeof(void*)))          // stop if we hit unmapped pages
            break;
        if (isRetAddr(*ptr, &whereCalled)) 
        {
            wchar_t buff[512];
            swprintf(buff, L"found retAddr %#08X, %#08X, calling %#08X\n", ptr, *ptr, whereCalled);
            //WszOutputDebugString(buff);
            MethodDesc* ftn = IP2MethodDesc((BYTE*) *ptr);
            if (ftn != 0) {
                swprintf(buff, L"    ");
                EEClass* cls = ftn->GetClass();
                if (cls != 0) {
                    DefineFullyQualifiedNameForClass();
                    LPCUTF8 clsName = GetFullyQualifiedNameForClass(cls);
                    if (clsName != 0)
                        swprintf(&buff[lstrlenW(buff)], L"%S::", clsName);
                    }
#ifdef _DEBUG
                swprintf(&buff[lstrlenW(buff)], L"%S", ftn->GetName());
                swprintf(&buff[lstrlenW(buff)], L"%S\n", ftn->m_pszDebugMethodSignature);
#else
                swprintf(&buff[lstrlenW(buff)], L"%S\n", ftn->GetName());
#endif
                LOG((LF_INTEROP, LL_INFO10, "%S\n", buff));
                }
                        
        }
        ptr++;
    }
    return(0);
}

extern LONG g_RefCount;
/*******************************************************************/
/* CoLogCurrentStack, log the stack from the current ESP.  Stop when we reach a 64K
   boundary */
int STDMETHODCALLTYPE CoLogCurrentStack(WCHAR * pwsz, BOOL fDumpStack) 
{
#ifdef _X86_
    if (g_RefCount > 0)
    {
        BYTE* top = (BYTE *)GetSP();
    
        if (pwsz != NULL)
        {
            LOG((LF_INTEROP, LL_INFO10, "%S\n", pwsz));
        }
        else
        {
            LOG((LF_INTEROP, LL_INFO10, "-----------\n"));
        }
        if (fDumpStack)
            // go back at most 64K, it will stop if we go off the
            // top to unmapped memory
            return(LogCurrentStack(top, 0xFFFF));
        else
            return 0;
    }
#else
    _ASSERTE(!"@NYI - CoLogCurrentStack(DebugHelp.cpp)");
#endif // _X86_
    return -1;
}

/*******************************************************************/
wchar_t* formatMethodTable(MethodTable* pMT, wchar_t* buff) {
    
    EEClass* cls = pMT->GetClass();
    DefineFullyQualifiedNameForClass();
    LPCUTF8 clsName = GetFullyQualifiedNameForClass(cls);
    if (clsName != 0) {
        swprintf(buff, L"%S", clsName);
        buff += lstrlenW(buff);
    }
    return(buff);
}

/*******************************************************************/
wchar_t* formatMethodDesc(MethodDesc* pMD, wchar_t* buff) {
    
    buff = formatMethodTable(pMD->GetMethodTable(), buff);
    wcscat(buff, L"::");

    buff += lstrlenW(buff);
    swprintf(buff, L"%S", pMD->GetName());
    buff += lstrlenW(buff);
#ifdef _DEBUG
    if (pMD->m_pszDebugMethodSignature) {
        swprintf(buff, L" %S", pMD->m_pszDebugMethodSignature);
        buff += lstrlenW(buff);
    }
#endif

    swprintf(buff, L"(%x)", pMD);
    buff += lstrlenW(buff);
    return(buff);
}


/*******************************************************************/
/* dump the stack, pretty printing IL methods if possible. This
   routine is very robust.  It will never cause an access violation
   and it always find return addresses if they are on the stack 
   (it may find some sperious ones however).  */ 

int dumpStack(BYTE* topOfStack, unsigned len) 
{
    size_t* top = (size_t*) topOfStack;
    size_t* end = (size_t*) &topOfStack[len];

    size_t* ptr = (size_t*) (((size_t) top) & ~3);    // make certain dword aligned.
    size_t whereCalled;
    WszOutputDebugString(L"***************************************************\n");
    while (ptr < end) 
    {
        wchar_t buff[512];
        wchar_t* buffPtr = buff;
        if (!isMemoryReadable(ptr, sizeof(void*)))          // stop if we hit unmapped pages
            break;
        if (isRetAddr(*ptr, &whereCalled)) 
        {
            swprintf(buffPtr, L"STK[%08X] = %08X ", ptr, *ptr);
            buffPtr += lstrlenW(buffPtr);
            wchar_t* kind = L"RETADDR ";

               // Is this a stub (is the return address a MethodDesc?
            MethodDesc* ftn = AsMethodDesc(*ptr);
            if (ftn != 0) {
                kind = L"     MD PARAM";
                // If another true return address is not directly before it, it is just
                // a methodDesc param.
                size_t prevRetAddr = ptr[1];
                if (isRetAddr(prevRetAddr, &whereCalled) && AsMethodDesc(prevRetAddr) == 0)
                    kind = L"STUBCALL";
                else  {
                    // Is it the magic sequence used by CallDescr?
                    if (isMemoryReadable((void*) (prevRetAddr - sizeof(short)), sizeof(short)) &&
                        ((short*) prevRetAddr)[-1] == 0x5A59)   // Pop ECX POP EDX
                        kind = L"STUBCALL";
                }
            }
            else    // Is it some other code the EE knows about?
                ftn = IP2MethodDesc((BYTE*)(*ptr));

            swprintf(buffPtr, L"%s ", kind);
            buffPtr += lstrlenW(buffPtr);

            if (ftn != 0)
                buffPtr = formatMethodDesc(ftn, buffPtr);
            else {
                wcscpy(buffPtr, L"<UNKNOWN FTN>");
                buffPtr += lstrlenW(buffPtr);
            }

            if (whereCalled != 0) {
                swprintf(buffPtr, L" Caller called Entry %X", whereCalled);
                buffPtr += lstrlenW(buffPtr);
            }

            wcscpy(buffPtr, L"\n");
            WszOutputDebugString(buff);
        }
        MethodTable* pMT = AsMethodTable(*ptr);
        if (pMT != 0) {
            swprintf(buffPtr, L"STK[%08X] = %08X          MT PARAM ", ptr, *ptr, pMT);
            buffPtr += lstrlenW(buffPtr);
            buffPtr = formatMethodTable(pMT, buffPtr);
            wcscpy(buffPtr, L"\n");
            WszOutputDebugString(buff);
        }
        ptr++;
    }
    return(0);
}

/*******************************************************************/
/* dump the stack from the current ESP.  Stop when we reach a 64K
   boundary */
int DumpCurrentStack()
{
#ifdef _X86_
    BYTE* top = (BYTE *)GetSP();
    
        // go back at most 64K, it will stop if we go off the
        // top to unmapped memory
    return(dumpStack(top, 0xFFFF));
#else
    _ASSERTE(!"@NYI - DumpCurrentStack(DebugHelp.cpp)");
    return 0;
#endif // _X86_
}

/*******************************************************************/
WCHAR* StringVal(STRINGREF objref) {
    return(objref->GetBuffer());
}
    
LPCUTF8 NameForMethodTable(UINT_PTR pMT) {
    DefineFullyQualifiedNameForClass();
    LPCUTF8 clsName = GetFullyQualifiedNameForClass(((MethodTable*)pMT)->GetClass());
    // Note we're returning local stack space - this should be OK for using in the debugger though
    return clsName;
}

LPCUTF8 ClassNameForObject(UINT_PTR obj) {
    return(NameForMethodTable((UINT_PTR)(((Object*)obj)->GetMethodTable())));
}
    
LPCUTF8 ClassNameForOBJECTREF(OBJECTREF obj) {
    return(ClassNameForObject((UINT_PTR)(OBJECTREFToObject(obj))));
}

LPCUTF8 NameForMethodDesc(UINT_PTR pMD) {
    return(((MethodDesc*)pMD)->GetName());
}

LPCUTF8 ClassNameForMethodDesc(UINT_PTR pMD) {
    DefineFullyQualifiedNameForClass ();
    if (((MethodDesc *)pMD)->GetClass())
    {
        return GetFullyQualifiedNameForClass(((MethodDesc*)pMD)->GetClass());
    }
    else
        return "GlobalFunctions";
}

PCCOR_SIGNATURE RawSigForMethodDesc(MethodDesc* pMD) {
    return(pMD->GetSig());
}

Thread * CurrentThreadInfo ()
{
    return GetThread ();
}

AppDomain *GetAppDomainForObject(UINT_PTR obj)
{
    return ((Object*)obj)->GetAppDomain();
}

DWORD GetAppDomainIndexForObject(UINT_PTR obj)
{
    return ((Object*)obj)->GetHeader()->GetAppDomainIndex();
}

AppDomain *GetAppDomainForObjectHeader(UINT_PTR hdr)
{
    DWORD indx = ((ObjHeader*)hdr)->GetAppDomainIndex();
    if (! indx)
        return NULL;
    return SystemDomain::GetAppDomainAtIndex(indx);
}

DWORD GetAppDomainIndexForObjectHeader(UINT_PTR hdr)
{
    return ((ObjHeader*)hdr)->GetAppDomainIndex();
}

SyncBlock *GetSyncBlockForObject(UINT_PTR obj)
{
    return ((Object*)obj)->GetHeader()->GetRawSyncBlock();
}

#include "../ildasm/formattype.cpp"
bool IsNameToQuote(const char *name) { return(false); }
/*******************************************************************/
void PrintTableForClass(UINT_PTR pClass)
{
    DefineFullyQualifiedNameForClass();
    LPCUTF8 name = GetFullyQualifiedNameForClass(((EEClass*)pClass));
    ((EEClass*)pClass)->DebugDumpVtable(name, true);
    ((EEClass*)pClass)->DebugDumpFieldLayout(name, true);
    ((EEClass*)pClass)->DebugDumpGCDesc(name, true);
}

void PrintMethodTable(UINT_PTR pMT)
{
  PrintTableForClass((UINT_PTR) ((MethodTable*)pMT)->GetClass() );
}

void PrintTableForMethodDesc(UINT_PTR pMD)
{
    PrintMethodTable((UINT_PTR) ((MethodDesc *)pMD)->GetClass()->GetMethodTable() );
}

void PrintException(OBJECTREF pObjectRef)
{
    if(pObjectRef == NULL) 
        return;

    COMPLUS_TRY {
        GCPROTECT_BEGIN(pObjectRef);

        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__EXCEPTION__INTERNAL_TO_STRING);        

        ARG_SLOT arg[1] = { 
            ObjToArgSlot(pObjectRef)
        };
        
        STRINGREF str = ArgSlotToString(pMD->Call(arg));

        if(str->GetBuffer() != NULL) {
            WszOutputDebugString(str->GetBuffer());
        }

        GCPROTECT_END();
    }
    COMPLUS_CATCH {
    } COMPLUS_END_CATCH;
} 

void PrintException(UINT_PTR pObject)
{
    OBJECTREF pObjectRef = NULL;
    GCPROTECT_BEGIN(pObjectRef);
    GCPROTECT_END();
}


/*******************************************************************/
char* FormatSig(MethodDesc* pMD) {

    CQuickBytes out;
    PCCOR_SIGNATURE pSig;
    ULONG cSig;
    pMD->GetSig(&pSig, &cSig);

    if (pSig == NULL)
        return "<null>";

    const char* sigStr = PrettyPrintSig(pSig, cSig, "*", &out, pMD->GetMDImport(), 0);

    char* ret = (char*) pMD->GetModule()->GetClassLoader()->GetHighFrequencyHeap()->AllocMem(strlen(sigStr)+1);
    strcpy(ret, sigStr);
    return(ret);
}

/*******************************************************************/
/* sends a current stack trace to the debug window */

struct PrintCallbackData {
    BOOL toStdout;
    BOOL withAppDomain;
#ifdef _DEBUG
    BOOL toLOG;
#endif
};

StackWalkAction PrintStackTraceCallback(CrawlFrame* pCF, VOID* pData)
{
    MethodDesc* pMD = pCF->GetFunction();
    wchar_t buff[1024];
    buff[0] = 0;

    PrintCallbackData *pCBD = (PrintCallbackData *)pData;

    if (pMD != 0) {
        EEClass* cls = pMD->GetClass();
        if (pCBD->withAppDomain)
            swprintf(&buff[lstrlenW(buff)], L"{[%3.3x] %s} ", pCF->GetAppDomain()->GetId(), pCF->GetAppDomain()->GetFriendlyName(FALSE));
        if (cls != 0) {
            DefineFullyQualifiedNameForClass();
            LPCUTF8 clsName = GetFullyQualifiedNameForClass(cls);
            if (clsName != 0)
                swprintf(&buff[lstrlenW(buff)], L"%S::", clsName);
        }
        swprintf(&buff[lstrlenW(buff)], L"%S %S  ", pMD->GetName(), FormatSig(pMD));
        if (pCF->IsFrameless() && pCF->GetJitManager() != 0) {
            METHODTOKEN methTok;
            IJitManager* jitMgr = pCF->GetJitManager();
            PREGDISPLAY regs = pCF->GetRegisterSet();
            
            DWORD offset;
            jitMgr->JitCode2MethodTokenAndOffset((PBYTE)GetControlPC(regs), &methTok, &offset);
            BYTE* start = jitMgr->JitToken2StartAddress(methTok);

            swprintf(&buff[lstrlenW(buff)], L"JIT ESP:%X MethStart:%X EIP:%X(rel %X)", 
                GetRegdisplaySP(regs), start, GetControlPC(regs), offset);
        } else
            swprintf(&buff[lstrlenW(buff)], L"EE implemented");
    } else {
        Frame* frame = pCF->GetFrame();
        swprintf(&buff[lstrlenW(buff)], L"EE Frame is" LFMT_ADDR, 
                 DBG_ADDR(frame));
    }

    if (pCBD->toStdout)
    {
        wcscat(buff, L"\n");
        PrintToStdOutW(buff);
    }
#ifdef _DEBUG
    else if (pCBD->toLOG) {
        MAKE_ANSIPTR_FROMWIDE(sbuff, buff);
        // For LogSpewAlways to work rightr the "\n" (newline)
        // must be in the fmt string not part of the args
        LogSpewAlways("    %s\n", sbuff);
    }
#endif
    else
    {
        wcscat(buff, L"\n");
        WszOutputDebugString(buff);
    }
    return SWA_CONTINUE;
}

void PrintStackTrace()
{
    WszOutputDebugString(L"***************************************************\n");
    PrintCallbackData cbd = {0, 0};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}

void PrintStackTraceToStdout()
{
    PrintCallbackData cbd = {1, 0};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}

#ifdef _DEBUG
void PrintStackTraceToLog()
{
    PrintCallbackData cbd = {0, 0, 1};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}
#endif

void PrintStackTraceWithAD()
{
    WszOutputDebugString(L"***************************************************\n");
    PrintCallbackData cbd = {0, 1};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}

void PrintStackTraceWithADToStdout()
{
    PrintCallbackData cbd = {1, 1};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}

#ifdef _DEBUG
void PrintStackTraceWithADToLog()
{
    PrintCallbackData cbd = {0, 1, 1};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}

void PrintStackTraceWithADToLog(Thread *pThread)
{
    PrintCallbackData cbd = {0, 1, 1};
    pThread->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}
#endif


/*******************************************************************/
// Get the system or current domain from the thread. 
BaseDomain* GetSystemDomain()
{
    return SystemDomain::System();
}

AppDomain* GetCurrentDomain()
{
    return SystemDomain::GetCurrentDomain();
}

void PrintDomainName(size_t ob)
{
    AppDomain* dm = (AppDomain*) ob;
    LPCWSTR st = dm->GetFriendlyName(FALSE);
    if(st != NULL)
        WszOutputDebugString(st);
    else
        WszOutputDebugString(L"<Domain with no Name>");
}



/*******************************************************************/
const char* GetClassName(void* ptr)
{
    DefineFullyQualifiedNameForClass();
    LPCUTF8 clsName = GetFullyQualifiedNameForClass(((EEClass*)ptr));
    // Note we're returning local stack space - this should be OK for using in the debugger though
    return (const char *) clsName;
}


#ifdef LOGGING
void LogStackTrace()
{
    PrintCallbackData cbd = {0, 0, 1};
    GetThread()->StackWalkFrames(PrintStackTraceCallback, &cbd, 0, 0);
}
#endif

	// The functions above are ONLY meant to be used in the 'watch' window of a debugger.
	// Thus they are NEVER called by the runtime itself (except for diagnosic purposes etc.   
	// We DO want these funcitons in free (but not golden) builds because that is where they 
	// are needed the most (when you don't have nice debug fields that tell you the names of things).
	// To keep the linker from optimizing these away, the following array provides a
	// reference (and there is a reference to this array in EEShutdown) so that it looks
	// referenced 

void* debug_help_array[] = {
    (void *) PrintStackTrace,
    (void *) DumpCurrentStack,
    (void *) StringVal,
    (void *) NameForMethodTable,
    (void *) ClassNameForObject,
    (void *) ClassNameForOBJECTREF,
    (void *) NameForMethodDesc,
    (void *) RawSigForMethodDesc,
    (void *) ClassNameForMethodDesc,
    (void *) CurrentThreadInfo,
    (void *) IP2MD,
    (void *) Entry2MethodDescMD,
    };

