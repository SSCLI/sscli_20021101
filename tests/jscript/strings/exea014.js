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


var iTestID = 53201;

//This set of tests verifies that \w works in various circumstances with lowercase
//exec014 part2
var sCat = '';
var regPat = "";
var objExp = "";
var m_scen = '';
var strTest = "";

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
	//Makes Sure that Arrays are eq	

	if (arrayExp.length != arrayAct.length)
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



function exea014() {
	

apInitTest("exea014");

	m_scen = ('Test 21 Lowercase  /((\\w{1})(\\w{2}))/');
	sCat = "/((\\w{1})(\\w{2}))/ ";
	apInitScenario(m_scen);
	regPat = /((\w{1})(\w{2}))/;
	objExp = new Array('abc','abc','a','bc');
	var regExp = new Array('abc','a','bc');

	var strTest = 'abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '      abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '          abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '          abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab      abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abcbc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '         abcbc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab	abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab          abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab          abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 22 Slightly more complex strings');
	strTest = 'abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc ab c abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc ab c abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 23 Lowercase tests /((\\w{1})(\\w{2}))/ multiples');
	strTest = 'abc abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '     abc abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc      ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc      |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '         abc abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '         abc abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 24 Slightly more complex strings w/ multiple finds');
	strTest = 'abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|           abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	strTest = 'abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abcadabra             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abcadabra|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|           abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 25 Lowercase /(\\w{1})(\\w{1})(\\w{1})/');
	sCat = "/(\\w{1})(\\w{1})(\\w{1})/ ";
	apInitScenario(m_scen);
	regPat = /(\w{1})(\w{1})(\w{1})/;
	objExp = new Array('abc','a','b','c');
	regExp = new Array('a', 'b', 'c');

	strTest = 'abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '      abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '          abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '          abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab      abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abcbc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '          abcbc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab	abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab          abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab          abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 26 Slightly more complex strings');
	strTest = 'abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc ab c abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc ab c abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 27 Lowercase tests /(\\w{1})(\\w{1})(\\w{1})/ multiples');
	strTest = 'abc abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '     abc abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc      ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc      |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '         abc abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '         abc abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 28 Slightly more complex strings w/ multiple finds');
	strTest = 'abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|           abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	strTest = 'abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abcadabra             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abcadabra|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|           abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 29 Lowercase /((\\w{1})(\\w{1})(\\w{1}))/');
	sCat = "/((\\w{1})(\\w{1})(\\w{1}))/ ";
	apInitScenario(m_scen);
	regPat = /((\w{1})(\w{1})(\w{1}))/;
	objExp = new Array('abc','abc','a','b','c');
	regExp = new Array('abc','a','b','c');

	strTest = 'abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '      abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '          abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '          abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab      abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abcbc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abcbc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab	abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab          abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab          abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 30 Slightly more complex strings');
	strTest = 'abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc ab c abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc ab c abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc ab c abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 31 Lowercase tests /((\\w{1})(\\w{1})(\\w{1}))/ multiples');
	strTest = 'abc abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '     abc abc';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc      ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc      |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '         abc abc       ';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '         abc abc       |';
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 32 Slightly more complex strings w/ multiple finds');
	strTest = 'abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abracadabra|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|           abc abc abracadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	strTest = 'abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abcadabra             '; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abcadabra|'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abcadabra'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|           abc abc abcadabra             |'; 
	ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 33 Failure Lowercase /\\w{27}/');
	sCat = "/\\w{27}/ ";
	apInitScenario(m_scen);
	regPat = /\w{27}/;
	objExp = null;
//no new regExp due to spec of RegExp.$1-9
//this set of tests smakes sure that RegExp.$ keep original values


	strTest = 'abc';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '      abc';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc       ';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc       |';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '          abc       ';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '          abc       |';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab abc';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab      abc';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ababc       ';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab	abc       |';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab          abc       ';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'ab          abc       |';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 34 Failure-Slightly more complex strings');
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc ab c abracadabra'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc ab c abracadabra             '; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc ab c abracadabra|'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc ab c abracadabra             |'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc ab c abracadabra'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc ab c abracadabra             |'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	m_scen = ('Test 35 Failure Lowercase tests /\\w{27}/ multiples');
	strTest = 'abc abc';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '     abc abc';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc      ';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc      |';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '         abc abc       ';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '         abc abc       |';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 36 Failure - Slightly more complex strings w/ multiple finds');
	strTest = 'abc abc abracadabra'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc abc abracadabra'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abracadabra             '; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abracadabra|'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc abc abracadabra             |'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abracadabra'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abracadabra             |'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|           abc abc abracadabra             |'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abcadabra'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '              abc abc abracadabra'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abcadabra             '; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abcadabra|'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|abc abc abcadabra             |'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|              abc abc abcadabra'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abc abc abcadabra             |'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '|           abc abc abcadabra             |'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end


	strTest = '											'; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = '                        '; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = ''; 
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	m_scen = ('Test 37 Failure Lowercase /\\W/');
	sCat = "/\\W/ ";
	apInitScenario(m_scen);
	regPat = /\W/;
	objExp = null;
//no new regExp due to spec of RegExp.$1-9
//this set of tests smakes sure that RegExp.$ keep original values

	strTest = 'abc';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

	strTest = 'abcdefeghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567891011121314151617181920_';
	verify(sCat+strTest, objExp, regPat.exec(strTest));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end

/*****************************************************************************/

    apEndTest();

}


exea014();


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
