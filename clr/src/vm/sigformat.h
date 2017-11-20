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
////////////////////////////////////////////////////////////////////////////////
// This Module contains routines that expose properties of Member (Classes, Constructors
//  Interfaces and Fields)
//
// Date: March/April 1998
////////////////////////////////////////////////////////////////////////////////


#ifndef _SIGFORMAT_H
#define _SIGFORMAT_H

#include "comclass.h"
#include "invokeutil.h"
#include "reflectutil.h"
#include "comstring.h"
#include "comvariant.h"
#include "comvarargs.h"
#include "field.h"

#define SIG_INC 256

class SigFormat
{
public:
	SigFormat();

	SigFormat(MethodDesc* pMeth, TypeHandle arrayType, BOOL fIgnoreMethodName = false);
	SigFormat(MetaSig &metaSig, LPCUTF8 memberName, LPCUTF8 className = NULL, LPCUTF8 ns = NULL);
    
	void FormatSig(MetaSig &metaSig, LPCUTF8 memberName, LPCUTF8 className = NULL, LPCUTF8 ns = NULL);
	
	~SigFormat();
	
	STRINGREF GetString();
	const char * GetCString();
	const char * GetCStringParmsOnly();
	
	int AddType(TypeHandle th);

protected:
	char*		_fmtSig;
	int			_size;
	int			_pos;
    TypeHandle  _arrayType; // null type handle if the sig is not for an array. This is currently only set 
                            // through the ctor taking a MethodInfo as its first argument. It will have to be 
                            // exposed some other way to be used in a more generic fashion

	int AddSpace();
	int AddString(LPCUTF8 s);
	
};

class FieldSigFormat : public SigFormat
{
public:
	FieldSigFormat(FieldDesc* pFld);
};

class PropertySigFormat : public SigFormat
{
public:
	PropertySigFormat(MetaSig &metaSig, LPCUTF8 memberName);
	void FormatSig(MetaSig &sig, LPCUTF8 memberName);
};

#endif // _SIGFORMAT_H

