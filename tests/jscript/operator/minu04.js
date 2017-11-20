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


var iTestID = 176965;


//          this tescase tests the - compound assignment op using combinations
//          of hard typed variables

// Scenarios:

// 1) untyped - untyped
// 2) untyped - boolean
// 3) untyped - Number
// 4) untyped - String
// 5) untyped - Object
// 6) untyped - Date
// 7) untyped - Array
// 8) untyped - function
// 9) untyped - RegExp
// 10) untyped - Error
// 11) untyped - byte
// 12) untyped - char
// 13) untyped - short
// 14) untyped - int
// 15) untyped - long
// 16) untyped - float
// 17) boolean - untyped
// 18) boolean - boolean
// 19) boolean - Number
// 20) boolean - String
// 21) boolean - Object
// 22) boolean - Date
// 23) boolean - Array
// 24) boolean - function
// 25) boolean - RegExp
// 26) boolean - Error
// 27) boolean - byte
// 28) boolean - char
// 29) boolean - short
// 30) boolean - int
// 31) boolean - long
// 32) boolean - float
// 33) Number - untyped
// 34) Number - boolean
// 35) Number - Number
// 36) Number - String
// 37) Number - Object
// 38) Number - Date
// 39) Number - Array
// 40) Number - function
// 41) Number - RegExp
// 42) Number - Error
// 43) Number - byte
// 44) Number - char
// 45) Number - short
// 46) Number - int
// 47) Number - long
// 48) Number - float
// 49) String - untyped
// 50) String - boolean
// 51) String - Number
// 52) String - String
// 53) String - Object
// 54) String - Date
// 55) String - Array
// 56) String - function
// 57) String - RegExp
// 58) String - Error
// 59) String - byte
// 60) String - char
// 61) String - short
// 62) String - int
// 63) String - long
// 64) String - float
// 65) object - untyped
// 66) object - boolean
// 67) object - Number
// 68) object - String
// 69) object - Object
// 70) object - Date
// 71) object - Array
// 72) object - function
// 73) object - RegExp
// 74) object - Error
// 75) object - byte
// 76) object - char
// 77) object - short
// 78) object - int
// 79) object - long
// 80) object - float
// 81) Date - untyped
// 82) Date - boolean
// 83) Date - Number
// 84) Date - String
// 85) Date - Object
// 86) Date - Date
// 87) Date - Array
// 88) Date - function
// 89) Date - RegExp
// 90) Date - Error
// 91) Date - byte
// 92) Date - char
// 93) Date - short
// 94) Date - int
// 95) Date - long
// 96) Date - float
// 97) Array - untyped
// 98) Array - boolean
// 99) Array - Number
// 100) Array - String
// 101) Array - Object
// 102) Array - Date
// 103) Array - Array
// 104) Array - function
// 105) Array - RegExp
// 106) Array - Error
// 107) Array - byte
// 108) Array - char
// 109) Array - short
// 110) Array - int
// 111) Array - long
// 112) Array - float
// 113) Function - untyped
// 114) Function - boolean
// 115) Function - Number
// 116) Function - String
// 117) Function - Object
// 118) Function - Date
// 119) Function - Array
// 120) Function - function
// 121) Function - RegExp
// 122) Function - Error
// 123) Function - byte
// 124) Function - char
// 125) Function - short
// 126) Function - int
// 127) Function - long
// 128) Function - float
// 129) RegExp - untyped
// 130) RegExp - boolean
// 131) RegExp - Number
// 132) RegExp - String
// 133) RegExp - Object
// 134) RegExp - Date
// 135) RegExp - Array
// 136) RegExp - function
// 137) RegExp - RegExp
// 138) RegExp - Error
// 139) RegExp - byte
// 140) RegExp - char
// 141) RegExp - short
// 142) RegExp - int
// 143) RegExp - long
// 144) RegExp - float
// 145) Error - untyped
// 146) Error - boolean
// 147) Error - Number
// 148) Error - String
// 149) Error - Object
// 150) Error - Date
// 151) Error - Array
// 152) Error - function
// 153) Error - RegExp
// 154) Error - Error
// 155) Error - byte
// 156) Error - char
// 157) Error - short
// 158) Error - int
// 159) Error - long
// 160) Error - float
// 161) byte - untyped
// 162) byte - boolean
// 163) byte - Number
// 164) byte - String
// 165) byte - Object
// 166) byte - Date
// 167) byte - Array
// 168) byte - function
// 169) byte - RegExp
// 170) byte - Error
// 171) byte - byte
// 172) byte - char
// 173) byte - short
// 174) byte - int
// 175) byte - long
// 176) byte - float
// 177) Char - untyped
// 178) Char - boolean
// 179) Char - Number
// 180) Char - String
// 181) Char - Object
// 182) Char - Date
// 183) Char - Array
// 184) Char - function
// 185) Char - RegExp
// 186) Char - Error
// 187) Char - byte
// 188) Char - char
// 189) Char - short
// 190) Char - int
// 191) Char - long
// 192) Char - float
// 193) short - untyped
// 194) short - boolean
// 195) short - Number
// 196) short - String
// 197) short - Object
// 198) short - Date
// 199) short - Array
// 200) short - function
// 201) short - RegExp
// 202) short - Error
// 203) short - byte
// 204) short - char
// 205) short - short
// 206) short - int
// 207) short - long
// 208) short - float
// 209) int - untyped
// 210) int - boolean
// 211) int - Number
// 212) int - String
// 213) int - Object
// 214) int - Date
// 215) int - Array
// 216) int - function
// 217) int - RegExp
// 218) int - Error
// 219) int - byte
// 220) int - char
// 221) int - short
// 222) int - int
// 223) int - long
// 224) int - float
// 225) long - untyped
// 226) long - boolean
// 227) long - Number
// 228) long - String
// 229) long - Object
// 230) long - Date
// 231) long - Array
// 232) long - function
// 233) long - RegExp
// 234) long - Error
// 235) long - byte
// 236) long - char
// 237) long - short
// 238) long - int
// 239) long - long
// 240) long - float
// 241) float - untyped
// 242) float - boolean
// 243) float - Number
// 244) float - String
// 245) float - Object
// 246) float - Date
// 247) float - Array
// 248) float - function
// 249) float - RegExp
// 250) float - Error
// 251) float - byte
// 252) float - char
// 253) float - short
// 254) float - int
// 255) float - long
// 256) float - float


function FuncRetVal() { return 655.35; }
function myObjVal() { return 123321; }

@if(!@aspx)
	function myObject() { this.toString = myObjVal; }
@else
	expando function myObject() { this.toString = myObjVal; }
@end


function minu04() {
    var tmp,res,expected,emptyVar;

    var uvar = 4321;
    var boolvar:boolean = true;
    var numvar:Number = 3213.321;
    var strvar:String = "this is a string";
    var obvar:Object = new Object;
    var datevar:Date = new Date(0);
    var arrvar:Array = new Array;
    arrvar[0] = 2.2;
    //arrvar[3] = 3.3;
    //arrvar[4] = 4.4;
    function funvar(){return 99;};
    var revar:RegExp = /\d{2}-\d{2}/g;
    var errvar:Error = new Error;
    var bytevar:byte = 50;
    var charvar:char = "g";
    var shrtvar:short = 4321;
    var intvar:int = 1234;
    var longvar:long = 321321321321;
    var flovar:float = 1.25;
    var strTM:String = -2146828275;
    var strNE:String = "no errors...";

    apInitTest("minu04");



///////////////////////////////////////////////////////////////////
/////////////////  RegExp   ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("RegExp - untyped");
    uvar = 4321;
    res = revar - uvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - boolean");
    boolvar = true;
    res = revar - boolvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - Number");
    numvar = 4321;
    res = revar - numvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - String");
    strvar = "4321";
    res = revar - strvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - Object");
    obvar = 4321;
    res = revar - obvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - Date");
    datevar = new Date(0);
    res = revar - datevar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("RegExp - Array");
    res = revar - arrvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - function");
    // revar initialized above
    res = revar - funvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - RegExp");
    revar = /\d{2}-\d{2}/g
    res = revar - revar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - Error");
    errvar = new Error();
    res = revar - errvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - byte");
    bytevar = 21;
    res = revar - bytevar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - char");
    charvar = "A";
    res = revar - charvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - short");
    shrtvar = -32000;
    res = revar - shrtvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - int");
    intvar = 4321;
    res = revar - intvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - long");
    longvar = 12344321;
    res = revar - longvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp - float");
    flovar = 4321;
    res = revar - flovar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  Error    ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("Error - untyped");
    errvar = new Error();
    uvar = 4321;
    res = errvar - uvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - boolean");
    errvar = new Error();
    boolvar = true;
    res = errvar - boolvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - Number");
    errvar = new Error();
    numvar = 4321;
    res = errvar - numvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - String");
    errvar = new Error();
    strvar = "4321";
    res = errvar - strvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - Object");
    errvar = new Error();
    obvar = 4321;
    res = errvar - obvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - Date");
    errvar = new Error();
    datevar = new Date(0);
    res = errvar - datevar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("Error - Array");
    errvar = new Error();
    // arrvar initialized above
    res = errvar - arrvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - function");
    errvar = new Error();
    // funvar initialized above
    res = errvar - funvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - RegExp");
    errvar = new Error();
    revar = /\d{2}-\d{2}/g
    res = errvar - revar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - Error");
    errvar = new Error();
    res = errvar - errvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - byte");
    errvar = new Error();
    bytevar = 21;
    res = errvar - bytevar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - char");
    errvar = new Error();
    charvar = "A";
    res = errvar - charvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - short");
    errvar = new Error();
    shrtvar = -32000;
    res = errvar - shrtvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - int");
    errvar = new Error();
    intvar = 4321;
    res = errvar - intvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - long");
    errvar = new Error();
    longvar = 12344321;
    res = errvar - longvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error - float");
    errvar = new Error();
    flovar = 4321;
    res = errvar - flovar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  Byte     ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("byte - untyped");
    bytevar = 123;
    uvar = 43;
    res = bytevar - uvar;
    expected = byte(80)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    bytevar = 123;
    uvar = 432;
    res = bytevar - uvar;
    expected = -309
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    bytevar = 123;
    uvar = -200;
    res = bytevar - uvar;
    expected = 323
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    apInitScenario("byte - boolean");
    bytevar = 123;
    boolvar = true;
    res = bytevar - boolvar;                        // boolean changes to 1
    expected = byte(122)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - Number");
    bytevar = 123;
    numvar = 43;
    res = bytevar - numvar;
    expected = byte(80)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    bytevar = 123;
    numvar = 43.1;
    res = bytevar - numvar;
    expected = 79.9
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - String");
    bytevar = 123;
    strvar = "43";
    res = bytevar - strvar;
    expected = 80;
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - Object");
    bytevar = 123;
    obvar = 43;
    res = bytevar - obvar;
    expected = byte(80)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - Date");
    bytevar = 123;
    datevar = new Date(-43);
    res = bytevar - datevar;
    expected = byte(166)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("byte - Array");
    bytevar = 123;
    // arrvar initialized above
    res = bytevar - arrvar;
    expected = 120.8
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - function");
    bytevar = 123;
    // funvar initialized above
    res = bytevar - funvar();
    expected = 24
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - RegExp");
    bytevar = 123;
    revar = /\d{2}-\d{2}/g
    res = bytevar - revar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - Error");
    bytevar = 123;
    errvar = new Error();
    res = bytevar - revar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - byte");
    bytevar = 123;
    res = bytevar - byte(21);
    expected = byte(102)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - char");
    bytevar = 123;
    charvar = "A";
    res = bytevar - charvar;
    expected = 58;
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - short");
    bytevar = 10;
    shrtvar = -20;
    res = bytevar - shrtvar;
    expected = byte(30)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    bytevar = 10;
    shrtvar = 400;
    res = bytevar - shrtvar;
    expected = -390
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - int");
    bytevar = 123;
    intvar = -100;
    res = bytevar - intvar;
    expected = 223;
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - long");
    bytevar = 123;
    longvar = 12;
    res = bytevar - longvar;
    expected = byte(111)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    bytevar = 123;
    longvar = 2147482414;
    res = bytevar - longvar;
    expected = -2147482291
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte - float");
    bytevar = 123;
    flovar = 8;
    res = bytevar - flovar;
    expected = byte(115)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    bytevar = 123;
    flovar = 8.1;
    res = bytevar - flovar;
    expected = 114.9
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  Char     ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("Char - untyped");
    charvar = "5";
    uvar = 6;
    res = charvar - uvar;
    expected = "/";
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    charvar = char("\u0001");
    uvar = -65;
    res = charvar - uvar;
    expected = "B";
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - boolean");
    charvar = "b";
    boolvar = true;
    res = charvar - boolvar;                    // boolean changes to 1
    expected = "a"
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - Number");
    charvar = "k";
    numvar = char("j");
    res = charvar - numvar;
    expected = char("\u0001");
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    charvar = "4";
    numvar = 1.1;
    res = strNE;
    expected = strTM;
    try{eval("res = charvar - numvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"268971");
    }


    apInitScenario("Char - String");
    charvar = "f";
    strvar = "5"
    res = charvar - strvar;
    expected = "a"
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - Object");
    charvar = "2";
    obvar = String("2");
    res = charvar - obvar;
    expected = char("0");
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    charvar = 0;
    obvar = -char(1);
    res = charvar - obvar;
    expected = char(1);
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - Array");
    charvar = "4";
    // arrvar initialized above
    arrvar[0] = 2.0;
    res = charvar - arrvar;
    expected = 2;
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"299784");
    }
    arrvar[0] = 2.2;

    apInitScenario("Char - function");
    charvar = char(164);
    // funvar initialized above
    res = charvar - funvar();
    expected = char("A");
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - RegExp");
    charvar = "s";
    revar = /\d{2}-\d{2}/g
    res = strNE;
    expected = strTM;
    try{eval("res = charvar - revar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - Error");
    charvar = "f";
    errvar = new Error();
    res = strNE;
    expected = strTM;
    try{eval("res = charvar - errvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - byte");
    charvar = "z";
    bytevar = 25;
    res = charvar - bytevar;
    expected = "a"
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - char");
    charvar = "B";
    res = charvar - char(1);
    expected = 65
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - short");
    charvar = char("\u0005");
    shrtvar = -65;
    res = charvar - shrtvar;
    expected = "F"
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - int");
    charvar = char("\u007b");
    intvar = 100;
    res = charvar - intvar;
    expected = char("\u0017");
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - long");
    charvar = "C";
    longvar = 2;
    res = charvar - longvar;
    expected = "A"
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    charvar = "A";
    longvar = -2;
    res = charvar - longvar;
    expected = "C"
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char - float");
    charvar = "b";
    flovar = -1;
    res = charvar - flovar;
    expected = char("c")
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    charvar = "t";
    flovar = 2.3;
    res = strNE;
    expected = strTM;
    try{eval("res = charvar - flovar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"268971");
    }


///////////////////////////////////////////////////////////////////
/////////////////  short    ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("short - untyped");
    shrtvar = 1234;
    uvar = 4321;
    res = shrtvar - uvar;
    expected = short(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - boolean");
    shrtvar = 1234;
    boolvar = true;
    res = shrtvar - boolvar;                        // boolean changes to 1
    expected = short(1233)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - Number");
    shrtvar = 1234;
    numvar = 4321;
    res = shrtvar - numvar;
    expected = short(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    shrtvar = 1234;
    numvar = 4321.1;
    res = shrtvar - numvar;
    expected = -3087.1
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - String");
    shrtvar = 1234;
    strvar = "4321";
    res = shrtvar - strvar;
    expected = short(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - Object");
    shrtvar = 1234;
    obvar = 4321;
    res = shrtvar - obvar;
    expected = short(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - Date");
    shrtvar = 1234;
    datevar = new Date(4321);
    res = shrtvar - datevar;
    expected = short(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("short - Array");
    shrtvar = 1234;
    // arrvar initialized above
    res = shrtvar - arrvar;
    expected = 1231.8
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - function");
    shrtvar = 1234;
    // funvar initialized above
    res = shrtvar - funvar();
    expected = 1135
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - RegExp");
    shrtvar = 1234;
    revar = /\d{2}-\d{2}/g
    res = shrtvar - revar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - Error");
    shrtvar = 1234;
    errvar = new Error();
    res = shrtvar - errvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - byte");
    shrtvar = -1200;
    bytevar = 34;
    res = shrtvar - bytevar;
    expected = short(-1234)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - char");
    shrtvar = 1234;
    charvar = "A";
    res = shrtvar - charvar;
    expected = short(1169)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - short");
    shrtvar = -10000;
    res = shrtvar - short(-9999);
    expected = short(-1)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - int");
    shrtvar = 2468;
    intvar = 34768
    res = shrtvar - intvar;
    expected = short(-32300);
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - long");
    shrtvar = 1234;
    longvar = -1234;
    res = shrtvar - longvar;
    expected = short(2468)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    shrtvar = 1234;
    longvar = 2147482414;
    res = shrtvar - longvar;
    expected = -2147481180
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short - float");
    shrtvar = 1234;
    flovar = 4321;
    res = shrtvar - flovar;
    expected = int(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    shrtvar = 1234;
    flovar = 4321.1;
    res = shrtvar - flovar;
    expected = -3087.1
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  int      ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("int - untyped");
    intvar = 1234;
    uvar = 4321;
    res = intvar - uvar;
    expected = int(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - boolean");
    intvar = 1234;
    boolvar = true;
    res = intvar - boolvar;                        // boolean changes to 1
    expected = int(1233)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - Number");
    intvar = 1234;
    numvar = 4321;
    res = intvar - numvar;
    expected = int(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    intvar = 1234;
    numvar = 4321.1;
    res = intvar - numvar;
    expected = -3087.1
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - String");
    intvar = 1234;
    strvar = "4321";
    res = intvar - strvar;
    expected = int(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - Object");
    intvar = 1234;
    obvar = 4321;
    res = intvar - obvar;
    expected = int(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - Date");
    intvar = 1234;
    datevar = new Date(4321);
    res = intvar - datevar;
    expected = int(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("int - Array");
    intvar = 1234;
    // arrvar initialized above
    res = intvar - arrvar;
    expected = 1231.8
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - function");
    intvar = 1234;
    // funvar initialized above
    res = intvar - funvar();
    expected = 1135
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - RegExp");
    intvar = 1234;
    revar = /\d{2}-\d{2}/g
    res = intvar - revar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - Error");
    intvar = 1234;
    errvar = new Error();
    res = intvar - errvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - byte");
    intvar = 1234;
    bytevar = 21;
    res = intvar - bytevar;
    expected = int(1213)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - char");
    intvar = 1234567865;
    charvar = "A";
    res = intvar - charvar;
    expected = int(1234567800)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - short");
    intvar = -10000000;
    shrtvar = 32000;
    res = intvar - shrtvar;
    expected = int(-10032000)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - int");
    intvar = 1234;
    res = intvar - int(1234567890);
    expected = int(-1234566656);
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - long");
    intvar = 2000000000;
    longvar = 4000000000;
    res = intvar - longvar;
    expected = int(-2000000000)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    intvar = 1234;
    longvar = -2147482414;
    res = intvar - longvar;
    expected = 2147483648
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int - float");
    intvar = 1234;
    flovar = 4321;
    res = intvar - flovar;
    expected = int(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    intvar = 1234;
    flovar = 4321.1;
    res = intvar - flovar;
    expected = -3087.1
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  long     ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("long - untyped");
    longvar = 1234;
    uvar = 4321;
    res = longvar - uvar;
    expected = long(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - boolean");
    longvar = 1234;
    boolvar = true;
    res = longvar - boolvar;                        // boolean changes to 1
    expected = long(1233)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - Number");
    longvar = 1234;
    numvar = 4321;
    res = longvar - numvar;
    expected = long(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    longvar = 1234;
    numvar = 4321.1;
    res = longvar - numvar;
    expected = -3087.1
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - String");
    longvar = 1234;
    strvar = "4321";
    res = longvar - strvar;
    expected = long(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - Object");
    longvar = 1234;
    obvar = 4321;
    res = longvar - obvar;
    expected = long(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - Date");
    longvar = 1234;
    datevar = new Date(4321);
    res = longvar - datevar;
    expected = long(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("long - Array");
    longvar = 1234;
    // arrvar initialized above
    res = longvar - arrvar;
    expected = 1231.8
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - function");
    longvar = 1234;
    // funvar initialized above
    res = longvar - funvar();
    expected = 1135
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - RegExp");
    longvar = 1234;
    revar = /\d{2}-\d{2}/g
    res = longvar - revar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - Error");
    longvar = 1234;
    errvar = new Error();
    res = longvar - errvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - byte");
    longvar = 12345678900;
    bytevar = 21;
    res = longvar - bytevar;
    expected = long(12345678879)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - char");
    longvar = -12345678900;
    charvar = "A";
    res = longvar - charvar;
    expected = long(-12345678965)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - short");
    longvar = -10000000;
    shrtvar = 32000;
    res = longvar - shrtvar;
    expected = long(-10032000)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - int");
    longvar = -90000000000;
    intvar = 1234567890
    res = longvar - intvar;
    expected = long(-91234567890);
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - long");
    longvar = 1234;
    res = longvar - long(1235);
    expected = long(-1)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    //apWriteDebug("bug: vs269005");
    longvar = -9223372036854775808;
    res = longvar - long(1);
    expected = -9223372036854775809
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long - float");
    longvar = 1234;
    flovar = 4321;
    res = longvar - flovar;
    expected = long(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    longvar = 1234;
    flovar = 4321.1;
    res = longvar - flovar;
    expected = -3087.1
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  float    ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("float - untyped");
    flovar = 1234;
    uvar = 4321;
    res = flovar - uvar;
    expected = float(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    flovar = 1234.5;
    uvar = 4321;
    res = flovar - uvar;
    expected = float(-3086.5)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    flovar = 1234;
    uvar = 4321.5;
    res = flovar - uvar;
    expected = float(-3087.5)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    flovar = 1234.25;
    uvar = 4321.25;
    res = flovar - uvar;
    expected = float(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - boolean");
    flovar = 1234;
    boolvar = true;
    res = flovar - boolvar;                        // boolean changes to 1
    expected = float(1233)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - Number");
    flovar = 1234;
    numvar = 4321;
    res = flovar - numvar;
    expected = float(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    flovar = 1234;
    numvar = 4321.1
    res = flovar - numvar;
    expected = float(-3087.1)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - String");
    flovar = 1234;
    strvar = "4321";
    res = flovar - strvar;
    //apWriteDebug("bug: VS265983");
    expected = int(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - Object");
    flovar = 1234;
    obvar = 4321;
    res = flovar - obvar;
    expected = int(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - Date");
    flovar = 1234;
    datevar = new Date(2234);
    res = flovar - datevar;
    expected = int(-1000)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("float - Array");
    flovar = 1234;
    // arrvar initialized above
    res = flovar - arrvar;
    expected = 1231.8
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - function");
    flovar = 1234;
    // funvar initialized above
    res = flovar - funvar();
    expected = 1135
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - RegExp");
    flovar = 1234;
    revar = /\d{2}-\d{2}/g
    res = flovar - revar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - Error");
    flovar = 1234;
    errvar = new Error();
    res = flovar - errvar;
    expected = NaN
    if (!isNaN(res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - byte");
    flovar = -600.1;
    bytevar = 34;
    res = flovar - bytevar;
    expected = float(-634.1)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - char");
    flovar = -600.1;
    charvar = "A";
    res = flovar - charvar;
    expected = float(-665.1)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - short");
    flovar = -10000;
    shrtvar = -32000;
    res = flovar - shrtvar;
    expected = float(22000)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - int");
    flovar = 1234;
    intvar = -1234567890
    res = flovar - intvar;
    expected = float(1234569124);
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - long");
    flovar = 1234;
    longvar = 12344321;
    res = flovar - longvar;
    expected = Number(-12343087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    flovar = 1234;
    longvar = -2147482414;
    res = flovar - longvar;
    expected = Number(2147483648)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("float - float");
    flovar = 1234;
    res = flovar - float(4321);
    expected = float(-3087)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    flovar = 1234;
    res = flovar - float(4321.1);
    expected = float(-3087.1)
    if (doVerify(expected,res)) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    apEndTest();
}


function doVerify(a,b) {
  var delta = 1e-4;  
  if (a === b) { return 0; }

  if (typeof(a) == "number" && typeof(b) == "number"){
    if (a<b && (a+delta>b)) return 0;
    if (a>b && (a<b+delta)) return 0;
  }
  return 1;
}



minu04();


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
