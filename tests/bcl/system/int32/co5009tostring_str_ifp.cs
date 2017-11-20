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
public class Co5009Format_str_nfi
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strClassMethod   = "Integer4.ToString(Integer4 value, String format, NumberFormatInfo info)";
 public static String s_strTFName        = "Co5009Format_str_nfi.cs";
 public static String s_strTFAbbrev      = "Co5009";
 public static String s_strTFPath        = "";
 public Boolean runTest()
   {
   Console.Error.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc = "Loc_000oo";
   String strBaseLoc = "Loc_0000oo_";
   Int32 in4a = (Int32)0;
   Int32 in4b = (Int32)0;
   String strOut = null;
   String str2 = null;
   NumberFormatInfo nfi1 = new NumberFormatInfo();
   nfi1.CurrencySymbol = "&";  
   nfi1.CurrencyDecimalDigits = 3;
   nfi1.NegativeSign = "^";  
   nfi1.NumberDecimalDigits = 3;    
   Int32[] in4TestValues = {Int32.MinValue, 
			    -1000,
			    -99,
			    -5,
			    -0,
			    0,
			    5,
			    13,
			    101,
			    1000,
			    Int32.MaxValue
   };
   String[] strResultCFormat1 = {"(&2,147,483,648.000)", 
                                 "(&1,000.000)",
                                 "(&99.000)",
                                 "(&5.000)",
                                 "&0.000",
                                 "&0.000",
                                 "&5.000",
                                 "&13.000",
                                 "&101.000",
                                 "&1,000.000",
                                 "&2,147,483,647.000",
   };
   String[] strResultCFormat2 = {"(&2,147,483,648.0000)", 
                                 "(&1,000.0000)",
                                 "(&99.0000)",
                                 "(&5.0000)",
                                 "&0.0000",
                                 "&0.0000",
                                 "&5.0000",
                                 "&13.0000",
                                 "&101.0000",
                                 "&1,000.0000",
                                 "&2,147,483,647.0000",
   };
   String[] strResultDFormat1 = {"^2147483648", 
				 "^1000",
				 "^99",
				 "^5",
				 "0",
				 "0",
				 "5",
				 "13",
				 "101",
				 "1000",
				 "2147483647"
   };
   String[] strResultDFormat2 = {"^2147483648", 
				 "^1000",
				 "^0099",
				 "^0005",
				 "0000",
				 "0000",
				 "0005",
				 "0013",
				 "0101",
				 "1000",
				 "2147483647"
   };
   String[] strResultEFormat1 = {"^2.147484E+009", 
                                 "^1.000000E+003",
                                 "^9.900000E+001",
                                 "^5.000000E+000",
                                 "0.000000E+000",
                                 "0.000000E+000",
                                 "5.000000E+000",
                                 "1.300000E+001",
                                 "1.010000E+002",
                                 "1.000000E+003",
                                 "2.147484E+009",
   };
   String[] strResultEFormat2 = {"^2.1475e+009", 
                                 "^1.0000e+003",
                                 "^9.9000e+001",
                                 "^5.0000e+000",
                                 "0.0000e+000",
                                 "0.0000e+000",
                                 "5.0000e+000",
                                 "1.3000e+001",
                                 "1.0100e+002",
                                 "1.0000e+003",
                                 "2.1475e+009",
   };
   String[] strResultFFormat1 = {"^2147483648.000", 
				 "^1000.000",
				 "^99.000",
				 "^5.000",
				 "0.000",
				 "0.000",
				 "5.000",
				 "13.000",
				 "101.000",
				 "1000.000",
				 "2147483647.000"
   };
   String[] strResultFFormat2 = {"^2147483648.0000", 
				 "^1000.0000",
				 "^99.0000",
				 "^5.0000",
				 "0.0000",
				 "0.0000",
				 "5.0000",
				 "13.0000",
				 "101.0000",
				 "1000.0000",
				 "2147483647.0000"
   };
   String[] strResultGFormat1 = {"^2147483648", 
				 "^1000",
				 "^99",
				 "^5",
				 "0",
				 "0",
				 "5",
				 "13",
				 "101",
				 "1000",
				 "2147483647"
   };
   String[] strResultGFormat2 = {"^2.147E+09", 
				 "^1000",
				 "^99",
				 "^5",
				 "0",
				 "0",
				 "5",
				 "13",
				 "101",
				 "1000",
				 "2.147E+09"
   };
   String[] strResultNFormat1 = {"^2,147,483,648.000", 
				 "^1,000.000",
				 "^99.000",
				 "^5.000",
				 "0.000",
				 "0.000",
				 "5.000",
				 "13.000",
				 "101.000",
				 "1,000.000",
				 "2,147,483,647.000"
   };
   String[] strResultNFormat2 = {"^2,147,483,648.0000", 
				 "^1,000.0000",
				 "^99.0000",
				 "^5.0000",
				 "0.0000",
				 "0.0000",
				 "5.0000",
				 "13.0000",
				 "101.0000",
				 "1,000.0000",
				 "2,147,483,647.0000"
   };
   String[] strResultXFormat1 = {"80000000", 
                                 "FFFFFC18",
                                 "FFFFFF9D",
                                 "FFFFFFFB",
                                 "0",
                                 "0",
                                 "5",
                                 "D",
                                 "65",
                                 "3E8",
                                 "7FFFFFFF"
   };
   String[] strResultXFormat2 = {"80000000", 
                                 "FFFFFC18",
                                 "FFFFFF9D",
                                 "FFFFFFFB",
                                 "0000",
                                 "0000",
                                 "0005",
                                 "000D",
                                 "0065",
                                 "03E8",
                                 "7FFFFFFF"
   };
   String[] strResultXFormat3 = {"80000000", 
                                 "fffffc18",
                                 "ffffff9d",
                                 "fffffffb",
                                 "0000",
                                 "0000",
                                 "0005",
                                 "000d",
                                 "0065",
                                 "03e8",
                                 "7fffffff"
   };
   try {
   LABEL_860_GENERAL:
   do
     {
     strBaseLoc = "Loc_2500qi_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "F",nfi1);
       if(!strOut.Equals(strResultFFormat1[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_815jd! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_2600qi_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "F4",nfi1);
       if(!strOut.Equals(strResultFFormat2[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_193jd! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_1100ds_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc+ i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "", nfi1);
       if(!strOut.Equals(strResultGFormat1[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_293qu! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_1200er_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "G", nfi1);
       if(!strOut.Equals(strResultGFormat1[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_347ew! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_1300we_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "G4", nfi1);
       if(!strOut.Equals(strResultGFormat2[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_349ex! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_1400fs_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "C", nfi1);
       if(!strOut.Equals(strResultCFormat1[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_832ee! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_1500ez_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "C4", nfi1);
       if(!strOut.Equals(strResultCFormat2[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_273oi! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_1600nd_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "D", nfi1);
       if(!strOut.Equals(strResultDFormat1[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_901sn! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_1700eu_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "D4", nfi1);
       if(!strOut.Equals(strResultDFormat2[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_172sn! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_1800ns_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "E", nfi1);
       if(!strOut.Equals(strResultEFormat1[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_347sq! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_1900wq_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "e4", nfi1);
       if(!strOut.Equals(strResultEFormat2[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_873op! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_2000ne_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "N", nfi1);
       if(!strOut.Equals(strResultNFormat1[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_129we! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_2100qu_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "N4", nfi1);
       if(!strOut.Equals(strResultNFormat2[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_321sj! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_2200aa_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "X", nfi1);
       if(!strOut.Equals(strResultXFormat1[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_384wi! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_2300nr_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "X4", nfi1);
       if(!strOut.Equals(strResultXFormat2[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_837qo! , i=="+i+" strOut=="+strOut);
	 }
       }
     strBaseLoc = "Loc_2400qi_";
     for (int i=0; i < in4TestValues.Length;i++)
       {
       strLoc = strBaseLoc + i.ToString();
       iCountTestcases++;
       strOut = in4TestValues[i].ToString( "x4", nfi1);
       if(!strOut.Equals(strResultXFormat3[i]))
	 {
	 iCountErrors++;
	 Console.WriteLine(s_strTFAbbrev+ "Err_833jd! , i=="+i+" strOut=="+strOut);
	 }
       }
     strLoc = "Loc_248oj";
     in4a = (Int32)(-50);
     strOut = "^humbug: 50";
     iCountTestcases++;
     if(!strOut.Equals(in4a.ToString( "'humbug:' ###", nfi1)))
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_125jw! ,strOut=="+in4a.ToString( "'humbug:' ###", nfi1));
       }
     strLoc = "Loc_250ej";
     in4a = (Int32)45;
     strOut = "00045";
     iCountTestcases++;
     if(!strOut.Equals(in4a.ToString( "0000#", nfi1)))
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_279eo! ,strOut=="+in4a.ToString( "0000#", nfi1));
       }
     strLoc = "Loc_129nq";
     in4a = (Int32)29145;
     strOut = "29,145";
     iCountTestcases++;
     if(!strOut.Equals(in4a.ToString( "#,#.", nfi1)))
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_931ee! ,strOut=="+in4a.ToString( "#,#.", nfi1));
       }
     strLoc = "Loc_145hw";
     in4a = (Int32)(34);
     strOut = "test 34 test";
     iCountTestcases++;
     if(!strOut.Equals(in4a.ToString( "test #### test", nfi1)))
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_871wj! ,strOut=="+in4a.ToString( "test #### test", nfi1));
       }
     strLoc = "Loc_328au";
     in4a = (Int32)3034;
     strOut = "30.3E+002";
     iCountTestcases++;
     if(!strOut.Equals(in4a.ToString( "##.#E+000",nfi1)))
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_892wo! ,strOut=="+in4a.ToString( "##.#E+000",nfi1));
       }
     strLoc = "Loc_378nq";
     in4a = (Int32)1234;
     strOut = "ABC#1234";
     iCountTestcases++;
     if(!strOut.Equals(in4a.ToString( "'ABC#'##",nfi1)))
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_128qx! ,strOut=="+in4a.ToString( "'ABC#'##",nfi1));
       }
     strLoc = "Loc_400ne";
     in4a = (Int32)(1234);
     strOut = "01234";
     iCountTestcases++;
     if(!strOut.Equals(in4a.ToString( "00000;00;00",nfi1)))
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_539tw! ,strOut=="+in4a.ToString( "00000;000000;00",nfi1));
       }
     strLoc = "Loc_412he";
     in4a = (Int32)(-123);
     strOut = "^000123";
     iCountTestcases++;
     if(!strOut.Equals(in4a.ToString( "00;^000000;00",nfi1)))
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_739em! ,strOut=="+in4a.ToString( "00;^000000;00", nfi1));
       }
     strLoc = "Loc_415ue";
     in4a = (Int32)0;
     strOut = "000";
     iCountTestcases++;
     if(!strOut.Equals(in4a.ToString( "00;00;000",nfi1)))
       {
       iCountErrors++;
       Console.WriteLine(s_strTFAbbrev+ "Err_638kl! ,strOut=="+in4a.ToString( "00;00;000", nfi1));
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
   Co5009Format_str_nfi cbA = new Co5009Format_str_nfi();
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
