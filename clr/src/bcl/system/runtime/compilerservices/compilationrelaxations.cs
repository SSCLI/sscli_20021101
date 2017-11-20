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
////////////////////////////////////////////////////////////////////////////////

namespace System.Runtime.CompilerServices 
{

	using System;

	/// <include file='doc\CompilationRelaxations.uex' path='docs/doc[@for="CompilationRelaxations"]/*' />
	[Serializable]
	enum CompilationRelaxations 
	{ 
		ImpreciseException 		= 0x0001, 
		ImpreciseFloat 			= 0x0002, 
		ImpreciseAssign 		= 0x0004 
	};
		
	/// <include file='doc\CompilationRelaxations.uex' path='docs/doc[@for="CompilationRelaxationsAttribute"]/*' />
	[Serializable, AttributeUsage(AttributeTargets.Module)]  
	public class CompilationRelaxationsAttribute : Attribute 
	{
		private int m_relaxations;		// The relaxations.
		
		/// <include file='doc\CompilationRelaxations.uex' path='docs/doc[@for="CompilationRelaxationsAttribute.CompilationRelaxationsAttribute"]/*' />
		public CompilationRelaxationsAttribute (
			int relaxations) 
		{ 
			m_relaxations = relaxations; 
		}
		
		/// <include file='doc\CompilationRelaxations.uex' path='docs/doc[@for="CompilationRelaxationsAttribute.CompilationRelaxations"]/*' />
		public int CompilationRelaxations
		{ 
			get 
			{ 
				return m_relaxations; 
			} 
		}
	}
	
}
