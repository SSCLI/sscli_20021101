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
public class Co3770Sort_oii
{
 public static readonly String s_strActiveBugNums = "";
 public static readonly String s_strDtTmVer       = "";
 public static readonly String s_strClassMethod   = "Array.Sort(Object[], int, int)";
 public static readonly String s_strTFName        = "Co3770Sort_oii";
 public static readonly String s_strTFAbbrev      = "Co3770";
 public static readonly String s_strTFPath        = "";
 public virtual bool runTest()
   {
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc="123_er";
   String strInfo = null;
   Console.Out.Write( s_strClassMethod );
   Console.Out.Write( ": " );
   Console.Out.Write( s_strTFPath + s_strTFName );
   Console.Out.Write( ": " );
   Console.Out.Write( s_strDtTmVer );
   Console.Out.WriteLine( " runTest started..." );
   Object o1;
   Char chValue;
   Int16 i16Value;
   int iReturnValue;
   Object[] oArrValues;
   Object[,] o2ArrValues;
   try
     {
     LABEL_860_GENERAL:
     do
       {
       oArrValues = new Object[cArr.Length];
       for(int i=0; i < cArr.Length; i++)
	 {
	 oArrValues[i]=cArr[i];
	 }
       Array.Sort(oArrValues, 2, 6);
       ++iCountTestcases;
       for(int i=0; i < oArrValues.Length; i++)
	 {
	 strLoc="765vp_" + i;
	 o1 = cSortedArr[i];
	 if ( o1.Equals(oArrValues[i]) != true )
	   {
	   ++iCountErrors;
	   Console.WriteLine( s_strTFAbbrev + ", Err_320ye_" + i + ", oArrValues[i]=="+ oArrValues[i] + ", o1==" +  o1 );
	   }
	 }
       oArrValues = new Object[i16Arr.Length];
       for(int i=0; i < i16Arr.Length; i++)
	 {
	 oArrValues[i]=i16Arr[i];
	 }
       Array.Sort(oArrValues, 3, 4);
       ++iCountTestcases;
       for(int i=0; i < oArrValues.Length; i++)
	 {
	 o1 = iSorted16Arr[i];
	 if ( o1.Equals(oArrValues[i]) != true )
	   {
	   ++iCountErrors;
	   Console.WriteLine( s_strTFAbbrev + "Err_246sdf_" + i + ", oArrValues[i]==" + oArrValues[i] + " o1==" + o1);
	   }
	 }
       try
	 {
	 ++iCountTestcases;
	 Array.Sort(null, 0, 1);
	 ++iCountErrors;
	 Console.WriteLine( s_strTFAbbrev + "Err_835xr!");
	 }
       catch (ArgumentException ex)
	 {
	 }
       catch (Exception ex)
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(" Error E_972qr!  , Wrong Exception thrown == " + ex.ToString());
	 Console.WriteLine(ex.StackTrace);
	 }
       oArrValues = new Object[cArr.Length];
       for(int i=0; i < cArr.Length; i++)
	 {
	 oArrValues[i]=cArr[i];
	 }
       try
	 {
	 ++iCountTestcases;
	 Array.Sort(oArrValues, -1, 3);
	 ++iCountErrors;
	 Console.WriteLine( s_strTFAbbrev + "Err_659fg!");
	 }
       catch (ArgumentException ex)
	 {
	 }
       catch (Exception ex)
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(" Error E_856df!  , Wrong Exception thrown == " + ex.ToString());
	 Console.WriteLine(ex.StackTrace);
	 }
       try
	 {
	 ++iCountTestcases;
	 Array.Sort(oArrValues, 1, -3);
	 ++iCountErrors;
	 Console.WriteLine( s_strTFAbbrev + "Err_6585fgj!");
	 }
       catch (ArgumentException ex)
	 {
	 }
       catch (Exception ex)
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(" Error E_gsjjj!  , Wrong Exception thrown == " + ex.ToString());
	 Console.WriteLine(ex.StackTrace);
	 }
       try
	 {
	 ++iCountTestcases;
	 Array.Sort(oArrValues, oArrValues.Length, 1);
	 ++iCountErrors;
	 Console.WriteLine( s_strTFAbbrev + "Err_469dfg!");
	 }
       catch (ArgumentException ex)
	 {
	 }
       catch (Exception ex)
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(" Error E_4986dg!  , Wrong Exception thrown == " + ex.ToString());
	 Console.WriteLine(ex.StackTrace);
	 }
       try
	 {
	 ++iCountTestcases;
	 Array.Sort(oArrValues, 2, 2+ oArrValues.Length);
	 ++iCountErrors;
	 Console.WriteLine( s_strTFAbbrev + "Err_735dfg!");
	 }
       catch (ArgumentException ex)
	 {
	 }
       catch (Exception ex)
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(" Error E_648sd!  , Wrong Exception thrown == " + ex.ToString());
	 Console.WriteLine(ex.StackTrace);
	 }
       } while ( false );
     }
   catch (Exception exc_general)
     {
     ++iCountErrors;
     Console.WriteLine( s_strTFAbbrev + "Error Err_8888yyy!  strLoc=="+ strLoc +" ,exc_general=="+ exc_general );
     }
   if ( iCountErrors == 0 ) {   return true; }
   else {  return false;}
   }
 internal static readonly Char[]			cArr	  = {'j', 'i', 'h', 'g', 'f', 'e', 'd', 'c', 'b', 'a'};
 internal static readonly Char[]			cSortedArr	 = {'j', 'i', 'c', 'd', 'e', 'f', 'g', 'h', 'b', 'a'};
 internal static readonly Int16[]		i16Arr				= {19, 238, 317, 6, 565, 0, -52, 60, -563, 753, 19, 238, 317};
 internal static readonly Int16[]		iSorted16Arr	= {19, 238, 317, -52, 0, 6, 565, 60, -563, 753, 19, 238, 317};
 internal static readonly Object[]	oArr			= {true, 'k', SByte.MinValue, Byte.MinValue, (short)2, 634, (long)436, (float)1.1, 1.23, "Hello World"};
 internal static readonly Char[,]		c2Arr		  = {{'a', 'b', 'c', 'd', 'e'},
								     {'k', 'l', 'm', 'n', 'o'}};
 public static void Main( String[] args )
   {
   bool bResult = false;	
   Co3770Sort_oii oCbTest = new Co3770Sort_oii();
   try
     {
     bResult = oCbTest.runTest();
     }
   catch ( Exception exc_main )
     {
     bResult = false;
     Console.WriteLine( s_strTFAbbrev + "FAiL!  Error Err_9999zzz!  Uncaught Exception caught in main(), exc_main=="+ exc_main );
     }
   if ( ! bResult )
     {
     Console.WriteLine( s_strTFAbbrev + s_strTFPath );
     Console.Error.WriteLine( " " );
     Console.Error.WriteLine( "FAiL!  "+ s_strTFAbbrev );  
     Console.Error.WriteLine( " " );
     }
   if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 11; 
   }
}
