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


var iTestID = 53743;


function verify(sCat, ob, vExp, bugNum){
 @if(@_fast)
    var nEpsilon, vAct;
 @end

    if (null == bugNum) bugNum = "";

    @if (@_jscript_version<7) // testing cos of numbers > 2^63 is just testing roundoff error.  This is not supported by the math libraries in v7.
      if (ob < 2^63)
        vAct = Math.sin(ob);
      else {
        vAct = 0;
        vExp = 0;
      }
    @else
	vAct = 0;
	vExp = 0;	
    @end

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
        apLogFailInfo( m_scen+(sCat.length?"--"+sCat:"")+" failed",vExp,vAct,bugNum);
}

var m_scen = "";

function sin002()
{
 
    apInitTest("sin002 ");

    //----------------------------------------------------------------------------
    apInitScenario("1. number, decimal, integer");

    m_scen = "number, decimal, integer";

    verify("pos min", 1, .8414709848078965, null);
    verify("pos min < n < VT_UI1 pos max", 127, .972630067242408, null);
    verify("VT_UI1 pos max", 255, -.5063916349244909, null);
    verify("VT_I2 pos excl-min", 256, -.9992080341070627, null);
    verify("VT_I2 pos excl-min < n < VT_I2 pos max", 12869, .8606225417289958, null);
    verify("VT_I2 pos max", 32767, .18750655394138946, null);
    verify("VT_I4 pos excl-min", 32768, .9278563334139247, null);
    verify("VT_I4 pos excl-min < n < pos max", 1143483646, -.6160942972555102, null);
    verify("pos max", 2147483647, -.7249165551464608, null);
    
    verify("neg max", -1, -.8414709848078965, null);
    verify("neg max > n > VT_UI1 neg min", -201, 0.06189025071872073, null);
    verify("VT_UI1 neg min", -256, .9992080341070627, null);
    verify("VT_I2 neg excl-max", -257, .5733571748155426, null);
    verify("VT_I2 neg excl-max > n > VT_I2 neg min", -12869, -.8606225417289958, null);
    verify("VT_I2 neg min", -32768, -.9278563334139247, null);
    verify("VT_I4 neg excl-max", -32769, -.8151392789744125, null);
    verify("VT_I4 neg excl-max > n > neg min", -1143483646, .6160942972555102, null);
    verify("neg min", -2147483648, .9713101757922817, null);
    
    verify("zero", 0, 0, null); 

    
    //----------------------------------------------------------------------------
    apInitScenario("2. number, hexidecimal");

    m_scen = "number, hexidecimal";

    verify("pos min < n < pos hex excl-min", 0xabcdef, .9235794332291494, null);
    verify("pos hex excl-min: 0x80000000", 0x80000000, -.9713101757922817, null);
    verify("max pos: 0xffffffff", 0xffffffff, .496677191748489, null);

    verify("neg max > n > neg hex excl-max", -0xabcdef, -.9235794332291494, null);
    verify("pos hex excl-min: -0x80000001", -0x80000001, .32468570024105514, null);
    verify("min neg: -0xffffffff", -0xffffffff, -.496677191748489, null);

    verify("zero", 0x0, 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("3. number, octal");

    m_scen = "number, octal";

    verify("pos min < n < pos hex excl-min", 01234567, .9824284614144432, null);
    verify("pos hex excl-min: 020000000000", 020000000000, -.9713101757922817, null);
    verify("max pos: 037777777777", 037777777777, .496677191748489, null);

    verify("neg max > n > neg hex excl-max", -076543210, -.1350184429998511, null);
    verify("pos hex excl-min: -020000000001", -020000000001, .32468570024105514, null);
    verify("min neg: -037777777777", -037777777777, -.496677191748489, null);

    verify("pos zero", 00, 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("4. number, float");

    m_scen = "number, float";

    verify("pos min", 2.2250738585072014e-308, 2.2250738585072014e-308, null);
    verify("pos min < n < VT_R4 pos max", 9876543210000, .9414848260366205, null);
    verify("VT_R4 pos max", 3.402823466e38, -0.7218324597981629, null);
    verify("VT_R8 pos excl-min", 3.402823467e38, 0.48416418254125315, null);
    verify("VT_R8 pos excl-min < n < pos max", 4.43269743388067e107, 0.9043111684792379, null);
    verify("pos max", 1.7976931348623157e308, -0.42374590839858816, null);

    verify("neg max", -2.2250738585072012595e-308, -2.2250738585072e-308, null);
    verify("neg max > n > VT_R4 neg min", -66100533593023300000, 0.9267368736533742, null);
    verify("VT_R4 neg min", -3.402823466e38, 0.7218324597981629, null);
    verify("VT_R8 neg excl-max", -3.402823467e38, -0.48416418254125315, null);
    verify("VT_R8 neg excl-max > n > neg min", -7.17170763763262e243, 0.22844798271611288, null);
    verify("neg min", -1.7976931348623157e308, 0.42374590839858816, null);

    verify("zero", 0, 0, null);

    
    //----------------------------------------------------------------------------
    apInitScenario("5. num string, decimal, integer");

    m_scen = "number, decimal, integer";

    verify("pos min", "1", .8414709848078965, null);
    verify("pos min < n < VT_UI1 pos max", "127", .972630067242408, null);
    verify("VT_UI1 pos max", "255", -.5063916349244909, null);
    verify("VT_I2 pos excl-min", "256", -.9992080341070627, null);
    verify("VT_I2 pos excl-min < n < VT_I2 pos max", "12869", .8606225417289958, null);
    verify("VT_I2 pos max", "32767", .18750655394138946, null);
    verify("VT_I4 pos excl-min", "32768", .9278563334139247, null);
    verify("VT_I4 pos excl-min < n < pos max", "1143483646", -.6160942972555102, null);
    verify("pos max", "2147483647", -.7249165551464608, null);

    verify("neg max", "-1", -.8414709848078965, null);
    verify("neg max > n > VT_UI1 neg min", "-201", 0.06189025071872073, null);
    verify("VT_UI1 neg min", "-256", .9992080341070627, null);
    verify("VT_I2 neg excl-max", "-257", .5733571748155426, null);
    verify("VT_I2 neg excl-max > n > VT_I2 neg min", "-12869", -.8606225417289958, null);
    verify("VT_I2 neg min", "-32768", -.9278563334139247, null);
    verify("VT_I4 neg excl-max", "-32769", -.8151392789744125, null);
    verify("VT_I4 neg excl-max > n > neg min", "-1143483646", .6160942972555102, null);
    verify("neg min", "-2147483648", .9713101757922817, null);

    verify("zero", "0", 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("7. num string, octal");

    m_scen = "num string, octal";

    verify("pos min < n < pos hex excl-min", "01234567", .3644521747875548, null);
    verify("pos hex excl-min: 020000000000", "020000000000", -.8513021533406929, null);
    verify("max pos: 037777777777", "037777777777", .996512085152881, null);

    verify("neg max > n > neg hex excl-max", "-076543210", -.9933397411918338, null);
    verify("pos hex excl-min: -020000000001", "-020000000001", 0.01846109551757109, null);
    verify("min neg: -037777777777", "-037777777777", -.996512085152881, null);

    verify("pos zero", "00", 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("8. num string, float");

    m_scen = "number, float";

    verify("pos min", "2.2250738585072014e-308", 2.2250738585072014e-308, null);
    verify("pos min < n < VT_R4 pos max", "9.87654321e12", .9414848260366205, null);
    verify("VT_R4 pos max", "3.402823466e+38", -0.7218324597981629, null);
    verify("VT_R8 pos excl-min", "3.402823467e+38", 0.48416418254125315, null);
    verify("VT_R8 pos excl-min < n < pos max", "4.43269743388067e107", 0.9043111684792379, null);
    verify("pos max", "1.7976931348623158e308", -0.42374590839858816, null);

    verify("neg max", "-2.2250738585072012595e-308", -2.225073858507201e-308, null);
    verify("neg max > n > VT_R4 neg min", "-6.61005335930233e19", 0.9267368736533742, null);
    verify("VT_R4 neg min", "-3.402823466e+38", 0.7218324597981629, null);
    verify("VT_R8 neg excl-max", "-3.402823467e+38", -0.48416418254125315, null);
    verify("VT_R8 neg excl-max > n > neg min", "-7.17170763763262e243", 0.22844798271611288, null);
    verify("neg min", "-1.797693134862315807e308", 0.42374590839858816, null);

    verify("zero", "0.0", 0, null);

 
    //----------------------------------------------------------------------------
    apInitScenario("13. constants");

    m_scen = "constants";

    verify("true",true, .8414709848078965, null);
    verify("false",false, 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("14. null");

    m_scen = "null";

    verify("",null, 0, null);


    apEndTest();

}


sin002();


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
