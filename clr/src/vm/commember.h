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
// COMMember.h
//  This module defines the native reflection routines used by Method, Field and Constructor
//
// Date: March/April 1998
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMMEMBER_H_
#define _COMMEMBER_H_

#include "comclass.h"
#include "invokeutil.h"
#include "reflectutil.h"
#include "comstring.h"
#include "comvarargs.h"
#include "fcall.h"

// NOTE: The following constants are defined in BindingFlags.cs
#define BINDER_IgnoreCase           0x01 
#define BINDER_DeclaredOnly         0x02
#define BINDER_Instance             0x04
#define BINDER_Static               0x08
#define BINDER_Public               0x10
#define BINDER_NonPublic            0x20
#define BINDER_FlattenHierarchy     0x40

#define BINDER_InvokeMethod         0x00100
#define BINDER_CreateInstance       0x00200
#define BINDER_GetField             0x00400
#define BINDER_SetField             0x00800
#define BINDER_GetProperty          0x01000
#define BINDER_SetProperty          0x02000
#define BINDER_PutDispProperty      0x04000
#define BINDER_PutRefDispProperty   0x08000

#define BINDER_ExactBinding         0x010000
#define BINDER_SuppressChangeType   0x020000
#define BINDER_OptionalParamBinding 0x040000

#define BINDER_IgnoreReturn         0x1000000

#define BINDER_DefaultLookup        (BINDER_Instance | BINDER_Static | BINDER_Public)
#define BINDER_AllLookup            (BINDER_Instance | BINDER_Static | BINDER_Public | BINDER_Instance)

// The following values define the MemberTypes constants.  These are defined in 
//  Reflection/MemberTypes.cs
#define MEMTYPE_Constructor         0x01    
#define MEMTYPE_Event               0x02
#define MEMTYPE_Field               0x04
#define MEMTYPE_Method              0x08
#define MEMTYPE_Property            0x10
#define MEMTYPE_TypeInfo            0x20
#define MEMTYPE_Custom              0x40
#define MEMTYPE_NestedType          0x80

// The following value class represents a ParameterModifier.  This is defined in the
//  reflection package

// These are the constants
#define PM_None             0x00
#define PM_ByRef            0x01
#define PM_Volatile         0x02
#define PM_CustomRequired   0x04


class COMMember
{
public:

private:
    //struct _GetNameArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    //struct _GetTokenNameArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //};
    //struct _GetDeclaringClassArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    //struct _GetEventDeclaringClassArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //};
    //typedef struct {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //} _GETSIGNATUREARGS;
    //struct _GetAttributeFlagsArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    //struct _GetMethodImplFlagsArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    //struct _GetCallingConventionArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    //struct _GetTokenAttributeFlagsArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //};
    //struct _GetReturnTypeArgs       {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    //struct _GetReflectedClassArgs   {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(BOOL, returnGlobalClass);
    //};
    //struct _GetAttributeArgs        {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis); 
    //    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, name);
    //} ;
    //struct _GetAttributesArgs       {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    //struct _PropertyEqualsArgs        {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);
    //};
    //struct _GetAddMethodArgs        {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    //};
    //struct _GetRemoveMethodArgs     {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    //};
    //struct _GetRaiseMethodArgs      {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    //};
    //struct _GetAccessorsArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    //};
    //struct _GetInternalGetterArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(DWORD, bVerifyAccess); 
    //    DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    //};
    //struct _GetInternalSetterArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(DWORD, bVerifyAccess); 
    //    DECLARE_ECALL_I4_ARG(DWORD, bNonPublic); 
    //};
    //struct _GetPropEventArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //};
    //struct _GetPropBoolArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //};
    //struct _GetFieldTypeArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTTOKENBASEREF, refThis);
    //};
    //struct _GetBaseDefinitionArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    //struct _GetParentDefinitionArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    //struct _IsOverloadedArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    //struct _HasLinktimeDemandArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //};
    //struct _InternalGetCurrentMethodArgs {
    //    DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
    //};
    //struct _ObjectToTypedReferenceArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(TypeHandle, th);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);
    //    DECLARE_ECALL_PTR_ARG(TypedByRef, typedReference); 
    //};
    //struct _TypedReferenceToObjectArgs {
    //    DECLARE_ECALL_PTR_ARG(TypedByRef, typedReference); 
    //};   

    // This method is called by the GetMethod function and will crawl backward
    //  up the stack for integer methods.
    static StackWalkAction SkipMethods(CrawlFrame*, VOID*);

    // InvokeArrayCons
    // This method will return a new Array Object from the constructor.
    static LPVOID InvokeArrayCons(ReflectArrayClass* pRC,MethodDesc* pMeth,
        PTRARRAYREF* objs,int argCnt);

    // The following structure is provided to the stack skip function.  It will
    //  skip cSkip methods and the return the next MethodDesc*.
    struct SkipStruct {
        StackCrawlMark* pStackMark;
        MethodDesc*     pMeth;
    };

    static EEClass* _DelegateClass;
    static EEClass* _MulticastDelegateClass;

    // This method will verify the type relationship between the target and
    //  the eeClass of the method we are trying to invoke.  It checks that for 
    //  non static method, target is provided.  It also verifies that the target is
    //  a subclass or implements the interface that this MethodInfo represents.  
    //  We may update the MethodDesc in the case were we need to lookup the real
    //  method implemented on the object for an interface.
    static void VerifyType(OBJECTREF target,EEClass* eeClass, EEClass* trueClass,int thisPtr,MethodDesc** ppMeth, TypeHandle typeTH, TypeHandle targetTH);

    static void TryCallMethod(MethodDesc *pMeth, ARG_SLOT* args, MetaSig* pSig);


public:
    // These are the base method tables for the basic Reflection
    //  types
    static MethodTable* m_pMTIMember;
    static MethodTable* m_pMTFieldInfo;
    static MethodTable* m_pMTPropertyInfo;
    static MethodTable* m_pMTEventInfo;
    static MethodTable* m_pMTType;
    static MethodTable* m_pMTMethodInfo;
    static MethodTable* m_pMTConstructorInfo;
    static MethodTable* m_pMTMethodBase;
    static MethodTable* m_pMTParameter;

    static MethodTable *GetParameterInfo()
    {
        if (m_pMTParameter)
            return m_pMTParameter;
    
        m_pMTParameter = g_Mscorlib.FetchClass(CLASS__PARAMETER);
        return m_pMTParameter;
    }

    static MethodTable *GetMemberInfo()
    {
        if (m_pMTIMember)
            return m_pMTIMember;
    
        m_pMTIMember = g_Mscorlib.FetchClass(CLASS__MEMBER);
        return m_pMTIMember;
    }

    // This is the Global InvokeUtil class
    static InvokeUtil*  g_pInvokeUtil;

    // DBCanConvertPrimitive
    // This method returns a boolean indicting if the source primitive can be
    //  converted to the target
    static FCDECL2(INT32, DBCanConvertPrimitive, ReflectClassBaseObject* vSource, ReflectClassBaseObject* vTarget);

    // DBCanConvertObjectPrimitive
    // This method returns a boolean indicating if the source object can be 
    //  converted to the target primitive.
    static FCDECL2(INT32, DBCanConvertObjectPrimitive, Object* vSourceObj, ReflectClassBaseObject* vTarget);

    // DirectFieldGet
    // This is a field set method that has a TypeReference that points to the data
    //struct _DirectFieldGetArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(BOOL, requiresAccessCheck);
    //    DECLARE_ECALL_OBJECTREF_ARG(TypedByRef, target);
    //};
    static FCDECL3(Object*, DirectFieldGet, ReflectBaseObject* refThisUNSAFE, TypedByRef target, BYTE requiresAccessCheck);

    //
    // Helper to split out EH from FCALL
    //
    static void VerifyNonFinalField(ReflectField* pRF);

    // DirectFieldSet
    // This is a field set method that has a TypeReference that points to the data
    //struct _DirectFieldSetArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(REFLECTBASEREF, refThis);
    //    DECLARE_ECALL_I4_ARG(BOOL, requiresAccessCheck);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, value);
    //    DECLARE_ECALL_OBJECTREF_ARG(TypedByRef, target);
    //};
    static FCDECL4_IVII(void, DirectFieldSet, ReflectBaseObject* refThisUNSAFE, TypedByRef target, Object* valueUNSAFE, BYTE requiresAccessCheck);

    // MakeTypedReference
    // This method will take an object, an array of FieldInfo's and create
    //  at TypedReference for it (Assuming its valid).  This will throw a
    //  MissingMemberException
    //struct _MakeTypedReferenceArgs {
    //    DECLARE_ECALL_OBJECTREF_ARG(PTRARRAYREF, flds);
    //    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, target);
    //    DECLARE_ECALL_PTR_ARG(TypedByRef*, value); 
    //};
    static FCDECL3(void, MakeTypedReference, TypedByRef* value, Object* targetUNSAFE, PTRArray* fldsUNSAFE);

    // DirectObjectFieldSet
    // When the TypedReference points to a object we call this method to
    //  set the field value
    static void DirectObjectFieldSet(FieldDesc* pField, TypedByRef target, OBJECTREF* pvalue, BOOL requiresAccessCheck);

    // DirectObjectFieldGet
    // When the TypedReference points to a object we call this method to
    //  get the field value
    static OBJECTREF DirectObjectFieldGet(FieldDesc* pField, TypedByRef target);

    // GetFieldInfoToString
    // This method will return the string representation of the information in FieldInfo.
    static FCDECL1(Object*, GetFieldInfoToString, ReflectBaseObject* refThisUNSAFE);

    // GetMethodInfoToString
    // This method will return the string representation of the information in MethodInfo.
    static FCDECL1(Object*, GetMethodInfoToString, ReflectBaseObject* refThisUNSAFE);

    // GetPropInfoToString
    // This method will return the string representation of the information in PropInfo.
    static FCDECL1(Object*, GetPropInfoToString, ReflectTokenBaseObject* refThisUNSAFE);

    // GetEventInfoToString
    // This method will return the string representation of the information in EventInfo.
    static FCDECL1(Object*, GetEventInfoToString, ReflectBaseObject* refThisUNSAFE);


    // GetMethodName
    // This method will return the name of a Method
    static FCDECL1(Object*, GetMethodName, ReflectBaseObject* refThisUNSAFE);

    // GetEventName
    // This method will return the name of a Event
    static FCDECL1(Object*, GetEventName, ReflectTokenBaseObject* refThisUNSAFE);

    // GetPropName
    // This method will return the name of a Property
    static FCDECL1(Object*, GetPropName, ReflectTokenBaseObject* refThisUNSAFE);

    // GetPropType
    // This method will return the Type of a Property
    static FCDECL1(Object*, GetPropType, ReflectTokenBaseObject* refThisUNSAFE);

    // GetTypeHandleImpl
    // This method will return the RuntimeMethodHandle for a MethodInfo object. 
    static FCDECL1(void*, GetMethodHandleImpl, ReflectBaseObject* pRefThis);

    // GetMethodFromHandleImp
    // This is a static method which will return a MethodInfo object based
    //  upon the passed in Handle.
    static FCDECL1(Object*, GetMethodFromHandleImp, LPVOID handle);

    static FCDECL1(size_t, GetFunctionPointer, size_t pMethodDesc);
    // GetFieldHandleImpl
    // This method will return the RuntimeFieldHandle for a FieldInfo object
    static FCDECL1(void*, GetFieldHandleImpl, ReflectBaseObject* pRefThis);

    // GetFieldFromHandleImp
    // This is a static method which will return a FieldInfo object based
    //  upon the passed in Handle.
    static FCDECL1(Object*, GetFieldFromHandleImp, LPVOID handle);

    // InternalGetEnumUnderlyingType
    // This method returns the defined values & names for an enum class.
    static FCDECL1(Object *, InternalGetEnumUnderlyingType, ReflectClassBaseObject *target); 

    // InternalGetEnumValue
    // This method returns the defined values & names for an enum class.
    static FCDECL1(Object *, InternalGetEnumValue, Object *pRefThis); 

    // InternalGetEnumValues
    // This method returns the defined values & names for an enum class.
    static FCDECL3(void, InternalGetEnumValues, ReflectClassBaseObject *target, 
                   Object **pReturnValues, Object **pReturnNames);

    // InternalBoxEnumI8
    // This method will create an Enum object and place the value inside
    //  it and return it.  The Type has been validated before calling.
    static FCDECL2(Object*, InternalBoxEnumI8, ReflectClassBaseObject* pEnumType, INT64 value);

    // InternalBoxEnumU8
    // This method will create an Enum object and place the value inside
    //  it and return it.  The Type has been validated before calling.
    static FCDECL2(Object*, InternalBoxEnumU8, ReflectClassBaseObject* pEnumType, UINT64 value);

    /*=============================================================================
    ** GetReturnType
    **
    ** Get the class representing the return type for a method
    **
    ** args->refThis: this Method object reference
    **/
    /*OBJECTREF*/
    static FCDECL1(Object*, GetReturnType, ReflectBaseObject* refThisUNSAFE);

    /*=============================================================================
    ** GetParameterTypes
    **
    ** This routine returns an array of Parameters
    **
    ** args->refThis: this Field object reference
    **/
    /*PTRARRAYREF String*/ 
    static FCDECL1(Object*, GetParameterTypes, ReflectBaseObject* refThisUNSAFE);

    /*=============================================================================
    ** GetFieldName
    **
    ** The name of this field is returned
    **
    ** args->refThis: this Field object reference
    **/
    /*STRINGREF String*/ 
	static FCDECL1(Object*, GetFieldName, ReflectBaseObject* refThisUNSAFE);

    /*=============================================================================
    ** GetDeclaringClass
    **
    ** Returns the class which declared this method or constructor. This may be a
    ** parent of the Class that getMethod() was called on.  Methods are always
    ** associated with a Class.  You cannot invoke a method from one class on
    ** another class even if they have the same signatures.  It is possible to do
    ** this with Delegates.
    **
    ** args->refThis: this object reference
    **/
    static FCDECL1(LPVOID, GetDeclaringClass, ReflectBaseObject* refThis);

    // This is the field based version
    static FCDECL1(Object*, GetFieldDeclaringClass, ReflectBaseObject* refThisUNSAFE);

    // This is the Property based version
    static FCDECL1(Object*, GetPropDeclaringClass, ReflectBaseObject* refThisUNSAFE);

    // This is the event based version
    static FCDECL1(Object*, GetEventDeclaringClass, ReflectTokenBaseObject* refThisUNSAFE);

    /*=============================================================================
    ** GetReflectedClass
    **
    ** This method returns the class from which this method was reflected.
    **
    ** args->refThis: this object reference
    **/
    /*Class*/ 
    static FCDECL2(Object*, GetReflectedClass, ReflectBaseObject* refThisUNSAFE, BYTE returnGlobalClass);

    /*=============================================================================
    ** GetFieldSignature
    **
    ** Returns the signature of the field.
    **
    ** args->refThis: this object reference
    **/
    /*STRINGREF*/ static FCDECL1(Object*, GetFieldSignature, ReflectBaseObject* refThisUNSAFE);

    // GetAttributeFlags
    // This method will return the attribute flag for a Member.  The 
    //  attribute flag is defined in the meta data.
    static FCDECL1(INT32, GetAttributeFlags, ReflectBaseObject* refThisUNSAFE);

    // GetCallingConvention
    // Return the calling convention
    static FCDECL1(INT32, GetCallingConvention, ReflectBaseObject* refThisUNSAFE);

    // GetMethodImplFlags
    // Return the method impl flags
    static FCDECL1(INT32, GetMethodImplFlags, ReflectBaseObject* refThisUNSAFE);

    // GetEventAttributeFlags
    // This method will return the attribute flag for an Event. 
    //  The attribute flag is defined in the meta data.
    static FCDECL1(INT32, GetEventAttributeFlags, ReflectTokenBaseObject* refThisUNSAFE);

    // GetPropAttributeFlags
    // This method will return the attribute flag for an Event. 
    //  The attribute flag is defined in the meta data.
    static FCDECL1(INT32, GetPropAttributeFlags, ReflectTokenBaseObject* refThisUNSAFE);

    /*=============================================================================
    ** InvokeBinderMethod
    **
    ** This routine will invoke the method on an object.  It will verify that
    **  the arguments passed are correct
    **
    ** args->refThis: this object reference
    **/

    static FCDECL9(Object*, InvokeMethod, ReflectBaseObject* refThis,
                                                                        Object* target,
                                                                        INT32 attrs,
                                                                        Object* binder,
                                                                        PTRArray* objs,
                                                                        Object* locale,
                                                                        BYTE isBinderDefault,
                                                                        AssemblyBaseObject* caller,
                                                                        BYTE verifyAccess
                                                                        );
    
    static OBJECTREF InvokeMethod_Internal(
                                                                        REFLECTBASEREF refThis,
                                                                        OBJECTREF target,
                                                                        INT32 attrs,
                                                                        OBJECTREF binder,
                                                                        PTRARRAYREF objs,
                                                                        OBJECTREF locale,
                                                                        BOOL isBinderDefault,
                                                                        ASSEMBLYREF caller,
                                                                        BOOL verifyAccess
                                                                        );
    

    // InvokeCons
    // This routine will invoke the constructor for a class.  It will verify that
    //  the arguments passed are correct

    struct _InvokeConsArgs          {
        REFLECTBASEREF      refThis; 
        OBJECTREF           locale;
        PTRARRAYREF         objs;
        OBJECTREF           binder;
    };

    static Object* InvokeConsInner(_InvokeConsArgs* pArgs);

    static FCDECL6(Object*, InvokeCons, ReflectBaseObject* refThisUNSAFE, INT32 attrs, Object* binderUNSAFE, PTRArray* objsUNSAFE, Object* localeUNSAFE, BYTE isBinderDefault);


    // SerializationInvoke
    // This routine will call the constructor for a class that implements ISerializable.  It knows
    // the arguments that it's receiving so does less error checking.

    struct StreamingContextData {
        Object * additionalContext;  // additionalContex was changed from OBJECTREF to Object to avoid having a 
        INT32     contextStates;     // constructor in this struct. GCC doesn't allow structs with constructors to be  
    };                               // passed by value
    struct _SerializationInvokeArgs {
        REFLECTBASEREF          refThis;
        // StreamingContextData    context;
        OBJECTREF               additionalContext;
        INT32                   contextStates;
        OBJECTREF               serializationInfo;
        OBJECTREF               target;
    };

    static void SerializationInvokeInner(_SerializationInvokeArgs *args);

    static FCDECL4(void, SerializationInvoke, ReflectBaseObject* refThisUNSAFE, Object * target, Object * SerializationInfoObj, StreamingContextData StreamingContextObj);

    // CreateInstance
    // This routine will create an instance of a Class by invoking the null constructor
    //  if a null constructor is present.  
    // Return LPVOID  (System.Object.)
    // Args: _CreateInstanceArgs
    static FCDECL2(Object*, CreateInstance, ReflectClassBaseObject* refThis, BYTE publicOnly);

    // FieldGet
    // This method will get the value associated with an object

    static FCDECL3(Object*, FieldGet, ReflectBaseObject* refThisUNSAFE, Object* targetUNSAFE, BYTE requiresAccessCheck);

    // FieldSet
    // This method will set the field of an associated object
    static FCDECL8(void, FieldSet, 
                            ReflectBaseObject*  refThisUNSAFE, 
                            Object*             targetUNSAFE, 
                            Object*             valueUNSAFE, 
                            INT32               attrs, 
                            Object*             binderUNSAFE, 
                            Object*             localeUNSAFE, 
                            BYTE                requiresAccessCheck, 
                            BYTE                isBinderDefault);

    // ObjectToTypedReference
    // This is an internal helper function to TypedReference class. 
    // We already have verified that the types are compatable. Assing the object in args
    // to the typed reference
    static FCDECL3_VII(void, ObjectToTypedReference, TypedByRef typedReference, Object* objUNSAFE, LPVOID handle);

    // This is an internal helper function to TypedReference class. 
    // It extracts the object from the typed reference.
    static FCDECL1(Object*, TypedReferenceToObject, TypedByRef typedReference);

    // GetExceptions
    // This method will return all of the exceptions which have been declared
    //  for a method or constructor
    //static LPVOID __stdcall GetExceptions(_GetExceptionsArgs* args);

    // Equals
    // This method will verify that two methods are equal....
    static FCDECL2(INT32, Equals, ReflectBaseObject* refThis, Object* obj);

    // Equals
    // This method will verify that two methods are equal....
    //static INT32 __stdcall TokenEquals(_TokenEqualsArgs* args);
    // Equals
    // This method will verify that two methods are equal....
    static FCDECL2(INT32, PropertyEquals, ReflectTokenBaseObject* refThisUNSAFE, Object* objUNSAFE);

    // CreateReflectionArgs
    // This method creates the global g_pInvokeUtil pointer.
    static void CreateReflectionArgs() {
        if (!g_pInvokeUtil)
            g_pInvokeUtil = new InvokeUtil(); 
    }

    // GetAddMethod
    // This will return the Add method for an Event
    static FCDECL2(Object*, GetAddMethod, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic);

    // GetRemoveMethod
    // This method return the unsync method on an event
    static FCDECL2(Object*, GetRemoveMethod, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic);

    // GetRemoveMethod
    // This method return the unsync method on an event
    static FCDECL2(Object*, GetRaiseMethod, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic);

    // GetGetAccessors
    // This method will return an array of the Get Accessors.  If there
    //  are no GetAccessors then we will return an empty array
    static FCDECL2(Object*, GetAccessors, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic);

    // InternalSetter
    // This method will return the Set Accessor method on a property
    static FCDECL3(Object*, InternalSetter, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic, BYTE bVerifyAccess);

    // InternalGetter
    // This method will return the Get Accessor method on a property
    static FCDECL3(Object*, InternalGetter, ReflectTokenBaseObject* refThisUNSAFE, BYTE bNonPublic, BYTE bVerifyAccess);

    // CanRead
    // This method will return a boolean value indicating if the Property is
    //  a can be read from.
    static FCDECL1(INT32, CanRead, ReflectTokenBaseObject* refThisUNSAFE);

    // CanWrite
    // This method will return a boolean value indicating if the Property is
    //  a can be written to.
    static FCDECL1(INT32, CanWrite, ReflectTokenBaseObject* refThisUNSAFE);

    // GetFieldType
    // This method will return a Class object which represents the
    //  type of the field.
    static FCDECL1(Object*, GetFieldType, ReflectTokenBaseObject* refThisUNSAFE);

    // GetBaseDefinition
    // Return the MethodInfo that represents the first definition of this
    //  virtual method.
    static FCDECL1(Object*, GetBaseDefinition, ReflectBaseObject* refThisUNSAFE);

    // GetParentDefinition
    // Return the MethodInfo that represents the previous definition of this
    //  virtual method in the hierarchy, or NULL if none.
    static FCDECL1(Object*, GetParentDefinition, ReflectBaseObject* refThisUNSAFE);

    // InternalGetCurrentMethod
    // Return the MethodInfo that represents the current method (two above this one)
    static FCDECL1(Object*, InternalGetCurrentMethod, StackCrawlMark* stackMark);

    // PublicProperty
    // This method will check to see if the passed property has any
    //  public accessors (Making it publically exposed.)
    static bool PublicProperty(ReflectProperty* pProp);

    // StaticProperty
    // This method will check to see if the passed property has any
    //  static methods as accessors.
    static bool StaticProperty(ReflectProperty* pProp);

    // PublicEvent
    // This method looks at each of the event accessors, if any
    //  are public then the event is considered public
    static bool PublicEvent(ReflectEvent* pEvent);

    // StaticEvent
    // This method will check to see if any of the accessor are static
    //  which will make it a static event
    static bool StaticEvent(ReflectEvent* pEvent);

    // FindAccessor
    // This method will find the specified property accessor
    //  pEEC - the class representing the object with the property
    //  tk - the token of the property
    //  type - the type of accessor method 
    static mdMethodDef FindAccessor(IMDInternalImport *pInternalImport,mdToken tk,
        CorMethodSemanticsAttr type,bool bNonPublic);

    static void CanAccess(MethodDesc* pMeth, RefSecContext *pSCtx, 
            bool checkSkipVer = false, bool verifyAccess = true, 
            bool isThisImposedSecurity = false, 
            bool knowForSureImposedSecurityState = false);

    static void CanAccessField(ReflectField* pRF, RefSecContext *pCtx);
    static EEClass *GetUnderlyingClass(TypeHandle *pTH);

    // DBCanConvertPrimitive

    // Terminate
    // This method will cleanup reflection in general.

    // Init cache related to field
    static VOID InitReflectField(FieldDesc *pField, ReflectField *pRF);

    static FCDECL1(INT32, IsOverloaded, ReflectBaseObject* refThisUNSAFE);

    static FCDECL1(INT32, HasLinktimeDemand, ReflectBaseObject* refThisUNSAFE);
};

#endif // _COMMETHODBASE_H_
