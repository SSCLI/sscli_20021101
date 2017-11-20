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
using System.Globalization;
using GenStrings;
using System;
public class Co5154Remove_ii
{
 public static readonly String s_strActiveBugNums = "";
 public static readonly String s_strDtTmVer       = "";
 public static readonly String s_strClassMethod   = "String.Remove(int, int)";
 public static readonly String s_strTFName        = "Co5154Remove_ii.";
 public static readonly String s_strTFAbbrev      = "Co5154";
 public static readonly String s_strTFPath        = "";
 public virtual bool runTest()
   {
   Console.Error.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc = "Loc_000oo";
   String str1;
   int iCount;
   int iStartIndex;
   try {
   int[] iArrInvalidValues = new Int32[]{ -1, -2, -100, -1000, -10000, -100000, -1000000, -10000000, -100000000, -1000000000, Int32.MinValue};
   int[] iArrLargeValues = new Int32[]{ Int32.MaxValue, Int32.MaxValue-1, Int32.MaxValue/2 , Int32.MaxValue/10 , Int32.MaxValue/100 };
   int[] iArrValidValues = new Int32[]{ 10000, 100000 , Int32.MaxValue/200 , Int32.MaxValue/1000 };
   iCountTestcases++;
   String strNewString = "This is a tesing string" ;
   String strTemp = new String( 'a', 10000);
   for(int iLoop = 0 ; iLoop < iArrInvalidValues.Length ; iLoop++ ){
   try
     {
     strNewString = strNewString.Remove(iArrInvalidValues[iLoop], 5);
     iCountErrors++;                        
     Console.Error.WriteLine( "Error_0000!! Expected exception not occured");
     } catch ( ArgumentOutOfRangeException ){
     } catch ( Exception ex )
       {
       Console.Error.WriteLine( "Error_2222!!! Unexpected exception " + ex.ToString() );
       iCountErrors++ ;
       }
   }
   iCountTestcases++;
   strNewString = "This is a tesing string" ;
   strTemp = new String( 'a', 10000);
   for(int iLoop = 0 ; iLoop < iArrInvalidValues.Length ; iLoop++ ){
   try
     {
     strNewString = strNewString.Remove(5, iArrInvalidValues[iLoop]);
     iCountErrors++;                        
     Console.Error.WriteLine( "Error_1111!! Expected exception not occured");
     } catch ( ArgumentOutOfRangeException ){
     } catch ( Exception ex )
       {
       Console.Error.WriteLine( "Error_3333!!! Unexpected exception " + ex.ToString() );
       iCountErrors++ ;
       }
   }
   iCountTestcases++;
   strNewString = "This is a tesing string" ;
   strTemp = new String( 'a', 10000);
   for(int iLoop = 0 ; iLoop < iArrInvalidValues.Length ; iLoop++ ){
   try
     {
     strNewString = strNewString.Remove(iArrInvalidValues[iLoop], iArrInvalidValues[iLoop]);
     iCountErrors++;                        
     Console.Error.WriteLine( "Error_1551!! Expected exception not occured");
     } catch ( ArgumentOutOfRangeException ){
     } catch ( Exception ex )
       {
       Console.Error.WriteLine( "Error_9804!!! Unexpected exception " + ex.ToString() );
       iCountErrors++ ;
       }
   }
   iCountTestcases++;
   for(int iLoop = 0 ; iLoop < iArrLargeValues.Length ; iLoop++ ){
   try
     {
     strNewString = strNewString.Remove(iArrLargeValues[iLoop], 5);
     iCountErrors++;                        
     Console.Error.WriteLine( "Error_34879!! Expected exception not occured");
     } catch ( ArgumentOutOfRangeException ){
     } catch ( Exception ex )
       {
       Console.Error.WriteLine( "Error_4434!!! Unexpected exception " + ex.ToString() );
       iCountErrors++ ;
       }
   }
   iCountTestcases++;
   for(int iLoop = 0 ; iLoop < iArrLargeValues.Length ; iLoop++ ){
   try
     {
     strNewString = strNewString.Remove(5, iArrLargeValues[iLoop]);
     iCountErrors++;                        
     Console.Error.WriteLine( "Error_7843!! Expected exception not occured");
     } catch ( ArgumentOutOfRangeException ){
     } catch ( Exception ex )
       {
       Console.Error.WriteLine( "Error_4324!!! Unexpected exception " + ex.ToString() );
       iCountErrors++ ;
       }
   }
   iCountTestcases++;
   for(int iLoop = 0 ; iLoop < iArrLargeValues.Length ; iLoop++ ){
   try
     {
     strNewString = strNewString.Remove(iArrLargeValues[iLoop], iArrLargeValues[iLoop]);
     iCountErrors++;                        
     Console.Error.WriteLine( "Error_4892!! Expected exception not occured");
     } catch ( ArgumentOutOfRangeException ){
     } catch ( Exception ex )
       {
       Console.Error.WriteLine( "Error_4340!!! Unexpected exception " + ex.ToString() );
       iCountErrors++ ;
       }
   }
   iCountTestcases++;
   strNewString =  new String( 'a' , Int32.MaxValue/200 );
   for(int iLoop = 0 ; iLoop < iArrValidValues.Length ; iLoop++ ){
   try
     {                            
     strTemp = strNewString.Remove(0,iArrValidValues[iLoop]);     
     if ( strNewString.Length != strTemp.Length + iArrValidValues[iLoop] ){
     iCountErrors++;
     Console.Error.WriteLine( "Error_6666!!!! Incorrect string length.... Expected...{0},  Actual...{1}", strTemp.Length + iArrValidValues[iLoop], strNewString.Length );
     }
     } catch ( Exception ex ){
     Console.Error.WriteLine( "Error_7777!!! Unexpected exception " + ex.ToString() );
     iCountErrors++ ;
     }
   }
   IntlStrings intl = new IntlStrings();
   String str2 = intl.GetString(8, true, true);
   str1 = intl.GetString(4, true, true);
   str1 = String.Concat(str2, str1);
   iCount = 11;
   iStartIndex = 1;
   iCountTestcases++;
   str1 = str1.Remove(iStartIndex, iCount);
   if(str1[0] != str2[0]) {
   Console.WriteLine("WHOA ERROR" + str1 + str2);
   iCountErrors++;
   }
   strLoc = "Loc_38947";
   str1 = "\t\t";
   iCount = 1;
   iStartIndex = -1;
   iCountTestcases++;
   try {
   str1.Remove(iStartIndex, iCount);
   iCountErrors++;
   Console.WriteLine( s_strTFAbbrev+ "Err_4982u");
   } catch (ArgumentException) {}
   catch ( Exception exc) {
   iCountErrors++;
   Console.WriteLine( s_strTFAbbrev+ "Err_9083u! , exc=="+exc);
   }
   strLoc = "Loc_129ew";
   str1 = "\t\t";
   iCount = 1;
   iStartIndex = 2;
   iCountTestcases++;
   try {
   str1.Remove(iStartIndex, iCount);
   iCountErrors++;
   Console.WriteLine( s_strTFAbbrev+ "Err_983uq");
   } catch ( ArgumentException) {}
   catch ( Exception exc) {
   iCountErrors++;
   Console.WriteLine( s_strTFAbbrev+ "Err_5388g! , exc=="+exc);
   }
   strLoc = "Loc_538wi";
   str1 = "\t\t";
   iCount = 3;
   iStartIndex = 0;
   iCountTestcases++;
   try {
   str1.Remove(iStartIndex, iCount);
   iCountErrors++;
   Console.WriteLine( s_strTFAbbrev+ "Err_128qu");
   } catch ( ArgumentException) {}
   catch ( Exception exc) {
   iCountErrors++;
   Console.WriteLine( s_strTFAbbrev+ "Err_3948u! , exc=="+exc);
   }
   strLoc = "Loc_3894u";
   str1 = "This is a string";
   iCount = str1.ToCharArray().Length;
   iStartIndex = 0;
   iCountTestcases++;
   if (!str1.Remove(iStartIndex, iCount).Equals(""))
     {
     iCountErrors++;
     Console.WriteLine( s_strTFAbbrev+ "Err_438du");
     }
   strLoc = "Loc_3948r";
   str1 = "\t\t\t\tThis is a string";
   iCount = 4;
   iStartIndex = 0;
   iCountTestcases++;
   if(!str1.Remove(iStartIndex, iCount).Equals("This is a string"))
     {
     iCountErrors++;
     Console.WriteLine( s_strTFAbbrev+ "Err_8904e");
     }
   strLoc = "Loc_3498j";
   str1 = "\n\nThis is a string";
   iCount = 6;
   iStartIndex = str1.LastIndexOf('s');
   iCountTestcases++;
   if(!str1.Remove(iStartIndex, iCount).Equals("\n\nThis is a "))
     {
     iCountErrors++;
     Console.WriteLine( s_strTFAbbrev+ "Err_1289m");
     }
   strLoc = "Loc_3487y";
   str1 = "This is a string";
   iCount = 6;
   iStartIndex = str1.IndexOf(' ');
   iCountTestcases++;
   if(!str1.Remove(iStartIndex, iCount).Equals("Thisstring"))
     {
     iCountErrors++;
     Console.WriteLine( s_strTFAbbrev+ "Err_3739c");
     }
   } catch (Exception exc_general ) {
   ++iCountErrors;
   Console.WriteLine(s_strTFAbbrev +" Error Err_8888yyy!  strLoc=="+ strLoc +", exc_general=="+exc_general);
   }
   if ( iCountErrors == 0 ) {   return true; }
   else {  return false;}
   }
 public static void Main(String[] args)
   {
   bool bResult = false;
   Co5154Remove_ii cbA = new Co5154Remove_ii();
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
