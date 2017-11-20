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
using System.Collections;
using System.Globalization;
using System.Text;
using System.Threading;
public class Co8001IsLetterOrDigit_Char
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer   = "";
 public static String s_strClassMethod   = "Char.IsLetterOrDigit(char)";
 public static String s_strTFName= "Co8001IsLetterOrDigit_Char.cs";
 public static String s_strTFAbbrev  = s_strTFName.Substring(0,6);
 public static String s_strTFPath= "";
 public bool runTest()
   {
   Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   String strLoc = "Loc_000oo";
   String strValue = String.Empty;
   int iCountErrors = 0;
   int iCountTestcases = 0;
   try
     {
     char c;
     int i;
     strLoc = "Loc_sb8001a";
     for(i = 0; i < 48; i++)
       {
       iCountTestcases++;
       c = (char)i;
       if(Char.IsLetterOrDigit(c))
	 {
	 iCountErrors++;
	 Console.WriteLine("Err_sb8001a! Character {0} (value {1}) failed", c, i);
	 }
       }
     strLoc = "Loc_sb8001b";
     for(; i < 58; i++)
       {
       iCountTestcases++;
       c = (char)i;
       if(!Char.IsLetterOrDigit(c))
	 {
	 iCountErrors++;
	 Console.WriteLine("Err_sb8001b! Character {0} (value {1}) failed", c, i);
	 }
       }
     strLoc = "Loc_sb8001c";
     for(; i < 65; i++)
       {
       iCountTestcases++;
       c = (char)i;
       if(Char.IsLetterOrDigit(c))
	 {
	 iCountErrors++;
	 Console.WriteLine("Err_sb8001c! Character {0} (value {1}) failed", c, i);
	 }
       }
     strLoc = "Loc_sb8001d";
     for(; i < 91; i++)
       {
       iCountTestcases++;
       c = (char)i;
       if(!Char.IsLetterOrDigit(c))
	 {
	 iCountErrors++;
	 Console.WriteLine("Err_sb8001d! Character {0} (value {1}) failed", c, i);
	 }
       }
     strLoc = "Loc_sb8001e";
     for(; i < 97; i++)
       {
       iCountTestcases++;
       c = (char)i;
       if(Char.IsLetterOrDigit(c))
	 {
	 iCountErrors++;
	 Console.WriteLine("Err_sb8001e! Character {0} (value {1}) failed", c, i);
	 }
       }
     strLoc = "Loc_sb8001f";
     for(; i < 123; i++)
       {
       iCountTestcases++;
       c = (char)i;
       if(!Char.IsLetterOrDigit(c))
	 {
	 iCountErrors++;
	 Console.WriteLine("Err_sb8001f! Character {0} (value {1}) failed", c, i);
	 }
       }
     strLoc = "Loc_sb8001g";
     for(; i < 192; i++)
       {
       iCountTestcases++;
       c = (char)i;
       if( ( Char.IsLetterOrDigit(c) && !(i==170 || i==181 || i==186) ) ||
	   (!Char.IsLetterOrDigit(c) &&  (i==170 || i==181 || i==186) ) )
	 {
	 iCountErrors++;
	 Console.WriteLine("Err_sb8001g! Character {0} (value {1}) failed", c, i);
	 }
       }
     strLoc = "Loc_sb8001h";
     for(; i < 256; i++)
       {
       iCountTestcases++;
       c = (char)i;
       if( (!Char.IsLetterOrDigit(c) && !(i==215 || i==247) ) ||
	   ( Char.IsLetterOrDigit(c) &&  (i==215 || i==247) ) )
	 {
	 iCountErrors++;
	 Console.WriteLine("Err_sb8001h! Character {0} (value {1}) failed", c, i);
	 }
       }
     strLoc = "Loc_sb8001i";
     c = (char)'\xA8B';
     if(!Char.IsLetterOrDigit(c))
       {
       iCountErrors++;
       Console.WriteLine("Err_sb8001! Character {0} failed", c, 0x0A9B);
       }
     strLoc = "Loc_sb8001j";
     c = (char)'\xFD3E';
     if(Char.IsLetterOrDigit(c))
       {
       iCountErrors++;
       Console.WriteLine("Err_sb8001j! Character {0} (value {1}) failed", c, 0xFD3E);
       }
     strLoc = "Loc_sb8001k";
     int numLettersOrDigits = 0;
     int checkNum = 45641;
     for(i = 0; i < 65535; i++)
       {
       iCountTestcases++;
       c = (char)i;
       if(Char.IsLetterOrDigit(c))
	 {
	 numLettersOrDigits++;
	 }
       }
     if(numLettersOrDigits != checkNum)
       {
       int diff = Math.Abs(checkNum - numLettersOrDigits);
       iCountErrors += diff;
       Console.WriteLine("Err_sb8001k! Total number of letters/numbers is incorrect");
       Console.WriteLine("Showing " + diff + " letters/numbers " + ((diff > 0) ?
								    "more" : "less") + " than expected (expecting " + checkNum + ")");
       }
     } catch (Exception exc_general ) {
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
     Console.WriteLine("FAiL! "+s_strTFName+" ,inCountErrors=="+iCountErrors.ToString()+" , BugNums?: "+s_strActiveBugNums );
     return false;
     }
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
   Co8001IsLetterOrDigit_Char cbA = new Co8001IsLetterOrDigit_Char();
   try {
   bResult = cbA.runTest();
   } catch (Exception exc_main){
   bResult = false;
   Console.WriteLine(s_strTFAbbrev + " : FAiL! Error Err_9999zzz! Uncaught Exception in main(), exc_main=="+exc_main.ToString());
   }
   if (!bResult)
     {
     Console.WriteLine ("Path: "+s_strTFPath+"\\"+s_strTFName);
     Console.WriteLine( " " );
     Console.WriteLine( "FAiL!  "+ s_strTFAbbrev);
     Console.WriteLine( " " );
     }
   if (bResult) Environment.ExitCode = 0; else Environment.ExitCode = 1;
   }
}
