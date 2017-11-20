//------------------------------------------------------------------------------
// <copyright file="_Win32.cs" company="Microsoft">
//     
//      Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//     
//      The use and distribution terms for this software are contained in the file
//      named license.txt, which can be found in the root of this distribution.
//      By using this software in any fashion, you are agreeing to be bound by the
//      terms of this license.
//     
//      You must not remove this notice, or any other, from this software.
//     
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Runtime.InteropServices;

    internal class Win32 {

        internal const int OverlappedInternalOffset     = 0;
        internal const int OverlappedInternalHighOffset = 4;
        internal const int OverlappedOffsetOffset       = 8;
        internal const int OverlappedOffsetHighOffset   = 12;
        internal const int OverlappedhEventOffset       = 16;
        internal const int OverlappedSize               = 20;

        internal static int HackedGetOverlappedResult(IntPtr m_UnmanagedOverlapped ) {
            //
            // read IO asynchronous stauts from Overlapped structure
            //
            int status = Marshal.ReadInt32(IntPtrHelper.Add(m_UnmanagedOverlapped, OverlappedInternalOffset));

            if (status == 0) {
                //
                // the Async IO call completed asynchronously:
                // save the bytes read
                //
                return Marshal.ReadInt32(IntPtrHelper.Add(m_UnmanagedOverlapped, OverlappedInternalHighOffset));
            }
            else {
                //
                // the Async IO call failed asynchronously:
                // save the bytes read
                //
                return -1;
            }

        } // HackedGetOverlappedResult()

    }; // internal class Win32


} // namespace System.Net
