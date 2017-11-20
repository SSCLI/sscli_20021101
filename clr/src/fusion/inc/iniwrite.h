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
#ifndef __INIWRITE_H_INCLUDED__
#define __INIWRITE_H_INCLUDED__

#include "histinfo.h"

class CIniWriter {
    public:
        CIniWriter();
        virtual ~CIniWriter();

        HRESULT Init(LPCWSTR pwzFileName);

        HRESULT AddSnapShot(LPCWSTR wzActivationDate, LPCWSTR wzURTVersion);
        HRESULT DeleteSnapShot(LPCWSTR wzActivationDate);
        HRESULT AddAssembly(LPCWSTR wzActivationDate, AsmBindHistoryInfo *pHist);
        HRESULT InsertHeaderData(LPCWSTR wzEXEPath, LPCWSTR wzAppName);

        HINI GetHIni();
        HRESULT Load();
        HRESULT Save(BOOL fFlush);

    private:
        DWORD                        _dwSig;
        LPWSTR                       _pwzFileName;

        HINI                         _hIni;
};

#endif

