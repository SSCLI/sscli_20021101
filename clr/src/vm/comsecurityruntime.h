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
** Header:  COMSecurityRuntime.h
**       
**
** Purpose:
**
** Date:  March 21, 1998
**
===========================================================*/
#ifndef __ComSecurityRuntime_h__
#define __ComSecurityRuntime_h__

#include "common.h"

#include "object.h"
#include "util.hpp"

// Field names inside managed class FrameSecurityDescriptor
#define ASSERT_PERMSET			"m_assertions"
#define DENY_PERMSET			"m_denials"
#define RESTRICTION_PERMSET		"m_restriction"

// Forward declarations to avoid pulling in too many headers.
class Frame;
enum StackWalkAction;


//-----------------------------------------------------------
// The COMSecurityRuntime implements all the native methods
// for the interpreted System/Security/SecurityRuntime.
//-----------------------------------------------------------
class COMSecurityRuntime
{
public:
    //-----------------------------------------------------------
    // Argument declarations for native methods.
    //-----------------------------------------------------------
    
    //typedef struct _InitSecurityRuntimeArgs
    //{
    //    OBJECTREF This;
    //} InitSecurityRuntimeArgs;
    
    
    //typedef struct _GetDeclaredPermissionsArg
    //{
    //    OBJECTREF This;
    //    INT32     iType;
    //    OBJECTREF pClass;
    //} GetDeclaredPermissionsArg;
    
    
    //typedef struct _GetSecurityObjectForFrameArgs
    //{
    //    INT32           create;
    //    StackCrawlMark* stackMark;
    //} GetSecurityObjectForFrameArgs;

    //typedef struct _SetSecurityObjectForFrameArgs
    //{
    //    OBJECTREF       pInputRefSecDesc;
    //    StackCrawlMark* stackMark;
    //} SetSecurityObjectForFrameArgs;


public:
    // Initialize the security engine. This is called when a SecurityRuntime
    // object is created, indicating that code-access security is to be
    // enforced. This should be called only once.
    static FCDECL1(void, InitSecurityRuntime, Object* ThisUNSAFE);
    // private helper for getting a security object
    static FCDECL2(Object*, GetSecurityObjectForFrame, StackCrawlMark* stackMark, INT32 create);
    static FCDECL2(void, SetSecurityObjectForFrame, StackCrawlMark* stackMark, Object* pInputRefSecDesc);
    static FCDECL3(Object*, GetDeclaredPermissionsP, Object* ThisUNSAFE, Object* pClassUNSAFE, INT32 iType);

    static BOOL IsInitialized() { return s_srData.fInitialized; }

    static FieldDesc* GetFrameSecDescField(DWORD dwAction);

    static void InitSRData();

protected:

	//-----------------------------------------------------------
	// Cached class and method pointers.
	//-----------------------------------------------------------
	typedef struct _SRData
	{
		BOOL		fInitialized;
		MethodTable * pSecurityRuntime;
		MethodTable * pFrameSecurityDescriptor;
		FieldDesc   * pFSD_assertions;	// Fields in FrameSecurityDescriptor
		FieldDesc   * pFSD_denials;
		FieldDesc   * pFSD_restriction;
	} SRData;

public:

	static SRData s_srData;

};


#endif /* __ComSecurityRuntime_h__ */

