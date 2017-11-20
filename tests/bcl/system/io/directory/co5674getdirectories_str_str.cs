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
public class Co5674GetDirectories_Str_Str
{
	public static String s_strActiveBugNums = "";
	public static String s_strDtTmVer       = "";
	public static String s_strClassMethod   = "Directory.GetDirectories()";
	public static String s_strTFName        = "Co5674GetDirectories_Str_Str.cs";
	public static String s_strTFAbbrev      = s_strTFName.Substring(0,6);
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
			DirectoryInfo dir2;
			String dirName = s_strTFAbbrev+"TestDir";
			String[] strArr;
			if(Directory.Exists(dirName))
				Directory.Delete(dirName, true);
			strLoc = "Loc_477g8";
			dir2 = new DirectoryInfo(".");
			iCountTestcases++;
			try {
				Directory.GetDirectories(null, "*");
				iCountErrors++;
				printerr( "Error_2988b! Expected exception not thrown");
			} catch (ArgumentNullException aexc) {
				printinfo( "Info_29087! Caught expected exception, aexc=="+aexc.Message);
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_0707t! Incorrect exception thrown, exc=="+exc.ToString());
			}
			iCountTestcases++;
			try {
				Directory.GetDirectories(".", null);
				iCountErrors++;
				printerr( "Error_y767b! Expected exception not thrown");
			} catch (ArgumentNullException aexc) {
				printinfo( "Info_342yg! Caught expected exception, aexc=="+aexc.Message);
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_2y67b! Incorrect exception thrown, aexc=="+exc.ToString());
			}
			strLoc = "Loc_4yg7b";
			iCountTestcases++;
			try {
				Directory.GetDirectories(String.Empty, "*");
				iCountErrors++;
				printerr( "Error_8ytbm! Expected exception not thrown");
			} catch (ArgumentException aexc) {
				printinfo( "Info_y687d! Caught expected, exc=="+aexc.Message);
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_2908y! Incorrect exception thrown, exc=="+exc.ToString());
			}
			iCountTestcases++;
			try {
				String[] sDirs = Directory.GetDirectories(".", String.Empty);
				if( sDirs.Length != 0){ 
                    iCountErrors++;
    				printerr( "Error_543543! Invalid number of directories returned");
                }
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_21t0b! Incorrect exception thrown, exc=="+exc.ToString());
			}
			strLoc = "Loc_1190x";
			dir2 = new DirectoryInfo(".");
			iCountTestcases++;
			try {
				Directory.GetDirectories("\n", "*");
				iCountErrors++;
				printerr( "Error_2198y! Expected exception not thrown");
			} catch (ArgumentException aexc) {
				printinfo( "Info_94170! Caught expected exception, aexc=="+aexc.Message);
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_17888! Incorrect exception thrown, exc=="+exc.ToString());
			}
			iCountTestcases++;
			try {
				String[] strDirs = Directory.GetDirectories(".", "\n");
				if ( strDirs.Length > 0) {
                    iCountErrors++ ;
                    printerr( "Error_27109! Incorrect number of directories are return.. " + strDirs.Length);
                }
            } catch (IOException e) {
                printinfo("Expected exception occured... " + e.Message );
			} catch (Exception exc) {
				iCountErrors++;
				printerr( "Error_107gy! Incorrect exception thrown, exc=="+exc.ToString());
			}
			strLoc = "Loc_4y982";
			dir2 = Directory.CreateDirectory(dirName);
			strArr = Directory.GetDirectories(dirName, "*");
			iCountTestcases++;
			if(strArr.Length != 0) {
				iCountErrors++;
				printerr("Error_207v7! Incorrect number of directories returned");
			}
			strLoc = "Loc_2398c";
			dir2.CreateSubdirectory("TestDir1");
			dir2.CreateSubdirectory("TestDir2");
			dir2.CreateSubdirectory("TestDir3");
			dir2.CreateSubdirectory("Test1Dir1");
			dir2.CreateSubdirectory("Test1Dir2");
			new FileInfo(dir2.ToString() +"TestFile1");
			new FileInfo(dir2.ToString() +"TestFile2");
			iCountTestcases++;
			strArr = Directory.GetDirectories(".\\"+dirName, "TestDir*");
			iCountTestcases++;
			if(strArr.Length != 3) {
				iCountErrors++;
				printerr( "Error_1yt75! Incorrect number of directories returned");
			}
			String[] names = new String[strArr.Length];
			int i = 0;
			foreach( String d in strArr)
				names[i++] = d.ToString().Substring( d.ToString().LastIndexOf(Path.DirectorySeparatorChar) + 1);
			iCountTestcases++;
			if(Array.IndexOf(names, "TestDir1") < 0) {
				iCountErrors++;
				printerr( "Error_3y775! Incorrect name=="+strArr[0].ToString());
			}
			iCountTestcases++;
			if(Array.IndexOf(names, "TestDir2") < 0) {
				iCountErrors++;
				printerr( "Error_90885! Incorrect name=="+strArr[1].ToString());
			}
			iCountTestcases++;
			if(Array.IndexOf(names, "TestDir3") < 0) {
				iCountErrors++;
				printerr( "Error_879by! Incorrect name=="+strArr[2].ToString());
			}
			strLoc = "Loc_249yv";
			strArr = Directory.GetDirectories(Environment.CurrentDirectory+"\\"+dirName, "*");
			iCountTestcases++;
			if(strArr.Length != 5) {
				iCountErrors++;
				printerr( "Error_t5792! Incorrect number of directories=="+strArr.Length);
			}
			names = new String[strArr.Length];
			i = 0;
			foreach( String d in strArr)
				names[i++] = d.ToString().Substring( d.ToString().LastIndexOf(Path.DirectorySeparatorChar) + 1);
			iCountTestcases++;
			if(Array.IndexOf(names, "Test1Dir1") < 0) {
				iCountErrors++;
				printerr( "Error_4898v! Incorrect name=="+strArr[0].ToString());
			}
			iCountTestcases++;
			if(Array.IndexOf(names, "Test1Dir2") < 0) {
				iCountErrors++;
				printerr( "Error_4598c! Incorrect name=="+strArr[1].ToString());
			}
			iCountTestcases++;
			if(Array.IndexOf(names, "TestDir1") < 0) {
				iCountErrors++;
				printerr( "Error_209d8! Incorrect name=="+strArr[2].ToString());
			}
			iCountTestcases++;
			if(Array.IndexOf(names, "TestDir2") < 0) {
				iCountErrors++;
				printerr( "Error_10vtu! Incorrect name=="+strArr[3].ToString());
			} 
			iCountTestcases++;
			if(Array.IndexOf(names, "TestDir3") < 0) {
				iCountErrors++;
				printerr( "Error_190vh! Incorrect name=="+strArr[4].ToString());
			}
			strLoc = "Loc_20v99";
			strArr = Directory.GetDirectories(Environment.CurrentDirectory+"\\"+dirName, "*Dir2");
			iCountTestcases++;
			if(strArr.Length != 2) {
				iCountErrors++;
				printerr( "Error_8019x! Incorrect number of directories=="+strArr.Length);
			}
			names = new String[strArr.Length];
			i = 0;
			foreach ( String d in strArr)
				names[i++] = d.ToString().Substring( d.ToString().LastIndexOf(Path.DirectorySeparatorChar) + 1);
			iCountTestcases++;
			if(Array.IndexOf(names, "Test1Dir2") < 0) {
				iCountErrors++;
				printerr( "Error_167yb! Incorrect name=="+strArr[0].ToString());
			} 
			iCountTestcases++;
			if(Array.IndexOf(names, "TestDir2") < 0) {
				iCountErrors++;
				printerr( "Error_49yb7! Incorrect name=="+strArr[1].ToString());
			}
			strLoc = "Loc_2498g";
			dir2.CreateSubdirectory("AAABB");
			dir2.CreateSubdirectory("aaabbcc");
			strArr = Directory.GetDirectories(".\\"+dirName, "*BB*");
			iCountTestcases++;
#if PLATFORM_UNIX // case-sensitive filesystem
                        if(strArr.Length != 1) {
#else
			if(strArr.Length != 2) {
#endif
				iCountErrors++;
				printerr( "Error_4y190! Incorrect number of directories=="+strArr.Length);
			} 
			names = new String[strArr.Length];
			i = 0;
			foreach ( String d in strArr)
				names[i++] = d.ToString().Substring( d.ToString().LastIndexOf(Path.DirectorySeparatorChar) + 1);
			iCountTestcases++;
			if(Array.IndexOf(names, "AAABB") < 0) {
				iCountErrors++;
				printerr( "Error_956yb! Incorrect name=="+strArr[0]);
			} 
#if !PLATFORM_UNIX // case-sensitive filesystem
			iCountTestcases++;
			if(Array.IndexOf(names, "aaabbcc")  < 0) {
				iCountErrors++;
				printerr( "Error_48yg7! Incorrect name=="+strArr[1]);
			}
#endif //!PLATFORM_UNIX
			strArr = Directory.GetDirectories(".\\"+dirName, "Directory");
			iCountTestcases++;
			if(strArr.Length != 0) {
				iCountErrors++;
				printerr( "Error_209v7! Incorrect number of directories=="+strArr.Length);
			}
			if(Directory.Exists(dirName))
				Directory.Delete(dirName, true);
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
		Co5674GetDirectories_Str_Str cbA = new Co5674GetDirectories_Str_Str();
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
