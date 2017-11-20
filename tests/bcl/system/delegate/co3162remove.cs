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
using System.Text;
using System;
using System.Collections;
using System.Reflection;
delegate void Co3162_dlg_1();
delegate int Co3162_dlg_3(int i);
delegate void Co3162_dlgmc_1();
public class Co3162Remove
{
 public virtual void method1() {}
 public virtual int method1(int i) {return 0;}
 public virtual int method2(int i) {return 0;}
 public virtual void method2() {}
 public virtual void method3(int i) {}
 public virtual void method3() {}
 public static void static_method1() {}
 public static int static_method1(int i) {return 0;}
 public static int static_method2(int i) {return 0;}
 public static void static_method2() {}
 public static void static_method3(int i) {}
 public static void static_method3() {}
 public virtual bool runTest()
   {
   Console.Out.WriteLine( "Delegate\\Co3162Remove. runTest() started." );
   int iCountErrors = 0;
   int iCountTestcases = 0;
   int in4w1 = 0;
   String strError = null;
   Co3162Remove cb1 = new Co3162Remove();
   Co3162Remove cb2 = new Co3162Remove();
   Co3162_dlg_1 dlg1 = null;
   Co3162_dlg_1 dlg2 = null;
   Co3162_dlgmc_1 mcDlg1a = null;
   Co3162_dlgmc_1 mcDlg1b = null;
   Delegate[] inputdlgs = null;
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Remove(null, null);
   iCountTestcases++;
   if(mcDlg1a != null)
     {
     iCountErrors++;
     print("E_r7331");
     }
   inputdlgs = new Delegate[5];
   inputdlgs[0] = new Co3162_dlgmc_1(cb1.method1);
   inputdlgs[1] = new Co3162_dlgmc_1(cb1.method1);
   inputdlgs[2] = new Co3162_dlgmc_1(cb1.method1);
   try {
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Combine(inputdlgs);
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Remove(mcDlg1a, inputdlgs[2]);
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_28wq");
     strError = "EXTENDEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   iCountTestcases++;
   if(mcDlg1a.GetInvocationList().Length != 2)
     {
     iCountErrors++;
     print("E_92ks");
     }
   inputdlgs = new Delegate[5];
   inputdlgs[0] = new Co3162_dlgmc_1(Co3162Remove.static_method1);
   inputdlgs[1] = new Co3162_dlgmc_1(Co3162Remove.static_method1);
   inputdlgs[2] = new Co3162_dlgmc_1(Co3162Remove.static_method1);
   try {
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Combine(inputdlgs);
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Remove(mcDlg1a, inputdlgs[2]);
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_284wq");
     strError = "EXTENDEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   iCountTestcases++;
   if(mcDlg1a.GetInvocationList().Length != 2)
     {
     iCountErrors++;
     print("E_921ks");
     }
   inputdlgs = new Delegate[5];
   inputdlgs[0] = new Co3162_dlgmc_1(cb1.method1);
   inputdlgs[1] = new Co3162_dlgmc_1(cb1.method1);
   inputdlgs[2] = new Co3162_dlgmc_1(cb1.method3);
   try {
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Combine(inputdlgs);
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Remove(mcDlg1a, inputdlgs[2]);
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_28wq");
     strError = "EXTENDEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   iCountTestcases++;
   if(!(mcDlg1a.GetInvocationList())[0].Method.Name.Equals("method1"))
     {
     iCountErrors++;
     print("E_389a");
     }
   if(!(mcDlg1a.GetInvocationList())[1].Method.Name.Equals("method1"))
     {
     iCountErrors++;
     print("E_1sjl");
     }
   if(mcDlg1a.GetInvocationList().Length != 2)
     {
     iCountErrors++;
     print("E_0erl");
     }
   inputdlgs = new Delegate[5];
   inputdlgs[0] = new Co3162_dlgmc_1(Co3162Remove.static_method1);
   inputdlgs[1] = new Co3162_dlgmc_1(Co3162Remove.static_method3);
   inputdlgs[2] = new Co3162_dlgmc_1(Co3162Remove.static_method1);
   try {
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Combine(inputdlgs);
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Remove(mcDlg1a, new Co3162_dlgmc_1(Co3162Remove.static_method3));
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_286wq");
     strError = "EXTENDEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   iCountTestcases++;  
   if(!mcDlg1a.GetInvocationList()[0].Method.Name.Equals("static_method1"))
     {
     iCountErrors++;
     print("E_3891a");
     }
   if(!(mcDlg1a.GetInvocationList())[1].Method.Name.Equals("static_method1"))
     {
     iCountErrors++;
     print("E_1sjl5");
     }
   if(mcDlg1a.GetInvocationList().Length != 2)
     {
     iCountErrors++;
     print("E_0erl");
     }
   dlg1 = new Co3162_dlg_1(cb1.method1);
   dlg2 = null;
   try {
   dlg2 = (Co3162_dlg_1)Delegate.Remove(dlg1, dlg1);
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_329s");
     strError = "EXTENDEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   iCountTestcases++;
   if(dlg2 != null)
     {
     iCountErrors++;
     print("E_934k");
     }
   dlg1 = new Co3162_dlg_1(Co3162Remove.static_method1);
   dlg2 = null;
   try {
   dlg2 = (Co3162_dlg_1)Delegate.Remove(dlg1, dlg1);
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_329s");
     strError = "EXTENDEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   iCountTestcases++;
   if(dlg2 != null)
     {
     iCountErrors++;
     print("E_934k");
     }
   dlg1 = new Co3162_dlg_1(cb1.method1);
   dlg2 = null;
   mcDlg1a = new Co3162_dlgmc_1(cb1.method1);
   try {
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Remove(dlg1, mcDlg1a);
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_293j");
     strError = "EXTENDEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   iCountTestcases++;
   if(dlg2 != null)
     {
     iCountErrors++;
     print("E_298i");
     }
   inputdlgs = new Delegate[5];
   inputdlgs[0] = new Co3162_dlgmc_1(cb1.method3);
   inputdlgs[1] = new Co3162_dlgmc_1(cb1.method2);
   inputdlgs[2] = new Co3162_dlgmc_1(cb1.method1);
   mcDlg1b = null;
   mcDlg1a = new Co3162_dlgmc_1(cb1.method3);
   try {
   mcDlg1b = (Co3162_dlgmc_1)Delegate.Combine(inputdlgs);
   mcDlg1b = (Co3162_dlgmc_1)Delegate.Remove(mcDlg1b, mcDlg1a);
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_30kl");
     strError = "EXTENDEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   iCountTestcases++;
   if(mcDlg1b.GetInvocationList().Length != 2)
     {
     iCountErrors++;
     print("E_328q");
     }
   inputdlgs = new Delegate[5];
   inputdlgs[0] = new Co3162_dlgmc_1(Co3162Remove.static_method3);
   inputdlgs[1] = new Co3162_dlgmc_1(Co3162Remove.static_method2);
   inputdlgs[2] = new Co3162_dlgmc_1(Co3162Remove.static_method1);
   mcDlg1b = null;
   mcDlg1a = new Co3162_dlgmc_1(Co3162Remove.static_method3);
   try {
   mcDlg1b = (Co3162_dlgmc_1)Delegate.Combine(inputdlgs);
   mcDlg1b = (Co3162_dlgmc_1)Delegate.Remove(mcDlg1b, mcDlg1a);
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_303kl");
     strError = "EXTENDEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   iCountTestcases++;
   if(mcDlg1b.GetInvocationList().Length != 2)
     {
     iCountErrors++;
     print("E_3281q");
     }
   inputdlgs = new Delegate[5];
   inputdlgs[0] = new Co3162_dlgmc_1(cb1.method1);
   inputdlgs[1] = new Co3162_dlgmc_1(cb1.method2);
   inputdlgs[2] = new Co3162_dlgmc_1(cb1.method3);
   mcDlg1b = null;
   try {
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Combine(inputdlgs[1], inputdlgs[2]);
   mcDlg1b = (Co3162_dlgmc_1)Delegate.Combine(inputdlgs);
   mcDlg1b = (Co3162_dlgmc_1)Delegate.Remove(mcDlg1b, mcDlg1a);
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_820k");
     strError = "EXTENDEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   in4w1 = mcDlg1b.GetInvocationList().Length;
   iCountTestcases++;
   if(in4w1 != 1)  
     {
     iCountErrors++;
     print("E_57is!  in4w1=" + in4w1);
     }
   inputdlgs = new Delegate[5];
   inputdlgs[0] = new Co3162_dlgmc_1(cb1.method1);
   inputdlgs[1] = new Co3162_dlgmc_1(cb1.method1);
   inputdlgs[2] = new Co3162_dlgmc_1(cb1.method3);
   dlg1 = new Co3162_dlg_1(cb1.method3);
   try {
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Combine(inputdlgs);
   mcDlg1a = (Co3162_dlgmc_1)Delegate.Remove(mcDlg1a, dlg1);
   } catch (Exception exc)
     {
     iCountErrors++;
     print("E_293j");
     strError = "EXTENDEDINFO: "+exc.ToString();
     Console.Error.WriteLine(strError);
     }
   iCountTestcases++;
   if(mcDlg1a.GetInvocationList().Length != 2)
     {
     iCountErrors++;
     print("E_471w");
     }
   if(!mcDlg1a.GetInvocationList()[0].Method.Name.ToString().Equals("method1"))
     {
     iCountErrors++;
     print("E_519e");
     }
   if(!mcDlg1a.GetInvocationList()[1].Method.Name.ToString().Equals("method1"))
     {
     iCountErrors++;
     print("E_548w");
     }
   if ( iCountErrors == 0 ) {   return true; }
   else {  return false;}
   }
 private void print(String error)
   {
   StringBuilder output = new StringBuilder("POINTTOBREAK: find ");
   output.Append(error);
   output.Append(" (Co3162Remove.)");
   Console.Out.WriteLine(output.ToString());
   }
 public static void Main( String[] args )
   {
   bool bResult = false; 
   Co3162Remove cb0 = new Co3162Remove();
   try
     {
     bResult = cb0.runTest();
     }
   catch ( System.Exception exc )
     {
     bResult = false;
     System.Console.Error.WriteLine( "Co3162Remove."  );
     System.Console.Error.WriteLine( exc.ToString() );
     }
   if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 11; 
   }
}
