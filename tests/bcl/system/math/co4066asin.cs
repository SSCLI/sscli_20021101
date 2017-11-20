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
public class Co4066Asin
{
 public virtual bool runTest()
   {
   Console.Out.WriteLine( "Co4066Asin.cs  runTest() started." );
   StringBuilder sblMsg = new StringBuilder( 99 );
   int iCountErrors = 0;
   int iCountTestcases = 0;
   double dubExpected = 0;
   double dubEquation = 0;
   double dubOne = 0;
   double dubTwo = 0;
   double dubValue = 0;
   do
     {
     try
       {
       ++iCountTestcases;
       dubExpected = 0.0;  
       dubOne = ( dubExpected - 0.01 );
       dubTwo = ( dubExpected + 0.01 );
       dubValue = ( 0 );
       dubEquation = Math.Asin( dubValue );
       if ( !( (dubOne <= dubEquation) && (dubEquation <= dubTwo) ) )	 
	 {
	 ++iCountErrors;
	 sblMsg.Append( "POINTTOBREAK: find E_c92n, Asin not equal to expected value" );
	 sblMsg.Append(  " dubEquation == " + dubEquation+ " dubOne == " + dubOne);
	 Console.Error.WriteLine( sblMsg.ToString() );
	 break;
	 }
       ++iCountTestcases;
       dubExpected = 0.551069;  
       dubOne = ( dubExpected - 0.1 );
       dubTwo = ( dubExpected + 0.1 );
       dubValue = ( Math.PI/6 );
       dubEquation = Math.Asin( dubValue );
       if ( ( dubOne <= dubEquation && dubEquation <= dubTwo ) == false )
	 {
	 ++iCountErrors;
	 sblMsg.Append( "POINTTOBREAK: find E_s02h, Acos not equal to expected value" );
	 Console.Error.WriteLine( sblMsg.ToString() );
	 break;
	 }
       ++iCountTestcases;
       dubExpected = 0.903339;  
       dubOne = ( dubExpected - 0.1 );
       dubTwo = ( dubExpected + 0.1 );
       dubValue = ( Math.PI/4 );
       dubEquation = Math.Asin( dubValue );
       if ( ( dubOne <= dubEquation && dubEquation <= dubTwo ) == false )
	 {
	 ++iCountErrors;
	 sblMsg.Append( "POINTTOBREAK: find E_f91a, Acos not equal to expected value" );
	 Console.Error.WriteLine( sblMsg.ToString() );
	 break;
	 }
       ++iCountTestcases;
       dubExpected = -1.570796;  
       dubOne = ( dubExpected - 0.1 );
       dubTwo = ( dubExpected + 0.1 );
       dubValue = ( -1 );
       dubEquation = Math.Asin( dubValue );
       if ( ( dubOne <= dubEquation && dubEquation <= dubTwo ) == false )
	 {
	 ++iCountErrors;
	 sblMsg.Append( "POINTTOBREAK: find E_cc22, Acos not equal to expected value" );
	 Console.Error.WriteLine( sblMsg.ToString() );
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
     Console.Error.Write( "Math\\Co4066Asin.cs: paSs.  iCountTestcases==" );
     Console.Error.WriteLine( iCountTestcases );
     return true;
     }
   else
     {
     Console.Error.Write( "Co4066Asin.cs iCountErrors==" );
     Console.Error.WriteLine( iCountErrors );
     Console.Error.WriteLine(  "Co4066Asin.cs   FAiL !"  );
     return false;
     }
   }
 public static void Main( String[] args )
   {
   bool bResult = false; 
   StringBuilder sblW = null;
   Co4066Asin cbA = new Co4066Asin();
   try
     {
     bResult = cbA.runTest();
     }
   catch ( Exception exc )
     {
     bResult = false;
     Console.Error.WriteLine(  "Co4066Asin.cs"  );
     sblW = new StringBuilder( "EXTENDEDINFO: (E_999zzz) " );
     sblW.Append( exc.ToString() );
     Console.Error.WriteLine(  sblW.ToString()  );
     }
   if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 1; 
   }
}
