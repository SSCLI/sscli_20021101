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
public class Co4050Abs_double
{
 public virtual bool runTest()
   {
   Console.Out.WriteLine( "Co4050Abs_double .cs  runTest() started." );
   StringBuilder sblMsg = new StringBuilder( 99 );
   int iCountErrors = 0;
   int iCountTestcases = 0;
   double doubleVal;
   double doubleKnown;
   do
     {
     try
       {
       ++iCountTestcases;
       doubleVal = -( (double)1.79769e+308 );
       doubleKnown = 1.79769e+308;
       if ( Math.Abs( doubleVal ) != doubleKnown )
	 {
	 ++iCountErrors;
	 sblMsg.Length =  99 ;
	 sblMsg.Append( "POINTTOBREAK: find E_b9wu, ABS wrong! " );
	 sblMsg.Append( "EXTENDEDINFO: Expected doubleKnown == " + doubleKnown );
	 sblMsg.Append( " intead got Math.Abs( doubleVal ) == " + Math.Abs( doubleVal ) );
	 break;
	 }
       ++iCountTestcases;
       doubleVal = -( 0.0 );
       doubleKnown = 0.0;
       if ( Math.Abs( doubleVal ) != doubleKnown )
	 {
	 ++iCountErrors;
	 sblMsg.Length =  99 ;
	 sblMsg.Append( "POINTTOBREAK: find E_pf71, ABS wrong! " );
	 sblMsg.Append( "EXTENDEDINFO: Expected doubleKnown == " + doubleKnown );
	 sblMsg.Append( " intead got Math.Abs( doubleVal ) == " + Math.Abs( doubleVal ) );
	 break;
	 }
       ++iCountTestcases;
       doubleVal = -( 1.0 );
       doubleKnown = 1.0;
       if ( Math.Abs( doubleVal ) != doubleKnown )
	 {
	 ++iCountErrors;
	 sblMsg.Length =  99 ;
	 sblMsg.Append( "POINTTOBREAK: find E_6mz2, ABS wrong! " );
	 sblMsg.Append( "EXTENDEDINFO: Expected doubleKnown == " + doubleKnown );
	 sblMsg.Append( " intead got Math.Abs( doubleVal ) == " + Math.Abs( doubleVal ) );
	 break;
	 }
       ++iCountTestcases;
       doubleVal = ( (double)1.79769e+307 );
       doubleKnown =1.79769e+307;
       if ( Math.Abs( doubleVal ) != doubleKnown )
	 {
	 ++iCountErrors;
	 sblMsg.Length =  99 ;
	 sblMsg.Append( "POINTTOBREAK: find E_ss81, ABS wrong! " );
	 sblMsg.Append( "EXTENDEDINFO: Expected doubleKnown == " + doubleKnown );
	 sblMsg.Append( " intead got Math.Abs( doubleVal ) == " + Math.Abs( doubleVal ) );
	 break;
	 }
       ++iCountTestcases;
       doubleVal = ( 0.0 );
       doubleKnown = 0.0;
       if ( Math.Abs( doubleVal ) != doubleKnown )
	 {
	 ++iCountErrors;
	 sblMsg.Length =  99 ;
	 sblMsg.Append( "POINTTOBREAK: find E_4f9v, ABS wrong! " );
	 sblMsg.Append( "EXTENDEDINFO: Expected doubleKnown == " + doubleKnown );
	 sblMsg.Append( " intead got Math.Abs( doubleVal ) == " + Math.Abs( doubleVal ) );
	 break;
	 }
       ++iCountTestcases;
       doubleVal = ( 1.0 );
       doubleKnown = 1.0;
       if ( Math.Abs( doubleVal ) != doubleKnown )
	 {
	 ++iCountErrors;
	 sblMsg.Length =  99 ;
	 sblMsg.Append( "POINTTOBREAK: find E_bbq11, ABS wrong! " );
	 sblMsg.Append( "EXTENDEDINFO: Expected doubleKnown == " + doubleKnown );
	 sblMsg.Append( " intead got Math.Abs( doubleVal ) == " + Math.Abs( doubleVal ) );
	 break;
	 }
       }
     catch ( Exception Exc )
       {
       ++iCountErrors;
       sblMsg.Length =  99 ;
       sblMsg.Append( "POINTTOBREAK: find E_f3h5, Generic Exception Caught, Exc.ToString() == " );
       sblMsg.Append( Exc.ToString() );
       Console.Error.WriteLine( sblMsg.ToString() );
       break;
       }
     }
   while ( false );
   if ( iCountErrors == 0 )
     {
     Console.Error.Write( "Math\\Co4050Abs_double .cs: paSs.  iCountTestcases==" );
     Console.Error.WriteLine( iCountTestcases );
     return true;
     }
   else
     {
     Console.Error.Write( "******* HOTFIX Opened on 11/24 - M10 tree!!!!*********");
     Console.Error.Write( "Co4050Abs_double .cs iCountErrors==" );
     Console.Error.WriteLine( iCountErrors );
     Console.Error.WriteLine(  "Co4050Abs_double .cs   FAiL !"  );
     return false;
     }
   }
 public static void Main( String[] args )
   {
   bool bResult = false; 
   StringBuilder sblW = null;
   Co4050Abs_double  cbA = new Co4050Abs_double ();
   try
     {
     bResult = cbA.runTest();
     }
   catch ( Exception exc )
     {
     bResult = false;
     Console.Error.WriteLine(  "Co4050Abs_double .cs"  );
     sblW = new StringBuilder( "EXTENDEDINFO: (E_999zzz) " );
     sblW.Append( exc.ToString() );
     Console.Error.WriteLine(  sblW.ToString()  );
     }
   if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 1; 
   }
}
