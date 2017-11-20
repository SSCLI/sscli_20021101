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
// NEXPORT.H -
//
//


#ifndef __nexport_h__
#define __nexport_h__

#include "object.h"
#include "stublink.h"
#include "ml.h"
#include "ceeload.h"
#include "class.h"


#include <pshpack1.h>
//--------------------------------------------------------------------------
// This structure forms a header for the marshaling code for an UMThunk.
//--------------------------------------------------------------------------
struct UMThunkMLStub
{
    UINT16        m_cbDstStack;   //# of bytes of stack portion of managed args
    UINT16        m_cbSrcStack;   //# of bytes of stack portion of unmanaged args
    UINT16        m_cbRetPop;     //# of bytes to pop on return to unmanaged
    UINT16        m_cbLocals;     //# of bytes required in the local array
    UINT16        m_cbRetValSize; //# of bytes of retval (including stack promotion)
    BYTE          m_fIsStatic;    //Is method static?
    BYTE          m_fThisCall;
	BYTE		  m_fThisCallHiddenArg; //Has hidden arg for returning structures (triggers special case in thiscall adjustment)
    BYTE          m_fpu;
    BYTE          m_fHasReturnBuffer;
    BYTE          m_fRetValRequiredGCProtect;

    const MLCode *GetMLCode() const
    {
        return (const MLCode *)(this+1);
    }
};

#include <poppack.h>


//----------------------------------------------------------------------
// This structure collects all information needed to marshal an
// unmanaged->managed thunk. The only information missing is the
// managed target and the "this" object (if any.) Those two pieces
// are broken out into a small UMEntryThunk.
//
// The idea is to share UMThunkMarshInfo's between multiple thunks
// that have the same signature while the UMEntryThunk contains the
// minimal info needed to distinguish between actual function pointers.
//----------------------------------------------------------------------
class UMThunkMarshInfo
{
    friend class UMThunkStubCache;


    public:



        //----------------------------------------------------------
        // This initializer can be called during load time.
        // It does not do any ML stub initialization or sigparsing.
        // The RunTimeInit() must be called subsequently before this
        // can safely be used.
        //----------------------------------------------------------
        VOID LoadTimeInit(PCCOR_SIGNATURE          pSig,
                          DWORD                    cSig,
                          Module                  *pModule,
                          BOOL                     fIsStatic,
                          BYTE                     nlType,
                          CorPinvokeMap            unmgdCallConv,
                          mdMethodDef              mdForNativeTypes = mdMethodDefNil);



        //----------------------------------------------------------
        // This initializer finishes the init started by LoadTimeInit.
        // It does all the ML stub creation, and can throw a COM+
        // exception.
        //
        // It can safely be called multiple times and by concurrent
        // threads.
        //----------------------------------------------------------
        VOID RunTimeInit();


        //----------------------------------------------------------
        // Combines LoadTime & RunTime inits for convenience.
        //----------------------------------------------------------
        VOID CompleteInit(PCCOR_SIGNATURE          pSig,
                          DWORD                    cSig,
                          Module                  *pModule,
                          BOOL                     fIsStatic,
                          BYTE                     nlType,
                          CorPinvokeMap            unmgdCallConv,
                          mdMethodDef              mdForNativeTypes = mdMethodDefNil);


        //----------------------------------------------------------
        // Destructor.
        //----------------------------------------------------------
        ~UMThunkMarshInfo();

        //----------------------------------------------------------
        // Accessor functions
        //----------------------------------------------------------
        PCCOR_SIGNATURE GetSig() const
        {
            _ASSERTE(IsAtLeastLoadTimeInited());
            return m_pSig;
        }

        Module *GetModule() const
        {
            _ASSERTE(IsAtLeastLoadTimeInited());
            return m_pModule;
        }

        BOOL IsStatic() const
        {
            _ASSERTE(IsAtLeastLoadTimeInited());
            return m_fIsStatic;
        }

        Stub *GetMLStub() const
        {
            _ASSERTE(IsCompletelyInited());
            return m_pMLStub;
        }

        Stub *GetExecStub() const
        {
            _ASSERTE(IsCompletelyInited());
            return m_pExecStub;
        }

        UINT32 GetCbRetPop() const
        {
            _ASSERTE(IsCompletelyInited());
            return m_cbRetPop;
        }

        UINT32 GetCbActualArgSize() const
        {
            _ASSERTE(IsCompletelyInited());
            return m_cbActualArgSize;
        }

        CorPinvokeMap GetUnmanagedCallConv() const
        {
            // In the future, we'll derive the unmgdCallConv from the signature
            // rather than having it passed in separately. To avoid having
            // to parse the signature at loadtimeinit (not necessarily a problem
            // but we want to keep the amount of loadtimeinit processing at
            // a minimum), we'll be extra-strict here to prevent other code
            // from depending on the callconv being available at loadtime.
            _ASSERTE(IsCompletelyInited());
            return m_unmgdCallConv;
        }

        BYTE GetNLType() const
        {
            _ASSERTE(IsAtLeastLoadTimeInited());
            return m_nlType;
        }

#ifdef _DEBUG
        BOOL IsAtLeastLoadTimeInited() const
        {
            return m_state == kLoadTimeInited || m_state == kRunTimeInited;
        }


        BOOL IsCompletelyInited() const
        {
            return m_state == kRunTimeInited;
        }


#endif




    private:
        size_t            m_state;        // the initialization state 

        enum {
            kLoadTimeInited = 0x4c55544d,   //'LUTM'
            kRunTimeInited  = 0x5255544d,   //'RUTM'
        };


        PCCOR_SIGNATURE   m_pSig;         // signature
        DWORD             m_cSig;         // signature size
        Module           *m_pModule;      // module
        BOOL              m_fIsStatic;    // static or virtual?
        Stub             *m_pMLStub;      // if interpreted, UmThunkMLHeader-prefixed ML stub for marshaling - NULL otherwise
        Stub             *m_pExecStub;    // UMEntryThunk jumps directly here
        UINT32            m_cbRetPop;     // stack bytes popped by callee (for UpdateRegDisplay)
        UINT32            m_cbActualArgSize; // caches m_pSig.SizeOfActualFixedArgStack()
        BYTE              m_nlType;       // charset
        CorPinvokeMap     m_unmgdCallConv; //calling convention
        mdMethodDef       m_mdForNativeTypes;  // (optional) nativetype metadata

};










//----------------------------------------------------------------------
// This structure contains the minimal information required to
// distinguish one function pointer from another, with the rest
// being stored in a shared UMThunkMarshInfo.
//
// This structure also contains the actual code bytes that form the
// front end of the thunk. A pointer to the m_code[] byte array is
// what is actually handed to unmanaged client code.
//----------------------------------------------------------------------
class UMEntryThunk
{
    friend class UMThunkStubCache;
    friend class Stub *GenerateUMThunkPrestub();

    public:
    	static UMEntryThunk* CreateUMEntryThunk();
    	static VOID FreeUMEntryThunk(UMEntryThunk* p);

	 void* operator new(size_t size, void* spot) {   return (spot); }
    void operator delete(void* spot) {}
    	
        VOID LoadTimeInit(const BYTE             *pManagedTarget,
                          OBJECTHANDLE            pObjectHandle,
                          UMThunkMarshInfo       *pUMThunkMarshInfo,
                          MethodDesc             *pMD, 
                          DWORD                   dwDomainId)
        {

            _ASSERTE(pUMThunkMarshInfo->IsAtLeastLoadTimeInited());
            _ASSERTE(pManagedTarget != NULL || pMD != NULL);

            m_pManagedTarget    = pManagedTarget;
            m_pObjectHandle     = pObjectHandle;
            m_pUMThunkMarshInfo = pUMThunkMarshInfo;
            m_dwDomainId        = dwDomainId;

            m_pMD = pMD;    // For debugging and profiling, so they can identify the target

            m_code.Encode((BYTE*)TheUMThunkPreStub()->GetEntryPoint(), this);
#ifdef _DEBUG
            m_state = kLoadTimeInited;
#endif

        }

        ~UMEntryThunk()
        {
            if (GetObjectHandle())
            {
                DestroyLongWeakHandle(GetObjectHandle());
            }
#ifdef _DEBUG
            FillMemory(this, sizeof(*this), 0xcc);
#endif
        }

        VOID RunTimeInit()
        {
            THROWSCOMPLUSEXCEPTION();

            m_pUMThunkMarshInfo->RunTimeInit();
            m_code.Encode((BYTE*)m_pUMThunkMarshInfo->GetExecStub()->GetEntryPoint(), this);

#ifdef _DEBUG
            m_state = kRunTimeInited;
#endif
        }

        // asm entrypoint
        static VOID __stdcall DoRunTimeInit(UMEntryThunk* pThis)
        {
            pThis->RunTimeInit();
        }

        VOID CompleteInit(const BYTE             *pManagedTarget,
                          OBJECTHANDLE            pObjectHandle,
                          UMThunkMarshInfo       *pUMThunkMarshInfo,
                          MethodDesc             *pMD,
                          DWORD                   dwAppDomainId)
        {
            THROWSCOMPLUSEXCEPTION();

            LoadTimeInit(pManagedTarget, pObjectHandle, pUMThunkMarshInfo, pMD, dwAppDomainId);
            RunTimeInit();
        }

        const BYTE *GetManagedTarget() const
        {
            _ASSERTE(m_state == kRunTimeInited || m_state == kLoadTimeInited);
            if (m_pManagedTarget)
            {
                return m_pManagedTarget;
            }
            else
            {
                return m_pMD->GetAddrofCode();
            }
        }

        OBJECTHANDLE GetObjectHandle() const
        {
            _ASSERTE(m_state == kRunTimeInited || m_state == kLoadTimeInited);
            return m_pObjectHandle;
        }

        const UMThunkMarshInfo *GetUMThunkMarshInfo() const
        {
            _ASSERTE(m_state == kRunTimeInited || m_state == kLoadTimeInited);
            return m_pUMThunkMarshInfo;
        }

        const BYTE *GetCode() const
        {
            _ASSERTE(m_state == kRunTimeInited || m_state == kLoadTimeInited);
            return m_code.GetEntryPoint();
        }

        static UMEntryThunk *RecoverUMEntryThunk(const VOID* pCode)
        {
            return (UMEntryThunk*)( ((LPBYTE)pCode) - offsetof(UMEntryThunk, m_code) );
        }


        MethodDesc *GetMethod() const
        {
            _ASSERTE(m_state == kRunTimeInited || m_state == kLoadTimeInited);
            return m_pMD;
        }

        DWORD GetDomainId() const
        {
            _ASSERTE(m_state == kRunTimeInited);
            return m_dwDomainId;
        }

        static DWORD GetOffsetOfMethodDesc()
        {
            return offsetof(UMEntryThunk, m_pMD);
        }

        static DWORD GetOffsetOfCode()
        {
            return offsetof(UMEntryThunk, m_code) + UMEntryThunkCode::GetEntryPointOffset();
        }

#ifdef _X86_
        static VOID EmitUMEntryThunkCall(CPUSTUBLINKER *psl);
#endif

#ifdef CUSTOMER_CHECKED_BUILD

        BOOL DeadTarget() const
        {
            return (m_pManagedTarget == NULL);
        }
#endif

        static UMEntryThunk* Decode(LPVOID pCallback);

    private:
        // The start of the managed code
        const BYTE             *m_pManagedTarget;

        // This is used for profiling.
        MethodDesc             *m_pMD;

        // Object handle holding "this" reference. May be a strong or weak handle.
        // Field is NULL for a static method.
        OBJECTHANDLE            m_pObjectHandle;

        // Pointer to the shared structure containing everything else
        UMThunkMarshInfo       *m_pUMThunkMarshInfo;

        DWORD                   m_dwDomainId;   // appdomain of module (cached for fast access)

#ifdef _DEBUG
        DWORD            m_state;        // the initialization state 

        enum {
            kLoadTimeInited = 0x4c554554,   //'LUET'
            kRunTimeInited  = 0x52554554,   //'RUET'
        };
#endif

        UMEntryThunkCode    m_code;
};











//--------------------------------------------------------------------------
// Onetime Init
//--------------------------------------------------------------------------
BOOL UMThunkInit();


//--------------------------------------------------------------------------
// Onetime Shutdown
//--------------------------------------------------------------------------




//-------------------------------------------------------------------------
// One-time creation of special prestub to initialize UMEntryThunks.
//-------------------------------------------------------------------------
Stub *GenerateUMThunkPrestub();

//-------------------------------------------------------------------------
// NExport stub
//-------------------------------------------------------------------------
Stub *CreateGenericNExportStub(UINT cbRetPop, 
    BOOL fHashThisCallAdjustment, BOOL fHashThisCallHiddenArg);

//-------------------------------------------------------------------------
// Recognize special SEH handler for NExport
//-------------------------------------------------------------------------
BOOL NExportSEH(EXCEPTION_REGISTRATION_RECORD* pEHR);
BOOL FastNExportSEH(EXCEPTION_REGISTRATION_RECORD* pEHR);

INT64 __stdcall DoUMThunkCall(Thread *pThread, UMThkCallFrame *pFrame);
EXCEPTION_HANDLER_DECL(UMThunkPrestubHandler);



#endif //__nexport_h__


