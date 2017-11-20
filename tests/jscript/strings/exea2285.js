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


var iTestID = 52628;

var sCat = '';
var regPat = "";
var objExp = "";
var m_scen = '';
var strTest = "";
var strTemp = "";
var result = "";

function verify(sCat1, sExp, sAct)
{
	//this function makes sure sAct and sExp are equal

	if (sExp != sAct)
		apLogFailInfo( m_scen+(sCat1.length?"--"+sCat1:"")+" failed", sExp,sAct, "");
}





function verifyStringObj(sCat1, sExp, sAct)
{
	//this function simply calls verify with the values of the string
	verify(sCat1, sExp.valueOf(), sAct.valueOf());
}





function ArrayEqual(sCat1, arrayExp, arrayAct)
{  var i;
	//Makes Sure that Arrays are equal
	if (arrayAct == null)
		verify(sCat1 + ' NULL Err', arrayExp, arrayAct);
	else if (arrayExp.length != arrayAct.length)
		verify(sCat1 + ' Array length', arrayExp.length, arrayAct.length);
	else
	{
		for (i in arrayExp)
			verify(sCat1 + ' index '+i, arrayExp[i], arrayAct[i]);
	}
}







function regVerify(sCat1, arrayReg, arrayAct)
{
	var i;
	var expArray = new Array('','','','','','','','','');

	for (i in arrayReg)
		if (i < 10)
			expArray[i] = arrayReg[i];
		else
			break;

	for(i =0; i<9;i++)
		verify(sCat1 + ' RegExp.$'+ (i+1) +' ', expArray[i], eval('RegExp.$'+(i+1)));
}



function exea2285() {
	

apInitTest("Exea2285");



	m_scen = ('Test 17 unicode   /\b\uC000\uC001\uC002\b/');

sCat = "/\b\uC000\uC001\uC002\b/ ";
	apInitScenario(m_scen);
regPat = /\b\uC000\uC001\uC002\b/;
objExp = new Array('\uC000\uC001\uC002');
var regExp = new Array();

var strTest = '\u005f\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


strTest = '      \u005f\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '          \u005f\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001 \u005f\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001      \u005f\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uFfFf \u005f\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001	\u005f\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001          \u005f\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001          \u005f\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 18 Slightly more complex strings');

strTest = '\u005f\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC000\uC001 \u005f\uC000\uC001\uC002\u005f \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 19 unicode   tests w/ multiples');

strTest = '\u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '     \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f      ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = ' \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f      ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '         \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '         \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 20 Slightly more complex strings w/ multiple finds');

strTest = '\u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '           \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000|'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '           \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f \u005f\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end



	m_scen = ('Test 21 unicode   /\b\uC000\uC001\uC002\B/');

sCat = "/\b\uC000\uC001\uC002\B/ ";
	apInitScenario(m_scen);
regPat = /\b\uC000\uC001\uC002\B/;
objExp = new Array('\uC000\uC001\uC002');
regExp = new Array();


strTest = '\u005f\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


strTest = '      \u005f\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '          \u005f\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001 \u005f\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001      \u005f\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uFfFf \u005f\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001	\u005f\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001          \u005f\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001          \u005f\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 22 Slightly more complex strings');

strTest = '\u005f\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC000\uC001 \u005f\uC000\uC001\uC002\uC501 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 23 unicode   tests w/ multiples');

strTest = '\u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '     \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501      ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = ' \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501      ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '         \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '         \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 24 Slightly more complex strings w/ multiple finds');

strTest = '\u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '           \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000|'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '           \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501 \u005f\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 25 unicode   /\B\uC000\uC001\uC002\b/');

sCat = "/\B\uC000\uC001\uC002\b/ ";
	apInitScenario(m_scen);
regPat = /\B\uC000\uC001\uC002\b/;
objExp = new Array('\uC000\uC001\uC002');
regExp = new Array();


strTest = '\uC501\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


strTest = '      \uC501\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '          \uC501\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001 \uC501\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001      \uC501\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uFfFf \uC501\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001	\uC501\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001          \uC501\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001          \uC501\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 26 Slightly more complex strings');

strTest = '\uC501\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC000\uC001 \uC501\uC000\uC001\uC002\u005f \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 27 unicode   tests w/ multiples');

strTest = '\uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '     \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f      ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = ' \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f      ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '         \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '         \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 28 Slightly more complex strings w/ multiple finds');

strTest = '\uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '           \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000|'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '           \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f \uC501\uC000\uC001\uC002\u005f\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 29 unicode   /\B\uC000\uC001\uC002\B/');

sCat = "/\B\uC000\uC001\uC002\B/ ";
	apInitScenario(m_scen);
regPat = /\B\uC000\uC001\uC002\B/;
objExp = new Array('\uC000\uC001\uC002');
regExp = new Array();


strTest = '\uC501\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


strTest = '      \uC501\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '          \uC501\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001 \uC501\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001      \uC501\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uFfFf \uC501\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001	\uC501\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001          \uC501\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC000\uC001          \uC501\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 30 Slightly more complex strings');

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC000\uC001 \uC501\uC000\uC001\uC002\uC501 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 31 unicode   tests w/ multiples');

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '     \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501      ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = ' \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501      ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '         \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '         \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 32 Slightly more complex strings w/ multiple finds');

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '           \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC000\uC001\uABCA\uC000\uC002\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000|'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '              \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '\uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

strTest = '           \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501 \uC501\uC000\uC001\uC002\uC501\uC000\uC501\uC000\uC001\uABCA\uC000             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


/*****************************************************************************/
	apEndTest();
}


exea2285();


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
