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
public class Co5471op_Remainder
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strClassMethod   = "Decimal%Decimal";
 public static String s_strTFName        = "Co5471op_Remainder.cs";
 public static String s_strTFAbbrev      = "Co5471";
 public static String s_strTFPath        = Environment.CurrentDirectory;
 public bool runTest()
   {
   Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strLoc = "Loc_000oo";
   String strValue = String.Empty;
   Decimal[] arrD1 = {Decimal.MinValue, (Decimal)Int64.MinValue, (Decimal)Int32.MinValue, (Decimal)Int16.MinValue, -128m, -1m, 0, 1, 
		      127, 255, (Decimal)Int16.MaxValue, (Decimal)Int32.MaxValue, (Decimal)Int64.MaxValue, Decimal.MaxValue,
   };
   Decimal[] arrD2 = {Decimal.MinValue, (Decimal)Int64.MinValue, (Decimal)Int32.MinValue, (Decimal)Int16.MinValue, -128m, -1m, 0, 1, 
		      127, 255, (Decimal)Int16.MaxValue, (Decimal)Int32.MaxValue, (Decimal)Int64.MaxValue, Decimal.MaxValue,
   };
   try
     {
     Decimal dec1, dec2, dec3;
     strLoc = "Loc_2098u";
     dec1 = new Decimal(50);
     dec2 = new Decimal();
     iCountTestcases++;
     try {
     dec3 = dec1%dec2;
     iCountErrors++;
     printerr( "Error_908hc! DivideByZeroException expected");
     } catch (DivideByZeroException dExc) {
     Console.WriteLine("Info_98ehc! Caught Expected Exception, exc=="+dExc.Message);
     } catch (Exception exc) {
     iCountErrors++;
     printerr( "Error_09c98! DivideByZeroException expected, got exc=="+exc.ToString());
     }
     iCountTestcases++;
     try {
     dec3 = dec1%dec2;
     iCountErrors++;
     printerr( "Error_2908x! DivideByZeroException expected");
     } catch (DivideByZeroException dExc) {
     Console.WriteLine("Info_298xh! Caught expected Exception, exc=="+dExc.Message);
     } catch (Exception exc) {
     iCountErrors++;
     printerr( "Error_109ux! DivideByZeroException expected, got exc=="+exc.ToString());
     }
     strLoc = "Loc_0ucx8";
     dec1 = 5;
     dec2 = 3;
     iCountTestcases++;
     if((dec1%dec2) != 2)
       {
       iCountErrors++;
       printerr( "Error_209cx! Expected==2 , got value=="+Decimal.Remainder(5, 3).ToString());
       }
     iCountTestcases++;
     if(dec1%dec2 != 2)
       {
       iCountErrors++;
       printerr( "Error_289cy! Expected==2, got value=="+(dec1%dec2).ToString());
       }
     strLoc = "Loc_109xu";
     dec1 = Decimal.MaxValue;
     dec2 = Decimal.MaxValue;
     iCountTestcases++;
     if((dec1%dec2) != 0)
       {
       iCountErrors++;
       printerr( "Error_198cy! Expected==0, got value=="+Decimal.Remainder(dec1, dec2).ToString());
       }
     iCountTestcases++;
     if(dec1%dec2 != 0)
       {
       iCountErrors++;
       printerr( "Error_298xy! Expected==0, got value=="+(dec1%dec2).ToString());
       }
     strLoc = "Loc_39ccs";
     dec1 = 10;
     dec2 = -3;
     iCountTestcases++;
     if((dec1%dec2) != 1)
       {
       iCountErrors++;
       printerr( "Error_298cx! Expected==1, got value=="+Decimal.Remainder(dec1, dec2).ToString());
       }
     dec1 = -10;
     dec2 = 3;
     iCountTestcases++;
     if(dec1%dec2 != -1)
       {
       iCountErrors++;
       printerr( "Erro_28hxc! Expected==-1, got value=="+(dec1%dec2).ToString());
       }
     strLoc = "Loc_129dc";
     dec1 = Decimal.MaxValue;
     dec2 = Decimal.MinValue;
     iCountTestcases++;
     if((dec1%dec2) != 0)
       {
       iCountErrors++;
       printerr( "Error_198yc! Expected==0, got value=="+Decimal.Remainder(dec1, dec2).ToString());
       }
     strLoc = "Loc_189hc";
     dec1 = new Decimal(17.3);
     dec2 = 3;
     iCountTestcases++;
     if(dec1%dec2 != new Decimal(2.3))
       {
       iCountErrors++;
       printerr( "Error_28fy3! Expected==2.3, got value=="+(dec1%dec2).ToString());
       }
     strLoc = "Loc_98ycy";
     dec1 = new Decimal(8.55);
     dec2 = new Decimal(2.25);
     iCountTestcases++;
     if((dec1%dec2) != new Decimal(1.8))
       {
       iCountErrors++;
       printerr( "Error_298chx! Expected==0.8, got value=="+Decimal.Remainder(dec1,dec2).ToString());
       }
     strLoc = "Loc_98ycy";
     iCountTestcases++;
     dec1 = Decimal.MinValue;
     dec2 = new Decimal(-32768);
     if((dec1%dec2) != new Decimal(1))
       {
       iCountErrors++;
       printerr( "Error_sdafs! Expected==1, got value=="+Decimal.Remainder(dec1,dec2).ToString());
       }
     strLoc = "Loc_98ycy";
     iCountTestcases++;
     try{
     for(int i=0; i<arrD1.Length; i++){
     for(int j=0; j<arrD2.Length; j++){
     dec1 = (arrD1[i]%arrD2[j]);
     }
     }
     }catch (DivideByZeroException){
     }catch(Exception ex){
     iCountErrors++;
     Console.WriteLine( "Err_857wG! Exception thrown, ", ex);
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
   Console.WriteLine ("EXTENDEDINFO: ("+ s_strTFAbbrev + ") "+ info);
   }
 public static void Main(String[] args)
   {
   bool bResult = false;
   Co5471op_Remainder cbA = new Co5471op_Remainder();
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
