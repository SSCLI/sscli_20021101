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
#include "namespace.h"

// These system class names are not assembly qualified.

#define g_ActivationServicesClassName "System.Runtime.Remoting.Activation.ActivationServices"
#define g_AppDomainClassName "System.AppDomain"
#define g_ArrayClassName "System.Array"
#define g_AssemblyBuilderClassName "System.Reflection.Emit.AssemblyBuilder"
#define g_AssemblyClassName "System.Reflection.Assembly"
#define g_AssemblyHashAlgorithmClassName "System.Configuration.Assemblies.AssemblyHashAlgorithm"
#define g_AssemblyNameClassName "System.Reflection.AssemblyName"
#define g_AssemblyNameFlagsClassName "System.Reflection.AssemblyNameFlags"

#define g_ByteArrayClassName "System.Byte[]"
#define g_BinderClassName "System.Reflection.Binder"

#define g_CollectionsArrayList "System.Collections.ArrayList"
#define g_CollectionsEnumerableItfName "System.Collections.IEnumerable"
#define g_CollectionsEnumeratorClassName "System.Collections.IEnumerator"


#define g_ConsoleClassName "System.Console"
#define g_ContextBoundObjectClassName "System.ContextBoundObject"
#define g_ContextClassName "System.Runtime.Remoting.Contexts.Context"
#define g_ContextProxyClassName "System.__ContextProxy"
#define g_CultureInfoClassName "System.Globalization.CultureInfo"

#define g_DateClassName     "System.DateTime"
#define g_DelegateClassName "System.Delegate"
#define g_DecimalClassName "System.Decimal"

#define g_EnumClassName "System.Enum"
#define g_EnumeratorToEnumClassName "System.Runtime.InteropServices.CustomMarshalers.EnumeratorToEnumVariantMarshaler"
#define g_EvidenceClassName "System.Security.Policy.Evidence"
#define g_ExceptionClassName "System.Exception"
#define g_EnvironmentClassName "System.Environment"
#define g_ThreadStopExceptionClassName "System.Threading.ThreadStopException"
#define g_ThreadAbortExceptionClassName "System.Threading.ThreadAbortException"
#define g_ExecutionEngineExceptionClassName "System.ExecutionEngineException"

#define g_HashtableName "System.Collections.Hashtable"
#define g_IdentityClassName "System.Runtime.Remoting.Identity"
#define g_IdentityInfoClassName "System.Security.Policy.IdentityInfo"
#define g_InteropRegistrationService "System.Runtime.InteropServices.RegistrationServices"
#define g_InteropTCEAdapterGenEventSource "System.Runtime.InteropServices.TCEAdapterGen.EventSource"

#define g_JitHelpers "System.__jithelper"

#define g_LocalDataStoreClassName "System.LocalDataStore"

#define g_MarshalByRefObjectClassName "System.MarshalByRefObject"
#define g_MessageClassName "System.Runtime.Remoting.Messaging.Message"
#define g_MultiDelegateClassName "System.MulticastDelegate"

#define g_NamedAttributeClassName "System.NamedAttribute"

#define g_ObjectClassName "System.Object"
#define g_ObjectName "Object"
#define g_ObjectArrayClassName "System.Object[]"
#define g_OleAutBinderClassName "System.OleAutBinder"
#define g_OperatingSystemClassName "System.OperatingSystem"
#define g_SystemExceptionClassName "System.SystemException"
#define g_OutOfMemoryExceptionClassName "System.OutOfMemoryException"
#define g_AppDomainUnloadedExceptionClassName "System.AppDomainUnloadedException"

#define g_PermissionListSetClassName "System.Security.PermissionListSet"
#define g_PermissionSetClassName "System.Security.PermissionSet"
#define g_PermissionTokenFactoryName "System.Security.PermissionTokenFactory"
#define g_PlatformIDClassName "System.PlatformID"
#define g_PolicyExceptionClassName "System.Security.Policy.PolicyException"

#define g_RealProxyClassName "System.Runtime.Remoting.Proxies.RealProxy"
#define g_ReflectionClassName "System.RuntimeType"
#define g_ReflectionConstructorName "System.Reflection.RuntimeConstructorInfo"
#define g_ReflectionDynamicModuleName "System.Reflection.Emit.ModuleBuilder"
#define g_ReflectionEventInfoName "System.Reflection.RuntimeEventInfo"
#define g_ReflectionExpandoItfName "System.Runtime.InteropServices.Expando.IExpando"
#define g_ReflectionFieldName "System.Reflection.RuntimeFieldInfo"
#define g_ReflectionMemberInfoName "System.Reflection.MemberInfo"
#define g_ReflectionMethodBaseName "System.Reflection.MethodBase"
#define g_ReflectionMethodInfoName "System.Reflection.MethodInfo"
#define g_ReflectionFieldInfoName "System.Reflection.FieldInfo"
#define g_ReflectionPropertyInfoName "System.Reflection.PropertyInfo"
#define g_ReflectionConstructorInfoName "System.Reflection.ConstructorInfo"
#define g_ReflectionMethodName "System.Reflection.RuntimeMethodInfo"
#define g_ReflectionModuleName "System.Reflection.Module"
#define g_ReflectionNameValueName "System.Reflection.NameValueName"
#define g_ReflectionParamInfoName "System.Reflection.ParameterInfo"
#define g_ReflectionPermissionClassName "System.Security.Permissions.ReflectionPermission"
#define g_ReflectionPointerClassName "System.Reflection.Pointer"
#define g_ReflectionPropInfoName "System.Reflection.RuntimePropertyInfo"
#define g_ReflectionReflectItfName "System.Reflection.IReflect"
#define g_ReflectionCustAttrProviderItfName "System.Reflection.ICustomAttributeProvider"
#define g_ReflectionTargetInvocationExceptionClassName "System.Reflection.TargetInvocationException"
#define g_ReflectionTypeLoadExceptionClassName "System.Reflection.ReflectionTypeLoadException"
#define g_RemotingProxyClassName "System.Runtime.Remoting.Proxies.RemotingProxy"
#define g_RemotingServicesClassName "System.Runtime.Remoting.RemotingServices"
#define g_RuntimeArgumentHandleClassName "System.RuntimeArgumentHandle"
#define g_RuntimeArgumentHandleName      "RuntimeArgumentHandle"
#define g_RuntimeFieldHandleClassName    "System.RuntimeFieldHandle"
#define g_RuntimeFieldHandleName         "RuntimeFieldHandle"
#define g_RuntimeMethodHandleClassName   "System.RuntimeMethodHandle"
#define g_RuntimeMethodHandleName        "RuntimeMethodHandle"
#define g_RuntimeTypeHandleClassName     "System.RuntimeTypeHandle"
#define g_RuntimeTypeHandleName          "RuntimeTypeHandle"

#define g_SecurityCallContextClassName "System.Security.Principal.SecurityCallContext"
#define g_SecurityCallersClassName "System.Security.Principal.SecurityCallers"
#define g_SecurityIdentityClassName "System.Security.Principal.SecurityIdentity"
#define g_SecurityPermissionClassName "System.Security.Permissions.SecurityPermission"
#define g_SerializationExceptionName "System.Runtime.SerializationException"
#define g_ServerIdentityClassName "System.Runtime.Remoting.ServerIdentity"
#define g_StackOverflowExceptionClassName "System.StackOverflowException"
#define g_StaticContainerClassName "System.__StaticContainer"
#define g_StringBufferClassName "System.Text.StringBuilder"
#define g_StringBufferName "StringBuilder"
#define g_StringClassName "System.String"
#define g_StringName "String"
#define g_SharedStaticsClassName "System.SharedStatics"

#define g_ThreadClassName "System.Threading.Thread"
#define g_TransparentProxyClassName "System.Runtime.Remoting.Proxies.__TransparentProxy"
#define g_TypeClassName   "System.Type"
#define g_TypeLoadExceptionClassName "System.TypeLoadException"
#define g_TypedReferenceClassName "System.TypedReference"

#define g_UnloadWorkerClassName "System.UnloadWorker"

#define g_ValueTypeClassName "System.ValueType"
#define g_MissingClassName "System.Reflection.Missing"
#define g_ActivatorClassName "System.Activator"
#define g_VariantArrayClassName "System.Variant[]"
#define g_VariantClassName "System.Variant"
#define g_VersionClassName "System.Version"
#define g_VoidPtrClassName "System.Void*"
#define g_GuidClassName "System.Guid"
#define g_ENCHelperClassName "System.Diagnostics.EditAndContinueHelper"
#define g_IConfigHelper "System.IConfigHelper"
