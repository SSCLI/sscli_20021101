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
using System.Globalization;
public class Co6043Format_u64_str_ifp
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strLastModCoder  = "";
 public static String s_strOrigCoder     = "";
 public static String s_strComponentBeingTested
   = "UInt64.ToString( UInt64, String, IFormatProvider )";
 public static String s_strTFName        = "Co6043Format_u64_str_ifp.cs";
 public static String s_strTFAbbrev      = "Co6043";
 public static String s_strTFPath        = "";
 int iCountErrors = 0;
 int iCountTestcases = 0;
 String m_strLoc="Loc_beforeRun";
 public Boolean runTest()
   {
   Console.WriteLine( s_strTFPath +" "+ s_strTFName +" ,for "+ s_strComponentBeingTested +"  ,Source ver "+ s_strDtTmVer );
   try
     {
     m_strLoc = "Loc_normalTests";
     NumberFormatInfo nfi = new NumberFormatInfo();
     nfi.CurrencySymbol =			"[$US dollars]";
     nfi.CurrencyDecimalDigits =		4;
     nfi.NegativeSign =				"_unsigned??_";
     nfi.NumberDecimalDigits =		4;
     nfi.NumberGroupSeparator =      "";
     nfi.CurrencyGroupSeparator =    "";
     UInt64[] primativeULong = { 
       0,
       100,
       1000,
       10000,
       UInt64.MaxValue,
       UInt64.MinValue,
     };
     String[] currencyResults = new String[ primativeULong.Length ]; 
     String[] decimalResults = new String[ primativeULong.Length ]; 
     String[] fixedResults = new String[ primativeULong.Length ]; 
     String[] generalResults = new String[ primativeULong.Length ]; 
     String[] numberResults = new String[ primativeULong.Length ]; 
     String[] sciResults = { 
       "0.000000E+000",
       "1.000000E+002",
       "1.000000E+003",
       "1.000000E+004",
       "1.844674E+019",
       "0.000000E+000",
       "1.844674E+019",
       "1.000000E+000"
     };
     String[] hexResults = new String[ primativeULong.Length ]; 
     UInt64 bb;
     UInt64 div;
     UInt64 remainder;
     Char ch;
     for( int i = 0; i < primativeULong.Length; i++ ) {
     currencyResults[i] =  nfi.CurrencySymbol + primativeULong[i] + "." + "".PadRight( nfi.NumberDecimalDigits, '0' ) ;
     decimalResults[i] =  primativeULong[i] + "";
     fixedResults[i] =  primativeULong[i] + "." + "".PadRight( nfi.NumberDecimalDigits, '0' );
     generalResults[i] =  primativeULong[i] + "";
     numberResults[i] =  primativeULong[i] + "." + "".PadRight( nfi.NumberDecimalDigits, '0' );
     bb = primativeULong[i];
     hexResults[i] =  "" ;
     while ( bb > 0 ) {
     div = bb/16;
     remainder = bb%16;
     if ( remainder < 10 ) hexResults[i] = remainder + hexResults[i];
     else {
     switch( (int)remainder ) {
     case 10 : ch = 'a'; break;
     case 11 : ch = 'b'; break;
     case 12 : ch = 'c'; break;
     case 13 : ch = 'd'; break;
     case 14 : ch = 'e'; break;
     case 15 : ch = 'f'; break;
     default : ch = ' '; break;
     }
     hexResults[i] = ch + hexResults[i];
     }
     bb = div;
     }
     if ( hexResults[i].Equals( "" ) ) hexResults[i] = "0";
     }
     for( int i = 0; i < primativeULong.Length; i++ ) {
     try {
     iCountTestcases++; 
     m_strLoc = "Starting testgroup #C." + iCountTestcases;
     m_strLoc = m_strLoc + "->" + currencyResults[i];
     if ( primativeULong[i].ToString( "C", nfi ).Equals( currencyResults[i] ) != true ) ErrorCode(primativeULong[i].ToString( "C", nfi ));
     } catch( Exception e ) {
     iCountErrors++;
     Console.WriteLine( "Testcase["+iCountTestcases+"]" );
     Console.WriteLine( "Exception:" + e );
     Console.WriteLine( "StrLoc = '"+m_strLoc+"'" );
     }
     }
     for( int i = 0; i < primativeULong.Length; i++ ) {
     try {
     iCountTestcases++; 
     m_strLoc = "Starting testgroup #D." + iCountTestcases;
     m_strLoc = m_strLoc + "->" + decimalResults[i];
     if ( primativeULong[i].ToString( "D", nfi ).Equals( decimalResults[i] ) != true ) ErrorCode(primativeULong[i].ToString( "d", nfi ));
     } catch( Exception e ) {
     iCountErrors++;
     Console.WriteLine( "Testcase["+iCountTestcases+"]" );
     Console.WriteLine( "Exception:" + e );
     Console.WriteLine( "StrLoc = '"+m_strLoc+"'" );
     }
     }
     for( int i = 0; i < primativeULong.Length; i++ ) {
     try {
     iCountTestcases++; 
     m_strLoc = "Starting testgroup #E." + iCountTestcases;
     m_strLoc = m_strLoc + "->" + sciResults[i];
     if ( primativeULong[i].ToString( "E", nfi ).Equals( sciResults[i] ) != true ) ErrorCode(primativeULong[i].ToString( "E", nfi ));
     } catch( Exception e ) {
     iCountErrors++;
     Console.WriteLine( "Testcase["+iCountTestcases+"]" );
     Console.WriteLine( "Exception:" + e );
     Console.WriteLine( "StrLoc = '"+m_strLoc+"'" );
     }
     }
     for( int i = 0; i < primativeULong.Length; i++ ) {
     try {
     iCountTestcases++; 
     m_strLoc = "Starting testgroup #F." + iCountTestcases;
     m_strLoc = m_strLoc + "->" + fixedResults[i];
     if ( primativeULong[i].ToString( "f", nfi ).Equals( fixedResults[i] ) != true ) ErrorCode(primativeULong[i].ToString( "F", nfi ));
     } catch( Exception e ) {
     iCountErrors++;
     Console.WriteLine( "Testcase["+iCountTestcases+"]" );
     Console.WriteLine( "Exception:" + e );
     Console.WriteLine( "StrLoc = '"+m_strLoc+"'" );
     }
     }
     for( int i = 0; i < primativeULong.Length; i++ ) {
     try {
     iCountTestcases++; 
     m_strLoc = "Starting testgroup #G." + iCountTestcases;
     m_strLoc = m_strLoc + "->" + generalResults[i];
     if ( primativeULong[i].ToString( "G", nfi ).Equals( generalResults[i] ) != true ) ErrorCode(primativeULong[i].ToString( "g", nfi ));
     } catch( Exception e ) {
     iCountErrors++;
     Console.WriteLine( "Testcase["+iCountTestcases+"]" );
     Console.WriteLine( "Exception:" + e );
     Console.WriteLine( "StrLoc = '"+m_strLoc+"'" );
     }
     }
     for( int i = 0; i < primativeULong.Length; i++ ) {
     try {
     iCountTestcases++; 
     m_strLoc = "Starting testgroup #N." + iCountTestcases;
     m_strLoc = m_strLoc + "->" + numberResults[i];
     if ( primativeULong[i].ToString( "n", nfi ).Equals( numberResults[i] ) != true ) ErrorCode(primativeULong[i].ToString( "n", nfi ));
     } catch( Exception e ) {
     iCountErrors++;
     Console.WriteLine( "Testcase["+iCountTestcases+"]" );
     Console.WriteLine( "Exception:" + e );
     Console.WriteLine( "StrLoc = '"+m_strLoc+"'" );
     }
     }
     for( int i = 0; i < primativeULong.Length; i++ ) {
     try {
     iCountTestcases++; 
     m_strLoc = "Starting testgroup #X." + iCountTestcases;
     m_strLoc = m_strLoc + "->" + hexResults[i];
     if ( primativeULong[i].ToString( "x", nfi ).Equals( hexResults[i] ) != true ) ErrorCode(primativeULong[i].ToString( "X", nfi ));
     } catch( Exception e ) {
     iCountErrors++;
     Console.WriteLine( "Testcase["+iCountTestcases+"]" );
     Console.WriteLine( "Exception:" + e );
     Console.WriteLine( "hexResults[" + i + "] = '" + hexResults[i] + "'" );
     Console.WriteLine( "StrLoc = '"+m_strLoc+"'" );
     }
     }
     }
   catch( Exception exc_general )
     {
     ++iCountErrors;
     Console.WriteLine( "Error Err_8888yyy ("+ s_strTFAbbrev +")!  Unexpected exception thrown sometime after m_strLoc=="+ m_strLoc +" ,exc_general=="+ exc_general );
     }
   Console.Write(Environment.NewLine);
   Console.WriteLine( "Total Tests Ran: " + iCountTestcases + " Failed Tests: " + iCountErrors );
   if ( iCountErrors == 0 )
     {
     Console.WriteLine( "paSs.   "+ s_strTFPath +" "+ s_strTFName +"  ,iCountTestcases=="+ iCountTestcases );
     return true;
     }
   else
     {
     Console.WriteLine( "FAiL!   "+ s_strTFPath +" "+ s_strTFName +"  ,iCountErrors=="+ iCountErrors +" ,BugNums?: "+ s_strActiveBugNums );
     return false;
     }
   }
 public void ErrorCode( String erk ) {
 m_strLoc = m_strLoc + " tested function produced <" + erk + ">";
 throw new Exception( "Test failed." );
 }
 public void ErrorCode() {
 throw new Exception( "Test failed." );
 }
 public static void Main( String[] args ) 
   {
   Environment.ExitCode = 1;  
   Boolean bResult = false; 
   Co6043Format_u64_str_ifp cbX= new Co6043Format_u64_str_ifp();
   try
     {
     bResult = cbX.runTest();
     }
   catch ( Exception exc_main )
     {
     bResult = false;
     Console.WriteLine( "FAiL!  Error Err_9999zzz ("+ s_strTFAbbrev +")!  Uncaught Exception caught fell to Main(), exc_main=="+ exc_main );
     }
   if ( ! bResult )
     {
     Console.WriteLine( s_strTFPath + s_strTFName );
     Console.WriteLine( " " );
     Console.WriteLine( "FAiL!  "+ s_strTFAbbrev );  
     Console.WriteLine( " " );
     }
   if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 1; 
   }
} 
