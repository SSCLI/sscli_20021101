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
using System.Text;
using System.IO;
using System.Collections;
using System.Globalization;
public class Co5567Read
{
	public static String s_strActiveBugNums = "";
	public static String s_strDtTmVer       = "";
	public static String s_strClassMethod   = "StringReader.Read(String)";
	public static String s_strTFName        = "Co5567Read.cs";
	public static String s_strTFAbbrev      = "Co5567";
	public static String s_strTFPath        = Environment.CurrentDirectory;
	public bool runTest()
	{
		Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
		int iCountErrors = 0;
		int iCountTestcases = 0;
		String strLoc = "Loc_000oo";
		String strValue = String.Empty;
		try
		{
			StringReader sr;
			Int32 i32;
			String str1;
			strLoc = "Loc_98yg7";
            iCountTestcases++;
            try{
                sr = new StringReader(null);
                i32 = sr.Read();
                iCountErrors++;
                printerr( "Error_198yz! Incorrect value returned, i32=="+i32);
            }catch( ArgumentNullException e ){
                printinfo("Expected exception occured" + e.Message);
            }catch(Exception e ){
                iCountErrors++;
                printerr("Err_3255!! Unexpected exception occured... " + e.ToString() );
            }
			strLoc = "Loc_4790s";
			sr = new StringReader(String.Empty);
			iCountTestcases++;
			i32 = sr.Read();
			if(i32 != -1) {
				iCountErrors++;
				printerr( "Error_099xa! Incorrect value returned, i32=="+i32);
			} 
			strLoc = "Loc_8388x";
			str1 = "Hello\0\t\v   \\ World";
			sr = new StringReader(str1);
			for(int i = 0 ; i < str1.Length ; i++) {
				iCountTestcases++;
				i32 = sr.Read();
				if(i32 != (Int32)str1[i]) {
					iCountErrors++;
					printerr( "Error_198x9! Expected=="+(Int32)str1[i]+" , got=="+i32);
				}
			} 
			strLoc = "Loc_298xy";
			str1 = String.Empty;
			Random r = new Random((Int32)DateTime.Now.Ticks);
			for(int i = 0 ; i < 5000 ; i++)
				str1 += (Char)r.Next(0,255);
			sr = new StringReader(str1);
			for(int i = 0 ; i < str1.Length ; i++) {
				iCountTestcases++;
				i32 = sr.Read();
				if(i32 != (Int32)str1[i]) {
					iCountErrors++;
					printerr( "Error_109ux! Expected=="+(Int32)str1[i]+" , got=="+i32);
				}
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
		Co5567Read cbA = new Co5567Read();
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
