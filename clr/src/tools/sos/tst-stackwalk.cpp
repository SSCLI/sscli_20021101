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
/*  STACKWALK.CPP:
 *
 */

#ifdef _MSC_VER
#pragma warning(disable:4189)
#pragma warning(disable:4244) // conversion from 'unsigned int' to 'unsigned short', possible loss of data
#endif //_MSC_VER

#include <malloc.h>

#include "exts.h"
#include "util.h"
#include "eestructs.h"


#ifndef PLATFORM_UNIX
#ifdef _DEBUG
#include <assert.h>
#define _ASSERTE(a) assert(a)
#else
#define _ASSERTE(a)
#define assert(a)
#endif
#endif //!PLATFORM_UNIX

/*
template <class T>
class smartPtr
{
public:
    smartPtr(DWORD_PTR remoteAddr) : m_remoteAddr(remoteAddr) { }

    T operator *()
    {
        T var;
        safemove(var, m_remoteAddr);
        return var;
    }

    T *operator&()
    {
        return (T *)m_remoteAddr;
    }

    operator++()
    {
        m_remoteAddr += sizeof(T);
    }

    operator--()
    {
        m_remoteAddr -= sizeof(T);
    }

    T *operator ()
    {
        return 
    }

protected:
    DWORD_PTR m_remoteAddr;
};
*/

Frame *g_pFrameNukeList = NULL;

void CleanupStackWalk()
{
    while (g_pFrameNukeList != NULL)
    {
        Frame *pDel = g_pFrameNukeList;
        g_pFrameNukeList = g_pFrameNukeList->m_pNukeNext;
        delete pDel;
    }
}

void UpdateJitMan(SLOT PC, IJitManager **ppIJM)
{
    *ppIJM = NULL;

    // Get the jit manager and assign appropriate fields into crawl frame
    JitMan jitMan;
    FindJitMan(PC, jitMan);

    if (jitMan.m_RS.pjit != 0)
    {
        switch (jitMan.m_jitType)
        {
        case JIT:
            {
                static EEJitManager eejm;
                DWORD_PTR pjm = jitMan.m_RS.pjit;
                eejm.Fill(pjm);

                pjm = jitMan.m_RS.pjit;
                eejm.IJitManager::Fill(pjm);

                eejm.m_jitMan = jitMan;
                *ppIJM = (IJitManager *)&eejm;
            }
            break;
        default:
            DebugBreak();
            break;
        }
    }
}

