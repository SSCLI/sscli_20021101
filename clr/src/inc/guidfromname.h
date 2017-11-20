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
#ifndef GUIDFROMNAME_H_
#define GUIDFROMNAME_H_

// GuidFromName.h - function prototype

void CorGuidFromNameW
(
    GUID *  pGuidResult,        // resulting GUID
    LPCWSTR wzName,             // the unicode name from which to generate a GUID
    SIZE_T  cchName             // name length in count of unicode character
);

void CorIIDFromCLSID
(
    GUID *  pGuidResult,        // resulting GUID
    REFGUID GuidClsid           // CLSID from which to derive GUID.
);

#endif // GUIDFROMNAME_H_
