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
/*++

Module Name: EHEncoder.h

Abstract: Defines the EH encoder interface that is used by both the VM (for decoding) 
          and JITters (for encoding)



--*/

#include "corjit.h"

class EHEncoder
{
public:

#ifdef USE_EH_ENCODER
static	void encode(BYTE** dest, unsigned val);
static	void encode(BYTE** dest, CORINFO_EH_CLAUSE val);
#endif

#ifdef USE_EH_DECODER
static	unsigned decode(const BYTE* src, unsigned* val);
static	unsigned decode(const BYTE* src, CORINFO_EH_CLAUSE *clause);
#endif

};




