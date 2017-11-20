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
public class Co5066ToLocalTime
{
 public static readonly String s_strActiveBugNums = "";
 public static readonly String s_strDtTmVer       = "";
 public static readonly String s_strClassMethod   = "DateTime.ToLocalTime()";
 public static readonly String s_strTFName        = "Co5066ToLocalTime.cs";
 public static readonly String s_strTFAbbrev      = "Co5066";
 public static readonly String s_strTFPath        = "";
 public virtual Boolean runTest()
   {
   Console.Error.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc = "Loc_000oo";
   DateTime dt1 ;
   DateTime dt2 ;
   try {
   do
     {
     strLoc = "Loc_471hq";
     dt1 = new DateTime(1999,1,1,12,32,59);
     dt2 = new DateTime(1999,1,1,12,32,59);
     dt2 = dt2.ToUniversalTime();
     dt2 = dt2.ToLocalTime();
     iCountTestcases++;
     if(!dt1.Equals(dt2))
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ "Err_834hw! , return=="+dt1.ToLocalTime());
       }
     strLoc = "Loc_487iw";
     dt1 = new DateTime(2000,2,29);
     dt2 = new DateTime(2000,2,29);
     dt2 = dt2.ToUniversalTime();
     dt2 = dt2.ToLocalTime();
     iCountTestcases++;
     if(!dt1.Equals(dt2))
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ "Err_394hw! , return=="+dt1.ToLocalTime());
       }
     strLoc = "Loc_123nq";
     dt1 = new DateTime(100,3,4,14,44,23);
     dt2 = new DateTime(100,3,4,14,44,23);
     dt2 = dt2.ToUniversalTime();
     dt2 = dt2.ToLocalTime();
     iCountTestcases++;
     if(!dt1.Equals(dt2))
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ "Err_137ne! , return=="+dt1.ToLocalTime());
       }
     strLoc = "Loc_473jo";
     dt1 = new DateTime(1754,4,7,7,32,23);
     dt2 = new DateTime(1754,4,7,7,32,23);
     dt2 = dt2.ToUniversalTime();
     dt2 = dt2.ToLocalTime();
     iCountTestcases++;
     if(!dt1.Equals(dt2))
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ "Err_128jw! ,return=="+dt1.ToLocalTime());
       }
     strLoc = "Loc_473jo";
     dt1 = new DateTime(2002, 4, 6, 22, 0, 0);
     dt2 = dt1.ToUniversalTime();
     dt2 = dt2.ToLocalTime();
     iCountTestcases++;
     if(!dt1.Equals(dt2))
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ "Err_128jw! ,return=="+dt1.ToLocalTime());
       }
     strLoc = "Loc_473jo";
     dt1 = new DateTime(2002, 10, 6, 22, 0, 0);
     dt2 = dt1.ToUniversalTime();
     dt2 = dt2.ToLocalTime();
     iCountTestcases++;
     if(!dt1.Equals(dt2))
       {
       iCountErrors++;
       Console.WriteLine( s_strTFAbbrev+ "Err_128jw! ,return=="+dt1.ToLocalTime());
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
     Console.Error.WriteLine("FAiL!   "+s_strTFPath+" "+s_strTFName+" ,iCountErrors=="+iCountErrors+" , BugNums?: "+s_strActiveBugNums );
     return false;
     }
   }
 public static void Main(String[] args)
   {
   Boolean bResult = false;
   Co5066ToLocalTime cbA = new Co5066ToLocalTime();
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
}
