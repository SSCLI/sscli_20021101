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
using System.Text;
using System.Collections; 
delegate void Co3150_dlg_1();
delegate void Co3150_dlg_2();
delegate void Co3150_dlg_3(int i);
delegate int  Co3150_dlg_5(int i);
delegate short Co3150_dlg_4 (int i, long j);
delegate short Co3150_dlg_6 (int i, int j);
public delegate void Co3150_dlgmc_1();
public delegate void Co3150_dlgmc_2(int i);
public class Co3150Equals
{
 public Co3150_dlgmc_1 MCHandler;
 public Co3150_dlgmc_2 MCHandler2;
 public void method1() {}
 public void method1(int i) {}
 public int method2(int i) {return 0;}
 public void method2() {}
 public void method3(int i) {}
 public void method3() {}
 public short method4(int i, long j) {return (short)(i+(int)j);}
 public short method4(int i, int j) {return (short)(i+j);}
 public static void method5() {Console.Error.WriteLine("Blah");}
 public static int method5(int i) {return i;}
 public static short method5(int i, long j) {return (short)(i+(int)j);}
 public void AddToDelegate(Co3150_dlgmc_1 _handler)
   {
   MCHandler = (Co3150_dlgmc_1) Delegate.Combine(MCHandler, _handler);
   }
 public void AddToDelegate2(Co3150_dlgmc_2 _handler)
   {
   MCHandler2 = (Co3150_dlgmc_2) Delegate.Combine(MCHandler2, _handler);
   }
 public Boolean runTest()
   {
   Console.Out.WriteLine( "Delegate\\Co3150Equals. runTest() started." );
   int iCountErrors = 0;
   int iCountTestcases = 0;
   String strError = null;
   Co3150Equals cb1 = new Co3150Equals();
   Co3150Equals cb2 = new Co3150Equals();
   Co3150_dlg_1 dlg1 = null;
   Co3150_dlg_2 dlg2 = null;
   Co3150_dlg_3 dlg3 = null;
   Co3150_dlg_3 dlg4 = null;
   Co3150_dlg_5 dlg5 = null;
   Co3150_dlg_5 dlg6 = null;
   Co3150_dlg_4 dlg7 = null;
   Co3150_dlg_4 dlg8 = null;
   Co3150_dlg_6 dlg9 = null;
   Co3150_dlgmc_1 mcDlg1a = null;
   Co3150_dlgmc_1 mcDlg1b = null;
   Co3150_dlgmc_1 mcDlg1c = null;
   Co3150_dlgmc_2 mcDlg2a = null;
   Co3150_dlgmc_2 mcDlg2c = null;
   dlg1 = new Co3150_dlg_1(Co3150Equals.method5);
   dlg2 = new Co3150_dlg_2(Co3150Equals.method5);
   iCountTestcases++;
   if(!dlg1.Equals(dlg2))
     {
     iCountErrors++;
     print("E_374n");   
     }
   dlg7 = new Co3150_dlg_4(Co3150Equals.method5);
   dlg8 = new Co3150_dlg_4(Co3150Equals.method5);
   iCountTestcases++;
   if(!dlg7.Equals(dlg8))
     {
     iCountErrors++;
     print("E_348e");
     }
   dlg7 = new Co3150_dlg_4(Co3150Equals.method5);
   iCountTestcases++;
   try {
   if(!dlg7.Equals(dlg7))
     {
     iCountErrors++;
     print("E_41jn");
     }
   } catch (Exception exc) {
   iCountErrors++;
   print("E_34ej");
   strError = "EXTENEDINFO: "+exc.ToString();
   Console.Error.WriteLine(strError);
   }
   dlg7 = new Co3150_dlg_4(Co3150Equals.method5);
   dlg6 = new Co3150_dlg_5(Co3150Equals.method5);
   iCountTestcases++;
   if(dlg6.Equals(dlg7))
     {
     iCountErrors++;
     print("E_719qu");
     }
   dlg1 = new Co3150_dlg_1(cb1.method1);
   iCountTestcases++;
   if(!dlg1.Equals(dlg1))
     {
     iCountErrors++;
     print("E_239a");
     }
   dlg1 = new Co3150_dlg_1(cb1.method1);
   dlg2 = new Co3150_dlg_2(cb1.method1);
   iCountTestcases++;
   if(!dlg1.Equals(dlg2))
     {
     iCountErrors++;
     print("E_49ww");
     }
   dlg3 = new Co3150_dlg_3(cb1.method1);
   dlg4 = new Co3150_dlg_3(cb1.method1);
   iCountTestcases++;
   if(!dlg3.Equals(dlg4))
     {
     iCountErrors++;
     print("E_20ka");
     }
   dlg3.DynamicInvoke(new Object[]{5});
   dlg5 = new Co3150_dlg_5(cb1.method2);
   dlg6 = new Co3150_dlg_5(cb1.method2);
   iCountTestcases++;
   if(!dlg5.Equals(dlg6))
     {
     iCountErrors++;
     print("E_239a");
     }
   dlg7 = new Co3150_dlg_4(cb1.method4);
   dlg8 = new Co3150_dlg_4(cb1.method4);
   iCountTestcases++;
   if(!dlg7.Equals(dlg8))
     {
     iCountErrors++;
     print("E_481f");
     }
   dlg8 = new Co3150_dlg_4(cb1.method4);
   dlg9 = new Co3150_dlg_6(cb1.method4);
   iCountTestcases++;
   if(dlg8.Equals(dlg9))
     {
     iCountErrors++;
     print("E_752n");
     }
   dlg1 = new Co3150_dlg_1(cb1.method1);
   dlg2 = null;
   iCountTestcases++;
   if(dlg1.Equals(dlg2))
     {
     iCountErrors++;
     print("E_58ak");
     }
   dlg1 = new Co3150_dlg_1(cb1.method1);
   dlg3 = new Co3150_dlg_3(cb1.method1);
   iCountTestcases++;
   if(dlg3.Equals(dlg1))
     {
     iCountErrors++;
     print("E_34uw");
     }
   dlg1 = new Co3150_dlg_1(cb1.method1);
   dlg5 = new Co3150_dlg_5(cb1.method2);
   iCountTestcases++;
   if(dlg5.Equals(dlg1))
     {
     iCountErrors++;
     print("E_32sk");
     }
   mcDlg1a = new Co3150_dlgmc_1(cb1.method1);
   mcDlg1b = new Co3150_dlgmc_1(cb1.method3);
   try {
   cb1.AddToDelegate(mcDlg1a);
   cb1.AddToDelegate(mcDlg1b);
   cb2.AddToDelegate(mcDlg1a);
   cb2.AddToDelegate(mcDlg1b);
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_389a");
     strError = "EXTENEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   iCountTestcases++;
   if(!(cb1.MCHandler).Equals(cb2.MCHandler))
     {
     iCountErrors++;
     print("E_93kw");
     }
   mcDlg2a = new Co3150_dlgmc_2(cb1.method1);
   mcDlg2c = new Co3150_dlgmc_2(cb2.method3);
   iCountTestcases++;
   if(mcDlg2a.Equals(mcDlg2c))
     {
     iCountErrors++;
     print("E_43qk");
     }
   Delegate[] args = new Delegate[3]; 
   args[0] = new Co3150_dlgmc_1(cb1.method1);
   args[1] = new Co3150_dlgmc_1(cb1.method2);
   args[2] = new Co3150_dlgmc_1(cb1.method3);
   try {
   mcDlg1c = (Co3150_dlgmc_1)Delegate.Combine(args[0],args[1]);
   mcDlg1c = (Co3150_dlgmc_1)Delegate.Combine(mcDlg1c, args[2]);
   mcDlg1b = (Co3150_dlgmc_1)Delegate.Combine(args[0],args[1]);
   } catch (Exception exc) {iCountErrors++;print("E_482i");}
   Console.WriteLine (mcDlg1c.GetInvocationList().Length);
   iCountTestcases++;
   if(!mcDlg1c.GetInvocationList()[0].Equals(args[0]))
     {
     iCountErrors++;
     print("E_shj2");
     }
   iCountTestcases++;
   if(!mcDlg1c.GetInvocationList()[1].Equals(args[1]))
     {
     iCountErrors++;
     print("E_zdf2");
     }
   iCountTestcases++;
   if(!mcDlg1c.GetInvocationList()[2].Equals(args[2]))
     {
     iCountErrors++;
     print("E_uy22");
     }
   iCountTestcases++;
   if(!mcDlg1b.GetInvocationList()[0].Equals(args[0]))
     {
     iCountErrors++;
     print("E_suy2");
     }
   iCountTestcases++;
   if(!mcDlg1b.GetInvocationList()[1].Equals(args[1]))
     {
     iCountErrors++;
     print("E_ayt2");
     }
   iCountTestcases++;
   if(mcDlg1c.GetInvocationList()[0].Equals(mcDlg1b))
     {
     iCountErrors++;
     print("E_38eu");
     }
   if ( iCountErrors == 0 ) {   return true; }
   else {  return false;}
   }
 private void print(String error)
   {
   StringBuilder output = new StringBuilder("POINTTOBREAK: find ");
   output.Append(error);
   output.Append(" (Co3150Equals.)");
   Console.Out.WriteLine(output.ToString());
   }
 public static void Main( String[] args ) 
   {
   Boolean bResult = false; 
   Co3150Equals cb0 = new Co3150Equals();
   try
     {
     bResult = cb0.runTest();
     }
   catch ( System.Exception exc )
     {
     bResult = false;
     System.Console.Error.WriteLine(  ("Co3150Equals.")  );
     System.Console.Error.WriteLine( exc.ToString() );
     }
   if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 11; 
   }
}
