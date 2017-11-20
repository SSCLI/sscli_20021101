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
public class Co9302ctor_stream_b
{
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "StreamReader(Stream)";
    public static String s_strTFName        = "Co9302ctor_stream_b.cs";
    public static String s_strTFAbbrev      = "Co9302";
    public static String s_strTFPath        = Environment.CurrentDirectory;
    public static int iPortNumber           = 0;
    private static int iCountErrors         = 0;
    public static ManualResetEvent m_PortSetEvent = new ManualResetEvent(false);

    private static string strValue ="Some string to test the network streams" ;
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
        int iCountErrors = 0;
        int iCountTestcases = 0;
        String strLoc = "Loc_000oo";
        String strValue = String.Empty;
        try
        {
            MemoryStream ms2;
            FileStream fs2;
            StreamReader sr2;
            if(File.Exists("Co9302Test.tmp"))
                File.Delete("Co9302Test.tmp");			
            strLoc = "Loc_1989x";
            iCountTestcases++;
            try 
            {
                sr2 = new StreamReader((Stream)null , true );
                iCountErrors++;
                printerr( "Error_298vb! Expected exception not thrown");
            } 
            catch (ArgumentNullException aexc) 
            {
                printinfo("Info_7887g! Caught expected exception, aexc=="+aexc.Message);
            } 
            catch (Exception exc) 
            {
                iCountErrors++;
                printerr( "Error_t98bh! Incorrect exception thrown, exc=="+exc.ToString());
            }
            strLoc = "Loc_g37jb";
            fs2 = new FileStream("Co9302Test.tmp", FileMode.Create, FileAccess.Write);
            iCountTestcases++;
            try 
            {
                sr2 = new StreamReader(fs2, false);
                iCountErrors++;
                printerr("Error_07t8b! Expected exception not thrown");
            } 
            catch (ArgumentException aexc) 
            {
                printinfo("Info_09877! Caught expected exception, aexc=="+aexc.Message);
            } 
            catch (Exception exc) 
            {
                iCountErrors++;
                printerr( "Error_9t798! Incorrect exception thrown, exc=="+exc.ToString());
            }
            fs2.Close();
            strLoc = "Loc_gfu8g";
            ms2 = new MemoryStream();
            ms2.Close();
            iCountTestcases++;
            try 
            {
                sr2 = new StreamReader(ms2, false);
                iCountErrors++;
                printerr( "Error_9578b! Expected exception not thrown");
            } 
            catch (ArgumentException aexc) 
            {
                printinfo("Info_6t877! Caught expected exception, aexc=="+aexc.Message);
            } 
            catch (Exception exc) 
            {
                iCountErrors++;
                printerr( "Error_7698b! Incorrect exception thrown, exc=="+exc.ToString());
            }
            strLoc = "Loc_787yg";
            ms2 = new MemoryStream();
            ms2.Write(new Byte[]{65,66,67,68}, 0, 4);
            ms2.Position = 0;
            sr2 = new StreamReader(ms2, false);
            iCountTestcases++;
            if(!sr2.ReadToEnd().Equals("ABCD")) 
            {
                iCountErrors++;
                printerr( "Error_6t897! Incorrect string read");
            }
            sr2.Close();
            strLoc = "Loc_787yg";
            ms2 = new MemoryStream();
            ms2.Write(new Byte[]{65,66,67,68}, 0, 4);
            ms2.Position = 0;
            sr2 = new StreamReader(ms2, false);
            iCountTestcases++;
            if(!sr2.ReadToEnd().Equals("ABCD")) 
            {
                iCountErrors++;
                printerr( "Error_6t897! Incorrect string read");
            }
            sr2.Close();
            fs2 = new FileStream("Co9302Test.tmp", FileMode.Create);
            fs2.Write(new Byte[]{65,66,67,68},0,4);
            fs2.Position = 0;
            sr2 = new StreamReader(fs2, false);
            iCountTestcases++;
            if(!sr2.ReadToEnd().Equals("ABCD")) 
            {
                iCountErrors++;
                printerr( "Error_9688y! Incorrect string read");
            }
            fs2.Close();				
            fs2 = new FileStream("Co9302Test.tmp", FileMode.Create);
            fs2.Write(new Byte[]{65,66,67,68},0,4);
            fs2.Position = 0;
            sr2 = new StreamReader(fs2, true);
            iCountTestcases++;
            if(!sr2.ReadToEnd().Equals("ABCD")) 
            {
                iCountErrors++;
                printerr( "Error_9688y! Incorrect string read");
            }
            fs2.Close();				
            iCountTestcases++;
            m_PortSetEvent.Reset();
            Thread tcpListenerThread = new Thread(new ThreadStart(Co9302ctor_stream_b.StartListeningTcp));
            tcpListenerThread.Start();
            Console.WriteLine("Listening");
            Thread.Sleep( 1000 );
            m_PortSetEvent.WaitOne();
            Teleport("127.0.0.1");
            Thread.Sleep( 1000 );
            if(File.Exists("Co9302Test.tmp"))
                File.Delete("Co9302Test.tmp");
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
            Console.WriteLine("FAiL! "+s_strTFName+" ,iCountErrors=="+iCountErrors.ToString() );
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
        StreamReader sr = null;

        try 
        {
            listener.Start();
            IPEndPoint ipe = (IPEndPoint) listener.LocalEndpoint;
            Interlocked.Exchange(ref iPortNumber, ipe.Port);
            Console.WriteLine("Using port: {0}", iPortNumber);
            m_PortSetEvent.Set();

            Socket s = listener.AcceptSocket();
            ns = new NetworkStream(s);
            sr = new StreamReader(ns , false);
            try 
            {
                String text = sr.ReadLine();
                if(text != strValue) 
                {
                    iCountErrors++;
                    Console.WriteLine( "Error_54543! Incorrect string read " + strValue);
                } 
            } 
            catch (Exception exc) 
            {
                iCountErrors++;
                Console.WriteLine( "Error_5555! Incorrect exception thrown, exc=="+exc.ToString());
            }
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
            if(sr != null)
            {
                sr.Close();
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
        StreamWriter sw = new StreamWriter(stream);
        sw.Write( strValue);
        sw.Flush();
        sw.Close();
    }
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co9302ctor_stream_b cbA = new Co9302ctor_stream_b();
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
