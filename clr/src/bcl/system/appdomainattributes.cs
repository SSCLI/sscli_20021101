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
/*=============================================================================
**
** File: AppDomainAttributes
**
** Author: 
**
** Purpose: For AppDomain-related custom attributes.
**
** Date: July, 2000
**
=============================================================================*/

namespace System {

    /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimization"]/*' />
    [Serializable()]
    public enum LoaderOptimization 
    {
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimization.NotSpecified"]/*' />
        NotSpecified            = 0,
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimization.SingleDomain"]/*' />
        SingleDomain            = 1,
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimization.MultiDomain"]/*' />
        MultiDomain             = 2,
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimization.MultiDomainHost"]/*' />
        MultiDomainHost         = 3,
    }

    /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimizationAttribute"]/*' />
    [AttributeUsage (AttributeTargets.Method)]  
    public sealed class LoaderOptimizationAttribute : Attribute
    {
        internal byte _val;

        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimizationAttribute.LoaderOptimizationAttribute"]/*' />
        public LoaderOptimizationAttribute(byte value)
        {
            _val = value;
        }
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimizationAttribute.LoaderOptimizationAttribute1"]/*' />
        public LoaderOptimizationAttribute(LoaderOptimization value)
        {
            _val = (byte) value;
        }
        /// <include file='doc\AppDomainAttributes.uex' path='docs/doc[@for="LoaderOptimizationAttribute.Value"]/*' />
        public LoaderOptimization Value 
        {  get {return (LoaderOptimization) _val;} }
    }
}

