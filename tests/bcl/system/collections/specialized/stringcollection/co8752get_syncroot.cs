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
using System.IO;
using System.Text;  
using System;
using System.Collections;
using System.Collections.Specialized;
public class Co8752get_SyncRoot
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "StringCollection.SyncRoot get";
    public static String s_strTFName        = "Co8752get_SyncRoot.cs";
    public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
    public static String s_strTFPath        = Environment.CurrentDirectory;
    public const int MAX_LEN = 50;          
    public virtual bool runTest()
    {
        Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver: " + s_strDtTmVer);
        int iCountErrors = 0;
        int iCountTestcases = 0;
        String strLoc = "Loc_000oo";
        StringCollection sc; 
        StringCollection sc1; 
        object root;         
        object root1;         
        string [] values = 
        {
            "",
            " ",
            "a",
            "aa",
            "text",
            "     spaces",
            "1",
            "$%^#",
            "2222222222222222222222222",
            System.DateTime.Today.ToString(),
            Int32.MaxValue.ToString()
        };
        try
        {
            Console.WriteLine("--- create collection ---");
            strLoc = "Loc_001oo"; 
            iCountTestcases++;
            sc = new StringCollection();
            sc1 = new StringCollection();
            Console.WriteLine("  toString: " + sc.SyncRoot.ToString());
            Console.WriteLine("1. SyncRoot for empty collection");
            Console.WriteLine("     - roots of two different collections");
            iCountTestcases++;
            root = sc.SyncRoot;
            root1 = sc1.SyncRoot;
            if ( root.Equals(root1) ) 
            {
                iCountErrors++;
                Console.WriteLine("Err_0001a, roots of two different collections are equal");
            }
            Console.WriteLine("     - roots of the same collection");
            iCountTestcases++;
            root1 = sc.SyncRoot;
            if ( !root.Equals(root1) ) 
            {
                iCountErrors++;
                Console.WriteLine("Err_0001b, roots for one collections are not equal");
            }
            Console.WriteLine("2. SyncRoot for filled collection");
            iCountTestcases++;
            sc.AddRange(values);
            sc1.AddRange(values);
            root = sc.SyncRoot;
            root1 = sc1.SyncRoot;
            Console.WriteLine("     - roots of two different collections");
            if ( root.Equals(root1) ) 
            {
                iCountErrors++;
                Console.WriteLine("Err_0002a, roots of two different collections are equal");
            }
            Console.WriteLine("     - roots of the same collection");
            iCountTestcases++;
            root1 = sc.SyncRoot;
            if ( !root.Equals(root1) ) 
            {
                iCountErrors++;
                Console.WriteLine("Err_0002b, roots for one collections are not equal");
            }
            Console.WriteLine("3. SyncRoot vs StringCollection");
            iCountTestcases++;
            root = sc.SyncRoot;
            string str = root.ToString();
            if ( str.IndexOf("StringCollection", 0) == -1 ) 
            {
                iCountErrors++;
                Console.WriteLine("Err_0003, root.ToString() doesn't contain \"StringCollection\"");
            }
        } 
        catch (Exception exc_general ) 
        {
            ++iCountErrors;
            Console.WriteLine (s_strTFAbbrev + " : Error Err_general!  strLoc=="+ strLoc +", exc_general==\n"+exc_general.ToString());
        }
        if ( iCountErrors == 0 )
        {
            Console.WriteLine( "Pass.   "+s_strTFPath +" "+s_strTFName+" ,iCountTestcases=="+iCountTestcases);
            return true;
        }
        else
        {
            Console.WriteLine("Fail!   "+s_strTFPath+" "+s_strTFName+" ,iCountErrors=="+iCountErrors+" , BugNums?: "+s_strActiveBugNums );
            return false;
        }
    }
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co8752get_SyncRoot cbA = new Co8752get_SyncRoot();
        try 
        {
            bResult = cbA.runTest();
        } 
        catch (Exception exc_main)
        {
            bResult = false;
            Console.WriteLine(s_strTFAbbrev + " : Fail! Error Err_main! Uncaught Exception in main(), exc_main=="+exc_main);
        }
        if (!bResult)
        {
            Console.WriteLine ("Path: "+s_strTFName + s_strTFPath);
            Console.WriteLine( " " );
            Console.WriteLine( "Fail!  "+ s_strTFAbbrev);
            Console.WriteLine( " " );
        }
        if (bResult) Environment.ExitCode=0; else Environment.ExitCode=1;
    }
}
