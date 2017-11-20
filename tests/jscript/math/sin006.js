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


var iTestID = 53759;

var inf = 1 / 0;
var nan = 0 / 0;

function p(a)
{
	var res;

	apInitScenario(a);
	with (Math)
	{
		if(!(res = eval(a)))
			apLogFailInfo( "wrong return value","true",res,"");
	}
}

function verify(sCat, vAct, vExp, bugNum){
 @if(@_fast)
    var nEpsilon;
 @end

    if (null == bugNum) bugNum = "";

    @if ((@_win32 || @_win64) && !@_alpha)
    nEpsilon=2.2204460492503131e-014;
    @else
    nEpsilon=1.22e-03;
    @end

    var fFailed = false;
    if (isNaN(vExp))
    {
        if (!isNaN(vAct))
            fFailed = true;
    }
    else
        if ((Math.abs(vExp-vAct)/vExp) > nEpsilon)
            fFailed = true;

    if (fFailed)
        apLogFailInfo((sCat.length?"--"+sCat:"")+" failed",vExp,vAct,bugNum);
}

function small(a)
{
	var fT = Math.abs(a) < 1e-15;
	if (fT)
		return 'true';
	return 'false (' + a + ')';
}

function equal(a, b)
{
	var fT = (a == b) && (1 / a == 1 / b);
	if (fT)
		return 'true';
	return 'false (' + a + ', ' + b + ', ' + (1/a) + ', ' + (1/b) + ')';
}

function sin006()
{
	apInitTest("sin006");

	p("isNaN(sin(nan))");
	p("isNaN(sin(+inf))");
	p("isNaN(sin(-inf))");
@if (@_win16)
	p("isNaN(sin(+1e300))");
	p("isNaN(sin(-1e300))");
@else
    @if (@_jscript_version<7) // testing cos of numbers > 2^63 is just testing roundoff error.  This is not supported by the math libraries in v7.
	verify( "sin(+1e300)", Math.sin(+1e300), 0.9804836454796092, null );
	verify( "sin(-1e300)", Math.sin(-1e300), -0.9804836454796092, null );
    @end
@end
	p("equal(sin(+PI / 2), +1)");
	p("equal(sin(-PI / 2), -1)");
	p("small(sin(+PI))");
	p("small(sin(-PI))");
	p("equal(sin(+0), +0)");
	p("equal(sin(-0), -0)");

	apEndTest();
}


sin006();


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
