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


var iTestID = 160218;

/* -------------------------------------------------------------------------
  Test: 	TYPANN18
   
  
 
  Component:	JScript
 
  Major Area:	Type Annotation
 
  Test Area:	Type annotation for ActiveXObject
 
  Keywords:	type annotation annotate object activexobject
 
 ---------------------------------------------------------------------------
  Purpose:	Verify the functionality of new or changed features
 
  Scenarios:

		1.  var i16 : Int16;

		2.  var int : Int16;

		3.  var i16 : Int16 = 3;

		4.  var i16 : Int16 = Date.parse ('1/4/1970');

		5.  var i16 : Int16 = 'hello';

		6.  var i16 : Int16 = true;

		7.  var i16 : Int16 = ReturnInt16Val();


  Abstract:	 Testing variable assignment through type annotation..
 ---------------------------------------------------------------------------
  Category:			Functionality
 
  Product:			JScript
 
  Related Files: 
 
  Notes:
 ---------------------------------------------------------------------------
  
 


-------------------------------------------------------------------------*/


/*----------
/
/  Helper functions
/
----------*/


function verify(sAct, sExp, sMes, sBug){
	if (sAct != sExp)
        apLogFailInfo( "*** Scenario failed: "+sMes, sExp, sAct, sBug);
}


function ReturnInt16Val() {
	var n : Int16 = 42;

	return n;
}


/*----------
/
/  Class definitions
/
----------*/

	class Person {
		private var iAge : int;
	
		public function GetAge() {
			return iAge;
		}
		
		public function SetAge(iNewAge) {
			iAge = iNewAge;
		}
	}

	class Student extends Person {
		private var iGPA : float;
	
		public function GetGPA() {
			return iGPA;
		}
		
		public function SetGPA(iNewGPA) {
			iGPA = iNewGPA;
		}
	}



import System

function typann18() {

    apInitTest("typann18: Type annotation -- Object types"); 

	apInitScenario("1.  var i16 : Int16;");
	try {
		var i1 : Int16;
	}
	catch (e) {
		if (e.number != 0) {
	        apLogFailInfo( "*** Scenario failed: 1.1 Error returned", e.description, "error", "");
		}
	}
	verify (i1, 0, "1.2 Wrong value","");
	verify (i1.GetType(), "System.Int16", "1.3 Wrong data type","");


	apInitScenario("2.  var int : Int16;");
	try {
		var int : Int16;
		int = 3;
	}
	catch (e) {
		if (e.number != 0) {
	        apLogFailInfo( "*** Scenario failed: 2.1 Error returned", e.description, "error", "");
		}
	}
	verify (int, 3, "2.2 Wrong value","");
	verify (int.GetType(), "System.Int16", "2.3 Wrong data type","");


	apInitScenario("3.  var i16 : Int16 = 3;");
	try {
		var i3 : Int16 = 3;
	}
	catch (e) {
		if (e.description != "An exception of type System.InvalidCastException was thrown.") {
	        apLogFailInfo( "*** Scenario failed: 3.1 Error returned", e.description, "error", "");
		}
	}
	verify (i3, 3, "3.2 Wrong value","");
	verify (i3.GetType(), "System.Int16", "3.3 Wrong data type","");


	apInitScenario("4.  var i16 : Int16 = Date.parse ('1/1/1970');");
	try {
		var i4 : Int16 = (new Date(65500)).valueOf();
	}
	catch (e) {
		if (e.description != "Type mismatch") {
	        apLogFailInfo( "*** Scenario failed: 4.1 Error returned", e.description, "error", "");
		}
	}
	verify (i4, 0, "4.2 Wrong value","");
//	verify (i4.GetType(), "System.Int16", "4.3 Wrong data type","");


	apInitScenario("5.  var i16 : Int16 = 'hello';");
	try {
		var i5 : Int16 = eval("new String('hello')");
	}
	catch (e) {
		if (e.description != "Type mismatch") {
	        apLogFailInfo( "*** Scenario failed: 5.1 Error returned", e.description, "error", "");
		}
	}
	verify (i5, 0, "5.2 Wrong value","");
//	verify (i5.GetType(), "System.Int16", "5.3 Wrong data type","");


	apInitScenario("6.  var i16 : Int16 = true;");
	try {
		var i6 : Int16 = eval("true");
	}
	catch (e) {
		if (e.description != "An exception of type System.InvalidCastException was thrown.") {
	        apLogFailInfo( "*** Scenario failed: 6.1 Error returned", e.description, "error", "");
		}
	}
	verify (i6, 1, "6.2 Wrong value","");
	verify (i6.GetType(), "System.Int16", "6.3 Wrong data type","");


	apInitScenario("7.  var i16 : Int16 = ReturnInt16Val();");
	try {
		var i7 : Int16 = ReturnInt16Val();
	}
	catch (e) {
		if (e.description != "An exception of type System.InvalidCastException was thrown.") {
	        apLogFailInfo( "*** Scenario failed: 7.1 Error returned", e.description, "error", "");
		}
	}
	verify (i7, 42, "7.2 Wrong value","");
	verify (i7.GetType(), "System.Int16", "7.3 Wrong data type","");


	apEndTest();
}


typann18();


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
