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
using System.Collections;
using System.Threading;
using System.Reflection;
public class Co3752SetValue_viii
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strClassMethod   = "Array.SetValue(Object, int, int, int)";
 public static String s_strTFName        = "Co3752SetValue_viii";
 public static String s_strTFAbbrev      = "Co3752";
 public static String s_strTFPath        = "";
 public bool runTest()
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
   Type tpValue;
   Int32[,,] iArr;
   Array objArr;
   Hashtable hshArrayValue;
   ArrayList alst1 = new ArrayList();
   try
     {
     LABEL_860_GENERAL:
     do
       {
       tpValue = typeof(Int32);
       iArr = (Int32[,,])Array.CreateInstance(tpValue, 10, 10, 10);
       for(int i = 0; i <= iArr.GetUpperBound(0); i++)
	 for(int j=0; j <= iArr.GetUpperBound(1); j++)
	   for(int k=0; k<= iArr.GetUpperBound(2); k++)
	     iArr.SetValue( (Object) (i*j*k), i, j, k);
       for(int i = 0; i <= iArr.GetUpperBound(0); i++)
	 {
	 for(int j=0; j <= iArr.GetUpperBound(1); j++)
	   {
	   for(int k=0; k<= iArr.GetUpperBound(2); k++)
	     {
	     ++iCountTestcases;
	     if((Int32) iArr.GetValue(i, j, k) !=i*j*k)
	       {
	       ++iCountErrors;
	       Console.WriteLine( s_strTFAbbrev + "Err_630jm_" + i + "_" + j + "_" + k + ", i*j*k==" + (Int32) iArr.GetValue(i,j,k));
	       }
	     }
	   }
	 }
       strLoc="Loc_645sa";
       int[] iTempArr = new int[3];
       for(int i = 0; i <= iArr.GetUpperBound(0); i++)
	 {
	 for(int j=0; j <= iArr.GetUpperBound(1); j++)
	   {
	   for(int k=0; k<= iArr.GetUpperBound(2); k++)
	     {
	     iTempArr[0]=i;
	     iTempArr[1]=j;
	     iTempArr[2]=k;
	     iArr.SetValue( (Object) (i*j*k), i, j, k);
	     }
	   }
	 }
       strLoc="Loc_362fs";
       for(int i = 0; i <= iArr.GetUpperBound(0); i++)
	 {
	 strLoc="Loc_362fs.2";
	 for(int j=0; j <= iArr.GetUpperBound(1); j++)
	   {
	   strLoc="Loc_362fs.3";
	   for(int k=0; k<= iArr.GetUpperBound(2); k++)
	     {
	     ++iCountTestcases;
	     if((Int32) iArr.GetValue(i,j,k)!=i*j*k)
	       {
	       strLoc="Loc_362fs.4";
	       ++iCountErrors;
	       Console.WriteLine( s_strTFAbbrev + "Err_763cs_" + i + "_" + j + "_" + k + ", i*j*k==" + (Int32) iArr.GetValue(i,j,k));
	       }
	     }
	   }
	 }
       hshArrayValue = new Hashtable();
       GoFillSomeTestValues(hshArrayValue);
       for(int ii=0; ii < ClassTypes.Length; ii++)
	 {
	 try
	   {
	   strLoc="Loc_838sf_" + ii;
	   tpValue = ClassTypes[ii];
	   objArr  = Array.CreateInstance(tpValue, 3, 3, 3);
	   if(hshArrayValue.ContainsKey(ClassTypes[ii].Name))
	     {
	     strLoc="Loc_276dd_" + ii;
	     alst1 = (ArrayList)hshArrayValue[ClassTypes[ii].Name];
	     for (int i=0; i<3; i++) {
	     for (int j=0; j<3; j++) {
	     for (int k=0; k<3; k++) {
	     strLoc="loc_649fs_" + ii + "_" + i + "_" + j + "_" + k;
	     objArr.SetValue ((Object) alst1[i+j+k], i, j, k);
	     }
	     }
	     }
	     Object val = null;
	     for (int i=0; i<3; i++) {
	     for (int j=0; j<3; j++) {
	     for (int k=0; k<3; k++) {
	     strLoc="loc_ui334_" + ii + "_" + i + "_" + j + "_" + k;
	     val = objArr.GetValue (i, j, k);
	     if (!val.Equals (alst1[i+j+k]))
	       {
	       ++iCountErrors;
	       Console.WriteLine( s_strTFAbbrev + ", Err_673sa_" + ii + "_" + i + "_" + j + "_" + k);
	       Console.Write ("Ret Value == " + val);
	       }
	     }
	     }
	     }
	     ++iCountTestcases;
	     try
	       {
	       objArr.SetValue( (Object) 0, objArr.GetLowerBound(0)-1, objArr.GetLowerBound(1), objArr.GetLowerBound(2));
	       ++iCountErrors;
	       Console.WriteLine( s_strTFAbbrev + ", Err_763sr! Exception not thrown");
	       }
	     catch(IndexOutOfRangeException ex)
	       {
	       }
	     catch(Exception ex)
	       {
	       ++iCountErrors;
	       Console.WriteLine( s_strTFAbbrev + ", Err_874sq! wrong exception thrown == " + ex);
	       }
	     ++iCountTestcases;
	     try
	       {
	       objArr.SetValue( (Object) 0, objArr.GetLowerBound(0), objArr.GetLowerBound(1)-1, objArr.GetLowerBound(2));
	       ++iCountErrors;
	       Console.WriteLine( s_strTFAbbrev + ", Err_763sr! Exception not thrown");
	       }
	     catch(IndexOutOfRangeException ex)
	       {
	       }
	     catch(Exception ex)
	       {
	       ++iCountErrors;
	       Console.WriteLine( s_strTFAbbrev + ", Err_874sq! wrong exception thrown == " + ex);
	       }
	     ++iCountTestcases;
	     try
	       {
	       objArr.SetValue( (Object) 0, objArr.GetLowerBound(0), objArr.GetLowerBound(1), objArr.GetLowerBound(2)-1);
	       ++iCountErrors;
	       Console.WriteLine( s_strTFAbbrev + ", Err_763sr! Exception not thrown");
	       }
	     catch(IndexOutOfRangeException ex)
	       {
	       }
	     catch(Exception ex)
	       {
	       ++iCountErrors;
	       Console.WriteLine( s_strTFAbbrev + ", Err_874sq! wrong exception thrown == " + ex);
	       }
	     }
	   }
	 catch(Exception ex)
	   {
	   ++iCountErrors;
	   Console.Error.WriteLine( " Error E_972qr! Unexpected Exception thrown == " + ex.ToString() + ", strLoc = " + strLoc);
	   Console.WriteLine ("Current Class: " + ClassTypes[ii].FullName);
	   }
	 }
       tpValue = typeof(Int32);
       iArr = (Int32[,,])Array.CreateInstance(tpValue, 10, 10, 10);
       for(int i = 0; i <= iArr.GetUpperBound(0); i++)
	 for(int j=0; j <= iArr.GetUpperBound(1); j++)
	   for(int k=0; k<= iArr.GetUpperBound(2); k++)
	     iArr.SetValue( (Object) (i*j*k), i, j, k);
       try
	 {
	 ++iCountTestcases;
	 iArr.SetValue( (Object) 0, iArr.GetLowerBound(0), iArr.GetLowerBound(1));
	 ++iCountErrors;
	 Console.WriteLine( s_strTFAbbrev + "Err_763sy!  exzception not thrown");
	 }
       catch(ArgumentException ex)
	 {
	 }
       catch(Exception ex)
	 {
	 ++iCountErrors;
	 Console.WriteLine( s_strTFAbbrev + "Err_743af!  wrong exception thrown=="+ ex);
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
 static Type [] ClassTypes = {
   typeof (System.DBNull),
   typeof (System.Boolean),
   typeof (System.Char),
   typeof (System.SByte),
   typeof (System.Byte),
   typeof (System.Int16),
   typeof (System.UInt16),
   typeof (System.Int32),
   typeof (System.UInt32),
   typeof (System.Int64),
   typeof (System.UInt64),
   typeof (System.Single),
   typeof (System.Double),
   typeof (System.String),
   typeof (System.DateTime),
   typeof (System.TimeSpan),
   typeof (System.Decimal),
   typeof (System.Decimal),
   typeof (System.Reflection.Missing),
   typeof (System.DBNull),
   typeof (System.Object),
   typeof (Simple),
   typeof (System.DBNull[]),
   Type.GetType("System.Void[]"),
   typeof (System.Boolean[]),
   typeof (System.Char[]),
   typeof (System.SByte[]),
   typeof (System.Byte[]),
   typeof (System.Int16[]),
   typeof (System.UInt16[]),
   typeof (System.Int32[]),
   typeof (System.UInt32[]),
   typeof (System.Int64[]),
   typeof (System.UInt64[]),
   typeof (System.Single[]),
   typeof (System.Double[]),
   typeof (System.String[]),
   typeof (System.DateTime[]),
   typeof (System.TimeSpan[]),
   typeof (System.Decimal[]),
   typeof (System.Decimal[]),
   typeof (System.Reflection.Missing[]),
   typeof (System.DBNull[]),
   typeof (System.Object[]),
   typeof (Simple[]),
   typeof (System.DBNull[][]),
   Type.GetType("System.Void[][]"),
   typeof (System.Boolean[][]),
   typeof (System.Char[][]),
   typeof (System.SByte[][]),
   typeof (System.Byte[][]),
   typeof (System.Int16[][]),
   typeof (System.UInt16[][]),
   typeof (System.Int32[][]),
   typeof (System.UInt32[][]),
   typeof (System.Int64[][]),
   typeof (System.UInt64[][]),
   typeof (System.Single[][]),
   typeof (System.Double[][]),
   typeof (System.String[][]),
   typeof (System.DateTime[][]),
   typeof (System.TimeSpan[][]),
   typeof (System.Decimal[][]),
   typeof (System.Decimal[][]),
   typeof (System.Reflection.Missing[][]),
   typeof (System.DBNull[][]),
   typeof (System.Object[][]),
   typeof (Simple[][]),
   typeof (System.DBNull[][][]),
   Type.GetType("System.Void[][][]"),
   typeof (System.Boolean[][][]),
   typeof (System.Char[][][]),
   typeof (System.SByte[][][]),
   typeof (System.Byte[][][]),
   typeof (System.Int16[][][]),
   typeof (System.UInt16[][][]),
   typeof (System.Int32[][][]),
   typeof (System.UInt32[][][]),
   typeof (System.Int64[][][]),
   typeof (System.UInt64[][][]),
   typeof (System.Single[][][]),
   typeof (System.Double[][][]),
   typeof (System.String[][][]),
   typeof (System.DateTime[][][]),
   typeof (System.TimeSpan[][][]),
   typeof (System.Decimal[][][]),
   typeof (System.Decimal[][][]),
   typeof (System.Reflection.Missing[][][]),
   typeof (System.DBNull[][][]),
   typeof (System.Object[][][]),
   typeof (Simple[][][]),
 };
 static Boolean[,,]	bArr		= {{{true, false, true}, {false, true, true}, {true, true, false}},
					   {{false, true, false}, {true, false, false}, {false, true, false}},
					   {{true, false, true}, {false, false, false}, {true, true, true}}};
 static Char[,,]		cArr		  = {{{'a', 'b', 'c'}, {'d', 'e', 'f'}, {'d', 'e', 'f'}},
						     {{'f', 'g', 'h'}, {'d', 'e', 'f'}, {'d', 'e', 'f'}},
						     {{'k', 'l', 'm'}, {'d', 'e', 'f'}, {'d', 'e', 'f'}}};
 static SByte[,,]		sbtArr	= {{{SByte.MinValue, -5, 0}, {5, SByte.MaxValue, -10}, {5, SByte.MaxValue, -10}},
					   {{SByte.MinValue, -5, 0}, {5, SByte.MaxValue, -10}, {5, SByte.MaxValue, -10}},
					   {{SByte.MinValue, -5, 0}, {5, SByte.MaxValue, -10}, {5, SByte.MaxValue, -10}}};
 static Byte[,,]			btArr		= {{{Byte.MinValue, 0, 5}, {100, Byte.MaxValue, 10}, {100, Byte.MaxValue, 10}},
							   {{Byte.MinValue, 0, 5}, {100, Byte.MaxValue, 10}, {100, Byte.MaxValue, 10}},
							   {{Byte.MinValue, 0, 5}, {100, Byte.MaxValue, 10}, {100, Byte.MaxValue, 10}}};
 static Int16[,,]		i16Arr	= {{{19, 565, 0}, {-52, 60, 64}, {-52, 60, 64}},
					   {{19, 238, 317}, {-52, 60, 64}, {-52, 60, 64}},
					   {{0, -52, 60}, {-52, 60, 64}, {-52, 60, 64}}};
 static Int32[,,]		i32Arr	= {{{19, 238, 317}, {6, 565, -563}, {19, 238, 317}},
					   {{0, -52, 60}, {6, 565, -563}, {19, 238, 317}},
					   {{19, 238, 317}, {6, 565, -563}, {19, 238, 317}}};
 static Int64[,,]		i64Arr	= {{{-530, Int64.MinValue, Int32.MinValue}, {Int16.MinValue, -127, 486}, {Int16.MinValue, -127, 486}},
					   {{0, Int64.MaxValue, Int32.MaxValue}, {Int16.MinValue, -127, 486}, {Int16.MinValue, -127, 486}},
					   {{-530, Int64.MinValue, Int32.MinValue}, {Int16.MinValue, -127, 486}, {Int16.MinValue, -127, 486}}};
 static Single[,,]	fArr		  = {{{-1.2e23f, 1.2e-32f, -1.23f}, {0.0f, 45.463f, Single.MaxValue}, {0.0f, 45.463f, Single.MinValue}},
					     {{1.23e23f, 1.23f, 0.0f}, {0.0f, 45.463f, Single.MaxValue}, {0.0f, 45.463f, Single.MinValue}},
					     {{1.23e23f, 1.23f, 0.0f}, {0.0f, 45.463f, Single.MaxValue}, {0.0f, 45.463f, Single.MinValue}}};
 static Double[,,]	dArr			= {{{-1.2e23, 1.2e-32, -1.23}, {0.0, 3.4, Double.MaxValue}, {0.0, 3.4, Double.MinValue}},
						   {{-1.2e23, 1.2e-32, -1.23}, {0.0, 3.4, Double.MaxValue}, {0.0, 3.4, Double.MinValue}},
						   {{1.23e23, 1.23, 0.0}, {0.0, 3.4, Double.MaxValue}, {0.0, 3.4, Double.MinValue}}};
 static String[,,]	strArr		= {{{"This", "is", "all"}, {"I", "have", "to"}, {"say", "until", "I"}},
					   {{"a", "good", "enough"}, {"reason", "to", "open"}, {"my", "mouth", "inadvertently"}},
					   {{"in", "a", "manner"}, {"not", "consistent", "with"}, {"the", "Working", "of"}}};
 static Object[,,]	oArr			= {{{true, 'k', SByte.MinValue}, {Byte.MinValue, (short)2, "Hello World"}, {1.23, 2.45, 3.4}},
						   {{634, (long)436, (float)1.1}, {Byte.MinValue, (short)2, "Hello World"}, {1.23, 2.45, 3.4}},
						   {{false, 1.23, 0.0}, {Byte.MinValue, (short)2, "Hello World"}, {1.23, 2.45, 3.4}}};
 private void GoFillSomeTestValues(Hashtable hshArrayValue)
   {
   String strLoc="Loc_666fs.0";
   try {
   ArrayList alst = new ArrayList();
   for(int ii=0; ii < bArr.GetLength(0); ii++)
     for(int jj=0; jj < bArr.GetLength(1); jj++)
       for(int kk=0; kk < bArr.GetLength(2); kk++)
	 alst.Add(bArr[ii, jj, kk]);
   hshArrayValue.Add(typeof(Boolean).Name, alst);
   strLoc="Loc_666fs.1";
   alst = new ArrayList();
   for(int ii=0; ii < cArr.GetLength(0); ii++)
     for(int jj=0; jj < cArr.GetLength(1); jj++)
       for(int kk=0; kk < cArr.GetLength(2); kk++)
	 alst.Add(cArr[ii, jj, kk]);
   hshArrayValue.Add(typeof(Char).Name, alst);
   strLoc="Loc_666fs.2";
   alst = new ArrayList();
   for(int ii=0; ii < sbtArr.GetLength(0); ii++)
     for(int jj=0; jj < sbtArr.GetLength(1); jj++)
       for(int kk=0; kk < sbtArr.GetLength(2); kk++)
	 alst.Add(sbtArr[ii, jj, kk]);
   hshArrayValue.Add(typeof(SByte).Name, alst);
   strLoc="Loc_666fs.3";
   alst = new ArrayList();
   for(int ii=0; ii < btArr.GetLength(0); ii++)
     for(int jj=0; jj < btArr.GetLength(1); jj++)
       for(int kk=0; kk < btArr.GetLength(2); kk++)
	 alst.Add(btArr[ii, jj, kk]);
   hshArrayValue.Add(typeof(Byte).Name, alst);
   strLoc="Loc_666fs.4";
   alst = new ArrayList();
   for(int ii=0; ii < i16Arr.GetLength(0); ii++)
     for(int jj=0; jj < i16Arr.GetLength(1); jj++)
       for(int kk=0; kk < i16Arr.GetLength(2); kk++)
	 alst.Add(i16Arr[ii, jj, kk]);
   hshArrayValue.Add(typeof(Int16).Name, alst);
   strLoc="Loc_666fs.5";
   alst = new ArrayList();
   for(int ii=0; ii < i32Arr.GetLength(0); ii++)
     for(int jj=0; jj < i32Arr.GetLength(1); jj++)
       for(int kk=0; kk < i32Arr.GetLength(2); kk++)
	 alst.Add(i32Arr[ii, jj, kk]);
   hshArrayValue.Add(typeof(Int32).Name, alst);
   strLoc="Loc_666fs.6";
   alst = new ArrayList();
   for(int ii=0; ii < i64Arr.GetLength(0); ii++)
     for(int jj=0; jj < i64Arr.GetLength(1); jj++)
       for(int kk=0; kk < i64Arr.GetLength(2); kk++)
	 alst.Add(i64Arr[ii, jj, kk]);
   hshArrayValue.Add(typeof(Int64).Name, alst);
   strLoc="Loc_666fs.7";
   alst = new ArrayList();
   for(int ii=0; ii < fArr.GetLength(0); ii++)
     for(int jj=0; jj < fArr.GetLength(1); jj++)
       for(int kk=0; kk < fArr.GetLength(2); kk++)
	 alst.Add(fArr[ii, jj, kk]);
   hshArrayValue.Add(typeof(Single).Name, alst);
   strLoc="Loc_666fs.8";
   alst = new ArrayList();
   for(int ii=0; ii < dArr.GetLength(0); ii++)
     for(int jj=0; jj < dArr.GetLength(1); jj++)
       for(int kk=0; kk < dArr.GetLength(2); kk++)
	 alst.Add(dArr[ii, jj, kk]);
   hshArrayValue.Add(typeof(Double).Name, alst);
   strLoc="Loc_666fs.9";
   alst = new ArrayList();
   for(int ii=0; ii < strArr.GetLength(0); ii++)
     for(int jj=0; jj < strArr.GetLength(1); jj++)
       for(int kk=0; kk < strArr.GetLength(2); kk++)
	 alst.Add(strArr[ii, jj, kk]);
   hshArrayValue.Add(typeof(String).Name, alst);
   strLoc="Loc_666fs.11";
   alst = new ArrayList();
   for(int ii=0; ii < oArr.GetLength(0); ii++)
     for(int jj=0; jj < oArr.GetLength(1); jj++)
       for(int kk=0; kk < oArr.GetLength(2); kk++)
	 alst.Add(oArr[ii, jj, kk]);
   hshArrayValue.Add(typeof(Object).Name, alst);
   }
   catch (Exception exc) {
   Console.WriteLine( "FAiL!  Error Err_6666zzz! Exception caught in GoFillSomeTestValues(), Location==" + strLoc + "; exc=="+ exc );
   }
   }
 public static void Main( String[] args )
   {
   bool bResult = false;	
   Co3752SetValue_viii oCbTest = new Co3752SetValue_viii();
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
class Simple
{
 Simple() { m_oObject = "Hello World";}
 Object m_oObject;
}
