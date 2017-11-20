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
using System.Globalization;
using GenStrings;
using System;
using System.IO;
using System.Collections;
public class Co4833Copy
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strClassMethod   = "Copy()";
 public static String s_strTFName        = "Co4833Copy";
 public static String s_strTFAbbrev      = "Co5504";
 public static String s_strTFPath        = Environment.CurrentDirectory;
 public bool runTest()
   {
   Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc = "Loc_000oo";
   String strValue = String.Empty;
   try
     {
     String str1 = String.Empty, str2 = String.Empty;
     strLoc = "Loc_498hv";
     iCountTestcases++;
     str1 = String.Concat (String.Empty);
     str2 = String.Copy (str1);
     if(!str2.Equals(String.Empty))
       {
       iCountErrors++;
       printerr( "Error_498ch! incorrect string returned for null argument=="+str2);
       } 
     strLoc = "Loc_498hv.2";
     iCountTestcases++;
     if(str1 != str2)
       {
       iCountErrors++;
       printerr( "Error_498ch.2! str1 != str2; str2 == "+ str2);
       } 
     strLoc = "Loc_498hv.3";
     str1 = "Hi There";
     str2 = String.Copy (str1);
     iCountTestcases++;
     if(!str2.Equals ("Hi There"))
       {
       iCountErrors++;
       printerr( "Error_498ch.3! incorrect string returned for null argument=="+str2);
       } 
     IntlStrings intl = new IntlStrings();
     str1 = intl.GetString(12, false, true);
     str2 = String.Copy (str1);
     String str3 = str1;
     iCountTestcases++;
     if(!str2.Equals (str3))
       {
       iCountErrors++;
       printerr( "Error_498ch.3! incorrect string returned for null argument=="+str2);
       }
     strLoc = "Loc_498hv.4";
     iCountTestcases++;
     if(str1 != str2)
       {
       iCountErrors++;
       printerr( "Error_498ch.4! str1 != str2; str2 == "+ str2);
       } 
     strLoc = "Loc_498hv.5";
     iCountTestcases++;
     try {
     str2 = String.Copy (null);
     iCountErrors++;
     printerr( "Error_498ch.5! expected argnullexc; instead resturned "+str2);
     }
     catch (ArgumentNullException ) {
     }
     } catch (Exception exc_general ) {
     ++iCountErrors;
     Console.WriteLine (s_strTFAbbrev + " : Error Err_8888yyy!  strLoc=="+ strLoc +", exc_general=="+exc_general.ToString());
     }
   if ( iCountErrors == 0 ) {   return true; }
   else {  return false;}
   }
 public void printerr ( String err )
   {
   Console.WriteLine ("POINTTOBREAK: ("+ s_strTFAbbrev + ") "+ err);
   }
 public void printinfo ( String info )
   {
   Console.WriteLine ("EXTENDEDINFO: ("+ s_strTFAbbrev + ") "+ info);
   }
 public static void Main(String[] args)
   {
   bool bResult = false;
   Co4833Copy cbA = new Co4833Copy();
   try {
   bResult = cbA.runTest();
   } catch (Exception exc_main){
   bResult = false;
   Console.WriteLine(s_strTFAbbrev + " : FAiL! Error Err_9999zzz! Uncaught Exception in main(), exc_main=="+exc_main.ToString());
   }
   if (!bResult)
     {
     Console.WriteLine ("Path: "+s_strTFName + s_strTFPath);
     Console.WriteLine( " " );
     Console.WriteLine( "FAiL!  "+ s_strTFAbbrev);
     Console.WriteLine( " " );
     }
   if (bResult) Environment.ExitCode = 0; else Environment.ExitCode = 1;
   }
}
