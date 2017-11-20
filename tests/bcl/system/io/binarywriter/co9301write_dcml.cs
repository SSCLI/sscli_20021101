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
public class Co9301Write_Dcml
{
	public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "BinaryWriter.Write(Char)";
	public static String s_strTFName        = "Co9301Write_Dcml.cs";
	public static String s_strTFAbbrev      = "Co5539";
	public static String s_strTFPath        = Environment.CurrentDirectory.ToString();
	public bool runTest()
	{
		Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
		int iCountErrors = 0;
		int iCountTestcases = 0;
		String strLoc = "Loc_000oo";
		try
		{
			BinaryWriter dw2 = null;
			Stream fs2 = null;
			BinaryReader dr2 = null;
			FileInfo fil2 = null;		    
			MemoryStream mstr = null;
			Decimal dbl2 = 0;
			int ii = 0;
			Decimal[] dblArr = new Decimal[] {
				 Decimal.One
				,Decimal.Zero
				,Decimal.MinusOne
				,Decimal.MinValue
				,Decimal.MaxValue
				,new Decimal(-1000.5)
				,new Decimal(-10.0E-40)
				,new Decimal(3.4E-40898)
                ,new Decimal(3.4E-28)
				,new Decimal(3.4E+28)
                ,new Decimal(0.45)
				,new Decimal(5.55)
				,new Decimal(3.4899E23)
			};
			strLoc = "Loc_8yfv7";
			fil2 = new FileInfo("TestFile.tmp");
			fs2 = fil2.Open(FileMode.Create);
			dw2 = new BinaryWriter(fs2);
			try {
			for(ii = 0 ; ii < dblArr.Length ; ii++) {
				Console.WriteLine("Writing: "+dblArr[ii]);
				dw2.Write(dblArr[ii]);			
			}
			dw2.Flush();
			fs2.Close();
			strLoc = "Loc_987hg";
			fs2 = fil2.Open(FileMode.Open);
			dr2 = new BinaryReader(fs2);
			Console.WriteLine("Starting Read: ");
			try {
				for(ii = 0 ; ;ii++) {
					iCountTestcases++;
					if((dbl2 = dr2.ReadDecimal()) != dblArr[ii]) {
						iCountErrors++;
						printerr( "Error_298hg! Expected=="+dblArr[ii]+" , got=="+dblArr[ii]);
					}
				}
			} catch (EndOfStreamException) {
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_3298h! Unexpected exception thrown, exc=="+exc.ToString());
			}
			iCountTestcases++;
			if(ii != dblArr.Length) {
				iCountErrors++;
				printerr( "Error_2g767! Incorrect number of elements on filestream");
			}
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_278gy! Unexpected exception, exc=="+exc.ToString());
			}
			fs2.Close();
			fil2.Delete();
			strLoc = "Loc_98yss";
			mstr = new MemoryStream();
			dw2 = new BinaryWriter(mstr);
			try {
			for(ii = 0 ; ii < dblArr.Length ; ii++) 
				dw2.Write(dblArr[ii]);
			dw2.Flush();
			mstr.Position = 0;
			strLoc = "Loc_287y5";
					dr2 = new BinaryReader(mstr);
			try {
				for(ii = 0 ; ; ii++) {
					iCountTestcases++;			
					if((dbl2 = dr2.ReadDecimal()) != dblArr[ii]) {
						iCountErrors++;
						printerr( "Error_948yg! Expected=="+dblArr[ii]+", got=="+dbl2);
					} 
				} 
			} catch (EndOfStreamException) {
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_238gy! Unexpected exception thrown, exc=="+exc.ToString());
			}
			iCountTestcases++;
			if(ii != dblArr.Length) {
				iCountErrors++;
				printerr( "Error_289yg! Incorrect number of elements in filestream");
			}
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_298gy! Unexpected exception, exc=="+exc.ToString());
			}
			mstr.Close();
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
			Console.WriteLine("FAiL! "+s_strTFName+" ,iCountErrors=="+iCountErrors.ToString() );
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
		Co9301Write_Dcml cbA = new Co9301Write_Dcml();
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
