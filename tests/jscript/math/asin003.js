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


var iTestID = 53729;


function verify(sCat, ob, vExp, bugNum)
{
@if(@_fast)
    var nEpsilon;
@end

    if (null == bugNum) bugNum = "";

    var obj = new Object();
    obj.mem = ob;

    var vAct = Math.asin(obj.mem);

    var fFailed = false;
    if (isNaN(vExp))
    {
        if (!isNaN(vAct))
            fFailed = true;
    }
    else
        if ((Math.abs(vExp-vAct)/vExp) > (nEpsilon=2.2204460492503131e-014))
            fFailed = true;

    if (fFailed)
        apLogFailInfo( m_scen+(sCat.length?"--"+sCat:"")+" failed",vExp,vAct,bugNum);
}

var m_scen = "";

function asin003()
{
 
    apInitTest("asin003 ");

    //----------------------------------------------------------------------------
    apInitScenario("1. number, decimal, integer");

    m_scen = "number, decimal, integer";

    verify("pos min", 1, 1.5707963267948966, null);
    verify("pos min < n < VT_UI1 pos max", 127, Number.NaN, null);
    verify("VT_UI1 pos max", 255, Number.NaN, null);
    verify("VT_I2 pos excl-min", 256, Number.NaN, null);
    verify("VT_I2 pos excl-min < n < VT_I2 pos max", 12869, Number.NaN, null);
    verify("VT_I2 pos max", 32767, Number.NaN, null);
    verify("VT_I4 pos excl-min", 32768, Number.NaN, null);
    verify("VT_I4 pos excl-min < n < pos max", 1143483646, Number.NaN, null);
    verify("pos max", 2147483647, Number.NaN, null);
    
    verify("neg max", -1, -1.5707963267948966, null);
    verify("neg max > n > VT_UI1 neg min", -201, Number.NaN, null);
    verify("VT_UI1 neg min", -256, Number.NaN, null);
    verify("VT_I2 neg excl-max", -257, Number.NaN, null);
    verify("VT_I2 neg excl-max > n > VT_I2 neg min", -12869, Number.NaN, null);
    verify("VT_I2 neg min", -32768, Number.NaN, null);
    verify("VT_I4 neg excl-max", -32769, Number.NaN, null);
    verify("VT_I4 neg excl-max > n > neg min", -1143483646, Number.NaN, null);
    verify("neg min", -2147483648, Number.NaN, null);
    
    verify("zero", 0, 0, null); 

    
    //----------------------------------------------------------------------------
    apInitScenario("2. number, hexidecimal");

    m_scen = "number, hexidecimal";

    verify("pos min < n < pos hex excl-min", 0xabcdef, Number.NaN, null);
    verify("pos hex excl-min: 0x80000000", 0x80000000, Number.NaN, null);
    verify("max pos: 0xffffffff", 0xffffffff, Number.NaN, null);

    verify("neg max > n > neg hex excl-max", -0xabcdef, Number.NaN, null);
    verify("pos hex excl-min: -0x80000001", -0x80000001, Number.NaN, null);
    verify("min neg: -0xffffffff", -0xffffffff, Number.NaN, null);

    verify("zero", 0x0, 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("3. number, octal");

    m_scen = "number, octal";

    verify("pos min < n < pos hex excl-min", 01234567, Number.NaN, null);
    verify("pos hex excl-min: 020000000000", 020000000000, Number.NaN, null);
    verify("max pos: 037777777777", 037777777777, Number.NaN, null);

    verify("neg max > n > neg hex excl-max", -076543210, Number.NaN, null);
    verify("pos hex excl-min: -020000000001", -020000000001, Number.NaN, null);
    verify("min neg: -037777777777", -037777777777, Number.NaN, null);

    verify("pos zero", 00, 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("4. number, float");

    m_scen = "number, float";

    verify("pos min", 2.2250738585072014e-308, 2.2250738585072014e-308, null);
    verify("pos min < n < VT_R4 pos max", 9876543210000, Number.NaN, null);
    verify("VT_R4 pos max", 3.402823466e38, Number.NaN, null);
    verify("VT_R8 pos excl-min", 3.402823467e38, Number.NaN, null);
    verify("VT_R8 pos excl-min < n < pos max", 4.43269743388067e107, Number.NaN, null);
    verify("pos max", 1.7976931348623157e308, Number.NaN, null);

    verify("neg max", -2.2250738585072012595e-308, -2.2250738585072e-308, null);
    verify("neg max > n > VT_R4 neg min", -66100533593023300000, Number.NaN, null);
    verify("VT_R4 neg min", -3.402823466e38, Number.NaN, null);
    verify("VT_R8 neg excl-max", -3.402823467e38, Number.NaN, null);
    verify("VT_R8 neg excl-max > n > neg min", -7.17170763763262e243, Number.NaN, null);
    verify("neg min", -1.7976931348623157e308, Number.NaN, null);

    verify("zero", 0, 0, null);

    
    //----------------------------------------------------------------------------
    apInitScenario("5. num string, decimal, integer");

    m_scen = "number, decimal, integer";

    verify("pos min", "1", 1.5707963267948966, null);
    verify("pos min < n < VT_UI1 pos max", "127", Number.NaN, null);
    verify("VT_UI1 pos max", "255", Number.NaN, null);
    verify("VT_I2 pos excl-min", "256", Number.NaN, null);
    verify("VT_I2 pos excl-min < n < VT_I2 pos max", "12869", Number.NaN, null);
    verify("VT_I2 pos max", "32767", Number.NaN, null);
    verify("VT_I4 pos excl-min", "32768", Number.NaN, null);
    verify("VT_I4 pos excl-min < n < pos max", "1143483646", Number.NaN, null);
    verify("pos max", "2147483647", Number.NaN, null);

    verify("neg max", "-1", -1.5707963267948966, null);
    verify("neg max > n > VT_UI1 neg min", "-201", Number.NaN, null);
    verify("VT_UI1 neg min", "-256", Number.NaN, null);
    verify("VT_I2 neg excl-max", "-257", Number.NaN, null);
    verify("VT_I2 neg excl-max > n > VT_I2 neg min", "-12869", Number.NaN, null);
    verify("VT_I2 neg min", "-32768", Number.NaN, null);
    verify("VT_I4 neg excl-max", "-32769", Number.NaN, null);
    verify("VT_I4 neg excl-max > n > neg min", "-1143483646", Number.NaN, null);
    verify("neg min", "-2147483648", Number.NaN, null);

    verify("zero", "0", 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("7. num string, octal");

    m_scen = "num string, octal";

    verify("pos min < n < pos hex excl-min", "01234567", Number.NaN, null);
    verify("pos hex excl-min: 020000000000", "020000000000", Number.NaN, null);
    verify("max pos: 037777777777", "037777777777", Number.NaN, null);

    verify("neg max > n > neg hex excl-max", "-076543210", Number.NaN, null);
    verify("pos hex excl-min: -020000000001", "-020000000001", Number.NaN, null);
    verify("min neg: -037777777777", "-037777777777", Number.NaN, null);

    verify("pos zero", "00", 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("8. num string, float");

    m_scen = "number, float";

    verify("pos min", "2.2250738585072014e-308", 2.2250738585072014e-308, null);
    verify("pos min < n < VT_R4 pos max", "9.87654321e12", Number.NaN, null);
    verify("VT_R4 pos max", "3.402823466e+38", Number.NaN, null);
    verify("VT_R8 pos excl-min", "3.402823467e+38", Number.NaN, null);
    verify("VT_R8 pos excl-min < n < pos max", "4.43269743388067e107", Number.NaN, null);
    verify("pos max", "1.7976931348623158e308", Number.NaN, null);

    verify("neg max", "-2.2250738585072012595e-308", -2.225073858507201e-308, null);
    verify("neg max > n > VT_R4 neg min", "-6.61005335930233e19", Number.NaN, null);
    verify("VT_R4 neg min", "-3.402823466e+38", Number.NaN, null);
    verify("VT_R8 neg excl-max", "-3.402823467e+38", Number.NaN, null);
    verify("VT_R8 neg excl-max > n > neg min", "-7.17170763763262e243", Number.NaN, null);
    verify("neg min", "-1.797693134862315807e308", Number.NaN, null);

    verify("zero", "0.0", 0, null);

   
   
    //----------------------------------------------------------------------------
    apInitScenario("13. constants");

    m_scen = "constants";

    verify("true",true, 1.5707963267948966, null);
    verify("false",false, 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("14. null");

    m_scen = "null";

    verify("",null, 0, null);


    apEndTest();

}


asin003();


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
