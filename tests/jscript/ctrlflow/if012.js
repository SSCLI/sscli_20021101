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


var iTestID = 52834;



@if(!@aspx)
	function obFoo() {};
@else
	expando function obFoo() {};
@end

function if012() {

    apInitTest("if012 ");

    var n;

    //----------------------------------------------------------------------------
    apInitScenario("1. built-in, non-exec");

    n = 0;
    if (Math && true)
        n++;
    else
        n--;
    
    if (n != 1) apLogFailInfo( "non-executable object references--Math failed ", 1, n,"");

    n = 0;
    if (Math && false)
        n++;
    else
        n--;
    
    if (n != -1) apLogFailInfo( "non-executable object references--Math failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("2. built-in, exec, not instanciated");

    n = 0;
    if (Array && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Array failed ", 1, n,"");

    n = 0;
    if (Array && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Array failed ", -1, n,"");

    n = 0;
    if (Boolean && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Boolean failed ", 1, n,"");

    n = 0;
    if (Boolean && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Boolean failed ", -1, n,"");

    n = 0;
    if (Date && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Date failed ", 1, n,"");

    n = 0;
    if (Date && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Date failed ", -1, n,"");

    n = 0;
    if (Number && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Number failed ", 1, n,"");

    n = 0;
    if (Number && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Number failed ", -1, n,"");

    n = 0;
    if (Object && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Object failed ", 1, n,"");

    n = 0;
    if (Object && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Object failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("3. built-in, exec, instanciated");

    n = 0;
    if (new Array() && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Array failed ", 1, n,"");

    n = 0;
    if (new Array() && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Array failed ", -1, n,"");

    n = 0;
    if (new Boolean() && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Boolean failed ", 1, n,"");

    n = 0;
    if (new Boolean() && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Boolean failed ", -1, n,"");

    n = 0;
    if (new Date() && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Date failed ", 1, n,"");

    n = 0;
    if (new Date() && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Date failed ", -1, n,"");

    n = 0;
    if (new Number() && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Number failed ", 1, n,"");

    n = 0;
    if (new Number() && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Number failed ", -1, n,"");

    n = 0;
    if (new Object() && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--Object failed ", 1, n,"");

    n = 0;
    if (new Object() && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--Object failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("4. user-defined, not instanciated");

    n = 0;
    if (obFoo && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--user-defined failed ", 1, n,"");

    n = 0;
    if (obFoo && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--user-defined failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("5. user-defined, instanciated");

    var ob = new obFoo();
    n = 0;
    if (ob && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--var instantiated user-defined failed ", 1, n,"");

    n = 0;
    if (ob && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--var instantiated user-defined failed ", -1, n,"");

    n = 0;
    if (new obFoo() && true)
        n++;
    else
        n--;

    if (n != 1) apLogFailInfo( "executable object references--instantiated user-defined failed ", 1, n,"");

    n = 0;
    if (new obFoo() && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "executable object references--instantiated user-defined failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("6. number, decimal, integer");

    // >0

        n = 0;    
        if (1 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos failed ", 1, n,"");

        n = 0;
        if (1 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos failed ", -1, n,"");
    
        n = 0;
        if (1234567890 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos < n < max pos failed ", 1, n,"");

        n = 0;
        if (1234567890 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos < n < max pos failed ", -1, n,"");

        n = 0;
        if (2147483647 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max pos failed ", 1, n,"");

        n = 0;
        if (2147483647 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max pos failed ", -1, n,"");

    // <0

        n = 0;
        if (-1 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max neg failed ", 1, n,"");

        n = 0;
        if (-1 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max neg failed ", -1, n,"");

        n = 0;
        if (-1234567890 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg < n < max neg failed ", 1, n,"");

        n = 0;
        if (-1234567890 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg < n < max neg failed ", -1, n,"");

        n = 0;
        if (-2147483647 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg failed ", 1, n,"");

        n = 0;
        if (-2147483647 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg failed ", -1, n,"");

    // 0

        n = 0;
        if (0 && true)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (0 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (-0 && true)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");

        n = 0;
        if (-0 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("7. number, decimal, float");

    // >0.0

        n = 0;    
        if (2.225073858507202e-308 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos failed ", 1, n,"");

        n = 0;
        if (2.22507385850720125959e-308 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos failed ", -1, n,"");
    
        n = 0;
        if (1.2345678e90 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos float < n < max pos failed ", 1, n,"");

        n = 0;
        if (1.2345678e90 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos float < n < max pos failed ", -1, n,"");

        n = 0;
        if (1.797693134862315807e308 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max pos failed ", 1, n,"");

        n = 0;
        if (1.797693134862315807e308 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max pos failed ", -1, n,"");

        n = 0;
        if (1.797693134862315807e309 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "> max pos (1.#INF) failed ", 1, n,"");

        n = 0;
        if (1.797693134862315807e309 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "> max pos (1.#INF) failed ", -1, n,"");

    // <0

        n = 0;
        if (-2.225073858507202e-308 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max neg failed ", 1, n,"");

        n = 0;
        if (-2.22507385850720125959e-308 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max neg failed ", -1, n,"");

        n = 0;
        if (-1.2345678e90 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg < n < max neg failed ", 1, n,"");

        n = 0;
        if (-1.2345678e90 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg < n < max neg failed ", -1, n,"");

        n = 0;
        if (-1.797693134862315807e308 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg failed ", 1, n,"");

        n = 0;
        if (-1.797693134862315807e308 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg failed ", -1, n,"");

        n = 0;
        if (-1.797693134862315807e309 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "< min neg (-1.#INF) failed ", 1, n,"");

        n = 0;
        if (-1.797693134862315807e309 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "< min neg (-1.#INF) failed ", -1, n,"");

    // 0.0

        n = 0;
        if (0.0 && true)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (0.0 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (-0.0 && true)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");

        n = 0;
        if (-0.0 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");

        
    //----------------------------------------------------------------------------
    apInitScenario("8. number, hexidecimal");

    // >0

        n = 0;    
        if (0x1 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos failed ", 1, n,"");

        n = 0;
        if (0x1 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos failed ", -1, n,"");
    
        n = 0;
        if (0x2468ACE && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos < n < max pos failed ", 1, n,"");

        n = 0;
        if (0x2468ACE && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos < n < max pos failed ", -1, n,"");

        n = 0;
        if (0xFFFFFFFF && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max pos failed ", 1, n,"");

        n = 0;
        if (0xFFFFFFFF && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max pos failed ", -1, n,"");

    // <0

        n = 0;
        if (-0x1 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max neg failed ", 1, n,"");

        n = 0;
        if (-0x1 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max neg failed ", -1, n,"");

        n = 0;
        if (-0x2468ACE && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg < n < max neg failed ", 1, n,"");

        n = 0;
        if (-0x2468ACE && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg < n < max neg failed ", -1, n,"");

        n = 0;
        if (-0xFFFFFFFF && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg failed ", 1, n,"");

        n = 0;
        if (-0xFFFFFFFF && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg failed ", -1, n,"");

    // 0

        n = 0;
        if (0x0 && true)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (0x0 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (-0x0 && true)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");

        n = 0;
        if (-0x0 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("9. number, octal");

    // >0

        n = 0;    
        if (01 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos failed ", 1, n,"");

        n = 0;
        if (01 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos failed ", -1, n,"");
    
        n = 0;
        if (01234567 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min pos < n < max pos failed ", 1, n,"");

        n = 0;
        if (01234567 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min pos < n < max pos failed ", -1, n,"");

        n = 0;
        if (037777777777 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max pos failed ", 1, n,"");

        n = 0;
        if (037777777777 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max pos failed ", -1, n,"");

    // <0

        n = 0;
        if (-01 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "max neg failed ", 1, n,"");

        n = 0;
        if (-01 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "max neg failed ", -1, n,"");

        n = 0;
        if (-01234567 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg < n < max neg failed ", 1, n,"");

        n = 0;
        if (-01234567 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg < n < max neg failed ", -1, n,"");

        n = 0;
        if (-037777777777 && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "min neg failed ", 1, n,"");

        n = 0;
        if (-037777777777 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "min neg failed ", -1, n,"");

    // 0

        n = 0;
        if (00 && true)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (00 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zero failed ", -1, n,"");

        n = 0;
        if (-00 && true)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");

        n = 0;
        if (-00 && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "-zero failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("10. string");

        n = 0;    
        if (" " && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "single space failed ", 1, n,"");

        n = 0;
        if (" " && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "single space failed ", -1, n,"");

        n = 0;
        if ("                                                                   " && true)
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
        if ("false" && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as false failed ", 1, n,"");

        n = 0;
        if ("false" && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as false failed ", -1, n,"");

        n = 0;
        if ("0" && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "ns 0 failed ", 1, n,"");

        n = 0;
        if ("0" && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "ns 0 failed ", -1, n,"");

        n = 0;
        if ("1234567890" && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "ns >0 failed ", 1, n,"");

        n = 0;
        if ("1234567890" && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "ns >0 failed ", -1, n,"");

        n = 0;
        if (" 1234567890" && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "ns >0, leading space failed ", 1, n,"");

        n = 0;
        if (" 1234567890" && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "ns >0, leading space failed ", -1, n,"");

        n = 0;
        if ("1234567890 " && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "ns >0, trailing space failed ", 1, n,"");

        n = 0;
        if ("1234567890 " && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "ns >0, trailing space failed ", -1, n,"");

        n = 0;
        if ("-1234567890" && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "ns <0 failed ", 1, n,"");

        n = 0;
        if ("-1234567890" && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "ns <0 failed ", -1, n,"");

        n = 0;
        if ("foo" && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as single word failed ", 1, n,"");

        n = 0;
        if ("foo" && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as single word failed ", -1, n,"");

        n = 0;
        if (" foo" && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as single word, leading space failed ", 1, n,"");

        n = 0;
        if (" foo" && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as single word, leading space failed ", -1, n,"");

        n = 0;
        if ("foo " && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as single word, trailing space failed ", 1, n,"");

        n = 0;
        if ("foo " && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as single word, trailing space failed ", -1, n,"");

        n = 0;
        if ("foo bar" && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as multiple word failed ", 1, n,"");

        n = 0;
        if ("foo bar" && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as multiple word failed ", -1, n,"");

        n = 0;
        if (" foo bar" && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as multiple word, leading space failed ", 1, n,"");

        n = 0;
        if (" foo bar" && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as multiple word, leading space failed ", -1, n,"");

        n = 0;
        if ("foo bar " && true)
            n++;
        else
            n--;

        if (n != 1) apLogFailInfo( "as multiple word, trailing space failed ", 1, n,"");

        n = 0;
        if ("foo bar " && false)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "as multiple word, trailing space failed ", -1, n,"");

        n = 0;
        if ("" && true)
            n++;
        else
            n--;

        if (n != -1) apLogFailInfo( "zls failed ", -1, n,"");

        n = 0;
        if ("" && false)
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
    if (true && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "constants--true failed ", -1, n,"");


    n = 0;
    if (false && true)
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
    if (null && true)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "null failed ", -1, n,"");

    n = 0;
    if (null && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "null failed ", -1, n,"");


    //----------------------------------------------------------------------------
    apInitScenario("13. undefined");

	var obUndef;
    n = 0;
    if (obUndef && true)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "null failed ", -1, n,"");

    n = 0;
    if (obUndef && false)
        n++;
    else
        n--;

    if (n != -1) apLogFailInfo( "null failed ", -1, n,"");
    

    apEndTest();

}


if012();


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
