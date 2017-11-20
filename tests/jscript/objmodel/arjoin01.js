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


var iTestID = 51730;


function verify(vAct, vExp, bugNum)
{
    if (null == bugNum) bugNum = "";

    if (vAct != vExp)
        apLogFailInfo( "", vExp, vAct, bugNum);
}

function arjoin01()
{
 
    apInitTest("arJoin01 ");

    //----------------------------------------------------------------------------
    apInitScenario("1. test method existence for Class");

    verify(typeof Array.prototype.join, "function", null);

    
    //----------------------------------------------------------------------------
    apInitScenario("2. test method existence for object");

    verify(typeof (new Array()).join, "function", null);

    
    //----------------------------------------------------------------------------
    apInitScenario("3. test BF: consecutive members zero-based, default delim");

    verify((new Array("J","S","c","r","i","p","t")).join(), "J,S,c,r,i,p,t", null);

    
    //----------------------------------------------------------------------------
    apInitScenario("4. test BF: consecutive members zero-based, explict delim");

    verify((new Array("J","S","c","r","i","p","t")).join("|"), "J|S|c|r|i|p|t", null);

    
    //----------------------------------------------------------------------------
    apInitScenario("5. test return type is string");

    verify(typeof (new Array(1,2,3,4)).join(), "string", null);


    apEndTest();

}


arjoin01();


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
