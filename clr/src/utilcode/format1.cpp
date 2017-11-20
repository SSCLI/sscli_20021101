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
/***************************************************************************/
/* routines for parsing file format stuff ... */
/* this is split off from format.cpp because this uses meta-data APIs that
   are not present in many builds.  Thus if someone needs things in the format.cpp
   file but does not have the meta-data APIs, I want it to link */

#include "stdafx.h"
#include "cor.h"
#include "corpriv.h"

/***************************************************************************/
COR_ILMETHOD_DECODER::COR_ILMETHOD_DECODER(COR_ILMETHOD* header, void *pInternalImport, bool verify) {

    PAL_TRY {
        // Decode the COR header into a more convenient form
        DecoderInit(this, (COR_ILMETHOD *) header);
    } 
    PAL_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        Code = 0; 
        SetLocalVarSigTok(0); 
    }
    PAL_ENDTRY

        // If there is a local variable sig, fetch it into 'LocalVarSig'    
    if (GetLocalVarSigTok() && pInternalImport)
    {
        IMDInternalImport* pMDI = reinterpret_cast<IMDInternalImport*>(pInternalImport);

        if (verify) {
            if ((!pMDI->IsValidToken(GetLocalVarSigTok())) || (TypeFromToken(GetLocalVarSigTok()) != mdtSignature)
                || (RidFromToken(GetLocalVarSigTok())==0)) {
                Code = 0;      // failure bad local variable signature token
                return;
            }
        }
        
        DWORD cSig = 0; 
        LocalVarSig = pMDI->GetSigFromToken(GetLocalVarSigTok(), &cSig); 
        
        if (verify) {
            if (!SUCCEEDED(validateTokenSig(GetLocalVarSigTok(), LocalVarSig, cSig, 0, pMDI)) ||
                *LocalVarSig != IMAGE_CEE_CS_CALLCONV_LOCAL_SIG) {
                Code = 0;
                return;
            }
        }
        
    }
}

