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
using System.Collections;
using System.Globalization;
using System.Text;
using System.Threading;
public class Co5730_fm_fa_fs_i_b
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "FileStream(String, FileMode, FileAccess, FileShare, IASyncResult)";
    public static String s_strTFName        = "Co5730_fm_fa_fs_i_b.cs";
    public static String s_strTFAbbrev      = s_strTFName.Substring(0,6);
    public static String s_strTFPath        = Environment.CurrentDirectory;
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
        String strLoc = "Loc_000oo";
        String strValue = String.Empty;
        int iCountErrors = 0;
        int iCountTestcases = 0;
        try
        {
            String filName = s_strTFAbbrev+"TestFile";
            Stream fs2;
            IAsyncResult iar;
            Int32 iReturn;
            Byte[] bArr;
            Byte[] bRead;
            if(File.Exists(filName)) 
                File.Delete(filName);
            strLoc = "Loc_100aa";
            iCountTestcases++;
            try 
            {
                fs2 = new FileStream(null, FileMode.Create, FileAccess.Read, FileShare.None, 100, false);
                iCountErrors++;
                printerr( "Error_100bb! Expected exception not thrown");
                fs2.Close();
            } 
            catch (ArgumentNullException aexc) 
            {
                printinfo( "Info_100cc! Caught expected exception, aexc=="+aexc.Message);
            } 
            catch (Exception exc) 
            {
                iCountErrors++;
                printerr( "Error_100dd Incorrect exception thrown, exc=="+exc.ToString());
            }	
            strLoc = "Loc_300aa";
            fs2 = new FileStream(filName, FileMode.Create, FileAccess.ReadWrite, FileShare.None, 100, false);
            bArr = new Byte[1024*100];
            for(int i = 0 ; i < bArr.Length ; i++)
                bArr[i] = (Byte)i;
            fs2.Write(bArr, 0, bArr.Length);
            fs2.Close();
            bRead = new Byte[bArr.Length];
            fs2 = new FileStream(filName, FileMode.Open);
            iar = fs2.BeginRead(bRead, 0, bRead.Length, null, null);
            iReturn = fs2.EndRead(iar);
            iCountTestcases++;
            if(!iar.IsCompleted) 
            {
                iCountErrors++;
                printerr( " Error_300dd! Operation should be complete");
            }
            iCountTestcases++;
            if(!iar.CompletedSynchronously) 
            {
                iCountErrors++;
                printerr( "Error_300cc! Unexpectedly completed ASync");
            }
            iCountTestcases++;
            if(iReturn != bArr.Length) 
            {
                iCountErrors++;
                printerr( "Error_300bb! Expected=="+bArr.Length+", Return=="+iReturn);
            }
            for(int i = 0 ; i < bRead.Length ; i++) 
            {
                iCountTestcases++;
                if(bRead[i] != bArr[i]) 
                {
                    printerr( "Error_300ff_"+i+"! Expected=="+bArr[i]+", got=="+bRead[i]);
                    iCountErrors++;
                }
            }
            fs2.Close();
            strLoc = "Loc_400aa";
            fs2 = new FileStream(filName, FileMode.Create, FileAccess.ReadWrite, FileShare.None, 100, true);
            bArr = new Byte[100*1024];
            for(int i = 0 ; i < bArr.Length ; i++)
                bArr[i] = (Byte)i;
            fs2.Write(bArr, 0, bArr.Length);
            fs2.Close();
            bRead = new Byte[bArr.Length];
            fs2 = new FileStream(filName, FileMode.Open);
            iar = fs2.BeginRead(bRead, 0, bRead.Length, null, null);
            iReturn = fs2.EndRead(iar);
            iCountTestcases++;
            if(!iar.IsCompleted) 
            {
                iCountErrors++;
                printerr( "Error_400bb! Operation should be complete");
            } 
            iCountTestcases++;
            if(!iar.CompletedSynchronously) 
            {
                iCountErrors++;
                printerr( "Error_400cc! Unexpectedly completed Synchronously");
            } 
            iCountTestcases++;
            if(iReturn != bArr.Length) 
            {
                iCountErrors++;
                printerr( "Error_400dd! Expected=="+bArr.Length+", Return=="+iReturn);
            }
            for(int i = 0 ; i < bRead.Length ; i++) 
            {
                iCountTestcases++;
                if(bRead[i] != bArr[i]) 
                {
                    printerr( "Error_300ff_"+i+"! Expected=="+bArr[i]+", got=="+bRead[i]);
                    iCountErrors++;
                }
            }
            fs2.Close();
            if(File.Exists(filName))
                File.Delete(filName);
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
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co5730_fm_fa_fs_i_b cbA = new Co5730_fm_fa_fs_i_b();
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
            Console.WriteLine ("Path: "+s_strTFPath+"\\"+s_strTFName);
            Console.WriteLine( " " );
            Console.WriteLine( "FAiL!  "+ s_strTFAbbrev);
            Console.WriteLine( " " );
        }
        if (bResult) Environment.ExitCode = 0; else Environment.ExitCode = 1;
    }
}
