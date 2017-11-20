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
public class Co5060GetHashCode
{
 public static readonly String s_strActiveBugNums = "";
 public static readonly String s_strDtTmVer       = "";
 public static readonly String s_strClassMethod   = "DateTime.GetHashCode()";
 public static readonly String s_strTFName        = "Co5060GetHashCode.cs";
 public static readonly String s_strTFAbbrev      = "Co5060";
 public static readonly String s_strTFPath        = "";
 public virtual Boolean runTest()
   {
   Console.Error.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc = "Loc_000oo";
   String strBaseLoc;
   DateTime dt1 ;
   DateTime dt2 ;
   String strInput1;
   String strInput2;
   Int64 in8a;
   try {
   LABEL_860_GENERAL:
   do
     {
     strLoc = "Loc_111ji";
     dt1 = DateTime.Parse("01/01/1999 00:00:00");
     dt2 = DateTime.Parse("01/01/1999 00:00:01");
     iCountTestcases++;
     if(dt1.GetHashCode() == dt2.GetHashCode())
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ "Err_128nu!");
       }
     dt1 = new DateTime(2000, 08, 15, 9, 0, 1);
     dt2 = new DateTime(2000, 08, 15, 9, 0, 1);
     iCountTestcases++;
     if(dt1.GetHashCode() != dt2.GetHashCode())
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ "Err_128nu!");
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
   Co5060GetHashCode cbA = new Co5060GetHashCode();
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
