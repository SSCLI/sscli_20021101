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


var iTestID = 53432;

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



function rxp_33() {
	

apInitTest("rxp_33");


	m_scen = ('Test 1 mixedcase /\u0000B1nmnoz/');

	apInitScenario(m_scen);
	regPat = /a/ig;
	regPat.compile("\\u0000B1nmnoz","");
	objExp = new Array('\u0000B1nmnoz');
	regExp = new Array();

	strTest = '\u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end


	strTest = '      \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end









	m_scen = ('Test 2 Slightly more complex strings');

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end





	m_scen = ('Test 3 mixedcase tests /\u0000B1nmnoz/ multiples');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '     \u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end



	m_scen = ('Test 4 Slightly more complex strings w/ multiple finds');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end













	m_scen = ('Test 5 mixedcase /(\u0000B1nmnoz)/');

	apInitScenario(m_scen);
	regPat = /a/ig;
	regPat = new RegExp("(\\u0000B1nmnoz)","");
	objExp = new Array('\u0000B1nmnoz', '\u0000B1nmnoz');
	regExp = new Array('\u0000B1nmnoz');

	strTest = '\u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '      \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end









	m_scen = ('Test 6 Slightly more complex strings');

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end




	m_scen = ('Test 7 mixedcase tests /(\u0000B1nmnoz)/ multiples');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '     \u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end




	m_scen = ('Test 8 Slightly more complex strings w/ multiple finds');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end













	m_scen = ('Test 9 mixedcase /(\u0000B1)(nmnoz)/');

	apInitScenario(m_scen);
	regPat = /a/ig;
	regPat = new RegExp("(\\u0000B1)(nmnoz)","");
	objExp = new Array('\u0000B1nmnoz','\u0000B1','nmnoz');
	regExp = new Array('\u0000B1','nmnoz');

	strTest = '\u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '      \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end










	m_scen = ('Test 10 Slightly more complex strings');

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end





	m_scen = ('Test 11 mixedcase tests /(\u0000B1)(nmnoz)/ multiples');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '     \u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end



	m_scen = ('Test 12 Slightly more complex strings w/ multiple finds');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end














	m_scen = ('Test 13 mixedcase /((\u0000B1)(nmnoz))/');

	apInitScenario(m_scen);
	regPat = /a/ig;
	regPat.compile("((\\u0000B1)(nmnoz))","");
	objExp = new Array('\u0000B1nmnoz', '\u0000B1nmnoz','\u0000B1','nmnoz');
	regExp = new Array('\u0000B1nmnoz','\u0000B1','nmnoz');

	strTest = '\u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '      \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end










	m_scen = ('Test 14 Slightly more complex strings');

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end





	m_scen = ('Test 15 mixedcase tests //((ab)(c))/ multiples');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '     \u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end




	m_scen = ('Test 16 Slightly more complex strings w/ multiple finds');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end















	m_scen = ('Test 17 mixedcase /(\u0000)(B1nmnoz)/');

	apInitScenario(m_scen);
	regPat = /a/ig;
	regPat = new RegExp("(\\u0000)(B1nmnoz)","");
	objExp = new Array('\u0000B1nmnoz', '\u0000','B1nmnoz');
	regExp = new Array('\u0000','B1nmnoz');

	strTest = '\u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '      \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end










	m_scen = ('Test 18 Slightly more complex strings');

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end





	m_scen = ('Test 19 mixedcase tests /(\u0000)(B1nmnoz)/ multiples');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '     \u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end




	m_scen = ('Test 20 Slightly more complex strings w/ multiple finds');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end














	m_scen = ('Test 21 mixedcase /((\u0000)(B1nmnoz))/');

	apInitScenario(m_scen);
	regPat = /a/ig;
	regPat = new RegExp("((\\u0000)(B1nmnoz))","");
	objExp = new Array('\u0000B1nmnoz','\u0000B1nmnoz','\u0000','B1nmnoz');
	regExp = new Array('\u0000B1nmnoz','\u0000','B1nmnoz');

	strTest = '\u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '      \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end










	m_scen = ('Test 22 Slightly more complex strings');

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end





	m_scen = ('Test 23 mixedcase tests /((\u0000)(B1nmnoz))/ multiples');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '     \u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end




	m_scen = ('Test 24 Slightly more complex strings w/ multiple finds');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end















	m_scen = ('Test 25 mixedcase /(\u0000)(B1)(nmnoz)/');

	apInitScenario(m_scen);
	regPat = /a/ig;
	regPat.compile("(\\u0000)(B1)(nmnoz)","");
	objExp = new Array('\u0000B1nmnoz','\u0000','B1','nmnoz');
	regExp = new Array('\u0000', 'B1', 'nmnoz');

	strTest = '\u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '      \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end










	m_scen = ('Test 26 Slightly more complex strings');

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end





	m_scen = ('Test 27 mixedcase tests /(\u0000)(B1)(nmnoz)/ multiples');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '     \u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end




	m_scen = ('Test 28 Slightly more complex strings w/ multiple finds');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end















	m_scen = ('Test 29 mixedcase /((\u0000)(B1)(nmnoz))/');

	apInitScenario(m_scen);
	regPat = /a/ig;
	regPat = new RegExp("((\\u0000)(B1)(nmnoz))","");
	objExp = new Array('\u0000B1nmnoz','\u0000B1nmnoz','\u0000','B1','nmnoz');
	regExp = new Array('\u0000B1nmnoz','\u0000','B1','nmnoz');

	strTest = '\u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '      \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end










	m_scen = ('Test 30 Slightly more complex strings');

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end





	m_scen = ('Test 31 mixedcase tests /((\u0000)(B1)(nmnoz))/ multiples');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '     \u0000B1nmnoz \u0000B1nmnoz';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      ';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      |';
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end




	m_scen = ('Test 32 Slightly more complex strings w/ multiple finds');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end














	m_scen = ('Test 33 Failure Lowercase /\u0000B1nmnoz\\/');

	apInitScenario(m_scen);
	regPat = /a/ig;
	regPat = new RegExp("\\u0000B1nmnoz\\\\","");
	objExp = null;
	//no new regExp due to spec of RegExp.$1-9
	//this set of tests smakes sure that RegExp.$ keep original values


	strTest = '\u0000B1nmnoz';
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '      \u0000B1nmnoz';
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       ';
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz       |';
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end









	m_scen = ('Test 34 Failure-Slightly more complex strings');

	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1 nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end





	m_scen = ('Test 35 Failure Lowercase tests /\u0000B1nmnoz/ multiples');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz';
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '     \u0000B1nmnoz \u0000B1nmnoz';
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      ';
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz      |';
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end



	m_scen = ('Test 36 Failure - Slightly more complex strings w/ multiple finds');

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000'; 
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '\u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000             '; 
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end

	strTest = '|              \u0000B1nmnoz \u0000B1nmnoz \u0000B1\000\u0000nmnoz\u0000\\\u0000B1\000\u0000|'; 
	verify(sCat+strTest, objExp, strTest.match(regPat));
	@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.match(regPat));
	@end












/*****************************************************************************/
	apEndTest();
}


rxp_33();


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
