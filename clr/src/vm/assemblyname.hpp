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
** Header:  AssemblyName.hpp
**
** Purpose: Implements AssemblyName (loader domain) architecture
**
** Date:  August 10, 1999
**
===========================================================*/
#ifndef _AssemblyName_H
#define _AssemblyName_H

class AssemblyName
{
	friend class BaseDomain;

 public:
    static HRESULT ConvertToAssemblyMetaData(StackingAllocator* alloc,
                                             ASSEMBLYNAMEREF* pName,
                                             AssemblyMetaDataInternal* pMetaData);
};

class AssemblyNameNative
{
    //struct GetFileInformationArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,   filename);
    //};
    //struct NoArgs
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refThis);
    //};

    public:
    static FCDECL1(Object*, GetFileInformation, StringObject* filenameUNSAFE);
    static FCDECL1(Object*, ToString, Object* refThisUNSAFE);
    static FCDECL1(Object*, GetPublicKeyToken, Object* refThisUNSAFE);
    static FCDECL1(Object*, EscapeCodeBase, StringObject* filenameUNSAFE);
};

#endif  // _AssemblyName_H

