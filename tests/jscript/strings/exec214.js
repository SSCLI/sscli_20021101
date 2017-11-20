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


var iTestID = 52619;

//This set of tests verifies that \w works in various circumstances with lowercase

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



function exec214() {
	

apInitTest("Exec214");


	m_scen = ('Test 1 nonalpha  /\\w{3}/');

sCat = "/\\w{3}/ ";
	apInitScenario(m_scen);
regPat = /\w{3}/;
objExp = new Array('___');
var regExp = new Array();

var strTest = '___';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end


strTest = '      ___';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '___       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '          ___       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

	m_scen = ('Test 5 nonalpha  /(\\w)/');

sCat = "/(\\w)/ ";
	apInitScenario(m_scen);
regPat = /(\w)/;
objExp = new Array('_', '_');
regExp = new Array('_');

strTest = '_';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '      _';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '_       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '_       |';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '          _       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '          _       |';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = ' _';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '      _';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '    _       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '	_       |';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '          _       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '          _       |';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

	m_scen = ('Test 6 Slightly more complex strings');

strTest = '   $\\$_'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                 $\\$_'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '   $\\$_             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|                 $\\$_|'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|   $\\_$             |'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|                 $\\_$'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '   $\\_$             |'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

	m_scen = ('Test 7 nonalpha  tests /(\\w{3})/ multiples');

strTest = ' _';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '      _';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = ' _      ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = ' _      |';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '          _       ';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '          _       |';
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end


	m_scen = ('Test 8 Slightly more complex strings w/ multiple finds');

strTest = '  $\\_$'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                $_\\$'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '  $\\_             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|                _$\\$|'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|  $_\\$             |'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|               _ $\\$'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '  $_\\$             |'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|             $\\_$             |'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = ' _ \\$'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '              _  $\\$'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '_  \\$             '; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|            _    \\$|'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|  \\$      _       |'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|              _  \\$'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '  \\_$             |'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|            _ \\$             |'; 
ArrayEqual(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end


	m_scen = ('Test 33 Failure Lowercase /\\w{27}/');

sCat = "/\\w{27}/ ";
	apInitScenario(m_scen);
regPat = /\w{27}/;
objExp = null;
//no new regExp due to spec of RegExp.$1-9
//this set of tests smakes sure that RegExp.$ keep original values


strTest = '';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '      ';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '       ';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '       |';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                 ';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                 |';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = ' ';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '      ';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '       ';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '	       |';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                 ';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                 |';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

	m_scen = ('Test 34 Failure-Slightly more complex strings');

verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                 $\\$'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '   $\\$             '; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|                 $\\$|'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|   $\\$             |'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|                 $\\$'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '   $\\$             |'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end


	m_scen = ('Test 35 Failure Lowercase tests /\\w{27}/ multiples');

strTest = ' ';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '      ';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '       ';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '       |';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                 ';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                 |';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

	m_scen = ('Test 36 Failure - Slightly more complex strings w/ multiple finds');

strTest = '  $\\$'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                $\\$'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '  $\\$             '; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|                $\\$|'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|  $\\$             |'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|                $\\$'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '  $\\$             |'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|             $\\$             |'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '  \\$'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                $\\$'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '  \\$             '; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|                \\$|'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|  \\$             |'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|                \\$'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '  \\$             |'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '|             \\$             |'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end


strTest = '											'; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = '                        '; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

strTest = ''; 
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end

	m_scen = ('Test 37 Failure Lowercase /\\w/');

sCat = "/\\w/ ";
	apInitScenario(m_scen);
regPat = /\w/;
objExp = null;
//no new regExp due to spec of RegExp.$1-9
//this set of tests smakes sure that RegExp.$ keep original values

strTest = '';
verify(sCat+strTest, objExp, regPat.exec(strTest));
@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, regPat.exec(strTest));
	@end
	@end




/*****************************************************************************/
	apEndTest();
}


exec214();


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
