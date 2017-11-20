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
** Header:  AssemblySink.hpp
**
** Purpose: Asynchronous call back for loading classes
**
** Date:  June 23, 1999
**
===========================================================*/
#ifndef _ASSEMBLYSINK_H
#define _ASSEMBLYSINK_H

class AppDomain;

class AssemblySink : public FusionSink
{
public:

    AssemblySink(AppDomain* pDomain);

    ULONG STDMETHODCALLTYPE Release(void);

#ifdef DEBUG
    STDMETHODIMP OnProgress(DWORD dwNotification,
                            HRESULT hrNotification,
                            LPCWSTR szNotification,
                            DWORD dwProgress,
                            DWORD dwProgressMax,
                            IUnknown* punk)
    {
        return FusionSink::OnProgress(dwNotification,
                                      hrNotification,
                                      szNotification,
                                      dwProgress,
                                      dwProgressMax,
                                      punk);
    }
#endif

    virtual HRESULT Wait();

private:
    DWORD m_Domain; // Which domain (index) do I belong to
};

#endif
