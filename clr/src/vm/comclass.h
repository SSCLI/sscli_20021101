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
// Date: March 27, 1998
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMCLASS_H_
#define _COMCLASS_H_

#include "excep.h"
#include "reflectwrap.h"
#include "comreflectioncommon.h"
#include "invokeutil.h"
#include "fcall.h"

// COMClass
// This is really to root of all reflection.  It represents
//  the Class object.  It also contains other shared resources
//  which are used by reflection and by clients of reflection.
class COMClass
{
public:

    enum NameSpecifier {
        TYPE_NAME = 0x1,
        TYPE_NAMESPACE = 0x2,
        TYPE_ASSEMBLY = 0x4
    } ;

private:

    //struct _GetEventArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //    DECLARE_ECALL_I4_ARG(DWORD, bindingAttr);
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, eventName);
    //};
    //struct _GetEventsArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //    DECLARE_ECALL_I4_ARG(DWORD, bindingAttr);
    //};
    //struct _GetPropertiesArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //    DECLARE_ECALL_I4_ARG(DWORD, bindingAttr); 
    //};
    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    //} _GETCLASSHANDLEARGS;
    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    //} _GETNAMEARGS;
    //struct _GetGUIDArgs{
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    //    DECLARE_ECALL_OBJECTREF_ARG(GUID *, retRef);
    //} ;
    //struct _GetInterfaceArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //    DECLARE_ECALL_I4_ARG(DWORD, bIgnoreCase); 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, interfaceName);
    //};
    //struct _GetInterfacesArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    //};
    //struct _GetMemberArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //    DECLARE_ECALL_I4_ARG(DWORD, bindingAttr); 
    //    DECLARE_ECALL_I4_ARG(DWORD, memberType); 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, memberName);
    //};
    //struct _GetMembersArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(DWORD, bindingAttr); 
    //};
    //struct _GetSerializableMembersArgs {
    //    DECLARE_ECALL_I4_ARG(DWORD, bFilterTransient); 
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refClass);
    //};
    //struct _GetMethodsArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(DWORD, bindingAttr); 
    //};
    //struct _GetConstructorsArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(DWORD, verifyAccess);
    //    DECLARE_ECALL_I4_ARG(DWORD, bindingAttr); 
    //};
    //struct _GetAttributeArgs        {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    //};
    //struct _GetAttributesArgs       {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    //};
    //struct _GetContextArgs          {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    //} ;
    //struct _SetContextArgs          
    //{
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //    DECLARE_ECALL_I4_ARG(LONG, flags); 
    //};
    //struct _IsArrayArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //};
    //struct _InternalGetArrayRankArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //};
    //struct _GetMemberMethodsArgs    {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //    DECLARE_ECALL_I4_ARG(DWORD, verifyAccess);
    //    DECLARE_ECALL_I4_ARG(INT32, argCnt); 
    //    DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, argTypes);
    //    DECLARE_ECALL_I4_ARG(INT32, callConv); 
    //    DECLARE_ECALL_I4_ARG(INT32, invokeAttr); 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    //};
    //struct _GetMemberFieldArgs  {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //    DECLARE_ECALL_I4_ARG(DWORD, verifyAccess);
    //    DECLARE_ECALL_I4_ARG(INT32, invokeAttr); 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    //};
    //struct _GetMemberPropertiesArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis); 
    //    DECLARE_ECALL_I4_ARG(DWORD, verifyAccess);
    //    DECLARE_ECALL_I4_ARG(INT32, argCnt); 
    //    DECLARE_ECALL_I4_ARG(INT32, invokeAttr); 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    //};
    //struct _GetUnitializedObjectArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, objType);
    //};
    //struct _SupportsInterfaceArgs          {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, refThis);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);
    //};
    
public:

    // Reflection uses a critical section to synchronized
    //  the creation of the base data structures.  This is
    //  initalized during startup.
    static CRITICAL_SECTION m_ReflectCrst;
    static CRITICAL_SECTION *m_pReflectCrst;
    static LONG m_ReflectCrstInitialized;

    static MethodTable* m_pMTRC_Class;      // reflection class method table
    static FieldDesc*   m_pDescrTypes;      // Array of Class stored inside a DescriptorInfo
    static FieldDesc*   m_pDescrMatchFlag;  // Partial Match Flag
    static FieldDesc*   m_pDescrRetType;    // Return type
    static FieldDesc*   m_pDescrRetModType; // Return Modifier type
    //  static FieldDesc*   m_pDescrCallConv;   // Calling Convention
    static FieldDesc*   m_pDescrAttributes; // Attributes

    // Return a boolean indicating if reflection has been initialized
    //   or not.
    static bool ReflectionInitialized()
    {
        return m_fAreReflectionStructsInitialized;
    }

    static FieldDesc* GetDescriptorObjectField(OBJECTREF desc);
    static FieldDesc* GetDescriptorReturnField(OBJECTREF desc);
    static FieldDesc* GetDescriptorReturnModifierField(OBJECTREF desc);
    //  static FieldDesc* GetDescriptorCallingConventionField(OBJECTREF desc);
    static FieldDesc* GetDescriptorAttributesField(OBJECTREF desc);


    // This method will make sure that reflection has been
    //  initialized.  Consumers of reflection services must call
    //  this before using reflection.
    static void EnsureReflectionInitialized()
    {
        THROWSCOMPLUSEXCEPTION();

        if(!m_fAreReflectionStructsInitialized)
        {
            MinimalReflectionInit();
        }

    }

    static MethodTable *GetRuntimeType();

    // This is called during termination...

    // IsPrimitive 
    // This method will return a boolean indicating if the type represents
    //  on of the primitive types.
    static FCDECL1(INT32, IsPrimitive, ReflectClassBaseObject* vRefThis);

    // GetAttributeFlags
    // This method will return the Attribute flags for a type.
    static FCDECL1(INT32, GetAttributeFlags, ReflectClassBaseObject* vRefThis);



    static FCDECL1(INT32, GetTypeDefToken, ReflectClassBaseObject* vRefThis);
    static FCDECL1(INT32, IsContextful, ReflectClassBaseObject* vRefThis);
    static FCDECL1(INT32, HasProxyAttribute, ReflectClassBaseObject* vRefThis);
    static FCDECL1(INT32, IsMarshalByRef, ReflectClassBaseObject* vRefThis);

    // GetTHFromObject
    // This method is a static method on type that returns
    //  the Handle (TypeHandle address) for an object.  It does
    //  not create the type handle object
    static FCDECL1(void*, GetTHFromObject, Object* vObject);

    // IsByRefImpl
    // This method will return a boolean indicating if the Type
    //  object is a ByRef
    static FCDECL1(INT32, IsByRefImpl, ReflectClassBaseObject* vObject);

    // IsPointerImpl
    // This method will return a boolean indicating if the Type
    //  object is a ByRef
    static FCDECL1(INT32, IsPointerImpl, ReflectClassBaseObject* vObject);

    // IsNestedTypeImpl
    // Return a boolean indicating if this is a nested type.
    static FCDECL1(INT32, IsNestedTypeImpl, ReflectClassBaseObject* vObject);

    // GetNestedDeclaringType
    // Return the declaring class for a nested type.
    static FCDECL1(Object*, GetNestedDeclaringType, ReflectClassBaseObject* vObject);

    // GetNestedTypes
    // This method will return an array of types which are the nested types
    //  defined by the type.  If no nested types are defined, a zero length
    //  array is returned.
    static FCDECL2(Object*, GetNestedTypes, ReflectClassBaseObject* vRefThis, INT32 invokeAttr);

    // GetNestedType
    // This method will search for a nested type based upon the name
    static FCDECL3(Object*, GetNestedType, ReflectClassBaseObject* vRefThis, StringObject* vStr, INT32 invokeAttr);

    static FCDECL2_INST_RET_VC(InterfaceMapData, data, GetInterfaceMap, ReflectClassBaseObject* refThis, ReflectClassBaseObject* type);

    // QuickLookupExistingArrayClassObj
    // Lookup an existing Type object that represents an Array.  Arrays are handled
    //  different from base objects.  Arrays are represented by ReflectArrayClass.  The
    //  Class object is stored there.  Will return NULL if the Type hasn't been
    //  created, meaning you should call ArrayTypeDesc::CreateClassObj
    // arrayType -- Pointer to the ArrayTypeDesc representing the Array
    static OBJECTREF QuickLookupExistingArrayClassObj(ArrayTypeDesc* arrayType);

    // GetMethod
    // This method returns an array of MethodInfo object representing all of the methods
    //  defined for this class.
    static FCDECL2(Object*, GetMethods, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr);

    // GetSuperclass
    // This method returns the Class Object representing the super class of this
    //  Class.  If there is not super class then we return null.
    static FCDECL1(LPVOID, GetSuperclass, ReflectClassBaseObject* refThis);

    // GetClassHandle
    // This method with return a unique ID meaningful to the EE and equivalent to
    // the result of the ldtoken instruction.
    static FCDECL1(void*, GetClassHandle, ReflectClassBaseObject* refThisUNSAFE);

    // GetClassFromHandle
    // This method with return a unique ID meaningful to the EE and equivalent to
    // the result of the ldtoken instruction.
    static FCDECL1(Object*, GetClassFromHandle, LPVOID handle);

    // RunClassConstructor triggers the class constructor
    static FCDECL1(void, RunClassConstructor, LPVOID handle);

    // GetName 
    // This method returns the unqualified name of the Class as a String.
    static FCDECL1(Object*, COMClass::GetName, ReflectClassBaseObject* refThis);

    // GetProperName 
    // This method returns the correctly qualified name of the Class as a String.
    static FCDECL1(Object*, COMClass::GetProperName, ReflectClassBaseObject* refThis);

    // GetFullName
    // This will return the fully qualified name of the class as a String.
    static FCDECL1(Object*, COMClass::GetFullName, ReflectClassBaseObject* refThis);

    // GetAssemblyQualifiedName
    // This will return the Assembly Qualified name of the class as a String.
    static FCDECL1(Object*, COMClass::GetAssemblyQualifiedName, ReflectClassBaseObject* refThis);

    // GetNameSpace
    // This will return the name space of a class as a String.
    static FCDECL1(LPVOID, GetNameSpace, ReflectClassBaseObject* refThis);

    // GetGUID
    // This method will return the version-independent GUID for the Class.  This is 
    //  a CLSID for a class and an IID for an Interface.
    static FCDECL1_INST_RET_VC(GUID, retRef, GetGUID, ReflectClassBaseObject* refThisUNSAFE);

    // GetClass
    // This is a static method defined on Class that will get a named class.
    //  The name of the class is passed in by string.  The class name may be
    //  either case sensitive or not.  This currently causes the class to be loaded
    //  because it goes through the class loader.
    static FCDECL1(Object*, GetClass1Arg,     StringObject* classNameUNSAFE);
    static FCDECL2(Object*, GetClass2Args,    StringObject* classNameUNSAFE, BYTE bThrowOnError);
    static FCDECL3(Object*, GetClass3Args,    StringObject* classNameUNSAFE, BYTE bThrowOnError, BYTE bIgnoreCase);
    static FCDECL4(Object*, GetClassInternal, StringObject* classNameUNSAFE, BYTE bThrowOnError, BYTE bIgnoreCase, BYTE bPublicOnly);
    static FCDECL5(Object*, GetClass,         StringObject* classNameUNSAFE, BYTE bThrowOnError, BYTE bIgnoreCase, StackCrawlMark* stackMark, BYTE* pbAssemblyIsLoading);
    static Object* GetClassInner(STRINGREF *refClassName, 
                                BOOL bThrowOnError, 
                                BOOL bIgnoreCase, 
                                StackCrawlMark *stackMark,
                                BYTE *pbAssemblyIsLoading,
                                BOOL bVerifyAccess,
                                BOOL bPublicOnly);


    // GetConstructors
    // This method will return an array of all the constructors for an object.
    static FCDECL3(Object*, GetConstructors, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr, BYTE verifyAccess);

    // GetField
    // This method will return the specified field
    static FCDECL3(Object*, GetField, ReflectClassBaseObject* refThisUNSAFE, StringObject* fieldNameUNSAFE, DWORD fBindAttr);

    // GetFields
    // This method will return a FieldInfo array of all of the
    //  fields defined for this Class
    static FCDECL3(Object*, GetFields, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr, BYTE bRequiresAccessCheck);

    // GetEvent
    // This method will return the specified event based upon
    //  the name
    static FCDECL3(Object*, GetEvent, ReflectClassBaseObject* refThisUNSAFE, StringObject* eventNameUNSAFE, DWORD bindingAttr);

    // GetEvents
    // This method will return an array of EventInfo for each of the events
    //  defined in the class
    static FCDECL2(Object*, GetEvents, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr);

    // GetProperties
    // This method will return an array of Properties for each of the
    //  properties defined in this class.  An empty array is return if
    //  no properties exist.
    static FCDECL2(Object*, GetProperties, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr);

    // GetInterface
    //  This method returns the interface based upon the name of the method.
        static FCDECL3(Object*, GetInterface, ReflectClassBaseObject* refThisUNSAFE, StringObject* interfaceNameUNSAFE, BYTE bIgnoreCase);

    // GetInterfaces
    // This routine returns a Class[] containing all of the interfaces implemented
    //  by this Class.  If the class implements no interfaces an array of length
    //  zero is returned.
    static FCDECL1(Object*, GetInterfaces, ReflectClassBaseObject* refThisUNSAFE);

    // GetMember
    // This method will return an array of Members which match the name
    //  passed in.  There may be 0 or more matching members.
    static FCDECL4(Object*, GetMember, ReflectClassBaseObject* refThisUNSAFE, StringObject* memberNameUNSAFE, DWORD memberType, DWORD bindingAttr);

    // GetMembers
    // This method returns an array of Members containing all of the members
    //  defined for the class.  Members include constructors, events, properties,
    //  methods and fields.
    static FCDECL2(Object*, GetMembers, ReflectClassBaseObject* refThisUNSAFE, DWORD bindingAttr);

    // GetSerializableMembers
    // Creates an array of all non-static fields and properties
    // on a class.  Properties are also excluded if they don't have get and set
    // methods. Transient fields and properties are excluded based on the value 
    // of args->bFilterTransient.  Essentially, transients are exluded for 
    // serialization but not for cloning.
    static FCDECL2(Object*, GetSerializableMembers, ReflectClassBaseObject* refClassUNSAFE, BYTE bFilterTransient);

    static FCDECL2(void, GetSerializationRegistryValues, BYTE *checkBit, BYTE *logNonSerializable);

    // IsArray
    // This method return true if the Class represents an array.
    static FCDECL1(INT32, IsArray, ReflectClassBaseObject* refThisUNSAFE);
    static FCDECL1(INT32, InvalidateCachedNestedType, ReflectClassBaseObject* refThisUNSAFE);

    // GetArrayElementType
    // This routine will return the base type of an array assuming
    //  the Class represents an array.  A NotSupported exception is
    //  thrown if the class is not an array.
    static FCDECL1(Object*, GetArrayElementType, ReflectClassBaseObject* refThisUNSAFE);

    // InternalGetArrayRank
    // This routine will return the rank of an array assuming the Class represents an array.  
    static FCDECL1(INT32, InternalGetArrayRank, ReflectClassBaseObject* refThisUNSAFE);

    // GetContextFlags
    static FCDECL1(LONG, GetContextFlags, ReflectClassBaseObject* refThisUNSAFE);

    // SetContextFlags
    static FCDECL2(void, SetContextFlags, ReflectClassBaseObject* refThisUNSAFE, LONG flags);

    // GetModule
    // This will return the module that the class is defined in.
    static FCDECL1(LPVOID, GetModule, ReflectClassBaseObject* refThis);


    // GetAssembly
    // This will return the assembly that the class is defined in.
    static FCDECL1(Object*, GetAssembly, ReflectClassBaseObject* refThisUNSAFE);

    // CreateClassObjFromModule
    // This method will create a new Module class given a Module.
    static HRESULT CreateClassObjFromModule(Module* pModule,REFLECTMODULEBASEREF* prefModule);

    // CreateClassObjFromDynamicModule
    // This method will create a new ModuleBuilder class given a Module.
    static HRESULT CreateClassObjFromDynamicModule(Module* pModule,REFLECTMODULEBASEREF* prefModule);

    static FCDECL5(Object*,GetMethodFromCache, ReflectClassBaseObject* refThis, StringObject* name, INT32 invokeAttr, INT32 argCnt, PtrArray* args);
    static FCDECL6(void,COMClass::AddMethodToCache, ReflectClassBaseObject* refThis, StringObject* name, INT32 invokeAttr, INT32 argCnt, PtrArray* args, Object* invokeMethod);
    
    // GetMemberMethods
    // This method will return all of the members methods which match the 
    //  specified attributes flag...
    static FCDECL7(Object*, GetMemberMethods, ReflectClassBaseObject* refThisUNSAFE, StringObject* nameUNSAFE, INT32 invokeAttr, INT32 callConv, PTRArray* argTypesUNSAFE, INT32 argCnt, BYTE verifyAccess);

    // GetMemberCons
    // This method returns all of the constructors that have a set number
    //  of methods.
    static FCDECL7(LPVOID, GetMemberCons, 
                            ReflectClassBaseObject* refThisUNSAFE, 
                            INT32       invokeAttr, 
                            INT32       callConv, 
                            PTRArray*   argTypesUNSAFE, 
                            INT32       argCnt, 
                            BYTE        verifyAccess, 
                            BYTE*       isDelegate);

    // GetMemberField
    // This method returns all of the fields which match the specified
    //  name.
    static FCDECL4(Object*, GetMemberField, ReflectClassBaseObject* refThisUNSAFE, StringObject* nameUNSAFE, INT32 invokeAttr, BYTE verifyAccess);

    // GetMemberProperties
    // This method returns all of the properties that have a set number
    //  of arguments.  The methods will be either get or set methods depending
    //  upon the invokeAttr flag.
    static FCDECL5(Object*, GetMemberProperties, ReflectClassBaseObject* refThisUNSAFE, StringObject* nameUNSAFE, INT32 invokeAttr, INT32 argCnt, BYTE verifyAccess);

    // GetMatchingProperties
    // This basically does a matching based upon the properties abstract 
    //  signature.
    static FCDECL6(Object*, GetMatchingProperties, ReflectClassBaseObject* refThisUNSAFE, 
                                                   StringObject* nameUNSAFE,
                                                   INT32 invokeAttr,
                                                   INT32 argCnt,
                                                   ReflectClassBaseObject* returnTypeUNSAFE,
                                                   BYTE verifyAccess);

    //GetUnitializedObject
    //This creates an instance of an object upon which no constructor has been run.
    static FCDECL1(Object*, GetUninitializedObject, ReflectClassBaseObject* objTypeUNSAFE);

    //CanCastTo
    //Check to see if we can cast from one runtime type to another.
    static FCDECL2(INT32, CanCastTo, ReflectClassBaseObject* vFrom, ReflectClassBaseObject* vTo);

    //CanCastTo
    //Check to see if we can cast from one runtime type to another.
    static FCDECL2(INT32, SupportsInterface, ReflectClassBaseObject* refThisUNSAFE, Object* objUNSAFE);

    // MatchField
    // This will check to see if there is a match on the field base upon name
    static LPVOID __stdcall MatchField(FieldDesc* pCurField,DWORD cFieldName,
        LPUTF8 szFieldName,ReflectClass* pRC,int bindingAttr);

    // Check if argument is a parent of "this"
    static FCDECL2(INT32, IsSubClassOf, ReflectClassBaseObject* vThis, ReflectClassBaseObject* vOther);

    static void GetNameInternal(ReflectClass *pRC, int nameType, CQuickBytes *qb);

private:
    // InitializeReflection
	static void InitializeReflectCrst();

    // This method will initalize reflection.
    static void MinimalReflectionInit();

    // This flag indicates if reflection has been initialized or not
    static bool m_fAreReflectionStructsInitialized;

    // _GetName
    // If the bFullName is true, teh fully qualified class name is return
    //  otherwise just the class name is returned.
    static StringObject* _GetName(ReflectClassBaseObject* refThis, int nameType);
    static LPCUTF8 _GetName(ReflectClass* pRC, BOOL fNameSpace, CQuickBytes *qb);


    // If you are tempted to use this, use TypeHandle::CreateClassObj instead!!
    // pVMCClass -- the EEClass we are creating the object for.
    // pRefClass -- A pointer to a pointer which we will return the newly created object
    static void COMClass::CreateClassObjFromEEClass(EEClass* pVMCClass, REFLECTCLASSBASEREF* pRefClass);

    // Internal helper function for GetProperName
    static INT32  __stdcall InternalIsPrimitive(REFLECTCLASSBASEREF args);

        // Needed so it can get at above method
    friend OBJECTREF EEClass::GetExposedClassObject();


    //
    // This is a temporary member until 3/15/2000.  Only if this member is set to 1
    // will we check the serialization bit to see if a class is serializable.
    //
    static DWORD m_checkSerializationBit;
    static void  GetSerializationBitValue();
};

#endif //_COMCLASS_H_
