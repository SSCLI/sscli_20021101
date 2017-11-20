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
// ===========================================================================
// File: symkinds.h
//
// Contains a list of the various concrete symbol kinds. Abstract symbol
// kinds are NOT defined here.
// ===========================================================================

#if !defined(SYMBOLDEF)
#error Must define SYMBOLDEF macro before including symkinds.h
#endif

SYMBOLDEF(NSSYM)
SYMBOLDEF(NSDECLSYM)
SYMBOLDEF(AGGSYM)
SYMBOLDEF(INFILESYM)
SYMBOLDEF(RESFILESYM)
SYMBOLDEF(OUTFILESYM)
SYMBOLDEF(MEMBVARSYM)
SYMBOLDEF(LOCVARSYM)
SYMBOLDEF(METHSYM)
SYMBOLDEF(FAKEMETHSYM) // this has got to come after METHSYM
SYMBOLDEF(PROPSYM)
SYMBOLDEF(FAKEPROPSYM)  // this has got to come after PROPSYM
SYMBOLDEF(SCOPESYM)
SYMBOLDEF(ARRAYSYM)
SYMBOLDEF(PTRSYM)
SYMBOLDEF(PINNEDSYM)
SYMBOLDEF(PARAMMODSYM)
SYMBOLDEF(VOIDSYM)
SYMBOLDEF(NULLSYM)
SYMBOLDEF(CACHESYM)
SYMBOLDEF(LABELSYM)
SYMBOLDEF(ERRORSYM)
SYMBOLDEF(ALIASSYM)
SYMBOLDEF(EXPANDEDPARAMSSYM)
SYMBOLDEF(INDEXERSYM)
SYMBOLDEF(PREDEFATTRSYM)
SYMBOLDEF(GLOBALATTRSYM)
SYMBOLDEF(EVENTSYM)
SYMBOLDEF(IFACEIMPLMETHSYM)
SYMBOLDEF(DELETEDTYPESYM)
SYMBOLDEF(SYNTHINFILESYM)

// 32 total, that's the max...
