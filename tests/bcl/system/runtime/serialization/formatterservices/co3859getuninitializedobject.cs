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
using System.Runtime.Remoting;
using System.Runtime.Serialization;
using System.IO;
using System.Reflection;
public class Co3859GetUninitializedObject
{
	public static String s_strActiveBugNums = "";
	public static String s_strDtTmVer       = "";
	public static String s_strClassMethod   = "FormatterServices.GetUninitializedObject(Type)";
	public static String s_strTFName        = "Co3859GetUninitializedObject.cs";
	public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
	public static String s_strTFPath        = Environment.CurrentDirectory;
	public bool runTest()
	{
		Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver : " + s_strDtTmVer);
		Console.WriteLine();
		int iCountErrors = 0;
		int iCountTestcases = 0;
		String strLoc = "Loc_000oo";
		Type tpName = null;
		Object obj1 =  null;
		Module mdClassLib = null;
		Type[] arrTpClassLib = null;
		try {
			do
			{
				tpName = Type.GetType("System.Int32");
				obj1 = FormatterServices.GetUninitializedObject(tpName);
				iCountTestcases++;
				if((Int32)obj1 != 0)
				{
					iCountErrors++;
					Console.WriteLine("Err_563ds! Wrong value, " + ((Int32)(obj1)).ToString());
				}
				tpName = Type.GetType("System.String");
				try
				{
					iCountTestcases++;
					obj1 = FormatterServices.GetUninitializedObject(tpName);
					iCountErrors++;
					Console.WriteLine("Err_8346fd! Exception not thrown");
				}
				catch(ArgumentException){}
				catch(Exception ex)
				{
					iCountErrors++;
					Console.WriteLine("Err_957fgd! Wrong exception thrown, " + ex.ToString());
				}
				try
				{
					iCountTestcases++;
					obj1 = FormatterServices.GetUninitializedObject(null);
					iCountErrors++;
					Console.WriteLine("Err_935vd! We were expecting this to throw!");
				}
				catch(ArgumentNullException){}
				catch(Exception ex)
				{
					iCountErrors++;
					Console.WriteLine("Err_935fd! Unexpected exception thrown, " + ex.ToString());
				}
				tpName = Type.GetType("baloney");
				obj1 = FormatterServices.GetUninitializedObject(tpName);
				iCountTestcases++;
				if(obj1==null)
				{
					iCountErrors++;
					Console.WriteLine("Err_035vcd! null baloney");
				}
				iCountTestcases++;
				if(((baloney)obj1).i_spy!=0)
				{
					iCountErrors++;
					Console.WriteLine("Err_729sm! wrong value returned, " + (((baloney)obj1).i_spy).ToString());
				}
				Console.WriteLine();
				mdClassLib = typeof(String).Module;
				arrTpClassLib = mdClassLib.GetTypes();
				for(int i=0; i<arrTpClassLib.Length;i++)
				{
					if(!arrTpClassLib[i].IsPublic || !arrTpClassLib[i].IsSerializable)
					continue;
					try
					{
						iCountTestcases++;
						obj1 = FormatterServices.GetUninitializedObject(arrTpClassLib[i]);
					}
					catch(ArgumentException)
					{
						Console.WriteLine("GetUninitializedObject throws for type, " + arrTpClassLib[i].FullName);
					}
					catch(Exception ex)
					{
						iCountErrors++;
						Console.WriteLine("Err_846qm! Unexpected exception thrown for the type, " + arrTpClassLib[i].FullName + ", " + ex.ToString());
					}
				}
				Console.WriteLine();
			} while (false);
			} catch (Exception exc_general ) {
			++iCountErrors;
			Console.WriteLine (s_strTFAbbrev + " : Error Err_8888yyy!  strLoc=="+ strLoc +", exc_general==\n"+exc_general.StackTrace);
		}
		if ( iCountErrors == 0 )
		{
			Console.WriteLine( "paSs.   "+s_strTFPath +" "+s_strTFName+" ,iCountTestcases=="+iCountTestcases.ToString());
			return true;
		}
		else
		{
			Console.WriteLine("FAiL!   "+s_strTFPath+" "+s_strTFName+" ,iCountErrors=="+iCountErrors.ToString()+" , BugNums?: "+s_strActiveBugNums );
			return false;
		}
	}
	public static void Main(String[] args)
	{
		bool bResult = false;
		Co3859GetUninitializedObject cbA = new Co3859GetUninitializedObject();
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
[Serializable] public class baloney
{
	internal String str_Humpty;
	public int i_spy;
	public baloney()
	{
		str_Humpty = "Hello World";
		i_spy = 20;
	}
}
