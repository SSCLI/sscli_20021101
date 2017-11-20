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
** Class:  __HandleProtector
**
**                                                    
**
** Purpose: Base class to help prevent race conditions with
** OS handles.                                                   
**
** Date:   December 6, 2001
**
===========================================================*/
using System;
using System.Runtime.InteropServices;
using System.Security;
using System.Threading;

/*



 */

namespace System.Threading {
internal abstract class __HandleProtector
{
    private int _inUse;  // ref count
    private int _closed; // bool. No Interlocked::CompareExchange(bool)
    private int _handle;   // The real OS handle

    private const int InvalidHandle = -1;

    protected internal __HandleProtector(IntPtr handle)
    {
        if (handle == (IntPtr) InvalidHandle)
            throw new ArgumentException("__HandleProtector doesn't expect an invalid handle!");
        _inUse = 1;
        _closed = 0;
        _handle = handle.ToInt32();
    }

    internal IntPtr Handle {
        get { return (IntPtr) _handle; }
    }

    internal bool IsClosed {
        get { return _closed != 0; }
    }

    // Returns true if we succeeded, else false.                                        
    //                   Use this coding style.
    //    bool incremented = false;
    //    try {
    //        if (_hp.TryAddRef(ref incremented)) {
    //            ... // Use handle
    //        }
    //        else
    //            throw new ObjectDisposedException("Your handle was closed.");
    //    }
    //    finally {
    //        if (incremented) _hp.Release();
    //    }
    internal bool TryAddRef(ref bool incremented)
    {
        if (_closed == 0) {
            Interlocked.Increment(ref _inUse);
            incremented = true;
            if (_closed == 0)
                return true;
            Release();
            incremented = false;
        }
        return false;
    }

    internal void Release()
    {
        if (Interlocked.Decrement(ref _inUse) == 0) {
            int h = _handle;
            if (h != InvalidHandle) {
                if (h == Interlocked.CompareExchange(ref _handle, InvalidHandle, h)) {
                    FreeHandle(new IntPtr(h));
                }
            }
        }
    }

    protected internal abstract void FreeHandle(IntPtr handle);

    internal void Close()
    {
        int c = _closed;
        if (c != 1) {
            if (c == Interlocked.CompareExchange(ref _closed, 1, c)) {
                Release();
            }
        }
    }

    // This should only be called for cases when you know for a fact that
    // your handle is invalid and you want to record that information.
    // An example is calling a syscall and getting back ERROR_INVALID_HANDLE.
    // This method will normally leak handles!
    internal void ForciblyMarkAsClosed()
    {
        _closed = 1;
        _handle = InvalidHandle;
    }
}
}
