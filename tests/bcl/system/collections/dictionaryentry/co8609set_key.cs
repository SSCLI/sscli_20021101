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
using System.Collections;
using System.Globalization;
using System.IO;
public class Co8609set_Key
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "DictionaryEntry.Key";
    public static String s_strTFName        = "Co8609set_Key.cs";
    public static String s_strTFAbbrev      = s_strTFName.Substring(0,6);
    public static String s_strTFPath        = Environment.CurrentDirectory;
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
        String strLoc = "Loc_000oo";
        String strValue = String.Empty;
        int iCountErrors = 0;
        int iCountTestcases = 0;
        DictionaryEntry entry;
        String strKey;
        try
        {
            strLoc = "Loc_384sdg";
            iCountTestcases++;
            entry = new DictionaryEntry();
            strKey = "Hello";
            entry.Key = strKey;
            if((String)entry.Key!=strKey)
            {
                iCountErrors++;
                Console.WriteLine( "Err_93745sdg! wrong value returned");
            }
            strLoc = "Loc_32497fxgb";
            iCountTestcases++;
            entry = new DictionaryEntry("What", "ever");
            strKey = "Hello";
            entry.Key = strKey;
            if((String)entry.Key!=strKey)
            {
                iCountErrors++;
                Console.WriteLine( "Err_89745sg! wrong value returned");
            }
            strLoc = "Loc_32497fxgb";
            iCountTestcases++;
            entry = new DictionaryEntry(5, 6);
            strKey = "Hello";
            entry.Key = strKey;
            if((String)entry.Key!=strKey)
            {
                iCountErrors++;
                Console.WriteLine( "Err_89745sg! wrong value returned");
            }
            entry = new DictionaryEntry("STring", 5);
            entry.Key = 5;
            if((Int32)entry.Key!=5)
            {
                iCountErrors++;
                Console.WriteLine( "Err_89745sg! wrong value returned");
            }
            strLoc = "Loc_dfyusg";
            iCountTestcases++;
            entry = new DictionaryEntry("STring", 5);
            try
            {
                entry.Key = null;
                iCountErrors++;
                Console.WriteLine( "Err_9745sdg! Exception not thrown");
            }
            catch(ArgumentNullException)
            {
            }
            catch(Exception ex)
            {
                iCountErrors++;
                Console.WriteLine( "Err_9745sdg! Unexpected exception thrown, " + ex.GetType().Name);
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
            Console.WriteLine("FAiL! "+s_strTFName+" ,inCountErrors=="+iCountErrors.ToString()+" , BugNums?: "+s_strActiveBugNums );
            return false;
        }
    }
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co8609set_Key cbA = new Co8609set_Key();
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
