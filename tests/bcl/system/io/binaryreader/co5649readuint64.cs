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
using System.Text;
using System.Threading;
using System.Net.Sockets;
using System.Net;
public class Co5649ReadUInt64
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "BinaryWriter.Write(UInt64)";
    public static String s_strTFName        = "Co5649ReadUInt64.cs";
    public static String s_strTFAbbrev      = "Co5649";
    public static String s_strTFPath        = Environment.CurrentDirectory.ToString();
    public static int iPortNumber           = 0;
    private static int iCountErrors         = 0;
    public static ManualResetEvent m_PortSetEvent = new ManualResetEvent(false);

    private static UInt64[] ui64Arr = new UInt64[] {
                                                       UInt64.MinValue
                                                       ,UInt64.MaxValue
                                                       ,0
                                                       ,100
                                                       ,1000
                                                       ,10000
                                                       ,UInt64.MaxValue-100
                                                   };
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
        int iCountTestcases = 0;
        String strLoc = "Loc_000oo";
        String strValue = String.Empty;
        try
        {
            BinaryWriter dw2 = null;
            Stream fs2 = null;
            BinaryReader dr2 = null;
            FileInfo fil2 = null;		    
            MemoryStream mstr = null;
            UInt64 ui64a = 0;
            int ii = 0;
            if(File.Exists("Co5649Test.tmp"))
                File.Delete("Co5649Test.tmp");
            strLoc = "Loc_8yfv7";
            fil2 = new FileInfo("Co5649Test.tmp");
            fs2 = fil2.Open(FileMode.Create);
            dw2 = new BinaryWriter(fs2);
            try 
            {
                for(ii = 0 ; ii < ui64Arr.Length ; ii++)
                    dw2.Write(ui64Arr[ii]);		   
                dw2.Flush();
                fs2.Close();
                strLoc = "Loc_987hg";
                fs2 = fil2.Open(FileMode.Open);
                dr2 = new BinaryReader(fs2);
                for(ii = 0 ; ii < ui64Arr.Length ;ii++) 
                {
                    iCountTestcases++;
                    if((ui64a = dr2.ReadUInt64()) != ui64Arr[ii]) 
                    {
                        iCountErrors++;
                        printerr( "Error_298hg_"+ii+"! Expected=="+ui64Arr[ii]+" , got=="+ui64a);
                    }
                }
                iCountTestcases++;
                try 
                {
                    ui64a = dr2.ReadUInt64();
                    iCountErrors++;
                    printerr( "Error_2389! Expected exception not thrown, ui64a=="+ui64a);
                } 
                catch (EndOfStreamException) 
                {
                } 
                catch (Exception exc) 
                {
                    iCountErrors++;
                    printerr( "Error_6498h! Unexpected exception thrown, exc=="+exc.ToString());
                }
            } 
            catch (Exception exc) 
            {
                iCountErrors++;
                printerr( "Error_278gy! Unexpected exception, exc=="+exc.ToString());
            }
            fs2.Close();
            fil2.Delete();
            strLoc = "Loc_98yss";
            mstr = new MemoryStream();
            dw2 = new BinaryWriter(mstr);
            try 
            {
                for(ii = 0 ; ii < ui64Arr.Length ; ii++) 
                    dw2.Write(ui64Arr[ii]);
                dw2.Flush();
                mstr.Position = 0;
                strLoc = "Loc_287y5";
                dr2 = new BinaryReader(mstr);
                for(ii = 0 ; ii < ui64Arr.Length ;ii++) 
                {
                    iCountTestcases++;
                    if((ui64a = dr2.ReadUInt64()) != ui64Arr[ii]) 
                    {
                        iCountErrors++;
                        printerr( "Error_48yf4_"+ii+"! Expected=="+ui64Arr[ii]+" , got=="+ui64a);
                    }
                }
                iCountTestcases++;
                try 
                {
                    ui64a = dr2.ReadUInt64();
                    iCountErrors++;
                    printerr( "Error_2d847! Expected exception not thrown, ui64a=="+ui64a);
                } 
                catch (EndOfStreamException) 
                {
                } 
                catch (Exception exc) 
                {
                    iCountErrors++;
                    printerr( "Error_238gy! Unexpected exception thrown, exc=="+exc.ToString());
                }
            } 
            catch (Exception exc) 
            {
                iCountErrors++;
                printerr( "Error_38f85! Unexpected exception, exc=="+exc.ToString());
            }
            mstr.Close();
            m_PortSetEvent.Reset();
            Thread tcpListenerThread = new Thread(new ThreadStart(Co5649ReadUInt64.StartListeningTcp));
            tcpListenerThread.Start();
            Console.WriteLine("Listening");
            Thread.Sleep( 1000 );
            m_PortSetEvent.WaitOne();
            Teleport("127.0.0.1");
            Thread.Sleep( 1000 );
            if(File.Exists("Co5649Test.tmp"))
                File.Delete("Co5649Test.tmp");
        } 
        catch (Exception exc_general ) 
        {
            ++iCountErrors;
            Console.WriteLine (s_strTFAbbrev + " : Error Err_8888yyy!  strLoc=="+ strLoc +", exc_general=="+exc_general.ToString());
        }
        if ( iCountErrors == 0 )
        {
            Console.WriteLine( "paSs. "+s_strTFName+" ,iCountTestcases=="+iCountTestcases.ToString());
            return true;
        }
        else
        {
            Console.WriteLine("FAiL! "+s_strTFName+" ,iCountErrors=="+iCountErrors.ToString()+" , BugNums?: "+s_strActiveBugNums );
            return false;
        }
    }
    public void printerr ( String err )
    {
        Console.WriteLine ("POINTTOBREAK: ("+ s_strTFAbbrev + ") "+ err);
    }
    public void printinfo ( String info )
    {
        Console.WriteLine ("INFO: ("+ s_strTFAbbrev + ") "+ info);
    }
    public static void StartListeningTcp() 
    {
        TcpThreadListener listener = new TcpThreadListener(0);
        NetworkStream ns = null;
        BinaryReader br = null;

        try 
        {
            listener.Start();
            IPEndPoint ipe = (IPEndPoint) listener.LocalEndpoint;
            Interlocked.Exchange(ref iPortNumber, ipe.Port);
            Console.WriteLine("Using port: {0}", iPortNumber);
            m_PortSetEvent.Set();

            Socket s = listener.AcceptSocket();
            ns = new NetworkStream(s);
            br = new BinaryReader( ns );   
            UInt64 iTemp ;
            for(int i = 0 ; i < ui64Arr.Length ; i++) 
            {
                iTemp = br.ReadUInt64();
                if(iTemp != ui64Arr[i]) 
                {
                    iCountErrors++;
                    Console.WriteLine( "Error_5453c_"+i+"! Expected=="+ui64Arr[i]+", got=="+iTemp);
                }
            }
            Console.WriteLine("We are done with the listening");
            ns.Close();
        }
        catch(Exception e) 
        {
            iCountErrors++ ;
            Console.WriteLine("Exception receiving Teleportation: " + e.Message + Environment.NewLine + e.StackTrace);
            m_PortSetEvent.Set();
        }
        finally
        {
            if (listener != null)
            {
                listener.Stop();
            }
            if (ns != null)
            {
                ns.Close();
            }
            if(br != null)
            {
                br.Close();
            }
        } //finally
    }
    internal class TcpThreadListener : TcpListener 
    {
        public TcpThreadListener(int port) : base(port) 
        {
        }
        internal void Shutdown() 
        {
            if ( Server != null )
                Server.Close();
        }	
    }
    public static void Teleport(string address) 
    {
        TcpClient tcpClient = new TcpClient();
        IPEndPoint epSend = null;
        IPAddress sendAddress = IPAddress.Parse(address);			
        epSend = new IPEndPoint(sendAddress, iPortNumber);
        tcpClient.Connect(epSend);
        Stream stream = tcpClient.GetStream();
        BinaryWriter bw = new BinaryWriter( stream );
        for(int i = 0 ; i < ui64Arr.Length ; i++)
            bw.Write(ui64Arr[i]);
    }
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co5649ReadUInt64 cbA = new Co5649ReadUInt64();
        try 
        {
            bResult = cbA.runTest();
        } 
        catch (Exception exc_main)
        {
            bResult = false;
            Console.WriteLine(s_strTFAbbrev + " : FAiL! Error Err_9999zzz! Uncaught Exception in main(), exc_main=="+exc_main.ToString());
        }
        if (!bResult)
        {
            Console.WriteLine ("Path: "+s_strTFName + s_strTFPath);
            Console.WriteLine( " " );
            Console.WriteLine( "FAiL!  "+ s_strTFAbbrev);
            Console.WriteLine( " " );
        }
        if (bResult) Environment.ExitCode = 0; else Environment.ExitCode = 1;
    }
}
