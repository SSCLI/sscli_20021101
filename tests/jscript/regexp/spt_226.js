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


var iTestID = 53528;

	var sCat = '';
	var regPat;
	var objExp;
var strTest;
var regExp;

var m_scen = '';


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



function spt_226() {
	

apInitTest("spt_226");


	m_scen = ('Test 1 nonalpha  /^\uC000\uC001\uC002/');

	apInitScenario(m_scen);
	regPat = /^\uC000\uC001\uC002/;
	regExp = new Array();
	
	objExp = new Array();
	strTest = '\uC000\uC001\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('D');
	strTest = '\uC000\uC001\uC002D';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('D                         ');
	strTest = '\uC000\uC001\uC002D                         ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('       ');
	strTest = '\uC000\uC001\uC002       ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end
	

	m_scen = ('Test 2 Slightly more complex strings');

	objExp = new Array(' \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '\uC000\uC001\uC002 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	
	objExp = new Array(' \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uC000\uC001\uC002 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end
	

	objExp = new Array('D \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001 \uC002            ');
	strTest = '\uC000\uC001\uC002D \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001 \uC002            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end
	
	
	m_scen = ('Test 3 nonalpha  tests w/ multiples');

	objExp = new Array(' \uC000\uC001\uC002');
	strTest = '\uC000\uC001\uC002 \uC000\uC001\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array(' \uC000\uC001\uC002      ');
	strTest = '\uC000\uC001\uC002 \uC000\uC001\uC002      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('D \uC000\uC001\uC002\uABCD      ');
	strTest = '\uC000\uC001\uC002D \uC000\uC001\uC002\uABCD      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 4 Slightly more complex strings w/ multiple finds');

	objExp = new Array(' \uC000\uC001\uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '\uC000\uC001\uC002 \uC000\uC001\uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array(' \uC000\uC001\uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uC000\uC001\uC002 \uC000\uC001\uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uC000\uC001\uC002\uC000\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uABCD              ');
	strTest = '\uC000\uC001\uC002\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uC000\uC001\uC002\uC000\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uABCD              '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('\uB000 D\uC000\uC001\uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uC000\uC001\uC002\uB000 D\uC000\uC001\uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array(' \uC000\uC001\uC002 \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uC000\uC001\uC002 \uC000\uC001\uC002 \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('D \uC000\uC000\uC001\uC000\uC001\uC002\uC000\uC001 \uC000\uC000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '\uC000\uC001\uC002D \uC000\uC000\uC001\uC000\uC001\uC002\uC000\uC001 \uC000\uC000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 5 nonalpha  /\uC000\uC001\uC002$/');

	apInitScenario(m_scen);
	regPat = /\uC000\uC001\uC002$/;
	regExp = new Array();

	objExp = new Array();
	strTest = '\uC000\uC001\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('D');
	strTest = 'D\uC000\uC001\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('      ');
	strTest = '      \uC000\uC001\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001 ');
	strTest = '\uC000\uC001 \uC000\uC001\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001      ');
	strTest = '\uC000\uC001      \uC000\uC001\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 6 Slightly more complex strings');

	objExp = new Array('\uC000\uB000\uC002 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA');
	strTest = '\uC000\uB000\uC002 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000\uB000\uC002 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA');
	strTest = '              \uC000\uB000\uC002 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 7 nonalpha  tests w/ multiples');

	objExp = new Array('\uC000\uC001\uC002 ');
	strTest = '\uC000\uC001\uC002 \uC000\uC001\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('     \uC000\uC001\uC002 ');
	strTest = '     \uC000\uC001\uC002 \uC000\uC001\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('  \uC000\uC001             \uC002       \uC000\uC001\uC002                  ');
	strTest = '  \uC000\uC001             \uC002       \uC000\uC001\uC002                  \uC000\uC001\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 8 Slightly more complex strings w/ multiple finds');

	objExp = new Array('\uC000\uC001 \uC000\uC001\uC002 \uC000D \uC000\u1997\uABCD \u00AC\u00AC \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA');
	strTest = '\uC000\uC001 \uC000\uC001\uC002 \uC000D \uC000\u1997\uABCD \u00AC\u00AC \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('			\uC000\uC001\uC002 \uC000D \uC000\u1997\uABCD \u00AC\u00AC \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA');
	strTest = '			\uC000\uC001\uC002 \uC000D \uC000\u1997\uABCD \u00AC\u00AC \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uD000\uABCA\uC000\uB000\uC000\uC001\uC002\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uC000\uC001\uC002\uC000\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA');
	strTest = '              \uD000\uABCA\uC000\uB000\uC000\uC001\uC002\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uC000\uC001\uC002\uC000\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              D\uC000\uC001\uC002\uABCD \uDBCA\uC000\uC001\uC002\u007F \uABCD\uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA');
	strTest = '              D\uC000\uC001\uC002\uABCD \uDBCA\uC000\uC001\uC002\u007F \uABCD\uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC000\uC001\uC002D \uC000\uC000\uC001\uC000\uC001\uC002\uC000\uC001 \uC000\uC000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000');
	strTest = '\uC000\uC000\uC001\uC002D \uC000\uC000\uC001\uC000\uC001\uC002\uC000\uC001 \uC000\uC000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC001\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 9 Failure - Lowercase /^\uC000\uC001\uC002 /');

	apInitScenario(m_scen);
	regPat = /^\uC000\uC001\uC002/;

	objExp = new Array(' \uC000\uC001\uC002\uA000\uB000\uFfFf');
	strTest = ' \uC000\uC001\uC002\uA000\uB000\uFfFf';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('D\uC000\uC001\uC002\uA000\uB000\uFfFf');
	strTest = 'D\uC000\uC001\uC002\uA000\uB000\uFfFf';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('D\uC000\uC001\uC002\uABCD\uA000\uB000\uFfFfD');
	strTest = 'D\uC000\uC001\uC002\uABCD\uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('                                       \uC000\uC001\uC002\uABCD\uA000\uB000\uFfFfD');
	strTest = '                                       \uC000\uC001\uC002\uABCD\uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('      \uC000\uC001\uC002\uA000\uB000\uFfFfD');
	strTest = '      \uC000\uC001\uC002\uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('          \uC000\uC001\uC002       \uA000\uB000\uFfFfD');
	strTest = '          \uC000\uC001\uC002       \uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('          D\uC000\uC001\uC002       \uA000\uB000\uFfFfD');
	strTest = '          D\uC000\uC001\uC002       \uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001 \uC000\uC001\uC002\uA000\uB000\uFfFfD');
	strTest = '\uC000\uC001 \uC000\uC001\uC002\uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001      \uC000\uC001\uC002\uA000\uB000\uFfFfD');
	strTest = '\uC000\uC001      \uC000\uC001\uC002\uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001\uC000\uC001\uC002 \uA000\uB000\uFfFfD      ');
	strTest = '\uC000\uC001\uC000\uC001\uC002 \uA000\uB000\uFfFfD      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001	\uC000\uC001\uC002\uABCD \uA000\uB000\uFfFfD      ');
	strTest = '\uC000\uC001	\uC000\uC001\uC002\uABCD \uA000\uB000\uFfFfD      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001          \uC000\uC001\uC002   \uA000\uB000\uFfFfD    ');
	strTest = '\uC000\uC001          \uC000\uC001\uC002   \uA000\uB000\uFfFfD    ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001\uABCD          D\uC000\uC001\uC002\uABCD  \uA000\uB000\uFfFfD     ');
	strTest = '\uC000\uC001\uABCD          D\uC000\uC001\uC002\uABCD  \uA000\uB000\uFfFfD     ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 10 Failure - Slightly more complex strings');


	objExp = new Array('              \uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001 \uC002 D\uC000\uC001\uC002\ufAaD\uA000\uB000\uFfFfD  \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uC000\uC001 \uC002 D\uC000\uC001\uC002\ufAaD\uA000\uB000\uFfFfD  \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              D\uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001');
	strTest = '              D\uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('D\uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001 \uC002            ');
	strTest = 'D\uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001 \uC002            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uABCD\uC000\uC001\uC002 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \uABCD\uC000\uC001\uC002 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uB000\uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001 D            ');
	strTest = '\uB000\uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001 D            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 11 Failure - Lowercase tests w/ multiples');


	objExp = new Array('     \uC000\uC001\uC002 \uA000\uB000\uFfFfD');
	strTest = '     \uC000\uC001\uC002 \uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('D\uC000\uC001\uC002 \uA000\uB000\uFfFfD      ');
	strTest = 'D\uC000\uC001\uC002 \uA000\uB000\uFfFfD      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('         \uC000\uC001\uC002 \uA000\uB000\uFfFfD       ');
	strTest = '         \uC000\uC001\uC002 \uA000\uB000\uFfFfD       ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('         \uC000\uC001\uC002D \uABCD\uA000\uB000\uFfFfD       ');
	strTest = '         \uC000\uC001\uC002D \uABCD\uA000\uB000\uFfFfD       ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 12 Failure - Slightly more complex strings w/ multiple finds');


	objExp = new Array('              \uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('              \uD000\uABCA\uC000\uB000\uC000\uC001\uC002\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uA000\uB000\uFfFfD\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uABCD');
	strTest = '              \uD000\uABCA\uC000\uB000\uC000\uC001\uC002\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uA000\uB000\uFfFfD\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uABCD'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uD000\uABCA\uC000\uB000\uC000\uC001\uC002\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uA000\uB000\uFfFfD\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uABCD              ');
	strTest = '\uD000\uABCA\uC000\uB000\uC000\uC001\uC002\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uA000\uB000\uFfFfD\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uABCD              '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              D\uC000\uC001\uC002\uABCD \uDBCA\uA000\uB000\uFfFfD \uABCD\uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\u1997');
	strTest = '              D\uC000\uC001\uC002\uABCD \uDBCA\uA000\uB000\uFfFfD \uABCD\uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\u1997'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\u1977\uC000\uC001\uC002\uB000 D\uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\u1977\uC000\uC001\uC002\uB000 D\uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('           D\uC000\uC001\uC002\uB000 \u1997\uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000D             ');
	strTest = '           D\uC000\uC001\uC002\uB000 \u1997\uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000D             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\u1997\uC000\uC001\uC002 \uC001\uA000\uB000\uFfFfD \uABCA\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '\u1997\uC000\uC001\uC002 \uC001\uA000\uB000\uFfFfD \uABCA\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \u1997\uC000\uC001\uC002 \uC001\uA000\uB000\uFfFfD \uABCA\uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \u1997\uC000\uC001\uC002 \uC001\uA000\uB000\uFfFfD \uABCA\uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('              \u1997\uC000\uC001\uC002\uABCA \uC001\uA000\uB000\uFfFfD\uC000 \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \u1997\uC000\uC001\uC002\uABCA \uC001\uA000\uB000\uFfFfD\uC000 \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uABCA\uC000\uC001\uC002 \ufAaD\uA000\uB000\uFfFfD \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uABCA\uC000\uC001\uC002 \ufAaD\uA000\uB000\uFfFfD \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000\uC000\uC001\uC002D \uC000\uC000\uC001\uA000\uB000\uFfFfD\uC000\uC001 \uC000\uC000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \uC000\uC000\uC001\uC002D \uC000\uC000\uC001\uA000\uB000\uFfFfD\uC000\uC001 \uC000\uC000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uDBCA\u007F\uC000\uC001\uC002D\u007F \uA000\uC000\uA000\uB000\uFfFfD\u1997 \uABCA\uFfFf\uC000\uC001\uC002\uabcd\uC000\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uDBCA\u007F\uC000\uC001\uC002D\u007F \uA000\uC000\uA000\uB000\uFfFfD\u1997 \uABCA\uFfFf\uC000\uC001\uC002\uabcd\uC000\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('           \uA000\u1977\uC000\uC001\uC002D\u007F D\uDBCA\uA000\uB000\uFfFfD \uA000\uD001\uB000\uD000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001\uD151\uD000\uD151             ');
	strTest = '           \uA000\u1977\uC000\uC001\uC002D\u007F D\uDBCA\uA000\uB000\uFfFfD \uA000\uD001\uB000\uD000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001\uD151\uD000\uD151             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 13 Failure - Lowercase /\uC000\uC001\uC002$/');

	apInitScenario(m_scen);
	regPat = /\uC000\uC001\uC002$/;
	
	objExp = new Array('\uC000\uC001\uC002\uA000\uB000\uFfFf');
	strTest = '\uC000\uC001\uC002\uA000\uB000\uFfFf';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('D\uC000\uC001\uC002\uA000\uB000\uFfFf');
	strTest = 'D\uC000\uC001\uC002\uA000\uB000\uFfFf';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('D\uC000\uC001\uC002\uABCD\uA000\uB000\uFfFfD');
	strTest = 'D\uC000\uC001\uC002\uABCD\uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001\uC002\uABCD\uA000\uB000\uFfFfD');
	strTest = '\uC000\uC001\uC002\uABCD\uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('      \uC000\uC001\uC002\uA000\uB000\uFfFfD');
	strTest = '      \uC000\uC001\uC002\uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001\uC002       \uA000\uB000\uFfFfD');
	strTest = '\uC000\uC001\uC002       \uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('D\uC000\uC001\uC002       \uA000\uB000\uFfFfD');
	strTest = 'D\uC000\uC001\uC002       \uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('          \uC000\uC001\uC002       \uA000\uB000\uFfFfD');
	strTest = '          \uC000\uC001\uC002       \uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('          D\uC000\uC001\uC002       \uA000\uB000\uFfFfD');
	strTest = '          D\uC000\uC001\uC002       \uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001 \uC000\uC001\uC002\uA000\uB000\uFfFfD');
	strTest = '\uC000\uC001 \uC000\uC001\uC002\uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001      \uC000\uC001\uC002\uA000\uB000\uFfFfD');
	strTest = '\uC000\uC001      \uC000\uC001\uC002\uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001\uC000\uC001\uC002 \uA000\uB000\uFfFfD      ');
	strTest = '\uC000\uC001\uC000\uC001\uC002 \uA000\uB000\uFfFfD      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001	\uC000\uC001\uC002\uABCD \uA000\uB000\uFfFfD      ');
	strTest = '\uC000\uC001	\uC000\uC001\uC002\uABCD \uA000\uB000\uFfFfD      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001          \uC000\uC001\uC002   \uA000\uB000\uFfFfD    ');
	strTest = '\uC000\uC001          \uC000\uC001\uC002   \uA000\uB000\uFfFfD    ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001\uABCD          D\uC000\uC001\uC002\uABCD  \uA000\uB000\uFfFfD     ');
	strTest = '\uC000\uC001\uABCD          D\uC000\uC001\uC002\uABCD  \uA000\uB000\uFfFfD     ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 14 Failure - Slightly more complex strings');

	objExp = new Array('\uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '\uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001 \uC002 D\uC000\uC001\uC002\ufAaD\uA000\uB000\uFfFfD  \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uC000\uC001 \uC002 D\uC000\uC001\uC002\ufAaD\uA000\uB000\uFfFfD  \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              D\uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001');
	strTest = '              D\uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('D\uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001 \uC002            ');
	strTest = 'D\uC000\uC001\uC002\uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001 \uC002            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uABCD\uC000\uC001\uC002 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \uABCD\uC000\uC001\uC002 \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uB000\uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001 D            ');
	strTest = '\uB000\uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001 \uC002 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001 D            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 15 Failure - Lowercase tests w/ multiples');

	objExp = new Array('\uC000\uC001\uC002 \uA000\uB000\uFfFfD');
	strTest = '\uC000\uC001\uC002 \uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('     \uC000\uC001\uC002 \uA000\uB000\uFfFfD');
	strTest = '     \uC000\uC001\uC002 \uA000\uB000\uFfFfD';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001\uC002 \uA000\uB000\uFfFfD      ');
	strTest = '\uC000\uC001\uC002 \uA000\uB000\uFfFfD      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('D\uC000\uC001\uC002 \uA000\uB000\uFfFfD      ');
	strTest = 'D\uC000\uC001\uC002 \uA000\uB000\uFfFfD      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('         \uC000\uC001\uC002 \uA000\uB000\uFfFfD       ');
	strTest = '         \uC000\uC001\uC002 \uA000\uB000\uFfFfD       ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('         \uC000\uC001\uC002D \uABCD\uA000\uB000\uFfFfD       ');
	strTest = '         \uC000\uC001\uC002D \uABCD\uA000\uB000\uFfFfD       ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 16 Failure - Slightly more complex strings w/ multiple finds');

	objExp = new Array('\uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '\uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uD000\uABCA\uC000\uB000\uC000\uC001\uC002\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uA000\uB000\uFfFfD\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uABCD');
	strTest = '              \uD000\uABCA\uC000\uB000\uC000\uC001\uC002\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uA000\uB000\uFfFfD\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uABCD'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uD000\uABCA\uC000\uB000\uC000\uC001\uC002\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uA000\uB000\uFfFfD\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uABCD              ');
	strTest = '\uD000\uABCA\uC000\uB000\uC000\uC001\uC002\uC000\uC001\uD000 \u1997\u9999\uABCD\uB000\uA000\uB000\uFfFfD\u1977\u00AC\uABCDD\uABCA\uC000\uD151 \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\uABCD              '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              D\uC000\uC001\uC002\uABCD \uDBCA\uA000\uB000\uFfFfD \uABCD\uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\u1997');
	strTest = '              D\uC000\uC001\uC002\uABCD \uDBCA\uA000\uB000\uFfFfD \uABCD\uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000\u1997'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\u1977\uC000\uC001\uC002\uB000 D\uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\u1977\uC000\uC001\uC002\uB000 D\uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('           D\uC000\uC001\uC002\uB000 \u1997\uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000D             ');
	strTest = '           D\uC000\uC001\uC002\uB000 \u1997\uA000\uB000\uFfFfD \uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000D             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\u1997\uC000\uC001\uC002 \uC001\uA000\uB000\uFfFfD \uABCA\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '\u1997\uC000\uC001\uC002 \uC001\uA000\uB000\uFfFfD \uABCA\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \u1997\uC000\uC001\uC002 \uC001\uA000\uB000\uFfFfD \uABCA\uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \u1997\uC000\uC001\uC002 \uC001\uA000\uB000\uFfFfD \uABCA\uC000\uC001\uABCA\uC000\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uC000\uC001\uC002 \uA000\uB000\uFfFfD \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \u1997\uC000\uC001\uC002\uABCA \uC001\uA000\uB000\uFfFfD\uC000 \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \u1997\uC000\uC001\uC002\uABCA \uC001\uA000\uB000\uFfFfD\uC000 \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uABCA\uC000\uC001\uC002 \ufAaD\uA000\uB000\uFfFfD \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uABCA\uC000\uC001\uC002 \ufAaD\uA000\uB000\uFfFfD \uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000\uC000\uC001\uC002D \uC000\uC000\uC001\uA000\uB000\uFfFfD\uC000\uC001 \uC000\uC000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000');
	strTest = '              \uC000\uC000\uC001\uC002D \uC000\uC000\uC001\uA000\uB000\uFfFfD\uC000\uC001 \uC000\uC000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uDBCA\u007F\uC000\uC001\uC002D\u007F \uA000\uC000\uA000\uB000\uFfFfD\u1997 \uABCA\uFfFf\uC000\uC001\uC002\uabcd\uC000\uC000D\uC000\uC001\uABCA\uC000             ');
	strTest = '\uDBCA\u007F\uC000\uC001\uC002D\u007F \uA000\uC000\uA000\uB000\uFfFfD\u1997 \uABCA\uFfFf\uC000\uC001\uC002\uabcd\uC000\uC000D\uC000\uC001\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('           \uA000\u1977\uC000\uC001\uC002D\u007F D\uDBCA\uA000\uB000\uFfFfD \uA000\uD001\uB000\uD000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001\uD151\uD000\uD151             ');
	strTest = '           \uA000\u1977\uC000\uC001\uC002D\u007F D\uDBCA\uA000\uB000\uFfFfD \uA000\uD001\uB000\uD000\uC000\uC001\uC002\uC000D\uC000\uC001\uABCA\uC000\uC001\uD151\uD000\uD151             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

/*****************************************************************************/
	apEndTest();
}


spt_226();


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
