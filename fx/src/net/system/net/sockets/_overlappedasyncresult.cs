//------------------------------------------------------------------------------
// <copyright file="_OverlappedAsyncResult.cs" company="Microsoft">
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

namespace System.Net.Sockets {
    using System;
    using System.Runtime.InteropServices;
    using System.Threading;
    using Microsoft.Win32;

    //
    //  OverlappedAsyncResult - used to take care of storage for async Socket operation
    //   from the BeginSend, BeginSendTo, BeginReceive, BeginReceiveFrom calls.
    //
    internal class OverlappedAsyncResult : LazyAsyncResult {

        //
        // internal class members
        //

        //
        // note: this fixes a possible AV caused by a race condition
        // between the Completion Port and the Winsock API, if the Completion
        // Port

        //
        // set bytes transferred explicitly to 0.
        // this fixes a possible race condition in NT.
        // note that (in NT) we don't look
        // at this value. passing in a NULL pointer, though, would save memory
        // and some time, but would cause an AV in winsock.
        //
        // note: this call could actually be avoided, because
        // when unmanaged memory is allocate the runtime will set it to 0.
        // furthermore, the memory pointed by m_BytesRead will be used only
        // for this single async call, and will not be reused. in the end there's
        // no reason for it to be set to something different than 0. we set
        // it explicitly to make the code more readable and to avoid
        // bugs in the future caused by code misinterpretation.
        //

        //
        // Memory for BytesTransferred pointer int.
        //
        internal static IntPtr  m_BytesTransferred = Marshal.AllocHGlobal( 4 );

        private IntPtr          m_UnmanagedBlob;    // Handle for global memory.
        private AutoResetEvent  m_OverlappedEvent;
        private int             m_CleanupCount;
        internal SocketAddress  m_SocketAddress;
        internal SocketAddress  m_SocketAddressOriginal; // needed for partial BeginReceiveFrom/EndReceiveFrom completion
        internal SocketFlags    m_Flags;

        //
        // these are used in alternative
        //
        internal WSABuffer      m_WSABuffer;
        private GCHandle        m_GCHandle; // Handle for pinned buffer.

        internal GCHandle        m_GCHandleSocketAddress; // Handle to FromAddress buffer
        internal GCHandle        m_GCHandleSocketAddressSize; // Handle to From Address size

        internal WSABuffer[]    m_WSABuffers;
        private GCHandle[]      m_GCHandles; // Handles for pinned buffers.

        //
        // Constructor. We take in the socket that's creating us, the caller's
        // state object, and the buffer on which the I/O will be performed.
        // We save the socket and state, pin the callers's buffer, and allocate
        // an event for the WaitHandle.
        //
        internal OverlappedAsyncResult(Socket socket, Object asyncState, AsyncCallback asyncCallback)
        : base(socket, asyncState, asyncCallback) {
            //
            // BeginAccept() allocates and returns an AcceptAsyncResult that will call
            // this constructor passign in a null buffer. no memory is allocated, so we
            // set m_UnmanagedBlob to 0 in order for the Cleanup function to know if it
            // needs to free unmanaged memory
            //
            m_UnmanagedBlob = IntPtr.Zero;

                //
                // we're using overlapped IO, allocate an overlapped structure
                // consider using a static growing pool of allocated unmanaged memory.
                //
                m_UnmanagedBlob = Marshal.AllocHGlobal(Win32.OverlappedSize);
                //
                // since the binding between the event handle and the callback
                // happens after the IO was queued to the OS, there is no race
                // condition and the Cleanup code can be called at most once.
                //
                m_CleanupCount = 1;
        }


        //
        // The overlapped function called (either by the thread pool or the socket)
        // when IO completes. (only called on Win9x)
        //
        internal void OverlappedCallback(object stateObject, bool Signaled) {
            OverlappedAsyncResult asyncResult = (OverlappedAsyncResult)stateObject;

            //
            // See if it's been completed already. If it has, we've been called
            // directly, so all we have to do it call the user's callback.
            //

            if (!asyncResult.IsCompleted) {
                //
                // the IO completed asynchronously, see if there was a failure:
                // We need to call WSAGetOverlappedResult to find out the errorCode,
                // of this IO, but we will, instead, look into the overlapped
                // structure to improve performance avoiding a PInvoke call.
                // the following code avoids a very expensive call to
                // the Win32 native WSAGetOverlappedResult() function.
                //
                asyncResult.Result = Win32.HackedGetOverlappedResult( asyncResult.m_UnmanagedBlob );

                //
                // this will release the unmanaged pin handles and unmanaged overlapped ptr
                //
                asyncResult.ReleaseUnmanagedStructures();
            }

            //
            // complete the IO and call the user's callback.
            //
            asyncResult.InvokeCallback(false);

        } // OverlappedCallback()


        //
        // SetUnmanagedStructures -
        // Fills in Overlapped Structures used in an Async Overlapped Winsock call
        //   these calls are outside the runtime and are unmanaged code, so we need
        //   to prepare specific structures and ints that lie in unmanaged memory
        //   since the Overlapped calls can be Async
        //
        internal void SetUnmanagedStructures(
            byte[] buffer,
            int offset,
            int size,
            SocketFlags socketFlags,
            EndPoint remoteEP,
            bool pinRemoteEP ) {

                //
                // create the event handle
                //

                m_OverlappedEvent = new AutoResetEvent(false);

                //
                // fill in the overlapped structure with the event handle.
                //

                Marshal.WriteIntPtr(
                    m_UnmanagedBlob,
                    Win32.OverlappedhEventOffset,
                    m_OverlappedEvent.Handle );

            //
            // Fill in Buffer Array structure that will be used for our send/recv Buffer
            //
            m_WSABuffer = new WSABuffer();
            m_GCHandle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            m_WSABuffer.Length = size;
            m_WSABuffer.Pointer = Marshal.UnsafeAddrOfPinnedArrayElement(buffer, offset);

            //
            // fill in flags if we use it.
            //
            m_Flags = socketFlags;

            //
            // if needed fill end point
            //
            if (remoteEP != null) {
                m_SocketAddress = remoteEP.Serialize();
                if (pinRemoteEP) {
                    m_GCHandleSocketAddress = GCHandle.Alloc(m_SocketAddress.m_Buffer, GCHandleType.Pinned);
                    m_GCHandleSocketAddressSize = GCHandle.Alloc(m_SocketAddress.m_Size, GCHandleType.Pinned);                
                }
            }

        } // SetUnmanagedStructures()

        internal void SetUnmanagedStructures(
            BufferOffsetSize[] buffers,
            SocketFlags socketFlags) {

                //
                // create the event handle
                //
                m_OverlappedEvent = new AutoResetEvent(false);

                //
                // fill in the overlapped structure with the event handle.
                //
                Marshal.WriteIntPtr(
                    m_UnmanagedBlob,
                    Win32.OverlappedhEventOffset,
                    m_OverlappedEvent.Handle );

            //
            // Fill in Buffer Array structure that will be used for our send/recv Buffer
            //
            m_WSABuffers = new WSABuffer[buffers.Length];
            m_GCHandles = new GCHandle[buffers.Length];
            for (int i = 0; i < buffers.Length; i++) {
                m_GCHandles[i] = GCHandle.Alloc(buffers[i].Buffer, GCHandleType.Pinned);
                m_WSABuffers[i].Length = buffers[i].Size;
                m_WSABuffers[i].Pointer = Marshal.UnsafeAddrOfPinnedArrayElement(buffers[i].Buffer, buffers[i].Offset);
            }
            //
            // fill in flags if we use it.
            //
            m_Flags = socketFlags;
        }


        //
        // This method is called after an asynchronous call is made for the user,
        // it checks and acts accordingly if the IO:
        // 1) completed synchronously.
        // 2) was pended.
        // 3) failed.
        //
        internal unsafe void CheckAsyncCallOverlappedResult(int errorCode) {
            //
            // Check if the Async IO call:
            // 1) was pended.
            // 2) completed synchronously.
            // 3) failed.
            //

                //
                // we're using overlapped IO under Win9x (or NT with registry setting overriding
                // completion port usage)
                //
                switch (errorCode) {

                case 0:
                    //
                    // the Async IO call completed synchronously:
                    // save the number of bytes transferred
                    // use our internal hacked WSAGetOverlappedResult()
                    //
                    Result = Win32.HackedGetOverlappedResult( m_UnmanagedBlob );
                    //
                    // let's queue to the ThreadPool to avoid growing the stack too much,
                    // and having execution exactly the same as if we were using completion ports.
                    //
                    goto case SocketErrors.WSA_IO_PENDING;

                case SocketErrors.WSA_IO_PENDING:

                    //
                    // the Async IO call was pended:
                    // Queue our event to the thread pool.
                    //
                    ThreadPool.RegisterWaitForSingleObject(
                                                          m_OverlappedEvent,
                                                          new WaitOrTimerCallback(OverlappedCallback),
                                                          this,
                                                          -1,
                                                          true );

                    //
                    // we're done, completion will be asynchronous
                    // in the callback. return
                    //
                    return;

                default:
                    //
                    // the Async IO call failed:
                    // set the number of bytes transferred to -1 (error)
                    //
                    ErrorCode = errorCode;
                    Result = -1;
                    ReleaseUnmanagedStructures();

                    break;
                }

        } // CheckAsyncCallOverlappedResult()

        //
        // The following property returns the Win32 unsafe pointer to
        // whichever Overlapped structure we're using for IO.
        //
        internal unsafe IntPtr IntOverlapped {
            get {
                    return m_UnmanagedBlob;
            }

        } // IntOverlapped


        private int m_HashCode = 0;
        private bool m_ComputedHashCode = false;
        public override int GetHashCode() {
            if (!m_ComputedHashCode) {
                //
                // compute HashCode on demand
                //
                m_HashCode = base.GetHashCode();
                m_ComputedHashCode = true;
            }
            return m_HashCode;
        }


        //
        // Utility cleanup routine. Frees pinned and unmanged memory.
        //
        private void CleanupUnmanagedStructures() {
            //
            // free the unmanaged memory if allocated.
            //
            if (((long)m_UnmanagedBlob) != 0) {
                Marshal.FreeHGlobal(m_UnmanagedBlob);
                m_UnmanagedBlob = IntPtr.Zero;
            }
            //
            // free handles to pinned buffers
            //
            if (m_GCHandle.IsAllocated) {
                m_GCHandle.Free();
            }

            if (m_GCHandleSocketAddress.IsAllocated) {
                m_GCHandleSocketAddress.Free();
            }

            if (m_GCHandleSocketAddressSize.IsAllocated) {
                m_GCHandleSocketAddressSize.Free();
            }

            if (m_GCHandles != null) {
                for (int i = 0; i < m_GCHandles.Length; i++) {
                    if (m_GCHandles[i].IsAllocated) {
                        m_GCHandles[i].Free();
                    }
                }
            }
            //
            // clenaup base class
            //
            base.Cleanup();

        } // CleanupUnmanagedStructures()


        private void ForceReleaseUnmanagedStructures() {
            CleanupUnmanagedStructures();
        }

        internal void ReleaseUnmanagedStructures() {
            if (Interlocked.Decrement(ref m_CleanupCount) == 0) {
                ForceReleaseUnmanagedStructures();
            }
        }


    }; // class OverlappedAsyncResult




} // namespace System.Net.Sockets
