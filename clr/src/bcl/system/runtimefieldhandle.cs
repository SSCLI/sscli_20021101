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
namespace System {
    
    using System;
	using System.Reflection;
	using System.Runtime.Serialization;

    //  This value type is used for making classlib type safe. 
    // 
    //  SECURITY : m_ptr cannot be set to anything other than null by untrusted
    //  code.  
    // 
    //  This corresponds to EE FieldDesc.
    /// <include file='doc\RuntimeFieldHandle.uex' path='docs/doc[@for="RuntimeFieldHandle"]/*' />
	[Serializable()]
    public struct RuntimeFieldHandle : ISerializable
    {
        private IntPtr m_ptr;

        /// <include file='doc\RuntimeFieldHandle.uex' path='docs/doc[@for="RuntimeFieldHandle.Value"]/*' />
        public IntPtr Value {
            get {
                return m_ptr;
            }
        }

		// ISerializable interface
		private RuntimeFieldHandle(SerializationInfo info, StreamingContext context)
		{
			if (info==null) {
                throw new ArgumentNullException("info");
            }
			FieldInfo f = (RuntimeFieldInfo) info.GetValue("FieldObj", typeof(RuntimeFieldInfo));
			if (f==null) {
				BCLDebug.Trace("SER", "[RuntimeFieldHandle.ctor]Null Type returned from GetValue.");
				throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientState"));
			}

			m_ptr = f.FieldHandle.m_ptr;
			if (m_ptr == (IntPtr)0)
			    throw new SerializationException(Environment.GetResourceString("Serialization_InsufficientState"));
		}

		/// <include file='doc\RuntimeFieldHandle.uex' path='docs/doc[@for="RuntimeFieldHandle.GetObjectData"]/*' />
		public void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
			if (m_ptr == (IntPtr)0)
			    throw new SerializationException(Environment.GetResourceString("Serialization_InvalidFieldState"));
			RuntimeFieldInfo fldInfo = (RuntimeFieldInfo)RuntimeFieldInfo.GetFieldFromHandle(this); 
            BCLDebug.Assert(fldInfo!=null, "[RuntimeFieldHandle.GetObjectData]fldInfo!=null");
		    info.AddValue("FieldObj",fldInfo, typeof(RuntimeFieldInfo));
        }
	}
}
