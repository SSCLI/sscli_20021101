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


var iTestID = 53819;

	//This testcase uses the /ABC/ pattern as the bases for the pattern searching.
	
var sCat = '';
var regPat = "";
var objExp = "";
var m_scen = '';
var strTest = "";
var strTemp = "";
var regExp = "";


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
{var i;
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



function tst_32() {
	

apInitTest("tst_32");
	
	
	
	m_scen = ('Test 1 mixedCase /\u0000\001Anmnoz/');

	
	sCat = "/\u0000\001Anmnoz/ ";
	apInitScenario(m_scen);
	regPat = /\u0000\001Anmnoz/;
	objExp = true;
	regExp = new Array();
	
	strTest = '\u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	
	strTest = '      \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '          \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '          \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A      \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A\u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A	\u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A          \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A          \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	m_scen = ('Test 2 Slightly more complex strings');

	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	
	m_scen = ('Test 3 mixedCASE tests /\u0000\001Anmnoz/ multiples');

	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '     \u0000\001Anmnoz \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz      ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz      |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '         \u0000\001Anmnoz \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '         \u0000\001Anmnoz \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	m_scen = ('Test 4 Slightly more complex strings w/ multiple finds');

	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|           \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001Anmnoz\u00001def\u0000\001A\000\u0000';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000            |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|            \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000           |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|           \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	m_scen = ('Test 5 mixedCASE /(\u0000\001Anmnoz)/');

	
	sCat = "/(\u0000\001Anmnoz)/ ";
	apInitScenario(m_scen);
	regPat = /(\u0000\001Anmnoz)/;
	objExp = true;
	regExp = new Array('\u0000\001Anmnoz');
	
	strTest = '\u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '      \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '          \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '          \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A      \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A\u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A	\u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A          \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A          \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	m_scen = ('Test 6 Slightly more complex strings');

	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	m_scen = ('Test 7 mixedCASE tests /(\u0000\001Anmnoz)/ multiples');

	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '     \u0000\001Anmnoz \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz      ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz      |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '         \u0000\001Anmnoz \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '         \u0000\001Anmnoz \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	
	m_scen = ('Test 8 Slightly more complex strings w/ multiple finds');

	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|           \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001Anmnoz\u00001def\u0000\001A\000\u0000';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000            |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|            \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000           | '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|           \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             | '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	m_scen = ('Test 9 mixedCASE /(\u0000\001A)(nmnoz)/');

	
	sCat = "/(\u0000\001A)(nmnoz)/ ";
	apInitScenario(m_scen);
	regPat = /(\u0000\001A)(nmnoz)/;
	objExp = true;
	regExp = new Array('\u0000\001A','nmnoz');
	
	strTest = '\u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '      \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '          \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '          \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A      \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A\u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A	\u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A          \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A          \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	
	m_scen = ('Test 10 Slightly more complex strings');

	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	
	m_scen = ('Test 11 mixedCASE tests /(\u0000\001A)(nmnoz)/ multiples');

	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '     \u0000\001Anmnoz \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz      ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz      |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '         \u0000\001Anmnoz \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '         \u0000\001Anmnoz \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	m_scen = ('Test 12 Slightly more complex strings w/ multiple finds');

	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|           \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001Anmnoz\u00001def\u0000\001A\000\u0000';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000            |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|            \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000           | '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|           \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             | '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	m_scen = ('Test 13 mixedCASE /((\u0000\001A)(nmnoz))/');

	
	sCat = "/((\u0000\001A)(nmnoz))/ ";
	apInitScenario(m_scen);
	regPat = /((\u0000\001A)(nmnoz))/;
	objExp = true;
	regExp = new Array('\u0000\001Anmnoz','\u0000\001A','nmnoz');
	
	strTest = '\u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '      \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '          \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '          \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A      \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A\u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A	\u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A          \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001A          \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	
	m_scen = ('Test 14 Slightly more complex strings');

	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001A nmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	
	m_scen = ('Test 15 mixedCASE tests /((\u0000\001A)(nmnoz))/ multiples');

	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '     \u0000\001Anmnoz \u0000\001Anmnoz';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz      ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz      |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '         \u0000\001Anmnoz \u0000\001Anmnoz       ';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '         \u0000\001Anmnoz \u0000\001Anmnoz       |';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	
	m_scen = ('Test 16 Slightly more complex strings w/ multiple finds');

	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|           \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001Anmnoz\u00001def\u0000\001A\000\u0000';
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|              \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000            |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|            \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '\u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000           | '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
	strTest = '|           \u0000\001Anmnoz \u0000\001Anmnoz \u0000\001A\000\u0000nmnoz\u00001def\u0000\001A\000\u0000             | '; 
	ArrayEqual(sCat+strTest, objExp, regPat.test(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.test(strTest));
	@end
	
/*****************************************************************************/
	apEndTest();
}


tst_32();


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
