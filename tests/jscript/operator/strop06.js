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


var iTestID = 53684;

function FuncRetVal() { return "655.35"; }
function FuncRetNulVal() { return ""; }
function myObject() { this.retVal = myObjVal(); }
function myObjVal() { return 123321; }

function strop06() {
    var tmp,res,expected,emptyVar;

    var myDate = new Date();
    myDate.setTime(0); // this is Wed Dec 31 16:00:00 PST 1969

    var numvar = 4321;
    var strvar = "57";
    var strvarnul = "";
    var boolvar = new Boolean(true);

    var ary = new Array();
    ary[2] = 12.14;
    ary[3] = 13.14;
    ary[4] = 15.14;

    var myObj = new myObject();
    var myStr = new String("112");

    apInitTest("strop06");

    apInitScenario("string funcretval += expr");
    res = FuncRetVal(); res += "alpha" + "beta";
    expected = "655.35alphabeta";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval += funcretval");
    res = FuncRetVal(); res += FuncRetVal();
    expected = "655.35655.35";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval += literal");
    res = FuncRetVal(); res += "JavaScript";
    expected = "655.35JavaScript";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval += variable");
    res = FuncRetVal(); res += strvar;
    expected = "655.3557";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval += null expr");
    res = FuncRetVal(); res += "" + "";
    expected = "655.35";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval += null funcretval");
    res = FuncRetVal(); res += FuncRetNulVal();
    expected = "655.35";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval += null literal");
    res = FuncRetVal(); res += "";
    expected = "655.35";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval += null variable");
    res = FuncRetVal(); res += strvarnul;
    expected = "655.35";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval += boolean");
    res = FuncRetVal(); res += boolvar;
    expected = "655.35true";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval += numeric literal");
    res = FuncRetVal(); res += 55.2;
    expected = "655.3555.2";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval += numeric var");
    res = FuncRetVal(); res += numvar;
    expected = "655.354321";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval += object");
    res = FuncRetVal(); res += myObj;
    expected = "655.35[object Object]";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    if( new Date("12/25/2002 12:00:00 PST").getTimezoneOffset()/60 == 8) {
    apInitScenario("string funcretval += date");
    res = FuncRetVal(); res += myDate;
    expected = "655.35Wed Dec 31 16:00:00 PST 1969";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }
    }

    apInitScenario("string funcretval + expr");
    res = FuncRetVal() + "alpha" + "beta";
    expected = "655.35alphabeta";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval + funcretval");
    res = FuncRetVal() + FuncRetVal();
    expected = "655.35655.35";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval + literal");
    res = FuncRetVal() + "JavaScript";
    expected = "655.35JavaScript";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval + variable");
    res = FuncRetVal() + strvar;
    expected = "655.3557";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval + null expr");
    res = FuncRetVal() + "" + "";
    expected = "655.35";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval + null funcretval");
    res = FuncRetVal() + FuncRetNulVal();
    expected = "655.35";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval + null literal");
    res = FuncRetVal() + "";
    expected = "655.35";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval + null variable");
    res = FuncRetVal() + strvarnul;
    expected = "655.35";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval + boolean");
    res = FuncRetVal() + boolvar;
    expected = "655.35true";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval + numeric literal");
    res = FuncRetVal() + 55.2;
    expected = "655.3555.2";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval + numeric var");
    res = FuncRetVal() + numvar;
    expected = "655.354321";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    apInitScenario("string funcretval + object");
    res = FuncRetVal() + myObj;
    expected = "655.35[object Object]";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }

    if( new Date("12/25/2002 12:00:00 PST").getTimezoneOffset()/60 == 8) {
    apInitScenario("string funcretval + date");
    res = FuncRetVal() + myDate;
    expected = "655.35Wed Dec 31 16:00:00 PST 1969";
    if (res != expected) {
	apLogFailInfo("wrong result",expected,res,"");
    }
    if (typeof(res) != typeof(expected)) {
	apLogFailInfo("wrong result",typeof(expected),typeof(res),"");
    }
    }

    apEndTest();
}

strop06();


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
