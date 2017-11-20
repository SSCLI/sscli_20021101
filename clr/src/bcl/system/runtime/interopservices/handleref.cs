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

    /// <include file='doc\HandleRef.uex' path='docs/doc[@for="HandleRef"]/*' />
    public struct HandleRef
    {

        // ! Do not add or rearrange fields as the EE depends on this layout.
        //------------------------------------------------------------------
        internal Object m_wrapper;
        internal IntPtr m_handle;
        //------------------------------------------------------------------


        /// <include file='doc\HandleRef.uex' path='docs/doc[@for="HandleRef.HandleRef"]/*' />
        public HandleRef(Object wrapper, IntPtr handle)
        {
            m_wrapper = wrapper;
            m_handle  = handle;
        }

        /// <include file='doc\HandleRef.uex' path='docs/doc[@for="HandleRef.Wrapper"]/*' />
        public Object Wrapper {
            get {
                return m_wrapper;
            }
        }
    
        /// <include file='doc\HandleRef.uex' path='docs/doc[@for="HandleRef.Handle"]/*' />
        public IntPtr Handle {
            get {
                return m_handle;
            }
        }
    
    
        /// <include file='doc\HandleRef.uex' path='docs/doc[@for="HandleRef.operatorIntPtr"]/*' />
        public static explicit operator IntPtr(HandleRef value)
        {
            return value.m_handle;
        }

    }

}
