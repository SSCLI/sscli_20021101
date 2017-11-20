//------------------------------------------------------------------------------
// <copyright file="_BufferOffsetSize.cs" company="Microsoft">
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
    //
    // this class is used by the BeginMultipleSend() API
    // to allow a user to send multiple buffers on a socket
    //
    internal class BufferOffsetSize {
        //
        // internal members
        //
        public byte[] Buffer;
        public int Offset;
        public int Size;

        public BufferOffsetSize(byte[] buffer, int offset, int size, bool copyBuffer) {
            GlobalLog.Assert(buffer!=null && buffer.Length>=size+offset, "BufferOffsetSize(Illegal parameters)", "");
            if (copyBuffer) {
                byte[] newBuffer = new byte[size];

                System.Buffer.BlockCopy(
                    buffer,     // src
                    offset,     // src index
                    newBuffer,  // dest
                    0,          // dest index
                    size );     // total size to copy

                offset = 0;
                buffer = newBuffer;
            }
            Buffer = buffer;
            Offset = offset;
            Size = size;
            GlobalLog.Print("BufferOffsetSize#" + ValidationHelper.HashString(this) + "::.ctor() copyBuffer:" + copyBuffer.ToString() + " this:[" + ToString() + "]");
        }

        public BufferOffsetSize(byte[] buffer, int offset, bool copyBuffer)
            : this(buffer, offset, buffer.Length - offset, copyBuffer) {
        }

        public BufferOffsetSize(int size, byte[] buffer, bool copyBuffer)
            : this(buffer, 0, size, copyBuffer) {
        }

        public BufferOffsetSize(byte[] buffer, bool copyBuffer)
            : this(buffer, 0, buffer.Length, copyBuffer) {
        }


    } // class BufferOffsetSize



} // namespace System.Net
