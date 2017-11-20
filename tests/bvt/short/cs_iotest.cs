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
using System;
using System.IO;
using System.IO.IsolatedStorage;
using System.Globalization;
using System.Text;
using System.Threading;
public class IOTest 
{
    //	used by AsyncTest
    static bool		didAsyncCallbackRun;
    //	For random error checking
    internal class BogusIAsyncResult : IAsyncResult 
    {
        public bool IsCompleted				{get { return false; }}
        public WaitHandle AsyncWaitHandle	{get { return null; }}
        public Object AsyncObject			{get { return null; }}
        public Object AsyncState			{get { return null; }}
        public bool CompletedSynchronously	{get { return true; }}
    }
    public static void Error(String str)	{throw new Exception(str);}
    public static void ByteTest(Stream s) 
    {
        Console.WriteLine("  (01)  ByteTest on "+s.GetType( ).Name);
        int		size	= 0x25000;
        int		r;
        for(int i=0;i<size;i++)			s.WriteByte((byte)i);
        s.Position=0;
        for(int i=0;i<size;i++) 
        {
            r=s.ReadByte( );
            if(r!=i%256)
                throw new Exception("Didn't get the correct value for i at: "
                    +i+"  expected: "+(i%256)+"  got: "+r);
        }
    }
    public static void SpanTest(Stream s) 
    {
        Console.WriteLine("  (02)  SpanTest on "+s.GetType( ).Name);
        //	If a read request spans across two buffers, make sure we get the data
        //	back in one read request, since it would be silly to not do so. This 
        //	caught a bug in BufferedStream, which was missing a case to handle the
        //	second read request.
        //	Here's a legend.  The space is at the 8K buffer boundary.
        //	/-------------------|------- -------|------------------\ 
        //	First for loop 	  Read(bytes)	 Second for loop
        int			max;
        int			r;
        byte[]		bytes;
        int			numBytes;
        const int	bufferSize	= 8*(1<<10);
        for(int i=0;i<bufferSize*2;i++)			s.WriteByte((byte)(i % 256));
        s.Position=0;
        max = bufferSize-(bufferSize>>2);
        for(int i=0;i<max;i++) 
        {
            r=s.ReadByte( );
            if(r!=i%256)
                throw new Exception("Didn't get the correct value for i at: "
                    +i+"  expected: "+(i%256)+"  got: "+r);
        }
        bytes = new byte[bufferSize>>1];
        numBytes=s.Read(bytes,0,bytes.Length);
        if (numBytes!=bytes.Length)
            throw new Exception("While reading into a byte[] spanning a buffer starting at position "
                +max+", didn't get the right number of bytes!  asked for: "
                +bytes.Length+"  got: "+numBytes);
        for(int i=0;i<bytes.Length;i++) 
        {
            if(bytes[i]!=((i+max)%256))
                throw new Exception("When reading in an array that spans across two buffers, "+
                    "got an incorrect value at index: "+i+
                    "  expected: "+((i+max)%256)+"  got: "+bytes[i]);
        }
        //	Read rest of data
        for(int i=0;i<max;i++) 
        {
            r=s.ReadByte( );
            if(r!=(i+max+bytes.Length)%256)
                throw new Exception("While reading the last part of the buffer, "
                    +"didn't get the correct value for i at: "+i
                    +"  expected: "+((i+max+bytes.Length)%256)+"  got: "+r);
        }
    }
    public static void WriteData(Stream s) 
    {
        BinaryWriter	bw;
        int				offset	= 10;
        byte[]			bytes	= new byte[256 + offset];
        for (int i=0;i<256;i++)		bytes[i+offset] = (byte)i;
        //	Test reading & writing at an offset into the user array...
        s.Write(bytes,offset,bytes.Length-offset);
        //	Test WriteByte methods.
        s.WriteByte(0);
        s.WriteByte(65);
        s.WriteByte(255);
        s.Flush( );
        bw = new BinaryWriter(s);
        for(int i=0;i<1000;i++)								bw.Write(i);
        bw.Write(false);
        bw.Write(true);
        for(char ch='A';ch<='Z';ch++)						bw.Write(ch);
        for(short i=-32000;i<=32000;i+=1000)				bw.Write(i);
        for(int i=-2000000000;i<=2000000000;i+=100000000)	bw.Write(i);
        bw.Write(0x0123456789ABCDE7L);
        bw.Write(0x7EDCBA9876543210L);
        bw.Write("Hello world");
        bw.Write(Math.PI);
        bw.Write((float)(-2.0*Math.PI));
        bw.Flush( );
    }
    public static void ReadData(Stream s) 
    {
        int				offset		= 5;
        byte[]			bytes		= new byte[256+offset];
        int				numBytes	= s.Read(bytes,offset,256);
        int				ret;
        int				read;
        BinaryReader	br;
        if(numBytes!=256)
            throw new Exception("When reading at an offset, expected 256 bytes back!  Got: "
                +numBytes+"  s.Position: "+s.Position+"  s.Length: "+s.Length);
        for(int i=0;i<256;i++)
            if (bytes[i+offset] != i)
                throw new Exception("Error at pos: "+(i+offset)+"  expected: "
                    +i+"  got: "+bytes[i+offset]);
        //	Test ReadByte methods
        read=s.ReadByte( );
        if(read!=0)
            throw new Exception("After calling ReadByte once, expected 0, but got: "+read);
        read=s.ReadByte( );
        if(read!=65)
            throw new Exception("After calling ReadByte twice, expected 65, but got: "+read);
        read=s.ReadByte( );
        if(read!=255)
            throw new Exception("After calling ReadByte thrice, expected 255, but got: "+read);
        br = new BinaryReader(s);
        for(int i=0;i<1000;i++) 
        {
            ret=br.ReadInt32( );
            if(ret!=i)
                Error("ReadInt32 failed, i="+i+"  read: "+ret);
        }
        if(br.ReadBoolean( ))
            throw new Exception("First call ReadBoolean( ) should have returned false!");
        if(!br.ReadBoolean( ))
            throw new Exception("Second call ReadBoolean( ) should have returned true!");
        for(char ch='A';ch<='Z';ch++)
            if(br.ReadChar( )!=ch)
                Error("ReadChar failed, ch="+ch);
        for(short i=-32000;i<=32000;i+=1000)
            if(br.ReadInt16( )!=i)
                Error("ReadInt16 failed, i="+i);
        for(int i=-2000000000;i<=2000000000;i+=100000000)
            if(br.ReadInt32( )!=i)
                Error("ReadInt32 failed, i="+i);
        if(br.ReadInt64( )!=0x0123456789ABCDE7L)
            Error("ReadInt64 forwards failed");
        if(br.ReadInt64( )!=0x7EDCBA9876543210L)
            Error("ReadInt64 backwards failed");
        if(!br.ReadString( ).Equals("Hello world"))
            Error("Read string failed");
        if(br.ReadDouble( )!=Math.PI)
            Error("ReadDouble for PI failed");
        if(br.ReadSingle( )!=(float)(-2.0*Math.PI))
            Error("ReadSingle for -2PI failed");
        s=br.BaseStream;
    }
    public static void TestStream(Stream s) 
    {
        long		size;
        Console.WriteLine("  (03)  TestStream on "+s.GetType( ).Name);
        s.Position=0;
        for (int i=0;i<100;i++)		WriteData(s);
        s.Position=0;
        if(s.CanRead) 
        {
            for(int i=0;i<100;i++)		ReadData(s);
            size = s.Position/100;
            if(s.Position!=s.Length)
                throw new Exception("After reading entire stream, (Position!=Length) Position: "
                    +s.Position+"  Length: "+s.Length);
            for(int i=0;i<100;i++) 
            {
                s.Position=((99-i)*size);
                ReadData(s);
            }
            ReadWriteBufferSwitchTest(s);
            s.Position=0;
            s.SetLength(0);
            ByteTest(s);
            SpanTest(s);
            s.Position=0;
            AsyncTest(s);
        }
        ErrorTest(s);
        CanPropertiesTest(s);
        SetLengthTest(s);
        BugRegressionTest(s);
    }
    public static bool BugRegressionTest(Stream s) 
    {
        Console.WriteLine("  (04)  Bug regression tests on "+s.GetType( ).Name);
        //	Bugs are numbered in the App Server/Universal RunTime database.
        Bug31445(s);
        Bug36477(s);
        return true;
    }
    public static bool Bug31445(Stream s) 
    {
        //	Bug was about overflows when you seek to Int64.MaxValue then write 2 
        //	bytes and get a negative position.  I added the overflow checks then
        //	we later decided you couldn't seek to a position more than 1 byte
        //	beyond the end of the stream.
        try									{s.Position=Int64.MaxValue;}
        catch(IOException)					{	}
        catch(ArgumentOutOfRangeException)	{	}
        catch(IsolatedStorageException)	{	}
        return true;
    }
    public static bool Bug36477(Stream s) 
    {
        //	This caught a bug in BufferedStream::FlushRead.	It was calling Seek
        //	on BufferedStream instead of the underlying stream, passing in Current
        //	for the SeekOrigin.	This did the correction for buffered but unconsumed bytes twice.
        //	Stream stream = new BufferedStream(s);
        Stream	stream	= s;
        if(!stream.CanRead||!stream.CanWrite||!stream.CanSeek)		//	test is not applicable
            return true;
        stream.SetLength(0);
        stream.WriteByte(1);
        stream.WriteByte(2);
        stream.WriteByte(3);
        stream.Position=0;
        stream.ReadByte( );
        stream.WriteByte(2);	//	Calls FlushRead( )
        return true;
    }
    public static bool ReadWriteBufferSwitchTest(Stream s) 
    {
        String	writableStr		= "read-only";
        if(s.CanWrite)
            writableStr="read/write";
        Console.WriteLine("  (05)  ReadWriteBufferSwitch test for a "+writableStr+" "+s.GetType( ).Name);
        if(s.Length<8200)		
            Console.WriteLine("ReadWriteBufferSwitchTest assumes stream len is at least 8200");
        s.Position = 0;
        //	Assume buffer size is 8192
        for(int i=0;i<8192;i++)		s.ReadByte( );
        //		Console.WriteLine("  (05)  Pos: "+s.Position+"  s.Length: "+s.Length);
        s.WriteByte(0);
        //	Calling the position property will verify internal buffer state.
        if(8193!=s.Position)
            throw new Exception("Position should be 8193!  Position: "+s.Position);
        s.ReadByte( );
        return true;
    }
    public static bool CanPropertiesTest(Stream s) 
    {
        Console.WriteLine("  (06)  Can-Properties Test on "+s.GetType( ).Name);
        byte[]			bytes				= new byte[1];
        int				bytesTransferred;
        IAsyncResult	asyncResult;
        if(s.CanRead) 
        {			//	Ensure all Read methods work, if CanRead is true.
            int		n	= s.ReadByte( );
            if(n==-1)
                throw new Exception("On a readable stream, ReadByte returned -1...");
            bytesTransferred=s.Read(bytes,0,1);
            if(bytesTransferred!=1)
                throw new Exception("Read(byte[],0,1) should have returned 1!  got: "+bytesTransferred);
            asyncResult=s.BeginRead(bytes,0,1,null,null);									/*	BeginRead	*/ 
            bytesTransferred=s.EndRead(asyncResult);										/*	EndRead		*/ 
            if(bytesTransferred!=1)
                throw new Exception("BeginRead(byte[],0,1) should have returned 1!  got: "
                    +bytesTransferred);
        }						//	End of (s.CanRead) Block
        else 
        {					//	Begin of (!s.CanRead) Block
            try 
            {
                s.ReadByte( );
                throw new Exception("ReadByte on an unreadable stream should have thrown!");
            }
            catch(NotSupportedException)	{	}
            try 
            {
                s.Read(bytes,0,1);
                throw new Exception("Read(bytes,0,1) on an unreadable stream should have thrown!");
            }
            catch(NotSupportedException)	{	}
            try 
            {
                s.BeginRead(bytes,0,1,null,null);											/*	BeginRead	*/ 
                throw new Exception("BeginRead on an unreadable stream should have thrown!");
            }
            catch(NotSupportedException)	{	}
        }						//	End of (!s.CanRead) Block
        if(s.CanWrite) 
        {		//	Ensure write methods work if CanWrite returns true.
            s.WriteByte(0);
            s.Write(bytes,0,1);
            asyncResult = s.BeginWrite(bytes,0,1,null,null);								/*	BeginWrite	*/ 
            s.EndWrite(asyncResult);														/*	EndWrite	*/ 
        }						//	End of (s.CanWrite) Block
        else 
        {					//	Begin of (!s.CanWrite) Block
            try 
            {
                s.WriteByte(2);
                throw new Exception("WriteByte on an unreadable stream should have thrown!");
            }
            catch(NotSupportedException)	{	}
            try 
            {
                s.Write(bytes,0,1);
                throw new Exception("Write(bytes,0,1) on an unreadable stream should have thrown!");
            }
            catch(NotSupportedException)	{	}
            try 
            {
                s.BeginWrite(bytes,0,1,null,null);											/*	BeginWrite	*/ 
                throw new Exception("BeginWrite on an unreadable stream should have thrown!");
            }
            catch(NotSupportedException)	{	}
        }						//	End of (!s.CanWrite) Block
        if(s.CanSeek) 
        {			//	Ensure length-related methods work if CanSeek returns true
            long	n	= s.Length;
            n=s.Position;
            if(s.Position>s.Length)
                throw new Exception("Position is beyond the length of the stream!");
            s.Position=0;
            s.Position=s.Length;
            if(s.Position!=s.Seek(0,SeekOrigin.Current))
                throw new Exception("s.Position!=s.Seek(0,SeekOrigin.Current)");
            if(s.CanWrite)		//	Verify you can set the length, if it's writable.
                s.SetLength(s.Length);
        }						//	End of (s.CanSeek) Block
        else 
        {					//	Begin of (!s.CanSeek) Block
            try 
            {
                s.Position=5;
                throw new Exception("set_Position should throw on a non-seekable stream!");
            }
            catch(NotSupportedException)	{	}
            try 
            {
                long	n	= s.Position;
                throw new Exception("get_Position should throw on a non-seekable stream!");
            }
            catch(NotSupportedException)	{	}
            try 
            {
                long	n	= s.Length;
                throw new Exception("get_Length should throw on a non-seekable stream!");
            }
            catch(NotSupportedException)	{	}
            try 
            {
                s.SetLength(1);
                throw new Exception("SetLength should throw on a non-seekable stream!");
            }
            catch(NotSupportedException)	{	}
            try 
            {
                s.Seek(0,SeekOrigin.Current);
                throw new Exception("Seek should throw on a non-seekable stream!");
            }
            catch(NotSupportedException)	{	}
        }						//	End of (!s.CanSeek) Block
        return true;
    }
    public static void AsyncTestCallback(IAsyncResult ar) 
    {
        didAsyncCallbackRun=true;
        Console.WriteLine("  (07)  AsyncTestCallback - operation completed "
            +(ar.CompletedSynchronously?"":"a")+"synchronously");
        Object		o	= ar.AsyncState;
        if(o==null)
            throw new Exception("State object in AsyncTestCallback was null!  "
                +"Should have been String.Empty!");
        if(!Object.ReferenceEquals(o, String.Empty))
            throw new Exception("State Object in AsyncTestCallback should have been String.Empty!  "
                +"o's Type: "+o.GetType( ).Name);
    }
    public static bool AsyncTest(Stream s) 
    {
        byte[]				bigArr		= new byte[80000];
        byte[]				smArr		= new byte[128];
        int					numBytes;
        AsyncCallback		cb;
        IAsyncResult		asyncResult;
        DateTime			startWait;
        TimeSpan			thirtySecs	= new TimeSpan(0,0,30);
        bool				firstTime;
        TimeSpan			diff;
        if(s.GetType( )==typeof(IsolatedStorageFileStream)) 
        {
            Console.WriteLine("AsyncTest won't run on an IsolatedStorageFileStream "
                +"since it doesn't support async operations.");
            return true;
        }
        Console.WriteLine("  (08)  Async test on "+s.GetType( ).Name);
        if(s.Position!=0)
            throw new Exception("AsyncTest assumes stream's position will be 0 when starting.");
        for(int i=0;i<smArr.Length;i++)		smArr[i] = (byte)i;
        //	Try writing something more than 64KB so we have a chance of doing an async write.
        for(int i=0;i<bigArr.Length;i++)	bigArr[i] = (byte)((i%26)+(byte)'A');
        //	Ensure that we do run our async callback at some point.
        didAsyncCallbackRun=false;
        cb = new AsyncCallback(AsyncTestCallback);
        asyncResult=s.BeginWrite(smArr,0,smArr.Length,cb,String.Empty);						/*	BeginWrite	*/ 
        s.EndWrite(asyncResult);															/*	EndWrite	*/ 
        if(s.Position!=smArr.Length)
            throw new Exception("After first BeginWrite call, (s.Position!=smArr.Length)  got: "
                +s.Position);
        asyncResult=s.BeginWrite(bigArr,0,bigArr.Length,null,String.Empty);
        s.EndWrite(asyncResult);
        if(s.Position!=bigArr.Length+smArr.Length)
            throw new Exception("After second BeginWrite call, s.Position wasn't right!  expected: "
                +(bigArr.Length+smArr.Length)+"  got: "+s.Position);
        //	And to be sure things work, test write with an offset.
        asyncResult=s.BeginWrite(smArr,smArr.Length/2,smArr.Length/2,null,null);			/*	BeginWrite	*/ 
        s.EndWrite(asyncResult);															/*	EndWrite	*/ 
        if(s.Position!=(smArr.Length/2+bigArr.Length+smArr.Length))
            throw new Exception("After third BeginWrite call, s.Position wasn't correct! expected: "
                +(smArr.Length/2+bigArr.Length+smArr.Length)
                +"  got: "+s.Position);
        startWait=DateTime.Now;
        firstTime=true;
        while(!didAsyncCallbackRun) 
        {
            if(firstTime) 
            {
                Console.WriteLine("Waiting for async callback to be run from first BeginWrite call.");
                firstTime=false;
            }
            else
                Console.Write(".");
            Thread.Sleep(20);
            diff=DateTime.Now-startWait;
            if(diff>thirtySecs)
                throw new Exception("Async callback didn't run yet after 2 BeginWRITE calls and "
                    +"30 seconds of blocking!  "
                    +"This could be a bug in a particular stream class, "
                    +"or an extremely pathetic race condition in this test.");
        }
        didAsyncCallbackRun=false;
        s.Position=0;
        byte[]		input	= new byte[(int)s.Length];
        //	Retest running the async callback here.
        asyncResult=s.BeginRead(input,0,smArr.Length,cb,String.Empty);						/*	BeginRead	*/ 
        numBytes=s.EndRead(asyncResult);													/*	BeginRead	*/ 
        if(numBytes!=smArr.Length)
            throw new Exception("After first BeginRead call, (numBytes!=smArr.Length)  got: "
                +numBytes);
        if(s.Position!=smArr.Length)
            throw new Exception("After first BeginRead call, (s.Position!=smArr.Length)  got: "
                +s.Position);
        asyncResult=s.BeginRead(input,smArr.Length,bigArr.Length,null,String.Empty);		/*	BeginRead	*/ 
        numBytes=s.EndRead(asyncResult);													/*	EndRead		*/ 
        if(numBytes!=bigArr.Length)
            throw new Exception("After second BeginRead call, (numBytes!=bigArr.Length)  got: "
                +numBytes);
        if(s.Position!=bigArr.Length+smArr.Length)
            throw new Exception("After second BeginRead call, s.Position wasn't right!  expected: "
                +(bigArr.Length+smArr.Length)+"  got: "+s.Position);
        asyncResult=s.BeginRead(input,smArr.Length+bigArr.Length,smArr.Length/2,null,null);	/*	BeginRead	*/ 
        numBytes=s.EndRead(asyncResult);													/*	EndRead		*/ 
        if(numBytes!=smArr.Length/2)
            throw new Exception("After third BeginRead call, (numBytes!=smArr.Length/2)  got: "
                +numBytes);
        if(s.Position!=(smArr.Length/2+bigArr.Length+smArr.Length))
            throw new Exception("After third BeginRead call, s.Position wasn't correct! expected: "
                +(smArr.Length/2+bigArr.Length+smArr.Length)
                +"  got: "+s.Position);
        for(int i=0;i<smArr.Length;i++)
            if (smArr[i]!=input[i])
                throw new Exception("When reading first smArr copy, position "
                    +i+" was wrong!  got: "
                    +input[i]+"  expected: "+smArr[i]);
        int		offset	= smArr.Length;
        for(int i=0;i<bigArr.Length;i++)
            if (bigArr[i]!=input[i+offset])
                throw new Exception("When reading bigArr copy, position "
                    +(i+offset)+" was wrong! i: "+i+"  got: "
                    +input[i+offset]+"  expected: "+bigArr[i]);
        offset=smArr.Length+bigArr.Length;
        for(int i=0;i<smArr.Length/2;i++)
            if (smArr[i+smArr.Length/2]!=input[i+offset])
                throw new Exception("When reading second smArr copy, position "
                    +(i+offset)+" was wrong! i: "+i+"  got: "
                    +input[i+offset]+"  expected: "
                    +smArr[i+smArr.Length/2]);
        startWait=DateTime.Now;
        firstTime=true;
        while(!didAsyncCallbackRun) 
        {
            if (firstTime) 
            {
                Console.WriteLine("Waiting for async callback to be run from "
                    +"first BeginRead call.");
                firstTime=false;
            }
            else
                Console.Write(".");
            Thread.Sleep(20);
            diff=DateTime.Now-startWait;
            if(diff>thirtySecs)
                throw new Exception("Async callback didn't run yet after 2 BeginREAD "
                    +"calls and 30 seconds of blocking!  "
                    +"This could be a bug in a particular stream "
                    +"class, or an extremely pathetic race "
                    +"condition in this test.");
        }
        didAsyncCallbackRun=false;
        return true;
    }
    public static bool ErrorTest(Stream s) 
    {
        Console.WriteLine("  (09)  Error test on stream: "+s.GetType( ).Name);
        //	Test EndRead & EndWrite's Type safety
        byte[]				bytes		= new byte[0];
        IAsyncResult		asyncResult;
        BogusIAsyncResult	bogus		= new BogusIAsyncResult( );
        if(s.CanRead) 
        {
            asyncResult = s.BeginRead(bytes,0,0,null,null);									/*	BeginRead	*/ 
            try 
            {
                s.EndWrite(asyncResult);													/*	EndWrite	*/ 
                throw new Exception("EndWrite with an asyncResult from BeginRead should have thrown!");
            }
            catch(ArgumentException)	{	}
        }
        if(s.CanWrite) 
        {
            asyncResult=s.BeginWrite(bytes,0,0,null,null);									/*	BeginWrite	*/ 
            try 
            {
                s.EndRead(asyncResult);														/*	EndRead		*/ 
                throw new Exception("EndRead with an asyncResult from BeginWrite should have thrown!");
            }
            catch(ArgumentException)	{	}
            //	Verify EndWrite doesn't allow using the same asyncResult twice.
            s.EndWrite(asyncResult);														/*	EndWrite	*/ 
            try 
            {
                s.EndWrite(asyncResult);													/*	EndWrite	*/ 
                throw new Exception("Exception EndWrite was called twice w/ same IAsyncResult from "
                    +s.GetType( ).Name+", but didn't throw!");
            }
            catch(InvalidOperationException)	{	}
        }
        try 
        {
            s.EndRead(bogus);																/*	EndRead		*/ 
            throw new Exception("EndRead with a bogus IAsyncResult object should have thrown!");
        }
        catch(ArgumentException)	{	}
        try 
        {
            s.EndWrite(bogus);																/*	EndWrite	*/ 
            throw new Exception("EndWrite with a bogus IAsyncResult object should have thrown!");
        }
        catch(ArgumentException)	{	}
        return true;
    }
    public static bool SetLengthTest(Stream s) 
    {
        Console.WriteLine("  (10)  SetLengthTest on "+s.GetType( ).Name);
        if(!s.CanSeek) 
        {
            Console.WriteLine("SetLengthTest shouldn't run on non-seekable stream "+s.GetType( ).Name);
            try 
            {
                s.SetLength(0);
                throw new Exception("SetLength to 0 on a non-seekable stream should have failed!");
            }
            catch(NotSupportedException)	{	}
            return true;
        }
        if(!s.CanWrite) 
        {
            Console.WriteLine("SetLengthTest shouldn't run on non-writable stream "+s.GetType( ).Name);
            try 
            {
                s.SetLength(0);
                throw new Exception("SetLength to 0 on a non-writable stream should have failed!  "
                    +"s.Length: "+s.Length);
            }
            catch(NotSupportedException)	{	}
            return true;
        }
        s.SetLength(0);
        if(s.Length!=0)
            throw new Exception("SetLength to 0, but Length is: "+s.Length);
        if(s.Position!=0)
            throw new Exception("Set length to 0.  Position should be zero too: "+s.Position);
        s.SetLength(10);
        if(s.Length!=10)
            throw new Exception("SetLength to 10, but Length is: "+s.Length);
        if(s.Position!=0)
            throw new Exception("Set length to 10, yet Position should be zero still: "+s.Position);
        if(s.CanRead) 
        {
            byte[]			bytes			= new byte[500];
            IAsyncResult	asyncResult		= s.BeginRead(bytes,0,500,null,null);
            int				numBytes		= s.EndRead(asyncResult);
            if(numBytes!=10)
                throw new Exception("Number of bytes got back from EndRead was wrong!  "
                    +"should have been 10, but got: "+numBytes);
            if(s.Position!=10)
                throw new Exception("After async read, position should be 10, but was: "+s.Position);
        }
        return true;
    }
    public static void Main(String[] args) 
    {
        String			root	= "";
        String			dir		= "";
        Boolean			r		= true;
        Environment.ExitCode=1;			//	1 for fail, 11 for pass.
        Console.WriteLine("Simple IO test");
        try 
        {
            Stream						stream;
            Stream						s;
            MemoryStream				ms;
            s=File.Create(root+dir+"test.bin");
            TestStream(s);
            s.Close( );
            s=null;
            File.Delete(root+dir+"test.bin");
            //	Test a purely async file stream.
            s = new FileStream(root+dir+"async.bin",
                FileMode.Create,FileAccess.ReadWrite,FileShare.None,4096,true);
            TestStream(s);
            s.Close( );
            s=null;
            File.Delete(root+dir+"async.bin");
            //	Write-only FileStream...
            stream = new FileStream(root+dir+"writeOnly.bin",
                FileMode.Create,FileAccess.Write,FileShare.None);
            TestStream(stream);
            stream.Close( );
            File.Delete(root+dir+"writeOnly.bin");
            ms = new MemoryStream( );
            TestStream(ms);
            //	Test a buffered MemoryStream.
            //			Console.WriteLine("testing a BufferedStream around a MemoryStream");
            s = new BufferedStream(new MemoryStream( ));
            TestStream(s);
            Console.WriteLine("done");
            Console.WriteLine("----------------------------------------------------------------");

        }
        catch(Exception e) 
        {
            r = false;
            Console.WriteLine( );
            Console.WriteLine("IOTest caught exception: {0}",e.ToString( ));
        }
        if (r) 
        {
            Console.WriteLine( );
            Console.WriteLine("paSs");
            Environment.ExitCode=0;
        }
        else 
        {
            Console.WriteLine( );
            Console.WriteLine("FAiL");
            Environment.ExitCode=1;
        }
    }
}
