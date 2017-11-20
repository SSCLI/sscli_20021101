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
public class Co5757set_Capacity
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "MemoryStream.Capacity";
    public static String s_strTFName        = "Co5757set_Capacity.cs";
    public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
    public static String s_strTFPath        = Environment.CurrentDirectory;
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
            strLoc = "Loc_00001";
            ms2 = new MemoryStream();
            iCountTestcases++;
            try 
            {
                ms2.Capacity = -3;
                iCountErrors++;
                printerr( "Error_00002! Expected exception not thrown, capacity=="+ms2.Capacity);
            } 
            catch (ArgumentOutOfRangeException aexc) 
            {
                printinfo( "Info_00003! Caught expected exception, aexc=="+aexc.Message);
            } 
            catch (Exception exc) 
            {
                iCountErrors++;
                printerr( "Error_00004! Incorrect exception thrown, exc=="+exc.ToString());
            }
            ms2.Close();
            strLoc = "Loc_00005";
            ms2 = new MemoryStream();
            ms2.Write(new Byte[]{1,2,3}, 0, 3);
            ms2.Flush();
            iCountTestcases++;
            try 
            {
                ms2.Capacity = 1;
                iCountErrors++;
                printerr( "Error_00006! Expected exception not thrown, capacity=="+ms2.Capacity);
            } 
            catch ( ArgumentOutOfRangeException iexc) 
            {
                printinfo( "Info_00007! Caught expected exception, iexc=="+iexc.Message);
            } 
            catch ( Exception exc) 
            {
                iCountErrors++;
                printerr( "Error_00008! Incorrect exception thrown, exc=="+exc.ToString());
            } 
            ms2.Close();
            strLoc = "Loc_00009";
            ms2 = new MemoryStream();
            ms2.Close();
            iCountTestcases++;
            try 
            {
                ms2.Capacity = 5;
                iCountErrors++;
                printerr( "Error_00010! Expected exception not thrown, capacity=="+ms2.Capacity);
            } 
            catch (ObjectDisposedException iexc) 
            {
                printinfo( "Info_00011! Caught expected exception, iexc=="+iexc.Message);
            } 
            catch (Exception exc) 
            {
                iCountErrors++;
                printerr( "Error_00012! Incorrect exception thrown, exc=="+exc.ToString());
            }
            strLoc = "Loc_00013";
            ms2 = new MemoryStream(new Byte[]{1,2,3}, 0, 3, false);
            iCountTestcases++;
            try 
            {
                ms2.Capacity = 5;
                iCountErrors++;
                printerr( "Error_00014! Expected exception not thrown, capacity=="+ms2.Capacity);
            } 
            catch (NotSupportedException iexc) 
            {
                printinfo( "Info_00015! Caught expected exception, iexc=="+iexc.Message);
            } 
            catch ( Exception exc) 
            {
                iCountErrors++;
                printerr( "Error_00016! Incorrect exception thrown, exc=="+exc.ToString());
            }
            ms2.Close();
            strLoc = "Loc_00017";
            ms2 = new MemoryStream();
            iCountTestcases++;
            if(ms2.Capacity != 0) 
            {
                iCountErrors++;
                printerr( "Error_00018! Expected==3, got=="+ms2.Capacity);
            }
            ms2.Capacity = 1000;
            iCountTestcases++;
            if(ms2.Capacity != 1000) 
            {
                iCountErrors++;
                printerr( "Error_00019! Expected==1000, got=="+ms2.Capacity);
            } 
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
        Co5757set_Capacity cbA = new Co5757set_Capacity();
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
