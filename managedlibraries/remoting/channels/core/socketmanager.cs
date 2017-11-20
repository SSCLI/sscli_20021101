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
//==========================================================================
//  File:       SocketManager.cs
//
//  Summary:    Class for managing a socket connection.
//
//==========================================================================

using System;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.Remoting.Messaging;
using System.Text;
using System.Threading;


namespace System.Runtime.Remoting.Channels
{
    internal delegate bool ValidateByteDelegate(byte b);


    internal abstract class SocketHandler
    {
        // socket manager data
        protected Socket NetSocket;    // network socket
        protected Stream NetStream; // network stream

        private RequestQueue _requestQueue; // request queue to use for this connection

        private byte[] _dataBuffer; // buffered data
        private int    _dataBufferSize; // size of data buffer
        private int    _dataOffset; // offset of remaining data in buffer
        private int    _dataCount;  // count of remaining bytes in buffer

        private AsyncCallback _beginReadCallback; // callback to use when doing an async read
        private IAsyncResult _beginReadAsyncResult; // async result from doing a begin read
        private WaitCallback _dataArrivedCallback; // callback to signal once data is available
        private Object _dataArrivedCallbackState; // state object to go along with callback
        
        private byte[] _byteBuffer = new byte[4]; // buffer for reading bytes


        // control cookie -
        //   The control cookie is used for synchronization when a "user"
        //   wants to retrieve this client socket manager from the socket
        //   cache.
        private int _controlCookie = 1;

                    

        // hide default constructor        
        private SocketHandler(){}

        public SocketHandler(Socket socket)
        {        
            _beginReadCallback = new AsyncCallback(this.BeginReadMessageCallback);
        
            NetSocket = socket;
            NetStream = new SocketStream(NetSocket);

            _dataBuffer = CoreChannel.BufferPool.GetBuffer();
            _dataBufferSize = _dataBuffer.Length;
            _dataOffset = 0;
            _dataCount = 0;
        } // SocketHandler

        internal SocketHandler(Socket socket, RequestQueue requestQueue) : this(socket)
        {        
            _requestQueue = requestQueue;
        } // SocketHandler



        // If this method returns true, then whoever called it can assume control
        //   of the client socket manager. If it returns false, the caller is on
        //   their honor not to do anything further with this object.
        public bool RaceForControl()
        {
            if (1 == Interlocked.Exchange(ref _controlCookie, 0))
                return true;

            return false;            
        } // RaceForControl

        public void ReleaseControl()
        {
            _controlCookie = 1;
        } // ReleaseControl


        // Determines if the remote connection is from localhost.
        internal bool IsLocal()
        {
            IPAddress remoteAddr = ((IPEndPoint)NetSocket.RemoteEndPoint).Address;
            return remoteAddr.Address == IPAddress.Loopback.Address;
        } // IsLocal
        


        // does any necessary cleanup before reading the incoming message
        protected abstract void PrepareForNewMessage();

        // allows derived classes to send an error message if the async read
        //   in BeginReadMessage fails.
        protected virtual void SendErrorMessageIfPossible(Exception e)
        {
        }     

        // allows socket handler to do something when an input stream it handed
        //   out is closed. The input stream is responsible for calling this method.
        //   (usually, only client socket handlers will do anything with this).
        //   (input stream refers to data being read off of the network)
        public virtual void OnInputStreamClosed()
        {            
        }
        

        public virtual void Close()
        {        
            if (_requestQueue != null)
                _requestQueue.ScheduleMoreWorkIfNeeded();
        
            if (NetStream != null)
            {
                NetStream.Close();
                NetStream = null;
            }
        
            if (NetSocket != null)
            {
                NetSocket.Close();
                NetSocket = null;
            }

            // return buffer to the pool
            if (_dataBuffer != null)
            {
                CoreChannel.BufferPool.ReturnBuffer(_dataBuffer);
                _dataBuffer = null;
            }
        } // Close


        public WaitCallback DataArrivedCallback
        {
            get { return _dataArrivedCallback; }
            set { _dataArrivedCallback = value; }            
        } // DataArrivedCallback

        public Object DataArrivedCallbackState
        {
            get { return _dataArrivedCallbackState; }
            set { _dataArrivedCallbackState = value; }
        } // DataArrivedCallbackState
        

        public void BeginReadMessage()
        {        
            bool bProcessNow = false;
        
            try
            {
                if (_requestQueue != null)
                    _requestQueue.ScheduleMoreWorkIfNeeded();
        
                PrepareForNewMessage();       
  
                if (_dataCount == 0)
                {
                    _beginReadAsyncResult =
                        NetStream.BeginRead(_dataBuffer, 0, _dataBufferSize, 
                                            _beginReadCallback, null);
                }
                else
                {            
                    // just queue the request if we already have some data
                    //   (note: we intentionally don't call the callback directly to avoid
                    //    overflowing the stack if we service a bunch of calls)    
                    bProcessNow = true;
                }
            }
            catch (Exception e)
            {
                CloseOnFatalError(e);
            }

            if (bProcessNow)
            {
                if (_requestQueue != null)
                    _requestQueue.ProcessNextRequest(this);
                else
                    ProcessRequestNow();
     
                _beginReadAsyncResult = null;
            }
        } // BeginReadMessage


        public void BeginReadMessageCallback(IAsyncResult ar)
        {        
            bool bProcessRequest = false;
        
            // data has been buffered; proceed to call provided callback
            try
            {
                _beginReadAsyncResult = null;  
            
                _dataOffset = 0;              
                _dataCount = NetStream.EndRead(ar);
                if (_dataCount <= 0)
                {
                    // socket has been closed
                    Close();
                }
                else
                {
                    bProcessRequest = true;
                }
            }
            catch (Exception e)
            {        
                CloseOnFatalError(e);       
            }

            if (bProcessRequest)
            {
                if (_requestQueue != null)
                    _requestQueue.ProcessNextRequest(this);
                else
                    ProcessRequestNow();
            }
        } // BeginReadMessageCallback     


        internal void CloseOnFatalError(Exception e)
        {
            try
            {
               SendErrorMessageIfPossible(e);
              
               // Something bad happened, so we should just close everything and 
               // return any buffers to the pool.
               Close();
            }
            catch
            {
                try
                {
                    Close();
                }
                catch
                {
                    // this is to prevent any weird errors with closing
                    // a socket from showing up as an unhandled exception.
                }
            }
        } // CloseOnFatalError


        // Called when the SocketHandler is pulled off the pending request queue.
        internal void ProcessRequestNow()
        {
            try
            {
                WaitCallback waitCallback = _dataArrivedCallback;
                if (waitCallback != null)
                    waitCallback.Invoke(this); 
            }
            catch (Exception e)
            {          
                CloseOnFatalError(e);                
            }            
        } // ProcessRequestNow


        internal void RejectRequestNowSinceServerIsBusy()
        {       
            CloseOnFatalError(
                new RemotingException(
                        CoreChannel.GetResourceString("Remoting_ServerIsBusy")));                 
        } // RejectRequestNow
 


        public int ReadByte()
        {
            if (Read(_byteBuffer, 0, 1) != -1)
                return _byteBuffer[0];
            else
                return -1;
        } // ReadByte

        public void WriteByte(byte value, Stream outputStream)
        {
            _byteBuffer[0] = value;
            outputStream.Write(_byteBuffer, 0, 1);
        } // WriteUInt16


        public UInt16 ReadUInt16() 
        {
            Read(_byteBuffer, 0, 2);
        
            return (UInt16)(_byteBuffer[0] & 0xFF | _byteBuffer[1] << 8);
        } // ReadUInt16
        
        public void WriteUInt16(UInt16 value, Stream outputStream)
        {
            _byteBuffer[0] = (byte)value;
            _byteBuffer[1] = (byte)(value >> 8);
            outputStream.Write(_byteBuffer, 0, 2);
        } // WriteUInt16


        public int ReadInt32() 
        {
            Read(_byteBuffer, 0, 4);
        
            return (int)((_byteBuffer[0] & 0xFF) |
                          _byteBuffer[1] << 8 |
                          _byteBuffer[2] << 16 |
                          _byteBuffer[3] << 24);
        } // ReadInt32

        public void WriteInt32(int value, Stream outputStream)
        {
            _byteBuffer[0] = (byte)value;
            _byteBuffer[1] = (byte)(value >> 8);
            _byteBuffer[2] = (byte)(value >> 16);
            _byteBuffer[3] = (byte)(value >> 24);
            outputStream.Write(_byteBuffer, 0, 4);
        } // WriteInt32


        protected bool ReadAndMatchFourBytes(byte[] buffer)
        {
            InternalRemotingServices.RemotingAssert(buffer.Length == 4, "expecting 4 byte buffer.");

            Read(_byteBuffer, 0, 4);
            
            bool bMatch = 
                (_byteBuffer[0] == buffer[0]) &&
                (_byteBuffer[1] == buffer[1]) &&
                (_byteBuffer[2] == buffer[2]) &&
                (_byteBuffer[3] == buffer[3]);

            return bMatch;
        } // ReadAndMatchFourBytes
        


        public int Read(byte[] buffer, int offset, int count)
        {
            int totalBytesRead = 0;

            // see if we have buffered data
            if (_dataCount > 0)
            {
                // copy minimum of buffered data size and bytes left to read
                int readCount = Math.Min(_dataCount, count);
                StreamHelper.BufferCopy(_dataBuffer, _dataOffset, buffer, offset, readCount);
                _dataCount -= readCount;
                _dataOffset += readCount;
                count -= readCount;
                offset += readCount;
                totalBytesRead += readCount;
            }

            // keep reading (whoever is calling this will make sure that they
            //   don't try to read too much).
            while (count > 0)
            {                
                if (count < 256)
                {
                    // if count is less than 256 bytes, I will buffer more data
                    // because it's not worth making a socket request for less.
                    BufferMoreData();

                    // copy minimum of buffered data size and bytes left to read
                    int readCount = Math.Min(_dataCount, count);
                    StreamHelper.BufferCopy(_dataBuffer, _dataOffset, buffer, offset, readCount);
                    _dataCount -= readCount;
                    _dataOffset += readCount;
                    count -= readCount;
                    offset += readCount;
                    totalBytesRead += readCount;    
                }
                else
                {
                    // just go directly to the socket
                    
                    // the internal buffer is guaranteed to be empty at this point, so just
                    //   read directly into the array given
                
                    int readCount = ReadFromSocket(buffer, offset, count);                    
                    count -= readCount;
                    offset += readCount;
                    totalBytesRead += readCount;
                }
            }
                        
            return totalBytesRead;
        } // Read


        // This should only be called when _dataCount is 0.
        private int BufferMoreData()
        {        
            InternalRemotingServices.RemotingAssert(_dataCount == 0, 
                "SocketHandler::BufferMoreData called with data still in buffer." +
                "DataCount=" + _dataCount + "; DataOffset" + _dataOffset);

            int bytesRead = ReadFromSocket(_dataBuffer, 0, _dataBufferSize);
                
            _dataOffset = 0;
            _dataCount = bytesRead;

            return bytesRead;
        } // BufferMoreData


        private int ReadFromSocket(byte[] buffer, int offset, int count)
        {
            int bytesRead = NetStream.Read(buffer, offset, count);
            if (bytesRead <= 0)
            {
                throw new RemotingException(       
                    CoreChannel.GetResourceString("Remoting_Socket_UnderlyingSocketClosed"));
            }

            return bytesRead;
        } // ReadFromSocket
        

        protected byte[] ReadToByte(byte b)
        {
            return ReadToByte(b, null);
        } /// ReadToByte

        protected byte[] ReadToByte(byte b, ValidateByteDelegate validator)
        {
            byte[] readBytes = null;

            // start at current position and return byte array consisting of bytes
            //   up to where we found the byte.
            if (_dataCount == 0)
                BufferMoreData();
                
            int dataEnd = _dataOffset + _dataCount; // one byte past last valid byte
            int startIndex = _dataOffset; // current position
            int endIndex = startIndex; // current index

            bool foundByte = false;
            bool bufferEnd;
            while (!foundByte)
            {            
                InternalRemotingServices.RemotingAssert(endIndex <= dataEnd, "endIndex shouldn't pass dataEnd");
                bufferEnd = endIndex == dataEnd;
                foundByte = !bufferEnd && (_dataBuffer[endIndex] == b);

                // validate character if necessary
                if ((validator != null) && !bufferEnd && !foundByte)
                {
                    if (!validator(_dataBuffer[endIndex]))
                    {
                        throw new RemotingException(
                            CoreChannel.GetResourceString(
                                "Remoting_Http_InvalidDataReceived"));
                    }
                }

                // we're at the end of the currently buffered data or we've found our byte
                if (bufferEnd || foundByte)
                {
                    // store processed byte in the readBytes array
                    int count = endIndex - startIndex;                                        
                    if (readBytes == null)
                    {
                        readBytes = new byte[count];
                        StreamHelper.BufferCopy(_dataBuffer, startIndex, readBytes, 0, count);
                    }
                    else
                    {
                        int oldSize = readBytes.Length;
                        byte[] newBytes = new byte[oldSize + count];
                        StreamHelper.BufferCopy(readBytes, 0, newBytes, 0, oldSize);
                        StreamHelper.BufferCopy(_dataBuffer, startIndex, newBytes, oldSize, count);
                        readBytes = newBytes;
                    }

                    // update data counters
                    _dataOffset += count;
                    _dataCount -= count;

                    if (bufferEnd)
                    {
                        // we still haven't found the byte, so buffer more data
                        //   and keep looking.
                        BufferMoreData();

                        // reset indices
                        dataEnd = _dataOffset + _dataCount; // last valid byte
                        startIndex = _dataOffset; // current position
                        endIndex = startIndex; // current index
                    }
                    else
                    if (foundByte)
                    {
                        // skip over the byte that we were looking for
                        _dataOffset += 1;
                        _dataCount -= 1;
                    }        
                }
                else
                {
                    // still haven't found character or end of buffer, so advance position
                    endIndex++;
                }
            }
                
            return readBytes;
        } // ReadToByte


        protected String ReadToChar(char ch)
        {
            return ReadToChar(ch, null);
        } // ReadToChar

        protected String ReadToChar(char ch, ValidateByteDelegate validator)
        {
            byte[] strBytes = ReadToByte((byte)ch, validator);
            if (strBytes == null)
                return null;
            if (strBytes.Length == 0)
                return String.Empty;
                
            String str = Encoding.ASCII.GetString(strBytes);

            return str;
        } // ReadToChar


        public String ReadToEndOfLine()
        {
            String str = ReadToChar('\r');
            if (ReadByte() == '\n')
                return str;
            else
                return null;
        } // ReadToEndOfLine        
               
    
    } // SocketHandler


} // namespace System.Runtime.Remoting.Channels


