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
using System.Runtime.Serialization;
using System.Reflection;
public class Co3890RecordArrayElementFixup_LIL
{
	public static String s_strActiveBugNums = "";
	public static String s_strDtTmVer       = "";
	public static String s_strClassMethod   = "ObjectManager.RecordDelayedFixup(Int64, Int32, Int64)";
	public static String s_strTFName        = "Co3890RecordArrayElementFixup_LIL.cs";
	public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
	public static String s_strTFPath        = Environment.CurrentDirectory;
	public bool runTest()
	{
		Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver : " + s_strDtTmVer);
		int iCountErrors = 0;
		int iCountTestcases = 0;
		String strLoc = "Loc_000oo";
		ObjectManager objmgr1 = null;
		StreamingContext sc1 = new StreamingContext(StreamingContextStates.All);
		ObjectIDGenerator objid1 = null;
		Int64 iRootID;
		Int64 iChildID;
		bool fFirstTime;
		String[] strArr;
		String strToBeFixed;
		TestArrayFixup[] arrTst1;
		TestArrayFixup tst1;
		Int32 indice;
		try {
			do
			{
				strLoc = "Loc_9457and";
				strArr = new String[1];
				strToBeFixed = "Hello World";
				indice = 0;
				objid1 = new ObjectIDGenerator();
				iRootID = objid1.GetId(strArr, out fFirstTime);
				iChildID = objid1.GetId(strToBeFixed, out fFirstTime);
				objmgr1 = new ObjectManager(null, sc1);
				objmgr1.RecordArrayElementFixup(iRootID, indice, iChildID);
				objmgr1.RegisterObject(strArr, iRootID);
				objmgr1.RegisterObject(strToBeFixed, iChildID);
				iCountTestcases++;
				if(!strArr[0].Equals(strToBeFixed))
				{
					iCountErrors++;
					Console.WriteLine("Err_0452fsd! Expected value not returned, " + strArr[0] + ", expected, " + strToBeFixed);
				}
				objmgr1.DoFixups();
				strLoc = "Loc_017ged";
				iCountTestcases++;
				if(!strArr[0].Equals(strToBeFixed))
				{
					iCountErrors++;
					Console.WriteLine("Err_90453vdf! Expected value not returned, " + strArr[0] + ", expected, " + strToBeFixed);
				}
				strLoc = "Loc_1294cd";
				arrTst1 = new TestArrayFixup[10];
				tst1 = new TestArrayFixup();
				indice = 5;
				objid1 = new ObjectIDGenerator();
				iRootID = objid1.GetId(arrTst1, out fFirstTime);
				iChildID = objid1.GetId(tst1, out fFirstTime);
				objmgr1 = new ObjectManager(null, sc1);
				objmgr1.RecordArrayElementFixup(iRootID, indice, iChildID);
				objmgr1.RegisterObject(arrTst1, iRootID);
				objmgr1.RegisterObject(tst1, iChildID);
				iCountTestcases++;
				if(arrTst1[5].iValue != tst1.iValue)
				{
					iCountErrors++;
					Console.WriteLine("Err_112sx! Expected value not returned, " + arrTst1[5].iValue + ", expected, " + tst1.iValue);
				}
				objmgr1.DoFixups();
				strLoc = "Loc_017ged";
				iCountTestcases++;
				if(arrTst1[5].iValue != tst1.iValue)
				{
					iCountErrors++;
					Console.WriteLine("Err_2048cd! Expected value not returned, " + arrTst1[5].iValue + ", expected, " + tst1.iValue);
				}
				try {
					iCountTestcases++;
					objmgr1.RecordArrayElementFixup(-1, 5, iChildID);
					iCountErrors++;
					Console.WriteLine("Err_034cd! exception not thrown");
					}catch(ArgumentOutOfRangeException){
					}catch(Exception ex){
					iCountErrors++;
					Console.WriteLine("Err_034cd! Unexpected exception, " + ex.ToString());
				}
				try {
					iCountTestcases++;
					objmgr1.RecordArrayElementFixup(2, 5, -5);
					iCountErrors++;
					Console.WriteLine("Err_943cdd! exception not thrown");
					}catch(ArgumentOutOfRangeException){
					}catch(Exception ex){
					iCountErrors++;
					Console.WriteLine("Err_0834csd! Unexpected exception, " + ex.ToString());
				}
				try {
					iCountTestcases++;
					objmgr1.RecordArrayElementFixup(100, -5, 50);
					Console.WriteLine("Loc_943cdd! exception not thrown");
					}catch(Exception ex){
					iCountErrors++;
					Console.WriteLine("Err_8321sd! Unexpected exception, " + ex.ToString());
				}
			} while (false);
			} catch (Exception exc_general ) {
			++iCountErrors;
			Console.WriteLine (s_strTFAbbrev + " : Error Err_8888yyy!  strLoc=="+ strLoc +", exc_general==\n"+exc_general.ToString());
		}
		if ( iCountErrors == 0 )
		{
			Console.WriteLine( "paSs.   "+s_strTFPath +" "+s_strTFName+" ,iCountTestcases=="+iCountTestcases);
			return true;
		}
		else
		{
			Console.WriteLine("FAiL!   "+s_strTFPath+" "+s_strTFName+" ,iCountErrors=="+iCountErrors+" , BugNums?: "+s_strActiveBugNums );
			return false;
		}
	}
	public static void Main(String[] args)
	{
		bool bResult = false;
		Co3890RecordArrayElementFixup_LIL cbA = new Co3890RecordArrayElementFixup_LIL();
		try {
			bResult = cbA.runTest();
			} catch (Exception exc_main){
			bResult = false;
			Console.WriteLine(s_strTFAbbrev + " : FAiL! Error Err_9999zzz! Uncaught Exception in main(), exc_main=="+exc_main);
		}
		if (!bResult)
		{
			Console.WriteLine ("Path: "+s_strTFName + s_strTFPath);
			Console.WriteLine( " " );
			Console.WriteLine( "FAiL!  "+ s_strTFAbbrev);
			Console.WriteLine( " " );
		}
		if (bResult) Environment.ExitCode=0; else Environment.ExitCode=1;
	}
}
[Serializable]internal class TestArrayFixup {
	internal Int32 iValue;
	public TestArrayFixup(){
		iValue = 10;
	}
}
