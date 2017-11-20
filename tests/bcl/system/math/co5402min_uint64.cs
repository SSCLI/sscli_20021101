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
public class Co5402Min_UInt64
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strClassMethod   = "Math.Min(UInt64,UInt64)";
 public static String s_strTFName        = "Co5402Min_UInt64.cs";
 public static String s_strTFAbbrev      = "Co5402";
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
     UInt64 i64a, i64b, i64ret;
     strLoc = "Loc_09utg";
     i64a = UInt64.Parse("0");
     i64b = UInt64.Parse("0");
     i64ret = Math.Min(i64a, i64b);
     iCountTestcases++;
     if(i64ret != UInt64.Parse("0"))
       {
       iCountErrors++;
       printerr("Error_20xxu");
       }
     strLoc = "Loc_209uc";
     i64a = UInt64.Parse("1");
     i64b = UInt64.Parse("0");
     i64ret = Math.Min(i64a,i64b);
     iCountTestcases++;
     if(i64ret != UInt64.Parse("0"))
       {
       iCountErrors++;
       printerr("Error_230uc! Expected==0 , value=="+i64ret.ToString());
       }
     strLoc = "Loc_209cc";
     i64a  = UInt64.Parse("0");
     i64b = UInt64.Parse("1");
     i64ret = Math.Min(i64a, i64b);
     iCountTestcases++;
     if(i64ret != UInt64.Parse("0"))
       {
       iCountErrors++;
       printerr("Error_948nc! Expected==1 , value=="+i64ret.ToString());
       }
     strLoc = "Loc_20uxw";
     i64a = UInt64.MaxValue;
     i64b = UInt64.MinValue;
     i64ret = Math.Min(i64a, i64b);
     iCountTestcases++;
     if(i64ret != UInt64.MinValue)
       {
       iCountErrors++;
       printerr("Error_2908x! expected=="+UInt64.MinValue.ToString()+" , value=="+i64ret.ToString());
       }
     strLoc = "Loc_6745x";
     i64a = UInt64.Parse("10");
     i64b = UInt64.Parse("9");
     i64ret = Math.Min(i64a, i64b);
     iCountTestcases++;
     if(i64ret != UInt64.Parse("9"))
       {
       iCountErrors++;
       printerr("Error_298xz! Expected==10 , value=="+i64ret.ToString());
       }
     strLoc = "Loc_298yx";
     i64a = UInt64.Parse("9");
     i64b = UInt64.Parse("10");
     i64ret = Math.Min(i64a, i64b);
     iCountTestcases++;
     if(i64ret != UInt64.Parse("9"))
       {
       iCountErrors++;
       printerr("Error_298di! Expeceted==10 , value=="+i64ret.ToString());
       }
     strLoc = "Loc_90uxf";
     i64a = UInt64.MaxValue;
     i64b = UInt64.MaxValue;
     i64ret = Math.Min(i64a, i64b);
     iCountTestcases++;
     if(i64ret != UInt64.MaxValue)
       {
       iCountErrors++;
       printerr("Error_298xq! Expected=="+UInt64.MaxValue.ToString()+" , value=="+i64ret.ToString());
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
     Console.WriteLine("FAiL! "+s_strTFName+" ,iCountErrors=="+iCountErrors.ToString()+" , BugNums?: "+s_strActiveBugNums );
     return false;
     }
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
   Co5402Min_UInt64 cbA = new Co5402Min_UInt64();
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
