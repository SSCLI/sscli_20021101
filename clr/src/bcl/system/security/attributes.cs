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

[assembly:System.Security.AllowPartiallyTrustedCallersAttribute()]
namespace System.Security
{
    // DynamicSecurityMethodAttribute:
    //  Indicates that calling the target method requires space for a security
    //  object to be allocated on the callers stack. This attribute is only ever
    //  set on certain security methods defined within mscorlib.
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="DynamicSecurityMethodAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = true, Inherited = false )] 
    sealed internal class DynamicSecurityMethodAttribute : System.Attribute
    {
    }

    // SuppressUnmanagedCodeSecurityAttribute:
    //  Indicates that the target P/Invoke method(s) should skip the per-call
    //  security checked for unmanaged code permission.
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="SuppressUnmanagedCodeSecurityAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Class | AttributeTargets.Interface, AllowMultiple = true, Inherited = false )] 
    sealed public class SuppressUnmanagedCodeSecurityAttribute : System.Attribute
    {
    }

    // UnverifiableCodeAttribute:
    //  Indicates that the target module contains unverifiable code.
    /// <include file='doc\Attributes.uex' path='docs/doc[@for="UnverifiableCodeAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Module, AllowMultiple = true, Inherited = false )] 
    sealed public class UnverifiableCodeAttribute : System.Attribute
    {
    }

    // AllowPartiallyTrustedCallersAttribute:
    //  Indicates that the Assembly is secure and can be used by untrusted
    //  and semitrusted clients
    //  For v.1, this is valid only on Assemblies, but could be expanded to 
    //  include Module, Method, class
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple = false, Inherited = false )] 
    sealed public class AllowPartiallyTrustedCallersAttribute : System.Attribute
    {
        public AllowPartiallyTrustedCallersAttribute () { }
    }
}
