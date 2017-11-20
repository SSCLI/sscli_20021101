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
/*  TLS.CPP:
 *
 *  Encapsulates TLS access for maximum performance. 
 *
 */

#include "common.h"

#include "tls.h"





#ifdef _DEBUG


static DWORD gSaveIdx;
LPVOID __stdcall GenericTlsGetValue()
{
    return TlsGetValue(gSaveIdx);
}


VOID ExerciseTlsStuff()
{

    // Exercise the TLS stub generator for as many indices as we can.
    // Ideally, we'd like to test:
    //
    //      0  (boundary case)
    //      31 (boundary case for Win95)
    //      32 (boundary case for Win95)
    //      63 (boundary case for WinNT 5)
    //      64 (boundary case for WinNT 5)
    //
    // Since we can't choose what index we get, we'll just
    // do as many as we can.
    DWORD tls[128];
    int i;
    PAL_TRY {
        for (i = 0; i < 128; i++) {
            tls[i] = ((DWORD)(-1));
        }

        for (i = 0; i < 128; i++) {
            tls[i] = TlsAlloc();

            if (tls[i] == ((DWORD)(-1))) {
                PAL_LEAVE;
            }
            LPVOID data = (LPVOID)(DWORD_PTR)(0x12345678+i*8);
            TlsSetValue(tls[i], data);
            gSaveIdx = tls[i];
            POPTIMIZEDTLSGETTER pGetter1 = MakeOptimizedTlsGetter(tls[i], GenericTlsGetValue);
            if (!pGetter1) {
                PAL_LEAVE;
            }


            _ASSERTE(data == pGetter1());

            FreeOptimizedTlsGetter(tls[i], pGetter1);
        }
    }
    PAL_FINALLY {
        for (i = 0; i < 128; i++) {
            if (tls[i] != ((DWORD)(-1))) {
                TlsFree(tls[i]);
            }
        }
    }
    PAL_ENDTRY
}

#endif // _DEBUG


//---------------------------------------------------------------------------
// Win95 and WinNT store the TLS in different places relative to the
// fs:[0]. This api reveals which. Can also return TLSACCESS_GENERIC if
// no info is available about the Thread location (you have to use the TlsGetValue
// api.) This is intended for use by stub generators that want to inline TLS
// access.
//---------------------------------------------------------------------------
TLSACCESSMODE GetTLSAccessMode(DWORD tlsIndex)
{
    TLSACCESSMODE tlsAccessMode = TLSACCESS_GENERIC;
    THROWSCOMPLUSEXCEPTION();



#ifdef _DEBUG
    {
        static BOOL firstTime = TRUE;
        if (firstTime) {
            firstTime = FALSE;
            ExerciseTlsStuff();
        }
    }
#endif

    return tlsAccessMode;
}


//---------------------------------------------------------------------------
// Creates a platform-optimized version of TlsGetValue compiled
// for a particular index. 
//
// LIMITATION: We make the client provide the function ("pGenericGetter") when the 
// access mode is TLSACCESS_GENERIC (all it has to do is call TlsGetValue
// for the specific TLS index.) This is because the generic getter has to
// be platform independent and the TLS manager can't create that at runtime.
// While it's possible to simulate these, it requires more machinery and code
// than is worth given that this service has only one or two clients.
//---------------------------------------------------------------------------
POPTIMIZEDTLSGETTER MakeOptimizedTlsGetter(DWORD tlsIndex, POPTIMIZEDTLSGETTER pGenericGetter)
{
    _ASSERTE(pGenericGetter != NULL);

    LPBYTE pCode = NULL;
    switch (GetTLSAccessMode(tlsIndex)) {


        case TLSACCESS_GENERIC:
            pCode = (LPBYTE)pGenericGetter;
            break;
    }
    return (POPTIMIZEDTLSGETTER)pCode;
}


//---------------------------------------------------------------------------
// Frees a function created by MakeOptimizedTlsGetter(). If the access
// mode was TLSACCESS_GENERIC, this function safely does nothing since
// the function was actually provided by the client.
//---------------------------------------------------------------------------
VOID FreeOptimizedTlsGetter(DWORD tlsIndex, POPTIMIZEDTLSGETTER pOptimizedTlsGetter)
{
}
