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
** Class: CasingHelper
**
**                                
**
** Purpose: Provide locale-correct casing operations in a somewhat
**          efficient manner.  Designed to be used by native code
**          (type lookup, reflection, etc).
**
** Date: October 14, 1999
**
============================================================*/
#include "stdafx.h"
#include "utilcode.h"

INT32 CasingHelper::InvariantToLower(LPUTF8 szOut, int cMaxBytes, LPCUTF8 szIn) {
    _ASSERTE(szOut);
    _ASSERTE(szIn);

    //Figure out the maximum number of bytes which we can copy without
    //running out of buffer.  If cMaxBytes is less than inLength, copy
    //one fewer chars so that we have room for the null at the end;
    int inLength = (int)(strlen(szIn)+1);
    int copyLen  = (inLength<=cMaxBytes)?inLength:(cMaxBytes-1);
    LPUTF8 szEnd;

    //Compute our end point.
    szEnd = szOut + copyLen;

    //Walk the string copying the characters.  Change the case on
    //any character between A-Z.
    for (; szOut<szEnd; szOut++, szIn++) {
        if (*szIn>='A' && *szIn<='Z') {
            *szOut = *szIn | 0x20;
        } else {
            *szOut = *szIn;
        }
    }

    //If we copied everything, tell them how many characters we copied, 
    //and arrange it so that the original position of the string + the returned
    //length gives us the position of the null (useful if we're appending).
    if (copyLen==inLength) {
        return copyLen-1;
    } else {
        *szOut=0;
        return -(inLength - copyLen);
    }
}

BOOL CasingHelper::IsLowerCase(LPCUTF8 szIn) {
    
    if (!szIn) {
        return TRUE;
    }

    for (;*szIn; szIn++) {
        if (*szIn>='A' && *szIn<='Z') {
            return FALSE;
        }
    }

    return TRUE;
}
