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


var iTestID = 71686;

// Global variables
//

var arr1;
var arr2;
var arr3;
var iLargestNumber = 0xFFFFFFFF; // (2^32)-1


function verify(sAct, sExp, sMes){
	if (sAct != sExp)
        apLogFailInfo( "*** Scenario failed: "+sMes+" ", sExp, sAct, "");
}

function arrcon01() {

    apInitTest("arrcon01: Array.prototype.concat method can take zero or more arguments"); 


	arr1 = new Array("One","Two","Three");


	apInitScenario("1 Concat with zero arguments");
	arr1.concat();
	verify (arr1.join(), "One,Two,Three", "1 Concat with no arguments modified the calling array");


	apInitScenario("2 Concat with one array argument");
	arr2 = new Array("Four","Five","Six");
	verify (arr1.concat(arr2),"One,Two,Three,Four,Five,Six", "2 Concat with one array argument");


	apInitScenario("3 Concat with multiple array arguments");
	arr3 = new Array("Seven", "Eight", "Nine");
	verify (arr1.concat(arr2,arr3), "One,Two,Three,Four,Five,Six,Seven,Eight,Nine", "3 Concat with multiple array arguments");

	
/* These two scenarios take a very long time to execute
   and are inappropriate for nightly test runs, so they
   have been moved to arrcon02.js

// IanChing: Purposely changed the spelling of apInitScenario so that MadDog won't 
//           count this in its Scenario List

	apInitScenari ("4 Concat 2^32 elements into arbitrary array");
	arr2 = new Array(iLargestNumber);
	try {
		arr3 = arr1.concat(arr2);
	}
	catch (e) {
		verify(e.number,-2146828281, "4 Wrong error returned");
	}


// IanChing: Purposely changed the spelling of apInitScenario so that MadDog won't 
//           count this in its Scenario List

	apInitScenari ("5 Concat into Array of 2^32 elements");
	arr2 = new Array(iLargestNumber);
	try {
		arr3 = arr1.concat(arr2);
	}
	catch (e) {
		verify(e.number,-2146828281,"5 Wrong error returned");
	}
*/

	apEndTest();
}





arrcon01();


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
