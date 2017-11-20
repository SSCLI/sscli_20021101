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


var iTestID = 176960;


//          this tescase tests the += compound assignment op using combinations
//          of hard typed variables
// 
// Scenarios:
// 
// 1) untyped += untyped
// 2) untyped += boolean
// 3) untyped += Number
// 4) untyped += String
// 5) untyped += Object
// 6) untyped += Date
// 7) untyped += Array
// 8) untyped += function
// 9) untyped += RegExp
// 10) untyped += Error
// 11) untyped += byte
// 12) untyped += char
// 13) untyped += short
// 14) untyped += int
// 15) untyped += long
// 16) untyped += float
// 17) boolean += untyped
// 18) boolean += boolean
// 19) boolean += Number
// 20) boolean += String
// 21) boolean += Object
// 22) boolean += Date
// 23) boolean += Array
// 24) boolean += function
// 25) boolean += RegExp
// 26) boolean += Error
// 27) boolean += byte
// 28) boolean += char
// 29) boolean += short
// 30) boolean += int
// 31) boolean += long
// 32) boolean += float
// 33) Number += untyped
// 34) Number += boolean
// 35) Number += Number
// 36) Number += String
// 37) Number += Object
// 38) Number += Date
// 39) Number += Array
// 40) Number += function
// 41) Number += RegExp
// 42) Number += Error
// 43) Number += byte
// 44) Number += char
// 45) Number += short
// 46) Number += int
// 47) Number += long
// 48) Number += float
// 49) String += untyped
// 50) String += boolean
// 51) String += Number
// 52) String += String
// 53) String += Object
// 54) String += Date
// 55) String += Array
// 56) String += function
// 57) String += RegExp
// 58) String += Error
// 59) String += byte
// 60) String += char
// 61) String += short
// 62) String += int
// 63) String += long
// 64) String += float
// 65) object += untyped
// 66) object += boolean
// 67) object += Number
// 68) object += String
// 69) object += Object
// 70) object += Date
// 71) object += Array
// 72) object += function
// 73) object += RegExp
// 74) object += Error
// 75) object += byte
// 76) object += char
// 77) object += short
// 78) object += int
// 79) object += long
// 80) object += float
// 81) Date += untyped
// 82) Date += boolean
// 83) Date += Number
// 84) Date += String
// 85) Date += Object
// 86) Date += Date
// 87) Date += Array
// 88) Date += function
// 89) Date += RegExp
// 90) Date += Error
// 91) Date += byte
// 92) Date += char
// 93) Date += short
// 94) Date += int
// 95) Date += long
// 96) Date += float
// 97) Array += untyped
// 98) Array += boolean
// 99) Array += Number
// 100) Array += String
// 101) Array += Object
// 102) Array += Date
// 103) Array += Array
// 104) Array += function
// 105) Array += RegExp
// 106) Array += Error
// 107) Array += byte
// 108) Array += char
// 109) Array += short
// 110) Array += int
// 111) Array += long
// 112) Array += float
// 113) Function += untyped
// 114) Function += boolean
// 115) Function += Number
// 116) Function += String
// 117) Function += Object
// 118) Function += Date
// 119) Function += Array
// 120) Function += function
// 121) Function += RegExp
// 122) Function += Error
// 123) Function += byte
// 124) Function += char
// 125) Function += short
// 126) Function += int
// 127) Function += long
// 128) Function += float
// 129) RegExp += untyped
// 130) RegExp += boolean
// 131) RegExp += Number
// 132) RegExp += String
// 133) RegExp += Object
// 134) RegExp += Date
// 135) RegExp += Array
// 136) RegExp += function
// 137) RegExp += RegExp
// 138) RegExp += Error
// 139) RegExp += byte
// 140) RegExp += char
// 141) RegExp += short
// 142) RegExp += int
// 143) RegExp += long
// 144) RegExp += float
// 145) Error += untyped
// 146) Error += boolean
// 147) Error += Number
// 148) Error += String
// 149) Error += Object
// 150) Error += Date
// 151) Error += Array
// 152) Error += function
// 153) Error += RegExp
// 154) Error += Error
// 155) Error += byte
// 156) Error += char
// 157) Error += short
// 158) Error += int
// 159) Error += long
// 160) Error += float
// 161) byte += untyped
// 162) byte += boolean
// 163) byte += Number
// 164) byte += String
// 165) byte += Object
// 166) byte += Date
// 167) byte += Array
// 168) byte += function
// 169) byte += RegExp
// 170) byte += Error
// 171) byte += byte
// 172) byte += char
// 173) byte += short
// 174) byte += int
// 175) byte += long
// 176) byte += float
// 177) Char += untyped
// 178) Char += boolean
// 179) Char += Number
// 180) Char += String
// 181) Char += Object
// 182) Char += Date
// 183) Char += Array
// 184) Char += function
// 185) Char += RegExp
// 186) Char += Error
// 187) Char += byte
// 188) Char += char
// 189) Char += short
// 190) Char += int
// 191) Char += long
// 192) Char += float
// 193) short += untyped
// 194) short += boolean
// 195) short += Number
// 196) short += String
// 197) short += Object
// 198) short += Date
// 199) short += Array
// 200) short += function
// 201) short += RegExp
// 202) short += Error
// 203) short += byte
// 204) short += char
// 205) short += short
// 206) short += int
// 207) short += long
// 208) short += float
// 209) int += untyped
// 210) int += boolean
// 211) int += Number
// 212) int += String
// 213) int += Object
// 214) int += Date
// 215) int += Array
// 216) int += function
// 217) int += RegExp
// 218) int += Error
// 219) int += byte
// 220) int += char
// 221) int += short
// 222) int += int
// 223) int += long
// 224) int += float
// 225) long += untyped
// 226) long += boolean
// 227) long += Number
// 228) long += String
// 229) long += Object
// 230) long += Date
// 231) long += Array
// 232) long += function
// 233) long += RegExp
// 234) long += Error
// 235) long += byte
// 236) long += char
// 237) long += short
// 238) long += int
// 239) long += long
// 240) long += float
// 241) float += untyped
// 242) float += boolean
// 243) float += Number
// 244) float += String
// 245) float += Object
// 246) float += Date
// 247) float += Array
// 248) float += function
// 249) float += RegExp
// 250) float += Error
// 251) float += byte
// 252) float += char
// 253) float += short
// 254) float += int
// 255) float += long
// 256) float += float

              






function asgnadd02() {
    var tmp,res,expected,emptyVar;

    var uvar = 4321;
    var boolvar:boolean = true;
    var numvar:Number = 3213.321;
    var strvar:String = "this is a string";
    var obvar:Object = new Object;
    var datevar:Date = new Date(0);
    var arrvar:Array = new Array;
    arrvar[2] = 2.2;
    arrvar[3] = 3.3;
    arrvar[4] = 4.4;
    function funvar(){var foo = 1};
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
    var strOnM:String = -2146823266;

    apInitTest("asgnadd02");



///////////////////////////////////////////////////////////////////
/////////////////  RegExp   ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("RegExp += untyped");
    uvar = 4321;
    res = strNE;
    expected = strTM;
    try{eval("revar += uvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += boolean");
    boolvar = true;
    res = strNE;
    expected = strTM;
    try{eval("revar += boolvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += Number");
    numvar = 4321;
    res = strNE;
    expected = strTM;
    try{eval("revar += numvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += String");
    strvar = "4321";
    res = strNE;
    expected = strTM;
    try{eval("revar += strvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += Object");
    obvar = 4321;
    res = strNE;
    expected = strTM;
    try{eval("revar += obvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += Date");
    datevar = new Date(0);
    res = strNE;
    expected = strTM;
    try{eval("revar += datevar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("RegExp += Array");
    res = strNE;
    expected = strTM;
    try{eval("revar += arrvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += function");
    // revar initialized above
    res = strNE;
    expected = strTM;
    try{eval("revar += funvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

// TODOPGM

    apInitScenario("RegExp += RegExp");
    revar = /\d{2}-\d{2}/g
    res = strNE;
    @if(@_fast)
      expected = strTM;
    @else
      expected = strOnM;
    @end
    try{eval("revar += /d/");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += Error");
    errvar = new Error();
    res = strNE;
    expected = strTM;
    try{eval("revar += errvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += byte");
    bytevar = 21;
    res = strNE;
    expected = strTM;
    try{eval("revar += bytevar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += char");
    charvar = "A";
    res = strNE;
    expected = strTM;
    try{eval("revar += charvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += short");
    shrtvar = -32000;
    res = strNE;
    expected = strTM;
    try{eval("revar += shrtvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += int");
    intvar = 4321;
    res = strNE;
    expected = strTM;
    try{eval("revar += intvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += long");
    longvar = 12344321;
    res = strNE;
    expected = strTM;
    try{eval("revar += longvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("RegExp += float");
    flovar = 4321;
    res = strNE;
    expected = strTM;
    try{eval("revar += flovar");}
    catch(e){res = e.number}
    if (expected != res) {
    apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  Error    ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("Error += untyped");
    errvar = new Error();
    uvar = 4321;
    res = strNE;
    expected = strTM;
    try{eval("errvar += uvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += boolean");
    errvar = new Error();
    boolvar = true;
    res = strNE;
    expected = strTM;
    try{eval("errvar += boolvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += Number");
    errvar = new Error();
    numvar = 4321;
    res = strNE;
    expected = strTM;
    try{eval("errvar += numvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += String");
    errvar = new Error();
    strvar = "4321";
    res = strNE;
    expected = strTM;
    try{eval("errvar += strvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += Object");
    errvar = new Error();
    obvar = 4321;
    res = strNE;
    expected = strTM;
    try{eval("errvar += obvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += Date");
    errvar = new Error();
    datevar = new Date(0);
    res = strNE;
    expected = strTM;
    try{eval("errvar += datevar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("Error += Array");
    errvar = new Error();
    // arrvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("errvar += arrvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += function");
    errvar = new Error();
    // funvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("errvar += funvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += RegExp");
    errvar = new Error();
    revar = /\d{2}-\d{2}/g
    res = strNE;
    expected = strTM;
    try{eval("errvar += revar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += Error");
    errvar = new Error();
    res = strNE;
    expected = strTM;
    try{eval("errvar += new Error()");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += byte");
    errvar = new Error();
    bytevar = 21;
    res = strNE;
    expected = strTM;     try{eval("errvar += bytevar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += char");
    errvar = new Error();
    charvar = "A";
    res = strNE;
    expected = strTM;
    try{eval("errvar += charvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += short");
    errvar = new Error();
    shrtvar = -32000;
    res = strNE;
    expected = strTM;
    try{eval("errvar += shrtvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += int");
    errvar = new Error();
    intvar = 4321;
    res = strNE;
    expected = strTM;
    try{eval("errvar += intvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += long");
    errvar = new Error();
    longvar = 12344321;
    res = strNE;
    expected = strTM;
    try{eval("errvar += longvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Error += float");
    errvar = new Error();
    flovar = 4321;
    res = strNE;
    expected = strTM;
    try{eval("errvar += flovar");}
    catch(e){res = e.number}
    if (expected != res) {
    apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  Byte     ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("byte += untyped");
    bytevar = 123;
    uvar = 43;
    bytevar += uvar;
    expected = byte(166)
    if (doVerify(expected,bytevar)) {
        apLogFailInfo("wrong return value",expected,bytevar,"");
    }
    bytevar = 123;
    uvar = 432;
    res = strNE;
    expected = strTM;
    try{eval("bytevar += uvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }
    bytevar = 123;
    uvar = -200;
    res = strNE;
    expected = strTM;
    try{eval("bytevar += uvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte += boolean");
    bytevar = 123;
    boolvar = true;
    bytevar += boolvar;                        // boolean changes to 1
    expected = byte(124)
    if (doVerify(expected,bytevar)) {
        apLogFailInfo("wrong return value",expected,bytevar,"");
    }


    apInitScenario("byte += Number");
    bytevar = 123;
    numvar = 43;
    bytevar += numvar;
    expected = byte(166)
    if (doVerify(expected,bytevar)) {
        apLogFailInfo("wrong return value",expected,bytevar,"");
    }
    bytevar = 123;
    numvar = 43.1;
    res = strNE;
    expected = strTM;
    try{eval("bytevar += numvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte += String");
    bytevar = 1;
    strvar = "43";
    expected = 143;
    bytevar += strvar
    if (doVerify(expected,bytevar)) {
        apLogFailInfo("wrong return value",expected,bytevar,"283993");
    }


    apInitScenario("byte += Object");
    bytevar = 123;
    obvar = 43;
    bytevar += obvar;
    expected = byte(166)
    if (doVerify(expected,bytevar)) {
        apLogFailInfo("wrong return value",expected,bytevar,"");
    }


    apInitScenario("byte += Date");
    bytevar = 123;
    datevar = new Date(0);
    res = strNE;
    expected = strTM;
    try{eval("bytevar += datevar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("byte += Array");
    bytevar = 123;
    // arrvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("bytevar += arrvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte += function");
    bytevar = 123;
    // funvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("bytevar += funvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte += RegExp");
    bytevar = 123;
    revar = /\d{2}-\d{2}/g
    res = strNE;
    expected = strTM;
    try{eval("bytevar += revar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte += Error");
    bytevar = 123;
    errvar = new Error();
    res = strNE;
    expected = strTM;
    try{eval("bytevar += errvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte += byte");
    bytevar = 123;
    bytevar += byte(21);
    expected = byte(144)
    if (doVerify(expected,bytevar)) {
        apLogFailInfo("wrong return value",expected,bytevar,"");
    }


    apInitScenario("byte += char");
    bytevar = 123;
    charvar = "A";
    expected = 188;
    bytevar += charvar
    if (doVerify(expected,bytevar)) {
        apLogFailInfo("wrong return value",expected,bytevar,"");
    }


    apInitScenario("byte += short");
    bytevar = 10;
    shrtvar = 20;
    bytevar += shrtvar;
    expected = byte(30)
    if (doVerify(expected,bytevar)) {
        apLogFailInfo("wrong return value",expected,bytevar,"");
    }
    bytevar = 10;
    shrtvar = 400;
    res = strNE;
    expected = strTM;
    try{eval("bytevar += shrtvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte += int");
    bytevar = 123;
    intvar = -100;
    bytevar += intvar;
    expected = byte(23);
    if (doVerify(expected,bytevar)) {
        apLogFailInfo("wrong return value",expected,bytevar,"267209");
    }


    apInitScenario("byte += long");
    bytevar = 123;
    longvar = 12;
    bytevar += longvar;
    expected = byte(135)
    if (doVerify(expected,bytevar)) {
        apLogFailInfo("wrong return value",expected,bytevar,"");
    }
    bytevar = 123;
    longvar = 2147482414;
    res = strNE;
    expected = strTM;
    try{eval("bytevar += longvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("byte += float");
    bytevar = 123;
    flovar = 8;
    bytevar += flovar;
    expected = byte(131)
    if (doVerify(expected,bytevar)) {
        apLogFailInfo("wrong return value",expected,bytevar,"");
    }
    bytevar = 123;
    flovar = 8.1;
    res = strNE;
    expected = strTM;
    try{eval("bytevar += flovar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  Char     ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("Char += untyped");
    charvar = "1";
    uvar = 5;
    charvar += uvar;
    expected = "6";
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }
    charvar = char("\u0001");
    uvar = 65;
    charvar += uvar;
    expected = "B";
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }


    apInitScenario("Char += boolean");
    charvar = "a";
    boolvar = true;
    charvar += boolvar;                    // boolean changes to 1
    expected = "b"
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }


    apInitScenario("Char += Number");
    charvar = "k";
    numvar = -char("j");
    charvar += numvar;
    expected = char("\u0001");
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }
    charvar = "4";
    numvar = 1.1;
    res = strNE;
    expected = strTM;
    try{eval("charvar += numvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char += String");
    charvar = "j";
    strvar = ""
    charvar += strvar;
    expected = "j"
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }


    apInitScenario("Char += Object");
    charvar = "2";
    obvar = "";
    charvar += obvar;
    expected = "2";
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }


    apInitScenario("Char += Date");
    charvar = 0;
    datevar = new Date(0);
    res = strNE;
    expected = strTM;
    try{eval("charvar += datevar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("Char += Array");
    charvar = "1";
    // arrvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("charvar += arrvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char += function");
    charvar = "\n";
    // funvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("charvar += funvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char += RegExp");
    charvar = "s";
    revar = /\d{2}-\d{2}/g
    res = strNE;
    expected = strTM;
    try{eval("charvar += revar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char += Error");
    charvar = "f";
    errvar = new Error();
    res = strNE;
    expected = strTM;
    try{eval("charvar += errvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char += byte");
    charvar = "a";
    bytevar = 25;
    charvar += bytevar;
    expected = "z"
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }


    apInitScenario("Char += char");
    charvar = 0;
    res = strNE;
    expected = strTM;
    try{eval("charvar += char(2)");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("Char += short");
    charvar = char("\u0005");
    shrtvar = 65;
    charvar += shrtvar;
    expected = "F"
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }


    apInitScenario("Char += int");
    charvar = char("\u007b");
    intvar = -100;
    charvar += intvar;
    expected = char("\u0017");
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }


    apInitScenario("Char += long");
    charvar = "A";
    longvar = 2;
    charvar += longvar;
    expected = "C"
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }
    charvar = "C";
    longvar = -2;
    charvar += longvar;
    expected = "A"
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }


    apInitScenario("Char += float");
    charvar = "b";
    flovar = -1;
    charvar += flovar;
    expected = "a"
    if (doVerify(expected,charvar)) {
        apLogFailInfo("wrong return value",expected,charvar,"");
    }
    charvar = "t";
    flovar = 2.3;
    res = strNE;
    expected = strTM;
    try{eval("charvar += flovar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  short    ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("short += untyped");
    shrtvar = 1234;
    uvar = 4321;
    shrtvar += uvar;
    expected = short(5555)
    if (doVerify(expected,shrtvar)) {
        apLogFailInfo("wrong return value",expected,shrtvar,"");
    }


    apInitScenario("short += boolean");
    shrtvar = 1234;
    boolvar = true;
    shrtvar += boolvar;                        // boolean changes to 1
    expected = short(1235)
    if (doVerify(expected,shrtvar)) {
        apLogFailInfo("wrong return value",expected,shrtvar,"");
    }


    apInitScenario("short += Number");
    shrtvar = 1234;
    numvar = 4321;
    shrtvar += numvar;
    expected = short(5555)
    if (doVerify(expected,shrtvar)) {
        apLogFailInfo("wrong return value",expected,shrtvar,"");
    }
    shrtvar = 1234;
    numvar = 4321.1;
    res = strNE;
    expected = strTM;
    try{eval("shrtvar += numvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short += String");
    shrtvar = 12;
    strvar = "21";
    shrtvar += strvar
    expected = 1221
    if (doVerify(expected,shrtvar)) {
        apLogFailInfo("wrong return value",expected,shrtvar,"283993");
    }


    apInitScenario("short += Object");
    shrtvar = 1234;
    obvar = 4321;
    shrtvar += obvar;
    expected = short(5555)
    if (doVerify(expected,shrtvar)) {
        apLogFailInfo("wrong return value",expected,shrtvar,"");
    }


    apInitScenario("short += Date");
    shrtvar = 1234;
    datevar = new Date(0);
    res = strNE;
    expected = strTM;
    try{eval("shrtvar += datevar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("short += Array");
    shrtvar = 1234;
    // arrvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("shrtvar += arrvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short += function");
    shrtvar = 1234;
    // funvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("shrtvar += funvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short += RegExp");
    shrtvar = 1234;
    revar = /\d{2}-\d{2}/g
    res = strNE;
    expected = strTM;
    try{eval("shrtvar += revar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short += Error");
    shrtvar = 1234;
    errvar = new Error();
    res = strNE;
    expected = strTM;
    try{eval("shrtvar += errvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short += byte");
    shrtvar = -1234;
    bytevar = 34;
    shrtvar += bytevar;
    expected = short(-1200)
    if (doVerify(expected,shrtvar)) {
        apLogFailInfo("wrong return value",expected,shrtvar,"");
    }


    apInitScenario("short += char");
    shrtvar = 1234;
    charvar = "A";
    res = strNE;
    expected = strTM;
    try{eval("shrtvar += charvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short += short");
    shrtvar = -10000;
    shrtvar += short(9999);
    expected = short(-1)
    if (doVerify(expected,shrtvar)) {
        apLogFailInfo("wrong return value",expected,shrtvar,"");
    }


    apInitScenario("short += int");
    shrtvar = 1234;
    intvar = 12345
    shrtvar += intvar;
    expected = short(13579);
    if (doVerify(expected,shrtvar)) {
        apLogFailInfo("wrong return value",expected,shrtvar,"");
    }


    apInitScenario("short += long");
    shrtvar = 1234;
    longvar = -1234;
    shrtvar += longvar;
    expected = short(0)
    if (doVerify(expected,shrtvar)) {
        apLogFailInfo("wrong return value",expected,shrtvar,"");
    }
    shrtvar = 1234;
    longvar = 2147482414;
    res = strNE;
    expected = strTM;
    try{eval("shrtvar += longvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("short += float");
    shrtvar = 1234;
    flovar = 4321;
    shrtvar += flovar;
    expected = int(5555)
    if (doVerify(expected,shrtvar)) {
        apLogFailInfo("wrong return value",expected,shrtvar,"");
    }
    shrtvar = 1234;
    flovar = 4321.1;
    res = strNE;
    expected = strTM;
    try{eval("shrtvar += flovar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  int      ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("int += untyped");
    intvar = 1234;
    uvar = 4321;
    intvar += uvar;
    expected = int(5555)
    if (doVerify(expected,intvar)) {
        apLogFailInfo("wrong return value",expected,intvar,"");
    }


    apInitScenario("int += boolean");
    intvar = 1234;
    boolvar = true;
    intvar += boolvar;                        // boolean changes to 1
    expected = int(1235)
    if (doVerify(expected,intvar)) {
        apLogFailInfo("wrong return value",expected,intvar,"");
    }


    apInitScenario("int += Number");
    intvar = 1234;
    numvar = 4321;
    intvar += numvar;
    expected = int(5555)
    if (doVerify(expected,intvar)) {
        apLogFailInfo("wrong return value",expected,intvar,"");
    }
    intvar = 1234;
    numvar = 4321.1;
    res = strNE;
    expected = strTM;
    try{eval("intvar += numvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int += String");
    intvar = 1234;
    strvar = "4321";
    intvar += strvar
    expected = 12344321
    if (doVerify(expected,intvar)) {
        apLogFailInfo("wrong return value",expected,intvar,"283993");
    }


    apInitScenario("int += Object");
    intvar = 1234;
    obvar = 4321;
    intvar += obvar;
    expected = int(5555)
    if (doVerify(expected,intvar)) {
        apLogFailInfo("wrong return value",expected,intvar,"");
    }


    apInitScenario("int += Date");
    intvar = 1234;
    datevar = new Date(0);
    res = strNE;
    expected = strTM;
    try{eval("intvar += datevar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("int += Array");
    intvar = 1234;
    // arrvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("intvar += arrvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int += function");
    intvar = 1234;
    // funvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("intvar += funvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int += RegExp");
    intvar = 1234;
    revar = /\d{2}-\d{2}/g
    res = strNE;
    expected = strTM;
    try{eval("intvar += revar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int += Error");
    intvar = 1234;
    errvar = new Error();
    res = strNE;
    expected = strTM;
    try{eval("intvar += errvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int += byte");
    intvar = 1234;
    bytevar = 21;
    intvar += bytevar;
    expected = int(1255)
    if (doVerify(expected,intvar)) {
        apLogFailInfo("wrong return value",expected,intvar,"");
    }


    apInitScenario("int += char");
    intvar = 1234;
    charvar = "A";
    intvar += charvar;
    expected = int(1299)
    if (doVerify(expected,intvar)) {
        apLogFailInfo("wrong return value",expected,intvar,"");
    }


    apInitScenario("int += short");
    intvar = -10000;
    shrtvar = -32000;
    intvar += shrtvar;
    expected = int(-42000)
    if (doVerify(expected,intvar)) {
        apLogFailInfo("wrong return value",expected,intvar,"");
    }


    apInitScenario("int += int");
    intvar = 1234;
    intvar += int(1234567890);
    expected = int(1234569124);
    if (doVerify(expected,intvar)) {
        apLogFailInfo("wrong return value",expected,intvar,"");
    }


    apInitScenario("int += long");
    intvar = 1234;
    longvar = 12344321;
    intvar += longvar;
    expected = int(12345555)
    if (doVerify(expected,intvar)) {
        apLogFailInfo("wrong return value",expected,intvar,"");
    }
    intvar = 1234;
    longvar = 2147482414;
    res = strNE;
    expected = strTM;
    try{eval("intvar += longvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("int += float");
    intvar = 1234;
    flovar = 4321;
    intvar += flovar;
    expected = int(5555)
    if (doVerify(expected,intvar)) {
        apLogFailInfo("wrong return value",expected,intvar,"");
    }
    intvar = 1234;
    flovar = 4321.1;
    res = strNE;
    expected = strTM;
    try{eval("intvar += flovar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  long     ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("long += untyped");
    longvar = 1234;
    uvar = 4321;
    longvar += uvar;
    expected = long(5555)
    if (doVerify(expected,longvar)) {
        apLogFailInfo("wrong return value",expected,longvar,"");
    }


    apInitScenario("long += boolean");
    longvar = 1234;
    boolvar = true;
    longvar += boolvar;                        // boolean changes to 1
    expected = long(1235)
    if (doVerify(expected,longvar)) {
        apLogFailInfo("wrong return value",expected,longvar,"");
    }


    apInitScenario("long += Number");
    longvar = 1234;
    numvar = 4321;
    longvar += numvar;
    expected = long(5555)
    if (doVerify(expected,longvar)) {
        apLogFailInfo("wrong return value",expected,longvar,"");
    }
    longvar = 1234;
    numvar = 4321.1;
    res = strNE;
    expected = strTM;
    try{eval("longvar += numvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long += String");
    longvar = 123456;
    strvar = "654321";
    expected = 123456654321;
    longvar += strvar
    if (doVerify(expected,longvar)) {
        apLogFailInfo("wrong return value",expected,longvar,"283993");
    }


    apInitScenario("long += Object");
    longvar = 1234;
    obvar = 4321;
    longvar += obvar;
    expected = long(5555)
    if (doVerify(expected,longvar)) {
        apLogFailInfo("wrong return value",expected,longvar,"");
    }


    apInitScenario("long += Date");
    longvar = 1234;
    datevar = new Date(0);
    res = strNE;
    expected = strTM;
    try{eval("longvar += datevar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }

    
    apInitScenario("long += Array");
    longvar = 1234;
    // arrvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("longvar += arrvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long += function");
    longvar = 1234;
    // funvar initialized above
    res = strNE;
    expected = strTM;
    try{eval("longvar += funvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long += RegExp");
    longvar = 1234;
    revar = /\d{2}-\d{2}/g
    res = strNE;
    expected = strTM;
    try{eval("longvar += revar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long += Error");
    longvar = 1234;
    errvar = new Error();
    res = strNE;
    expected = strTM;
    try{eval("longvar += errvar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long += byte");
    longvar = 12345678900;
    bytevar = 21;
    longvar += bytevar;
    expected = long(12345678921)
    if (doVerify(expected,longvar)) {
        apLogFailInfo("wrong return value",expected,longvar,"");
    }


    apInitScenario("long += char");
    longvar = -12345678965;
    charvar = "A";
    longvar += charvar;
    expected = long(-12345678900)
    if (doVerify(expected,longvar)) {
        apLogFailInfo("wrong return value",expected,longvar,"");
    }


    apInitScenario("long += short");
    longvar = -10000;
    shrtvar = -32000;
    longvar += shrtvar;
    expected = long(-42000)
    if (doVerify(expected,longvar)) {
        apLogFailInfo("wrong return value",expected,longvar,"");
    }


    apInitScenario("long += int");
    longvar = -91234567890;
    intvar = 1234567890
    longvar += intvar;
    expected = long(-90000000000);
    if (doVerify(expected,longvar)) {
        apLogFailInfo("wrong return value",expected,longvar,"");
    }


    apInitScenario("long += long");
    longvar = 1234;
    longvar += long(-1235);
    expected = long(-1)
    if (doVerify(expected,longvar)) {
        apLogFailInfo("wrong return value",expected,longvar,"");
    }
    longvar = 9223372036854775807;
    res = strNE;
    expected = strTM;
    try{eval("longvar += long(1)");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


    apInitScenario("long += float");
    longvar = 1234;
    flovar = 4321;
    longvar += flovar;
    expected = long(5555)
    if (doVerify(expected,longvar)) {
        apLogFailInfo("wrong return value",expected,longvar,"");
    }
    longvar = 1234;
    flovar = 4321.1;
    res = strNE;
    expected = strTM;
    try{eval("longvar += flovar");}
    catch(e){res = e.number}
    if (expected != res) {
        apLogFailInfo("wrong return value",expected,res,"");
    }


///////////////////////////////////////////////////////////////////
/////////////////  float    ///////////////////////////////////////
///////////////////////////////////////////////////////////////////
    apInitScenario("float += untyped");
    flovar = 1234;
    uvar = 4321;
    flovar += uvar;
    expected = float(5555)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }
    flovar = 1234.5;
    uvar = 4321;
    flovar += uvar;
    expected = float(5555.5)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }
    flovar = 1234;
    uvar = 4321.5;
    flovar += uvar;
    expected = float(5555.5)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }
    flovar = 1234.25;
    uvar = 4321.25;
    flovar += uvar;
    expected = float(5555.5)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += boolean");
    flovar = 1234;
    boolvar = true;
    flovar += boolvar;                        // boolean changes to 1
    expected = float(1235)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += Number");
    flovar = 1234;
    numvar = 4321;
    flovar += numvar;
    expected = float(5555)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }
    flovar = 1234;
    numvar = 4321.1
    flovar += numvar;
    expected = float(5555.1)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += String");
    flovar = 12;
    strvar = "21.5";
    flovar += strvar;
    expected = 1221.5
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"283993");
    }


    apInitScenario("float += Object");
    flovar = 1234;
    obvar = 4321;
    flovar += obvar;
    expected = int(5555)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += Date");
    flovar = 1234;
    datevar = new Date(0);
    flovar += datevar;
    if (!isNaN(flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }

    
    apInitScenario("float += Array");
    flovar = 1234;
    // arrvar initialized above
    flovar += arrvar;
    if (!isNaN(flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += function");
    flovar = 1234;
    // funvar initialized above
    flovar += funvar;
    if (!isNaN(flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += RegExp");
    flovar = 1234;
    revar = /\d{2}-\d{2}/g
    flovar += revar;
    if (!isNaN(flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += Error");
    flovar = 1234;
    errvar = new Error();
    flovar += errvar;
    if (!isNaN(flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += byte");
    flovar = -634.1;
    bytevar = 34;
    flovar += bytevar;
    expected = float(-600.1)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += char");
    flovar = -665.1;
    charvar = "A";
    flovar += charvar;
    expected = float(-600.1)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += short");
    flovar = -10000;
    shrtvar = -32000;
    flovar += shrtvar;
    expected = float(-42000)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += int");
    flovar = 1234;
    intvar = 1234567890
    flovar += intvar;
    expected = float(1234569124);
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += long");
    flovar = 1234;
    longvar = 12344321;
    flovar += longvar;
    expected = Number(12345555)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }
    flovar = 1234;
    longvar = 2147482414;
    flovar += longvar;
    expected = Number(2147483648)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }


    apInitScenario("float += float");
    flovar = 1234;
    flovar += float(4321);
    expected = float(5555)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
    }
    flovar = 1234;
    flovar += float(4321.1);
    expected = float(5555.1)
    if (doVerify(expected,flovar)) {
        apLogFailInfo("wrong return value",expected,flovar,"");
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
//function FuncRetVal() { return 655.35; }
//function myObject() { this.retVal = myObjVal(); }
//function myObjVal() { return 123321; }


asgnadd02();


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
