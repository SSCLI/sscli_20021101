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


var iTestID = 53277;

	var sCat = '';
	var regPat = "";
	var objExp = "";
	var regExp = "";	
	var strTest = "";
	var m_scen = '';
	var strTemp = "";

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





function rep_0134() {
	
  @if(@_fast)
    var i, strTest, regExp;
  @end

apInitTest("rep_0134");


	m_scen = ('Test 1 Lowercase /abc/gi');

	sCat = "/abc/gi ";
	apInitScenario(m_scen);
	regPat = /abc/gi;
	regExp = new Array();

	objExp = 'a';

	strTest = 'abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end


	objExp = '      a';

	strTest = '      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a       ';

	strTest = 'abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a       |';

	strTest = 'abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '          a       ';

	strTest = '          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '          a       |';

	strTest = '          abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab a';

	strTest = 'ab abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab      a';

	strTest = 'ab      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'aba       ';

	strTest = 'ababc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab	a       |';

	strTest = 'ab	abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab          a       ';

	strTest = 'ab          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab          a       |';

	strTest = 'ab          abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	m_scen = ('Test 2 Slightly more complex strings');

	objExp = 'a ab c abracadabra'; 

	strTest = 'abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a ab c abracadabra'; 

	strTest = '              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a ab c abracadabra             '; 

	strTest = 'abc ab c abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a ab c abracadabra|'; 

	strTest = '|              abc ab c abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a ab c abracadabra             |'; 

	strTest = '|abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a ab c abracadabra'; 

	strTest = '|              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a ab c abracadabra             |'; 

	strTest = 'abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end


	m_scen = ('Test 3 Lowercase tests /abc/gi multiples');

	objExp = 'a a';

	strTest = 'abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '     a a';

	strTest = '     abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a      ';

	strTest = 'abc abc      ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a      |';

	strTest = 'abc abc      |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '         a a       ';

	strTest = '         abc abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '         a a       |';

	strTest = '         abc abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	m_scen = ('Test 4 Slightly more complex strings w/ multiple finds');

	objExp = 'a a abracadabra'; 

	strTest = 'abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a a abracadabra'; 

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a abracadabra             '; 

	strTest = 'abc abc abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a abracadabra|'; 

	strTest = '|              abc abc abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a a abracadabra             |'; 

	strTest = '|abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a abracadabra'; 

	strTest = '|              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a abracadabra             |'; 

	strTest = 'abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|           a a abracadabra             |'; 

	strTest = '|           abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a aadabra'; 

	strTest = 'abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a a abracadabra'; 

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a aadabra             '; 

	strTest = 'abc abc abcadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a aadabra|'; 

	strTest = '|              abc abc abcadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a a aadabra             |'; 

	strTest = '|abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a aadabra'; 

	strTest = '|              abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a aadabra             |'; 

	strTest = 'abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|           a a aadabra             |'; 

	strTest = '|           abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	m_scen = ('Test 5 Lowercase /(abc)/gi');

	sCat = "/(abc)/gi ";
	apInitScenario(m_scen);
	regPat = /(abc)/gi;
	regExp = new Array('abc');

	objExp = 'a';

	strTest = 'abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '      a';

	strTest = '      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a       ';

	strTest = 'abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a       |';

	strTest = 'abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '          a       ';

	strTest = '          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '          a       |';

	strTest = '          abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab a';

	strTest = 'ab abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab      a';

	strTest = 'ab      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'aba       ';

	strTest = 'ababc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab	a       |';

	strTest = 'ab	abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab          a       ';

	strTest = 'ab          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab          a       |';

	strTest = 'ab          abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	m_scen = ('Test 6 Slightly more complex strings');

	objExp = 'a ab c abracadabra'; 

	strTest = 'abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a ab c abracadabra'; 

	strTest = '              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a ab c abracadabra             '; 

	strTest = 'abc ab c abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a ab c abracadabra|'; 

	strTest = '|              abc ab c abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a ab c abracadabra             |'; 

	strTest = '|abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a ab c abracadabra'; 

	strTest = '|              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a ab c abracadabra             |'; 

	strTest = 'abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	m_scen = ('Test 7 Lowercase tests /(abc)/gi multiples');

	objExp = 'a a';

	strTest = 'abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '     a a';

	strTest = '     abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a      ';

	strTest = 'abc abc      ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a      |';

	strTest = 'abc abc      |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '         a a       ';

	strTest = '         abc abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '         a a       |';

	strTest = '         abc abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end


	m_scen = ('Test 8 Slightly more complex strings w/ multiple finds');

	objExp = 'a a abracadabra'; 

	strTest = 'abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a a abracadabra'; 

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a abracadabra             '; 

	strTest = 'abc abc abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a abracadabra|'; 

	strTest = '|              abc abc abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a a abracadabra             |'; 

	strTest = '|abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a abracadabra'; 

	strTest = '|              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a abracadabra             |'; 

	strTest = 'abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|           a a abracadabra             |'; 

	strTest = '|           abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a aadabra'; 

	strTest = 'abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a a abracadabra'; 

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a aadabra             '; 

	strTest = 'abc abc abcadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a aadabra|'; 

	strTest = '|              abc abc abcadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a a aadabra             |'; 

	strTest = '|abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a aadabra'; 

	strTest = '|              abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a aadabra             |'; 

	strTest = 'abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|           a a aadabra             |'; 

	strTest = '|           abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	m_scen = ('Test 9 Lowercase /(ab)(c)/gi');

	sCat = "/(ab)(c)/gi ";
	apInitScenario(m_scen);
	regPat = /(ab)(c)/gi;
	regExp = new Array('ab','c');

	objExp = 'a';

	strTest = 'abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '      a';

	strTest = '      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a       ';

	strTest = 'abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a       |';

	strTest = 'abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '          a       ';

	strTest = '          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '          a       |';

	strTest = '          abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab a';

	strTest = 'ab abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab      a';

	strTest = 'ab      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'aba       ';

	strTest = 'ababc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab	a       |';

	strTest = 'ab	abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab          a       ';

	strTest = 'ab          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab          a       |';

	strTest = 'ab          abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end


	m_scen = ('Test 10 Slightly more complex strings');

	objExp = 'a ab c abracadabra'; 

	strTest = 'abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a ab c abracadabra'; 

	strTest = '              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a ab c abracadabra             '; 

	strTest = 'abc ab c abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a ab c abracadabra|'; 

	strTest = '|              abc ab c abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a ab c abracadabra             |'; 

	strTest = '|abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a ab c abracadabra'; 

	strTest = '|              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a ab c abracadabra             |'; 

	strTest = 'abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end


	m_scen = ('Test 11 Lowercase tests /(ab)(c)/gi multiples');

	objExp = 'a a';

	strTest = 'abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '     a a';

	strTest = '     abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a      ';

	strTest = 'abc abc      ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a      |';

	strTest = 'abc abc      |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '         a a       ';

	strTest = '         abc abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '         a a       |';

	strTest = '         abc abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	m_scen = ('Test 12 Slightly more complex strings w/ multiple finds');

	objExp = 'a a abracadabra'; 

	strTest = 'abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a a abracadabra'; 

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a abracadabra             '; 

	strTest = 'abc abc abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a abracadabra|'; 

	strTest = '|              abc abc abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a a abracadabra             |'; 

	strTest = '|abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a abracadabra'; 

	strTest = '|              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a abracadabra             |'; 

	strTest = 'abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|           a a abracadabra             |'; 

	strTest = '|           abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end


	objExp = 'a a aadabra'; 

	strTest = 'abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a a abracadabra'; 

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a aadabra             '; 

	strTest = 'abc abc abcadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a aadabra|'; 

	strTest = '|              abc abc abcadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a a aadabra             |'; 

	strTest = '|abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a aadabra'; 

	strTest = '|              abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a aadabra             |'; 

	strTest = 'abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|           a a aadabra             |'; 

	strTest = '|           abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	m_scen = ('Test 13 Lowercase /((ab)(c))/gi');

	sCat = "/((ab)(c))/gi ";
	apInitScenario(m_scen);
	regPat = /((ab)(c))/gi;
	regExp = new Array('abc','ab','c');

	objExp = 'a';

	strTest = 'abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '      a';

	strTest = '      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a       ';

	strTest = 'abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a       |';

	strTest = 'abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '          a       ';

	strTest = '          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '          a       |';

	strTest = '          abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab a';

	strTest = 'ab abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab      a';

	strTest = 'ab      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'aba       ';

	strTest = 'ababc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab	a       |';

	strTest = 'ab	abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab          a       ';

	strTest = 'ab          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'ab          a       |';

	strTest = 'ab          abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end


	m_scen = ('Test 14 Slightly more complex strings');

	objExp = 'a ab c abracadabra'; 

	strTest = 'abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a ab c abracadabra'; 

	strTest = '              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a ab c abracadabra             '; 

	strTest = 'abc ab c abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a ab c abracadabra|'; 

	strTest = '|              abc ab c abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a ab c abracadabra             |'; 

	strTest = '|abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a ab c abracadabra'; 

	strTest = '|              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a ab c abracadabra             |'; 

	strTest = 'abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end


	m_scen = ('Test 15 Lowercase tests //((ab)(c))/gi multiples');

	objExp = 'a a';

	strTest = 'abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '     a a';

	strTest = '     abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a      ';

	strTest = 'abc abc      ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a      |';

	strTest = 'abc abc      |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '         a a       ';

	strTest = '         abc abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '         a a       |';

	strTest = '         abc abc       |';
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end


	m_scen = ('Test 16 Slightly more complex strings w/ multiple finds');

	objExp = 'a a abracadabra'; 

	strTest = 'abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a a abracadabra'; 

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a abracadabra             '; 

	strTest = 'abc abc abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a abracadabra|'; 

	strTest = '|              abc abc abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a a abracadabra             |'; 

	strTest = '|abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a abracadabra'; 

	strTest = '|              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a abracadabra             |'; 

	strTest = 'abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|           a a abracadabra             |'; 

	strTest = '|           abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end


	objExp = 'a a aadabra'; 

	strTest = 'abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '              a a abracadabra'; 

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a aadabra             '; 

	strTest = 'abc abc abcadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a aadabra|'; 

	strTest = '|              abc abc abcadabra|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|a a aadabra             |'; 

	strTest = '|abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|              a a aadabra'; 

	strTest = '|              abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = 'a a aadabra             |'; 

	strTest = 'abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end

	objExp = '|           a a aadabra             |'; 

	strTest = '|           abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.replace(regPat, "a"));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.replace(regPat, "a"));
	@end


/*****************************************************************************/
	apEndTest();
}


rep_0134();


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
