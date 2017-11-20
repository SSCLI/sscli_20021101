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


var iTestID = 52835;



@if(!@aspx)
	function obFoo() {};
@else
	expando function obFoo() {};
@end

function if013() {

    apInitTest("if013 ");

    var n;

    //----------------------------------------------------------------------------
    apInitScenario("1. built-in, non-exec");

    n = 0;
    if (true && Math)
        n++;
    else
        n--;
    
    if (n != 1) apLogFailInfo( "non-executable object references--Math failed ", 1, n,"");

    n = 0;
    if (false && Math)
        n++;
    else
        n--;
    
    if (n != -1) apLogFailInfo( "non-executable object references--Math failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("2. built-in, exec, not instanciated");

    n = 0;
    if (true && Array)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Array failed ", 1, n,"");

    n = 0;
    if (false && Array)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Array failed ", -1, n,"");

    n = 0;
    if (true && Boolean)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Boolean failed ", 1, n,"");

    n = 0;
    if (false && Boolean)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Boolean failed ", -1, n,"");

    n = 0;
    if (true && Date)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Date failed ", 1, n,"");

    n = 0;
    if (false && Date)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Date failed ", -1, n,"");

    n = 0;
    if (true && Number)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Number failed ", 1, n,"");

    n = 0;
    if (false && Number)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Number failed ", -1, n,"");

    n = 0;
    if (true && Object)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Object failed ", 1, n,"");

    n = 0;
    if (false && Object)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Object failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("3. built-in, exec, instanciated");

    n = 0;
    if (true && new Array())
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Array failed ", 1, n,"");

    n = 0;
    if (false && new Array())
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Array failed ", -1, n,"");

    n = 0;
    if (true && new Boolean())
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Boolean failed ", 1, n,"");

    n = 0;
    if (false && new Boolean())
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Boolean failed ", -1, n,"");

    n = 0;
    if (true && new Date())
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Date failed ", 1, n,"");

    n = 0;
    if (false && new Date())
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Date failed ", -1, n,"");

    n = 0;
    if (true && new Number())
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Number failed ", 1, n,"");

    n = 0;
    if (false && new Number())
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Number failed ", -1, n,"");

    n = 0;
    if (true && new Object())
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Object failed ", 1, n,"");

    n = 0;
    if (false && new Object())
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Object failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("4. user-defined, not instanciated");

    n = 0;
    if (true && obFoo)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--user-defined failed ", 1, n,"");

    n = 0;
    if (false && obFoo)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--user-defined failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("5. user-defined, instanciated");

    var ob = new obFoo();
    n = 0;
    if (true && ob)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--var instantiated user-defined failed ", 1, n,"");

    n = 0;
    if (false && ob)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--var instantiated user-defined failed ", -1, n,"");

    n = 0;
    if (true && new obFoo())
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--instantiated user-defined failed ", 1, n,"");

    n = 0;
    if (false && new obFoo())
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--instantiated user-defined failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("6. number, decimal, integer");

    // >0

        n = 0;    
        if (true && 1)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos failed ", 1, n,"");

	    n = 0;
        if (false && 1)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos failed ", -1, n,"");
    
        n = 0;
        if (true && 1234567890)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos < n < max pos failed ", 1, n,"");

        n = 0;
        if (false && 1234567890)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos < n < max pos failed ", -1, n,"");

        n = 0;
        if (true && 2147483647)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max pos failed ", 1, n,"");

        n = 0;
        if (false && 2147483647)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max pos failed ", -1, n,"");

    // <0

        n = 0;
        if (true && -1)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max neg failed ", 1, n,"");

        n = 0;
        if (false && -1)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max neg failed ", -1, n,"");

        n = 0;
        if (true && -1234567890)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg < n < max neg failed ", 1, n,"");

        n = 0;
        if (false && -1234567890)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg < n < max neg failed ", -1, n,"");

        n = 0;
        if (true && -2147483647)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg failed ", 1, n,"");

        n = 0;
        if (false && -2147483647)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg failed ", -1, n,"");

    // 0

        n = 0;
        if (true && 0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (false && 0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (true && -0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");

        n = 0;
        if (false && -0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("7. number, decimal, float");

    // >0.0

        n = 0;    
        if (true && 2.225073858507202e-308)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos failed ", 1, n,"");

        n = 0;
        if (false && 2.225073858507202e-308)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos failed ", -1, n,"");
    
        n = 0;
        if (true && 1.2345678e90)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos float < n < max pos failed ", 1, n,"");

        n = 0;
        if (false && 1.2345678e90)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos float < n < max pos failed ", -1, n,"");

        n = 0;
        if (true && 1.797693134862315807e308)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max pos failed ", 1, n,"");

        n = 0;
        if (false && 1.797693134862315807e308)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max pos failed ", -1, n,"");

        n = 0;
        if (true && 1.797693134862315807e309)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "> max pos (1.#INF) failed ", 1, n,"");

        n = 0;
        if (false && 1.797693134862315807e309)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "> max pos (1.#INF) failed ", -1, n,"");

    // <0

        n = 0;
        if (true && -2.225073858507202e-308)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max neg failed ", 1, n,"");

        n = 0;
        if (false && -2.225073858507202e-308)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max neg failed ", -1, n,"");

        n = 0;
        if (true && -1.2345678e90)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg < n < max neg failed ", 1, n,"");

        n = 0;
        if (false && -1.2345678e90)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg < n < max neg failed ", -1, n,"");

        n = 0;
        if (true && -1.797693134862315807e308)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg failed ", 1, n,"");

        n = 0;
        if (false && -1.797693134862315807e308)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg failed ", -1, n,"");

        n = 0;
        if (true && -1.797693134862315807e309)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "< min neg (-1.#INF) failed ", 1, n,"");

        n = 0;
        if (false && -1.797693134862315807e309)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "< min neg (-1.#INF) failed ", -1, n,"");

    // 0.0

        n = 0;
        if (true && 0.0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (false && 0.0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (true && -0.0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");

        n = 0;
        if (false && -0.0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");

        
    //----------------------------------------------------------------------------
    apInitScenario("8. number, hexidecimal");

    // >0

        n = 0;    
        if (true && 0x1)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos failed ", 1, n,"");

        n = 0;
        if (false && 0x1)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos failed ", -1, n,"");
    
        n = 0;
        if (true && 0x2468ACE)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos < n < max pos failed ", 1, n,"");

        n = 0;
        if (false && 0x2468ACE)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos < n < max pos failed ", -1, n,"");

        n = 0;
        if (true && 0xFFFFFFFF)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max pos failed ", 1, n,"");

        n = 0;
        if (false && 0xFFFFFFFF)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max pos failed ", -1, n,"");

    // <0

        n = 0;
        if (true && -0x1)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max neg failed ", 1, n,"");

        n = 0;
        if (false && -0x1)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max neg failed ", -1, n,"");

        n = 0;
        if (true && -0x2468ACE)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg < n < max neg failed ", 1, n,"");

        n = 0;
        if (false && -0x2468ACE)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg < n < max neg failed ", -1, n,"");

        n = 0;
        if (true && -0xFFFFFFFF)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg failed ", 1, n,"");

        n = 0;
        if (false && -0xFFFFFFFF)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg failed ", -1, n,"");

    // 0

        n = 0;
        if (true && 0x0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (false && 0x0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (true && -0x0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");

        n = 0;
        if (false && -0x0)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("9. number, octal");

    // >0

        n = 0;    
        if (true && 01)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos failed ", 1, n,"");

        n = 0;
        if (false && 01)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos failed ", -1, n,"");
    
        n = 0;
        if (true && 01234567)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos < n < max pos failed ", 1, n,"");

        n = 0;
        if (false && 01234567)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos < n < max pos failed ", -1, n,"");

        n = 0;
        if (true && 037777777777)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max pos failed ", 1, n,"");

        n = 0;
        if (false && 037777777777)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max pos failed ", -1, n,"");

    // <0

        n = 0;
        if (true && -01)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max neg failed ", 1, n,"");

        n = 0;
        if (false && -01)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max neg failed ", -1, n,"");

        n = 0;
        if (true && -01234567)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg < n < max neg failed ", 1, n,"");

        n = 0;
        if (false && -01234567)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg < n < max neg failed ", -1, n,"");

        n = 0;
        if (true && -037777777777)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg failed ", 1, n,"");

        n = 0;
        if (false && -037777777777)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg failed ", -1, n,"");

    // 0

        n = 0;
        if (true && 00)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (false && 00)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (true && -00)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");

        n = 0;
        if (false && -00)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("10. string");

        n = 0;    
        if (true && " ")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "single space failed ", 1, n,"");

        n = 0;
        if (false && " ")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "single space failed ", -1, n,"");

        n = 0;
        if (true && "                                                                   ")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "multiple spaces failed ", 1, n,"");

        n = 0;
        if ("                                                                   " && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "multiple spaces failed ", -1, n,"");

        n = 0;
        if (true && "false")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as false failed ", 1, n,"");

        n = 0;
        if (false && "false")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as false failed ", -1, n,"");

        n = 0;
        if (true && "0")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "ns 0 failed ", 1, n,"");

        n = 0;
        if (false && "0")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "ns 0 failed ", -1, n,"");

        n = 0;
        if (true && "1234567890")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "ns >0 failed ", 1, n,"");

        n = 0;
        if (false && "1234567890")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "ns >0 failed ", -1, n,"");

        n = 0;
        if (true && " 1234567890")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "ns >0, leading space failed ", 1, n,"");

        n = 0;
        if (false && " 1234567890")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "ns >0, leading space failed ", -1, n,"");

        n = 0;
        if (true && "1234567890 ")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "ns >0, trailing space failed ", 1, n,"");

        n = 0;
        if (false && "1234567890 ")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "ns >0, trailing space failed ", -1, n,"");

        n = 0;
        if (true && "-1234567890")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "ns <0 failed ", 1, n,"");

        n = 0;
        if (false && "-1234567890")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "ns <0 failed ", -1, n,"");

        n = 0;
        if (true && "foo")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as single word failed ", 1, n,"");

        n = 0;
        if (false && "foo")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as single word failed ", -1, n,"");

        n = 0;
        if (true && " foo")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as single word, leading space failed ", 1, n,"");

        n = 0;
        if (false && " foo")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as single word, leading space failed ", -1, n,"");

        n = 0;
        if (true && "foo ")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as single word, trailing space failed ", 1, n,"");

        n = 0;
        if (false && "foo ")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as single word, trailing space failed ", -1, n,"");

        n = 0;
        if (true && "foo bar")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as multiple word failed ", 1, n,"");

        n = 0;
        if (false && "foo bar")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as multiple word failed ", -1, n,"");

        n = 0;
        if (true && " foo bar")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as multiple word, leading space failed ", 1, n,"");

        n = 0;
        if (false && " foo bar")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as multiple word, leading space failed ", -1, n,"");

        n = 0;
        if (true && "foo bar ")
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as multiple word, trailing space failed ", 1, n,"");

        n = 0;
        if (false && "foo bar ")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as multiple word, trailing space failed ", -1, n,"");

        n = 0;
        if (true && "")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zls failed ", -1, n,"");

        n = 0;
        if (false && "")
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zls failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("11. constants");

    n = 0;
    if (true && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "constants--true failed ", 1, n,"");

    n = 0;
    if (false && true)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "constants--true failed ", -1, n,"");


    n = 0;
    if (true && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "constants--false failed ", -1, n,"");

    n = 0;
    if (false && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "constants--false failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("12. null");

    n = 0;
    if (true && null)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "null failed ", -1, n,"");

    n = 0;
    if (false && null)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "null failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("13. undefined");

	var obUndef;
    n = 0;
    if (true && obUndef)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "null failed ", -1, n,"");

    n = 0;
    if (false && obUndef)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "null failed ", -1, n,"");
    

    apEndTest();

}


if013();


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
