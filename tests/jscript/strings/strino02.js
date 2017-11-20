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


var iTestID = 72436;

//////////
//
//  Test Case:
//
//     strino02: String.prototype.indexOf should return 0 when searching for '' in ''
//
//  Author:
//




//////////
//
//  Helper functions


function verify(sAct, sExp, sMes){
	if (sAct != sExp)
        apLogFailInfo( "*** Scenario failed: "+sMes+" ", sExp, sAct, "");
}


    // function x is defined here for scenario 18

var iCount;
function x(){ iCount = 3; }



//////////
//
// Global variables
//


var str1;
var iLargestNumber = 0xFFFFFFFF; // (2^32)-1



function strino02() {

    apInitTest("strino02: String.prototype.indexOf should return 0 when searching for '' in ''"); 


	str1 = "";


	apInitScenario("1 no pos argument");
	verify (str1.indexOf(""), 0, "1 Wrong index returned");	


	apInitScenario("2 pos == null");
	verify (str1.indexOf("", null), 0, "2 Wrong index returned");	


	apInitScenario("3 pos == undefined");
	verify (str1.indexOf("", undefined), 0, "3 Wrong index returned");	


	apInitScenario("4 pos == 0");
	verify (str1.indexOf("", 0), 0, "4 Wrong index returned");	


	apInitScenario("5 pos == 1");
	verify (str1.indexOf("", 1), 0, "5 Wrong index returned");	


	apInitScenario("6 pos == -1");
	verify (str1.indexOf("", -1), 0, "6 Wrong index returned - VS7");	


	apInitScenario("7 pos == String.length - 1");
	verify (str1.indexOf("", str1.length - 1), 0, "7 Wrong index returned");	


	apInitScenario("8 pos == String.length");
	verify (str1.indexOf("", str1.length), 0, "8 Wrong index returned");	


	apInitScenario("9 pos == String.length + 1");
	verify (str1.indexOf("", str1.length + 1), 0, "9 Wrong index returned");	


	apInitScenario("10 pos == large positive number");
	verify (str1.indexOf("", iLargestNumber), 0, "10 Wrong index returned");	


	apInitScenario("11 pos == large negative number");
	verify (str1.indexOf("", -iLargestNumber), 0, "11 Wrong index returned");	


	apInitScenario("12 pos is float value");
	verify (str1.indexOf("", 3.14159), 0, "12 Wrong index returned");	


	apInitScenario("13 pos is empty string");
	verify (str1.indexOf("", ""), 0, "13 Wrong index returned");	


	apInitScenario("14 pos is non-empty string of letters");
	verify (str1.indexOf("", "abc"), 0, "14 Wrong index returned");	


	apInitScenario("15 pos is non-empty string of numbers");
	verify (str1.indexOf("", "123"), 0, "15 Wrong index returned");	


	apInitScenario("16 pos is array");
	var arr1 = new Array(3);
	arr1[0] = 3;
	arr1[1] = 45;
	arr1[2] = -7;
	verify (str1.indexOf("", arr1), 0, "16 Wrong index returned");	


	apInitScenario("17 pos is object");
	var obj1 = new Object();
	obj1.mem = 3;
	verify (str1.indexOf("", obj1), 0, "17 Wrong index returned");	


	apInitScenario("18 pos is function");
	// function x is defined above	
	verify (str1.indexOf("", x), 0, "18 Wrong index returned");	


	apEndTest();
}

strino02();


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
