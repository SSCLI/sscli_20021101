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

#ifndef __LOCK_H_
#define __LOCK_H_

#include "helpers.h"
#include "fusionp.h"

class CCriticalSection
{
    public:
        CCriticalSection(CRITICAL_SECTION *pcs)
        : _pcs(pcs)
        , _bEntered(FALSE)
        {
            ASSERT(pcs);
        }

        ~CCriticalSection()
        {
            if (_bEntered) {
                ::LeaveCriticalSection(_pcs);
            }
        }

        HRESULT Lock()
        {
            HRESULT                          hr = S_OK;
            
            PAL_TRY {
                ASSERT(!_bEntered);
                ::EnterCriticalSection(_pcs);
            }
            PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                hr = E_OUTOFMEMORY;
            }
            PAL_ENDTRY

            if (hr == S_OK) {
                _bEntered = TRUE;
            }

            return hr;
        }

        HRESULT Unlock()
        {
            HRESULT                      hr = S_OK;
            
            if (_bEntered) {
                ::LeaveCriticalSection(_pcs);
                _bEntered = FALSE;
            }
            else {
                ASSERT(0);
                hr = E_UNEXPECTED;
            }

            return hr;
        }

    private:
        CRITICAL_SECTION                    *_pcs;
        BOOL                                 _bEntered;
};

class CMutex
{
    public:
        CMutex(HANDLE hMutex)
        : _hMutex(hMutex)
        , _bLocked(FALSE)
        {
            ASSERT(hMutex);
        }

        ~CMutex()
        {
            if (_bLocked) {
                if(!(::ReleaseMutex(_hMutex))){
#ifdef DEBUG
                    WCHAR szMsgBuf[MAX_PATH*2 + 1];
                    wnsprintfW( szMsgBuf, MAX_PATH*2, L"ReleaseMutex FAILED for <%x>, hr = <%x>\r\n", 
                                                    _hMutex, FusionpHresultFromLastError());
                    WriteToLogFile(szMsgBuf);
#endif  // DEBUG
                }
            }
        }

        HRESULT Lock()
        {
            HRESULT                          hr = S_OK;
            DWORD                            dwWait;

            if(_hMutex == INVALID_HANDLE_VALUE) // no need to take lock.
                goto exit;

            ASSERT(!_bLocked);
            dwWait = ::WaitForSingleObject(_hMutex, INFINITE);
            if((dwWait != WAIT_OBJECT_0) && (dwWait != WAIT_ABANDONED)){
                    hr = FusionpHresultFromLastError();
#ifdef DEBUG
                WCHAR szMsgBuf[MAX_PATH*2 + 1];
                wnsprintfW( szMsgBuf, MAX_PATH*2, L"WaitForMutex FAILED for <%x>, hr = <%x>, Error=<%x> \r\n", 
                                                _hMutex, hr, GetLastError());
                WriteToLogFile(szMsgBuf);
#endif  // DEBUG
            }

            if (hr == S_OK) {
                _bLocked = TRUE;
            }

        exit :
            return hr;
        }

        HRESULT Unlock()
        {
            HRESULT                      hr = S_OK;

            if (_bLocked) {
                if(!(::ReleaseMutex(_hMutex))){
                    hr = FusionpHresultFromLastError();
#ifdef DEBUG
                    WCHAR szMsgBuf[MAX_PATH*2 + 1];
                    wnsprintfW( szMsgBuf, MAX_PATH*2, L"ReleaseMutex FAILED for <%x>, hr = <%x>\r\n",
                                                    _hMutex, hr);
                    WriteToLogFile(szMsgBuf);
#endif  // DEBUG
                }
                _bLocked = FALSE;
            }
            else {
                ASSERT(0);
                hr = E_UNEXPECTED;
            }

            return hr;
        }

    private:
        HANDLE                               _hMutex;
        BOOL                                 _bLocked;
};

#endif  // __LOCK_H_
