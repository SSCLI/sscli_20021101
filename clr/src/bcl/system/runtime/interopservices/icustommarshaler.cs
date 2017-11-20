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
** Class: ICustomMarshaler
**
**                                                  
**
** Purpose: This the base interface that must be implemented by all custom
**          marshalers.
**
** Date: August 19, 1999
**
=============================================================================*/

namespace System.Runtime.InteropServices {
	using System;

    /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler"]/*' />
    public interface ICustomMarshaler
    {		
        /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler.MarshalNativeToManaged"]/*' />
        Object MarshalNativeToManaged( IntPtr pNativeData );

        /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler.MarshalManagedToNative"]/*' />
        IntPtr MarshalManagedToNative( Object ManagedObj );

        /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler.CleanUpNativeData"]/*' />
        void CleanUpNativeData( IntPtr pNativeData );

        /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler.CleanUpManagedData"]/*' />
        void CleanUpManagedData( Object ManagedObj );

        /// <include file='doc\ICustomMarshaler.uex' path='docs/doc[@for="ICustomMarshaler.GetNativeDataSize"]/*' />
        int GetNativeDataSize();
    }
}
