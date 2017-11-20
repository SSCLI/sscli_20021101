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
//
// ParameterModifier is a struct that is used to attach modifier to parameters
//	so that binding can work against signatures where the types have been modified.
//	Modifications include, ByRef, etc.
//
// Date: Aug 99
//
namespace System.Reflection {
    
	using System;
    /// <include file='doc\ParameterModifier.uex' path='docs/doc[@for="ParameterModifier"]/*' />
    public struct ParameterModifier {
		internal bool[] _byRef;
		/// <include file='doc\ParameterModifier.uex' path='docs/doc[@for="ParameterModifier.ParameterModifier"]/*' />
		public ParameterModifier(int paramaterCount) {
			if (paramaterCount <= 0)
				throw new ArgumentException(Environment.GetResourceString("Arg_ParmArraySize"));

			_byRef = new bool[paramaterCount];
		}

        /// <include file='doc\ParameterModifier.uex' path='docs/doc[@for="ParameterModifier.this"]/*' />
        public bool this[int index] {
            get {return _byRef[index]; }
            set {_byRef[index] = value;}
        }

    }
}
