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
    /*============================================================
    **
    ** File:    message.h
    **       
    **
    ** Purpose: Encapsulates a function call frame into a message 
    **          object with an interface that can enumerate the 
    **          arguments of the messagef
    **
    ** Date:    Mar 5, 1999
    **
    ===========================================================*/
    #ifndef ___MESSAGE_H___
    #define ___MESSAGE_H___
    
    #include "fcall.h"
    
    void GetObjectFromStack(OBJECTREF* ppDest, PVOID val, const CorElementType eType, EEClass *pCls, BOOL fIsByRef = FALSE);
    
    
    //+----------------------------------------------------------
    //
    //  Struct:     MessageObject
    // 
    //  Synopsis:   Physical mapping of the System.Runtime.Remoting.Message
    //              object.
    //
    //
    //------------------------------------------------------------
    class MessageObject : public Object
    {
        friend class CMessage;
        friend class Binder;

        STRINGREF          pMethodName;    // Method name
        BASEARRAYREF       pMethodSig;     // Array of parameter types
        REFLECTBASEREF     pMethodBase;    // Reflection method object
        OBJECTREF          pHashTable;     // hashtable for properties
        STRINGREF          pURI;           // object's URI
        STRINGREF          pTypeName;       // not used in VM, placeholder
        OBJECTREF          pFault;         // exception

        OBJECTREF          pID;            // not used in VM, placeholder
        OBJECTREF          pSrvID;         // not used in VM, placeholder
        OBJECTREF          pArgMapper;     // not used in VM, placeholder
        OBJECTREF          pCallCtx;       // not used in VM, placeholder


        FramedMethodFrame  *pFrame;
        MethodDesc         *pMethodDesc;
        MetaSig            *pMetaSigHolder;
        MethodDesc         *pDelegateMD;
        INT32               iLast;
        INT32               iFlags;
        INT32               initDone;       // called the native Init routine
    };

#ifdef USE_CHECKED_OBJECTREFS
    typedef REF<MessageObject> MESSAGEREF;
#else
    typedef MessageObject* MESSAGEREF;
#endif

    // *******
    // Note: Needs to be in sync with flags in Message.cs
    // *******
    enum
    {
        MSGFLG_BEGININVOKE = 0x01,
        MSGFLG_ENDINVOKE   = 0x02,
        MSGFLG_CTOR        = 0x04,
        MSGFLG_ONEWAY      = 0x08,
        MSGFLG_FIXEDARGS   = 0x10,
        MSGFLG_VARARGS     = 0x20
    };
    
    //+----------------------------------------------------------
    //
    //  Class:      CMessage
    // 
    //  Synopsis:   EE counterpart to Microsoft.Runtime.Message.
    //              Encapsulates code to read a function call 
    //              frame into an interface that can enumerate
    //              the parameters.
    //
    //------------------------------------------------------------
    class CMessage
    {
    public:
    
       // Methods used for stack walking
       static FCDECL1(INT32, GetArgCount, MessageObject *pMsg);
       
       //struct GetArgArgs
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(MESSAGEREF, pMessage);
       //    DECLARE_ECALL_I4_ARG(INT32, argNum);
       //};
       static FCDECL2(Object*, GetArg, MessageObject* pMessage, INT32 argNum);

       //struct GetArgsArgs
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(MESSAGEREF, pMessage);
       //};
       static FCDECL1(Object*, GetArgs, MessageObject* pMessageUNSAFE);

       //struct PropagateOutParametersArgs
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(MESSAGEREF, pMessage);
       //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, RetVal);
       //    DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, pOutPrms);
       //};
       static FCDECL3(void, PropagateOutParameters, MessageObject* pMessageUNSAFE, ArrayBase* pOutPrmsUNSAFE, Object* RetValUNSAFE);

       //struct GetReturnValueArgs
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(MESSAGEREF, pMessage);
       //};
       static FCDECL1(Object*, GetReturnValue, MessageObject* pMessageUNSAFE);

       //struct GetMethodNameArgs
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF*, pTypeNAssemblyName);
       //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, pMethodBase);
       //};
       static FCDECL2(Object*, GetMethodName, ReflectBaseObject* pMethodBaseUNSAFE, STRINGREF* pTypeNAssemblyName);

       //struct GetMethodBaseArgs
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(MESSAGEREF, pMessage);
       //};
       static FCDECL1(Object*, GetMethodBase, MessageObject* pMessageUNSAFE);

       //struct InitArgs
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(MESSAGEREF, pMessage);
       //};
       static FCDECL1(void, Init, MessageObject* pMessageUNSAFE);

       //struct GetAsyncBeginInfoArgs
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(MESSAGEREF, pMessage);
       //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF*, ppState);
       //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF*, ppACBD);
       //};
       static FCDECL3(void, GetAsyncBeginInfo, MessageObject* pMessageUNSAFE, OBJECTREF* ppACBD, OBJECTREF* ppState);

       //struct GetAsyncResultArgs
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(MESSAGEREF, pMessage);
       //};
       static FCDECL1(LPVOID, GetAsyncResult, MessageObject* pMessageUNSAFE);

       //struct GetAsyncObjectArgs
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(MESSAGEREF, pMessage);
       //};
       static FCDECL1(Object*, GetAsyncObject, MessageObject* pMessageUNSAFE);

       //struct DebugOutArgs
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, pOut);
       //};
       static FCDECL1(void, DebugOut, StringObject* pOutUNSAFE);

       //struct DispatchArgs 
       //{
       //    DECLARE_ECALL_OBJECTREF_ARG(MESSAGEREF, pMessage);
       //    DECLARE_ECALL_I4_ARG(BOOL, fContext);
       //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pServer);
       //};
       static FCDECL3(BOOL, Dispatch, MessageObject* pMessageUNSAFE, Object* pServerUNSAFE, BYTE fContext);

       static FCDECL0(UINT32, GetMetaSigLen);
       static FCDECL1(BOOL, CMessage::HasVarArgs, MessageObject * poMessage);
       static FCDECL1(PVOID, CMessage::GetVarArgsPtr, MessageObject * poMessage);

       //struct MethodAccessCheckArgs
       //{
       //    DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
       //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, method); 
       //};
       static FCDECL2(void, MethodAccessCheck, ReflectBaseObject* methodUNSAFE, StackCrawlMark* stackMark);

       // private helpers
    private:
       static REFLECTBASEREF __stdcall GetExposedObjectFromMethodDesc(MethodDesc *pMD);
       static PVOID GetStackPtr(INT32 ndx, FramedMethodFrame *pFrame, MetaSig *pSig);       
       static MetaSig* __stdcall GetMetaSig(MessageObject *pMsg);
       static INT64 __stdcall CallMethod(const void *pTarget,
                                         INT32 cArgs,
                                         FramedMethodFrame *pFrame,
                                         OBJECTREF pObj);
       static INT64 CopyOBJECTREFToStack(PVOID pvDest, OBJECTREF pSrc,
                     CorElementType typ, EEClass *pClass, MetaSig *pSig,
                     BOOL fCopyClassContents);
       static LPVOID GetLastArgument(MessageObject *pMsg);
    };
    
    #endif // ___MESSAGE_H___
