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
// Void
//	This class represents an empty variant
////////////////////////////////////////////////////////////////////////////////

namespace System {
    
	using System;
	using System.Runtime.Remoting;
	using System.Runtime.Serialization;
    /// <include file='doc\Empty.uex' path='docs/doc[@for="Empty"]/*' />
    [Serializable()] internal sealed class Empty : ISerializable
    {
        //Package private constructor
        internal Empty() {
        }
    
    	/// <include file='doc\Empty.uex' path='docs/doc[@for="Empty.Value"]/*' />
    	public static readonly Empty Value = new Empty();
    	
    	/// <include file='doc\Empty.uex' path='docs/doc[@for="Empty.ToString"]/*' />
    	public override String ToString()
    	{
    		return String.Empty;
    	}
    
        /// <include file='doc\Empty.uex' path='docs/doc[@for="Empty.GetObjectData"]/*' />
        public void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }
            UnitySerializationHolder.GetUnitySerializationInfo(info, UnitySerializationHolder.EmptyUnity, null, null);
        }
    }
}
