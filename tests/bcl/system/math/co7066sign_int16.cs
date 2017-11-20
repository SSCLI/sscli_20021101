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
public class Co7066Sign_Int16
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strClassMethod   = "Math.Sign(Int16)"; 
 public static String s_strTFName        = "Co7066Sign_Int16.cs";
 public static String s_strTFAbbrev      = s_strTFName.Substring(0,6);
 public static String s_strTFPath        = "";
 public bool runTest()
   {
   Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   String strLoc = "Loc_000oo";
   String strValue = String.Empty;
   int iCountErrors = 0;
   int iCountTestcases = 0;
   try
     {
     Int16 i16a;
     Int32 i32ret;			
     strLoc = "Loc_498yg";
     i16a = (Int16)0;
     i32ret = Math.Sign(i16a);
     iCountTestcases++;
     if (i32ret != 0)
       {
       iCountErrors++;
       printerr("Error_100aa Expected==0, value==" + i32ret.ToString());
       }
     strLoc = "Loc_298vy";
     i16a = 1;
     i32ret = Math.Sign(i16a);
     iCountTestcases++;
     if (i32ret != 1)
       {
       iCountErrors++;
       printerr("Error_200aa Expected==1, value==" + i32ret.ToString());
       }
     strLoc = "Loc_398vy";
     i16a = -1;
     i32ret = Math.Sign(i16a);
     iCountTestcases++;
     if (i32ret != -1)
       {
       iCountErrors++;
       printerr("Error_300aa Expected==-1, value==" + i32ret.ToString());
       }
     strLoc = "Loc_398vy";
     i16a = Int16.MaxValue;
     i32ret = Math.Sign(i16a);
     iCountTestcases++;
     if (i32ret != 1)
       {
       iCountErrors++;
       printerr("Error_400aa Expected==1, value==" + i32ret.ToString());
       }
     strLoc = "Loc_698vy";
     i16a = Int16.MinValue;
     i32ret = Math.Sign(i16a);
     iCountTestcases++;
     if (i32ret != -1)
       {
       iCountErrors++;
       printerr("Error_500aa Expected==-1, value==" + i32ret.ToString());
       }
     strLoc = "Loc_798vy";
     i16a = 7;
     i32ret = Math.Sign(i16a);
     iCountTestcases++;
     if (i32ret != 1)
       {
       iCountErrors++;
       printerr("Error_600aa Expected==1, value==" + i32ret.ToString());
       }
     strLoc = "Loc_898vy";
     i16a = -10;
     i32ret = Math.Sign(i16a);
     iCountTestcases++;
     if (i32ret != -1)
       {
       iCountErrors++;
       printerr("Error_700aa Expected==-1, value==" + i32ret.ToString());
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
   Console.WriteLine ("INFO: ("+ s_strTFAbbrev + ") "+ info);
   }
 public static void Main(String[] args)
   {
   bool bResult = false;
   Co7066Sign_Int16 cbA = new Co7066Sign_Int16();
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
