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
/*============================================================
**
** File:    AssemblyHash
**
**                                     
**
** Purpose: 
**
** Date:    June 4, 1999
**
===========================================================*/
namespace System.Configuration.Assemblies {
    using System;
    /// <include file='doc\AssemblyHash.uex' path='docs/doc[@for="AssemblyHash"]/*' />
    [Serializable()]
    public struct AssemblyHash : ICloneable
    {
        private AssemblyHashAlgorithm _Algorithm;
        private byte[] _Value;
        
        /// <include file='doc\AssemblyHash.uex' path='docs/doc[@for="AssemblyHash.Empty"]/*' />
        public static readonly AssemblyHash Empty = new AssemblyHash(AssemblyHashAlgorithm.None, null);
    
        /// <include file='doc\AssemblyHash.uex' path='docs/doc[@for="AssemblyHash.AssemblyHash"]/*' />
        public AssemblyHash(byte[] value) {
            _Algorithm = AssemblyHashAlgorithm.SHA1;
            _Value = null;
    
            if (value != null) {
                int length = value.Length;
                _Value = new byte[length];
                Array.Copy(value, _Value, length);
            }
        }
    
        /// <include file='doc\AssemblyHash.uex' path='docs/doc[@for="AssemblyHash.AssemblyHash1"]/*' />
        public AssemblyHash(AssemblyHashAlgorithm algorithm, byte[] value) {
            _Algorithm = algorithm;
            _Value = null;
    
            if (value != null) {
                int length = value.Length;
                _Value = new byte[length];
                Array.Copy(value, _Value, length);
            }
        }
    
        // Hash is made up of a byte array and a value from a class of supported 
        // algorithm types.
        /// <include file='doc\AssemblyHash.uex' path='docs/doc[@for="AssemblyHash.Algorithm"]/*' />
        public AssemblyHashAlgorithm Algorithm {
            get { return _Algorithm; }
            set { _Algorithm = value; }
        }

        /// <include file='doc\AssemblyHash.uex' path='docs/doc[@for="AssemblyHash.GetValue"]/*' />
        public byte[] GetValue() {
            return _Value;
        }

        /// <include file='doc\AssemblyHash.uex' path='docs/doc[@for="AssemblyHash.SetValue"]/*' />
        public void SetValue(byte[] value) {
            _Value = value;
        }
    
        /// <include file='doc\AssemblyHash.uex' path='docs/doc[@for="AssemblyHash.Clone"]/*' />
        public Object Clone() {
            return new AssemblyHash(_Algorithm, _Value);
        }
    }

}
