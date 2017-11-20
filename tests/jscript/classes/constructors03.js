//# ==++== 
//# 
//#   
//#    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//#   
//#    The use and distribution terms for this software are contained in the file
//#    named license.txt, which can be found in the root of this distribution.
//#    By using this software in any fashion, you are agreeing to be bound by the
//#    terms of this license.
//#   
//#    You must not remove this notice, or any other, from this software.
//#   
//# 
//# ==--== 
//####################################################################################
@cc_on


import System;

var NULL_DISPATCH = null;
var apGlobalObj;
var apPlatform;
var lFailCount;


var iTestID = 193754;



// -----------------------------------------------------------------------
var actual = 0;
var expected = 0;
var exceptionThrown = false;
var expectedError = "";
var actualError = "";
var globalCtr = 0;


// -----------------------------------------------------------------------
var exceptionThrown15_1 = false;
var actualError15_1 = "";
var expectedError15_1 = "SyntaxError: A constructor function may not have a return type";

try
{
   eval ("  class Alpha15_1                        " +
         "  {                                      " +
         "     public function Alpha15_1(): int    " +
         "     {                                   " +
         "        var i: int;                      " +
         "        i = 10;                          " +
         "        return i;                        " +
         "     }                                   " +
         "  }                                      " +
         "  class Beta15_1 extends Alpha15_1       " +
         "  {                                      " +
         "  }                                      ", "unsafe");
}
catch (error)
{
   exceptionThrown15_1 = true;
   actualError15_1 = error;
}


// -----------------------------------------------------------------------
var exceptionThrown15_2 = false;
var actualError15_2 = "";
var expectedError15_2 = "TypeError: Cannot return a value from a constructor or void function";

try
{
   eval ("  class Alpha15_2                        " +
         "  {                                      " +
         "     public function Alpha15_2()         " +
         "     {                                   " +
         "        var i: String;                   " +
         "        i = \"hello\";                   " +
         "        return i;                        " +
         "     }                                   " +
         "  }                                      " +
         "  class Beta15_2 extends Alpha15_2       " +
         "  {                                      " +
         "  }                                      ", "unsafe");
}
catch (error)
{
   exceptionThrown15_2 = true;
   actualError15_2 = error;
}


// -----------------------------------------------------------------------
var exceptionThrown16 = false;
var actualError16 = "";
var expectedError16 = "SyntaxError: Syntax error. Write 'function identifier(...) : Type{' rather than 'Type identifier(...){' to declare a typed function";

try
{
   eval ("  class Alpha16              " +
         "  {                          " +
         "     public void Alpha16()   " +
         "     {                       " +
         "     }                       " +
         "  }                          ");
}
catch (error)
{
   exceptionThrown16 = true;
   actualError16 = error;
}


// -----------------------------------------------------------------------
class Alpha17_1
{
   public var value = "";
   
   public function Alpha17_1 (x: boolean)
   {
      value = "boolean";
   }

   public function Alpha17_1 (x: int)
   {
      value = "int";
   }

   public function Alpha17_1 (x: String)
   {
      value = "String";
   }

   public function Alpha17_1 (x: char)
   {
      value = "char";
   }
}


// -----------------------------------------------------------------------
class Alpha17_2
{
   public var value: String = "none";
   
   public function Alpha17_2 (x: int, y: String)
   {
      value = "two";
   }
   
   public function Alpha17_2 (x: int)
   {
      value = "one";
   }
}  


// -----------------------------------------------------------------------
class Alpha17_3
{
   public var value: String = "none";

   public function Alpha17_3 (x: int, y: String)
   {
      value = "two";
   }
   
   public function Alpha17_3()
   {
      value = "three";
   }
   
   public function Alpha17_3 (x: boolean)
   {
      value = "four";
   }
   
   public function Alpha17_3 (x: int)
   {
      value = "one";
   }
}


// -----------------------------------------------------------------------
class Alpha17_4
{
   public var value = "none";
   
   public function Alpha17_4 (x: int, y: boolean)
   {
      value = "one";
   }
   
   public function Alpha17_4 (x: double, y: Boolean)
   {
      value = "two";
   }
}


// -----------------------------------------------------------------------
class Alpha17_5
{
   public var value = "none";
   
   public function Alpha17_5 (x: int, y: Number)
   {
      value = "one";
   }
   
   public function Alpha17_5 (x: double, y: String)
   {
      value = "two";
   }  
}


// -----------------------------------------------------------------------
/*
Waiting for Peter's reply

class Alpha17_6
{
   public var value = "none";
   
   public function Alpha17_6 (x: int, y: Object)
   {
      value = "one";
   }
   
   public function Alpha17_6 (x: double, y: String)
   {
      value = "two";
   }
}
*/


// -----------------------------------------------------------------------
class Alpha17_7
{
   public var value = "none";
   
   public function Alpha17_7 (x: boolean, y: Object)
   {
      value = "one";
   }
   
   public function Alpha17_7 (x: int, y: String) 
   {
      value = "two";
   }
}


// -----------------------------------------------------------------------
class Alpha17_8
{
   public var value = "none";
   
   public function Alpha17_8 (x: int, y: String, z: boolean)
   {
      value = "two"
   }

   public function Alpha17_8 (x: int, y: String)
   {
      value = "one";
   }
}



// -----------------------------------------------------------------------
function constructors03()
{
   apInitTest ("Constructors03");


   // -----------------------------------------------------------------------
   apInitScenario ("15.1 Superclass constructor returns a value; type annotated");
   // Compile error
   
   if (exceptionThrown15_1 == false)
      apLogFailInfo ("No compile error", "Should give a compile error", exceptionThrown15_1, "200532");
   if (actualError15_1 != expectedError15_1)
      apLogFailInfo ("Wrong compile error message", expectedError15_1, actualError15_1, "200532");  
      
      
   // -----------------------------------------------------------------------
   apInitScenario ("15.2 Superclass constructor returns a value; not type annotated");
   // Compile error
         
   if (exceptionThrown15_2 == false)
      apLogFailInfo ("No compile error", "Should give a compile error", exceptionThrown15_2, "200532");
   if (actualError15_2 != expectedError15_2)
      apLogFailInfo ("Wrong compile error message", expectedError15_2, actualError15_2, "200532"); 


   // -----------------------------------------------------------------------
   apInitScenario ("16. C++ style declaration");
   // Compile error
   
   if (exceptionThrown16 == false)
      apLogFailInfo ("No compile error", "Should give a compile error", exceptionThrown16, "200378");
   if (actualError16 != expectedError16)
      apLogFailInfo ("Wrong compile error message", expectedError16, actualError16, "200378"); 
   
         
   // -----------------------------------------------------------------------
   apInitScenario ("17.1 Overloaded constructors -- Alpha (boolean), Alpha (int), Alpha (String)");
   
   // new Alpha (true)
   actual = "";
   expected = "boolean";
   var alpha17_1_1: Alpha17_1 = new Alpha17_1 (true);
   actual = alpha17_1_1.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.1 (1)", expected, actual, "");
   
   // new Alpha (5)   
   actual = "";
   expected = "int";
   var alpha17_1_2: Alpha17_1 = new Alpha17_1 (5);
   actual = alpha17_1_2.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.1 (2)", expected, actual, "");

   // new Alpha ("hello")
   actual = "";
   expected = "String";
   var alpha17_1_3: Alpha17_1 = new Alpha17_1 ("hello");
   actual = alpha17_1_3.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.1 (3)", expected, actual, "");

   // new Alpha ('c')
   var charVariable: char = 'c';
   actual = "";
   expected = "char";
   var alpha17_1_4: Alpha17_1 = new Alpha17_1 (charVariable);
   actual = alpha17_1_4.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.1 (4)", expected, actual, "");

   // new Alpha()     
   var alpha17_1_5: Alpha17_1 = new Alpha17_1 ();
    
   expected = "String";
   actual = "";
   actual = alpha17_1_5.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.1 (5)", expected, actual, "");  

         
   // -----------------------------------------------------------------------
   apInitScenario ("17.2 Overloaded constructors -- Alpha (int), Alpha (int, String)");

   // new Alpha (5)
   actual = "";
   expected = "one";
   var alpha17_2_1: Alpha17_2 = new Alpha17_2 (5);
   actual = alpha17_2_1.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.2 (1)", expected, actual, "");

   // new Alpha (5, "hello")
   actual = "";
   expected = "two";
   var alpha17_2_2: Alpha17_2 = new Alpha17_2 (5, "hello");
   actual = alpha17_2_2.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.2 (2)", expected, actual, "");

   // new Alpha()
   actual = "";
   expected = "one";
   var alpha17_2_3: Alpha17_2 = new Alpha17_2();
   actual = alpha17_2_3.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.2 (3)", expected, actual, "");
   
   // new Alpha ("hello")   
   exceptionThrown = false;
   expectedError = "TypeError: Type mismatch";
   actualError = "";      
   try
   {
      eval ("var alpha17_2_4: Alpha17_2 = new Alpha17_2 (\"hello\");");
   }  
   catch (error)
   {
      exceptionThrown = true;
      actualError = error;
   }  
   if (exceptionThrown == false)
      apLogFailInfo ("Error in 17.2 (4)", "Should give a compile error", exceptionThrown, "");
   if (actualError != expectedError)
      apLogFailInfo ("Wrong compile error", expectedError, actualError, "");    
      
      
   // -----------------------------------------------------------------------
   apInitScenario ("17.3 Overloaded constructors -- Alpha (int), Alpha (int, String), Alpha()");
   
   // new Alpha (5)
   actual = "";
   expected = "one";
   var alpha17_3_1: Alpha17_3 = new Alpha17_3 (5);
   actual = alpha17_3_1.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.3 (1)", expected, actual, "");

   // new Alpha (5, "hello")
   actual = "";
   expected = "two";
   var alpha17_3_2: Alpha17_3 = new Alpha17_3 (5, "hello");
   actual = alpha17_3_2.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.3 (2)", expected, actual, "");

   // new Alpha()
   actual = "";
   expected = "three";
   var alpha17_3_3: Alpha17_3 = new Alpha17_3();
   actual = alpha17_3_3.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.3 (3)", expected, actual, "");
   
   // new Alpha ("hello")                             
   exceptionThrown = false;
   expectedError = "ReferenceError: More than one constructor matches this argument list";
   actualError = "";      
   try
   {
      eval ("var alpha17_3_4: Alpha17_3 = new Alpha17_3 (\"hello\");");
   }  
   catch (error)
   {
      exceptionThrown = true;
      actualError = error;
   }  
   if (exceptionThrown == false)
      apLogFailInfo ("Error in 17.3 (4)", "Should give a compile error", exceptionThrown, "");
   if (actualError != expectedError)
      apLogFailInfo ("Wrong compile error", expectedError, actualError, "");   
      
   // new Alpha (true)
   actual = "";
   expected = "four";
   var alpha17_3_5: Alpha17_3 = new Alpha17_3 (true);
   actual = alpha17_3_5.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.3 (5)", expected, actual, "");
         
         
   // -----------------------------------------------------------------------
   apInitScenario ("17.4 Overloaded constructors -- Alpha (int, boolean), Alpha (double, Boolean)");
   
   // new Alpha (5, true)
   actual = "";
   expected = "one";
   var alpha17_4_1: Alpha17_4 = new Alpha17_4 (5, true);
   actual = alpha17_4_1.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.4 (1)", expected, actual, "");   
      
      
   // -----------------------------------------------------------------------
   apInitScenario ("17.5 Overloaded constructors -- Alpha (int, Number), Alpha (double, String)");
   
   // new Alpha (true, “hello”)
   actual = "";
   expected = "two";
   var alpha17_5_1: Alpha17_5 = new Alpha17_5 (true, "hello");
   actual = alpha17_5_1.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.5 (1)", expected, actual, "");
      
      
   // -----------------------------------------------------------------------
   apInitScenario ("17.6 Overloaded constructors -- Alpha (int, Object), Alpha (double, String)");
/*
Waiting for Peter's reply   
   
   // new Alpha (true, “hello”)
   actual = "";
   expected = "two";
   var alpha17_6_1: Alpha17_6 = new Alpha17_6 (true, "hello");
   actual = alpha17_6_1.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.6 (1)", expected, actual, "");
*/      
         
   
   // -----------------------------------------------------------------------
   apInitScenario ("17.7 Overloaded constructors -- Alpha (boolean, Object), Alpha (int, String)");

   var alpha17_7_1: Alpha17_7 = new Alpha17_7 (true, "hello");
   
   expected = "one";
   actual = "";
   actual = alpha17_7_1.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.7", expected, actual, "");
      
      
   // -----------------------------------------------------------------------
   apInitScenario ("17.8 Overloaded constructors -- Alpha (int, String), Alpha (int, String, boolean)");
   
   // new Alpha (5, “hello”)
   actual = "";
   expected = "one";
   var alpha17_8_1: Alpha17_8 = new Alpha17_8 (5, "hello");
   actual = alpha17_8_1.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.8 (1)", expected, actual, "");
   
   // new Alpha (5, “hello”, true)   
   actual = "";
   expected = "two";
   var alpha17_8_2: Alpha17_8 = new Alpha17_8 (5, "hello", true);
   actual = alpha17_8_2.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.8 (2)", expected, actual, "");
      
   // new Alpha (5)      
   actual = "";
   expected = "one";
   var alpha17_8_3: Alpha17_8 = new Alpha17_8 (5);
   actual = alpha17_8_3.value;
   if (actual != expected)
      apLogFailInfo ("Error in 17.8 (3)", expected, actual, "");
      
      
   apEndTest();
}



constructors03();


if(lFailCount >= 0) System.Environment.ExitCode = lFailCount;
else System.Environment.ExitCode = 1;

function apInitTest(stTestName) {
    lFailCount = 0;

    apGlobalObj = new Object();
    apGlobalObj.apGetPlatform = function Funca() { return "Rotor" }
    apGlobalObj.LangHost = function Funcb() { return 1033;}
    apGlobalObj.apGetLangExt = function Funcc(num) { return "EN"; }

    apPlatform = apGlobalObj.apGetPlatform();
    var sVer = "1.0";  //navigator.appVersion.toUpperCase().charAt(navigator.appVersion.toUpperCase().indexOf("MSIE")+5);
    apGlobalObj.apGetHost = function Funcp() { return "Rotor " + sVer; }
    print ("apInitTest: " + stTestName);
}

function apInitScenario(stScenarioName) {print( "\tapInitScenario: " + stScenarioName);}

function apLogFailInfo(stMessage, stExpected, stActual, stBugNum) {
    lFailCount = lFailCount + 1;
    print ("***** FAILED:");
    print ("\t\t" + stMessage);
    print ("\t\tExpected: " + stExpected);
    print ("\t\tActual: " + stActual);
}

function apGetLocale(){ return 1033; }
function apWriteDebug(s) { print("dbg ---> " + s) }
function apEndTest() {}
