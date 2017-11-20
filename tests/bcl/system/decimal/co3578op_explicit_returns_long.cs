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
public class Co3578long_operator_narrow
{
 static String strName = "Decimal.$narrow";
 static String strTest = "Co3578long_operator_narrow.cs";
 static String strPath = Environment.CurrentDirectory;
 public Boolean runTest()
   {
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc="123_er";
   Console.Out.Write( strName );
   Console.Out.Write( ": " );
   Console.Out.Write( strPath + strTest );
   Console.Out.WriteLine( " runTest started..." );
   Decimal[] dcmlFirstValues = {Decimal.One, Decimal.MinusOne, new Decimal(-1234.5678), new Decimal(0.0001), Decimal.Zero}; 
   Int64[] in8FirstValues = {1, -1, -1234, 0, 0}; 
   Decimal[] dcmlSecondValues = {new Decimal(1234.567895969984), 
				 new Decimal(Int64.MinValue),
				 new Decimal(Int32.MinValue),
				 new Decimal(Int16.MaxValue), new Decimal(Int16.MinValue)};
   Int64[] in8SecondValues = {1234, 
			      Int64.MinValue,
			      Int32.MinValue,
			      Int16.MaxValue, Int16.MinValue};
   Decimal[] dcmlExceptions = {Decimal.MaxValue, Decimal.MinValue, Decimal.Parse("9223372036854775808.58062856235", CultureInfo.InvariantCulture), Decimal.Parse("-9223372036854775809.580623465230523", CultureInfo.InvariantCulture)}; 
   Int64 in8a;
   Decimal dcml1;
   try
     {
     do
       {
       for (int aa = 0; aa < dcmlFirstValues.Length; aa++)
	 {
	 in8a = (long)dcmlFirstValues[aa];
	 ++iCountTestcases;
	 if ( in8a != in8FirstValues[aa] )
	   {
	   ++iCountErrors;
	   Console.Error.WriteLine(  "POINTTOBREAK: Error E_972qr_" + aa + "  in8a ==" + in8a  );
	   }
	 }
       dcml1 = new Decimal();
       for (int aa = 0; aa < dcmlSecondValues.Length; aa++)
	 {
	 in8a = (long)dcmlSecondValues[aa];
	 ++iCountTestcases;
	 if ( in8a != in8SecondValues[aa] )
	   {
	   ++iCountErrors;
	   Console.Error.WriteLine(  "POINTTOBREAK: Error E_783sf_" + aa + "  in8a ==" + in8a + " dcmlSecondValues[aa] == " + dcmlSecondValues[aa] );
	   }
	 }
       for (int aa = 0; aa < dcmlExceptions.Length; aa++)
	 {
	 try
	   {
	   ++iCountTestcases;
	   in8a = (long)dcmlExceptions[aa];
	   ++iCountErrors;
	   Console.Error.WriteLine(  "POINTTOBREAK: Error E_852sp_" + aa + "!  Exception not thrown"  );
	   }
	 catch (OverflowException)
	   {
	   }
	 catch (Exception ex)
	   {
	   ++iCountErrors;
	   Console.Error.WriteLine(  "POINTTOBREAK: Error E_259pl_" + aa + "!  , Wrong Exception thrown == " + ex.ToString()  );
	   }
	 }
       } while ( false );
     }
   catch (Exception exc_general)
     {
     ++iCountErrors;
     Console.Error.WriteLine ( "POINTTOBREAK: Error Err_103! strLoc=="+ strLoc + " ,exc_general=="+exc_general  );
     }
   if ( iCountErrors == 0 )
     {
     Console.Error.WriteLine( "paSs. " + strPath + strTest + "  iCountTestcases==" + iCountTestcases );
     return true;
     }
   else
     {
     Console.Error.WriteLine( "FAiL! " + strPath + strTest + "   iCountErrors==" + iCountErrors );
     return false;
     }
   }
 public static void Main( String[] args ) 
   {
   Boolean bResult = false;	
   Co3578long_operator_narrow oCbTest = new Co3578long_operator_narrow();
   try
     {
     bResult = oCbTest.runTest();
     }
   catch ( Exception exc_main )
     {
     bResult = false;
     Console.Error.WriteLine(  "POINTTOBREAK:  FAiL!  Error Err_999zzz! (" + strTest + ") Uncaught Exception caught in main(), exc_main==" + exc_main  );
     }
   if ( ! bResult )
     Console.Error.WriteLine(  "PATHTOSOURCE:  " + strPath + strTest + "   FAiL!"  );
   if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 1; 
   }
}
