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


var iTestID = 53586;

var sCat = '';
var regPat = "";
var objExp = "";
var strTest = "";
var regExp = "";
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
{
	//redirects function call to verify
	verify(sCat1, arrayExp, arrayAct);
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



function src_018() {
	

apInitTest("src_018");


	m_scen = ('Test 1 Lowercase /\w\w\w|\W\W\W\W\W\W\W\w\W\W\W\W\W/');

	sCat = "Pattern: exist | non-exist ";
	apInitScenario(m_scen);
	regPat = /\w\w\w|\W\W\W\W\W\W\W\w\W\W\W\W\W/;
	objExp = new Array('abc');
	regExp = new Array();
	
	objExp = 0;
	strTest = 'abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end





	strTest = 'abc xvf';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 6;
	strTest = '      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	
	objExp = 10;
	strTest = '          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	
	objExp = 3;
	strTest = 'ab abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	
	objExp = 8;
	strTest = 'ab      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	
	objExp = 3;
	strTest = 'ab abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'ab	abc xvf       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 12;
	strTest = 'ab          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	m_scen = ('Test 2 Slightly more complex strings');

	objExp = 0;
	strTest = 'abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	
	objExp = 0;
	strTest = 'abc ab c abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	
	objExp = 20;
	strTest = '              a b c abc c abracadabrab'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 7;
	strTest = ' a b c abc c abracadabrab c            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	
	objExp = 14;
	strTest = '              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	
	objExp = 0;
	strTest = 'abc ab c abracadabrab d            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	m_scen = ('Test 3 Lowercase tests w/ multiples');

	strTest = 'abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 5;
	strTest = '     abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc abc      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'd abc abc xvf      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 9;
	strTest = '         abc abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '         abc xvf z abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 4 Slightly more complex strings w/ multiple finds');

	objExp = 0;
	strTest = 'abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc abc abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc xvfbooran siez abc xvfthedral abracadabrae'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc xvfbo siez abc xvfthedral abracadabrae              '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              d abc xvf z abc xvf eabracadabras'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc xvf z abc abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 11;
	strTest = '           abc xvf z abc abracadabrad             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc z abc z abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc z abc rabracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc abc abc xvfdabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              s abc xvf z abc xvf abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 9;
	strTest = ' a b c v abc abc xvfdabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              a abc xvf aaz abc xvfb az abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'fg abc xvfg mz abc xvfs rz abc xvfaadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '           mt abc xvfg dz abc xvfo mknz abc xvfdabrablol             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 5 Lowercase /\W\W\W\W\W\W\W\w\W\W\W\W\W|\w\w\w/');

	sCat = "Pattern: non-exist | exist ";
	apInitScenario(m_scen);
	regPat = /\W\W\W\W\W\W\W\w\W\W\W\W\W|\w\w\w/;
	objExp = new Array('abc');
	regExp = new Array();

	objExp = 0;
	strTest = 'abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	objExp = 2;
	strTest = 'd abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	strTest = 'd abc xvf';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc xvf';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 6;
	strTest = '      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'd abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 10;
	strTest = '          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 12;
	strTest = '          d abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'ab abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 8;
	strTest = 'ab      abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'ab abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'ab	abc xvf       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 12;
	strTest = 'ab          abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = 'ab e          z abc xvf       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 6 Slightly more complex strings');

	objExp = 0;
	strTest = 'abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc ab c abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              d abc ab c abracadabrab'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'd abc ab c abracadabrab c            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              e abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'n abc ab c abracadabrab d            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	m_scen = ('Test 7 Lowercase tests w/ multiples');

	objExp = 0;
	strTest = 'abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 5;
	strTest = '     abc abc';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc abc      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'd abc abc xvf      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 9;
	strTest = '         abc abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '         abc xvf z abc       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 8 Slightly more complex strings w/ multiple finds');

	objExp = 0;
	strTest = 'abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc abc abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc abc abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 21;
	strTest = '              or a n abc xvfbo siez abc xvfthedral abracadabrae'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 6;
	strTest = 'or an abc xvfbo siez abc xvfthedral abracadabrae              '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              d abc xvf z abc xvf eabracadabras'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 't abc xvf z abc abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 13;
	strTest = '           d abc xvf z abc abracadabrad             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 's abc z abc z abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              s abc z abc rabracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc abc abc xvfdabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              s abc xvf z abc xvf abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'r abc z abc abc xvfdabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              a abc xvf aaz abc xvfb az abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'fg abc xvfg mz abc xvfs rz abc xvfaadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '           mt abc xvfg dz abc xvfo mknz abc xvfdabrablol             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	m_scen = ('Test 9 Lowercase /\w\w\w|\w\w\w\w/');

	sCat = "Pattern: exist found | exist ";
	apInitScenario(m_scen);
	regPat = /\w\w\w|\w\w\w\w/;
	objExp = new Array('abc');
	regExp = new Array();

	objExp = 0;
	strTest = 'abc  mnzaa';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	objExp = 2;
	strTest = 'd abc mnza';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	strTest = 'd abc e mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc e mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 6;
	strTest = '      abc mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'd abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 10;
	strTest = '          abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 12;
	strTest = '          d abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'ab abc mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 8;
	strTest = 'ab      abc mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'ab abc  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'ab	abc e  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 12;
	strTest = 'ab          abc    mnzad    ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = 'ab e          d abc xvf   mnzad     ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 10 Slightly more complex strings');

	objExp = 0;
	strTest = 'abc mnzad ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc mnzad ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 7;
	strTest = 'ab c d abc xvf mnzad  abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              d abc mnzad ab c abracadabrab'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'd abc mnzad ab c abracadabrab c            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              e abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc  mnzad ab c abracadabrab d            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	m_scen = ('Test 11 Lowercase tests w/ multiples');

	objExp = 0;
	strTest = 'abc  mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 5;
	strTest = '     abc  mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 9;
	strTest = '         abc  mnzad       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '         abc xvf e mnzad       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 12 Slightly more complex strings w/ multiple finds');

	objExp = 0;
	strTest = 'abc  mnzad abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc  mnzad abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc  mnzad abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 20;
	strTest = '              or az abc xvfbo sien mnzadthedral abracadabrae'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 6;
	strTest = 'or az abc xvfbo sien mnzadthedral abracadabrae              '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc xvf f mnzad eabracadabras'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc xvf d mnzad abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 13;
	strTest = '           z abc xvf s mnzad abracadabrad             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc b mnzad z abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc b mnzad rabracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc  mnzad abc xvfdabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc xvf b mnzada abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc v mnzad abc xvfdabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc xvf aab mnzadab az abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'fz abc xvfg ma mnzads rz abc xvfaadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '           mz abc xvfg df mnzad mknz abc xvfdabrablol             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 13 Lowercase /\w\w\w\w|\w\w\w/');

	sCat = "Pattern: exist | exist found ";
	apInitScenario(m_scen);
	regPat = /\w\w\w\w|\w\w\w/;
	objExp = new Array('abc');
	regExp = new Array();

	objExp = 0;
	strTest = 'abc mnza';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	objExp = 2;
	strTest = 'z abc mnza';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	strTest = 'z abc xvf mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc xvf mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 6;
	strTest = '      abc mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 10;
	strTest = '          abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 12;
	strTest = '          z abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'ab abc mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 8;
	strTest = 'ab      abc mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'az abc  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'ab	abc xvf  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 12;
	strTest = 'ab          abc    mnzad    ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = 'ab e          z abc xvf   mnzad     ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 14 Slightly more complex strings');

	objExp = 0;
	strTest = 'abc mnzad ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc mnzad ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 7;
	strTest = 'ab c z abc xvf mnzad  abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc mnzad ab c abracadabrab'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc mnzad ab c abracadabrab c            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc  mnzad ab c abracadabrab d            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	m_scen = ('Test 15 Lowercase tests w/ multiples');

	objExp = 0;
	strTest = 'abc  mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 5;
	strTest = '     abc  mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 9;
	strTest = '         abc  mnzad       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '         abc xvf e mnzad       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 16 Slightly more complex strings w/ multiple finds');

	objExp = 0;
	strTest = 'abc  mnzad abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc  mnzad abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc  mnzad abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 21;
	strTest = '              o r az abc xvfbo sien mnzadthedral abracadabrae'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 7;
	strTest = 'o r az abc xvfbo sien mnzadthedral abracadabrae              '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc xvf f mnzad eabracadabras'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc xvf d mnzad abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 13;
	strTest = '           z abc xvf s mnzad abracadabrad             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc b mnzad z abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc b mnzad rabracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc  mnzad abc xvfdabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc xvf b mnzada abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc v mnzad abc xvfdabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc xvf aab mnzadab az abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'fz abc xvfg ma mnzads rz abc xvfaadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '           mz abc xvfg df mnzad mknz abc xvfdabrablol             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	m_scen = ('Test 17 Lowercase /\w\w\w|\w\w\w/');

	sCat = "Both Patterns same ";
	apInitScenario(m_scen);
	regPat = /\w\w\w|\w\w\w/;
	objExp = new Array('abc');
	regExp = new Array();

	objExp = 0;
	strTest = 'abc mnza';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	objExp = 2;
	strTest = 'z abc mnza';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	strTest = 'z abc xvf mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc xvf mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 6;
	strTest = '      abc mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 10;
	strTest = '          abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 12;
	strTest = '          z abc        mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'ab abc mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 8;
	strTest = 'ab      abc mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'az abc  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'ab	abc xvf  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 12;
	strTest = 'ab          abc    mnzad    ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = 'ab e          z abc xvf   mnzad     ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 18 Slightly more complex strings');

	objExp = 0;
	strTest = 'abc mnzad ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc mnzad ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 7;
	strTest = 'ab c z abc xvf mnzad  abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc mnzad ab c abracadabrab'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc mnzad ab c abracadabrab c            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc ab c abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc  mnzad ab c abracadabrab d            '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	m_scen = ('Test 19 Lowercase tests w/ multiples');

	objExp = 0;
	strTest = 'abc  mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 5;
	strTest = '     abc  mnzad';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc  mnzad      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 9;
	strTest = '         abc  mnzad       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '         abc xvf e mnzad       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 20 Slightly more complex strings w/ multiple finds');

	objExp = 0;
	strTest = 'abc  mnzad abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '              abc  mnzad abracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc  mnzad abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 21;
	strTest = '              o r az abc xvfbo sien mnzadthedral abracadabrae'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 7;
	strTest = 'o r az abc xvfbo sien mnzadthedral abracadabrae              '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc xvf f mnzad eabracadabras'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc xvf d mnzad abracadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 13;
	strTest = '           z abc xvf s mnzad abracadabrad             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc b mnzad z abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc b mnzad rabracadabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 0;
	strTest = 'abc  mnzad abc xvfdabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc xvf b mnzada abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 2;
	strTest = 'z abc v mnzad abc xvfdabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 16;
	strTest = '              z abc xvf aab mnzadab az abc xvfdabra'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 3;
	strTest = 'fz abc xvfg ma mnzads rz abc xvfaadabra             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	objExp = 14;
	strTest = '           mz abc xvfg df mnzad mknz abc xvfdabrablol             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 21 Failure - Lowercase /\w\W\w\W\W\W\w\W|\w\w\W\w\W\W\W\W\W/');

	sCat = "Both Patterns do not exist ";
	apInitScenario(m_scen);
	regPat = /\w\W\w\W\W\W\w\W|\w\W\w\w\w\W\w\W\W\W\W\W/;
objExp = -1;
	regExp = new Array();

	strTest = 'abc mnza';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	strTest = 'z abc mnza';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	strTest = 'z abc xvf mnzad';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'abc xvf mnzad';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '      abc mnzad';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'abc        mnzad';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'z abc        mnzad';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '          abc        mnzad';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '          z abc        mnzad';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'ab abc mnzad';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'ab      abc mnzad';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'az abc  mnzad      ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'ab	abc xvf  mnzad      ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'ab          abc    mnzad    ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'ab e          z abc xvf   mnzad     ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 22 Failure - Slightly more complex strings');

	strTest = 'abc mnzad ab c abracadabra'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '              abc mnzad ab c abracadabra'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'ab c z abc xvf mnzad  abracadabra             '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '              z abc mnzad ab c abracadabrab'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'z abc mnzad ab c abracadabrab c            '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '              z abc ab c abracadabra'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'z abc  mnzad ab c abracadabrab d            '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end


	m_scen = ('Test 23 Failure - Lowercase tests w/ multiples');

	strTest = 'abc  mnzad';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '     abc  mnzad';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'abc  mnzad      ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'z abc  mnzad      ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '         abc  mnzad       ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '         abc xvf e mnzad       ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	m_scen = ('Test 24 Failure - Slightly more complex strings w/ multiple finds');

	strTest = 'abc  mnzad abracadabra'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '              abc  mnzad abracadabra'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'abc  mnzad abracadabra             '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '              o r az abc xvfbo sien mnzadthedral abracadabrae'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'o r az abc xvfbo sien mnzadthedral abracadabrae              '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '              z abc xvf f mnzad eabracadabras'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'z abc xvf d mnzad abracadabra             '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '           z abc xvf s mnzad abracadabrad             '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'z abc b mnzad z abc xvfdabra'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '              z abc b mnzad rabracadabra'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'abc  mnzad abc xvfdabra             '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '              z abc xvf b mnzada abc xvfdabra'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'z abc v mnzad abc xvfdabra             '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '              z abc xvf aab mnzadab az abc xvfdabra'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = 'fz abc xvfg ma mnzads rz abc xvfaadabra             '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end

	strTest = '           mz abc xvfg df mnzad mknz abc xvfdabrablol             '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
/*****************************************************************************/
	apEndTest();
}


src_018();


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
