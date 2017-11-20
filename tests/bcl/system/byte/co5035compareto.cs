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
public class Co5035CompareTo
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strClassMethod   = "Byte.CompareTo()";
 public static String s_strTFName        = "Co5035CompareTo.cs";
 public static String s_strTFAbbrev      = "Co5035";
 public static String s_strTFPath        = "";
 public Boolean runTest()
   {
   Console.Error.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc = "Loc_000oo";
   String strBaseLoc = "";
   Byte byt1a = (Byte)0;
   Byte byt1b = (Byte)0;
   try {
   do
     {
     strLoc = "Loc_122fj";
     byt1a = (Byte)(5);
     byt1b = (Byte)(5);
     iCountTestcases++;
     if(byt1a.CompareTo(byt1b) != 0)
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev +"Err_873ye!");
       }
     strLoc = "Loc_312je";
     byt1a = Byte.MinValue;
     byt1b = Byte.MinValue;
     iCountTestcases++;
     if(byt1a.CompareTo(byt1b) != 0)
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev +"Err_399ad!");
       }
     strLoc = "Loc_348je";
     byt1a = (Byte)0;
     byt1b = (Byte)0;
     iCountTestcases++;
     if(byt1a.CompareTo(byt1b) != 0)
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_838su! , byt1a=="+byt1a);
       }
     strLoc = "Loc_938jd";
     byt1a = Byte.MaxValue;
     byt1b = Byte.MinValue;
     iCountTestcases++;
     if(byt1a.CompareTo(byt1b) <= 0)
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_387aj!");
       }
     strLoc = "Loc_578sj";
     byt1a = (Byte)100;
     byt1b = (Byte)99;
     iCountTestcases++;
     if(byt1a.CompareTo(byt1b) <=0)
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_428ju!");
       }
     strLoc = "Loc_743je";
     byt1a = (Byte)10;
     byt1b = (Byte)11;
     iCountTestcases++;
     if(byt1a.CompareTo(byt1b) >= 0)
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_218sk!");
       }
     strLoc = "Loc_939eu";
     byt1a = Byte.MinValue;
     byt1b = Byte.MaxValue;
     iCountTestcases++;
     if(byt1a.CompareTo(byt1b) >= 0)
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_377as!");
       }
     strLoc = "Loc_377su";
     iCountTestcases++;
     try {
     byt1a.CompareTo(null);
     } catch (ArgumentException ){
     iCountErrors++;
     Console.WriteLine(s_strTFAbbrev+ "Err_382hf! , return=="+byt1a.CompareTo(null));
     }
     strBaseLoc = "Loc_1200dh_";
     Byte[] bytArr = {
       (Byte)0,
       (Byte)5,
       (Byte)10,
       (Byte)20,
       (Byte)59,
       (Byte)120,
       Byte.MaxValue};
     for (int ii = 0; ii < bytArr.Length ; ii++)
       {
       strLoc = strBaseLoc + ii.ToString();
       iCountTestcases++;
       if(bytArr[ii].CompareTo(null) <= 0)
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_237fe! ,bytArr=="+bytArr[ii].CompareTo(null));
	 }
       }
     } while (false);
   } catch (Exception exc_general ) {
   ++iCountErrors;
   Console.WriteLine(s_strTFAbbrev +" Error Err_8888yyy!  strLoc=="+ strLoc +", exc_general=="+exc_general);
   }
   if ( iCountErrors == 0 )
     {
     Console.Error.WriteLine( "paSs.   "+s_strTFPath +" "+s_strTFName+" ,iCountTestcases=="+iCountTestcases);
     return true;
     }
   else
     {
     Console.Error.WriteLine("FAiL!   "+s_strTFPath+" "+s_strTFName+" ,iCountErrors=="+iCountErrors+" , BugNums?: "+s_strActiveBugNums );
     return false;
     }
   }
 public static void Main(String[] args) 
   {
   Boolean bResult = false;
   Co5035CompareTo cbA = new Co5035CompareTo();
   try {
   bResult = cbA.runTest();
   } catch (Exception exc_main){
   bResult = false;
   Console.WriteLine(s_strTFAbbrev+ "FAiL! Error Err_9999zzz! Uncaught Exception in main(), exc_main=="+exc_main);
   }
   if (!bResult)
     {
     Console.WriteLine(s_strTFName+ s_strTFPath);
     Console.Error.WriteLine( " " );
     Console.Error.WriteLine( "FAiL!  "+ s_strTFAbbrev);
     Console.Error.WriteLine( " " );
     }
   if (bResult) Environment.ExitCode = 0; else Environment.ExitCode = 1;
   }
}
