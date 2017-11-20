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
#ifndef _CUSTOMATTRIBUTE_H_
#define _CUSTOMATTRIBUTE_H_

#include "fcall.h"

class COMCustomAttribute
{
public:

    // custom attributes utility functions
    static FCDECL2(INT32, GetMemberToken, BaseObjectWithCachedData *pMember, INT32 memberType);
    static FCDECL2(LPVOID, GetMemberModule, BaseObjectWithCachedData *pMember, INT32 memberType);
    static FCDECL1(INT32, GetAssemblyToken, AssemblyBaseObject *assembly);
    static FCDECL1(LPVOID, GetAssemblyModule, AssemblyBaseObject *assembly);
    static FCDECL1(INT32, GetModuleToken, ReflectModuleBaseObject *module);
    static FCDECL1(LPVOID, GetModuleModule, ReflectModuleBaseObject *module);
    static FCDECL1(INT32, GetMethodRetValueToken, BaseObjectWithCachedData *method);
    static FCDECL5(LPVOID, GetCustomAttributeList, DWORD token, LPVOID module, ReflectClassBaseObject* caType, CustomAttributeClass* caItem, INT32 level);
    static FCDECL3(LPVOID, CreateCAObject, CustomAttributeClass* refThis, INT32* propNum, Object** assembly);
    static FCDECL3(INT32, IsCADefined, ReflectClassBaseObject* caType, LPVOID module, DWORD token);
    static FCDECL5(LPVOID, GetDataForPropertyOrField, CustomAttributeClass* refThis, BYTE* isProperty, OBJECTREF* value, OBJECTREF* type, BYTE isLast);

    static INT32 __stdcall IsDefined(Module *pModule, 
                           mdToken token, 
                           TypeHandle attributeClass, 
                           BOOL checkAccess = FALSE);
};

#endif

