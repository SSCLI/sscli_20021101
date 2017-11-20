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
** Class:  FileStream
**
**                                                    
**
** Purpose: Exposes a Stream around a file, with full 
** synchronous and asychronous support, and buffering.
**
** Date:  February 21, 2000
**
===========================================================*/
using System;
using Microsoft.Win32;
using System.Security;
using System.Security.Permissions;
using System.Threading;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Messaging;
using System.Runtime.CompilerServices;

/*
 * Implementation notes:
 * FileStream supports different modes of accessing the disk - async mode
 * and sync mode.  They are two completely different codepaths in the
 * sync & async methods (ie, Read/Write vs. BeginRead/BeginWrite).  File
 * handles in NT can be opened in only sync or overlapped (async) mode,
 * and we have to deal with this pain.  Stream has implementations of
 * the sync methods in terms of the async ones and vice versa, so we'll
 * call through to our base class to get those methods when necessary.
 *
 *                                                                           
 */

namespace System.IO {
    /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream"]/*' />

    public class FileStream : Stream
    {
        internal const int DefaultBufferSize = 4096;


        private static readonly bool _canUseAsync = false;

        private byte[] _buffer;   // Shared read/write buffer.  Alloc on first use.
        private String _fileName; // Fully qualified file name.
        private bool _isAsync;    // Whether we opened the handle for overlapped IO
        private bool _canRead;
        private bool _canWrite;
        private bool _canSeek;
        private bool _ownsHandle; // Whether we can call Close on this handle.
        private bool _isPipe;     // Whether to disable async buffering code.
        private int _readPos;     // Read pointer within shared buffer.
        private int _readLen;     // Number of bytes read in buffer from file.
        private int _writePos;    // Write pointer within shared buffer.
        private int _bufferSize;  // Length of internal buffer, if it's allocated.
        private IntPtr _handle;
        private long _pos;        // Cache current location in the file.
        private __HandleProtector _handleProtector;  // See the __HandleProtector class.
        private long _appendStart;// When appending, prevent overwriting file.

        // This is an internal object implementing IAsyncResult with fields
        // for all of the relevant data necessary to complete the IO operation.
        // This is used by AsyncFSCallback and all of the async methods.
        unsafe internal class AsyncFileStream_AsyncResult : IAsyncResult
        {
            // User code callback
            internal AsyncCallback _userCallback;

            internal Object _userStateObject;
            internal GCHandle _bufferHandle;  // GCHandle to pin byte[].

            internal bool _isWrite;     // Whether this is a read or a write
            internal bool _isComplete;
            internal bool _completedSynchronously;  // Which thread called callback
            internal bool _bufferIsPinned;   // Whether our _bufferHandle is valid.
            internal int _numBufferedBytes;





            /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.AsyncFileStream_AsyncResult.AsyncState"]/*' />
            public virtual Object AsyncState
            {
                get { return _userStateObject; }
            }

            /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.AsyncFileStream_AsyncResult.IsCompleted"]/*' />
            public bool IsCompleted
            {
                get { return _isComplete; }
                set { _isComplete = value; }
            }

            /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.AsyncFileStream_AsyncResult.AsyncWaitHandle"]/*' />
            public WaitHandle AsyncWaitHandle
            {
                get { return null; }
            }

            // Returns true iff the user callback was called by the thread that 
            // called BeginRead or BeginWrite.  If we use an async delegate or
            // threadpool thread internally, this will be false.  This is used
            // by code to determine whether a successive call to BeginRead needs 
            // to be done on their main thread or in their callback to avoid a
            // stack overflow on many reads or writes.
            public bool CompletedSynchronously
            {
                get { return _completedSynchronously; }
            }

            internal static AsyncFileStream_AsyncResult CreateBufferedReadResult(int numBufferedBytes, AsyncCallback userCallback, Object userStateObject) {
                AsyncFileStream_AsyncResult asyncResult = new AsyncFileStream_AsyncResult();
                asyncResult._userCallback = userCallback;
                asyncResult._userStateObject = userStateObject;
                asyncResult._isComplete = true;
                asyncResult._isWrite = false;
                asyncResult._numBufferedBytes = numBufferedBytes;
                return asyncResult;
            }
            
            internal void CallUserCallback()
            {
                // Convenience method for me, since I have to do this in a number 
                // of places in the buffering code for fake IAsyncResults.  Note 
                // that AsyncFSCallback intentionally does not use this method.
                if (_userCallback != null) {
                    // Call user's callback on a threadpool thread.  We do
                    // not technically need to call EndInvoke - the async
                    // delegate's IAsyncResult finalizer will clean up for us.
                    // Set completedSynchronously to false, since it's on another 
                    // thread, not the main thread.
                    _completedSynchronously = false;
                    _userCallback.BeginInvoke(this, null, null);
                }
            }

            // If this OS may return from a call to ReadFile or WriteFile then
            // later fill in the buffer, presumably we should make sure we don't
            // move the buffer around in the GC heap.
            internal void PinBuffer(byte[] buffer)
            {
                _bufferHandle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
                _bufferIsPinned = true;
            }

            internal void UnpinBuffer()
            {
                if (_bufferIsPinned) {
                    _bufferHandle.Free();
                    _bufferIsPinned = false;
                }
            }
        }

        //This exists only to support IsolatedStorageFileStream.
        //Any changes to FileStream must include the corresponding changes in IsolatedStorage.
        internal FileStream() { 
            _fileName = null;
            _handleProtector = null;
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.FileStream"]/*' />
        public FileStream(String path, FileMode mode) 
            : this(path, mode, (mode == FileMode.Append ? FileAccess.Write : FileAccess.ReadWrite), FileShare.Read, DefaultBufferSize, false, Path.GetFileName(path), false) {
        }
    
        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.FileStream1"]/*' />
        public FileStream(String path, FileMode mode, FileAccess access) 
            : this(path, mode, access, FileShare.Read, DefaultBufferSize, false, Path.GetFileName(path), false) {
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.FileStream2"]/*' />
        public FileStream(String path, FileMode mode, FileAccess access, FileShare share) 
            : this(path, mode, access, share, DefaultBufferSize, false, Path.GetFileName(path), false) {
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.FileStream3"]/*' />
        public FileStream(String path, FileMode mode, FileAccess access, FileShare share, int bufferSize) : this(path, mode, access, share, bufferSize, false, Path.GetFileName(path), false)
        {
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.FileStream4"]/*' />
        public FileStream(String path, FileMode mode, FileAccess access, FileShare share, int bufferSize, bool useAsync) : this(path, mode, access, share, bufferSize, useAsync, Path.GetFileName(path), false)
        {
        }

        internal FileStream(String path, FileMode mode, FileAccess access, FileShare share, int bufferSize, bool useAsync, String msgPath, bool bFromProxy)
        {
            // Note: msgPath must be safe to hand back to untrusted code.

            _fileName = msgPath;  // To handle odd cases of finalizing partially constructed objects.

            if (path == null)
                throw new ArgumentNullException("path", Environment.GetResourceString("ArgumentNull_Path"));
            if (path.Length == 0)
                throw new ArgumentException(Environment.GetResourceString("Argument_EmptyPath"));
            if (mode < FileMode.CreateNew || mode > FileMode.Append ||
                access < FileAccess.Read || access > FileAccess.ReadWrite ||
                share < FileShare.None || share > FileShare.ReadWrite) {
                String badArg = "mode";
                if (access < FileAccess.Read || access > FileAccess.ReadWrite)
                    badArg = "access";
                if (share < FileShare.None || share > FileShare.ReadWrite)
                    badArg = "share";
                throw new ArgumentOutOfRangeException(badArg, Environment.GetResourceString("ArgumentOutOfRange_Enum"));
            }
            if (bufferSize <= 0)
                throw new ArgumentOutOfRangeException("bufferSize", Environment.GetResourceString("ArgumentOutOfRange_NeedPosNum"));

            int fAccess = access == FileAccess.Read? GENERIC_READ:
                access == FileAccess.Write? GENERIC_WRITE:
                GENERIC_READ | GENERIC_WRITE;
            
            // Get absolute path - Security needs this to prevent something
            // like trying to create a file in c:\tmp with the name 
            // "..\WinNT\System32\ntoskrnl.exe".  Store it for user convenience.
            String filePath = Path.GetFullPathInternal(path);

            _fileName = filePath;

            // Build up security permissions required, as well as validate we
            // have a sensible set of parameters.  IE, creating a brand new file
            // for reading doesn't make much sense.
            FileIOPermissionAccess secAccess = FileIOPermissionAccess.NoAccess;
            if ((access & FileAccess.Read) != 0) {
                if (mode==FileMode.Append)
                    throw new ArgumentException(Environment.GetResourceString("Argument_InvalidAppendMode"));
                else
                    secAccess = secAccess | FileIOPermissionAccess.Read;
            }
            // I can't think of any combos of FileMode we should disallow if we
            // don't have read access.  Writing would pretty much always be valid
            // in those cases.

            if ((access & FileAccess.Write) != 0) {
                if (mode==FileMode.Append)
                    secAccess = secAccess | FileIOPermissionAccess.Append;
                else
                    secAccess = secAccess | FileIOPermissionAccess.Write;
            }
            else {
                // No write access
                if (mode==FileMode.Truncate || mode==FileMode.CreateNew || mode==FileMode.Create || mode==FileMode.Append)
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Argument_InvalidFileMode&AccessCombo"), mode, access));
            }

            new FileIOPermission(secAccess, new String[] { filePath }, false, false).Demand();

            bool seekToEnd = (mode==FileMode.Append);
            // Must use a valid Win32 constant here...
            if (mode == FileMode.Append)
                mode = FileMode.OpenOrCreate;

            // By default, FileStream-provided handles are not inheritable by 
            // child processes, but with FileShare.Inheritable you can redirect 
            // stdout from a child process to a log file from your parent, etc.
            Win32Native.SECURITY_ATTRIBUTES secAttrs = null;
            if ((share & FileShare.Inheritable) != 0) {
                secAttrs = new Win32Native.SECURITY_ATTRIBUTES();
                secAttrs.nLength = (int)Marshal.SizeOf(secAttrs);
                secAttrs.bInheritHandle = 1;
                share &= ~FileShare.Inheritable;
            }

            // Do the right thing for whatever platform we're on.  This way,
            // someone can easily write code that opens a file asynchronously
            // no matter what their platform is.
            IntPtr handle;

            if (_canUseAsync && useAsync) {
                handle = Win32Native.CreateFile(filePath, fAccess, share, secAttrs, mode, FILE_FLAG_OVERLAPPED, Win32Native.NULL);
                _isAsync = true;
            }
            else {
                handle = Win32Native.CreateFile(filePath, fAccess, share, secAttrs, mode, FILE_ATTRIBUTE_NORMAL, Win32Native.NULL);
                _isAsync = false;
            }
                
            if (handle != Win32Native.INVALID_HANDLE_VALUE) {
                _handleProtector = new __FileStreamHandleProtector(handle, true);
            }
            else {
                // Return a meaningful error, using the RELATIVE path to
                // the file to avoid returning extra information to the caller
                // unless they have path discovery permission, in which case
                // the full path is fine & useful.
                
                // NT5 oddity - when trying to open "C:\" as a FileStream,
                // we usually get ERROR_PATH_NOT_FOUND from the OS.  We should
                // probably be consistent w/ every other directory.
                int hr = Marshal.GetLastWin32Error();
                if (hr==__Error.ERROR_PATH_NOT_FOUND && filePath.Equals(Directory.InternalGetDirectoryRoot(filePath)))
                    hr = __Error.ERROR_ACCESS_DENIED;

                // We need to give an exception, and preferably it would include
                // the fully qualified path name.  Do security check here.  If
                // we fail, give back the msgPath, which should not reveal much.
                bool canGiveFullPath = false;

                if (!bFromProxy)
                {
                    try {
                        new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { _fileName }, false, false ).Demand();
                        canGiveFullPath = true;
                    }
                    catch(SecurityException) {}
                }

                if (canGiveFullPath)
                    __Error.WinIOError(hr, _fileName);
                else
                    __Error.WinIOError(hr, msgPath);
            }

            // Disallow access to all non-file devices from the FileStream
            // constructors that take a String.  Everyone else can call 
            // CreateFile themselves then use the constructor that takes an 
            // IntPtr.  Disallows "con:", "com1:", "lpt1:", etc.
            int fileType = Win32Native.GetFileType(handle);
            if (fileType != Win32Native.FILE_TYPE_DISK) {
                _handleProtector.Close();
                throw new NotSupportedException(Environment.GetResourceString("NotSupported_FileStreamOnNonFiles"));
            }


            _canRead = (access & FileAccess.Read) != 0;
            _canWrite = (access & FileAccess.Write) != 0;
            _canSeek = true;
            _isPipe = false;
            _pos = 0;
            _bufferSize = bufferSize;
            _readPos = 0;
            _readLen = 0;
            _writePos = 0;

            // For Append mode...
            if (seekToEnd) {
                _appendStart = SeekCore(0, SeekOrigin.End);
            }
            else {
                _appendStart = -1;
            }
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.FileStream5"]/*' />
        public FileStream(IntPtr handle, FileAccess access) 
            : this(handle, access, true, DefaultBufferSize, false) {
        }
        
        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.FileStream6"]/*' />
        public FileStream(IntPtr handle, FileAccess access, bool ownsHandle) 
            : this(handle, access, ownsHandle, DefaultBufferSize, false) {
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.FileStream7"]/*' />
        public FileStream(IntPtr handle, FileAccess access, bool ownsHandle, int bufferSize)
            : this(handle, access, ownsHandle, bufferSize, false) {
        }

        // Note we explicitly do a Demand, not a LinkDemand here.
        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.FileStream8"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public FileStream(IntPtr handle, FileAccess access, bool ownsHandle, int bufferSize, bool isAsync) {
            if (access < FileAccess.Read || access > FileAccess.ReadWrite)
                throw new ArgumentOutOfRangeException("access", Environment.GetResourceString("ArgumentOutOfRange_Enum"));
            if (bufferSize <= 0)
                throw new ArgumentOutOfRangeException("bufferSize", Environment.GetResourceString("ArgumentOutOfRange_NeedPosNum"));
            if (handle==Win32Native.INVALID_HANDLE_VALUE)
                throw new ArgumentException(Environment.GetResourceString("Arg_InvalidHandle"), "handle");

            int handleType = Win32Native.GetFileType(handle) & 0x7FFF;
            _handleProtector = new __FileStreamHandleProtector(handle, ownsHandle);
            _isAsync = isAsync && _canUseAsync;  // On Win9x, just do the right thing.
            _canRead = 0 != (access & FileAccess.Read);
            _canWrite = 0 != (access & FileAccess.Write);
            _canSeek = handleType == Win32Native.FILE_TYPE_DISK;
            _bufferSize = bufferSize;
            _readPos = 0;
            _readLen = 0;
            _writePos = 0;
            _fileName = null;
            _isPipe = handleType == Win32Native.FILE_TYPE_PIPE;

                if (handleType != Win32Native.FILE_TYPE_PIPE)
                    VerifyHandleIsSync();
            if (_canSeek) {
                _pos = SeekCore(0, SeekOrigin.Current);
                if (_pos > Length) {
                    _pos = SeekCore(0, SeekOrigin.End);
                }
            }
            else
                _pos = 0;
        }


        // Verifies that this handle supports synchronous IO operations (unless you
        // didn't open it for either reading or writing).
        private unsafe void VerifyHandleIsSync()
        {
            // Do NOT use this method on pipes.  Reading or writing to a pipe may
            // cause an app to block incorrectly, introducing a deadlock (depending
            // on whether a write will wake up an already-blocked thread or this
            // FileStream's thread).

            // Do NOT change this to use a byte[] of length 0, or test test won't
            // work.  Our ReadFile & WriteFile methods are special cased to return
            // for arrays of length 0, since we'd get an IndexOutOfRangeException 
            // while using C#'s fixed syntax.
            byte[] bytes = new byte[1];
            int hr = 0;
            int r = 0;
            
            // Note that if the handle is a pipe, ReadFile will block until there
            // has been a write on the other end.  We'll just have to deal with it,
            // I guess...  For the read end of a pipe, you can mess up and 
            // accidentally read synchronously from an async pipe.
            if (CanRead) {
                r = ReadFileNative(_handleProtector, bytes, 0, 0, null, out hr);
            }
            else if (CanWrite) {
                r = WriteFileNative(_handleProtector, bytes, 0, 0, null, out hr);
            }

            if (hr==ERROR_INVALID_PARAMETER)
                throw new ArgumentException(Environment.GetResourceString("Arg_HandleNotSync"));
            if (hr == Win32Native.ERROR_INVALID_HANDLE)
                __Error.WinIOError(hr, "<OS handle>");
        }


        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.CanRead"]/*' />
        public override bool CanRead {
            get { return _canRead; }
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.CanWrite"]/*' />
        public override bool CanWrite {
            get { return _canWrite; }
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.CanSeek"]/*' />
        public override bool CanSeek {
            get { return _canSeek; }
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.IsAsync"]/*' />
        public virtual bool IsAsync {
            get { return _isAsync; }
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Length"]/*' />
        public override long Length {
            get {
                if (_handleProtector.IsClosed) __Error.FileNotOpen();
                if (!CanSeek) __Error.SeekNotSupported();
                int hi = 0, lo = 0;

                bool incremented = false;
                try {
                    if (_handleProtector.TryAddRef(ref incremented)) 
                        lo = Win32Native.GetFileSize(_handleProtector.Handle, out hi);
                    else
                        __Error.FileNotOpen();
                }
                finally {
                    if (incremented) _handleProtector.Release();
                }

                if (lo==-1) {  // Check for either an error or a 4GB - 1 byte file.
                    int hr = Marshal.GetLastWin32Error();
                    if (hr != 0)
                        __Error.WinIOError(hr, String.Empty);
                }
                long len = (((long)hi) << 32) | ((uint) lo);
                // If we're writing near the end of the file, we must include our
                // internal buffer in our Length calculation.  Don't flush because
                // we use the length of the file in our async write method.
                if (_writePos > 0 && _pos + _writePos > len)
                    len = _writePos + _pos;
                return len;
            }
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Name"]/*' />
        public String Name {
            get {
                if (_fileName == null)
                    return Environment.GetResourceString("IO_UnknownFileName");
                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, new String[] { _fileName }, false, false ).Demand();
                return _fileName;
            }
        }

        internal String NameInternal {
            get {
                if (_fileName == null)
                    return "<UnknownFileName>";
                return _fileName;
            }
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Position"]/*' />
        public override long Position {
            get { 
                if (_handleProtector.IsClosed) __Error.FileNotOpen();
                if (!CanSeek) __Error.SeekNotSupported();
                BCLDebug.Assert((_readPos==0 && _readLen==0 && _writePos >= 0) || (_writePos==0 && _readPos <= _readLen), "We're either reading or writing, but not both.");

                VerifyOSHandlePosition();

                return _pos + (_readPos - _readLen + _writePos);
            }
            set {
                if (value < 0) throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
                if (_writePos > 0) FlushWrite();
                _readPos = 0;
                _readLen = 0;
                Seek(value, SeekOrigin.Begin);
            }
        }


        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Close"]/*' />
        public override void Close()
        {
            Dispose(true);
            GC.nativeSuppressFinalize(this);
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Dispose"]/*' />
        protected virtual void Dispose(bool disposing)
        {
            // Nothing will be done differently based on whether we are 
            // disposing vs. finalizing.
            if (_handleProtector != null) {
                if (!_handleProtector.IsClosed)
                    Flush();
                _handleProtector.Close();
            }

            _canRead = false;
            _canWrite = false;
            _canSeek = false;
            _buffer = null;
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Finalize"]/*' />
        ~FileStream()
        {
            if (_handleProtector != null) {
                BCLDebug.Correctness(_handleProtector.IsClosed, "You didn't close a FileStream & it got finalized.  Name: \""+_fileName+"\"");
                Dispose(false);
            }
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Flush"]/*' />
        public override void Flush()
        {
            if (_handleProtector.IsClosed) __Error.FileNotOpen();
            if (_writePos > 0) {
                FlushWrite();
            }
            else if (_readPos < _readLen && CanSeek) {
                FlushRead();
            }
        }

        // Reading is done by blocks from the file, but someone could read
        // 1 byte from the buffer then write.  At that point, the OS's file
        // pointer is out of sync with the stream's position.  All write 
        // functions should call this function to preserve the position in the file.
        private void FlushRead() {
            if (_readPos - _readLen != 0)
                //Seek(_readPos - _readLen, SeekOrigin.Current);
                SeekCore(_readPos - _readLen, SeekOrigin.Current);
            _readPos = 0;
            _readLen = 0;
        }

        // Writes are buffered.  Anytime the buffer fills up 
        // (_writePos + delta > _bufferSize) or the buffer switches to reading
        // and there is dirty data (_writePos > 0), this function must be called.
        private void FlushWrite() {

            WriteCore(_buffer, 0, _writePos);
            _writePos = 0;
        }


        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Handle"]/*' />
        public virtual IntPtr Handle {
            [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            [SecurityPermissionAttribute(SecurityAction.InheritanceDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
            get { 
                Flush();
                // Explicitly dump any buffered data, since the user could move our
                // position or write to the file.
                _readPos = 0;
                _readLen = 0;
                _writePos = 0;
                return _handleProtector.Handle;
            }
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.SetLength"]/*' />
        public override void SetLength(long value)
        {
            if (value < 0)
                throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (_handleProtector.IsClosed) __Error.FileNotOpen();
            if (!CanSeek) __Error.SeekNotSupported();
            if (!CanWrite) __Error.WriteNotSupported();
            // Handle buffering updates.
            if (_writePos > 0) {
                FlushWrite();
            }
            else if (_readPos < _readLen) {
                FlushRead();
            }
            if (_appendStart != -1 && value < _appendStart)
                throw new IOException(Environment.GetResourceString("IO.IO_SetLengthAppendTruncate"));
            SetLengthCore(value);
        }

        // We absolutely need this method broken out so that BeginWriteCore can call
        // a method without having to go through buffering code that might call
        // FlushWrite.
        private void SetLengthCore(long value)
        {
            BCLDebug.Assert(value >= 0, "value >= 0");
            long origPos = _pos;

            bool incremented = false;
            try {
                if (_handleProtector.TryAddRef(ref incremented)) {
                    if (_pos != value)
                        SeekCore(value, SeekOrigin.Begin);
                    if (!Win32Native.SetEndOfFile(_handleProtector.Handle)) {
                        int hr = Marshal.GetLastWin32Error();
                        if (hr==__Error.ERROR_INVALID_PARAMETER)
                            throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("ArgumentOutOfRange_FileLengthTooBig"));
                        __Error.WinIOError(hr, String.Empty);
                    }
                    // Return file pointer to where it was before setting length
                    if (origPos != value) {
                        if (origPos < value)
                            SeekCore(origPos, SeekOrigin.Begin);
                        else
                            SeekCore(0, SeekOrigin.End);
                    }
                    VerifyOSHandlePosition();
                }
                else
                    __Error.FileNotOpen();
            }
            finally {
                if (incremented) _handleProtector.Release();
            }
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Read"]/*' />
         public override int Read([In, Out] byte[] array, int offset, int count) {
            if (array==null)
                throw new ArgumentNullException("array", Environment.GetResourceString("ArgumentNull_Buffer"));
            if (offset < 0)
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (count < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (array.Length - offset < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));
            
            if (_handleProtector.IsClosed) __Error.FileNotOpen();
            
            BCLDebug.Assert((_readPos==0 && _readLen==0 && _writePos >= 0) || (_writePos==0 && _readPos <= _readLen), "We're either reading or writing, but not both.");

            bool isBlocked = false;
            int n = _readLen - _readPos;
            // if the read buffer is empty, read into either user's array or our
            // buffer, depending on number of bytes user asked for and buffer size.
            if (n == 0) {
                if (!CanRead) __Error.ReadNotSupported();
                if (_writePos > 0) FlushWrite();
                if (count >= _bufferSize) {
                    n = ReadCore(array, offset, count);
                    return n;
                }
                if (_buffer == null) _buffer = new byte[_bufferSize];
                n = ReadCore(_buffer, 0, _bufferSize);
                if (n == 0) return 0;
                isBlocked = n < _bufferSize;
                _readPos = 0;
                _readLen = n;
            }
            // Now copy min of count or numBytesAvailable (ie, near EOF) to array.
            if (n > count) n = count;
            Buffer.InternalBlockCopy(_buffer, _readPos, array, offset, n);
            _readPos += n;

            // If we hit the end of the buffer and didn't have enough bytes, we must
            // read some more from the underlying stream.  However, if we got
            // fewer bytes from the underlying stream than we asked for (ie, we're 
            // probably blocked), don't ask for more bytes.
            if (n < count && !isBlocked) {
                BCLDebug.Assert(_readPos == _readLen, "Read buffer should be empty!");                
                int moreBytesRead = ReadCore(array, offset + n, count - n);
                n += moreBytesRead;
            }

            return n;
        }

        private unsafe int ReadCore(byte[] buffer, int offset, int count) {
            BCLDebug.Assert(!_handleProtector.IsClosed, "_handle != Win32Native.INVALID_HANDLE_VALUE");
            BCLDebug.Assert(CanRead, "CanRead");
            BCLDebug.Assert(_writePos == 0, "_writePos == 0");
            int hr = 0;
            int r = ReadFileNative(_handleProtector, buffer, offset, count, null, out hr);
            if (r == -1) {
                // For pipes, ERROR_BROKEN_PIPE is the normal end of the pipe.
                if (hr == ERROR_BROKEN_PIPE) {
                    r = 0;
                }
                else {
                    if (hr == ERROR_INVALID_PARAMETER)
                        throw new ArgumentException(Environment.GetResourceString("Arg_HandleNotSync"));
                    
                    __Error.WinIOError(hr, String.Empty);
                }
            }
            BCLDebug.Assert(r >= 0, "FileStream's ReadCore is likely broken.");
            _pos += r;

            VerifyOSHandlePosition();
            return r;
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Seek"]/*' />
        public override long Seek(long offset, SeekOrigin origin) {
            if (origin<SeekOrigin.Begin || origin>SeekOrigin.End)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidSeekOrigin"));
            if (_handleProtector.IsClosed) __Error.FileNotOpen();
            if (!CanSeek) __Error.SeekNotSupported();

            BCLDebug.Assert((_readPos==0 && _readLen==0 && _writePos >= 0) || (_writePos==0 && _readPos <= _readLen), "We're either reading or writing, but not both.");

            // If we've got bytes in our buffer to write, write them out.
            // If we've read in and consumed some bytes, we'll have to adjust
            // our seek positions ONLY IF we're seeking relative to the current
            // position in the stream.  This simulates doing a seek to the new
            // position, then a read for the number of bytes we have in our buffer.
            if (_writePos > 0) {
                FlushWrite();
            }
            else if (origin == SeekOrigin.Current) {
                // Don't call FlushRead here, which would have caused an infinite
                // loop.  Simply adjust the seek origin.  This isn't necessary
                // if we're seeking relative to the beginning or end of the stream.
                offset -= (_readLen - _readPos);
            }

            VerifyOSHandlePosition();

            long oldPos = _pos + (_readPos - _readLen);
            long pos = SeekCore(offset, origin);

            // Prevent users from overwriting data in a file that was opened in
            // append mode.
            if (_appendStart != -1 && pos < _appendStart) {
                SeekCore(oldPos, SeekOrigin.Begin);
                throw new IOException(Environment.GetResourceString("IO.IO_SeekAppendOverwrite"));
            }

            // We now must update the read buffer.  We can in some cases simply
            // update _readPos within the buffer, copy around the buffer so our 
            // Position property is still correct, and avoid having to do more 
            // reads from the disk.  Otherwise, discard the buffer's contents.
            if (_readLen > 0) {
                // We can optimize the following condition:
                // oldPos - _readPos <= pos < oldPos + _readLen - _readPos
                if (oldPos == pos) {
                    if (_readPos > 0) {
                        //Console.WriteLine("Seek: seeked for 0, adjusting buffer back by: "+_readPos+"  _readLen: "+_readLen);
                        Buffer.InternalBlockCopy(_buffer, _readPos, _buffer, 0, _readLen - _readPos);
                        _readLen -= _readPos;
                        _readPos = 0;
                    }
                    // If we still have buffered data, we must update the stream's 
                    // position so our Position property is correct.
                    if (_readLen > 0)
                        SeekCore(_readLen, SeekOrigin.Current);
                }
                else if (oldPos - _readPos < pos && pos < oldPos + _readLen - _readPos) {
                    int diff = (int)(pos - oldPos);
                    //Console.WriteLine("Seek: diff was "+diff+", readpos was "+_readPos+"  adjusting buffer - shrinking by "+ (_readPos + diff));
                    Buffer.InternalBlockCopy(_buffer, _readPos+diff, _buffer, 0, _readLen - (_readPos + diff));
                    _readLen -= (_readPos + diff);
                    _readPos = 0;
                    if (_readLen > 0)
                        SeekCore(_readLen, SeekOrigin.Current);
                }
                else {
                    // Lose the read buffer.
                    _readPos = 0;
                    _readLen = 0;
                }
                BCLDebug.Assert(_readLen >= 0 && _readPos <= _readLen, "_readLen should be nonnegative, and _readPos should be less than or equal _readLen");
                BCLDebug.Assert(pos == Position, "Seek optimization: pos != Position!  Buffer math was mangled.");
            }
            return pos;
        }

        // This doesn't do argument checking.  Necessary for SetLength, which must
        // set the file pointer beyond the end of the file.
        private long SeekCore(long offset, SeekOrigin origin) {
            BCLDebug.Assert(!_handleProtector.IsClosed && CanSeek, "_handle != Win32Native.INVALID_HANDLE_VALUE && CanSeek");
            BCLDebug.Assert(origin>=SeekOrigin.Begin && origin<=SeekOrigin.End, "origin>=SeekOrigin.Begin && origin<=SeekOrigin.End");
            int hr = 0;
            long ret = 0;
            
            bool incremented = false;
            try {
                if (_handleProtector.TryAddRef(ref incremented)) {
                    ret = Win32Native.SetFilePointer(_handleProtector.Handle, offset, origin, out hr);
                }
                else
                    __Error.FileNotOpen();
            }
            finally {
                if (incremented) _handleProtector.Release();
            }

            if (ret == -1) __Error.WinIOError(hr, String.Empty);
            _pos = ret;
            return ret;
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Write"]/*' />
        public override void Write(byte[] array, int offset, int count) {
            if (array==null)
                throw new ArgumentNullException("array", Environment.GetResourceString("ArgumentNull_Buffer"));
            if (offset < 0)
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (count < 0)
                throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (array.Length - offset < count)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            if (_handleProtector.IsClosed) __Error.FileNotOpen();
            if (_writePos==0) {
                // Ensure we can write to the stream, and ready buffer for writing.
                if (!CanWrite) __Error.WriteNotSupported();
                if (_readPos < _readLen) FlushRead();
                _readPos = 0;
                _readLen = 0;
            }

            // If our buffer has data in it, copy data from the user's array into
            // the buffer, and if we can fit it all there, return.  Otherwise, write
            // the buffer to disk and copy any remaining data into our buffer.
            // The assumption here is memcpy is cheaper than disk (or net) IO.
            // (10 milliseconds to disk vs. ~20-30 microseconds for a 4K memcpy)
            // So the extra copying will reduce the total number of writes, in 
            // non-pathological cases (ie, write 1 byte, then write for the buffer 
            // size repeatedly)
            if (_writePos > 0) {
                int numBytes = _bufferSize - _writePos;   // space left in buffer
                if (numBytes > 0) {
                    if (numBytes > count)
                        numBytes = count;
                    Buffer.InternalBlockCopy(array, offset, _buffer, _writePos, numBytes);
                    _writePos += numBytes;
                    if (count==numBytes) return;
                    offset += numBytes;
                    count -= numBytes;
                }
                // Reset our buffer.  We essentially want to call FlushWrite
                // without calling Flush on the underlying Stream.
                WriteCore(_buffer, 0, _writePos);
                _writePos = 0;
            }
            // If the buffer would slow writes down, avoid buffer completely.
            if (count >= _bufferSize) {
                WriteCore(array, offset, count);
                return;
            }
            else if (count == 0)
                return;  // Don't allocate a buffer then call memcpy for 0 bytes.
            if (_buffer==null) _buffer = new byte[_bufferSize];
            // Copy remaining bytes into buffer, to write at a later date.
            Buffer.InternalBlockCopy(array, offset, _buffer, _writePos, count);
            _writePos = count;
            return;
        }

        private unsafe void WriteCore(byte[] buffer, int offset, int count) {
            BCLDebug.Assert(!_handleProtector.IsClosed, "_handle != Win32Native.INVALID_HANDLE_VALUE");
            BCLDebug.Assert(CanWrite, "CanWrite");
            BCLDebug.Assert(_readPos == _readLen, "_readPos == _readLen");
            int hr = 0;
            int r = WriteFileNative(_handleProtector, buffer, offset, count, null, out hr);
            if (r == -1) {
                // For pipes, ERROR_NO_DATA is not an error, but the pipe is closing.
                if (hr == ERROR_NO_DATA) {
                    r = 0;
                }
                else {
                    // Note that ERROR_INVALID_PARAMETER may be returned for writes
                    // where the position is too large (ie, writing at Int64.MaxValue 
                    // on Win9x) OR for synchronous writes to a handle opened 
                    // asynchronously.
                    if (hr == ERROR_INVALID_PARAMETER)
                        throw new IOException(Environment.GetResourceString("IO.IO_FileTooLongOrHandleNotSync"));
                    __Error.WinIOError(hr, String.Empty);
                }
            }
            BCLDebug.Assert(r >= 0, "FileStream's WriteCore is likely broken.");
            _pos += r;
            return;
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.BeginRead"]/*' />
        public override IAsyncResult BeginRead(byte[] array, int offset, int numBytes, AsyncCallback userCallback, Object stateObject)
        {
            if (array==null)
                throw new ArgumentNullException("array");
            if (offset < 0)
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (numBytes < 0)
                throw new ArgumentOutOfRangeException("numBytes", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (array.Length - offset < numBytes)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            if (_handleProtector.IsClosed) __Error.FileNotOpen();
            return base.BeginRead(array, offset, numBytes, userCallback, stateObject);
        }


        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.EndRead"]/*' />
        public unsafe override int EndRead(IAsyncResult asyncResult)
        {
            // Note there are 3 significantly different IAsyncResults we'll accept
            // here.  One is from Stream::BeginRead.  The other two are variations
            // on our AsyncFileStream_AsyncResult.  One is from BeginReadCore,
            // while the other is from the BeginRead buffering wrapper.
            if (asyncResult==null)
                throw new ArgumentNullException("asyncResult");

            return base.EndRead(asyncResult);
        }

        // Reads a byte from the file stream.  Returns the byte cast to an int
        // or -1 if reading from the end of the stream.
        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.ReadByte"]/*' />
        public override int ReadByte() {
            if (_handleProtector.IsClosed) __Error.FileNotOpen();
            if (_readLen==0 && !CanRead) __Error.ReadNotSupported();
            BCLDebug.Assert((_readPos==0 && _readLen==0 && _writePos >= 0) || (_writePos==0 && _readPos <= _readLen), "We're either reading or writing, but not both.");
            if (_readPos == _readLen) {
                if (_writePos > 0) FlushWrite();
                BCLDebug.Assert(_bufferSize > 0, "_bufferSize > 0");
                if (_buffer == null) _buffer = new byte[_bufferSize];
                _readLen = ReadCore(_buffer, 0, _bufferSize);
                _readPos = 0;
            }
            if (_readPos == _readLen)
                return -1;

            return _buffer[_readPos++];
        }


        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.BeginWrite"]/*' />
        public override IAsyncResult BeginWrite(byte[] array, int offset, int numBytes, AsyncCallback userCallback, Object stateObject)
        {
            if (array==null)
                throw new ArgumentNullException("array");
            if (offset < 0)
                throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (numBytes < 0)
                throw new ArgumentOutOfRangeException("numBytes", Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));
            if (array.Length - offset < numBytes)
                throw new ArgumentException(Environment.GetResourceString("Argument_InvalidOffLen"));

            if (_handleProtector.IsClosed) __Error.FileNotOpen();


           return base.BeginWrite(array, offset, numBytes, userCallback, stateObject);
        }


        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.EndWrite"]/*' />
        public unsafe override void EndWrite(IAsyncResult asyncResult)
        {
            if (asyncResult==null)
                throw new ArgumentNullException("asyncResult");

            base.EndWrite(asyncResult);
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.WriteByte"]/*' />
        public override void WriteByte(byte value)
        {
            if (_handleProtector.IsClosed) __Error.FileNotOpen();
            if (_writePos==0) {
                if (!CanWrite) __Error.WriteNotSupported();
                if (_readPos < _readLen) FlushRead();
                _readPos = 0;
                _readLen = 0;
                BCLDebug.Assert(_bufferSize > 0, "_bufferSize > 0");
                if (_buffer==null) _buffer = new byte[_bufferSize];
            }
            if (_writePos == _bufferSize)
                FlushWrite();

            _buffer[_writePos++] = value;
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Lock"]/*' />
        public virtual void Lock(long position, long length) {
            if (position < 0 || length < 0)
                throw new ArgumentOutOfRangeException((position < 0 ? "position" : "length"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));

            bool incremented = false;
            try {
                if (_handleProtector.TryAddRef(ref incremented)) {
                    if (!Win32Native.LockFile(_handleProtector.Handle, position, length))
                        __Error.WinIOError();
                }
                else
                    __Error.FileNotOpen();
            }
            finally {
                if (incremented) _handleProtector.Release();
            }
        }

        /// <include file='doc\FileStream.uex' path='docs/doc[@for="FileStream.Unlock"]/*' />
        public virtual void Unlock(long position, long length) {
            if (position < 0 || length < 0)
                throw new ArgumentOutOfRangeException((position < 0 ? "position" : "length"), Environment.GetResourceString("ArgumentOutOfRange_NeedNonNegNum"));

            bool incremented = false;
            try {
                if (_handleProtector.TryAddRef(ref incremented)) {
                    if (!Win32Native.UnlockFile(_handleProtector.Handle, position, length))
                        __Error.WinIOError();
                }
                else
                    __Error.FileNotOpen();
            }
            finally {
                if (incremented) _handleProtector.Release();
            }
        }

        // Checks the position of the OS's handle equals what we expect it to.
        private void VerifyOSHandlePosition()
        {
            if (!CanSeek)
                return;

            // This will fail if someone else moved the FileStream's handle or if
            // we've hit a bug in FileStream's position updating code.

            // Calling SeekCore(...) changes _pos, making it very difficult for 
            // debugging.
            long posVariable = _pos;
            long curPos = SeekCore(0, SeekOrigin.Current);
            if (curPos != posVariable) {
                // Just do the right thing.
                _readPos = 0;
                _readLen = 0;
            }
        }

        // Windows API definitions, from winbase.h and others
        
        private const int FILE_ATTRIBUTE_NORMAL = 0x00000080;
        private const int FILE_FLAG_OVERLAPPED = 0x40000000;
        private const int GENERIC_READ = unchecked((int)0x80000000);
        private const int GENERIC_WRITE = 0x40000000;
    
        private const int FILE_BEGIN = 0;
        private const int FILE_CURRENT = 1;
        private const int FILE_END = 2;

        // Error codes (not HRESULTS), from winerror.h
        private const int ERROR_BROKEN_PIPE = 109;
        private const int ERROR_NO_DATA = 232;
        private const int ERROR_HANDLE_EOF = 38;
        private const int ERROR_INVALID_PARAMETER = 87;
        private const int ERROR_IO_PENDING = 997;


        internal unsafe int ReadFileNative(__HandleProtector hp, byte[] bytes, int offset, int count, NativeOverlapped* overlapped, out int hr)
        {
            BCLDebug.Assert(offset >= 0, "offset >= 0");
            BCLDebug.Assert(count >= 0, "count >= 0");
            BCLDebug.Assert(bytes != null, "bytes != null");

            // Don't corrupt memory when multiple threads are erroneously writing
            // to this stream simultaneously.
            if (bytes.Length - offset < count)
                throw new IndexOutOfRangeException(Environment.GetResourceString("IndexOutOfRange_IORaceCondition"));

            // You can't use the fixed statement on an array of length 0.
            if (bytes.Length==0) {
                hr = 0;
                return 0;
            }

            int r = 0;
            int numBytesRead = 0;

            bool incremented = false;
            try {
                if (hp.TryAddRef(ref incremented)) {
                    fixed(byte* p = bytes) {
                        r = ReadFile(hp.Handle, p + offset, count, out numBytesRead, overlapped);
                    }
                }
                else
                    hr = Win32Native.ERROR_INVALID_HANDLE;  // Handle was closed.
            }
            finally {
                if (incremented) hp.Release();
            }

            if (r==0) {
                hr = Marshal.GetLastWin32Error();
                // Note: we should never silently swallow an error here without some
                // extra work.  We must make sure that BeginReadCore won't return an 
                // IAsyncResult that will cause EndRead to block, since the OS won't
                // call AsyncFSCallback for us.                                     
                if (hr == ERROR_BROKEN_PIPE) {
                    // This handle was a pipe, and it's done. Not an error, but EOF.
                    // However, note that the OS will not call AsyncFSCallback!
                    // Let the caller handle this, since BeginReadCore & ReadCore 
                    // need to do different things.
                    return -1;
                }

                // For invalid handles, detect the error and mark our handle
                // as closed to give slightly better error messages.  Also
                // help ensure we avoid handle recycling bugs.
                if (hr == Win32Native.ERROR_INVALID_HANDLE)
                    _handleProtector.ForciblyMarkAsClosed();

                return -1;
            }
            else
                hr = 0;
            return numBytesRead;
        }

        [DllImport(Microsoft.Win32.Win32Native.KERNEL32, SetLastError=true), SuppressUnmanagedCodeSecurityAttribute]
        unsafe private static extern int ReadFile(IntPtr handle, byte* bytes, int numBytesToRead, out int numBytesRead, NativeOverlapped* overlapped);

        
        internal unsafe int WriteFileNative(__HandleProtector hp, byte[] bytes, int offset, int count, NativeOverlapped* overlapped, out int hr) {
            BCLDebug.Assert(offset >= 0, "offset >= 0");
            BCLDebug.Assert(count >= 0, "count >= 0");
            BCLDebug.Assert(bytes != null, "bytes != null");

            // Don't corrupt memory when multiple threads are erroneously writing
            // to this stream simultaneously.  (Note that the OS is reading from
            // the array we pass to WriteFile, but if we read beyond the end and
            // that memory isn't allocated, we could get an AV.)
            if (bytes.Length - offset < count)
                throw new IndexOutOfRangeException(Environment.GetResourceString("IndexOutOfRange_IORaceCondition"));

            // You can't use the fixed statement on an array of length 0.
            if (bytes.Length==0) {
                hr = 0;
                return 0;
            }

            int numBytesWritten = 0;
            int r = 0;
            
            bool incremented = false;
            try {
                if (hp.TryAddRef(ref incremented)) {
                    fixed(byte* p = bytes) {
                        r = WriteFile(hp.Handle, p + offset, count, out numBytesWritten, overlapped);
                    }
                }
                else
                    hr = Win32Native.ERROR_INVALID_HANDLE;  // Handle was closed.
            }
            finally {
                if (incremented) hp.Release();
            }

            if (r==0) {
                hr = Marshal.GetLastWin32Error();
                // Note: we should never silently swallow an error here without some
                // extra work.  We must make sure that BeginWriteCore won't return an 
                // IAsyncResult that will cause EndWrite to block, since the OS won't
                // call AsyncFSCallback for us.                                     

                if (hr==ERROR_NO_DATA) {
                    // This handle was a pipe, and the pipe is being closed on the 
                    // other side.  Let the caller handle this, since BeginWriteCore 
                    // & WriteCore need to do different things.
                    return -1;
                }
                
                // For invalid handles, detect the error and mark our handle
                // as closed to give slightly better error messages.  Also
                // help ensure we avoid handle recycling bugs.
                if (hr == Win32Native.ERROR_INVALID_HANDLE)
                    _handleProtector.ForciblyMarkAsClosed();

                return -1;
            }
            else
                hr = 0;
            return numBytesWritten;          
        }

        private sealed class __FileStreamHandleProtector : __HandleProtector
        {
            private bool _ownsHandle;

            internal __FileStreamHandleProtector(IntPtr handle, bool ownsHandle) : base(handle)
            {
                _ownsHandle = ownsHandle;
            }

            protected internal override void FreeHandle(IntPtr handle)
            {
                if (_ownsHandle)
                    Win32Native.CloseHandle(handle);
            }
        }
        

        [DllImport(Microsoft.Win32.Win32Native.KERNEL32 , SetLastError=true), SuppressUnmanagedCodeSecurityAttribute]
        private static unsafe extern int WriteFile(IntPtr handle, byte* bytes, int numBytesToWrite, out int numBytesWritten, NativeOverlapped* lpOverlapped);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool RunningOnWinNTNative();

        [DllImport(Microsoft.Win32.Win32Native.KERNEL32, SetLastError=true), SuppressUnmanagedCodeSecurityAttribute]
        private static extern bool SetEvent(IntPtr eventHandle);
    }
}
