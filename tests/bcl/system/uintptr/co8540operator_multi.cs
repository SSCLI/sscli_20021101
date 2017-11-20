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
using System.Collections;
using System.IO;
using System.Reflection;
public class Co8540operator_multi : IDescribeTestedMethods
{
 public static readonly String s_strActiveBugNums = "";
 public static readonly String s_strDtTmVer       = "";
 public static readonly String s_strClassMethod   = "UIntPtr.multiple methods";
 public static readonly String s_strTFName        = "Co8540operator_multi.cs";
 public static readonly String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
 public static readonly String s_strTFPath        = Environment.CurrentDirectory;
 public unsafe virtual bool runTest()
   {
   Console.Error.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc = "Loc_000oo";
   UIntPtr ip1;
   UIntPtr ip2;
   void* vd1;
   void* vd2;
   UInt32 iValue;          
   UInt64 lValue;
   Boolean fValue;
   try {
   strLoc = "Loc_743wg";
   iValue = 16;
   ip1 = (UIntPtr)iValue;
   iCountTestcases++;
   if((UInt32)ip1 != iValue){
   iCountErrors++;
   Console.WriteLine("Err_2975sf! Wrong value returned");
   }
   iValue = Int32.MaxValue;
   ip1 = (UIntPtr)iValue;
   iCountTestcases++;
   if((UInt32)ip1 != iValue)
     {
     iCountErrors++;
     Console.WriteLine("Err_2975sf! Wrong value returned");
     }
   strLoc = "Loc_0084wf";
   lValue = 16;
   ip1 = (UIntPtr)lValue;
   iCountTestcases++;
   if((UInt64)ip1 != lValue){
   iCountErrors++;
   Console.WriteLine("Err_974325sdg! Wrong value returned");
   }
   lValue = Int32.MaxValue;
   ip1 = (UIntPtr)lValue;
   iCountTestcases++;
   if((UInt64)ip1 != lValue)
     {
     iCountErrors++;
     Console.WriteLine("Err_974325sdg! Wrong value returned");
     }
   strLoc = "Loc_008742sf";
   fValue = true;
   vd1 = &fValue;
   ip1 = (UIntPtr)vd1;
   vd2 = (void* )ip1;
   iCountTestcases++;
   if((*((Boolean*)vd2)) != fValue){
   iCountErrors++;
   Console.WriteLine("Err_984753sdg! Wrong value returned!");
   }
   strLoc = "Loc_98734sdg";
   ip1 = (UIntPtr)16;
   ip2 = (UIntPtr)16;
   iCountTestcases++;
   if(ip1 != ip2){
   iCountErrors++;
   Console.WriteLine("Err_984753sdg! Wrong value returned!");
   }
   ip1 = (UIntPtr)16;
   ip2 = (UIntPtr)17;
   iCountTestcases++;
   if(ip1 == ip2){
   iCountErrors++;
   Console.WriteLine("Err_984753sdg! Wrong value returned!");
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
   bool bResult = false;
   Co8540operator_multi cbA = new Co8540operator_multi();
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
 public MemberInfo[] GetTestedMethods()
   {		
   MethodInfo[] methods = typeof(UIntPtr).GetMethods(BindingFlags.Public|BindingFlags.Static);
   return methods;
   }
}
interface IDescribeTestedMethods
{
 MemberInfo[] GetTestedMethods();
}
