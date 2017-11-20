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
using System.Threading;
using System;
using System.IO;
public class Co2918Duration
{
 public static String strName = "TimeSpan.Duration";
 public static String strTest = "Co2918Duration";
 public static String strPath = "";
 public virtual bool runTest()
   {
   int iCountErrors = 0;
   int iCountTestcases = 0;
   Console.Error.WriteLine( strName + ": " + strTest + " runTest started..." );
   TimeSpan	ts;
   int iDays = -1;
   int iHours = -1;
   int iMinutes = -1;
   int iSeconds = -1;
   do
     {
     ++iCountTestcases;
     Console.Error.WriteLine( "[]  Construct TimeSpan object using positive hours, month and seconds" );
     try
       {
       iDays = 4;
       iHours = 12;
       iMinutes = 23;
       iSeconds = 34;
       ts = new TimeSpan( iDays, iHours, iMinutes, iSeconds );
       TimeSpan tsDuration = ts.Duration();
       if ( iDays != tsDuration.Days )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Days <" + iDays + "> ";
	 strInfo = strInfo + "Returned Days <" + tsDuration.Days + "> ";
	 Console.WriteLine( strTest+ "E_101a: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       if ( iHours != tsDuration.Hours )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Hours <" + iHours + "> ";
	 strInfo = strInfo + "Returned Hours <" + tsDuration.Hours + "> ";
	 Console.WriteLine( strTest+ "E_101a: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       if ( iMinutes != tsDuration.Minutes )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Minutes <" + iMinutes + "> ";
	 strInfo = strInfo + "Returned Minutes <" + tsDuration.Minutes + "> ";
	 Console.WriteLine( strTest+ "E_101b: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       if ( iSeconds != tsDuration.Seconds )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Seconds <" + iSeconds + "> ";
	 strInfo = strInfo + "Returned Seconds <" + tsDuration.Seconds + "> ";
	 Console.WriteLine( strTest+ "E_101c: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       }
     catch (Exception ex)
       {
       Console.WriteLine( strTest+ "E_10001: Unexpected Exception: " + ex.ToString() );
       ++iCountErrors;
       break;
       }
     ++iCountTestcases;
     Console.Error.WriteLine( "[]  Construct TimeSpan object using negative hours, month and seconds" );
     try
       {
       iDays = -4;
       iHours = -12;
       iMinutes = -23;
       iSeconds = -34;
       ts = new TimeSpan( iDays, iHours, iMinutes, iSeconds );
       TimeSpan tsDuration = ts.Duration();
       if ( iDays != -tsDuration.Days )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Days <" + -iDays + "> ";
	 strInfo = strInfo + "Returned Days <" + tsDuration.Days + "> ";
	 Console.WriteLine( strTest+ "E_101a: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       if ( iHours != -tsDuration.Hours )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Hours <" + -iHours + "> ";
	 strInfo = strInfo + "Returned Hours <" + tsDuration.Hours + "> ";
	 Console.WriteLine( strTest+ "E_202a: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       if ( iMinutes != -tsDuration.Minutes )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Minutes <" + -iMinutes + "> ";
	 strInfo = strInfo + "Returned Minutes <" + tsDuration.Minutes + "> ";
	 Console.WriteLine( strTest+ "E_202b: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       if ( iSeconds != -tsDuration.Seconds )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Seconds <" + -iSeconds + "> ";
	 strInfo = strInfo + "Returned Seconds <" + tsDuration.Seconds + "> ";
	 Console.WriteLine( strTest+ "E_202c: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       }
     catch (Exception ex)
       {
       Console.WriteLine( strTest+ "E_20202: Unexpected Exception: " + ex.ToString() );
       ++iCountErrors;
       break;
       }
     ++iCountTestcases;
     Console.Error.WriteLine( "[]  Construct TimeSpan object using zero hours, month and seconds" );
     try
       {
       iDays = 0;
       iHours = 0;
       iMinutes = 0;
       iSeconds = 0;
       ts = new TimeSpan( iDays, iHours, iMinutes, iSeconds );
       TimeSpan tsDuration = ts.Duration();
       if ( iDays != tsDuration.Days )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Days <" + iDays + "> ";
	 strInfo = strInfo + "Returned Days <" + tsDuration.Days + "> ";
	 Console.WriteLine( strTest+ "E_101a: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       if ( iHours != ts.Hours )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Hours <" + iHours + "> ";
	 strInfo = strInfo + "Returned Hours <" + ts.Hours + "> ";
	 Console.WriteLine( strTest+ "E_303a: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       if ( iMinutes != ts.Minutes )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Minutes <" + iMinutes + "> ";
	 strInfo = strInfo + "Returned Minutes <" + ts.Minutes + "> ";
	 Console.WriteLine( strTest+ "E_303a: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       if ( iSeconds != ts.Seconds )
	 {
	 String strInfo = strTest + " error: ";
	 strInfo = strInfo + "Expected Seconds <" + iSeconds + "> ";
	 strInfo = strInfo + "Returned Seconds <" + ts.Seconds + "> ";
	 Console.WriteLine( strTest+ "E_303a: " + strInfo );
	 ++iCountErrors;
	 break;
	 }
       }
     catch (Exception ex)
       {
       Console.WriteLine( strTest+ "E_30303: Unexpected Exception: " + ex.ToString() );
       ++iCountErrors;
       break;
       }
     }
   while ( false );
   Console.Error.Write( strName );
   Console.Error.Write( ": " );
   if ( iCountErrors == 0 )
     {
     Console.Error.WriteLine( strTest + " iCountTestcases==" + iCountTestcases + " paSs" );
     return true;
     }
   else
     {
     System.String strFailMsg = null;
     Console.WriteLine( strTest+ strPath );
     Console.WriteLine( strTest+ "FAiL" );
     Console.Error.WriteLine( strTest + " iCountErrors==" + iCountErrors );
     return false;
     }
   }
 public static void Main( String[] args )
   {
   bool bResult = false;	
   Co2918Duration oCbTest = new Co2918Duration();
   try
     {
     bResult = oCbTest.runTest();
     }
   catch( Exception ex )
     {
     bResult = false;
     Console.WriteLine( strTest+ strPath );
     Console.WriteLine( strTest+ "E_1000000" );
     Console.WriteLine( strTest+ "FAiL: Uncaught exception detected in Main()" );
     Console.WriteLine( strTest+ ex.ToString() );
     }
   if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 1;
   }
} 
