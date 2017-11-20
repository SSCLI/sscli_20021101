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
** Class:  __UnmanagedMemoryStream
**
**                                                    
**
** Purpose: Create a stream over unmanaged memory, mostly
**          useful for memory-mapped .resources files.
**
** Date:  October 20, 2000
**
===========================================================*/
using System;
using System.Runtime.InteropServices;

namespace System.IO {

    /*
     * This class is used to access a block of bytes in memory, likely outside 
     * the GC heap (or pinned in place in the GC heap, but a MemoryStream may 
     * make more sense in those cases).  It's great if you have a pointer and
     * a length for a section of memory mapped in by someone else and you don't
     * want to copy this into the GC heap.  UnmanagedMemoryStream assumes these 
     * two things:
     *
     * 1) All the memory in the specified block is readable (and potentially 
     *    writable).
     * 2) The lifetime of the block of memory is at least as long as the lifetime
     *    of the UnmanagedMemoryStream.
     * 3) You clean up the memory when appropriate.  The UnmanagedMemoryStream 
     *    currently will do NOTHING to free this memory.
     * 4) All calls to Write and WriteByte may not be threadsafe currently.
     *                                                                         
     *
     *                                                                             
     * 
     * Note: feel free to make this class not sealed if necessary.
     */
    [CLSCompliant(false)]
    internal sealed class __UnmanagedMemoryStream : Stream
    {
        private const long MemStreamMaxLength = Int32.MaxValue;

        private unsafe byte* _mem;
        private int _length;
        private int _capacity;
        private int _position;
        private bool _writable;
        private bool _isOpen;

        internal unsafe __UnmanagedMemoryStream(byte* memory, long length, long capacity, bool canWrite) 
        {
            BCLDebug.Assert(memory != null, "Expected a non-zero address to start reading from!");
            BCLDebug.Assert(length <= capacity, "Length was greater than the capacity!");

            _mem = memory;
            _length = (int)length;
            _capacity = (int)capacity;
            _writable = canWrite;
            _isOpen = true;
        }

        public override bool CanRead {
    		get { return _isOpen; }
        }

        public override bool CanSeek {
    		get { return _isOpen; }
        }

        public override bool CanWrite {
    		get { return _writable; }
        }

        public unsafe override void Close()
        {
            _isOpen = false;
            _writable = false;
            _mem = null;
        }

        public override void Flush() {
        }

        public override long Length {
            get {
                if (!_isOpen) __Error.StreamIsClosed();
                return _length;
            }
        }

        public override long Position {
    		get { 
                if (!_isOpen) __Error.StreamIsClosed();
                return _position;
            }
    		set {
                if (!_isOpen) __Error.StreamIsClosed();
                if (value < 0)
                    throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
    			_position = (int)value;
    		}
        }

        public unsafe byte* GetBytePtr()
        {
            return _mem + _position;
        }

        public override unsafe int Read([In, Out] byte[] buffer, int offset, int count) {
            if (!_isOpen) __Error.StreamIsClosed();
    		if (buffer==null)
    			throw new ArgumentNullException("buffer", Environment.GetResourceString("ArgumentNull_Buffer"));
    		if (offset < 0)
    			throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
    		if (count < 0)
    			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (buffer.Length - offset < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            // Use a local variable to avoid a race where another thread 
            // changes our position after we decide we can read some bytes.
            int pos = _position;
            int n = _length - pos;
            if (n > count) n = count;
            if (n < 0) n = 0;  // _position could be beyond EOF
            BCLDebug.Assert(pos + n >= 0, "_position + n >= 0");  // len is less than 2^31 -1.

            memcpy(_mem, pos, buffer, offset, n);
            _position = pos + n;
            return n;
        }

        public override unsafe int ReadByte() {
            if (!_isOpen) __Error.StreamIsClosed();
            int pos = _position;  // Use a local to avoid a race condition
            if (pos >= _length) return -1;
            _position = pos + 1;
            return _mem[pos];
        }

        public override unsafe long Seek(long offset, SeekOrigin loc) {
            if (!_isOpen) __Error.StreamIsClosed();
            if (offset > MemStreamMaxLength)
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_MemStreamLength"));
    		switch(loc) {
    		case SeekOrigin.Begin:
                if (offset < 0)
                    throw new IOException(Environment.GetResourceString("IO.IO_SeekBeforeBegin"));
    			_position = (int)offset;
    			break;
    			
    		case SeekOrigin.Current:
                if (offset + _position < 0)
                    throw new IOException(Environment.GetResourceString("IO.IO_SeekBeforeBegin"));
    			_position += (int)offset;
    			break;
    			
    		case SeekOrigin.End:
    			if (_length + offset < 0)
    				throw new IOException(Environment.GetResourceString("IO.IO_SeekBeforeBegin"));
    			_position = _length + (int)offset;
    			break;
    			
    		default:
    			throw new ArgumentException(Environment.GetResourceString("Argument_InvalidSeekOrigin"));
    		}

            BCLDebug.Assert(_position >= 0, "_position >= 0");
    		return _position;
        }

        public override void SetLength(long value) {
            if (!_writable) __Error.WriteNotSupported();
            if (value > _capacity)
                throw new IOException(Environment.GetResourceString("IO.IO_FixedCapacity"));
            _length = (int) value;
            if (_position > value) _position = (int) value;
        }

        public override unsafe void Write(byte[] buffer, int offset, int count) {
            if (!_isOpen) __Error.StreamIsClosed();
            if (!_writable) __Error.WriteNotSupported();
    		if (buffer==null)
    			throw new ArgumentNullException("buffer", Environment.GetResourceString("ArgumentNull_Buffer"));
    		if (offset < 0)
    			throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
    		if (count < 0)
    			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (buffer.Length - offset < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            int pos = _position;  // Use a local to avoid a race condition
            int n = pos + count;
            // Check for overflow
            if (n < 0)
                throw new IOException(Environment.GetResourceString("IO.IO_StreamTooLong"));

            if (n > _length) {
                if (n > _capacity)
                    throw new IOException(Environment.GetResourceString("IO.IO_FixedCapacity"));
                _length = n;
            }
            memcpy(buffer, offset, _mem, pos, count);
            _position = n;
            return;
        }

        public override unsafe void WriteByte(byte value) {
            if (!_isOpen) __Error.StreamIsClosed();
            if (!_writable) __Error.WriteNotSupported();
            int pos = _position;  // Use a local to avoid a race condition
            if (pos == _length) {
                if (pos + 1 > _capacity)
                    throw new IOException(Environment.GetResourceString("IO.IO_FixedCapacity"));
                _length++;
            }
            _mem[pos] = value;
            _position = pos + 1;
        }


        internal unsafe static void memcpy(byte* src, int srcIndex, byte[] dest, int destIndex, int len) {
            BCLDebug.Assert(dest.Length - destIndex >= len, "not enough bytes in dest");
            // If dest has 0 elements, the fixed statement will throw an 
            // IndexOutOfRangeException.  Special-case 0-byte copies.
            if (len==0)
                return;
            fixed(byte* pDest = dest) {
                memcpyimpl(src+srcIndex, pDest+destIndex, len);
            }
        }

        internal unsafe static void memcpy(byte[] src, int srcIndex, byte* dest, int destIndex, int len) {
            BCLDebug.Assert(src.Length - srcIndex >= len, "not enough bytes in src");
            // If src has 0 elements, the fixed statement will throw an 
            // IndexOutOfRangeException.  Special-case 0-byte copies.
            if (len==0)
                return;
            fixed(byte* pSrc = src) {
                memcpyimpl(pSrc+srcIndex, dest+destIndex, len);
            }
        }

        internal unsafe static void memcpyimpl(byte* src, byte* dest, int len) {

            // Portable naive implementation
            while (len-- > 0)
                *dest++ = *src++;
        }        
    }
}
