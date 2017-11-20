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
public class Co9064InvalidPathChars
{
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "Path.InvalidPathChars";
    public static String s_strTFName        = "Co9064InvalidPathChars.cs";
    public static String s_strTFAbbrev      = "Co9064";
    public static String s_strTFPath        = Environment.CurrentDirectory;
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
        int iCountErrors = 0;
        int iCountTestcases = 0;
        String strLoc = "Loc_000oo";
        try
        {
            Char[] cArrInvalidPath ;
            strLoc = "Loc_89y9t";
            iCountTestcases++;
            cArrInvalidPath = Path.InvalidPathChars;
            if(! VerifyInvalidPathChars( cArrInvalidPath ) )
            {
                iCountErrors++;
                printerr( "Error_0001! Incorrect invalid path chars ::" + cArrInvalidPath);
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
            Console.WriteLine("FAiL! "+s_strTFName+" ,iCountErrors=="+iCountErrors.ToString());
            return false;
        }
    }   
    private bool VerifyInvalidPathChars( char[] cArrInvalidPath )
    {
        char[] InvalidPathChars = { '\"', '<', '>', '|', '\0', '\b', (Char)16, (Char)17, (Char)18, (Char)20, (Char)21, (Char)22, (Char)23, (Char)24, (Char)25 };
        if ( InvalidPathChars.Length != cArrInvalidPath.Length )
            return false ;               
        for(int iLoop = 0 ; iLoop < cArrInvalidPath.Length ; iLoop++ ) 
            if ( cArrInvalidPath[iLoop] !=  InvalidPathChars[iLoop] )
                return false ;   
        return true ;        
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
        Co9064InvalidPathChars cbA = new Co9064InvalidPathChars();
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
