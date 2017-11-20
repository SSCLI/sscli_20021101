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
#ifndef __HISTNODE_H_INCLUDED__
#define __HISTNODE_H_INCLUDED__

#include "histinfo.h"

class CHistoryInfoNode {
    public:
        CHistoryInfoNode();
        virtual ~CHistoryInfoNode();

        HRESULT Init(AsmBindHistoryInfo *pHistInfo);
        static HRESULT Create(AsmBindHistoryInfo *pHistInfo, CHistoryInfoNode **pphn);

    public:
        DWORD                                   _dwSig;
        AsmBindHistoryInfo                      _bindHistoryInfo;
};



#endif

