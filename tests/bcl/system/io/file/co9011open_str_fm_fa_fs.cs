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
public class Co5708Open_fm_fa
{
	public static String s_strActiveBugNums = "";
	public static String s_strDtTmVer       = "";
	public static String s_strClassMethod   = "File.OpenText(String)";
	public static String s_strTFName        = "Co5708Open_fm_fa.cs";
	public static String s_strTFAbbrev      = s_strTFName.Substring(0,6);
	public static String s_strTFPath        = Environment.CurrentDirectory;
	public bool runTest()
	{
		Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
		String strLoc = "Loc_000oo";
		String strValue = String.Empty;
		int iCountErrors = 0;
		int iCountTestcases = 0;
		try
		{
			String fileName = s_strTFAbbrev+"TestFile";
			Stream fs2 = null, fs3;
			if(File.Exists(fileName)) 
				File.Delete(fileName);
			strLoc = "Loc_2498V";
			fs2 = File.Open(fileName , FileMode.Create, FileAccess.ReadWrite, FileShare.None);
			iCountTestcases++;
			try {
				fs3 = File.Open(fileName, FileMode.Open);
				iCountErrors++;
				printerr( "Error_209uv! Shouldn't be able to open an open file");
				fs3.Close();
			} catch (IOException iexc) {
				printinfo( "Info_2gyg7! Caught expected exception, iexc=="+iexc.Message);
			} catch (Exception exc) {			   
				iCountErrors++;
				printerr( "Error_287gv! Incorrect exception thrown, exc=="+exc.ToString());
			}
			fs2.Close();
			strLoc = "Loc_f5498";
			fs2 = File.Open(fileName, FileMode.Create, FileAccess.ReadWrite, FileShare.Read);
			iCountTestcases++;			
			fs3 = File.Open(fileName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
			fs2.Write(new Byte[]{10}, 0, 1);
			fs2.Flush();
			fs3.Read(new Byte[1], 0, 1);
			iCountTestcases++;
			try {
				fs3.Write(new Byte[]{10}, 0, 1);
				iCountErrors++;
				printerr( "Error_958vc! Expected exception not thrown");
			} catch (NotSupportedException iexc) {
				printinfo( "Info_45g98! Caught expected exception, iexc=="+iexc.Message);
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_20939! Incorrect exception thrown, exc=="+exc.ToString());
			}
			fs2.Close();
			fs3.Close();
			strLoc = "Loc_2498x";
			iCountTestcases++;
			try {
				fs3 = File.Open(null, FileMode.Open, FileAccess.Write, FileShare.ReadWrite);
				iCountErrors++;
				printerr( "Error_0001! Expected exception not thrown");
			} catch (ArgumentNullException iexc) {
				printinfo( "Info_0002! Caught expected exception, iexc=="+iexc.Message);
			} catch (Exception exc) {
				iCountErrors++;
				printerr("Error_0003! Unexpected exception thrown, exc=="+exc.ToString());
			}
			fs3.Close();
			strLoc = "Loc_2498x";
			iCountTestcases++;
			try {
				fs3 = File.Open("", FileMode.Open, FileAccess.Write, FileShare.ReadWrite);
				iCountErrors++;
				printerr( "Error_1001! Expected exception not thrown");
			} catch (ArgumentException iexc) {
				printinfo( "Info_2002! Caught expected exception, iexc=="+iexc.Message);
			} catch (Exception exc) {
				iCountErrors++;
				printerr("Error_2003! Unexpected exception thrown, exc=="+exc.ToString());
			}
			fs3.Close();
			strLoc = "Loc_2498x";
			fs2 = File.Open(fileName, FileMode.Create, FileAccess.ReadWrite, FileShare.Write);
			iCountTestcases++;
			try {
				fs3 = File.Open(fileName, FileMode.Open, FileAccess.Write, FileShare.ReadWrite);
				fs3.Write(new Byte[]{1,2}, 0, 2);
			} catch (Exception exc) {
				iCountErrors++;
				printerr("Error_2980x! Unexpected exception thrown, exc=="+exc.ToString());
			}
			fs2.Close();
			fs3.Close();
			strLoc = "Loc_4897y";
			fs2 = File.Open(fileName, FileMode.Create, FileAccess.ReadWrite, FileShare.ReadWrite);
			iCountTestcases++;
			try {
				fs3 = File.Open(fileName, FileMode.Open, FileAccess.Write, FileShare.ReadWrite);
				fs2.Write(new Byte[]{1}, 0, 1);
				fs3.Close();
				fs3 = File.Open(fileName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
				fs2.Write(new Byte[]{2}, 0, 1);
				fs3.Close();
				fs3 = File.Open(fileName, FileMode.Open, FileAccess.ReadWrite, FileShare.ReadWrite);
				fs2.Write(new Byte[]{3}, 0, 1);
				fs3.Close();
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_287g9! Unexpected exception thrown, exc=="+exc.ToString());
			}
			fs2.Close();
			if(File.Exists(fileName))
				File.Delete(fileName);
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
		Co5708Open_fm_fa cbA = new Co5708Open_fm_fa();
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
