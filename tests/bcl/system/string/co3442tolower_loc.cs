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
public class Co3442ToLower_Loc
{
 static String strName = "String.ToLower";
 static String strTest = "Co3442ToLower_Loc.";
 static String strPath = "";
 public Boolean runTest()
   {
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc="123_er";
   Console.Out.Write( strName );
   Console.Out.Write( ": " );
   Console.Out.Write( strPath + strTest );
   Console.Out.WriteLine( " runTest started..." );
   String strAlphabetsLower = "abcdefghijklmnopqrstuvwxyz";
   String strAlphabetsUpper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   String str1 = null;
   String str2 = null;
   CultureInfo locUSEnglish = null; 
   CultureInfo locGDR = null;  
   CultureInfo locJPN = null;  
   try
     {
     do
       {
       Console.Error.WriteLine( "[] Create locales" ); 
       ++iCountTestcases;
       try
	 {
	 locUSEnglish = new CultureInfo("en-us"); 
	 if ( locUSEnglish == null )
	   {
	   Console.WriteLine( strTest+ "E_101" );
	   Console.WriteLine( strTest+ "Failed to construct default locUSEnglish object" );
	   ++iCountErrors;
	   break;
	   }
	 locGDR = new CultureInfo( 1031 ); 
	 if ( locGDR == null )
	   {
	   Console.WriteLine( strTest+ "E_303" );
	   Console.WriteLine( strTest+ "Failed to construct German locale object" );
	   ++iCountErrors;
	   break;
	   }
	 locJPN = new CultureInfo( 1041 ); 
	 if ( locJPN == null )
	   {
	   Console.WriteLine( strTest+ "E_404" );
	   Console.WriteLine( strTest+ "Failed to construct Japanese locale object" );
	   ++iCountErrors;
	   break;
	   }
	 }
       catch (Exception ex)
	 {
	 Console.WriteLine( strTest+ "E_505" );
	 Console.WriteLine( strTest+ "Unexpected Exception: " + ex.ToString() );
	 ++iCountErrors;
	 break;
	 }
       str1 = strAlphabetsUpper;
       str2 = str1.ToLower(locUSEnglish);
       ++iCountTestcases;
       if ( String.Compare(str2,strAlphabetsLower) != 0 )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK: Error E_972qr!  str2==" + str2  );
	 }
       str1 = strAlphabetsUpper;
       str2 = str1.ToLower(locUSEnglish);
       ++iCountTestcases;
       if ( String.Compare(str2,strAlphabetsLower) != 0 )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK: Error E_724sm!  str2==" + str2  );
	 }
       str1 = strAlphabetsUpper;
       str2 = str1.ToLower(locGDR);
       ++iCountTestcases;
       if ( String.Compare(str2,strAlphabetsLower) != 0 )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK: Error E_790df!  str2==" + str2  );
	 }
       str1 = strAlphabetsUpper;
       str2 = str1.ToLower(locJPN);
       ++iCountTestcases;
       if ( String.Compare(str2,strAlphabetsLower) != 0 )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK: Error E_702mi!  str2==" + str2  );
	 }
       locUSEnglish = null;
       str1 = strAlphabetsUpper;
       try
	 {
	 ++iCountTestcases;
	 str2 = str1.ToLower(locUSEnglish);
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK: Error E_972qr!  Exception not thrown"  );
	 }
       catch (ArgumentNullException )
	 {
	 }
       catch (Exception ex)
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK: Error E_83err!  , Wrong Exception thrown == " + ex.ToString()  );
	 }
       } while ( false );
     }
   catch (Exception exc_general)
     {
     ++iCountErrors;
     Console.Error.WriteLine ( "POINTTOBREAK: Error Err_103! strLoc=="+ strLoc + " ,exc_general=="+exc_general  );
     }
   if ( iCountErrors == 0 ) {   return true; }
   else {  return false;}
   }
 public static void Main( String[] args ) 
   {
   Boolean bResult = false;	
   Co3442ToLower_Loc oCbTest = new Co3442ToLower_Loc();
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
   if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 11; 
   }
}
