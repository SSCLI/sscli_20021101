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
// File: predefattr.h
//
// Contains a list of the predefined attributes.
// ===========================================================================

#if !defined(PREDEFATTRDEF)
#error Must define PREDEFATTRDEF macro before including predefattr.h
#endif

//            id                   name                 type                attribute usage

// special behaviour attributes
PREDEFATTRDEF(PA_ATTRIBUTEUSAGE,   PN_COUNT,            PT_ATTRIBUTEUSAGE,  0)
PREDEFATTRDEF(PA_OBSOLETE,         PN_COUNT,            PT_OBSOLETE,        0)
PREDEFATTRDEF(PA_CLSCOMPLIANT,     PN_COUNT,            PT_CLSCOMPLIANT,    0)
PREDEFATTRDEF(PA_CONDITIONAL,      PN_COUNT,            PT_CONDITIONAL,     0)
PREDEFATTRDEF(PA_REQUIRED,         PN_COUNT,            PT_REQUIRED,        0)

// magic attributes
PREDEFATTRDEF(PA_NAME,             PN_COUNT,            PT_INDEXERNAME,     0)

// extra checking for interop only
PREDEFATTRDEF(PA_DLLIMPORT,        PN_COUNT,            PT_DLLIMPORT,       0)
PREDEFATTRDEF(PA_COMIMPORT,        PN_COUNT,            PT_COMIMPORT,       0)
PREDEFATTRDEF(PA_GUID,             PN_COUNT,            PT_GUID,            0)
PREDEFATTRDEF(PA_IN,               PN_COUNT,            PT_IN,              0)
PREDEFATTRDEF(PA_OUT,              PN_COUNT,            PT_OUT,             0)
PREDEFATTRDEF(PA_STRUCTOFFSET,     PN_COUNT,            PT_FIELDOFFSET,     0)
PREDEFATTRDEF(PA_STRUCTLAYOUT,     PN_COUNT,            PT_STRUCTLAYOUT,    0)
PREDEFATTRDEF(PA_PARAMARRAY,       PN_COUNT,            PT_PARAMS,          0)
PREDEFATTRDEF(PA_COCLASS,          PN_COUNT,            PT_COCLASS,         0)
