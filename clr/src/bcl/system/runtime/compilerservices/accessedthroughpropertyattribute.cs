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
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
namespace System.Runtime.CompilerServices 
{
	using System;

	/// <include file='doc\AccessedThroughPropertyAttribute.uex' path='docs/doc[@for="AccessedThroughPropertyAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Field)]
	public sealed class AccessedThroughPropertyAttribute : Attribute
	{
		private readonly string propertyName;

		/// <include file='doc\AccessedThroughPropertyAttribute.uex' path='docs/doc[@for="AccessedThroughPropertyAttribute.AccessedThroughPropertyAttribute"]/*' />
		public AccessedThroughPropertyAttribute(string propertyName)
		{
			this.propertyName = propertyName;
		}

		/// <include file='doc\AccessedThroughPropertyAttribute.uex' path='docs/doc[@for="AccessedThroughPropertyAttribute.PropertyName"]/*' />
		public string PropertyName 
		{
			get 
			{
				return propertyName;
			}
		}
	}
}

