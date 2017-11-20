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
** Class: Overlapped
**
** Author:                                          
**
** Purpose: Class for converting information to and from the native 
**          overlapped structure used in asynchronous file i/o
**
** Date: January, 2000
**
=============================================================================*/


namespace System.Threading 
{   
    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;
    using System.Security;
    using System.Security.Permissions;

    // Valuetype that represents the (unmanaged) Win32 OVERLAPPED structure
    // the layout of this structure must be identical to OVERLAPPED.
    // Two additional dwords are reserved at the end
    /// <include file='doc\Overlapped.uex' path='docs/doc[@for="NativeOverlapped"]/*' />
    [System.Runtime.InteropServices.StructLayout(LayoutKind.Sequential)]
    public struct NativeOverlapped
    {
        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="NativeOverlapped.InternalLow"]/*' />
        public int  InternalLow;
        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="NativeOverlapped.InternalHigh"]/*' />
        public int  InternalHigh;
        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="NativeOverlapped.OffsetLow"]/*' />
        public int  OffsetLow;
        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="NativeOverlapped.OffsetHigh"]/*' />
        public int  OffsetHigh;
        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="NativeOverlapped.EventHandle"]/*' />
        public int  EventHandle;
        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="NativeOverlapped.ReservedCOR1"]/*' />
        internal Int32 ReservedCOR1;
        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="NativeOverlapped.ReservedCOR2"]/*' />
        internal GCHandle ReservedCOR2;
        internal IntPtr ReservedCOR3;
        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="NativeOverlapped.ReservedClasslib"]/*' />
        internal GCHandle ReservedClasslib;
    }

    /// <include file='doc\Overlapped.uex' path='docs/doc[@for="Overlapped"]/*' />
    /// <internalonly/>
    public class Overlapped
    {
        private IAsyncResult asyncResult;
        private int offsetLow;
        private int offsetHigh;
        private int eventHandle;

        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="Overlapped.Overlapped"]/*' />
        public Overlapped() { }

        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="Overlapped.Overlapped1"]/*' />
        public Overlapped(int offsetLo, int offsetHi, int hEvent, IAsyncResult ar)
        {
            OffsetLow = offsetLo;
            OffsetHigh = offsetHi;
            EventHandle = hEvent;
            asyncResult = ar;
        }

        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="Overlapped.AsyncResult"]/*' />
        public IAsyncResult AsyncResult
        {
            get { return asyncResult; }
            set { asyncResult = value; }
        }

        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="Overlapped.OffsetLow"]/*' />
        public int OffsetLow
        {
            get { return offsetLow; }
            set { offsetLow = value; }
        }

        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="Overlapped.OffsetHigh"]/*' />
        public int OffsetHigh
        {
            get { return offsetHigh; }
            set { offsetHigh = value; }
        }

        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="Overlapped.EventHandle"]/*' />
        public int EventHandle
        {
            get { return eventHandle; }
            set { eventHandle = value; }
        }


        /*====================================================================
        *  Unpacks an unmanaged native Overlapped struct. 
        *  Unpins the native Overlapped struct
        ====================================================================*/
        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="Overlapped.Unpack"]/*' />
        [CLSCompliant(false)]
        unsafe public static Overlapped Unpack(NativeOverlapped* nativeOverlappedPtr)
        {
            if (nativeOverlappedPtr == null)
                throw new ArgumentNullException("nativeOverlappedPtr");

            // Debugging aid
            if (nativeOverlappedPtr->ReservedCOR1 == unchecked((int)0xdeadbeef))
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_OverlappedFreed"));

            if (AppDomain.CurrentDomain.GetId() 
                != (*nativeOverlappedPtr).ReservedCOR1)
            {
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_UnpackInWrongAppdomain")); 
            }
            
            Overlapped overlapped = new Overlapped();

            overlapped.offsetLow = (*nativeOverlappedPtr).OffsetLow;
            overlapped.offsetHigh = (*nativeOverlappedPtr).OffsetHigh;
            overlapped.internalLow = (*nativeOverlappedPtr).InternalLow;
            overlapped.internalHigh = (*nativeOverlappedPtr).InternalHigh;
            overlapped.eventHandle = (*nativeOverlappedPtr).EventHandle;

            GCHandle eeHandleForAr = (*nativeOverlappedPtr).ReservedClasslib;
            overlapped.asyncResult = (IAsyncResult) eeHandleForAr.__InternalTarget;
            
            return overlapped;
        }

        /// <include file='doc\Overlapped.uex' path='docs/doc[@for="Overlapped.Free"]/*' />
        [CLSCompliant(false)]
        unsafe public static void Free(NativeOverlapped* nativeOverlappedPtr)
        {
            if (nativeOverlappedPtr == null)
                throw new ArgumentNullException("nativeOverlappedPtr");

            // Debugging aid
            if (nativeOverlappedPtr->ReservedCOR1 == unchecked((int)0xdeadbeef))
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_OverlappedFreedTwice"));
            nativeOverlappedPtr->ReservedCOR1 = unchecked((int)0xdeadbeef);

#if _DEBUG
            nativeOverlappedPtr->InternalHigh = unchecked((int)0xdeadbeef);
            nativeOverlappedPtr->InternalLow = unchecked((int)0xdeadbeef);
            nativeOverlappedPtr->OffsetHigh = unchecked((int)0xdeadbeef);
            nativeOverlappedPtr->OffsetLow = unchecked((int)0xdeadbeef);
            nativeOverlappedPtr->EventHandle = unchecked((int)0xdeadbeef);
#endif

            GCHandle eeHandleForAr = (*nativeOverlappedPtr).ReservedClasslib;
            eeHandleForAr.__InternalFree();
            (*nativeOverlappedPtr).ReservedCOR2.__InternalFree();
            if (nativeOverlappedPtr->ReservedCOR3 != (IntPtr)null)
            {
#if _DEBUG
                if (nativeOverlappedPtr->ReservedCOR3 == (IntPtr)unchecked((int)0xdeadbeef))
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_OverlappedFreedTwice"));
#endif
                CodeAccessSecurityEngine.ReleaseDelayedCompressedStack( nativeOverlappedPtr->ReservedCOR3 );
            }

#if _DEBUG
            nativeOverlappedPtr->ReservedCOR3 = new IntPtr( unchecked((int)0xdeadbeef) );
#endif

            FreeNativeOverlapped(nativeOverlappedPtr);
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private unsafe static extern NativeOverlapped* AllocNativeOverlapped();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        unsafe private static extern void FreeNativeOverlapped(NativeOverlapped* nativeOverlappedPtr);

        private int internalLow;        // reserved for OS use
        private int internalHigh;       // reserved for OS use
    }
}
