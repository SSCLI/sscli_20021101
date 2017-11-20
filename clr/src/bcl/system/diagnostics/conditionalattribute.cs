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
using System;

namespace System.Diagnostics {
    /// <include file='doc\ConditionalAttribute.uex' path='docs/doc[@for="ConditionalAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Method, AllowMultiple=true), Serializable]
    public sealed class ConditionalAttribute : Attribute
    {
	/// <include file='doc\ConditionalAttribute.uex' path='docs/doc[@for="ConditionalAttribute.ConditionalAttribute"]/*' />
	public ConditionalAttribute(String conditionString)
	{
		m_conditionString = conditionString;
	}

	/// <include file='doc\ConditionalAttribute.uex' path='docs/doc[@for="ConditionalAttribute.ConditionString"]/*' />
	public String ConditionString {
		get {
			return m_conditionString;
		}
	}

	private String m_conditionString;
    }
}
