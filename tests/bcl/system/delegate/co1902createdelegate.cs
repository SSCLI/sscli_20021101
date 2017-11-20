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
using System.Threading;
using System.Reflection;
class foo {
 public int bar() {
 return 17;
 }
 public static Int32 barprime(Int32 x) {
 return x;
 }
}
public class objDelegate
{
 public static void meth (String msg) {
 Console.WriteLine (msg);
 }
 public void instmeth (String msg) {
 Console.WriteLine (msg);
 }
}
public class tryProps
{
 String strfoo="anything";
 private static string name;
 public static string Name {
 get {
 return name; 
 }
 set {
 name = value; 
 }
 }
 private string age;
 public string Age {
 get {
 return age; 
 }
 set {
 age = value; 
 }
 }
}
public delegate Int32 DelA();
public delegate Int32 DelB(Int32 x);
public delegate Int32 DelC(System.DateTime a, System.DateTime b);
public delegate void staticDelegate (String msg);
public delegate void instDelegate (String msg);
public delegate string retStrDelegate ();
public delegate void vvDelegate ();
class Co1902CreateDelegate
{
 public String s_strActiveBugNums          = "";
 public String s_strDtTmVer                = "";
 public String s_strComponentBeingTested   = "System.Delegate";
 public String s_strTFName                 = "Co1902CreateDelegate()";
 public String s_strTFAbbrev               = "Co1902CreateDelegate";
 public String s_strTFPath                 = "";
 public Boolean verbose               	= false;
 public Boolean runTest()
   {
   int iCountTestcases = 0;
   int iCountErrors    = 0;
   iCountTestcases++;
   try {
   foo f = new foo();
   Delegate cdel = Delegate.CreateDelegate(Type.GetType("DelA"), f, "bar");
   Object ret = ((DelA) cdel).DynamicInvoke(null);
   if (17 != (int) ret)
     throw new Exception("Wrong return value from delegate");
   }
   catch (Exception ex) {
   ++iCountErrors;
   Console.WriteLine("Err_001a,  Unexpected exception was thrown ex: " + ex.ToString());
   }
   iCountTestcases++;
   try {
   MethodInfo mi = Type.GetType("foo").GetMethod("barprime");
   Delegate cdel = Delegate.CreateDelegate(Type.GetType("DelB"), mi);
   Object[] objs = new Object[1];
   objs[0] = 18;
   Object ret = ((DelB) cdel).DynamicInvoke(objs);
   if (18 != (int) ret)
     throw new Exception("Wrong return value from delegate");	 
   }
   catch (Exception ex) {
   ++iCountErrors;
   Console.WriteLine("Err_002a,  Unexpected exception was thrown ex: " + ex.ToString());
   }
   iCountTestcases++;
   try {
   MethodInfo methInfo = Type.GetType("foo").GetMethod("bar");
   Delegate cdel = Delegate.CreateDelegate(Type.GetType("DelA"), methInfo);
   throw new Exception("Yehx003abc: previous call should have thrown an exception");	    
   }
   catch (System.ArgumentException) {
   if (verbose) {
   Console.WriteLine ("YehExpected000a : Expected exception");
   }
   }
   catch (Exception ex) {
   ++iCountErrors;
   Console.WriteLine("Err_003a,  Unexpected exception was thrown ex: " + ex.ToString());
   }
   iCountTestcases++;
   try {
   foo f = new foo();
   Delegate cdel = Delegate.CreateDelegate(Type.GetType(null), f, "bar");
   throw new Exception("Yehx004abc: previous call should have thrown an exception");	    
   }
   catch (System.ArgumentException) {
   if (verbose){
   Console.WriteLine ("YehExpected000b : Expected exception");
   }
   }
   catch (Exception ex) {
   ++iCountErrors;
   Console.WriteLine("Err_004a,  Unexpected exception was thrown ex: " + ex.ToString());
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("staticDelegate"), Type.GetType("objDelegate"), "meth");
   if (verbose) {
   Object[] objs = new Object[1];
   objs[0] = (String)"Using CreateDelegate(Type, Type, String) correctly";
   staticDelegate.DynamicInvoke (objs);
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_005,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("staticDelegate"), Type.GetType("objDelegate"), "");
   throw new Exception("Yehx001: previous call should have thrown an exception");
   }
   catch (System.ArgumentException){
   if (verbose) {
   Console.WriteLine ("YehExpected001 : Expected exception");
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_006,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("staticDelegate"), Type.GetType(""), "meth");
   throw new Exception("Yehx002: previous call should have thrown an exception");
   }
   catch (System.ArgumentException){
   if (verbose) {
   Console.WriteLine ("YehExpected002 : Expected exception");
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_007,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType(""), Type.GetType("objDelegate"), "meth");
   throw new Exception("Yehx003: previous call should have thrown an exception");
   }
   catch (System.ArgumentException){
   if (verbose) {
   Console.WriteLine ("YehExpected003 : Expected exception");
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_008,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("objDelegate"), Type.GetType("objDelegate"), "meth");
   throw new Exception("Yehx004: previous call should have thrown an exception");
   }
   catch (System.ArgumentException){
   if (verbose) {
   Console.WriteLine ("YehExpected004 : Expected exception");
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_009,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("staticDelegate"), Type.GetType("objDelegate"), "objDelegate");
   throw new Exception("Yehx005: previous call should have thrown an exception");
   }
   catch (System.ArgumentException){
   if (verbose) {
   Console.WriteLine ("YehExpected005 : Expected exception");
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_010,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("staticDelegate"), Type.GetType("foo"), "barprime");
   throw new Exception("Yehx006: previous call should have thrown an exception");
   }
   catch (System.ArgumentException){
   if (verbose) {
   Console.WriteLine ("YehExpected006 : Expected exception");
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_011,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   MethodInfo methInfo = Type.GetType("objDelegate").GetMethod("meth");
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("staticDelegate"), methInfo);
   if (verbose) {
   Object[] objs = new Object[1];
   objs[0] = (String)"Running staticDelegate.DynamicInvoke after CreateDelegate (Type, MethodInfo) with correct parameters";
   staticDelegate.DynamicInvoke (objs);
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_012,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("staticDelegate"), null);
   throw new Exception("Yehx007: previous call should have thrown an exception");
   }
   catch (System.ArgumentException){
   if (verbose) {
   Console.WriteLine ("YehExpected007 : Expected exception");
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_013,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   MethodInfo methInfo = Type.GetType("objDelegate").GetMethod("meth");
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType(""), methInfo);
   throw new Exception("Yehx008: previous call should have thrown an exception");
   }
   catch (System.ArgumentException){
   if (verbose) {
   Console.WriteLine ("YehExpected008 : Expected exception");
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_014,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   MethodInfo methInfo = Type.GetType("objDelegate").GetMethod("meth");
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("objDelegate"), methInfo);
   throw new Exception("Yehx009: previous call should have thrown an exception");
   }
   catch (System.ArgumentException){
   if (verbose) {
   Console.WriteLine ("YehExpected009 : Expected exception");
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_015,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   objDelegate instanceDelInst= new objDelegate();
   Delegate instanceDelegate = Delegate.CreateDelegate(Type.GetType("instDelegate"), instanceDelInst, "meth");
   throw new Exception("Yehx010: previous call should have thrown an exception");
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected010 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_016,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   objDelegate stDelInst= new objDelegate();
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("staticDelegate"), Type.GetType("objDelegate"), "instmeth");
   throw new Exception("Yehx011: previous call should have thrown an exception");
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected011 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_017,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   objDelegate instanceDelInst= new objDelegate();
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("instDelegate"), instanceDelInst, "instmeth");
   if (verbose) {
   Object[] objs = new Object[1];
   objs[0] = (String)"Using CreateDelegate(Type, Object, String) correctly";
   staticDelegate.DynamicInvoke (objs);
   }
   }
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_018,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate instanceDelegate = Delegate.CreateDelegate(Type.GetType("instDelegate"), null, "instmeth");
   throw new Exception("Yehx012: previous call should have thrown an exception");
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected012 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_019,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   foo f=new foo();			
   Delegate instanceDelegate = Delegate.CreateDelegate(Type.GetType("instDelegate"), f, "instmeth");
   throw new Exception("Yehx013: previous call should have thrown an exception");
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected013 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_020,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("vvDelegate"), Type.GetType("tryProps"), "strFoo");
   throw new Exception("Yehx016: previous call should have thrown an exception");
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected016 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_022,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(null, Type.GetType("objDelegate"), "meth");
   throw new Exception("Yehx016: previous call should have thrown an exception");		
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected016 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_022b,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("staticDelegate"), null, "meth");
   throw new Exception("Yehx017: previous call should have thrown an exception");		
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected017 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_022,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("staticDelegate"), Type.GetType("objDelegate"), null);
   throw new Exception("Yehx018: previous call should have thrown an exception");		
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected018 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_023,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   MethodInfo methInfo = Type.GetType("objDelegate").GetMethod("meth");
   Delegate staticDelegate = Delegate.CreateDelegate(null, methInfo);
   throw new Exception("Yehx019: previous call should have thrown an exception");		
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected019 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_024,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   objDelegate instanceDelInst= new objDelegate();
   Delegate staticDelegate = Delegate.CreateDelegate(null, instanceDelInst, "instmeth");
   throw new Exception("Yehx021: previous call should have thrown an exception");				
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected021 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_026,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   objDelegate instanceDelInst= new objDelegate();
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("instDelegate"), null, "instmeth");
   throw new Exception("Yehx022: previous call should have thrown an exception");				
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected022 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_027,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   objDelegate instanceDelInst= new objDelegate();
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("instDelegate"), instanceDelInst, null);
   throw new Exception("Yehx022: previous call should have thrown an exception");				
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected022 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_027,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   objDelegate instanceDelInst= new objDelegate();
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType(""), instanceDelInst, "instmeth");
   throw new Exception("Yehx023: previous call should have thrown an exception");				
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected023 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_028,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   objDelegate instanceDelInst= new objDelegate();
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("instDelegate"), instanceDelInst, "");
   throw new Exception("Yehx023: previous call should have thrown an exception");				
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected023 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_028,  Unexpected: " + exc.ToString() );
   }
   iCountTestcases++;
   try {
   tryProps instancetryProps= new tryProps();
   Delegate staticDelegate = Delegate.CreateDelegate(Type.GetType("retStrDelegate"), instancetryProps, "strFoo");
   throw new Exception("Yehx024: previous call should have thrown an exception");
   }
   catch (System.ArgumentException)
     {
     if (verbose) {
     Console.WriteLine ("YehExpected024 : Expected exception");
     }
     }	
   catch (Exception exc){
   ++iCountErrors;
   Console.WriteLine( "YehErr_029,  Unexpected: " + exc.ToString() );
   }
   if ( iCountErrors == 0 ) {   return true; }
   else {  return false;}
   }
 public static void Main( String [] args )
   {
   Co1902CreateDelegate runClass = new Co1902CreateDelegate();
   if ( args.Length > 0 )
     {
     Console.WriteLine( "Verbose ON!" );
     runClass.verbose = true;
     }
   Boolean bResult = runClass.runTest();
   if (! bResult) {
   Console.WriteLine( runClass.s_strTFPath  + runClass.s_strTFName );
   Console.Error.WriteLine( " " );
   Console.Error.WriteLine( "FAiL!  " + runClass.s_strTFAbbrev );
   Console.Error.WriteLine( " " );
   Console.Error.WriteLine( "ACTIVE BUGS: "  + runClass.s_strActiveBugNums );
   }
   if ( bResult == true )
     Environment.ExitCode = 0;
   else
     Environment.ExitCode = 11;
   }
}
