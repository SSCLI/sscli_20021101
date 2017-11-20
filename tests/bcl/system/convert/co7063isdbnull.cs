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
public class Co7063IsDBNull
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strClassMethod   = "IsDBNull (Object)"; 
 public static String s_strTFName        = "Co7063IsDBNull.cs";
 public static String s_strTFAbbrev      = s_strTFName.Substring(0,6);
 public static String s_strTFPath        = "";
 public bool runTest()
   {
   Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   String strLoc = "Loc_000oo";
   String strValue = String.Empty;
   int iCountErrors = 0;
   int iCountTestcases = 0;
   try
     {
     Boolean bTest;
     strLoc = "Loc_100vy";
     iCountTestcases++;
     bTest = Convert.IsDBNull (DBNull.Value); 	
     if (bTest != true)
       {
       ++iCountErrors;	
       printerr( "Error_100aa! Sent In: Value.DBNull, Expected: true, Recieved: " + bTest);
       }
     strLoc = "Loc_201vy";
     iCountTestcases++;
     Int32 nTest = 6678;
     bTest = Convert.IsDBNull (nTest); 	
     if (bTest != false)
       {
       ++iCountErrors;	
       printerr( "Error_200aa! Sent In: Int32, Expected: false, Recieved: " + bTest);
       }
     strLoc = "Loc_301vy";
     iCountTestcases++;
     Object oTest = new Object ();
     bTest = Convert.IsDBNull (oTest); 	
     if (bTest != false)
       {
       ++iCountErrors;	
       printerr( "Error_300aa! Sent In: Object, Expected: false, Recieved: " + bTest);
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
   Co7063IsDBNull cbA = new Co7063IsDBNull();
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
