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


var iTestID = 51809;



@if(!@aspx)
	function obFoo() {};
@else
	expando function obFoo() {};
@end

function verifyEx (scen, ob, nob, expect) {
@if (@_fast)  
    var obj;             
@end     

    var n = 0;

    (obj = new Object()).mem = ob;
    if ((obj.mem ? ++n : --n) != expect) 
        apLogFailInfo (scen+"--"+nob+" failed ", expect, n,"");
}


function cond010() 
{
    apInitTest("cond010 ");


    //----------------------------------------------------------------------------
    apInitScenario("1. built-in, non-exec");

    verifyEx("built-in, non-exec",Math,"Math",1);


    //----------------------------------------------------------------------------
    apInitScenario("2. built-in, exec, not instanciated");

    verifyEx("built-in, exec, not instanciated",Array,"Array",1);
    verifyEx("built-in, exec, not instanciated",Boolean,"Boolean",1);
    verifyEx("built-in, exec, not instanciated",Date,"Date",1);
    verifyEx("built-in, exec, not instanciated",Number,"Number",1);
    verifyEx("built-in, exec, not instanciated",Object,"Object",1);


    //----------------------------------------------------------------------------
    apInitScenario("3. built-in, exec, instanciated");

    verifyEx("built-in, exec, instanciated",new Array(),"new Array()",1);
    verifyEx("built-in, exec, instanciated",new Boolean(),"new Boolean()",1);
    verifyEx("built-in, exec, instanciated",new Date(),"new Date()",1);
    verifyEx("built-in, exec, instanciated",new Number(),"new Number()",1);
    verifyEx("built-in, exec, instanciated",new Object(),"new Object()",1);


    //----------------------------------------------------------------------------
    apInitScenario("4. user-defined, not instanciated");

    verifyEx("user-defined, not instanciated",obFoo,"obFoo",1);


    //----------------------------------------------------------------------------
    apInitScenario("5. user-defined, instanciated");

    verifyEx("user-defined, instanciated",new obFoo(),"new obFoo()",1);


    //----------------------------------------------------------------------------
    apInitScenario("6. number, decimal, integer");

    verifyEx("literal, number, integer",1,"min pos",1);
    verifyEx("literal, number, integer",1234567890,"min pos < n < max pos",1);
    verifyEx("literal, number, integer",2147483647,"max pos",1);

    verifyEx("literal, number, integer",-1,"max neg",1);
    verifyEx("literal, number, integer",-1234567890,"min neg < n < max neg",1);
    verifyEx("literal, number, integer",-2147483647,"min neg",1);

    verifyEx("literal, number, integer",0,"pos zero",-1);
    verifyEx("literal, number, integer",-0,"neg zero",-1);
   

    //----------------------------------------------------------------------------
    apInitScenario("7. number, decimal, float");

    verifyEx("literal, number, float",2.225073858507202e-308,"min pos",1);
    verifyEx("literal, number, float",1.2345678e90,"min pos < n < max pos",1);
    verifyEx("literal, number, float",1.797693134862315807e308,"max pos",1);
    verifyEx("literal, number, float",1.797693134862315807e309,"> max pos float (1.#INF)",1);

    verifyEx("literal, number, float",-2.225073858507202e-308,"max neg",1);
    verifyEx("literal, number, float",-1.2345678e90,"min neg < n < max neg",1);
    verifyEx("literal, number, float",-1.797693134862315807e308,"min neg",1);
    verifyEx("literal, number, float",-1.797693134862315807e309,"< min neg float (-1.#INF)",1);

    verifyEx("literal, number, float",0.0,"pos zero",-1);
    verifyEx("literal, number, float",-0.0,"neg zero",-1);

        
    //----------------------------------------------------------------------------
    apInitScenario("8. number, hexidecimal");

    verifyEx("literal, number, integer",0x1,"min pos",1);
    verifyEx("literal, number, integer",0x2468ACE,"min pos < n < max pos",1);
    verifyEx("literal, number, integer",0xFFFFFFFF,"max pos",1);

    verifyEx("literal, number, integer",-0x1,"max neg",1);
    verifyEx("literal, number, integer",-0x2468ACE,"min neg < n < max neg",1);
    verifyEx("literal, number, integer",-0xFFFFFFFF,"min neg",1);

    verifyEx("literal, number, integer",0x0,"pos zero",-1);
    verifyEx("literal, number, integer",-0x0,"neg zero",-1);


    //----------------------------------------------------------------------------
    apInitScenario("9. number, octal");

    verifyEx("literal, number, integer",01,"min pos",1);
    verifyEx("literal, number, integer",01234567,"min pos < n < max pos",1);
    verifyEx("literal, number, integer",037777777777,"max pos",1);

    verifyEx("literal, number, integer",-01,"max neg",1);
    verifyEx("literal, number, integer",-01234567,"min neg < n < max neg",1);
    verifyEx("literal, number, integer",-037777777777,"min neg",1);

    verifyEx("literal, number, integer",00,"pos zero",-1);
    verifyEx("literal, number, integer",-00,"neg zero",-1);

    
    //----------------------------------------------------------------------------
    apInitScenario("10. string");

    verifyEx("literal, string"," ","single space",1);
    verifyEx("literal, string","                                                                   ","multiple spaces",1);
    verifyEx("literal, string","false","as false",1);
	verifyEx("literal, string","0","ns 0",1);
    verifyEx("literal, string","1234567890","ns > 0",1);
    verifyEx("literal, string"," 1234567890","ns > 0, leading space",1);
    verifyEx("literal, string","1234567890 ","ns > 0, trailing space",1);
    verifyEx("literal, string","-1234567890","ns < 0",1);
    verifyEx("literal, string","foo","as single word",1);
    verifyEx("literal, string"," foo","as single word, leading space",1);
    verifyEx("literal, string","foo ","as single word, trailing space",1);
    verifyEx("literal, string","foo bar","as multiple word",1);
    verifyEx("literal, string"," foo bar","as multiple word, leading space",1);
    verifyEx("literal, string","foo bar ","as multiple word, trailing space",1);
    verifyEx("literal, string","","zls",-1);

              
    //----------------------------------------------------------------------------
    apInitScenario("11. constants");

    verifyEx("constants",true, "true", 1);
    verifyEx("constants",false,"false",-1);


    //----------------------------------------------------------------------------
    apInitScenario("12. null");

    verifyEx("object references",null,"null",-1);


    //----------------------------------------------------------------------------
    apInitScenario("13. undefined");
    
    var obUndef;
    verifyEx("undefined", obUndef,"", -1);


    apEndTest();

}


cond010();


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
