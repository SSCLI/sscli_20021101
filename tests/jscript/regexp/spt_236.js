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


var iTestID = 53529;

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



function spt_236() {
	

apInitTest("spt_236");


	m_scen = ('Test 1 nonalpha  /^\uC000\uC002/');

	apInitScenario(m_scen);
	regPat = /^\uC000\uC002/;
	regExp = new Array();
	
	objExp = new Array();
	strTest = '\uC000\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('d');
	strTest = '\uC000\uC002d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('d                         ');
	strTest = '\uC000\uC002d                         ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('       ');
	strTest = '\uC000\uC002       ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end
	

	m_scen = ('Test 2 Slightly more complex strings');

	objExp = new Array(' \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '\uC000\uC002 \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	
	objExp = new Array(' \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '\uC000\uC002 \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end
	

	objExp = new Array('d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000 \uC002            ');
	strTest = '\uC000\uC002d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000 \uC002            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end
	
	
	m_scen = ('Test 3 nonalpha  tests w/ multiples');

	objExp = new Array(' \uC000\uC002');
	strTest = '\uC000\uC002 \uC000\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array(' \uC000\uC002      ');
	strTest = '\uC000\uC002 \uC000\uC002      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('d \uC000\uC002      ');
	strTest = '\uC000\uC002d \uC000\uC002      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 4 Slightly more complex strings w/ multiple finds');

	objExp = new Array(' \uC000\uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '\uC000\uC002 \uC000\uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array(' \uC000\uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '\uC000\uC002 \uC000\uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000} � ,\uC000\uC002\uC000	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000              ');
	strTest = '\uC000\uC002\uC000} � ,\uC000\uC002\uC000	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000              '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array(', d\uC000\uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '\uC000\uC002, d\uC000\uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array(' \uC000\uC002 \uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '\uC000\uC002 \uC000\uC002 \uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('d \uC000\uC000\uC000\uC002\uC000 \uC000\uC000\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '\uC000\uC002d \uC000\uC000\uC000\uC002\uC000 \uC000\uC000\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 5 nonalpha  /\uC000\uC002$/');

	apInitScenario(m_scen);
	regPat = /\uC000\uC002$/;
	regExp = new Array();

	objExp = new Array();
	strTest = '\uC000\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('d');
	strTest = 'd\uC000\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('      ');
	strTest = '      \uC000\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000 ');
	strTest = '\uC000 \uC000\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000      ');
	strTest = '\uC000      \uC000\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 6 Slightly more complex strings');

	objExp = new Array('\uC000,\uC002 \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA');
	strTest = '\uC000,\uC002 \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000,\uC002 \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA');
	strTest = '              \uC000,\uC002 \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 7 nonalpha  tests w/ multiples');

	objExp = new Array('\uC000\uC002 ');
	strTest = '\uC000\uC002 \uC000\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('     \uC000\uC002 ');
	strTest = '     \uC000\uC002 \uC000\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('  \uC000             \uC002       \uC000\uC002                  ');
	strTest = '  \uC000             \uC002       \uC000\uC002                  \uC000\uC002';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 8 Slightly more complex strings w/ multiple finds');

	objExp = new Array('\uC000 \uC000\uC002 \uC000d \uC000� \n\n \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA');
	strTest = '\uC000 \uC000\uC002 \uC000d \uC000� \n\n \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('			\uC000\uC002 \uC000d \uC000� \n\n \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA');
	strTest = '			\uC000\uC002 \uC000d \uC000� \n\n \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              }\uABCA\uC000,\uC000\uC002\uC000} � ,\uC000\uC002\uC000	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA');
	strTest = '              }\uABCA\uC000,\uC000\uC002\uC000} � ,\uC000\uC002\uC000	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              d\uC000\uC002 \r\uC000\uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA');
	strTest = '              d\uC000\uC002 \r\uC000\uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC000\uC002d \uC000\uC000\uC000\uC002\uC000 \uC000\uC000\uC000\uC002\uC000d\uC000\uABCA\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000');
	strTest = '\uC000\uC000\uC002d \uC000\uC000\uC000\uC002\uC000 \uC000\uC000\uC000\uC002\uC000d\uC000\uABCA\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC000\uC002'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 9 Failure - Lowercase /^\uC000\uC002 /');

	apInitScenario(m_scen);
	regPat = /^\uC000\uC002/;

	objExp = new Array(' \uC000\uC002{,~');
	strTest = ' \uC000\uC002{,~';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('d\uC000\uC002{,~');
	strTest = 'd\uC000\uC002{,~';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('d\uC000\uC002{,~d');
	strTest = 'd\uC000\uC002{,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('                                       \uC000\uC002{,~d');
	strTest = '                                       \uC000\uC002{,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('      \uC000\uC002{,~d');
	strTest = '      \uC000\uC002{,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('          \uC000\uC002       {,~d');
	strTest = '          \uC000\uC002       {,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('          d\uC000\uC002       {,~d');
	strTest = '          d\uC000\uC002       {,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000 \uC000\uC002{,~d');
	strTest = '\uC000 \uC000\uC002{,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000      \uC000\uC002{,~d');
	strTest = '\uC000      \uC000\uC002{,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC000\uC002 {,~d      ');
	strTest = '\uC000\uC000\uC002 {,~d      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000	\uC000\uC002 {,~d      ');
	strTest = '\uC000	\uC000\uC002 {,~d      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000          \uC000\uC002   {,~d    ');
	strTest = '\uC000          \uC000\uC002   {,~d    ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000          d\uC000\uC002  {,~d     ');
	strTest = '\uC000          d\uC000\uC002  {,~d     ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 10 Failure - Slightly more complex strings');


	objExp = new Array('              \uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              \uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000 \uC002 d\uC000\uC002?{,~d  \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '\uC000 \uC002 d\uC000\uC002?{,~d  \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              d\uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              d\uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('d\uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000 \uC002            ');
	strTest = 'd\uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000 \uC002            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000\uC002 \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              \uC000\uC002 \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array(',\uC000\uC002 {,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000 d            ');
	strTest = ',\uC000\uC002 {,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000 d            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 11 Failure - Lowercase tests w/ multiples');


	objExp = new Array('     \uC000\uC002 {,~d');
	strTest = '     \uC000\uC002 {,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('d\uC000\uC002 {,~d      ');
	strTest = 'd\uC000\uC002 {,~d      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('         \uC000\uC002 {,~d       ');
	strTest = '         \uC000\uC002 {,~d       ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('         \uC000\uC002d {,~d       ');
	strTest = '         \uC000\uC002d {,~d       ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 12 Failure - Slightly more complex strings w/ multiple finds');


	objExp = new Array('              \uC000\uC002 {,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              \uC000\uC002 {,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('              }\uABCA\uC000,\uC000\uC002\uC000} � ,{,~d	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              }\uABCA\uC000,\uC000\uC002\uC000} � ,{,~d	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('}\uABCA\uC000,\uC000\uC002\uC000} � ,{,~d	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000              ');
	strTest = '}\uABCA\uC000,\uC000\uC002\uC000} � ,{,~d	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000              '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              d\uC000\uC002 \r{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000�');
	strTest = '              d\uC000\uC002 \r{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000�'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('	\uC000\uC002, d{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '	\uC000\uC002, d{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('           d\uC000\uC002, �{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000d             ');
	strTest = '           d\uC000\uC002, �{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000d             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('�\uC000\uC002 {,~d \uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '�\uC000\uC002 {,~d \uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              �\uC000\uC002 {,~d \uABCA\uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              �\uC000\uC002 {,~d \uABCA\uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('              �\uC000\uC002\uABCA {,~d\uC000 \uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              �\uC000\uC002\uABCA {,~d\uC000 \uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uABCA\uC000\uC002 ?{,~d \uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '\uABCA\uC000\uC002 ?{,~d \uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000\uC000\uC002d \uC000\uC000{,~d\uC000 \uC000\uC000\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              \uC000\uC000\uC002d \uC000\uC000{,~d\uC000 \uC000\uC000\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\r\uC000\uC002d {\uC000{,~d� \uABCA~\uC000\uC002@\uC000\uC000d\uC000\uABCA\uC000             ');
	strTest = '\r\uC000\uC002d {\uC000{,~d� \uABCA~\uC000\uC002@\uC000\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('           {	\uC000\uC002d d\r{,~d {*,}\uC000\uC002\uC000d\uC000\uABCA\uC000.}.             ');
	strTest = '           {	\uC000\uC002d d\r{,~d {*,}\uC000\uC002\uC000d\uC000\uABCA\uC000.}.             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 13 Failure - Lowercase /\uC000\uC002$/');

	apInitScenario(m_scen);
	regPat = /\uC000\uC002$/;
	
	objExp = new Array('\uC000\uC002{,~');
	strTest = '\uC000\uC002{,~';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('d\uC000\uC002{,~');
	strTest = 'd\uC000\uC002{,~';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	objExp = new Array('d\uC000\uC002{,~d');
	strTest = 'd\uC000\uC002{,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC002{,~d');
	strTest = '\uC000\uC002{,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('      \uC000\uC002{,~d');
	strTest = '      \uC000\uC002{,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC002       {,~d');
	strTest = '\uC000\uC002       {,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('d\uC000\uC002       {,~d');
	strTest = 'd\uC000\uC002       {,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('          \uC000\uC002       {,~d');
	strTest = '          \uC000\uC002       {,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('          d\uC000\uC002       {,~d');
	strTest = '          d\uC000\uC002       {,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000 \uC000\uC002{,~d');
	strTest = '\uC000 \uC000\uC002{,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000      \uC000\uC002{,~d');
	strTest = '\uC000      \uC000\uC002{,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC000\uC002 {,~d      ');
	strTest = '\uC000\uC000\uC002 {,~d      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000	\uC000\uC002 {,~d      ');
	strTest = '\uC000	\uC000\uC002 {,~d      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000          \uC000\uC002   {,~d    ');
	strTest = '\uC000          \uC000\uC002   {,~d    ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000          d\uC000\uC002  {,~d     ');
	strTest = '\uC000          d\uC000\uC002  {,~d     ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 14 Failure - Slightly more complex strings');

	objExp = new Array('\uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '\uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              \uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000 \uC002 d\uC000\uC002?{,~d  \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '\uC000 \uC002 d\uC000\uC002?{,~d  \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              d\uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              d\uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('d\uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000 \uC002            ');
	strTest = 'd\uC000\uC002{,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000 \uC002            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000\uC002 \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              \uC000\uC002 \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array(',\uC000\uC002 {,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000 d            ');
	strTest = ',\uC000\uC002 {,~d \uC000 \uC002 \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000 d            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end


	m_scen = ('Test 15 Failure - Lowercase tests w/ multiples');

	objExp = new Array('\uC000\uC002 {,~d');
	strTest = '\uC000\uC002 {,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('     \uC000\uC002 {,~d');
	strTest = '     \uC000\uC002 {,~d';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC002 {,~d      ');
	strTest = '\uC000\uC002 {,~d      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('d\uC000\uC002 {,~d      ');
	strTest = 'd\uC000\uC002 {,~d      ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('         \uC000\uC002 {,~d       ');
	strTest = '         \uC000\uC002 {,~d       ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('         \uC000\uC002d {,~d       ');
	strTest = '         \uC000\uC002d {,~d       ';
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	m_scen = ('Test 16 Failure - Slightly more complex strings w/ multiple finds');

	objExp = new Array('\uC000\uC002 {,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '\uC000\uC002 {,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000\uC002 {,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              \uC000\uC002 {,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC002 {,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '\uC000\uC002 {,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              }\uABCA\uC000,\uC000\uC002\uC000} � ,{,~d	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              }\uABCA\uC000,\uC000\uC002\uC000} � ,{,~d	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('}\uABCA\uC000,\uC000\uC002\uC000} � ,{,~d	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000              ');
	strTest = '}\uABCA\uC000,\uC000\uC002\uC000} � ,{,~d	\nd\uABCA\uC000. \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000              '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              d\uC000\uC002 \r{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000�');
	strTest = '              d\uC000\uC002 \r{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000�'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('	\uC000\uC002, d{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '	\uC000\uC002, d{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('           d\uC000\uC002, �{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000d             ');
	strTest = '           d\uC000\uC002, �{,~d \uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000d             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('�\uC000\uC002 {,~d \uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '�\uC000\uC002 {,~d \uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              �\uC000\uC002 {,~d \uABCA\uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              �\uC000\uC002 {,~d \uABCA\uC000\uABCA\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uC000\uC002 {,~d \uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '\uC000\uC002 {,~d \uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              �\uC000\uC002\uABCA {,~d\uC000 \uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              �\uC000\uC002\uABCA {,~d\uC000 \uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\uABCA\uC000\uC002 ?{,~d \uC000\uC002\uC000d\uC000\uABCA\uC000             ');
	strTest = '\uABCA\uC000\uC002 ?{,~d \uC000\uC002\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('              \uC000\uC000\uC002d \uC000\uC000{,~d\uC000 \uC000\uC000\uC000\uC002\uC000d\uC000\uABCA\uC000');
	strTest = '              \uC000\uC000\uC002d \uC000\uC000{,~d\uC000 \uC000\uC000\uC000\uC002\uC000d\uC000\uABCA\uC000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('\r\uC000\uC002d {\uC000{,~d� \uABCA~\uC000\uC002@\uC000\uC000d\uC000\uABCA\uC000             ');
	strTest = '\r\uC000\uC002d {\uC000{,~d� \uABCA~\uC000\uC002@\uC000\uC000d\uC000\uABCA\uC000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

	objExp = new Array('           {	\uC000\uC002d d\r{,~d {*,}\uC000\uC002\uC000d\uC000\uABCA\uC000.}.             ');
	strTest = '           {	\uC000\uC002d d\r{,~d {*,}\uC000\uC002\uC000d\uC000\uABCA\uC000.}.             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.split(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.split(regPat));
	@end

/*****************************************************************************/
	apEndTest();
}


spt_236();


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
