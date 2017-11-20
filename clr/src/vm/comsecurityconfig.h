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
////////////////////////////////////////////////////////////////////////////////
//
//   File:          COMSecurityConfig.h
//
//   Purpose:       Native implementation for security config access and manipulation
//
//   Date created : August 30, 2000
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMSecurityConfig_H_
#define _COMSecurityConfig_H_

#include "arraylist.h"

class COMSecurityConfig
{
public:

    // Duplicated in System.Security.Util.Config.cs
    enum ConfigId
    {
        None = 0,
        MachinePolicyLevel = 1,
        UserPolicyLevel = 2,
        EnterprisePolicyLevel = 3
    };

    // Duplicated in System.Security.Util.Config.cs
    enum QuickCacheEntryType
    {
        ExecutionZoneMyComputer = 0x1,
        ExecutionZoneIntranet = 0x2,
        ExecutionZoneInternet = 0x4,
        ExecutionZoneTrusted = 0x8,
        ExecutionZoneUntrusted = 0x10,
        RequestSkipVerificationZoneMyComputer = 0x20,
        RequestSkipVerificationZoneIntranet = 0x40,
        RequestSkipVerificationZoneInternet = 0x80,
        RequestSkipVerificationZoneTrusted = 0x100,
        RequestSkipVerificationZoneUntrusted = 0x200,
        UnmanagedZoneMyComputer = 0x400,
        UnmanagedZoneIntranet = 0x800,
        UnmanagedZoneInternet = 0x1000,
        UnmanagedZoneTrusted = 0x2000,
        UnmanagedZoneUntrusted = 0x4000,
        ExecutionAll = 0x8000,
        RequestSkipVerificationAll = 0x10000,
        UnmanagedAll = 0x20000,
        SkipVerificationZoneMyComputer = 0x40000,
        SkipVerificationZoneIntranet = 0x80000,
        SkipVerificationZoneInternet = 0x100000,
        SkipVerificationZoneTrusted = 0x200000,
        SkipVerificationZoneUntrusted = 0x400000,
        SkipVerificationAll = 0x800000,
        FullTrustZoneMyComputer = 0x1000000,
        FullTrustZoneIntranet = 0x2000000,
        FullTrustZoneInternet = 0x4000000,
        FullTrustZoneTrusted = 0x8000000,
        FullTrustZoneUntrusted = 0x10000000,
        FullTrustAll = 0x20000000,
    };

    // Duplicated in System.Security.Util.Config.cs
    enum ConfigRetval
    {
        NoFile = 0,
        ConfigFile = 1,
        CacheFile = 2
    };

    //struct _InitDataEx {
    //    DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, cache );
    //    DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, config );
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL3(INT32, EcallInitDataEx, INT32 id, StringObject* configUNSAFE, StringObject* cacheUNSAFE);

    //struct _InitData {
    //    DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, config );
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL2(INT32, EcallInitData, INT32 id, StringObject* configUNSAFE);
    static ConfigRetval InitData( INT32 id, WCHAR* configFileName, WCHAR* cacheFileName );
    static ConfigRetval InitData( void* configData, BOOL addToList );

    //struct _SaveCacheData {
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL1(BOOL, EcallSaveCacheData, INT32 id);
    static BOOL SaveCacheData( INT32 id );

    //struct _ResetCacheData {
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL1(void, EcallResetCacheData, INT32 id);
    static void ResetCacheData( INT32 id );

    //struct _ClearCacheData {
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL1(void, EcallClearCacheData, INT32 id);
    static void ClearCacheData( INT32 id );


    //struct _SaveDataString {
    //    DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, data );
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL2(BOOL, EcallSaveDataString, INT32 id, StringObject* dataUNSAFE);

    //struct _SaveDataByte {
    //    DECLARE_ECALL_I4_ARG( INT32, length );
    //    DECLARE_ECALL_I4_ARG( INT32, offset );
    //    DECLARE_ECALL_OBJECTREF_ARG( U1ARRAYREF, data );
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL4(BOOL, EcallSaveDataByte, INT32 id, U1Array* dataUNSAFE, INT32 offset, INT32 length);
    static BOOL __stdcall SaveData( INT32 id, void* buffer, size_t bufferSize );

    //struct _RecoverData {
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL1(BOOL, EcallRecoverData, INT32 id);
    static BOOL RecoverData( INT32 id );

    //struct _GetRawData {
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL1(Object*, GetRawData, INT32 id);

    //struct _SetGetQuickCacheEntry {
    //    DECLARE_ECALL_I4_ARG( QuickCacheEntryType, type );
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL2(DWORD, EcallGetQuickCacheEntry, INT32 id, QuickCacheEntryType type);
    static DWORD GetQuickCacheEntry( INT32 id, QuickCacheEntryType type );
    static FCDECL2(void, EcallSetQuickCache, INT32 id, QuickCacheEntryType type);
    static void SetQuickCache( INT32 id, QuickCacheEntryType type );

    //struct _GetCacheEntry {
    //    DECLARE_ECALL_PTR_ARG( CHARARRAYREF*, policy );
    //    DECLARE_ECALL_OBJECTREF_ARG( CHARARRAYREF, evidence );
    //    DECLARE_ECALL_I4_ARG( DWORD, numEvidence );
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL4(BOOL, GetCacheEntry, INT32 id, DWORD numEvidence, CHARArray* evidenceUNSAFE, CHARARRAYREF* policy);

    //struct _AddCacheEntry {
    //    DECLARE_ECALL_OBJECTREF_ARG( CHARARRAYREF, policy );
    //    DECLARE_ECALL_OBJECTREF_ARG( CHARARRAYREF, evidence );
    //    DECLARE_ECALL_I4_ARG( DWORD, numEvidence );
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL4(void, AddCacheEntry, INT32 id, DWORD numEvidence, CHARArray* evidenceUNSAFE, CHARArray* policyUNSAFE);

    //struct _GetCacheSecurityOn {
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL1(DWORD, GetCacheSecurityOn, INT32 id);

    //struct _SetCacheSecurityOn {
    //    DECLARE_ECALL_I4_ARG( INT32, value );
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL2(void, SetCacheSecurityOn, INT32 id, INT32 value);

    //struct _NoArgs {
    //};
    static FCDECL0(Object*, EcallGetMachineDirectory);
    static FCDECL0(Object*, EcallGetUserDirectory);
    static FCDECL0(BOOL, EcallIsCompilationDomain);

    static BOOL GetMachineDirectory( WCHAR* buffer, size_t bufferCount );
    static BOOL GetUserDirectory( WCHAR* buffer, size_t bufferCount, BOOL fTryDefault );

    //struct _WriteToEventLog{
    //    DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, message );
    //};
    static FCDECL1(BOOL, EcallWriteToEventLog, StringObject* messageUNSAFE);
    static BOOL WriteToEventLog( WCHAR* buffer );

    //struct _GetStoreLocation {
    //    DECLARE_ECALL_I4_ARG( INT32, id );
    //};
    static FCDECL1(Object*, EcallGetStoreLocation, INT32 id);
    static BOOL GetStoreLocation( INT32 id, WCHAR* buffer, size_t bufferCount );

    //struct _TurnCacheOff {
    //    DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackmark);
    //};
    static FCDECL1(void, EcallTurnCacheOff, StackCrawlMark* stackmark);

    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,  message);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,  file);
    //} _DebugOut;    
    static FCDECL2(HRESULT, DebugOut, StringObject* fileUNSAFE, StringObject* messageUNSAFE);

    static void Init( void );
    static void Cleanup( void );

    static void* GetData( INT32 id );

    static ArrayListStatic  entries_;
    static CrstStatic       dataLock_;
};


#endif


