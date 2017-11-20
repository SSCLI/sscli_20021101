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
// ECALL.CPP -
//
// Handles our private native calling interface.
//

#include "common.h"
#include "vars.hpp"
#include "object.h"
#include "util.hpp"
#include "stublink.h"
#include "cgensys.h"
#include "ecall.h"
#include "excep.h"
#include "jitinterface.h"
#include "security.h"
#include "ndirect.h"
#include "comarrayinfo.h"
#include "comstring.h"
#include "comstringbuffer.h"
#include "comsecurityruntime.h"
#include "comcodeaccesssecurityengine.h"
#include "comobject.h"
#include "comclass.h"
#include "comdelegate.h"
#include "customattribute.h"
#include "commember.h"
#include "comdynamic.h"
#include "commethodrental.h"
#include "../classlibnative/inc/comnlsinfo.h"
#include "commodule.h"
#include "comndirect.h"
#include "comsystem.h"
#include "comutilnative.h"
#include "comsynchronizable.h"
#include "../classlibnative/inc/comfloatclass.h"
#include "comstreams.h"
#include "comvariant.h"
#include "comdecimal.h"
#include "comcurrency.h"
#include "comdatetime.h"
#include "comisolatedstorage.h"
#include "comsecurityconfig.h"
#include "array.h"
#include "comnumber.h"
#include "remoting.h"
#include "debugdebugger.h"
#include "message.h"
#include "stackbuildersink.h"
#include "remoting.h"
#include "assemblyname.hpp"
#include "assemblynative.hpp"
#include "rwlock.h"
#include "comthreadpool.h"
#include "comwaithandle.h"
#include "comevent.h"
#include "commutex.h"
#include "nativeoverlapped.h"
#include "jitinterface.h"
#include "proftoeeinterfaceimpl.h"
#include "eeconfig.h"
#include "appdomainnative.hpp"
#include "handletablepriv.h"
#include "objecthandle.h"
#include "confighelper.h"
#include "comarrayhelpers.h"

#include "fusionwrap.h"


ECFunc* FindImplsForClass(MethodTable* pMT);

FCDECL0(LPVOID, GetPrivateContextsPerfCountersEx);
FCDECL0(LPVOID, GetGlobalContextsPerfCountersEx);

ArrayStubCache *ECall::m_pArrayStubCache = NULL;



// Note: if you mess these up (ie, accidentally use FCFuncElement for an ECall method),
// you'll get an access violation or similar bizarre-seeming error.  This MUST be correct.
#define FCFuncElement(A,B,C) A, B, C, NULL, CORINFO_INTRINSIC_Illegal
#define FCIntrinsic(A,B,C, intrinsicID) A, B, C, NULL, intrinsicID


static
ECFunc  gRemotingFuncs[] =
{
    {FCFuncElement("IsTransparentProxy", &gsig_SM_Obj_RetBool, (LPVOID)CRemotingServices::IsTransparentProxy)},
    {FCFuncElement("GetRealProxy", NULL, (LPVOID)CRemotingServices::GetRealProxy)},
    {FCFuncElement("Unwrap", NULL, (LPVOID)CRemotingServices::Unwrap)},
    {FCFuncElement("AlwaysUnwrap", NULL, (LPVOID)CRemotingServices::AlwaysUnwrap)},
    {FCFuncElement("CheckCast",    NULL, (LPVOID)CRemotingServices::NativeCheckCast)},
    {FCFuncElement("nSetRemoteActivationConfigured", NULL, (LPVOID)CRemotingServices::SetRemotingConfiguredFlag)},
    {FCFuncElement("CORProfilerTrackRemoting", NULL, (LPVOID)ProfilingFCallHelper::FC_TrackRemoting)},
    {FCFuncElement("CORProfilerTrackRemotingCookie", NULL, (LPVOID)ProfilingFCallHelper::FC_TrackRemotingCookie)},
    {FCFuncElement("CORProfilerTrackRemotingAsync", NULL, (LPVOID)ProfilingFCallHelper::FC_TrackRemotingAsync)},
    {FCFuncElement("CORProfilerRemotingClientSendingMessage", NULL, (LPVOID)ProfilingFCallHelper::FC_RemotingClientSendingMessage)},
    {FCFuncElement("CORProfilerRemotingClientReceivingReply", NULL, (LPVOID)ProfilingFCallHelper::FC_RemotingClientReceivingReply)},
    {FCFuncElement("CORProfilerRemotingServerReceivingMessage", NULL, (LPVOID)ProfilingFCallHelper::FC_RemotingServerReceivingMessage)},
    {FCFuncElement("CORProfilerRemotingServerSendingReply", NULL, (LPVOID)ProfilingFCallHelper::FC_RemotingServerSendingReply)},
    {FCFuncElement("CORProfilerRemotingClientInvocationFinished", NULL, (LPVOID)ProfilingFCallHelper::FC_RemotingClientInvocationFinished)},
#ifdef REMOTING_PERF
    {FCFuncElement("LogRemotingStage",    NULL, (LPVOID)CRemotingServices::LogRemotingStage)},
#endif
    {FCFuncElement("CreateTransparentProxy",        NULL, (LPVOID)CRemotingServices::CreateTransparentProxy)},
    {FCFuncElement("AllocateUninitializedObject",   NULL, (LPVOID)CRemotingServices::AllocateUninitializedObject)},
    {FCFuncElement("CallDefaultCtor",               NULL, (LPVOID)CRemotingServices::CallDefaultCtor)},
    {FCFuncElement("AllocateInitializedObject",     NULL, (LPVOID)CRemotingServices::AllocateInitializedObject)},
    {NULL, NULL, NULL}
};

static
ECFunc  gRealProxyFuncs[] =
{
    {FCFuncElement("SetStubData",        NULL, (LPVOID)CRealProxy::SetStubData)},
    {FCFuncElement("GetStubData",        NULL, (LPVOID)CRealProxy::GetStubData)},
    {FCFuncElement("GetStub",            NULL, (LPVOID)CRealProxy::GetStub)},
    {FCFuncElement("GetDefaultStub",     NULL, (LPVOID)CRealProxy::GetDefaultStub)},
    {FCFuncElement("GetProxiedType",     NULL, (LPVOID)CRealProxy::GetProxiedType)},
    {NULL, NULL, NULL}
};

static
ECFunc gContextFuncs[] =
{
    {FCFuncElement("SetupInternalContext", NULL, (LPVOID)Context::SetupInternalContext)},
    {FCFuncElement("CleanupInternalContext", NULL, (LPVOID)Context::CleanupInternalContext)},
    {FCFuncElement("ExecuteCallBackInEE", NULL, (LPVOID)Context::ExecuteCallBack)},
    {NULL, NULL, NULL, NULL}
};


static
ECFunc gRWLockFuncs[] =
{
    {FCFuncElement("AcquireReaderLock", NULL, (LPVOID) CRWLock::StaticAcquireReaderLockPublic)},
    {FCFuncElement("AcquireWriterLock", NULL, (LPVOID) CRWLock::StaticAcquireWriterLockPublic)},
    {FCFuncElement("ReleaseReaderLock", NULL, (LPVOID) CRWLock::StaticReleaseReaderLockPublic)},
    {FCFuncElement("ReleaseWriterLock", NULL, (LPVOID) CRWLock::StaticReleaseWriterLockPublic)},
    {FCFuncElement("UpgradeToWriterLock", NULL, (LPVOID) CRWLock::StaticUpgradeToWriterLockPublic)},
    {FCFuncElement("DowngradeFromWriterLock", NULL, (LPVOID) CRWLock::StaticDowngradeFromWriterLock)},
    {FCFuncElement("ReleaseLock", NULL, (LPVOID) CRWLock::StaticReleaseLock)},
    {FCFuncElement("RestoreLock", NULL, (LPVOID) CRWLock::StaticRestoreLockPublic)},
    {FCFuncElement("PrivateGetIsReaderLockHeld", NULL, (LPVOID) CRWLock::StaticIsReaderLockHeld)},
    {FCFuncElement("PrivateGetIsWriterLockHeld", NULL, (LPVOID) CRWLock::StaticIsWriterLockHeld)},
    {FCFuncElement("PrivateGetWriterSeqNum", NULL, (LPVOID) CRWLock::StaticGetWriterSeqNum)},
    {FCFuncElement("AnyWritersSince", NULL, (LPVOID) CRWLock::StaticAnyWritersSince)},
    {FCFuncElement("PrivateInitialize", NULL, (LPVOID) CRWLock::StaticPrivateInitialize)},
    {NULL, NULL, NULL, NULL}
};

static
ECFunc  gMessageFuncs[] =
{
    {FCFuncElement("nGetMetaSigLen",         NULL, (PVOID)   CMessage::GetMetaSigLen)},
    {FCFuncElement("InternalGetArgCount",           NULL, (PVOID)   CMessage::GetArgCount)},
    {FCFuncElement("InternalHasVarArgs",            NULL, (PVOID)   CMessage::HasVarArgs)},
    {FCFuncElement("GetVarArgsPtr",         NULL, (PVOID)   CMessage::GetVarArgsPtr)},
    {FCFuncElement("InternalGetArg",         NULL, (PVOID)  CMessage::GetArg)},
    {FCFuncElement("InternalGetArgs",        NULL, (PVOID)  CMessage::GetArgs)},
    {FCFuncElement("InternalGetMethodBase",  NULL, (PVOID)  CMessage::GetMethodBase)},
    {FCFuncElement("InternalGetMethodName",  NULL, (PVOID)  CMessage::GetMethodName)},
    {FCFuncElement("PropagateOutParameters", NULL, (PVOID)  CMessage::PropagateOutParameters)},
    {FCFuncElement("GetReturnValue",         NULL, (PVOID)  CMessage::GetReturnValue)},
    {FCFuncElement("Init",                   NULL, (PVOID)  CMessage::Init)},
    {FCFuncElement("GetAsyncBeginInfo",      NULL, (PVOID)  CMessage::GetAsyncBeginInfo)},
    {FCFuncElement("GetAsyncResult",         NULL, (PVOID)  CMessage::GetAsyncResult)},
    {FCFuncElement("GetThisPtr",             NULL, (PVOID)  CMessage::GetAsyncObject)},
    {FCFuncElement("OutToUnmanagedDebugger", NULL, (PVOID)  CMessage::DebugOut)},
    {FCFuncElement("Dispatch",               NULL, (PVOID)  CMessage::Dispatch)},
    {FCFuncElement("MethodAccessCheck",      NULL, (PVOID)  CMessage::MethodAccessCheck)},
    {NULL, NULL, NULL, NULL}
};

static
ECFunc gConvertFuncs[] = 
{
    {FCFuncElement("ToBase64String",    NULL, (LPVOID)  BitConverter::ByteArrayToBase64String)},
    {FCFuncElement("FromBase64String",  NULL, (LPVOID)  BitConverter::Base64StringToByteArray)},
        {FCFuncElement("ToBase64CharArray",  NULL, (LPVOID)  BitConverter::ByteArrayToBase64CharArray)},
        {FCFuncElement("FromBase64CharArray",  NULL, (LPVOID)  BitConverter::Base64CharArrayToByteArray)},
        {NULL, NULL, NULL, NULL}
};



static
ECFunc  gChannelServicesFuncs[] =
{
    {FCFuncElement("GetPrivateContextsPerfCounters", NULL, (PVOID) GetPrivateContextsPerfCountersEx)},
    {FCFuncElement("GetGlobalContextsPerfCounters", NULL, (PVOID) GetGlobalContextsPerfCountersEx)},
    {NULL, NULL, NULL, NULL}
};


static
ECFunc  gCloningFuncs[] =
{
    {FCFuncElement("GetEmptyArrayForCloning", NULL, (PVOID) SystemNative::GetEmptyArrayForCloning)},
    {NULL, NULL, NULL, NULL}
};

static
ECFunc  gEnumFuncs[] =
{
    {FCFuncElement("InternalGetUnderlyingType", NULL, (LPVOID) COMMember::InternalGetEnumUnderlyingType)},
    {FCFuncElement("InternalGetValue", NULL, (LPVOID) COMMember::InternalGetEnumValue)},
    {FCFuncElement("InternalGetEnumValues", NULL, (LPVOID) COMMember::InternalGetEnumValues)},
    {FCFuncElement("InternalBoxEnumI8", NULL, (PVOID) COMMember::InternalBoxEnumI8)},
    {FCFuncElement("InternalBoxEnumU8", NULL, (PVOID) COMMember::InternalBoxEnumU8)},
    {NULL, NULL, NULL, NULL}
};

static
ECFunc  gStackBuilderSinkFuncs[] =
{
    {FCFuncElement("PrivateProcessMessage",  NULL, (PVOID)  CStackBuilderSink::PrivateProcessMessage)},
    {NULL, NULL, NULL, NULL}
};

#include "comvarargs.h"

static
ECFunc gParseNumbersFuncs[] =
{
  {FCFuncElement("IntToDecimalString", NULL, (LPVOID)ParseNumbers::IntToDecimalString)},
  {FCFuncElement("IntToString", NULL, (LPVOID)ParseNumbers::IntToString)},
  {FCFuncElement("LongToString", NULL,(LPVOID)ParseNumbers::LongToString)},
  {FCFuncElement("StringToInt", NULL, (LPVOID)ParseNumbers::StringToInt)},
  {FCFuncElement("StringToLong", NULL, (LPVOID)ParseNumbers::StringToLong)},
  {FCFuncElement("RadixStringToLong", NULL, (LPVOID)ParseNumbers::RadixStringToLong)},
  {NULL, NULL, NULL}
};

static
ECFunc gTimeZoneFuncs[] =
{
    {FCFuncElement("nativeGetTimeZoneMinuteOffset",    NULL, (LPVOID)COMNlsInfo::nativeGetTimeZoneMinuteOffset)},
    {FCFuncElement("nativeGetStandardName",         NULL, (LPVOID)COMNlsInfo::nativeGetStandardName)},
    {FCFuncElement("nativeGetDaylightName",         NULL, (LPVOID)COMNlsInfo::nativeGetDaylightName)},
    {FCFuncElement("nativeGetDaylightChanges",      NULL, (LPVOID)COMNlsInfo::nativeGetDaylightChanges)},
    {NULL, NULL, NULL}
};

static
ECFunc gGuidFuncs[] =
{
  {FCFuncElement("CompleteGuid", NULL, (LPVOID)GuidNative::CompleteGuid)},
  {NULL, NULL, NULL}
};


static
ECFunc  gObjectFuncs[] =
{
    {FCFuncElement("GetExistingType", NULL, (LPVOID) ObjectNative::GetExistingClass)},
    {FCFuncElement("FastGetExistingType", NULL, (LPVOID) ObjectNative::FastGetClass)},
    {FCFuncElement("GetHashCode", NULL, (LPVOID)ObjectNative::GetHashCode)},
    {FCFuncElement("Equals", NULL, (LPVOID)ObjectNative::Equals)},
    {FCFuncElement("InternalGetType", NULL, (LPVOID)ObjectNative::GetClass)},
    {FCFuncElement("MemberwiseClone", NULL, (LPVOID)ObjectNative::Clone)},
    {NULL, NULL, NULL}
};

#ifdef _X86_
#define FastAllocateStringGeneric NULL
#else
FCDECL1(StringObject*, FastAllocateStringGeneric, DWORD cchString);
#endif

static
ECFunc  gStringFuncs[] =
{
    //FastAllocateString is set dynamically and must be kept as the first element in this array.
    //The actual set is done in JitInterfacex86.cpp
  {FCFuncElement("FastAllocateString",    NULL,                     (LPVOID)FastAllocateStringGeneric)},
  {FCFuncElement("Equals",               &gsig_IM_Obj_RetBool,                         (LPVOID)COMString::EqualsObject)},
  {FCFuncElement("Equals",               &gsig_IM_Str_RetBool,                         (LPVOID)COMString::EqualsString)},
  {FCFuncElement("FillString",            NULL,                     (LPVOID)COMString::FillString)},
  {FCFuncElement("FillStringChecked",     NULL,                     (LPVOID)COMString::FillStringChecked)},
  {FCFuncElement("FillStringEx",          NULL,                     (LPVOID)COMString::FillStringEx)},
  {FCFuncElement("FillSubstring",         NULL,                     (LPVOID)COMString::FillSubstring)},
  {FCFuncElement("FillStringArray",       NULL,                     (LPVOID)COMString::FillStringArray)},
  {FCFuncElement("nativeCompareOrdinal",  NULL,                     (LPVOID)COMString::FCCompareOrdinal)},
  {FCFuncElement("nativeCompareOrdinalWC",NULL,                     (LPVOID)COMString::FCCompareOrdinalWC)},
  {FCFuncElement("GetHashCode",           NULL,                     (LPVOID)COMString::GetHashCode)},
  {FCIntrinsic("InternalLength",          NULL,                     (LPVOID)COMString::Length, CORINFO_INTRINSIC_StringLength)},
  {FCIntrinsic("InternalGetChar",         NULL,                     (LPVOID)COMString::GetCharAt, CORINFO_INTRINSIC_StringGetChar)},
  {FCFuncElement("CopyTo",                NULL,                     (LPVOID)COMString::GetPreallocatedCharArray)},
  {FCFuncElement("CopyToByteArray",       NULL,                     (LPVOID)COMString::InternalCopyToByteArray)},
   {FCFuncElement("InternalCopyTo",        NULL,                     (LPVOID)COMString::GetPreallocatedCharArray)},
  {FCFuncElement("IsFastSort",            NULL,                     (LPVOID)COMString::IsFastSort)},
#ifdef _DEBUG
  {FCFuncElement("ValidModifiableString", NULL,                     (LPVOID)COMString::ValidModifiableString)},
#endif
  {FCFuncElement("nativeCompareOrdinalEx",NULL,                     (LPVOID)COMString::CompareOrdinalEx)},
  {FCFuncElement("IndexOf",               &gsig_IM_Char_Int_Int_RetInt,        (LPVOID)COMString::IndexOfChar)},
  {FCFuncElement("IndexOfAny",               &gsig_IM_ArrChar_Int_Int_RetInt, (LPVOID)COMString::IndexOfCharArray)},
  {FCFuncElement("LastIndexOf",           &gsig_IM_Char_Int_Int_RetInt,        (LPVOID)COMString::LastIndexOfChar)},
  {FCFuncElement("LastIndexOfAny",           &gsig_IM_ArrChar_Int_Int_RetInt,   (LPVOID)COMString::LastIndexOfCharArray)},
  {FCFuncElement("nativeSmallCharToUpper",NULL,                     (LPVOID)COMString::SmallCharToUpper)},

  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_ArrChar_Int_Int_RetVoid,            (LPVOID)COMString::StringInitCharArray)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_ArrChar_RetVoid,                    (LPVOID)COMString::StringInitChars)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_PtrChar_RetVoid,                    (LPVOID)COMString::StringInitWCHARPtr)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_PtrChar_Int_Int_RetVoid,            (LPVOID)COMString::StringInitWCHARPtrPartial)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_PtrSByt_RetVoid,                    (LPVOID)COMString::StringInitCharPtr)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_PtrSByt_Int_Int_RetVoid,            (LPVOID)COMString::StringInitCharPtrPartial)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_Char_Int_RetVoid,                   (LPVOID)COMString::StringInitCharCount)},
  {FCFuncElement(COR_CTOR_METHOD_NAME,    &gsig_IM_PtrSByt_Int_Int_Encoding_RetVoid,   (LPVOID)COMString::StringInitSBytPtrPartialEx)},
  {FCFuncElement("Join",                  &gsig_SM_Str_ArrStr_Int_Int_RetStr,           (LPVOID)COMString::JoinArray)},
  {FCFuncElement("Substring",             NULL,                                         (LPVOID)COMString::Substring)},
  {FCFuncElement("TrimHelper",            NULL,                                         (LPVOID)COMString::TrimHelper)},
  {FCFuncElement("Split",                 NULL,                                         (LPVOID)COMString::Split)},
  {FCFuncElement("Remove",                NULL,                                         (LPVOID)COMString::Remove)},
  {FCFuncElement("Replace",               &gsig_IM_Char_Char_RetStr,                    (LPVOID)COMString::Replace)},
  {FCFuncElement("Replace",               &gsig_IM_Str_Str_RetStr,                      (LPVOID)COMString::ReplaceString)},
  {FCFuncElement("Insert",                NULL,                                         (LPVOID)COMString::Insert)},
  {FCFuncElement("PadHelper",             NULL,                                         (LPVOID)COMString::PadHelper)},
  {NULL, NULL, NULL}
};

//In order for our code that sets up the fast string allocator to work,
//FastAllocateString must always be the first method in gStringFuncs.
//Code in JitInterfacex86.cpp assigns the value to m_pImplementation after
//that code has been dynamically generated.
LPVOID *FCallFastAllocateStringImpl = &(gStringFuncs[0].m_pImplementation);

ECFunc  gStringBufferFuncs[] =
{
  {FCFuncElement("InternalGetCurrentThread", NULL,                      (LPVOID)COMStringBuffer::GetCurrentThread)},
  {FCFuncElement("InternalSetCapacity",NULL,                            (LPVOID)COMStringBuffer::SetCapacity)},
  {FCFuncElement("Insert",&gsig_IM_Int_ArrChar_Int_Int_RetStringBuilder,(LPVOID)COMStringBuffer::InsertCharArray)},
  {FCFuncElement("Insert",&gsig_IM_Int_Str_Int_RetStringBuilder,        (LPVOID)COMStringBuffer::InsertString)},
  {FCFuncElement("Remove",NULL,                                         (LPVOID)COMStringBuffer::Remove)},
  {FCFuncElement("MakeFromString",NULL,                                 (LPVOID)COMStringBuffer::MakeFromString)},
  {FCFuncElement("Replace", &gsig_IM_Str_Str_Int_Int_RetStringBuilder,  (LPVOID)COMStringBuffer::ReplaceString)},
  {NULL, NULL, NULL}
};

static
ECFunc gValueTypeFuncs[] =
{
  {FCFuncElement("GetMethodTablePtrAsInt", NULL, (LPVOID)ValueTypeHelper::GetMethodTablePtr)},
  {FCFuncElement("CanCompareBits", NULL, (LPVOID)ValueTypeHelper::CanCompareBits)},
  {FCFuncElement("FastEqualsCheck", NULL, (LPVOID)ValueTypeHelper::FastEqualsCheck)},
  {NULL, NULL, NULL}
};

static
ECFunc gDiagnosticsDebugger[] =
{
    {FCFuncElement("BreakInternal",       NULL, (LPVOID)DebugDebugger::Break)},
    {FCFuncElement("LaunchInternal",      NULL, (LPVOID)DebugDebugger::Launch )},
    {FCFuncElement("IsDebuggerAttached",  NULL, (LPVOID)DebugDebugger::IsDebuggerAttached )}, /**/
    {FCFuncElement("Log",                 NULL, (LPVOID)DebugDebugger::Log )},
    {FCFuncElement("IsLogging",           NULL, (LPVOID)DebugDebugger::IsLogging )},
    {NULL, NULL,NULL}
};


static
ECFunc gDiagnosticsStackTrace[] =
{
    {FCFuncElement("GetStackFramesInternal",      NULL, (LPVOID)DebugStackTrace::GetStackFramesInternal )},
    {NULL, NULL,NULL}
};


static
ECFunc gDiagnosticsLog[] =
{
    {FCFuncElement("AddLogSwitch", NULL, (LPVOID)Log::AddLogSwitch)},
    {FCFuncElement("ModifyLogSwitch", NULL, (LPVOID)Log::ModifyLogSwitch)},
    {NULL, NULL,NULL}
};


static
ECFunc gDiagnosticsAssert[] =
{
    {FCFuncElement("ShowDefaultAssertDialog", NULL, (LPVOID)DebuggerAssert::ShowDefaultAssertDialog)},
    {NULL, NULL,NULL}
};


static ECFunc gEnvironmentFuncs[] =
{
     {FCFuncElement("nativeGetTickCount", NULL, (LPVOID)SystemNative::GetTickCount)},
     {FCFuncElement("ExitNative",                       NULL, (LPVOID)SystemNative::Exit)},
     {FCFuncElement("nativeSetExitCode",                NULL, (LPVOID)SystemNative::SetExitCode)},
     {FCFuncElement("nativeGetExitCode",                NULL, (LPVOID)SystemNative::GetExitCode)},
     {FCFuncElement("nativeGetWorkingSet",NULL, (LPVOID)SystemNative::GetWorkingSet)},
     {FCFuncElement("nativeGetEnvironmentVariable",     NULL, (LPVOID)SystemNative::_GetEnvironmentVariable)}, /**/
     {FCFuncElement("nativeHasShutdownStarted",NULL, (LPVOID)SystemNative::HasShutdownStarted)},
     {FCFuncElement("nativeGetVersion",                 NULL, (LPVOID)SystemNative::GetVersionString)},
     {FCFuncElement("nativeGetEnvironmentCharArray",    NULL, (LPVOID)SystemNative::GetEnvironmentCharArray)},
     {FCFuncElement("GetCommandLineNative",             NULL, (LPVOID)SystemNative::_GetCommandLine)},
     {FCFuncElement("GetCommandLineArgsNative",         NULL, (LPVOID)SystemNative::GetCommandLineArgs)},
     {NULL, NULL, NULL}
};

static ECFunc gRuntimeEnvironmentFuncs[] =
{
     {FCFuncElement("GetModuleFileName",        NULL, (LPVOID)SystemNative::_GetModuleFileName)},
     {FCFuncElement("GetDeveloperPath",         NULL, (LPVOID)SystemNative::GetDeveloperPath)},
     {FCFuncElement("GetRuntimeDirectoryImpl",  NULL, (LPVOID)SystemNative::GetRuntimeDirectory)},
     {FCFuncElement("GetHostBindingFile",       NULL, (LPVOID)SystemNative::GetHostBindingFile)},
     {FCFuncElement("FromGlobalAccessCache",    NULL, (LPVOID)SystemNative::FromGlobalAccessCache)},
     {NULL, NULL, NULL}
};

static
ECFunc gSerializationFuncs[] =
{
    {FCFuncElement("nativeGetUninitializedObject", NULL, (LPVOID)COMClass::GetUninitializedObject)},
    {FCFuncElement("nativeGetSerializableMembers", NULL, (LPVOID)COMClass::GetSerializableMembers)},
    {FCFuncElement("GetSerializationRegistryValues", NULL, (LPVOID)COMClass::GetSerializationRegistryValues)},
    {NULL, NULL, NULL}
};

static
ECFunc gExceptionFuncs[] =
{
    {FCFuncElement("GetClassName",        NULL, (LPVOID)ExceptionNative::GetClassName)},
    {FCFuncElement("InternalGetMethod",   NULL, (LPVOID)SystemNative::CaptureStackTraceMethod)},
    {NULL, NULL, NULL}
};

static
ECFunc gFileStreamFuncs[] =
{
    {FCFuncElement("RunningOnWinNTNative",    NULL, (LPVOID)COMStreams::RunningOnWinNT)},
    {FCFuncElement("nGetFullPathHelper",      NULL, (LPVOID)COMStreams::GetFullPathHelper)},
    {NULL, NULL, NULL}
};

static
ECFunc gFusionWrapFuncs[] =
{
    {FCFuncElement("GetNextAssembly", NULL, (LPVOID) FusionWrap::GetNextAssembly)},
    {FCFuncElement("GetDisplayName", NULL, (LPVOID) FusionWrap::GetDisplayName)},
    {FCFuncElement("ReleaseFusionHandle", NULL, (LPVOID) FusionWrap::ReleaseFusionHandle)},
    {NULL, NULL, NULL}
};

static
ECFunc gConsoleFuncs[] =
{
    {FCFuncElement("ConsoleHandleIsValidNative", NULL, (LPVOID) COMStreams::ConsoleHandleIsValid)},
    {FCFuncElement("GetConsoleCPNative", NULL, (LPVOID) COMStreams::ConsoleInputCP)}, /**/
    {FCFuncElement("GetConsoleOutputCPNative", NULL, (LPVOID) COMStreams::ConsoleOutputCP)}, /**/
    {NULL, NULL, NULL}
};


static
ECFunc gCodePageEncodingFuncs[] =
{
    {FCFuncElement("BytesToUnicodeNative",   NULL, (LPVOID)COMStreams::BytesToUnicode )},
    {FCFuncElement("UnicodeToBytesNative",   NULL, (LPVOID)COMStreams::UnicodeToBytes )},
    {FCFuncElement("GetCPMaxCharSizeNative", NULL, (LPVOID)COMStreams::GetCPMaxCharSize )}, /**/
    {NULL, NULL, NULL}
};


static
ECFunc gTypedReferenceFuncs[] =
{
    {FCFuncElement("ToObject",                          NULL,     (LPVOID)COMMember::TypedReferenceToObject)},
    {FCFuncElement("InternalObjectToTypedReference",    NULL,     (LPVOID)COMMember::ObjectToTypedReference)},
    {FCFuncElement("InternalMakeTypedReferences",       NULL,     (LPVOID)COMMember::MakeTypedReference)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMClassFuncs[] =
{
    {FCFuncElement("IsPrimitiveImpl",          NULL, (LPVOID) COMClass::IsPrimitive)},
    {FCFuncElement("GetAttributeFlagsImpl",    NULL, (LPVOID) COMClass::GetAttributeFlags)},
    {FCFuncElement("InternalGetTypeDefTokenHelper",  NULL, (LPVOID) COMClass::GetTypeDefToken)},
    {FCFuncElement("InternalIsContextful",     NULL, (LPVOID) COMClass::IsContextful)},
    {FCFuncElement("InternalIsMarshalByRef",   NULL, (LPVOID) COMClass::IsMarshalByRef)},
    {FCFuncElement("InternalHasProxyAttribute",   NULL, (LPVOID) COMClass::HasProxyAttribute)},
    {FCFuncElement("InternalGetTypeHandleFromObject",  NULL, (LPVOID) COMClass::GetTHFromObject)},
    {FCFuncElement("IsByRefImpl",              NULL, (LPVOID) COMClass::IsByRefImpl)},
    {FCFuncElement("IsPointerImpl",            NULL, (LPVOID) COMClass::IsPointerImpl)},
    {FCFuncElement("GetNestedTypes",           NULL, (LPVOID) COMClass::GetNestedTypes)},
    {FCFuncElement("GetNestedType",            NULL, (LPVOID) COMClass::GetNestedType)},
    {FCFuncElement("IsNestedTypeImpl",         NULL, (LPVOID) COMClass::IsNestedTypeImpl)},
    {FCFuncElement("InternalGetNestedDeclaringType",    NULL, (LPVOID) COMClass::GetNestedDeclaringType)},
    {FCFuncElement("InternalGetInterfaceMap",  NULL, (LPVOID) COMClass::GetInterfaceMap)},
    {FCFuncElement("IsSubclassOf",             NULL, (LPVOID) COMClass::IsSubClassOf)},
    {FCFuncElement("CanCastTo",                NULL, (LPVOID) COMClass::CanCastTo)}, /**/
    {FCFuncElement("GetTypeInternal",          NULL, (LPVOID) COMClass::GetClassInternal)}, /**/
    {FCFuncElement("GetField",                 NULL, (LPVOID) COMClass::GetField)}, /**/
    {FCFuncElement("InternalGetSuperclass",    NULL, (LPVOID) COMClass::GetSuperclass)}, /**/
    {FCFuncElement("InternalGetClassHandle",   NULL, (LPVOID) COMClass::GetClassHandle)},
    {FCFuncElement("GetTypeFromHandleImpl",    NULL, (LPVOID) COMClass::GetClassFromHandle)},
    {FCFuncElement("InternalGetName",          NULL, (LPVOID) COMClass::GetName)}, /**/
    {FCFuncElement("InternalGetProperlyQualifiedName",NULL, (LPVOID) COMClass::GetProperName)},
    {FCFuncElement("InternalGetFullName",      NULL, (LPVOID) COMClass::GetFullName)}, /**/
    {FCFuncElement("InternalGetAssemblyQualifiedName",NULL, (LPVOID) COMClass::GetAssemblyQualifiedName)},
    {FCFuncElement("InternalGetNameSpace",     NULL, (LPVOID) COMClass::GetNameSpace)},
    {FCFuncElement("GetElementType",           NULL, (LPVOID) COMClass::GetArrayElementType)},
    {FCFuncElement("GetTypeImpl",              NULL,                            (LPVOID) COMClass::GetClass)}, /**/
    {FCFuncElement("GetType",                  &gsig_SM_Str_RetType,            (LPVOID) COMClass::GetClass1Arg)}, /**/
    {FCFuncElement("GetType",                  &gsig_SM_Str_Bool_RetType,       (LPVOID) COMClass::GetClass2Args)}, /**/
    {FCFuncElement("GetType",                  &gsig_SM_Str_Bool_Bool_RetType,  (LPVOID) COMClass::GetClass3Args)}, /**/
    // This one is found in COMMember because it basically a constructor method
    {FCFuncElement("CreateInstanceImpl",       NULL, (LPVOID) COMMember::CreateInstance)}, /**/
    {FCFuncElement("InternalGetFields",        NULL, (LPVOID) COMClass::GetFields)}, /**/
    {FCFuncElement("InternalGetModule",        NULL, (LPVOID) COMClass::GetModule)},
    {FCFuncElement("InternalGetAssembly",      NULL, (LPVOID) COMClass::GetAssembly)},
    {FCFuncElement("nGetMethodFromCache",      NULL, (LPVOID) COMClass::GetMethodFromCache)},
    {FCFuncElement("nAddMethodToCache",        NULL, (LPVOID) COMClass::AddMethodToCache)},
    {FCFuncElement("GetMemberCons",            NULL, (LPVOID) COMClass::GetMemberCons)}, /**/
    {FCFuncElement("GetMatchingProperties",    NULL, (LPVOID) COMClass::GetMatchingProperties)},

    {FCFuncElement("InternalGetGUID",          NULL, (LPVOID) COMClass::GetGUID)},
    {FCFuncElement("IsArrayImpl",              NULL, (LPVOID) COMClass::IsArray)},
    {FCFuncElement("InvalidateCachedNestedType",NULL, (LPVOID) COMClass::InvalidateCachedNestedType)}, /**/
    {FCFuncElement("GetInterfaces",            NULL, (LPVOID) COMClass::GetInterfaces)},
    {FCFuncElement("GetInterface",             NULL, (LPVOID) COMClass::GetInterface)},
    {FCFuncElement("GetConstructorsInternal",  NULL, (LPVOID) COMClass::GetConstructors)},
    {FCFuncElement("GetMethods",               NULL, (LPVOID) COMClass::GetMethods)},
    {FCFuncElement("GetEvent",                 NULL, (LPVOID) COMClass::GetEvent)},
    {FCFuncElement("GetEvents",                NULL, (LPVOID) COMClass::GetEvents)},
    {FCFuncElement("GetProperties",            NULL, (LPVOID) COMClass::GetProperties)},
    {FCFuncElement("GetMember",                NULL, (LPVOID) COMClass::GetMember)},
    {FCFuncElement("GetMembers",               NULL, (LPVOID) COMClass::GetMembers)},
    {FCFuncElement("GetMemberMethod",          NULL, (LPVOID) COMClass::GetMemberMethods)},
    {FCFuncElement("GetMemberField",           NULL, (LPVOID) COMClass::GetMemberField)},
    {FCFuncElement("GetMemberProperties",      NULL, (LPVOID) COMClass::GetMemberProperties)},
    {FCFuncElement("SupportsInterface",        NULL, (LPVOID) COMClass::SupportsInterface)},
    {FCFuncElement("InternalGetArrayRank",     NULL, (LPVOID) COMClass::InternalGetArrayRank)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMMethodFuncs[] =
{
    {FCFuncElement("GetMethodHandleImpl",           NULL, (LPVOID) COMMember::GetMethodHandleImpl)},
    {FCFuncElement("GetMethodFromHandleImp",    NULL, (LPVOID) COMMember::GetMethodFromHandleImp)},
    {FCFuncElement("InternalGetName",          NULL, (LPVOID) COMMember::GetMethodName)}, /**/
    {FCFuncElement("InternalGetReturnType",    NULL, (LPVOID) COMMember::GetReturnType)},
    {FCFuncElement("InternalInvoke",           NULL, (LPVOID) COMMember::InvokeMethod)}, /**/
    {FCFuncElement("InternalToString",         NULL, (LPVOID) COMMember::GetMethodInfoToString)},

    // All of these functions are shared between both constructors and methods....
    {FCFuncElement("InternalDeclaringClass",           NULL, (LPVOID) COMMember::GetDeclaringClass)}, /**/
    {FCFuncElement("InternalReflectedClass",           NULL, (LPVOID) COMMember::GetReflectedClass)}, /**/
    {FCFuncElement("InternalGetAttributeFlags",        NULL, (LPVOID) COMMember::GetAttributeFlags)},
    {FCFuncElement("InternalGetCallingConvention",     NULL, (LPVOID) COMMember::GetCallingConvention)},
    {FCFuncElement("GetMethodImplementationFlags",     NULL, (LPVOID) COMMember::GetMethodImplFlags)},
    {FCFuncElement("InternalGetParameters",    NULL, (LPVOID) COMMember::GetParameterTypes)},
    {FCFuncElement("Equals",                   NULL, (LPVOID) COMMember::Equals)},
    {FCFuncElement("GetBaseDefinition",        NULL, (LPVOID) COMMember::GetBaseDefinition)},
    {FCFuncElement("GetParentDefinition",        NULL, (LPVOID) COMMember::GetParentDefinition)},
    {FCFuncElement("InternalGetCurrentMethod", NULL, (LPVOID) COMMember::InternalGetCurrentMethod)},
    {FCFuncElement("IsOverloadedInternal",     NULL, (LPVOID) COMMember::IsOverloaded)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMMethodHandleFuncs[] =
{
    {FCFuncElement("InternalGetFunctionPointer",        NULL, (LPVOID) COMMember::GetFunctionPointer)},
};

static
ECFunc gCOMEventFuncs[] =
{
    {FCFuncElement("InternalGetName",          NULL, (LPVOID) COMMember::GetEventName)}, /**/
    {FCFuncElement("InternalGetAttributeFlags",NULL, (LPVOID) COMMember::GetEventAttributeFlags)},
    {FCFuncElement("InternalDeclaringClass",   NULL, (LPVOID) COMMember::GetEventDeclaringClass)}, /**/
    {FCFuncElement("InternalReflectedClass",   NULL, (LPVOID) COMMember::GetReflectedClass)}, /**/
    {FCFuncElement("Equals",                   NULL, (LPVOID) COMMember::Equals)}, 
    {FCFuncElement("GetAddMethod",             NULL, (LPVOID) COMMember::GetAddMethod)},
    {FCFuncElement("GetRemoveMethod",          NULL, (LPVOID) COMMember::GetRemoveMethod)},
    {FCFuncElement("GetRaiseMethod",          NULL, (LPVOID) COMMember::GetRaiseMethod)},
    {FCFuncElement("InternalToString",         NULL, (LPVOID) COMMember::GetEventInfoToString)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMPropFuncs[] =
{
    {FCFuncElement("InternalGetName",          NULL, (LPVOID) COMMember::GetPropName)}, /**/
    {FCFuncElement("InternalGetType",          NULL, (LPVOID) COMMember::GetPropType)},
    {FCFuncElement("InternalToString",         NULL, (LPVOID) COMMember::GetPropInfoToString)},
    {FCFuncElement("InternalGetAttributeFlags",NULL, (LPVOID) COMMember::GetPropAttributeFlags)},
    {FCFuncElement("InternalDeclaringClass",   NULL, (LPVOID) COMMember::GetPropDeclaringClass)}, /**/
    {FCFuncElement("InternalReflectedClass",   NULL, (LPVOID) COMMember::GetReflectedClass)}, /**/
    {FCFuncElement("Equals",                   NULL, (LPVOID) COMMember::PropertyEquals)},
    {FCFuncElement("GetAccessors",             NULL, (LPVOID) COMMember::GetAccessors)},
    {FCFuncElement("InternalGetter",           NULL, (LPVOID) COMMember::InternalGetter)},
    {FCFuncElement("InternalSetter",           NULL, (LPVOID) COMMember::InternalSetter)},
    {FCFuncElement("InternalCanRead",          NULL, (LPVOID) COMMember::CanRead)},
    {FCFuncElement("InternalCanWrite",         NULL, (LPVOID) COMMember::CanWrite)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMDefaultBinderFuncs[] =
{
    {FCFuncElement("CanConvertPrimitive",        NULL, (LPVOID) COMMember::DBCanConvertPrimitive)},
    {FCFuncElement("CanConvertPrimitiveObjectToType", NULL, (LPVOID) COMMember::DBCanConvertObjectPrimitive)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMConstructorFuncs[] =
{
    {FCFuncElement("GetMethodHandleImpl",           NULL, (LPVOID) COMMember::GetMethodHandleImpl)},
    {FCFuncElement("InternalDeclaringClass",        NULL, (LPVOID) COMMember::GetDeclaringClass)}, /**/
    {FCFuncElement("Equals",                        NULL, (LPVOID) COMMember::Equals)},

    {FCFuncElement("GetParameters",                 NULL, (LPVOID) COMMember::GetParameterTypes)}, /**/
    {FCFuncElement("InternalInvoke",                NULL, (LPVOID) COMMember::InvokeCons)}, /**/
    {FCFuncElement("InternalGetName",               NULL, (LPVOID) COMMember::GetMethodName)}, /**/

    {FCFuncElement("InternalInvokeNoAllocation",    NULL, (LPVOID) COMMember::InvokeMethod)},
    {FCFuncElement("SerializationInvoke",           NULL, (LPVOID) COMMember::SerializationInvoke)},
    // All of these functions are shared between both constructors and methods....
    {FCFuncElement("InternalReflectedClass",        NULL, (LPVOID) COMMember::GetReflectedClass)}, /**/
    {FCFuncElement("InternalToString",              NULL, (LPVOID) COMMember::GetMethodInfoToString)},
    {FCFuncElement("InternalGetAttributeFlags",     NULL, (LPVOID) COMMember::GetAttributeFlags)},
    {FCFuncElement("GetMethodImplementationFlags",  NULL, (LPVOID) COMMember::GetMethodImplFlags)},
    {FCFuncElement("InternalGetCallingConvention",  NULL, (LPVOID) COMMember::GetCallingConvention)},
    {FCFuncElement("IsOverloadedInternal",          NULL, (LPVOID) COMMember::IsOverloaded)},
    {FCFuncElement("HasLinktimeDemand",             NULL, (LPVOID) COMMember::HasLinktimeDemand)},
   {NULL, NULL, NULL}
};

static
ECFunc gCOMFieldFuncs[] =
{
    {FCFuncElement("GetFieldHandleImpl",        NULL, (LPVOID) COMMember::GetFieldHandleImpl)},
    {FCFuncElement("GetFieldFromHandleImp",     NULL, (LPVOID) COMMember::GetFieldFromHandleImp)},
    {FCFuncElement("InternalGetValue",          NULL, (LPVOID) COMMember::FieldGet)}, /**/
    {FCFuncElement("InternalSetValue",          NULL, (LPVOID) COMMember::FieldSet)}, /**/

    {FCFuncElement("SetValueDirectImpl",       NULL, (LPVOID) COMMember::DirectFieldSet)},
    {FCFuncElement("GetValueDirectImpl",       NULL, (LPVOID) COMMember::DirectFieldGet)},
    {FCFuncElement("InternalGetName",          NULL, (LPVOID) COMMember::GetFieldName)}, /**/
    {FCFuncElement("InternalToString",         NULL, (LPVOID) COMMember::GetFieldInfoToString)},
    {FCFuncElement("InternalDeclaringClass",   NULL, (LPVOID) COMMember::GetFieldDeclaringClass)}, /**/
    {FCFuncElement("InternalReflectedClass",   NULL, (LPVOID) COMMember::GetReflectedClass)}, /**/
    {FCFuncElement("InternalGetSignature",     NULL, (LPVOID) COMMember::GetFieldSignature)},
    {FCFuncElement("InternalGetFieldType",     NULL, (LPVOID) COMMember::GetFieldType)}, /**/
    {FCFuncElement("InternalGetAttributeFlags",NULL, (LPVOID) COMMember::GetAttributeFlags)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMModuleFuncs[] =
{
    {FCFuncElement("GetFields",             NULL, (LPVOID) COMModule::GetFields)},
    {FCFuncElement("InternalGetField",       NULL, (LPVOID) COMModule::GetField)},
    {FCFuncElement("nGetAssembly",                      NULL, (LPVOID) COMModule::GetAssembly)},
    {FCFuncElement("IsDynamic",                         NULL, (LPVOID) COMModule::IsDynamic)},
    {FCFuncElement("InternalGetTypeToken",              NULL, (LPVOID) COMModule::GetClassToken)},
    {FCFuncElement("InternalGetTypeSpecToken",          NULL, (LPVOID) COMModule::GetTypeSpecToken)},
    {FCFuncElement("InternalGetTypeSpecTokenWithBytes", NULL, (LPVOID) COMModule::GetTypeSpecTokenWithBytes)},
    {FCFuncElement("InternalGetMemberRef",              NULL, (LPVOID) COMModule::GetMemberRefToken)},
    {FCFuncElement("InternalGetMemberRefOfMethodInfo",  NULL, (LPVOID) COMModule::GetMemberRefTokenOfMethodInfo)},
    {FCFuncElement("InternalGetMemberRefOfFieldInfo",   NULL, (LPVOID) COMModule::GetMemberRefTokenOfFieldInfo)},
    {FCFuncElement("InternalGetMemberRefFromSignature", NULL, (LPVOID) COMModule::GetMemberRefTokenFromSignature)},
    {FCFuncElement("InternalSetFieldRVAContent",        NULL, (LPVOID) COMModule::SetFieldRVAContent)},
    {FCFuncElement("InternalLoadInMemoryTypeByName",    NULL, (LPVOID) COMModule::LoadInMemoryTypeByName)},
    {FCFuncElement("nativeGetArrayMethodToken",         NULL, (LPVOID) COMModule::GetArrayMethodToken)},
    {FCFuncElement("pGetCaller",                        NULL, (LPVOID) COMModule::GetCaller)},
    {FCFuncElement("GetTypeInternal",                   NULL, (LPVOID) COMModule::GetClass)},
    {FCFuncElement("InternalGetName",                   NULL, (LPVOID) COMModule::GetName)}, /**/
    {FCFuncElement("GetTypesInternal",                  NULL, (LPVOID) COMModule::GetClasses)},
    {FCFuncElement("InternalGetStringConstant",         NULL, (LPVOID) COMModule::GetStringConstant)},
    {FCFuncElement("InternalSetModuleProps",            NULL, (LPVOID) COMModule::SetModuleProps)},
    {FCFuncElement("InteralGetFullyQualifiedName",      NULL, (LPVOID) COMModule::GetFullyQualifiedName)},
    {FCFuncElement("GetHINSTANCE",                      NULL, (LPVOID) COMModule::GetHINST)},
    {FCFuncElement("GetMethods",                        NULL, (LPVOID) COMModule::GetMethods)},
    {FCFuncElement("InternalGetMethod",                 NULL, (LPVOID) COMModule::InternalGetMethod)},
    {FCFuncElement("IsResource",                        NULL, (LPVOID) COMModule::IsResource)},

    {FCFuncElement("InternalPreSavePEFile",  NULL, (LPVOID) COMDynamicWrite::PreSavePEFile)},
    {FCFuncElement("InternalSavePEFile",     NULL, (LPVOID) COMDynamicWrite::SavePEFile)},
    {FCFuncElement("InternalAddResource",    NULL, (LPVOID) COMDynamicWrite::AddResource)},
    {FCFuncElement("InternalSetResourceCounts",         NULL, (LPVOID) COMDynamicWrite::SetResourceCounts)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMCustomAttributeFuncs[] =
{
    {FCFuncElement("GetMemberToken",            NULL, (LPVOID) COMCustomAttribute::GetMemberToken)},
    {FCFuncElement("GetMemberModule",           NULL, (LPVOID) COMCustomAttribute::GetMemberModule)},
    {FCFuncElement("GetAssemblyToken",          NULL, (LPVOID) COMCustomAttribute::GetAssemblyToken)},
    {FCFuncElement("GetAssemblyModule",         NULL, (LPVOID) COMCustomAttribute::GetAssemblyModule)},
    {FCFuncElement("GetModuleToken",            NULL, (LPVOID) COMCustomAttribute::GetModuleToken)},
    {FCFuncElement("GetModuleModule",           NULL, (LPVOID) COMCustomAttribute::GetModuleModule)},
    {FCFuncElement("GetMethodRetValueToken",    NULL, (LPVOID) COMCustomAttribute::GetMethodRetValueToken)},
    {FCFuncElement("IsCADefined",               NULL, (LPVOID) COMCustomAttribute::IsCADefined)},
    {FCFuncElement("GetCustomAttributeList",    NULL, (LPVOID) COMCustomAttribute::GetCustomAttributeList)},
    {FCFuncElement("CreateCAObject",            NULL, (LPVOID) COMCustomAttribute::CreateCAObject)},
    {FCFuncElement("GetDataForPropertyOrField", NULL, (LPVOID) COMCustomAttribute::GetDataForPropertyOrField)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMClassWriter[] =
{
    {FCFuncElement("InternalDefineClass",     NULL, (LPVOID) COMDynamicWrite::CWCreateClass)},
    {FCFuncElement("InternalSetParentType",   NULL, (LPVOID) COMDynamicWrite::CWSetParentType)},
    {FCFuncElement("InternalAddInterfaceImpl",NULL, (LPVOID) COMDynamicWrite::CWAddInterfaceImpl)},
    {FCFuncElement("InternalDefineMethod",    NULL, (LPVOID) COMDynamicWrite::CWCreateMethod)},
    {FCFuncElement("InternalSetMethodIL",     NULL, (LPVOID) COMDynamicWrite::CWSetMethodIL)},
    {FCFuncElement("TermCreateClass",         NULL, (LPVOID) COMDynamicWrite::CWTermCreateClass)},
    {FCFuncElement("InternalDefineField",     NULL, (LPVOID) COMDynamicWrite::CWCreateField)},
    {FCFuncElement("InternalSetPInvokeData",  NULL, (LPVOID) COMDynamicWrite::InternalSetPInvokeData)},
    {FCFuncElement("InternalDefineProperty",  NULL, (LPVOID) COMDynamicWrite::CWDefineProperty)},
    {FCFuncElement("InternalDefineEvent",     NULL, (LPVOID) COMDynamicWrite::CWDefineEvent)},
    {FCFuncElement("InternalDefineMethodSemantics",     NULL, (LPVOID) COMDynamicWrite::CWDefineMethodSemantics)},
    {FCFuncElement("InternalSetMethodImpl",   NULL, (LPVOID) COMDynamicWrite::CWSetMethodImpl)},
    {FCFuncElement("InternalDefineMethodImpl",   NULL, (LPVOID) COMDynamicWrite::CWDefineMethodImpl)},
    {FCFuncElement("InternalGetTokenFromSig", NULL, (LPVOID) COMDynamicWrite::CWGetTokenFromSig)},
    {FCFuncElement("InternalSetFieldOffset",  NULL, (LPVOID) COMDynamicWrite::CWSetFieldLayoutOffset)},
    {FCFuncElement("InternalSetClassLayout",  NULL, (LPVOID) COMDynamicWrite::CWSetClassLayout)},
    {FCFuncElement("InternalSetParamInfo",    NULL, (LPVOID) COMDynamicWrite::CWSetParamInfo)},
    {FCFuncElement("InternalSetMarshalInfo",    NULL, (LPVOID) COMDynamicWrite::CWSetMarshal)},
    {FCFuncElement("InternalSetConstantValue",    NULL, (LPVOID) COMDynamicWrite::CWSetConstantValue)},
    {FCFuncElement("InternalCreateCustomAttribute",  NULL, (LPVOID) COMDynamicWrite::CWInternalCreateCustomAttribute)},
    {FCFuncElement("InternalAddDeclarativeSecurity",  NULL, (LPVOID) COMDynamicWrite::CWAddDeclarativeSecurity)},
    {NULL, NULL, NULL}
};


static
ECFunc gCOMMethodRental[] =
{
    {FCFuncElement("SwapMethodBodyHelper",     NULL, (LPVOID) COMMethodRental::SwapMethodBody)},
    {NULL, NULL, NULL}
};


static
ECFunc gCOMCodeAccessSecurityEngineFuncs[] =
{
    {FCFuncElement("IncrementOverridesCount", NULL, (LPVOID)Security::IncrementOverridesCount)},
    {FCFuncElement("DecrementOverridesCount", NULL, (LPVOID)Security::DecrementOverridesCount)},
    {FCFuncElement("GetDomainPermissionListSet", NULL, (LPVOID)ApplicationSecurityDescriptor::GetDomainPermissionListSet)},
    {FCFuncElement("UpdateDomainPermissionListSet", NULL, (LPVOID)ApplicationSecurityDescriptor::UpdateDomainPermissionListSet)},
    {FCFuncElement("UpdateOverridesCount", NULL, (LPVOID)COMCodeAccessSecurityEngine::UpdateOverridesCount)},
    {FCFuncElement("GetResult", NULL, (LPVOID)COMCodeAccessSecurityEngine::GetResult)},
    {FCFuncElement("SetResult", NULL, (LPVOID)COMCodeAccessSecurityEngine::SetResult)},
    {FCFuncElement("ReleaseDelayedCompressedStack", NULL, (LPVOID)COMCodeAccessSecurityEngine::FcallReleaseDelayedCompressedStack)},
    {FCFuncElement("Check",                     NULL, (LPVOID)COMCodeAccessSecurityEngine::Check)},
    {FCFuncElement("CheckSet",                  NULL, (LPVOID)COMCodeAccessSecurityEngine::CheckSet)},
    {FCFuncElement("CheckNReturnSO",            NULL, (LPVOID)COMCodeAccessSecurityEngine::CheckNReturnSO)},
    {FCFuncElement("GetPermissionsP",           NULL, (LPVOID)COMCodeAccessSecurityEngine::GetPermissionsP)},
    {FCFuncElement("_GetGrantedPermissionSet",  NULL, (LPVOID)COMCodeAccessSecurityEngine::GetGrantedPermissionSet)},
    {FCFuncElement("GetCompressedStackN",       NULL, (LPVOID)COMCodeAccessSecurityEngine::EcallGetCompressedStack)}, /**/
    {FCFuncElement("GetDelayedCompressedStack", NULL, (LPVOID)COMCodeAccessSecurityEngine::EcallGetDelayedCompressedStack)}, /**/
    {FCFuncElement("InitSecurityEngine",        NULL, (LPVOID)COMCodeAccessSecurityEngine::InitSecurityEngine)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMSecurityManagerFuncs[] =
{
    {FCFuncElement("_IsSecurityOn", NULL, (LPVOID)Security::IsSecurityOnNative)},
    {FCFuncElement("GetGlobalFlags", NULL, (LPVOID)Security::GetGlobalSecurity)},
    {FCFuncElement("SetGlobalFlags", NULL, (LPVOID)Security::SetGlobalSecurity)},
    {FCFuncElement("SaveGlobalFlags", NULL, (LPVOID)Security::SaveGlobalSecurity)},
    {FCFuncElement("Log", NULL, (LPVOID)Security::Log)},
    {FCFuncElement("_GetGrantedPermissions", NULL, (LPVOID)Security::GetGrantedPermissions)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMSecurityZone[] =
{
    {FCFuncElement("_CreateFromUrl", NULL, (LPVOID)Security::CreateFromUrl)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMFileIOAccessFuncs[] =
{
    {FCFuncElement("_LocalDrive", NULL, (LPVOID)Security::LocalDrive)},
    {NULL, NULL, NULL}
};

static
ECFunc gCOMStringExpressionSetFuncs[] =
{
    {FCFuncElement("GetLongPathName", NULL, (LPVOID)Security::EcallGetLongPathName)},
    {NULL, NULL, NULL}
};


static
ECFunc gCOMUrlStringFuncs[] =
{
    {FCFuncElement("_GetDeviceName", NULL, (LPVOID)Security::GetDeviceName)},
    {NULL, NULL, NULL}
};



static
ECFunc gCOMSecurityRuntimeFuncs[] =
{
    {FCFuncElement("GetSecurityObjectForFrame", NULL, (LPVOID)COMSecurityRuntime::GetSecurityObjectForFrame)}, /**/
    {FCFuncElement("SetSecurityObjectForFrame", NULL, (LPVOID)COMSecurityRuntime::SetSecurityObjectForFrame)},
    {FCFuncElement("GetDeclaredPermissionsP", &gsig_IM_Obj_Int_RetPMS, (LPVOID)COMSecurityRuntime::GetDeclaredPermissionsP)},
    {FCFuncElement("InitSecurityRuntime", NULL, (LPVOID)COMSecurityRuntime::InitSecurityRuntime)},
    {NULL, NULL, NULL}
};


static ECFunc gBCLDebugFuncs[] =
{
    {FCFuncElement("GetRegistryValue", NULL, (LPVOID)ManagedLoggingHelper::GetRegistryLoggingValues )},
    {NULL, NULL, NULL}
};


static
ECFunc gAppDomainSetupFuncs[] =
{
    {FCFuncElement("UpdateContextProperty",    NULL, (LPVOID)AppDomainNative::UpdateContextProperty)},
    {NULL, NULL, NULL}
};

static
ECFunc gAppDomainFuncs[] =
{
    {FCFuncElement("GetDefaultDomain",          NULL, (LPVOID)AppDomainNative::GetDefaultDomain)},
    {FCFuncElement("GetFusionContext",          NULL, (LPVOID)AppDomainNative::GetFusionContext)},
    {FCFuncElement("IsStringInterned",          NULL, (LPVOID)AppDomainNative::IsStringInterned)},
    {FCFuncElement("IsUnloadingForcedFinalize", NULL, (LPVOID)AppDomainNative::IsUnloadingForcedFinalize)},
    {FCFuncElement("CreateBasicDomain",         NULL, (LPVOID)AppDomainNative::CreateBasicDomain)},
    {FCFuncElement("SetupDomainSecurity",       NULL, (LPVOID)AppDomainNative::SetupDomainSecurity)},
    {FCFuncElement("GetSecurityDescriptor",     NULL, (LPVOID)AppDomainNative::GetSecurityDescriptor)},
    {FCFuncElement("UpdateLoaderOptimization",  NULL, (LPVOID)AppDomainNative::UpdateLoaderOptimization)},
    {FCFuncElement("nGetFriendlyName",          NULL, (LPVOID)AppDomainNative::GetFriendlyName)},
    {FCFuncElement("GetAssemblies",             NULL, (LPVOID)AppDomainNative::GetAssemblies)},
    {FCFuncElement("nCreateDynamicAssembly",    NULL, (LPVOID)AppDomainNative::CreateDynamicAssembly)},
    {FCFuncElement("nExecuteAssembly",          NULL, (LPVOID)AppDomainNative::ExecuteAssembly)},
    {FCFuncElement("nUnload",                   NULL, (LPVOID)AppDomainNative::Unload)},
    {FCFuncElement("GetId",                     NULL, (LPVOID)AppDomainNative::GetId)},
    {FCFuncElement("nForcePolicyResolution",    NULL, (LPVOID)AppDomainNative::ForcePolicyResolution)},
    {FCFuncElement("GetOrInternString",         NULL, (LPVOID)AppDomainNative::GetOrInternString)},
    {FCFuncElement("GetDynamicDir",             NULL, (LPVOID)AppDomainNative::GetDynamicDir)},
    {FCFuncElement("nForceResolve",             NULL, (LPVOID)AppDomainNative::ForceResolve)},
    {FCFuncElement("IsTypeUnloading",           NULL, (LPVOID)AppDomainNative::IsTypeUnloading)},
    {FCFuncElement("nGetGrantSet",              NULL, (LPVOID)AppDomainNative::GetGrantSet)},
    {FCFuncElement("GetIdForUnload",            NULL, (LPVOID)AppDomainNative::GetIdForUnload)},
    {FCFuncElement("IsDomainIdValid",           NULL, (LPVOID)AppDomainNative::IsDomainIdValid)},
    {FCFuncElement("GetUnloadWorker",           NULL, (LPVOID)AppDomainNative::GetUnloadWorker)},
    {FCFuncElement("IsFinalizingForUnload",     NULL, (LPVOID)AppDomainNative::IsFinalizingForUnload)},
    {NULL, NULL, NULL}
};

static
ECFunc gAssemblyFuncs[] =
{
    {FCFuncElement("GetFullName",                       NULL,                           (LPVOID)AssemblyNative::GetStringizedName)},
    {FCFuncElement("GetLocation",                       NULL,                           (LPVOID)AssemblyNative::GetLocation)},
    {FCFuncElement("GetResource",                       NULL,                           (LPVOID)AssemblyNative::GetResource)},
    {FCFuncElement("nGetCodeBase",                      NULL,                           (LPVOID)AssemblyNative::GetCodeBase)},
    {FCFuncElement("nGetExecutingAssembly",             NULL,                           (LPVOID)AssemblyNative::GetExecutingAssembly)},
    {FCFuncElement("nGetFlags",                         NULL,                           (LPVOID)AssemblyNative::GetFlags)},
    {FCFuncElement("nGetHashAlgorithm",                 NULL,                           (LPVOID)AssemblyNative::GetHashAlgorithm)},
    {FCFuncElement("nGetLocale",                        NULL,                           (LPVOID)AssemblyNative::GetLocale)},
    {FCFuncElement("nGetPublicKey",                     NULL,                           (LPVOID)AssemblyNative::GetPublicKey)},
    {FCFuncElement("nGetSimpleName",                    NULL,                           (LPVOID)AssemblyNative::GetSimpleName)},
    {FCFuncElement("nGetVersion",                       NULL,                           (LPVOID)AssemblyNative::GetVersion)},
    {FCFuncElement("nLoad",                             NULL,                           (LPVOID)AssemblyNative::Load)}, /**/
    {FCFuncElement("nLoadImage",                        NULL,                           (LPVOID)AssemblyNative::LoadImage)},
    {FCFuncElement("nLoadModule",                       NULL,                           (LPVOID)AssemblyNative::LoadModuleImage)},
    {FCFuncElement("GetType",                           &gsig_IM_Str_RetType,           (LPVOID)AssemblyNative::GetType1Args)}, /**/
    {FCFuncElement("GetType",                           &gsig_IM_Str_Bool_RetType,      (LPVOID)AssemblyNative::GetType2Args)}, /**/
    {FCFuncElement("GetType",                           &gsig_IM_Str_Bool_Bool_RetType, (LPVOID)AssemblyNative::GetType3Args)}, /**/
    {FCFuncElement("GetTypeInternal",                   NULL,                           (LPVOID)AssemblyNative::GetTypeInternal)}, /**/
    {FCFuncElement("ParseTypeName",                     NULL,                           (LPVOID)AssemblyNative::ParseTypeName)}, /**/
    {FCFuncElement("nGetManifestResourceInfo",          NULL,                           (LPVOID)AssemblyNative::GetManifestResourceInfo)},
    {FCFuncElement("nGetModules",                       NULL,                           (LPVOID)AssemblyNative::GetModules)},
    {FCFuncElement("GetModule",                         NULL,                           (LPVOID)AssemblyNative::GetModule)},
    {FCFuncElement("GetReferencedAssemblies",           NULL,                           (LPVOID)AssemblyNative::GetReferencedAssemblies)},
    {FCFuncElement("GetExportedTypes",                  NULL,                           (LPVOID)AssemblyNative::GetExportedTypes)},
    {FCFuncElement("nGetManifestResourceNames",         NULL,                           (LPVOID)AssemblyNative::GetResourceNames)},
    {FCFuncElement("nPrepareForSavingManifestToDisk",   NULL,                           (LPVOID)AssemblyNative::PrepareSavingManifest)},
    {FCFuncElement("nSaveToFileList",                   NULL,                           (LPVOID)AssemblyNative::AddFileList)},
    {FCFuncElement("nSetHashValue",                     NULL,                           (LPVOID)AssemblyNative::SetHashValue)},
    {FCFuncElement("nSaveExportedType",                 NULL,                           (LPVOID)AssemblyNative::AddExportedType)},
    {FCFuncElement("nAddStandAloneResource",            NULL,                           (LPVOID)AssemblyNative::AddStandAloneResource)},
    {FCFuncElement("nSavePermissionRequests",           NULL,                           (LPVOID)AssemblyNative::SavePermissionRequests)},
    {FCFuncElement("nSaveManifestToDisk",               NULL,                           (LPVOID)AssemblyNative::SaveManifestToDisk)},
    {FCFuncElement("nAddFileToInMemoryFileList",        NULL,                           (LPVOID)AssemblyNative::AddFileToInMemoryFileList)},
    {FCFuncElement("nGetEntryPoint",                    NULL,                           (LPVOID)AssemblyNative::GetEntryPoint)},
    {FCFuncElement("GetEntryAssembly",                  NULL,                           (LPVOID)AssemblyNative::GetEntryAssembly)},
    {FCFuncElement("CreateQualifiedName",               NULL,                           (LPVOID)AssemblyNative::CreateQualifiedName)},
    {FCFuncElement("nForceResolve",                     NULL,                           (LPVOID)AssemblyNative::ForceResolve)},
    {FCFuncElement("nGetGrantSet",                      NULL,                           (LPVOID)AssemblyNative::GetGrantSet)},
    {FCFuncElement("nGetOnDiskAssemblyModule",          NULL,                           (LPVOID)AssemblyNative::GetOnDiskAssemblyModule)},
    {FCFuncElement("nGetInMemoryAssemblyModule",        NULL,                           (LPVOID)AssemblyNative::GetInMemoryAssemblyModule)},
    {FCFuncElement("nGlobalAssemblyCache",              NULL,                           (LPVOID)AssemblyNative::GlobalAssemblyCache)},
    {FCFuncElement("nDefineDynamicModule",              NULL,                           (LPVOID)COMModule::DefineDynamicModule)},
    {FCFuncElement("nGetEvidence",                      NULL,                           (LPVOID)Security::GetEvidence)},
    {NULL, NULL, NULL}
};

static
ECFunc gAssemblyNameFuncs[] =
{
    {FCFuncElement("nGetFileInformation",   NULL, (LPVOID)AssemblyNameNative::GetFileInformation)},
    {FCFuncElement("nToString",             NULL, (LPVOID)AssemblyNameNative::ToString)},
    {FCFuncElement("nGetPublicKeyToken",    NULL, (LPVOID)AssemblyNameNative::GetPublicKeyToken)},
    {FCFuncElement("EscapeCodeBase", NULL, (LPVOID)AssemblyNameNative::EscapeCodeBase)},
   {NULL, NULL, NULL}
};

static
ECFunc gCharacterFuncs[] =
{
    {FCFuncElement("ToString", NULL, (LPVOID)COMCharacter::ToString)},
    {NULL, NULL, NULL}
};

static
ECFunc gDelegateFuncs[] =
{
    {FCFuncElement("InternalCreate",        NULL, (LPVOID) COMDelegate::InternalCreate)}, /**/
    {FCFuncElement("InternalCreateStatic",  NULL, (LPVOID) COMDelegate::InternalCreateStatic)},
    {FCFuncElement("InternalAlloc",         NULL, (LPVOID) COMDelegate::InternalAlloc)},
    {FCFuncElement("InternalCreateMethod",  NULL, (LPVOID) COMDelegate::InternalCreateMethod)},
    {FCFuncElement("InternalFindMethodInfo",NULL, (LPVOID) COMDelegate::InternalFindMethodInfo)},

    // The FCall mechanism knows how to wire multiple different constructor calls into a
    // single entrypoint, without the following entry.  But we need this entry to satisfy
    // frame creation within the body:
    {FCFuncElement("NeverCallThis",         NULL, (LPVOID) COMDelegate::DelegateConstruct)},
    {NULL, NULL, NULL}
};


static
ECFunc gFloatFuncs[] =
{
    {FCFuncElement("IsPositiveInfinity", NULL, (LPVOID)COMFloat::IsPositiveInfinity)},
    {FCFuncElement("IsNegativeInfinity", NULL, (LPVOID)COMFloat::IsNegativeInfinity)},
    {FCFuncElement("IsInfinity", NULL, (LPVOID)COMFloat::IsInfinity)},
    {NULL, NULL, NULL}
};

static
ECFunc gDoubleFuncs[] =
{
    {FCFuncElement("IsPositiveInfinity", NULL, (LPVOID)COMDouble::IsPositiveInfinity)},
    {FCFuncElement("IsNegativeInfinity", NULL, (LPVOID)COMDouble::IsNegativeInfinity)},
    {FCFuncElement("IsInfinity", NULL, (LPVOID)COMDouble::IsInfinity)},
    {NULL, NULL, NULL}
};

static
ECFunc gMathFuncs[] =
{
    {FCIntrinsic("Sin", NULL, (LPVOID)COMDouble::Sin, CORINFO_INTRINSIC_Sin)},
    {FCIntrinsic("Cos", NULL, (LPVOID)COMDouble::Cos, CORINFO_INTRINSIC_Cos)},
    {FCIntrinsic("Sqrt", NULL, (LPVOID)COMDouble::Sqrt, CORINFO_INTRINSIC_Sqrt)},
    {FCIntrinsic("Round", NULL, (LPVOID)COMDouble::Round, CORINFO_INTRINSIC_Round)},
    {FCIntrinsic("Abs", &gsig_SM_Flt_RetFlt, (LPVOID)COMDouble::AbsFlt, CORINFO_INTRINSIC_Abs)},
    {FCIntrinsic("Abs", &gsig_SM_Dbl_RetDbl, (LPVOID)COMDouble::AbsDbl, CORINFO_INTRINSIC_Abs)},
    {FCFuncElement("Exp", NULL, (LPVOID)COMDouble::Exp)},
    {FCFuncElement("Pow", NULL, (LPVOID)COMDouble::Pow)},
    {FCFuncElement("PowHelper", NULL, (LPVOID)COMDouble::PowHelper)},
    {FCFuncElement("Tan", NULL, (LPVOID)COMDouble::Tan)},
    {FCFuncElement("Floor", NULL, (LPVOID)COMDouble::Floor)},
    {FCFuncElement("Log", NULL, (LPVOID)COMDouble::Log)},
    {FCFuncElement("Sinh", NULL, (LPVOID)COMDouble::Sinh)},
    {FCFuncElement("Cosh", NULL, (LPVOID)COMDouble::Cosh)},
    {FCFuncElement("Tanh", NULL, (LPVOID)COMDouble::Tanh)},
    {FCFuncElement("Acos", NULL, (LPVOID)COMDouble::Acos)},
    {FCFuncElement("Asin", NULL, (LPVOID)COMDouble::Asin)},
    {FCFuncElement("Atan", NULL, (LPVOID)COMDouble::Atan)},
    {FCFuncElement("Atan2", NULL, (LPVOID)COMDouble::Atan2)},
    {FCFuncElement("Log10", NULL, (LPVOID)COMDouble::Log10)},
    {FCFuncElement("Ceiling", NULL, (LPVOID)COMDouble::Ceil)},
    {FCFuncElement("IEEERemainder", NULL, (LPVOID)COMDouble::IEEERemainder)},
    {FCFuncElement("InternalRound", NULL, (LPVOID)COMDouble::RoundDigits)},
    {NULL, NULL, NULL}
};

static
ECFunc gThreadFuncs[] =
{
    {FCFuncElement("StartInternal",             NULL, (LPVOID)ThreadNative::Start)}, /**/
    {FCFuncElement("SuspendInternal",           NULL, (LPVOID)ThreadNative::Suspend)},
    {FCFuncElement("ResumeInternal",            NULL, (LPVOID)ThreadNative::Resume)},
    {FCFuncElement("GetPriorityNative",         NULL, (LPVOID)ThreadNative::GetPriority)},
    {FCFuncElement("SetPriorityNative",         NULL, (LPVOID)ThreadNative::SetPriority)},
    {FCFuncElement("InterruptInternal",         NULL, (LPVOID)ThreadNative::Interrupt)},
    {FCFuncElement("IsAliveNative",             NULL, (LPVOID)ThreadNative::IsAlive)},
    {FCFuncElement("Join",      &gsig_IM_RetVoid,     (LPVOID)ThreadNative::Join)},
    {FCFuncElement("Join",      &gsig_IM_Int_RetBool, (LPVOID)ThreadNative::JoinTimeout)},
    {FCFuncElement("Sleep",                     NULL, (LPVOID)ThreadNative::Sleep)},
    {FCFuncElement("SetStart",                  NULL, (LPVOID)ThreadNative::SetStart)}, /**/
    {FCFuncElement("SetBackgroundNative",       NULL, (LPVOID)ThreadNative::SetBackground)}, /**/
    {FCFuncElement("IsBackgroundNative",        NULL, (LPVOID)ThreadNative::IsBackground)},
    {FCFuncElement("GetThreadStateNative",      NULL, (LPVOID)ThreadNative::GetThreadState)},
    {FCFuncElement("GetContextInternal",        NULL, (LPVOID)ThreadNative::GetContextFromContextID)},
    {FCFuncElement("GetDomainInternal",         NULL, (LPVOID)ThreadNative::GetDomain)}, /**/
    {FCFuncElement("GetFastDomainInternal", NULL, (LPVOID)ThreadNative::FastGetDomain)}, /**/
    {FCFuncElement("EnterContextInternal", NULL, (LPVOID)ThreadNative::EnterContextFromContextID)},    
    {FCFuncElement("ReturnToContext", NULL, (LPVOID)ThreadNative::ReturnToContextFromContextID)},    
    {FCFuncElement("InformThreadNameChange", NULL, (LPVOID)ThreadNative::InformThreadNameChange)},    
    {FCFuncElement("AbortInternal", NULL, (LPVOID)ThreadNative::Abort)},    
    {FCFuncElement("ResetAbortNative", NULL, (LPVOID)ThreadNative::ResetAbort)},    
    {FCFuncElement("IsRunningInDomain", NULL, (LPVOID)ThreadNative::IsRunningInDomain)},
    {FCFuncElement("IsThreadpoolThreadNative", NULL, (LPVOID)ThreadNative::IsThreadpoolThread)},
    {FCFuncElement("SpinWait", NULL, (LPVOID)ThreadNative::SpinWait)},
    {FCFuncElement("GetFastCurrentThreadNative",NULL, (LPVOID)ThreadNative::FastGetCurrentThread)},
    {FCFuncElement("GetCurrentThreadNative",    NULL, (LPVOID)ThreadNative::GetCurrentThread)},
    {FCFuncElement("GetDomainLocalStore",       NULL, (LPVOID)ThreadNative::GetDomainLocalStore)}, /**/
    {FCFuncElement("InternalFinalize",          NULL, (LPVOID)ThreadNative::Finalize)},
    {FCFuncElement("SetDomainLocalStore",       NULL, (LPVOID)ThreadNative::SetDomainLocalStore)}, /**/
    {NULL, NULL, NULL}
};

static
ECFunc gThreadPoolFuncs[] =
{
    {FCFuncElement("RegisterWaitForSingleObjectNative", NULL, (LPVOID)ThreadPoolNative::CorRegisterWaitForSingleObject)},
    {FCFuncElement("QueueUserWorkItem", NULL, (LPVOID)ThreadPoolNative::CorQueueUserWorkItem)},
    {FCFuncElement("GetMaxThreadsNative", NULL, (LPVOID)ThreadPoolNative::CorGetMaxThreads)},
    {FCFuncElement("GetAvailableThreadsNative", NULL, (LPVOID)ThreadPoolNative::CorGetAvailableThreads)},
};


static
ECFunc gTimerFuncs[] =
{
    {FCFuncElement("ChangeTimerNative", NULL, (LPVOID)TimerNative::CorChangeTimer)},
    {FCFuncElement("DeleteTimerNative", NULL, (LPVOID)TimerNative::CorDeleteTimer)},
    {FCFuncElement("AddTimerNative", NULL, (LPVOID)TimerNative::CorCreateTimer)},
    {NULL, NULL, NULL}
};

static
ECFunc gRegisteredWaitHandleFuncs[] =
{
    {FCFuncElement("UnregisterWaitNative", NULL, (LPVOID)ThreadPoolNative::CorUnregisterWait)},
    {FCFuncElement("WaitHandleCleanupNative", NULL, (LPVOID)ThreadPoolNative::CorWaitHandleCleanupNative)},
    {NULL, NULL, NULL}
};

static
ECFunc gWaitHandleFuncs[] =
{
    {FCFuncElement("WaitOneNative", NULL, (LPVOID)WaitHandleNative::CorWaitOneNative)}, /**/
    {FCFuncElement("WaitMultiple",   NULL, (LPVOID)WaitHandleNative::CorWaitMultipleNative)},
    {NULL, NULL, NULL}
};

static
ECFunc gManualResetEventFuncs[] =
{
    {FCFuncElement("CreateManualResetEventNative",  NULL, (LPVOID)ManualResetEventNative::CorCreateManualResetEvent)}, /**/
    {FCFuncElement("SetManualResetEventNative",     NULL, (LPVOID)ManualResetEventNative::CorSetEvent)}, /**/
    {FCFuncElement("ResetManualResetEventNative",   NULL, (LPVOID)ManualResetEventNative::CorResetEvent)},
    {NULL, NULL, NULL}

};

static
ECFunc gAutoResetEventFuncs[] =
{
    {FCFuncElement("CreateAutoResetEventNative",    NULL, (LPVOID)AutoResetEventNative::CorCreateAutoResetEvent)},
    {FCFuncElement("SetAutoResetEventNative",       NULL, (LPVOID)AutoResetEventNative::CorSetEvent)},
    {FCFuncElement("ResetAutoResetEventNative",     NULL, (LPVOID)AutoResetEventNative::CorResetEvent)},
    {NULL, NULL, NULL}

};

static
ECFunc gMutexFuncs[] =
{
    {FCFuncElement("CreateMutexNative", NULL, (LPVOID)MutexNative::CorCreateMutex)},
    {FCFuncElement("ReleaseMutexNative", NULL, (LPVOID)MutexNative::CorReleaseMutex)},
};

static
ECFunc gNumberFuncs[] =
{
    {FCFuncElement("FormatDecimal", NULL, (LPVOID)COMNumber::FormatDecimal)},
    {FCFuncElement("FormatDouble",  NULL, (LPVOID)COMNumber::FormatDouble)},
    {FCFuncElement("FormatInt32",   NULL, (LPVOID)COMNumber::FormatInt32)},
    {FCFuncElement("FormatUInt32",  NULL, (LPVOID)COMNumber::FormatUInt32)},
    {FCFuncElement("FormatInt64",   NULL, (LPVOID)COMNumber::FormatInt64)},
    {FCFuncElement("FormatUInt64",  NULL, (LPVOID)COMNumber::FormatUInt64)},
    {FCFuncElement("FormatSingle",  NULL, (LPVOID)COMNumber::FormatSingle)},
    {FCFuncElement("ParseInt32",    NULL, (LPVOID)COMNumber::ParseInt32)},
    {FCFuncElement("ParseUInt32",   NULL, (LPVOID)COMNumber::ParseUInt32)},
    {FCFuncElement("ParseInt64",    NULL, (LPVOID)COMNumber::ParseInt64)},
    {FCFuncElement("ParseUInt64",   NULL, (LPVOID)COMNumber::ParseUInt64)},
    {FCFuncElement("ParseDecimal",  NULL, (LPVOID)COMNumber::ParseDecimal)},
    {FCFuncElement("ParseDouble",   NULL, (LPVOID)COMNumber::ParseDouble)},
    {FCFuncElement("TryParseDouble",NULL, (LPVOID)COMNumber::TryParseDouble)},
    {FCFuncElement("ParseSingle",   NULL, (LPVOID)COMNumber::ParseSingle)},
    {NULL, NULL, NULL}
};

static
ECFunc gVariantFuncs[] =
{
    {FCFuncElement("SetFieldsR4",           NULL, (LPVOID)COMVariant::SetFieldsR4)},
    {FCFuncElement("SetFieldsR8",           NULL, (LPVOID)COMVariant::SetFieldsR8)},
    {FCFuncElement("SetFieldsObject",       NULL, (LPVOID)COMVariant::SetFieldsObject)},
    {FCFuncElement("GetR4FromVar",          NULL, (LPVOID)COMVariant::GetR4FromVar)},
    {FCFuncElement("GetR8FromVar",          NULL, (LPVOID)COMVariant::GetR8FromVar)},
    {FCFuncElement("InitVariant",                   NULL, (LPVOID)COMVariant::InitVariant)},
    {FCFuncElement("InternalTypedByRefToVariant",   NULL, (LPVOID)COMVariant::TypedByRefToVariantEx)},
    {FCFuncElement("InternalVariantToTypedByRef",   NULL, (LPVOID)COMVariant::VariantToTypedRefAnyEx)},
    {FCFuncElement("BoxEnum",                       NULL, (LPVOID)COMVariant::BoxEnum)},
    {NULL, NULL, NULL}
};



static
ECFunc gBitConverterFuncs[] =
{
    {FCFuncElement("GetBytes",           &gsig_SM_Char_RetArrByte, (LPVOID)BitConverter::CharToBytes)}, /**/
    {FCFuncElement("GetBytes",           &gsig_SM_Shrt_RetArrByte, (LPVOID)BitConverter::I2ToBytes)}, /**/
    {FCFuncElement("GetBytes",           &gsig_SM_Int_RetArrByte,  (LPVOID)BitConverter::I4ToBytes)}, /**/
    {FCFuncElement("GetBytes",           &gsig_SM_Long_RetArrByte, (LPVOID)BitConverter::I8ToBytes)}, /**/
    {FCFuncElement("GetUInt16Bytes",     NULL, (LPVOID)BitConverter::U2ToBytes)},
    {FCFuncElement("GetUInt32Bytes",     NULL, (LPVOID)BitConverter::U4ToBytes)},
    {FCFuncElement("GetUInt64Bytes",     NULL, (LPVOID)BitConverter::U8ToBytes)},
    {FCFuncElement("ToChar",             NULL, (LPVOID)BitConverter::BytesToChar)},
    {FCFuncElement("ToInt16",            NULL, (LPVOID)BitConverter::BytesToI2)},
    {FCFuncElement("ToInt32",            NULL, (LPVOID)BitConverter::BytesToI4)},
    {FCFuncElement("ToInt64",            NULL, (LPVOID)BitConverter::BytesToI8)},
    {FCFuncElement("ToUInt16",           NULL, (LPVOID)BitConverter::BytesToU2)},
    {FCFuncElement("ToUInt32",           NULL, (LPVOID)BitConverter::BytesToU4)},
    {FCFuncElement("ToUInt64",           NULL, (LPVOID)BitConverter::BytesToU8)},
    {FCFuncElement("ToSingle",           NULL, (LPVOID)BitConverter::BytesToR4)},
    {FCFuncElement("ToDouble",           NULL, (LPVOID)BitConverter::BytesToR8)},
    {FCFuncElement("ToString",           NULL, (LPVOID)BitConverter::BytesToString)},
    {NULL, NULL, NULL}
};

static
ECFunc gDecimalFuncs[] =
{
    {FCFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_Flt_RetVoid,   (LPVOID)COMDecimal::InitSingle)},
    {FCFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_Dbl_RetVoid,   (LPVOID)COMDecimal::InitDouble)},
    {FCFuncElement("Add",         NULL,               (LPVOID)COMDecimal::Add)},
    {FCFuncElement("Compare",     NULL,               (LPVOID)COMDecimal::Compare)},
    {FCFuncElement("Divide",      NULL,               (LPVOID)COMDecimal::Divide)},
    {FCFuncElement("Floor",       NULL,               (LPVOID)COMDecimal::Floor)},
    {FCFuncElement("GetHashCode", NULL,               (LPVOID)COMDecimal::GetHashCode)},
    {FCFuncElement("Remainder",   NULL,               (LPVOID)COMDecimal::Remainder)},
    {FCFuncElement("Multiply",    NULL,               (LPVOID)COMDecimal::Multiply)},
    {FCFuncElement("Round",       NULL,               (LPVOID)COMDecimal::Round)},
    {FCFuncElement("Subtract",    NULL,               (LPVOID)COMDecimal::Subtract)},
    {FCFuncElement("ToCurrency",  NULL,               (LPVOID)COMDecimal::ToCurrency)},
    {FCFuncElement("ToDouble",    NULL,               (LPVOID)COMDecimal::ToDouble)},
    {FCFuncElement("ToSingle",    NULL,               (LPVOID)COMDecimal::ToSingle)},
    {FCFuncElement("ToString",    NULL,               (LPVOID)COMDecimal::ToString)},
    {FCFuncElement("Truncate",    NULL,               (LPVOID)COMDecimal::Truncate)},
    {NULL, NULL, NULL}
};

static
ECFunc gCurrencyFuncs[] =
{
    {FCFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_Flt_RetVoid,   (LPVOID)COMCurrency::InitSingle)},
    {FCFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_Dbl_RetVoid,   (LPVOID)COMCurrency::InitDouble)},
    {FCFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_Str_RetVoid,   (LPVOID)COMCurrency::InitString)},
    {FCFuncElement("Add",         NULL,               (LPVOID)COMCurrency::Add)},
    {FCFuncElement("Floor",       NULL,               (LPVOID)COMCurrency::Floor)},
    {FCFuncElement("Multiply",    NULL,               (LPVOID)COMCurrency::Multiply)},
    {FCFuncElement("Round",       NULL,               (LPVOID)COMCurrency::Round)},
    {FCFuncElement("Subtract",    NULL,               (LPVOID)COMCurrency::Subtract)},
    {FCFuncElement("ToDecimal",   NULL,               (LPVOID)COMCurrency::ToDecimal)},
    {FCFuncElement("ToDouble",    NULL,               (LPVOID)COMCurrency::ToDouble)},
    {FCFuncElement("ToSingle",    NULL,               (LPVOID)COMCurrency::ToSingle)},
    {FCFuncElement("ToString",    NULL,               (LPVOID)COMCurrency::ToString)},
    {FCFuncElement("Truncate",    NULL,               (LPVOID)COMCurrency::Truncate)},
    {NULL, NULL, NULL}
};

static
ECFunc gDateTimeFuncs[] =
{
    {FCFuncElement("GetSystemFileTime",  NULL,             (LPVOID)COMDateTime::FCGetSystemFileTime)},
    {NULL, NULL, NULL}
};

static
ECFunc gCharacterInfoFuncs[] =
{
    {FCFuncElement("nativeInitTable",               NULL, (LPVOID)COMNlsInfo::nativeInitUnicodeCatTable)}, /**/
    {FCFuncElement("nativeGetCategoryDataTable",        NULL, (LPVOID)COMNlsInfo::nativeGetUnicodeCatTable)},
    {FCFuncElement("nativeGetCategoryLevel2Offset",        NULL, (LPVOID)COMNlsInfo::nativeGetUnicodeCatLevel2Offset)},
    {FCFuncElement("nativeGetNumericDataTable",        NULL, (LPVOID)COMNlsInfo::nativeGetNumericTable)},
    {FCFuncElement("nativeGetNumericLevel2Offset",        NULL, (LPVOID)COMNlsInfo::nativeGetNumericLevel2Offset)},
    {FCFuncElement("nativeGetNumericFloatData",        NULL, (LPVOID)COMNlsInfo::nativeGetNumericFloatData)},
    {NULL, NULL, NULL}
};

static
ECFunc gCompareInfoFuncs[] =
{
    {FCFuncElement("Compare",                 NULL, (LPVOID)COMNlsInfo::Compare)},
    {FCFuncElement("CompareRegion",           NULL, (LPVOID)COMNlsInfo::CompareRegion)}, /**/
    {FCFuncElement("IndexOfChar",             NULL, (LPVOID)COMNlsInfo::IndexOfChar)},
    {FCFuncElement("IndexOfString",           NULL, (LPVOID)COMNlsInfo::IndexOfString)},
    {FCFuncElement("LastIndexOfChar",         NULL, (LPVOID)COMNlsInfo::LastIndexOfChar)},
    {FCFuncElement("LastIndexOfString",       NULL, (LPVOID)COMNlsInfo::LastIndexOfString)},
    {FCFuncElement("nativeIsPrefix",          NULL, (LPVOID)COMNlsInfo::nativeIsPrefix)},
    {FCFuncElement("nativeIsSuffix",          NULL, (LPVOID)COMNlsInfo::nativeIsSuffix)},
    {FCFuncElement("InitializeNativeCompareInfo",   NULL, (LPVOID)COMNlsInfo::InitializeNativeCompareInfo)},
    {NULL, NULL, NULL}
};

static
ECFunc gGlobalizationAssemblyFuncs[] =
{
    {FCFuncElement("nativeCreateGlobalizationAssembly",  NULL,                            (LPVOID)COMNlsInfo::nativeCreateGlobalizationAssembly)},
    {NULL, NULL, NULL}
};

static
ECFunc gEncodingTableFuncs[] =
{
    {FCFuncElement("GetNumEncodingItems",  NULL, (LPVOID)COMNlsInfo::nativeGetNumEncodingItems)},
    {FCFuncElement("GetEncodingData",  NULL, (LPVOID)COMNlsInfo::nativeGetEncodingTableDataPointer)},
    {FCFuncElement("GetCodePageData",  NULL, (LPVOID)COMNlsInfo::nativeGetCodePageTableDataPointer)},
    {NULL, NULL, NULL}
};


static
ECFunc gCalendarFuncs[] =
{
    {FCFuncElement("nativeGetTwoDigitYearMax",   NULL,             (LPVOID)COMNlsInfo::nativeGetTwoDigitYearMax)},
    {NULL, NULL, NULL},
};

static
ECFunc gCultureInfoFuncs[] =
{
    {FCFuncElement("IsSupportedLCID",                   NULL,      (LPVOID)COMNlsInfo::IsSupportedLCID)},
    {FCFuncElement("IsInstalledLCID",                   NULL,      (LPVOID)COMNlsInfo::IsInstalledLCID)},
    {FCFuncElement("nativeGetUserDefaultLCID",          NULL,      (LPVOID)COMNlsInfo::nativeGetUserDefaultLCID)},
    {FCFuncElement("nativeGetUserDefaultUILanguage",    NULL,      (LPVOID)COMNlsInfo::nativeGetUserDefaultUILanguage)}, /**/
    {FCFuncElement("nativeGetSystemDefaultUILanguage",  NULL,      (LPVOID)COMNlsInfo::nativeGetSystemDefaultUILanguage)},
    {FCFuncElement("nativeGetThreadLocale",             NULL,      (LPVOID)COMNlsInfo::nativeGetThreadLocale)},
    {FCFuncElement("nativeSetThreadLocale",             NULL,      (LPVOID)COMNlsInfo::nativeSetThreadLocale)},
    {NULL, NULL, NULL}
};

static
ECFunc gCultureTableFuncs[] =
{
    {FCFuncElement("nativeInitCultureInfoTable",        NULL,      (LPVOID)COMNlsInfo::nativeInitCultureInfoTable)},
    {FCFuncElement("nativeGetHeader",                   NULL,      (LPVOID)COMNlsInfo::nativeGetCultureInfoHeader)},
    {FCFuncElement("nativeGetNameOffsetTable",          NULL,      (LPVOID)COMNlsInfo::nativeGetCultureInfoNameOffsetTable)},
    {FCFuncElement("nativeGetStringPoolStr",            NULL,      (LPVOID)COMNlsInfo::nativeGetCultureInfoStringPoolStr)},
    {FCFuncElement("nativeGetDataItemFromCultureID",    NULL,      (LPVOID)COMNlsInfo::nativeGetCultureDataFromID)},
    {FCFuncElement("GetInt32Value",                     NULL,      (LPVOID)COMNlsInfo::GetCultureInt32Value)},
    {FCFuncElement("GetStringValue",                    NULL,      (LPVOID)COMNlsInfo::GetCultureStringValue)},
    {FCFuncElement("GetDefaultInt32Value",              NULL,      (LPVOID)COMNlsInfo::GetCultureDefaultInt32Value)},
    {FCFuncElement("GetDefaultStringValue",             NULL,      (LPVOID)COMNlsInfo::GetCultureDefaultStringValue)},
    {FCFuncElement("GetMultipleStringValues",           NULL,      (LPVOID)COMNlsInfo::GetCultureMultiStringValues)},
    {NULL, NULL, NULL}
};


static
ECFunc gRegionTableFuncs[] =
{
    {FCFuncElement("nativeInitRegionInfoTable",         NULL,      (LPVOID)COMNlsInfo::nativeInitRegionInfoTable)},
    {FCFuncElement("nativeGetHeader",                   NULL,      (LPVOID)COMNlsInfo::nativeGetRegionInfoHeader)},
    {FCFuncElement("nativeGetNameOffsetTable",          NULL,      (LPVOID)COMNlsInfo::nativeGetRegionInfoNameOffsetTable)},
    {FCFuncElement("nativeGetStringPoolStr",            NULL,      (LPVOID)COMNlsInfo::nativeGetRegionInfoStringPoolStr)},
    {FCFuncElement("nativeGetDataItemFromRegionID",     NULL,      (LPVOID)COMNlsInfo::nativeGetRegionDataFromID)},
    {FCFuncElement("GetInt32Value",                     NULL,      (LPVOID)COMNlsInfo::nativeGetRegionInt32Value)},
    {FCFuncElement("GetStringValue",                    NULL,      (LPVOID)COMNlsInfo::nativeGetRegionStringValue)},
    {NULL, NULL, NULL}
};

static
ECFunc gCalendarTableFuncs[] =
{
    {FCFuncElement("nativeInitCalendarTable",           NULL,      (LPVOID)COMNlsInfo::nativeInitCalendarTable)},
    {FCFuncElement("GetInt32Value",                     NULL,      (LPVOID)COMNlsInfo::nativeGetCalendarInt32Value)},
    {FCFuncElement("GetStringValue",                    NULL,      (LPVOID)COMNlsInfo::nativeGetCalendarStringValue)},
    {FCFuncElement("GetMultipleStringValues",           NULL,      (LPVOID)COMNlsInfo::nativeGetCalendarMultiStringValues)},
    {FCFuncElement("nativeGetEraName",                  NULL,      (LPVOID)COMNlsInfo::nativeGetEraName)},
    //{FCFuncElement("nativeGetHeader",                   NULL,      (LPVOID)COMNlsInfo::nativeGetCalendarHeader)},
    //{FCFuncElement("nativeGetStringPoolStr",            NULL,      (LPVOID)COMNlsInfo::nativeGetCalendarStringPoolStr)},
    {NULL, NULL, NULL}
};

static
ECFunc gTextInfoFuncs[] =
{
    {FCFuncElement("nativeChangeCaseChar",        NULL,             (LPVOID)COMNlsInfo::nativeChangeCaseChar)},
    {FCFuncElement("nativeChangeCaseString",      NULL,             (LPVOID)COMNlsInfo::nativeChangeCaseString)},
    {FCFuncElement("AllocateDefaultCasingTable",  NULL,             (LPVOID)COMNlsInfo::AllocateDefaultCasingTable)},
    {FCFuncElement("InternalAllocateCasingTable", NULL,             (LPVOID)COMNlsInfo::AllocateCasingTable)},
    {FCFuncElement("nativeGetCaseInsHash",        NULL,             (LPVOID)COMNlsInfo::GetCaseInsHash)},
    {FCFuncElement("nativeGetTitleCaseChar",      NULL,             (LPVOID)COMNlsInfo::nativeGetTitleCaseChar)},
    {NULL, NULL, NULL}
};

static
ECFunc gSortKeyFuncs[] =
{
    {FCFuncElement("Compare",              NULL,  (LPVOID)COMNlsInfo::SortKey_Compare)},
    {FCFuncElement("nativeCreateSortKey",  NULL,  (LPVOID)COMNlsInfo::nativeCreateSortKey)},
    {NULL, NULL, NULL}
};

static
ECFunc gArrayFuncs[] =
{
    {FCFuncElement("Copy",                 NULL,   (LPVOID)SystemNative::ArrayCopy)}, /**/
    {FCFuncElement("Clear",                NULL,   (LPVOID)SystemNative::ArrayClear)},
    {FCFuncElement("GetRankNative",        NULL,   (LPVOID)Array_Rank)},
    {FCFuncElement("GetLowerBound",        NULL,   (LPVOID)Array_LowerBound)},
    {FCFuncElement("GetUpperBound",        NULL,   (LPVOID)Array_UpperBound)},
    {FCIntrinsic  ("GetLength",            &gsig_IM_Int_RetInt, (LPVOID)Array_GetLength,       CORINFO_INTRINSIC_Array_GetDimLength)},
    {FCIntrinsic  ("GetLengthNative",      &gsig_IM_RetInt,     (LPVOID)Array_GetLengthNoRank, CORINFO_INTRINSIC_Array_GetLengthTotal)},
    {FCFuncElement("Initialize",           NULL,   (LPVOID)Array_Initialize)},
    {FCFuncElement("InternalCreate",       NULL,   (LPVOID)COMArrayInfo::CreateInstance)}, /**/
    {FCFuncElement("TrySZIndexOf",         NULL,   (LPVOID)ArrayHelper::TrySZIndexOf)},
    {FCFuncElement("TrySZLastIndexOf",     NULL,   (LPVOID)ArrayHelper::TrySZLastIndexOf)},
    {FCFuncElement("TrySZBinarySearch",    NULL,   (LPVOID)ArrayHelper::TrySZBinarySearch)},
    {FCFuncElement("TrySZSort",            NULL,   (LPVOID)ArrayHelper::TrySZSort)},
    {FCFuncElement("TrySZReverse",         NULL,   (LPVOID)ArrayHelper::TrySZReverse)},
    {FCFuncElement("InternalGetValue",     NULL,   (LPVOID)COMArrayInfo::GetValue)}, /**/
    {FCFuncElement("InternalGetValueEx",   NULL,   (LPVOID)COMArrayInfo::GetValueEx)},
    {FCFuncElement("InternalSetValue",     NULL,   (LPVOID)COMArrayInfo::SetValue)},
    {FCFuncElement("InternalSetValueEx",   NULL,   (LPVOID)COMArrayInfo::SetValueEx)}, /**/
    {FCFuncElement("InternalCreateEx",     NULL,   (LPVOID)COMArrayInfo::CreateInstanceEx)},
    {NULL, NULL, NULL}
};

static
ECFunc gBufferFuncs[] =
{
    {FCFuncElement("BlockCopy",        NULL,   (LPVOID)Buffer::BlockCopy)}, /**/
    {FCFuncElement("InternalBlockCopy",NULL,   (LPVOID)Buffer::InternalBlockCopy)}, /**/
    {FCFuncElement("GetByte",          NULL,   (LPVOID)Buffer::GetByte)}, /**/
    {FCFuncElement("SetByte",          NULL,   (LPVOID)Buffer::SetByte)}, /**/
    {FCFuncElement("ByteLength",       NULL,   (LPVOID)Buffer::ByteLength)}, /**/
    {NULL, NULL, NULL}
};

static
ECFunc gGCInterfaceFuncs[] =
{
    {FCFuncElement("GetGenerationWR",               NULL,   (LPVOID)GCInterface::GetGenerationWR)},
    {FCFuncElement("GetGeneration",                 NULL,   (LPVOID)GCInterface::GetGeneration)},
    {FCFuncElement("nativeGetTotalMemory",          NULL,   (LPVOID)GCInterface::GetTotalMemory)}, /**/
    {FCFuncElement("nativeCollectGeneration",       NULL,   (LPVOID)GCInterface::CollectGeneration)}, /**/
    {FCFuncElement("nativeGetMaxGeneration",        NULL,   (LPVOID)GCInterface::GetMaxGeneration)},
    {FCFuncElement("WaitForPendingFinalizers",      NULL,   (LPVOID)GCInterface::RunFinalizers)}, /**/

    {FCFuncElement("KeepAlive",                     NULL,   (LPVOID)GCInterface::KeepAlive)},
    {FCFuncElement("nativeGetCurrentMethod",        NULL,   (LPVOID)GCInterface::InternalGetCurrentMethod)},
    {FCFuncElement("SetCleanupCache",               NULL,   (LPVOID)GCInterface::NativeSetCleanupCache)},
    {FCFuncElement("nativeSuppressFinalize",        NULL,   (LPVOID)GCInterface::FCSuppressFinalize)},
    {FCFuncElement("nativeReRegisterForFinalize",   NULL,   (LPVOID)GCInterface::FCReRegisterForFinalize)},
    {NULL, NULL, NULL}
};


static
ECFunc gInteropMarshalFuncs[] =
{
    {FCFuncElement("GetLastWin32Error",         NULL,                      (LPVOID)GetLastWin32Error )},
    {FCFuncElement("SizeOf",                    &gsig_SM_Type_RetInt,      (LPVOID)SizeOfClass )}, /**/
    {FCFuncElement("GetSystemMaxDBCSCharSize",  NULL,                      (LPVOID)GetSystemMaxDBCSCharSize) }, /**/
    {FCFuncElement("PtrToStructureHelper",      NULL,                      (LPVOID)PtrToStructureHelper)}, /**/
    {FCFuncElement("SizeOf",                    &gsig_SM_Obj_RetInt,       (LPVOID)FCSizeOfObject )},
    {FCFuncElement("OffsetOfHelper",            NULL,                      (LPVOID)OffsetOfHelper )},
    {FCFuncElement("UnsafeAddrOfPinnedArrayElement", NULL,                 (LPVOID)FCUnsafeAddrOfPinnedArrayElement )},
    {FCFuncElement("Prelink",                   NULL,                      (LPVOID)NDirect_Prelink_Wrapper )},
    {FCFuncElement("NumParamBytes",             NULL,                      (LPVOID)NDirect_NumParamBytes )},
    {FCFuncElement("CopyBytesToNative",         NULL,                      (LPVOID)CopyToNative )},
    {FCFuncElement("Copy",                      &gsig_SM_ArrChar_Int_IntPtr_Int_RetVoid,    (LPVOID)CopyToNative )},
    {FCFuncElement("Copy",                      &gsig_SM_ArrShrt_Int_IntPtr_Int_RetVoid,   (LPVOID)CopyToNative )},
    {FCFuncElement("Copy",                      &gsig_SM_ArrInt_Int_IntPtr_Int_RetVoid,     (LPVOID)CopyToNative )},
    {FCFuncElement("Copy",                      &gsig_SM_ArrLong_Int_IntPtr_Int_RetVoid,    (LPVOID)CopyToNative )},
    {FCFuncElement("Copy",                      &gsig_SM_ArrFlt_Int_IntPtr_Int_RetVoid,   (LPVOID)CopyToNative )},
    {FCFuncElement("Copy",                      &gsig_SM_ArrDbl_Int_IntPtr_Int_RetVoid,  (LPVOID)CopyToNative )},
    {FCFuncElement("CopyBytesToManaged",        NULL,                      (LPVOID)CopyToManaged )},
    {FCFuncElement("Copy",                      &gsig_SM_IntPtr_ArrChar_Int_Int_RetVoid,   (LPVOID)CopyToManaged )},
    {FCFuncElement("Copy",                      &gsig_SM_IntPtr_ArrShrt_Int_Int_RetVoid,  (LPVOID)CopyToManaged )},
    {FCFuncElement("Copy",                      &gsig_SM_IntPtr_ArrInt_Int_Int_RetVoid,    (LPVOID)CopyToManaged )},
    {FCFuncElement("Copy",                      &gsig_SM_IntPtr_ArrLong_Int_Int_RetVoid,   (LPVOID)CopyToManaged )},
    {FCFuncElement("Copy",                      &gsig_SM_IntPtr_ArrFlt_Int_Int_RetVoid,  (LPVOID)CopyToManaged )},
    {FCFuncElement("Copy",                      &gsig_SM_IntPtr_ArrDbl_Int_Int_RetVoid, (LPVOID)CopyToManaged )},
    {FCFuncElement("PtrToStringAnsi",           NULL,                      (LPVOID)PtrToStringAnsi)},
    {FCFuncElement("PtrToStringUni",            NULL,                      (LPVOID)PtrToStringUni)},
    {FCFuncElement("StructureToPtr",            NULL,                      (LPVOID)StructureToPtr)}, /**/
    {FCFuncElement("DestroyStructure",          NULL,                      (LPVOID)DestroyStructure)},
    {FCFuncElement("GetExceptionPointers",      NULL,                      (LPVOID)ExceptionNative::GetExceptionPointers )},
    {FCFuncElement("GetExceptionCode",          NULL,                      (LPVOID)ExceptionNative::GetExceptionCode )},
    {FCFuncElement("ThrowExceptionForHR",       NULL,                      (LPVOID)Interop::ThrowExceptionForHR)},
    {FCFuncElement("GetHRForException",         NULL,                      (LPVOID)Interop::GetHRForException)},
    {NULL, NULL, NULL}
};


static
ECFunc gArrayWithOffsetFuncs[] =
{
    {FCFuncElement("CalculateCount",          NULL,                      (LPVOID)CalculateCount )},
    {NULL, NULL, NULL}
};


static
ECFunc gPolicyManagerFuncs[] =
{
    {FCFuncElement("_InitData", NULL, (LPVOID)COMSecurityConfig::EcallInitData)},
    {FCFuncElement("_InitDataEx", NULL, (LPVOID)COMSecurityConfig::EcallInitDataEx)},
    {FCFuncElement("SaveCacheData", NULL, (LPVOID)COMSecurityConfig::EcallSaveCacheData)},
    {FCFuncElement("ResetCacheData", NULL, (LPVOID)COMSecurityConfig::EcallResetCacheData)},
    {FCFuncElement("ClearCacheData", NULL, (LPVOID)COMSecurityConfig::EcallClearCacheData)},
    {FCFuncElement("_SaveDataString", NULL, (LPVOID)COMSecurityConfig::EcallSaveDataString)},
    {FCFuncElement("_SaveDataByte", NULL, (LPVOID)COMSecurityConfig::EcallSaveDataByte)},
    {FCFuncElement("RecoverData", NULL, (LPVOID)COMSecurityConfig::EcallRecoverData)},
    {FCFuncElement("GetData", NULL, (LPVOID)COMSecurityConfig::GetRawData)},
    {FCFuncElement("IsCompilationDomain", NULL, (LPVOID)COMSecurityConfig::EcallIsCompilationDomain)},
    {FCFuncElement("GetQuickCacheEntry", NULL, (LPVOID)COMSecurityConfig::EcallGetQuickCacheEntry)},
    {FCFuncElement("SetQuickCache", NULL, (LPVOID)COMSecurityConfig::EcallSetQuickCache)},
    {FCFuncElement("GetCacheEntry", NULL, (LPVOID)COMSecurityConfig::GetCacheEntry)},
    {FCFuncElement("AddCacheEntry", NULL, (LPVOID)COMSecurityConfig::AddCacheEntry)},
    {FCFuncElement("TurnCacheOff", NULL, (LPVOID)COMSecurityConfig::EcallTurnCacheOff)},
    {FCFuncElement("_GetMachineDirectory", NULL, (LPVOID)COMSecurityConfig::EcallGetMachineDirectory)},
    {FCFuncElement("_GetUserDirectory", NULL, (LPVOID)COMSecurityConfig::EcallGetUserDirectory)},
    {FCFuncElement("WriteToEventLog", NULL, (LPVOID)COMSecurityConfig::EcallWriteToEventLog)},
    {FCFuncElement("_GetStoreLocation", NULL, (LPVOID)COMSecurityConfig::EcallGetStoreLocation)},
    {FCFuncElement("GetCacheSecurityOn", NULL, (LPVOID)COMSecurityConfig::GetCacheSecurityOn)},
    {FCFuncElement("SetCacheSecurityOn", NULL, (LPVOID)COMSecurityConfig::SetCacheSecurityOn)},
    {FCFuncElement("_DebugOut", NULL, (LPVOID)COMSecurityConfig::DebugOut)},
    {NULL, NULL, NULL}
};


static
ECFunc gIsolatedStorage[] =
{
    {FCFuncElement("nGetCaller", NULL, (LPVOID)COMIsolatedStorage::GetCaller)},
    {NULL, NULL, NULL}
};

static
ECFunc gIsolatedStorageFile[] =
{
    {FCFuncElement("nGetRootDir", NULL, (LPVOID)COMIsolatedStorageFile::GetRootDir)},
    {FCFuncElement("nReserve", NULL, (LPVOID)COMIsolatedStorageFile::Reserve)},
    {FCFuncElement("nGetUsage", NULL, (LPVOID)COMIsolatedStorageFile::GetUsage)},
    {FCFuncElement("nOpen", NULL, (LPVOID)COMIsolatedStorageFile::Open)},
    {FCFuncElement("nClose", NULL, (LPVOID)COMIsolatedStorageFile::Close)},
    {NULL, NULL, NULL}
};

static
ECFunc  gTypeLoadExceptionFuncs[] =
{
    {FCFuncElement("FormatTypeLoadExceptionMessage", NULL, (LPVOID)FormatTypeLoadExceptionMessage)},
    {NULL, NULL, NULL}
};

static
ECFunc  gFileLoadExceptionFuncs[] =
{
    {FCFuncElement("FormatFileLoadExceptionMessage", NULL, (LPVOID)FormatFileLoadExceptionMessage)},
    {NULL, NULL, NULL}
};

static
ECFunc  gSignatureHelperFuncs[] =
{
    {FCFuncElement("GetCorElementTypeFromClass", NULL, (LPVOID)COMModule::GetSigTypeFromClassWrapper)}, /**/
    {NULL, NULL, NULL}
};


static
ECFunc  gMissingMethodExceptionFuncs[] =
{
    {FCFuncElement("FormatSignature", NULL, (LPVOID)MissingMethodException_FormatSignature)},
    {NULL, NULL, NULL}
};

static
ECFunc gInterlockedFuncs[] =
{
    {FCFuncElement("Increment",              &gsig_SM_RefInt_RetInt,                      (LPVOID)COMInterlocked::Increment32 )},
    {FCFuncElement("Decrement",              &gsig_SM_RefInt_RetInt,                      (LPVOID)COMInterlocked::Decrement32)},
    {FCFuncElement("Increment",              &gsig_SM_RefLong_RetLong,                    (LPVOID)COMInterlocked::Increment64 )},
    {FCFuncElement("Decrement",              &gsig_SM_RefLong_RetLong,                    (LPVOID)COMInterlocked::Decrement64)},
    {FCFuncElement("Exchange",               &gsig_SM_RefInt_Int_RetInt,           (LPVOID)COMInterlocked::Exchange)},
    {FCFuncElement("CompareExchange",        &gsig_SM_RefInt_Int_Int_RetInt,    (LPVOID)COMInterlocked::CompareExchange)},
    {FCFuncElement("Exchange",               &gsig_SM_RefFlt_Flt_RetFlt,         (LPVOID)COMInterlocked::ExchangeFloat)},
    {FCFuncElement("CompareExchange",        &gsig_SM_RefFlt_Flt_Flt_RetFlt,  (LPVOID)COMInterlocked::CompareExchangeFloat)},
    {FCFuncElement("Exchange",               &gsig_SM_RefObj_Obj_RetObj,        (LPVOID)COMInterlocked::ExchangeObject)},
    {FCFuncElement("CompareExchange",        &gsig_SM_RefObj_Obj_Obj_RetObj, (LPVOID)COMInterlocked::CompareExchangeObject)},
    {FCFuncElement("CompareExchangePointer", &gsig_SM_RefIntPtr_IntPtr_IntPtr_RetIntPtr,(LPVOID)COMInterlocked::CompareExchangePointer)},
    {NULL, NULL, NULL}
};

static
ECFunc gVarArgFuncs[] =
{
    {FCFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_RuntimeArgumentHandle_RetVoid,           (LPVOID)COMVarArgs::Init)},
    {FCFuncElement(COR_CTOR_METHOD_NAME,      &gsig_IM_RuntimeArgumentHandle_PtrVoid_RetVoid,   (LPVOID)COMVarArgs::Init2)},
    {FCFuncElement("GetRemainingCount",       NULL,                                             (LPVOID)COMVarArgs::GetRemainingCount)},
    {FCFuncElement("GetNextArgType",          NULL,                                             (LPVOID)COMVarArgs::GetNextArgType)},
    {FCFuncElement("GetNextArg",              NULL,                                             (LPVOID)COMVarArgs::GetNextArg)},
    {FCFuncElement("InternalGetNextArg",      NULL,                                             (LPVOID)COMVarArgs::GetNextArg2)},
    {NULL, NULL, NULL}
};


static
ECFunc gMonitorFuncs[] =
{
    {FCFuncElement("Enter",                  NULL,                       (LPVOID)JIT_MonEnter )},
    {FCFuncElement("Exit",                   NULL,                       (LPVOID)JIT_MonExit )},
    {FCFuncElement("TryEnterTimeout",        NULL,                       (LPVOID)JIT_MonTryEnter )},
    {FCFuncElement("ObjWait",   NULL, (LPVOID)ObjectNative::WaitTimeout)},
    {FCFuncElement("ObjPulse",    NULL,(LPVOID)ObjectNative::Pulse)},
    {FCFuncElement("ObjPulseAll",   NULL, (LPVOID)ObjectNative::PulseAll)},
    {NULL, NULL, NULL}

};

static
ECFunc gOverlappedFuncs[] =
{
    {FCFuncElement("AllocNativeOverlapped",   NULL,                       (LPVOID)AllocNativeOverlapped )},
    {FCFuncElement("FreeNativeOverlapped",   NULL,                       (LPVOID)FreeNativeOverlapped )},
    {NULL, NULL, NULL}
};

static
ECFunc gCompilerFuncs[] =
{
    {FCFuncElement("GetObjectValue", NULL, (LPVOID)ObjectNative::GetObjectValue)},
    {FCFuncElement("InitializeArray",   NULL, (LPVOID)COMArrayInfo::InitializeArray )},
    {FCFuncElement("RunClassConstructor",   NULL, (LPVOID)COMClass::RunClassConstructor )},
    {NULL, NULL, NULL}
};

static
ECFunc gStrongNameKeyPairFuncs[] =
{
    {FCFuncElement("nGetPublicKey",             NULL,                       (LPVOID)Security::GetPublicKey )},
    {NULL, NULL, NULL}
};



static
ECFunc gGCHandleFuncs[] =
{
    {FCFuncElement("InternalAlloc", NULL,     (LPVOID)GCHandleInternalAlloc)},
    {FCFuncElement("InternalFree", NULL,     (LPVOID)GCHandleInternalFree)},
    {FCFuncElement("InternalGet", NULL,     (LPVOID)GCHandleInternalGet)},
    {FCFuncElement("InternalSet", NULL,     (LPVOID)GCHandleInternalSet)},
    {FCFuncElement("InternalCompareExchange", NULL,     (LPVOID)GCHandleInternalCompareExchange)},
    {FCFuncElement("InternalAddrOfPinnedObject", NULL, (LPVOID)GCHandleInternalAddrOfPinnedObject)},
    {FCFuncElement("InternalCheckDomain", NULL,     (LPVOID)GCHandleInternalCheckDomain)},
    {NULL, NULL, NULL}
};

static
ECFunc gConfigHelper[] =
{
    {FCFuncElement("RunParser",            NULL, (LPVOID)ConfigNative::RunParser)},
    {NULL, NULL, NULL}
};




#define ECClassesElement(A,B,C) A, B, C

// Note these have to remain sorted by name:namespace pair (Assert will wack you if you dont)
static
ECClass gECClasses[] =
{
    {ECClassesElement("AppDomain", "System", gAppDomainFuncs)},
    {ECClassesElement("AppDomainSetup", "System", gAppDomainSetupFuncs)},
    {ECClassesElement("ArgIterator", "System", gVarArgFuncs)},
    {ECClassesElement("Array", "System", gArrayFuncs)},
    {ECClassesElement("ArrayWithOffset", "System.Runtime.InteropServices", gArrayWithOffsetFuncs)},
    {ECClassesElement("Assembly", "System.Reflection", gAssemblyFuncs)},
    {ECClassesElement("AssemblyName", "System.Reflection", gAssemblyNameFuncs)},
    {ECClassesElement("Assert", "System.Diagnostics", gDiagnosticsAssert)},
    {ECClassesElement("AsyncFileStreamProto", "System.IO", gFileStreamFuncs)},
    {ECClassesElement("AutoResetEvent", "System.Threading", gAutoResetEventFuncs)},
    {ECClassesElement("BCLDebug", "System", gBCLDebugFuncs)},
    {ECClassesElement("BitConverter", "System", gBitConverterFuncs)},
    {ECClassesElement("Buffer", "System", gBufferFuncs)},
    {ECClassesElement("Calendar", "System.Globalization", gCalendarFuncs)},
    {ECClassesElement("CalendarTable", "System.Globalization", gCalendarTableFuncs)},
    {ECClassesElement("ChannelServices", "System.Runtime.Remoting.Channels", gChannelServicesFuncs)},
    {ECClassesElement("Char", "System", gCharacterFuncs)},
    {ECClassesElement("CharacterInfo", "System.Globalization", gCharacterInfoFuncs)},
    {ECClassesElement("CloningFormatter", "System.Runtime.Serialization", gCloningFuncs)},
    {ECClassesElement("CodeAccessSecurityEngine", "System.Security", gCOMCodeAccessSecurityEngineFuncs)},
    {ECClassesElement("CodePageEncoding", "System.Text", gCodePageEncodingFuncs)},
    {ECClassesElement("CompareInfo", "System.Globalization", gCompareInfoFuncs)},
    {ECClassesElement("Config", "System.Security.Util", gPolicyManagerFuncs)},
    {ECClassesElement("ConfigServer", "System", gConfigHelper)},
    {ECClassesElement("Console", "System", gConsoleFuncs)},
    {ECClassesElement("Context", "System.Runtime.Remoting.Contexts", gContextFuncs)},
    {ECClassesElement("Convert", "System", gConvertFuncs)},
    {ECClassesElement("CultureInfo", "System.Globalization", gCultureInfoFuncs)},
    {ECClassesElement("CultureTable", "System.Globalization", gCultureTableFuncs)},
    {ECClassesElement("Currency", "System", gCurrencyFuncs)},
    {ECClassesElement("CurrentSystemTimeZone", "System", gTimeZoneFuncs)},
    {ECClassesElement("CustomAttribute", "System.Reflection", gCOMCustomAttributeFuncs)},
    {ECClassesElement("DateTime", "System", gDateTimeFuncs)},
    {ECClassesElement("Debugger", "System.Diagnostics", gDiagnosticsDebugger)},
    {ECClassesElement("Decimal", "System", gDecimalFuncs)},
    {ECClassesElement("DefaultBinder", "System", gCOMDefaultBinderFuncs)},
    {ECClassesElement("Delegate", "System", gDelegateFuncs)},
    {ECClassesElement("Double", "System", gDoubleFuncs)},
    {ECClassesElement("EncodingTable", "System.Globalization", gEncodingTableFuncs)},
    {ECClassesElement("Enum", "System", gEnumFuncs)},
    {ECClassesElement("Environment", "System", gEnvironmentFuncs)},
    {ECClassesElement("Exception", "System", gExceptionFuncs)},
    {ECClassesElement("FileIOAccess", "System.Security.Permissions", gCOMFileIOAccessFuncs)},
    {ECClassesElement("FileLoadException", "System.IO", gFileLoadExceptionFuncs)},
    {ECClassesElement("FileStream", "System.IO", gFileStreamFuncs)},
    {ECClassesElement("FormatterServices", "System.Runtime.Serialization", gSerializationFuncs)},
    {ECClassesElement("FrameSecurityDescriptor", "System.Security", gCOMCodeAccessSecurityEngineFuncs)},
    {ECClassesElement("Fusion", "Microsoft.Win32", gFusionWrapFuncs)},
    {ECClassesElement("GC", "System", gGCInterfaceFuncs)},
    {ECClassesElement("GCHandle", "System.Runtime.InteropServices", gGCHandleFuncs)},
    {ECClassesElement("GlobalizationAssembly", "System.Globalization", gGlobalizationAssemblyFuncs)},
    {ECClassesElement("Guid", "System", gGuidFuncs)},
    {ECClassesElement("Interlocked", "System.Threading", gInterlockedFuncs)},
    {ECClassesElement("IsolatedStorage", "System.IO.IsolatedStorage", gIsolatedStorage)},
    {ECClassesElement("IsolatedStorageFile", "System.IO.IsolatedStorage", gIsolatedStorageFile)},
    {ECClassesElement("Log", "System.Diagnostics", gDiagnosticsLog)},
    {ECClassesElement("ManualResetEvent", "System.Threading", gManualResetEventFuncs)},
    {ECClassesElement("Marshal", "System.Runtime.InteropServices", gInteropMarshalFuncs)},
    {ECClassesElement("Math", "System", gMathFuncs)},

    {ECClassesElement("Message", "System.Runtime.Remoting.Messaging", gMessageFuncs)},
    {ECClassesElement("MethodRental", "System.Reflection.Emit", gCOMMethodRental)},
    {ECClassesElement("MissingFieldException", "System",  gMissingMethodExceptionFuncs)},
    {ECClassesElement("MissingMemberException", "System",  gMissingMethodExceptionFuncs)},
    {ECClassesElement("MissingMethodException", "System", gMissingMethodExceptionFuncs)},
    {ECClassesElement("Module", "System.Reflection", gCOMModuleFuncs)},
    {ECClassesElement("Monitor", "System.Threading", gMonitorFuncs)},
    {ECClassesElement("Mutex", "System.Threading", gMutexFuncs)},
    {ECClassesElement("Number", "System", gNumberFuncs)},
    {ECClassesElement("Object", "System", gObjectFuncs)},
    {ECClassesElement("Overlapped", "System.Threading", gOverlappedFuncs)},
    {ECClassesElement("ParseNumbers", "System", gParseNumbersFuncs)},
    {ECClassesElement("Path", "System.IO", gFileStreamFuncs)},
    {ECClassesElement("PolicyLevel", "System.Security.Policy", gPolicyManagerFuncs)},
    {ECClassesElement("PolicyManager", "System.Security", gPolicyManagerFuncs)},
    {ECClassesElement("ReaderWriterLock", "System.Threading", gRWLockFuncs)},
    {ECClassesElement("RealProxy", "System.Runtime.Remoting.Proxies", gRealProxyFuncs)},
    {ECClassesElement("RegionTable", "System.Globalization", gRegionTableFuncs)},
    {ECClassesElement("RegisteredWaitHandle", "System.Threading", gRegisteredWaitHandleFuncs)},
    {ECClassesElement("RemotingServices", "System.Runtime.Remoting", gRemotingFuncs)},
    {ECClassesElement("RuntimeConstructorInfo", "System.Reflection", gCOMConstructorFuncs)},
    {ECClassesElement("RuntimeEnvironment", "System.Runtime.InteropServices", gRuntimeEnvironmentFuncs)},
    {ECClassesElement("RuntimeEventInfo", "System.Reflection", gCOMEventFuncs)},
    {ECClassesElement("RuntimeFieldInfo", "System.Reflection", gCOMFieldFuncs)},
    {ECClassesElement("RuntimeHelpers", "System.Runtime.CompilerServices", gCompilerFuncs)},
    {ECClassesElement("RuntimeMethodHandle", "System", gCOMMethodHandleFuncs)},
    {ECClassesElement("RuntimeMethodInfo", "System.Reflection", gCOMMethodFuncs)},
    {ECClassesElement("RuntimePropertyInfo", "System.Reflection", gCOMPropFuncs)},
    {ECClassesElement("RuntimeType", "System", gCOMClassFuncs)},
    {ECClassesElement("SecurityManager", "System.Security", gCOMSecurityManagerFuncs)},
    {ECClassesElement("SecurityRuntime", "System.Security", gCOMSecurityRuntimeFuncs)},
    {ECClassesElement("SignatureHelper", "System.Reflection.Emit", gSignatureHelperFuncs)},
    {ECClassesElement("Single", "System", gFloatFuncs)},
    {ECClassesElement("SortKey", "System.Globalization", gSortKeyFuncs)},
    {ECClassesElement("StackBuilderSink", "System.Runtime.Remoting.Messaging", gStackBuilderSinkFuncs)},
    {ECClassesElement("StackTrace", "System.Diagnostics", gDiagnosticsStackTrace)},
    {ECClassesElement("String", "System", gStringFuncs)},
    {ECClassesElement("StringBuilder", "System.Text", gStringBufferFuncs)},
    {ECClassesElement("StringExpressionSet", "System.Security.Util", gCOMStringExpressionSetFuncs)},
    {ECClassesElement("StrongNameKeyPair", "System.Reflection", gStrongNameKeyPairFuncs)},
    {ECClassesElement("TextInfo", "System.Globalization", gTextInfoFuncs)},
    {ECClassesElement("Thread", "System.Threading", gThreadFuncs)},
    {ECClassesElement("ThreadPool", "System.Threading", gThreadPoolFuncs)},
    {ECClassesElement("Timer", "System.Threading", gTimerFuncs)},
    {ECClassesElement("Type", "System", gCOMClassFuncs)},
    {ECClassesElement("TypeBuilder", "System.Reflection.Emit", gCOMClassWriter)},
    {ECClassesElement("TypeLoadException", "System", gTypeLoadExceptionFuncs)},
    {ECClassesElement("TypedReference", "System", gTypedReferenceFuncs)},
    {ECClassesElement("URLString", "System.Security.Util", gCOMUrlStringFuncs)},
    {ECClassesElement("ValueType", "System", gValueTypeFuncs)},
    {ECClassesElement("Variant", "System", gVariantFuncs)},
    {ECClassesElement("WaitHandle", "System.Threading", gWaitHandleFuncs)},
    {ECClassesElement("Zone", "System.Security.Policy", gCOMSecurityZone)},
};

    // To provide a quick check, this is the lowest and highest
    // addresses of any FCALL starting address
static BYTE* gLowestFCall = (BYTE*)-1;
static BYTE* gHighestFCall = 0;

#define FCALL_HASH_SIZE 256
static ECFunc* gFCallMethods[FCALL_HASH_SIZE];
static SpinLock gFCallLock;

inline unsigned FCallHash(const void* pTarg) {
    return (unsigned)((((size_t) pTarg) / 4)  % FCALL_HASH_SIZE);
}

inline ECFunc** getCacheEntry(MethodDesc* pMD) {
#define BYMD_CACHE_SIZE 32
    _ASSERTE(((BYMD_CACHE_SIZE-1) & BYMD_CACHE_SIZE) == 0);     // Must be a power of 2
    static ECFunc* gByMDCache[BYMD_CACHE_SIZE] = { 0 };
    return &gByMDCache[((((size_t) pMD) >> 3) & BYMD_CACHE_SIZE-1)];
}

/*******************************************************************************/
USHORT FindImplsIndexForClass(MethodTable* pMT)
{
    LPCUTF8 pszNamespace = 0;
    LPCUTF8 pszName = pMT->GetClass()->GetFullyQualifiedNameInfo(&pszNamespace);

    // Array classes get null from the above routine, but they have no ecalls.
    if (pszName == NULL)
        return((USHORT) -1);

    unsigned low  = 0;
    unsigned high = sizeof(gECClasses)/sizeof(ECClass);

#ifdef _DEBUG
    static bool checkedSort = false;
    LPCUTF8 prevClass = "";
    LPCUTF8 prevNameSpace = "";
    if (!checkedSort) {
        checkedSort = true;
        for (unsigned i = 0; i < high; i++)  {
                // Make certain list is sorted!
            int cmp = strcmp(gECClasses[i].m_wszClassName, prevClass);
            if (cmp == 0)
                cmp = strcmp(gECClasses[i].m_wszNameSpace, prevNameSpace);
            _ASSERTE(cmp > 0);      // Hey, you forgot to sort the new class

            prevClass = gECClasses[i].m_wszClassName;
            prevNameSpace = gECClasses[i].m_wszNameSpace;
        }
    }
#endif
    while (high > low) {
        unsigned mid  = (high + low) / 2;
        int cmp = strcmp(pszName, gECClasses[mid].m_wszClassName);
        if (cmp == 0)
            cmp = strcmp(pszNamespace, gECClasses[mid].m_wszNameSpace);

        if (cmp == 0) {
            return(mid);
        }
        if (cmp > 0)
            low = mid+1;
        else
            high = mid;
        }

    return((USHORT) -1);
}

ECFunc* GetImplsForIndex(USHORT index)
{
    if (index == (USHORT) -1)
        return NULL;
    else
        return gECClasses[index].m_pECFunc;        
}

ECFunc* FindImplsForClass(MethodTable* pMT)
{
    return GetImplsForIndex(FindImplsIndexForClass(pMT));
}


/*******************************************************************************/
/*  Finds the implementation for the given method desc.  */

static ECFunc *GetECForIndex(USHORT index, ECFunc *impls)
{
    if (index == (USHORT) -1)
        return NULL;
    else
        return impls + index;
}

static USHORT FindECIndexForMethod(MethodDesc *pMD, ECFunc *impls)
{
    ECFunc* cur = impls;

    LPCUTF8 szMethodName = pMD->GetName();
    PCCOR_SIGNATURE pMethodSig;
    ULONG       cbMethodSigLen;
    pMD->GetSig(&pMethodSig, &cbMethodSigLen);
    Module* pModule = pMD->GetModule();

    for (;cur->m_wszMethodName != 0; cur++)  {
        if (strcmp(cur->m_wszMethodName, szMethodName) == 0) {
            if (cur->m_wszMethodSig != NULL) {
                PCCOR_SIGNATURE pBinarySig;
                ULONG       pBinarySigLen;
                if (FAILED(cur->m_wszMethodSig->GetBinaryForm(&pBinarySig, &pBinarySigLen)))
                    continue;
                if (!MetaSig::CompareMethodSigs(pMethodSig, cbMethodSigLen, pModule,
                                                pBinarySig, pBinarySigLen, cur->m_wszMethodSig->GetModule()))
                    continue;
            }
            // We have found a match!

            return (USHORT)(cur - impls);
        }
    }
    return (USHORT)-1;
}

static ECFunc* FindECFuncForMethod(MethodDesc* pMD)
{
    // check the cache
    ECFunc**cacheEntry = getCacheEntry(pMD);
    ECFunc* cur = *cacheEntry;
    if (cur != 0 && cur->m_pMD == pMD)
        return(cur);

    cur = FindImplsForClass(pMD->GetMethodTable());
    if (cur == 0)
        return(0);

    cur = GetECForIndex(FindECIndexForMethod(pMD, cur), cur);
    if (cur == 0)
        return(0);

    *cacheEntry = cur;                          // put in the cache
    return cur;
}


/*******************************************************************************/
/* ID is formed of 2 USHORTs - class index in high word, method index in low word.  */

#define NO_IMPLEMENTATION 0xffff // No class will have this many ecall methods

ECFunc *FindECFuncForID(DWORD id)
{
    if (id == NO_IMPLEMENTATION)
        return NULL;

    USHORT ImplsIndex = (USHORT) (id >> 16);
    USHORT ECIndex = (USHORT) (id & 0xffff);

    return GetECForIndex(ECIndex, GetImplsForIndex(ImplsIndex));
}

DWORD GetIDForMethod(MethodDesc *pMD)
{
    USHORT ImplsIndex = FindImplsIndexForClass(pMD->GetMethodTable());
    if (ImplsIndex == (USHORT) -1)
        return NO_IMPLEMENTATION;
    USHORT ECIndex = FindECIndexForMethod(pMD, GetImplsForIndex(ImplsIndex));
    if (ECIndex == (USHORT) -1)
        return NO_IMPLEMENTATION;

    return (ImplsIndex<<16) | ECIndex;
}


/*******************************************************************************/
/* returns 0 if it is an ECALL, otherwise returns the native entry point (FCALL) */

void* FindImplForMethod(MethodDesc* pMDofCall)  {

    DWORD_PTR rva = pMDofCall->GetRVA();
    
    ECFunc* ret;

    MethodDesc *pBaseMD = pMDofCall;

    //
    // Delegate constructors are FCalls for which the entrypoint points to the target of the delegate
    // We have to intercept these and set the call target to the helper COMDelegate::DelegateConstruct
    // 
    if (pMDofCall->GetClass()->IsAnyDelegateClass()) 
    {
        rva = 0;

        // We need to set up the ECFunc properly.  We don't want to use the pMD passed in,
        // since it may disappear.  Instead, use the stable one on Delegate.  Remember
        // that this is 1:M between the FCall and the pMDofCalls.
        pBaseMD = g_Mscorlib.GetMethod(METHOD__DELEGATE__CONSTRUCT_DELEGATE);
    }

    if (rva == 0)
    {
        ret = FindECFuncForMethod(pBaseMD);
    }
    else
    {
        _ASSERTE(pMDofCall->GetModule()->IsPreload());
        ret = FindECFuncForID((DWORD)(size_t)(rva));
    }

    if (ret == 0)
        return(0);

    ret->m_pMD = pBaseMD;             // remember for reverse mapping

    if (ret->IsFCall()) {
        // Add to the reverse mapping table for FCalls.
        pMDofCall->SetFCall(true);
        gFCallLock.GetLock();
        if(gLowestFCall > ret->m_pImplementation)
            gLowestFCall = (BYTE*) ret->m_pImplementation;
        if(gHighestFCall < ret->m_pImplementation)
            gHighestFCall = (BYTE*) ret->m_pImplementation;

        // add to hash table, makeing sure I am not already there
        ECFunc** spot = &gFCallMethods[FCallHash(ret->m_pImplementation)];
        for(;;) {
            if (*spot == 0) {                   // found end of list
                *spot = ret;
                _ASSERTE(ret->m_pNext == 0);
                break;
            }
            if (*spot == ret)                   // already in list
                break;
            spot = &(*spot)->m_pNext;
        }
        gFCallLock.FreeLock();
    }

    pMDofCall->SetAddrofCode((BYTE*) ret->m_pImplementation);
    if (!ret->IsFCall())
        return(0);
    return(ret->m_pImplementation);
}

/************************************************************************
 * NDirect.SizeOf(Object)
 */


FCIMPL1(VOID, FCComCtor, LPVOID pV)
{
}
FCIMPLEND

ArgBasedStubCache *ECall::m_pECallStubCache = NULL;


/* static */
BOOL ECall::Init()
{
    m_pECallStubCache = new ArgBasedStubCache(ArgBasedStubCache::NUMFIXEDSLOTS << 1);
    if (!m_pECallStubCache) {
        return FALSE;
    }

    m_pArrayStubCache = new ArrayStubCache();
    if (!m_pArrayStubCache) {
        return FALSE;
    }

    //In order for our code that sets up the fast string allocator to work,
    //this FastAllocateString must always be the first method in gStringFuncs.
    //Code in JitInterfacex86.cpp assigns the value to m_pImplementation after
    //that code has been dynamically generated.
#ifdef _X86_
    //
    //
    _ASSERTE(strcmp(gStringFuncs[0].m_wszMethodName ,"FastAllocateString")==0);

    // If allocation logging is on, then calls to FastAllocateString are diverted to this ecall
    // method. This allows us to log the allocation, something that the earlier fcall didnt.
    if (
#ifdef PROFILING_SUPPORTED
        CORProfilerTrackAllocationsEnabled() || 
#endif
        LoggingOn(LF_GCALLOC, LL_INFO10))
    {
        gStringFuncs[0].m_pImplementation = (LPVOID) COMString::SlowAllocateString;
    }
#endif // _X86_

    gFCallLock.Init(LOCK_FCALL);
    return TRUE;
}

/* static */


/* static */
ECFunc* ECall::FindTarget(const void* pTarg)
{
    // Could this possibily be an FCall?
    if (pTarg < gLowestFCall || pTarg > gHighestFCall)
        return(NULL);

    ECFunc *pECFunc = gFCallMethods[FCallHash(pTarg)];
    while (pECFunc != 0) {
        if (pECFunc->m_pImplementation == pTarg)
            return pECFunc;
        pECFunc = pECFunc->m_pNext;
    }
    return NULL;
}

MethodDesc* MapTargetBackToMethod(const void* pTarg)
{
    ECFunc *info = ECall::FindTarget(pTarg);
    if (info == 0)
        return(0);
    return(info->m_pMD);
}

/* static */
CorInfoIntrinsics ECall::IntrinsicID(MethodDesc* pMD)
{
    ECFunc* info = FindTarget(pMD->GetAddrofCode());    // fast hash lookup
    if (info == 0)
        info = FindECFuncForMethod(pMD);                // try slow path

    if (info == 0)
        return(CORINFO_INTRINSIC_Illegal);
    return(info->IntrinsicID());
}

/* static */
Stub *ECall::GetECallMethodStub(StubLinker *pstublinker, ECallMethodDesc *pMD)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!pMD->IsSynchronized());

    MethodTable* pMT = pMD->GetMethodTable();

    if (pMT->IsArray())
    {
        Stub *pstub = GenerateArrayOpStub((CPUSTUBLINKER*)pstublinker, (ArrayECallMethodDesc*)pMD);
        _ASSERTE(pstub != 0);   // Should handle all cases now.
        return pstub;
    }



    if (pMT->IsInterface())
    {
        // If this is one of the managed standard interfaces then insert the check for
        // transparent proxies.
        CRemotingServices::GenerateCheckForProxy((CPUSTUBLINKER*)pstublinker);

        // CheckForProxy never returns
        return pstublinker->Link(pMD->GetClass()->GetClassLoader()->GetStubHeap());
    }


#ifdef _DEBUG
    if (!pMD->IsJitted() && (pMD->GetRVA() == 0)) {
        // ECall is a set of tables to call functions within the EE from the classlibs.
        // First we use the class name & namespace to find an array of function pointers for
        // a class, then use the function name (& sometimes signature) to find the correct
        // function pointer for your method.  Methods in the BCL will be marked as 
        // [MethodImplAttribute(MethodImplOptions.InternalCall)] and extern.
        //
        // You'll see this assert in several situations, almost all being the fault of whomever
        // last touched a particular ecall or fcall method, either here or in the classlibs.
        // However, you must also ensure you don't have stray copies of mscorlib.dll on your machine.
        // 1) You forgot to add your class to gEEClasses, the list of classes w/ ecall & fcall methods.
        // 2) You forgot to add your particular method to the ECFunc array for your class.
        // 3) You misspelled the name of your function and/or classname.
        // 4) The signature of the managed function doesn't match the hardcoded metadata signature
        //    listed in your ECFunc array.  The hardcoded metadata sig is only necessary to disambiguate
        //    overloaded ecall functions - usually you can leave it set to NULL.
        // 5) Your copy of mscorlib.dll & mscoree.dll are out of sync - rebuild both.
        // 6) You've loaded the wrong copy of mscorlib.dll.  In msdev's debug menu,
        //    select the "Modules..." dialog.  Verify the path for mscorlib is right.
        // 7) Someone mucked around with how the signatures in metasig.h are parsed, changing the
        //    interpretation of a part of the signature (this is very rare & extremely unlikely,
        //    but has happened at least once).
        _ASSERTE(!"Could not find an ECALL entry for a function!  Read comment above this assert in vm/ecall.cpp");
    }
#endif // _DEBUG



    INDEBUG(printf("%s::%s -- ECALL\n", pMD->m_pszDebugClassName, pMD->m_pszDebugMethodName));
    _ASSERTE(!"ECALLs not supported");
    return NULL;
}



/*static*/




#ifdef _DEBUG
void FCallAssert(void*& cache, void* target) 
{
    if (cache == 0) 
    {
        // special case: COMDelegate::DelegateConstruct is not in the list.
        // also note that this check has to occur prior to the IA64 code below
        // so that it can be platform independent
        if (target == COMDelegate::DelegateConstruct)
        {
            cache = (void*)1;
            return;
        }

        //
        // this also has a many MethodDesc for one FCall, so we special case it as well
        //
        if (target == FCComCtor)
        {
            cache = (void*)1;
            return;
        }


        MethodDesc* pMD = MapTargetBackToMethod(target);
        if (pMD != 0) 
        {
            _ASSERTE(FindImplForMethod(pMD) && "Use FCFuncElement not ECFuncElement");
            return;
        }

            // Slow but only for debugging.  This is needed because in some places
            // we call FCALLs directly from EE code.

        unsigned num = sizeof(gECClasses)/sizeof(ECClass);
        for (unsigned i=0; i < num; i++) 
        {
            ECFunc* ptr = gECClasses[i].m_pECFunc;
            while (ptr->m_pImplementation != 0) 
            {
                if (ptr->m_pImplementation  == target)
                {
                    _ASSERTE(ptr->IsFCall() && "Use FCFuncElement not ECFuncElement");
                    cache = ptr->m_pImplementation;
                    return;
                }
                ptr++;
                }
            }

        _ASSERTE(!"Could not find FCall implemenation in ECall.cpp");
    }
}

void HCallAssert(void*& cache, void* target) {
    if (cache != 0)
        cache = MapTargetBackToMethod(target);
    _ASSERTE(cache == 0 || "Use FCIMPL for fcalls");
}

#endif


