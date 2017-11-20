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
public class Co6001GetHashCode
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strClassMethod   = "Array.GetHashCode";
 public static String s_strTFName        = "Co6001GetHashCode.cs";
 public static String s_strTFAbbrev      = "Co6001";
 public static String s_strTFPath        = "";
 public bool runTest()
   {
   Console.Error.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc = "Loc_000oo";
   short[] in2Arr = new Int16[10];
   int[] in4Arr = new Int32[5];
   Boolean[] boArr = new Boolean[3];
   int iHashCode1, iHashCode2;
   try {
   do
     {
     strLoc = "Loc_01hc";
     iCountTestcases++;
     in2Arr = new Int16[10];
     iHashCode1 = in2Arr.GetHashCode();
     iHashCode2 = in2Arr.GetHashCode();
     if(iHashCode1 != iHashCode2)
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ ", Err_01hc! , Array.GetHashCode on a same instance twice result different value." );
       }
     strLoc = "Loc_02hc";
     iCountTestcases++;
     in4Arr = new Int32[5];
     iHashCode1 = in2Arr.GetHashCode();
     iHashCode2 = in4Arr.GetHashCode();
     if(iHashCode1 == iHashCode2)
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ ", Err_02hc! , Array.GetHashCode on a different array type instances result same value." );
       }
     boArr = new Boolean[3];
     strLoc = "Loc_03hc";
     iCountTestcases++;
     iHashCode1 = in2Arr.GetHashCode();
     iHashCode2 = boArr.GetHashCode();
     if(iHashCode1 == iHashCode2)
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ ", Err_03hc! , Array.GetHashCode on a different array type instances result same value." );
       }
     strLoc = "Loc_04hc";
     iCountTestcases++;
     Array Arr1 = Array.CreateInstance(typeof(double), 10);
     Array Arr2 = Array.CreateInstance(typeof(Single), 0);
     iHashCode1 = Arr1.GetHashCode();
     iHashCode2 = Arr2.GetHashCode();
     if( iHashCode1 == iHashCode2 )
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ ", Err_04hc! , Array.GetHashCode on a different array type instances result same value." );
       }
     int[][] i4Arr = new int[3][];
     i4Arr[0] = new int[] {1, 2, 3};
     i4Arr[1] = new int[] {1, 2, 3, 4, 5, 6};
     i4Arr[2] = new int[] {1, 2, 3, 4, 5, 6, 7, 8, 9};
     strLoc = "Loc_05hc";
     iCountTestcases++;
     iHashCode1 = i4Arr.GetHashCode();
     iHashCode2 = i4Arr.GetHashCode();
     if( iHashCode1 != iHashCode2 )
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ ", Err_05hc! , Array.GetHashCode on a same instance of jagged array should result same value." );
       }
     strLoc = "Loc_06hc";
     iCountTestcases++;
     string[,,,,,] strArr = new string[6,5,4,3,2,1];
     iHashCode2 = strArr.GetHashCode();
     if( iHashCode1 == iHashCode2 )
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ ", Err_06hc! , Array.GetHashCode on a different array type instances result same value." );
       }
     } while (false);
   } catch (Exception exc_general ) {
   ++iCountErrors;
   Console.WriteLine(s_strTFAbbrev +", Error Err_8888yyy!  strLoc=="+ strLoc +", exc_general=="+exc_general);
   }
   if ( iCountErrors == 0 )
     {
     Console.Error.WriteLine( "paSs.   "+s_strTFPath +" "+s_strTFName+" ,iCountTestcases=="+iCountTestcases);
     return true;
     }
   else
     {
     Console.Error.WriteLine("FAiL!   "+s_strTFPath+" "+s_strTFName+" ,inCountErrors=="+iCountErrors+" , BugNums?: "+s_strActiveBugNums );
     return false;
     }
   }
 public static void Main(String[] args)
   {
   bool bResult = false;
   Co6001GetHashCode cbA = new Co6001GetHashCode();
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
   if (bResult) Environment.ExitCode = 0; else Environment.ExitCode = 11;
   }
}
