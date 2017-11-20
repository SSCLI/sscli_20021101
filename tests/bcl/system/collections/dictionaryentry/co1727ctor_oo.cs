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
using System;
using System.Collections;
using System.Globalization;
class Co1727
{
    public String s_strActiveBugNums                 = "";
    public static String s_strDtTmVer                = "";
    public static String s_strComponentBeingTested   = "List()";
    public String s_strTFName                        = "Co1727ctor_OO.cs";
    public String s_strTFAbbrev                      = "Co1727";
    public String s_strTFPath                        = "";
    public Boolean verbose                           = false;
    public Boolean runTest()
    {
        Console.Error.WriteLine( s_strTFPath + " " + s_strTFName + " , for " + s_strComponentBeingTested + "  ,Source ver " + s_strDtTmVer );
        int iCountTestcases = 0;
        int iCountErrors    = 0;
        if ( verbose ) Console.WriteLine( "make new valid dictionary entry" );
        try
        {
            ++iCountTestcases;
            DictionaryEntry de = new DictionaryEntry( 1, 2 );
            if ( ! 1.Equals( de.Key ) )
            {
                ++iCountErrors;
                Console.WriteLine( "Err_001a,  incorrect key" );
            }
            if ( ! 2.Equals( de.Value ) )
            {
                ++iCountErrors;
                Console.WriteLine( "Err_001b,  incorrect value" );
            }
        }
        catch (Exception ex)
        {
            ++iCountErrors;
            Console.WriteLine( "Err_001c,  Unexpected exception was thrown ex: " + ex.ToString() );
        }
        if ( verbose ) Console.WriteLine( "make dictionary entry which should throw ArgumentNullException since key is null" );
        try
        {
            ++iCountTestcases;
            DictionaryEntry de = new DictionaryEntry( null, 1 );
            ++iCountErrors;
            Console.WriteLine( "Err_002a,  should have thrown ArgumentNullException" );
        }
        catch ( ArgumentNullException)
        {}
        catch (Exception ex)
        {
            ++iCountErrors;
            Console.WriteLine( "Err_002b,  Excpected ArgumentNullException but thrown ex: " + ex.ToString() );
        }
        if ( verbose ) Console.WriteLine( "make new valid dictionary entry with value null" );
        try
        {
            ++iCountTestcases;
            DictionaryEntry de = new DictionaryEntry( this, null );
            if ( ! this.Equals( de.Key ) )
            {
                ++iCountErrors;
                Console.WriteLine( "Err_003a,  incorrect key" );
            }
            if ( de.Value != null )
            {
                ++iCountErrors;
                Console.WriteLine( "Err_003b,  incorrect value" );
            }
        }
        catch (Exception ex)
        {
            ++iCountErrors;
            Console.WriteLine( "Err_003c,  Unexpected exception was thrown ex: " + ex.ToString() );
        }
        if ( iCountErrors == 0 )
        {
            Console.WriteLine( "paSs.   "+ s_strTFPath +" "+ s_strTFName +"  ,iCountTestcases="+ iCountTestcases.ToString() );
            return true;
        }
        else
        {
            Console.WriteLine( "FAiL!   "+ s_strTFPath +" "+ s_strTFName +"  ,iCountErrors="+ iCountErrors.ToString() +" ,BugNums?: "+ s_strActiveBugNums );
            return false;
        }
    }
    public static void Main( String [] args )
    {
        Co1727 runClass = new Co1727();
        Boolean bResult = runClass.runTest();
        if ( ! bResult )
        {
            Console.WriteLine( runClass.s_strTFPath + runClass.s_strTFName );
            Console.Error.WriteLine( " " );
            Console.Error.WriteLine( "FAiL!  "+ runClass.s_strTFAbbrev );  
            Console.Error.WriteLine( " " );
            Console.Error.WriteLine( "ACTIVE BUGS: " + runClass.s_strActiveBugNums ); 
        }
        if ( bResult == true ) Environment.ExitCode = 0;
        else Environment.ExitCode = 1; 
    }
}  
