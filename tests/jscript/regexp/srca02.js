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


var iTestID = 53632;

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





function srca02() {
	

apInitTest("srca02");
	m_scen = ('Test 17 UPPERCASE /(A)(BC)/');

	
	sCat = "/(A)(BC)/ ";
	apInitScenario(m_scen);
	regPat = /(A)(BC)/;
	objExp = new Array('ABC', 'A','BC');
	regExp = new Array('A','BC');

	objExp = 0;
	strTest = 'ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 6;
	strTest = '      ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 10;
	strTest = '          ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '          ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 3;
	strTest = 'AB ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 8;
	strTest = 'AB      ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 2;
	strTest = 'ABABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 3;
	strTest = 'AB	ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = 'AB          ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'AB          ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 18 Slightly more complex strings');

	
	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC AB C ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC AB C ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC AB C ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC AB C ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 19 UPPERCASE tests /(A)(BC)/ multiples');

	
	strTest = 'ABC ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 5;
	strTest = '     ABC ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC ABC      |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 9;
	strTest = '         ABC ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '         ABC ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 20 Slightly more complex strings w/ multiple finds');

	
	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = '|           ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	objExp = 0;
	strTest = 'ABC ABC ABCADABRA';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC ABC ABRACADABRA            |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 13;
	strTest = '|            ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA           | '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = '|           ABC ABC ABRACADABRA             | '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	m_scen = ('Test 21 UPPERCASE /((A)(BC))/');

	
	sCat = "/((A)(BC))/ ";
	apInitScenario(m_scen);
	regPat = /((A)(BC))/;
	objExp = new Array('ABC','ABC','A','BC');
	regExp = new Array('ABC','A','BC');

	objExp = 0;
	strTest = 'ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 6;
	strTest = '      ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 10;
	strTest = '          ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '          ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 3;
	strTest = 'AB ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 8;
	strTest = 'AB      ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 2;
	strTest = 'ABABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 3;
	strTest = 'AB	ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = 'AB          ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'AB          ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 22 Slightly more complex strings');

	
	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC AB C ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC AB C ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC AB C ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC AB C ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 23 UPPERCASE tests /((A)(BC))/ multiples');

	
	strTest = 'ABC ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 5;
	strTest = '     ABC ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC ABC      |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 9;
	strTest = '         ABC ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '         ABC ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 24 Slightly more complex strings w/ multiple finds');

	
	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = '|           ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	objExp = 0;
	strTest = 'ABC ABC ABCADABRA';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC ABC ABRACADABRA            |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 13;
	strTest = '|            ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA           | '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = '|           ABC ABC ABRACADABRA             | '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 25 UPPERCASE /(A)(B)(C)/');

	
	sCat = "/(A)(B)(C)/ ";
	apInitScenario(m_scen);
	regPat = /(A)(B)(C)/;
	objExp = new Array('ABC','A','B','C');
	regExp = new Array('A', 'B', 'C');

	objExp = 0;
	strTest = 'ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 6;
	strTest = '      ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 10;
	strTest = '          ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '          ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 3;
	strTest = 'AB ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 8;
	strTest = 'AB      ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 2;
	strTest = 'ABABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 3;
	strTest = 'AB	ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = 'AB          ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'AB          ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 26 Slightly more complex strings');

	
	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC AB C ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC AB C ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC AB C ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC AB C ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 27 UPPERCASE tests /(A)(B)(C)/ multiples');

	
	strTest = 'ABC ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 5;
	strTest = '     ABC ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC ABC      |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 9;
	strTest = '         ABC ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '         ABC ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 28 Slightly more complex strings w/ multiple finds');

	
	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = '|           ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	objExp = 0;
	strTest = 'ABC ABC ABCADABRA';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC ABC ABRACADABRA            |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 13;
	strTest = '|            ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA           | '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = '|           ABC ABC ABRACADABRA             | '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 29 UPPERCASE /((A)(B)(C))/');

	
	sCat = "/((A)(B)(C))/ ";
	apInitScenario(m_scen);
	regPat = /((A)(B)(C))/;
	objExp = new Array('ABC','ABC','A','B','C');
	regExp = new Array('ABC','A','B','C');

	objExp = 0;
	strTest = 'ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 6;
	strTest = '      ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 10;
	strTest = '          ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '          ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 3;
	strTest = 'AB ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 8;
	strTest = 'AB      ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 2;
	strTest = 'ABABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 3;
	strTest = 'AB	ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = 'AB          ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'AB          ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 30 Slightly more complex strings');

	
	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC AB C ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC AB C ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC AB C ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC AB C ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC AB C ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 31 UPPERCASE tests /((A)(B)(C))/ multiples');

	
	strTest = 'ABC ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 5;
	strTest = '     ABC ABC';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC      ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC ABC      |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 9;
	strTest = '         ABC ABC       ';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '         ABC ABC       |';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 32 Slightly more complex strings w/ multiple finds');

	
	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = '|           ABC ABC ABRACADABRA             |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	objExp = 0;
	strTest = 'ABC ABC ABCADABRA';
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 14;
	strTest = '              ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA             '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 15;
	strTest = '|              ABC ABC ABRACADABRA|'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 1;
	strTest = '|ABC ABC ABRACADABRA            |'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 13;
	strTest = '|            ABC ABC ABRACADABRA'; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 0;
	strTest = 'ABC ABC ABRACADABRA           | '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	objExp = 12;
	strTest = '|           ABC ABC ABRACADABRA             | '; 
	ArrayEqual(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	m_scen = ('Test 33 Failure UPPERCASE /aBc/');

	
	sCat = "/aBc/ ";
	apInitScenario(m_scen);
	regPat = /aBc/;
objExp = -1;
	//no new regExp expected values due to spec of RegExp.$1-9
	//these tests verify that old values are kept

	strTest = 'ABC';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '      ABC';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC       ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC       |';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '          ABC       ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '          ABC       |';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'AB ABC';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'AB      ABC';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABABC       ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'AB	ABC       |';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'AB          ABC       ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'AB          ABC       |';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	m_scen = ('Test 34 Failure-Slightly more complex strings');

	
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '              ABC AB C ABRACADABRA'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC AB C ABRACADABRA             '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '|              ABC AB C ABRACADABRA|'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '|ABC AB C ABRACADABRA             |'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '|              ABC AB C ABRACADABRA|'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC AB C ABRACADABRA             |'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end


	m_scen = ('Test 35 Failure UPPERCASE tests /aBc/ multiples');

	
	strTest = 'ABC ABC';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '     ABC ABC';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC ABC      ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC ABC      |';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '         ABC ABC       ';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '         ABC ABC       |';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	m_scen = ('Test 36 Failure - Slightly more complex strings w/ multiple finds');

	
	strTest = 'ABC ABC ABRACADABRA'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '              ABC ABC ABRACADABRA'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC ABC ABRACADABRA             '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '|              ABC ABC ABRACADABRA|'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '|ABC ABC ABRACADABRA             |'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '|              ABC ABC ABRACADABRA'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC ABC ABRACADABRA             |'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '|           ABC ABC ABRACADABRA             |'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC ABC ABCADABRA';
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '              ABC ABC ABRACADABRA'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC ABC ABRACADABRA             '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '|              ABC ABC ABRACADABRA|'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '|ABC ABC ABRACADABRA            |'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '|            ABC ABC ABRACADABRA'; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = 'ABC ABC ABRACADABRA           | '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

	strTest = '|           ABC ABC ABRACADABRA             | '; 
	verify(sCat+strTest, objExp, strTest.search(regPat));
	@if (!@_fast)
		@if (!@_fast)
		regVerify(sCat+strTest, regExp, strTest.search(regPat));
	@end
	@end

/*****************************************************************************/
	apEndTest();
}


srca02();


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
