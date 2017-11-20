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
namespace System.Runtime.InteropServices {

	using System;
	using System.Runtime.CompilerServices;

    /// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset"]/*' />
    public struct ArrayWithOffset
    {
        //private ArrayWithOffset()
        //{
        //    throw new Exception();
        //}
    
        /// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset.ArrayWithOffset"]/*' />
        public ArrayWithOffset(Object array, int offset)
        {
            m_array  = array;
            m_offset = offset;
            m_count  = 0;
            m_count  = CalculateCount();
        }
    
        /// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset.GetArray"]/*' />
        public Object GetArray()
        {
            return m_array;
        }
    
        /// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset.GetOffset"]/*' />
        public int GetOffset()
        {
            return m_offset;
        }
    
    	/// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset.GetHashCode"]/*' />
    	public override int GetHashCode()
    	{
    		return m_count + m_offset;
    	}
    	
    	/// <include file='doc\ArrayWithOffset.uex' path='docs/doc[@for="ArrayWithOffset.Equals"]/*' />
    	public override bool Equals(Object obj)
    	{
    		if (obj!=null && (obj is ArrayWithOffset)) {
    			ArrayWithOffset that = (ArrayWithOffset) obj;
    			return that.m_array == m_array && that.m_offset == m_offset && that.m_count == m_count;
    		}
    		else
    			return false;
    	}
    	
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern int CalculateCount();
    
        private Object m_array;
        private int    m_offset;
        private int    m_count;
    }

}
