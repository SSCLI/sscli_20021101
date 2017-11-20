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
// ============================================================================
// File: stackwalktypes.h
//
// ============================================================================
// Contains types used by stackwalk.h.

#ifndef __STACKWALKTYPES_H__
#define __STACKWALKTYPES_H__

class CrawlFrame;

struct _METHODTOKEN;
typedef struct _METHODTOKEN * METHODTOKEN;

//************************************************************************
// Stack walking
//************************************************************************
enum StackCrawlMark
{
    LookForMe = 0,
    LookForMyCaller = 1,
        LookForMyCallersCaller = 2,
};

enum StackWalkAction {
    SWA_CONTINUE    = 0,    // continue walking
    SWA_ABORT       = 1,    // stop walking, early out in "failure case"
    SWA_FAILED      = 2     // couldn't walk stack
};

#define SWA_DONE SWA_CONTINUE


// Pointer to the StackWalk callback function.
typedef StackWalkAction (*PSTACKWALKFRAMESCALLBACK)(
    CrawlFrame       *pCF,      //
    VOID*             pData     // Caller's private data

);

#endif  // __STACKWALKTYPES_H__
