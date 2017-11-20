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
public class Co6003IsFixedSize
{
   public static String s_strActiveBugNums = "";
   public static String s_strDtTmVer       = "";
   public static String s_strClassMethod   = "Array.IsFixedSize";
   public static String s_strTFName        = "Co6003IsFixedSize";
   public static String s_strTFAbbrev      = "Co6003";
   public static String s_strTFPath        = "";
   public bool runTest()
   {
      Console.Error.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
      int iCountErrors = 0;
      int iCountTestcases = 0;
      String strLoc = "Loc_000oo";
	  Boolean bFixedSize = false;
      try {
      do
      {
         strLoc = "Loc_01ifs";
         iCountTestcases++;
         short[] in2Arr = new Int16[10];
		 bFixedSize = in2Arr.IsFixedSize;
         if(!bFixedSize)
         {
            iCountErrors++;
            Console.WriteLine( s_strTFAbbrev+ " ,Err_01ifs! , Array.IsFixedSize failed on single dimension array." );
         }
 		 strLoc = "Loc_02ifs";
         iCountTestcases++;
         int[] in4Arr = new Int32[0];
		 bFixedSize = in4Arr.IsFixedSize;
         if(!bFixedSize)
         {
            iCountErrors++;
            Console.WriteLine( s_strTFAbbrev+ " ,Err_02ifs! , Array.IsFixedSize failed on 0 size single dimension array." );
         }
		 int[][] i4Arr = new int[3][];
		 i4Arr[0] = new int[] {1, 2, 3};
		 i4Arr[1] = new int[] {1, 2, 3, 4, 5, 6};
		 i4Arr[2] = new int[] {1, 2, 3, 4, 5, 6, 7, 8, 9};
 		 strLoc = "Loc_03ifx";
         iCountTestcases++;
		 bFixedSize = i4Arr.IsFixedSize;
         if(!bFixedSize)
         {
            iCountErrors++;
            Console.WriteLine( s_strTFAbbrev+ " ,Err_03ifs! , Array.IsFixedSize failed on jagged array." );
         }
 		 strLoc = "Loc_04ifx";
         iCountTestcases++;
         Object [][] objArr = new Object[0][];
		 bFixedSize = objArr.IsFixedSize;
         if(!bFixedSize)
         {
            iCountErrors++;
            Console.WriteLine( s_strTFAbbrev+ " ,Err_04ifs! , Array.IsFixedSize failed on 0 size jagged array." );
         }
 		 strLoc = "Loc_05ifx";
         iCountTestcases++;
         Double[,] doArr = new Double[99, 78];
		 bFixedSize = doArr.IsFixedSize;
         if(!bFixedSize)
         {
            iCountErrors++;
            Console.WriteLine( s_strTFAbbrev+ " ,Err_05ifs! , Array.IsFixedSize failed on rectangle array." );
         }
 		 strLoc = "Loc_06ifx";
         iCountTestcases++;
         Hashtable[,] htArr = new Hashtable[0,0];
		 bFixedSize = htArr.IsFixedSize;
         if(!bFixedSize)
         {
            iCountErrors++;
            Console.WriteLine( s_strTFAbbrev+ " ,Err_06ifs! , Array.IsFixedSize failed on 0 size rectangle array." );
         }
 		 strLoc = "Loc_07ifx";
         iCountTestcases++;
         string[,,,,,] strArr = new string[6,5,4,3,2,1];
		 bFixedSize = strArr.IsFixedSize;
         if(!bFixedSize)
         {
            iCountErrors++;
            Console.WriteLine( s_strTFAbbrev+ " ,Err_07ifs! , Array.IsFixedSize failed on 6 dimensions rectangle array." );
         }
      } while (false);
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
         Console.Error.WriteLine("FAiL!   "+s_strTFPath+" "+s_strTFName+" ,inCountErrors=="+iCountErrors+" , BugNums?: "+s_strActiveBugNums );
         return false;
      }
   }
   public static void Main(String[] args)
   {
      bool bResult = false;
      Co6003IsFixedSize cbA = new Co6003IsFixedSize();
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
