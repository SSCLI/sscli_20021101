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
public class Co6021Parse_str_int_nfi
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strLastModCoder  = "";
 public static String s_strOrigCoder     = "";
 public static String s_strComponentBeingTested
   = "UInt16.Parse( String, Int32, NumberFormatInfo )";
 public static String s_strTFName        = "Co6021Parse_str_int_nfi.cs";
 public static String s_strTFAbbrev      = "Co6021";
 public static String s_strTFPath        = "";
 int iCountErrors = 0;
 int iCountTestcases = 0;
 String m_strLoc="Loc_beforeRun";
 Boolean m_verbose = false;
 public Boolean runTest()
   {
   Console.WriteLine( s_strTFPath +" "+ s_strTFName +" ,for "+ s_strComponentBeingTested +"  ,Source ver "+ s_strDtTmVer );
   String strBaseLoc;
   XenoUInt16 xeno = new XenoUInt16();
   UInt16 i = ((UInt16) 0 );
   try
     {
     NumberFormatInfo nfi = NumberFormatInfo.CurrentInfo;
     m_strLoc = "Loc_normalTests";
     String testStr = "";
     UInt16 testUI;
     while( xeno.HasMoreValues() ) {
     i = ((UInt16) xeno.GetNextValue() );
     iCountTestcases++;
     testStr =  i.ToString( "d");
     testUI = UInt16.Parse( testStr, NumberStyles.Any, nfi );
     if ( testUI != i ) {
     Console.WriteLine( "Fail! " + testUI + " != " + i );
     iCountErrors++;
     }
     iCountTestcases++;
     testUI = UInt16.Parse( testStr.PadLeft( i, ' '), NumberStyles.Any, nfi );
     if ( testUI != i ) {
     Console.WriteLine( "Fail! (pad left)" + testUI + " != " + i );
     iCountErrors++;
     }
     iCountTestcases++;
     testUI = UInt16.Parse( testStr.PadRight( i, ' '), NumberStyles.Any, nfi  );
     if ( testUI != i ) {
     Console.WriteLine( "Fail! (pad right)" + testUI + " != " + i );
     iCountErrors++;
     }
     iCountTestcases++;
     testUI = UInt16.Parse( testStr.PadRight( i, ' ').PadLeft( i,' ' ), NumberStyles.Any, nfi );
     if ( testUI != i ) {
     Console.WriteLine( "Fail! (pad right+left) " + testUI + " != " + i );
     iCountErrors++;
     }
     try {
     iCountTestcases++;
     testStr =  i.ToString( "E");
     testUI = UInt16.Parse( testStr, NumberStyles.AllowCurrencySymbol, nfi );
     iCountErrors++;
     Console.WriteLine( "Failed! NumberStyle.AllowCurrencySymbol::No exception Thrown! String = '"+testStr+"'" );
     }
     catch( FormatException fe ) {}
     catch( Exception e ) {
     iCountErrors++;
     Console.WriteLine( "Failed! Wrong exception: '" + e + "'" );
     }
     }
     try {
     iCountTestcases++;
     testStr =  i.ToString( "E", nfi );
     testUI = UInt16.Parse( testStr, NumberStyles.AllowLeadingSign );
     iCountErrors++;
     Console.WriteLine( "Failed! No exception Thrown! String = '"+testStr+"'" );
     }
     catch( FormatException fe ) {}
     catch( Exception e ) {
     iCountErrors++;
     Console.WriteLine( "Failed! Wrong exception: '" + e + "'" );
     }
     try {
     iCountTestcases++;
     UInt16 UI = UInt16.Parse( null, NumberStyles.Any, nfi );
     iCountErrors++;
     Console.WriteLine( "Failed! No exception Thrown! String = '"+testStr+"'" );
     }
     catch( ArgumentException ae ) {}
     catch( Exception e ) {
     iCountErrors++;
     Console.WriteLine( "Failed! Wrong exception: '" + e + "'" );
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
   Co6021Parse_str_int_nfi cbX = new Co6021Parse_str_int_nfi();
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
public class XenoUInt16 {
 Double LowEpsilon = ((Double) .85 );
 Double HighEpsilon = ((Double) .85 );
 Boolean LowFinished;
 Boolean HighFinished;
 Boolean AllFinished;
 UInt16 Minimum;
 UInt16 Maximum;
 UInt16 CurrentMid;
 public Boolean IsValidMid() {
 UInt16 m = Minimum;
 UInt16 M = Maximum;
 if ( m > M ) throw new ArgumentException( "Xeno:Min > Max");
 while ( m < 0 ) {
 m++;
 M++;
 }
 while ( m > 0 ) {
 m--;
 M--;
 }
 CurrentMid = UInt16.Parse(  ((M-m)/2).ToString() );
 return true;
 }
 public XenoUInt16() {
 LowFinished = false;
 HighFinished = false;
 AllFinished = false;
 Minimum = UInt16.MinValue;
 Maximum = UInt16.MaxValue;
 if ( IsValidMid() == false ) AllFinished = true;
 }
 public Boolean HasMoreValues() {
 return ( !AllFinished );
 }
 public UInt16 GetNextValue() {
 if ( AllFinished == false ) {
 if ( LowFinished == false ) {
 CurrentMid = (UInt16) (Convert.ToDouble(CurrentMid)*LowEpsilon);
 if ( CurrentMid <= 0 ) {
 IsValidMid();
 LowFinished = true;
 return Minimum;
 }
 return UInt16.Parse(  (Minimum + CurrentMid).ToString() );
 }
 if ( HighFinished == false ) {
 CurrentMid = (UInt16) (Convert.ToDouble(CurrentMid)*HighEpsilon);
 if ( CurrentMid <= 0 ) {
 IsValidMid();
 HighFinished = true;
 return Maximum;
 }
 return UInt16.Parse(  (Maximum - CurrentMid).ToString() );
 }
 AllFinished = true;
 IsValidMid();
 return CurrentMid;
 }
 throw new OverflowException( "No more values in Range" );
 }
}
