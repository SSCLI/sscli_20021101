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
//============================================================
//
// File:    StreamHelper.cs
//
// Summary: Helper methods for streams.
//
//===========================================================


using System;
using System.IO;
using System.Runtime.Remoting;


namespace System.Runtime.Remoting.Channels
{

    internal class StreamHelper
    {
        internal static void CopyStream(Stream source, Stream target)
        {
            if (source == null)
                return;

            // see if this is a ChunkedMemoryStream (we can do a direct write)
            ChunkedMemoryStream chunkedMemStream = source as ChunkedMemoryStream;
            if (chunkedMemStream != null)
            {
                chunkedMemStream.WriteTo(target);
            }
            else
            {
                // see if this is a MemoryStream (we can do a direct write)
                MemoryStream memContentStream = source as MemoryStream;
                if (memContentStream != null)
                {
                    memContentStream.WriteTo(target);
                }
                else                    
                {
                    // otherwise, we need to copy the data through an intermediate buffer
                
                    byte[] buffer = CoreChannel.BufferPool.GetBuffer();
                    int bufferSize = buffer.Length;
                    int readCount = source.Read(buffer, 0, bufferSize);
                    while (readCount > 0)
                    {
                        target.Write(buffer, 0, readCount);
                        readCount = source.Read(buffer, 0, bufferSize);
                    }   
                    CoreChannel.BufferPool.ReturnBuffer(buffer);
                }
            }
            
        } // CopyStream


        internal static void BufferCopy(byte[] source, int srcOffset, 
                                        byte[] dest, int destOffset,
                                        int count)
        {
            if (count > 8)
            {
                Buffer.BlockCopy(source, srcOffset, dest, destOffset, count);
            }
            else
            {
                for (int co = 0; co < count; co++)
                    dest[destOffset + co] = source[srcOffset + co];
            }
        } // BufferCopy

        
    } // StreamHelper

} // namespace System.Runtime.Remoting.Channels
