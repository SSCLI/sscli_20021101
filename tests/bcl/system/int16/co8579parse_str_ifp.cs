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
public class Co8579Parse_str_ifp
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strClassMethod   = "Int16.Parse(NumberFormatInfo nfi)";
 public static String s_strTFName        = "Co8579Parse_str_ifp.cs";
 public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
 public static String s_strTFPath        = Environment.CurrentDirectory;
 public Boolean runTest()
   {
   Console.Error.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc = "Loc_000oo";
   String strBaseLoc = "Loc_0000oo_";
   Int16 in2a = (Int16)0;
   String strOut = null;
   Int16[] in2TestValues = {Int16.MinValue, 
			    -1000,
			    -99,
			    -5,
			    -0,
			    0,
			    5,
			    13,
			    101,
			    1000,
			    Int16.MaxValue
   };
   NumberFormatInfo nfi1 = new NumberFormatInfo();
   nfi1.NegativeSign = "^"; 
   nfi1.PositiveSign = "~"; 
   try {
   strBaseLoc = "Loc_1100ds_";
   for (int i=0; i < in2TestValues.Length;i++)
     {
     strLoc = strBaseLoc+ i.ToString();
     iCountTestcases++;
     strOut = in2TestValues[i].ToString( "", nfi1);
     in2a = Int16.Parse(strOut, nfi1);
     if(in2a != in2TestValues[i])
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_293qu! , i=="+i+" in2a=="+in2a);
       }
     }
   strBaseLoc = "Loc_1300we_";
   for (int i=0; i < in2TestValues.Length;i++)
     {
     strLoc = strBaseLoc + i.ToString();
     iCountTestcases++;
     strOut = in2TestValues[i].ToString( "G5", nfi1);
     in2a = Int16.Parse(strOut, nfi1);
     if(in2a != in2TestValues[i])
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_349ex! , i=="+i+" in2a=="+in2a+" strOut=="+strOut);
       }
     }
   strOut = null;
   iCountTestcases++;
   try {
   in2a = Int16.Parse(strOut, nfi1);
   iCountErrors++;
   Console.WriteLine(s_strTFAbbrev+ "Err_273qp! , in2a=="+in2a);
   } catch (ArgumentException) {}
   catch (Exception exc) {
   iCountErrors++;
   Console.WriteLine(s_strTFAbbrev+ "Err_982qo! ,exc=="+exc);
   }
   strOut = "32768";
   iCountTestcases++;
   try {
   in2a = Int16.Parse(strOut, nfi1);
   iCountErrors++;
   Console.WriteLine(s_strTFAbbrev+ "Err_481sm! , in2a=="+in2a);
   } catch (OverflowException) {}
   catch (Exception exc) {
   iCountErrors++;
   Console.WriteLine(s_strTFAbbrev+ "Err_523eu! ,exc=="+exc);
   }
   strOut ="-32769";
   iCountTestcases++;
   try {
   in2a = Int16.Parse(strOut, nfi1);
   iCountErrors++;
   Console.WriteLine(s_strTFAbbrev+ "Err_382er! ,in2a=="+in2a);
   } catch (FormatException) {}
   catch (Exception exc) {
   iCountErrors++;
   Console.WriteLine(s_strTFAbbrev+ "Err_371jy! ,exc=="+exc);
   }
   iCountTestcases++;
   strOut = in2TestValues[0].ToString( "C", nfi1);
   try {
   in2a = Int16.Parse(strOut, nfi1);
   iCountErrors++;
   Console.WriteLine(s_strTFAbbrev+ "Err_382er! ,in2a=="+in2a);
   } catch (FormatException) {}
   catch (Exception exc) {
   iCountErrors++;
   Console.WriteLine(s_strTFAbbrev+ "Err_371jy! ,exc=="+exc);
   }
   in2a = Int16.Parse("~123", nfi1);
   iCountTestcases++;
   if(in2a != 123)
     {
     iCountErrors++;
     Console.WriteLine("Err_93475sdg! in2a=="+in2a);
     }
   nfi1.NumberNegativePattern = 3;
   strOut ="123^";
   iCountTestcases++;
   try {
   in2a = Int16.Parse(strOut, nfi1);
   iCountErrors++;
   Console.WriteLine(s_strTFAbbrev+ "Err_382er! ,in2a=="+in2a);
   } catch (FormatException) {}
   catch (Exception exc) {
   iCountErrors++;
   Console.WriteLine(s_strTFAbbrev+ "Err_371jy! ,exc=="+exc);
   }
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
   Co8579Parse_str_ifp cbA = new Co8579Parse_str_ifp();
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
