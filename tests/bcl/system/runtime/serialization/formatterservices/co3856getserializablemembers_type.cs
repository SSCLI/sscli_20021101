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
using System.Runtime.Serialization;
using System.IO;
using System.Reflection;
public class Co3856GetSerializableMembers_type
{
	public static String s_strActiveBugNums = "";
	public static String s_strDtTmVer       = "";
	public static String s_strClassMethod   = "FormatterServices.GetSerializableMembers(Type)";
	public static String s_strTFName        = "Co3856GetSerializableMembers_type.cs";
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
		MemberInfo[] members = null;
		Module mdClassLib = null;
		Type[] arrTpClassLib = null;
		Type[] arrTpPopularValutTypes = {Type.GetType("System.Int16"), Type.GetType("System.Int32"),
			Type.GetType("System.UInt16"), Type.GetType("System.Char"),
			Type.GetType("System.SByte"), Type.GetType("System.Int64"),
			Type.GetType("System.Boolean")
		};
		try {
			do
			{
				tpName = Type.GetType("System.Int32");
				members = FormatterServices.GetSerializableMembers(tpName);
				iCountTestcases++;
				if(members.Length!=1)
				{
					iCountErrors++;
					Console.WriteLine("Err_743cd! Int32 has more members than before, " + members.Length.ToString());
				}
				iCountTestcases++;
				if(!members[0].Name.Equals("m_value"))
				{
					iCountErrors++;
					Console.WriteLine("Err_4692fd! Int32's member name is different than before, " + members[0].Name);
				}
				for(int i=0; i<arrTpPopularValutTypes.Length;i++)
				{
					members = FormatterServices.GetSerializableMembers(arrTpPopularValutTypes[i]);
					iCountTestcases++;
					if(members.Length!=1)
					{
						iCountErrors++;
						Console.WriteLine("Err_863xw! " + arrTpPopularValutTypes[i].Name + " has more members than before, " + members.Length.ToString());
					}
					iCountTestcases++;
					if(!members[0].Name.Equals("m_value"))
					{
						iCountErrors++;
						Console.WriteLine("Err_395fd! " + arrTpPopularValutTypes[i].Name + " member name is different than before, " + members[0].Name);
					}
				}
				mdClassLib = typeof(String).Module;
				arrTpClassLib = mdClassLib.GetTypes();
				for(int i=0; i<arrTpClassLib.Length;i++)
				{
					if(!arrTpClassLib[i].IsPublic || !arrTpClassLib[i].IsSerializable)
					continue;
					try
					{
						iCountTestcases++;
						members = FormatterServices.GetSerializableMembers(arrTpClassLib[i]);
					}
					catch(Exception ex)
					{
						iCountErrors++;
						Console.WriteLine("Err_846qm! Unexpected exception thrown for the type, " + arrTpClassLib[i].FullName + ", " + ex.ToString());
					}
				}
				try
				{
					iCountTestcases++;
					members = FormatterServices.GetSerializableMembers(null);
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
				members = FormatterServices.GetSerializableMembers(tpName);
				iCountTestcases++;
				if(tpName==null)
				{
					iCountErrors++;
					Console.WriteLine("Err_035vcd! null baloney");
				}
				iCountTestcases++;
				if(members.Length!=2)
				{
					iCountErrors++;
					Console.WriteLine("Err_9e5fd! baloney has more members than before, " + members.Length.ToString());
				}
				iCountTestcases++;
				if(!members[0].Name.Equals("str_Humpty"))
				{
					iCountErrors++;
					Console.WriteLine("Err_036cs! baloney's member name is different than before, " + members[0].Name);
				}
				iCountTestcases++;
				if(!members[1].Name.Equals("i_spy"))
				{
					iCountErrors++;
					Console.WriteLine("Err_302xs! baloney's member name is different than before, " + members[1].Name);
				}
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
		Co3856GetSerializableMembers_type cbA = new Co3856GetSerializableMembers_type();
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
[Serializable] class baloney
{
	internal String str_Humpty;
	public int i_spy;
}
