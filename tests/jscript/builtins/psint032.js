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


var iTestID = 53186;


// Decimal number to binary string conversion
function bin (num, fAll)
{
	num = parseInt(num);
	if (fAll==null) fAll = false;
    var sBin="";

    for (var i=0; (num>>>i || fAll) && i<32; i++)
        sBin = (num>>>i&1) + sBin;

    return sBin;
}


// Decimal number to hexidecimal string conversion
function hex (num)
{
    num = parseInt(num);
    
    var sHex = "0123456789ABCDEF".charAt(num&0x0f);

    if (num>>>4) sHex = hex(num>>>4) + sHex;
    
    return sHex;
}


// Decimal to octal conversion
function oct (num)
{
    if (num < 0)
        return "-"+oct(Math.abs(num));

    if (num < 8)
        return ""+num;
    else
        return oct(Math.floor(num/8)) + num%8+"";
}


// Hex or oct to decimal conversion
function dec (num)
{
    if (typeof num != "string") return num;
    if (num.charAt(0)!= "0") num = "0x"+num;
    return eval(num);
}


// bin string to dec number conversion
function decBin (sNum)
{
    var val = 0, cb = sNum.length;
    for (var i=0; i<cb; i++)
        if (sNum.charAt(i)=="1") val |= 1<<(cb-1-i);
    return val;
}

// hex string to dec number conversion
// Handles signed int min (0x80000000) to signed int max (0x7fffffff), for
// everything else the behaviour is undefined.  The prefixes "0x"|"0X" are optional.
function decHex (sNum)
{
    var val = 0, cb = sNum.length;
    sNum = sNum.toLowerCase();
    for (var i=(sNum.substring(0,2)=="0x" ? 2 : 0); i<cb; i++)
        val |= "0123456789abcdef".indexOf(sNum.charAt(i))<<(4*(cb-1-i));
    return val;
}

// universal trim function
// defaults to trimming tabs, newlines, and spaces.
function trim (sIn, sTChars)
{
    if (!sIn.length) return sIn;
    if (sTChars == undefined) sTChars = "\t\n ";

    var iSt=-1, iEnd=sIn.length;

    while ( ++iSt<iEnd && sTChars.indexOf(sIn.charAt(iSt)) != -1 ) {}
    while ( --iEnd>iSt && sTChars.indexOf(sIn.charAt(iEnd)) != -1 ) {}

    return sIn.substring(iSt,iEnd+1);
}


// Equvilant to VBA's Chr function
function chr (num)
{
    if (num > 255 || num < 0) return null;

    var sHex = "0123456789abcdef";
    return unescape( "%"+sHex.charAt(num>>4&0x0f)+sHex.charAt(num&0x0f) );
}


// Equivilant to VBA's Asc function
function asc (str)
{
//    return parseInt( escape(str.charAt(0), 0).substring(1,3), 16 );
    return str.charCodeAt(0);
}


// Returns a "proper form" string from the input string sIn.
// The standard token delimiters are tab, newline, space, period, hypen,
// comma, & double/single quotes.  These can be overridden with the optional
// opDlm parameter and added to if the opDlm param contains a '+' char
// as the first char.
function proper (sIn, opDlm)
{
    var i;
    var std = "\t\n .-,\"\'";
    if (opDlm == undefined) opDlm = std;
    var sDlm = opDlm.charAt(0)=='+' ? std+opDlm.substring(1,opDlm.length) : opDlm;
    var ch, ich=0, cb=sIn.length, s=sIn.toLowerCase();

    for (i=0; i<cb; i++)
    {
        ch = s.charAt(i);
        if (sDlm.indexOf(ch)>=0)
            ich = i+1;
        else
            if (ich==i)
                s = s.substring(0,i)+ch.toUpperCase()+s.substring(i+1,cb);
    }
    
    return s;
}


// Equiv to VBA's String function
function bldStr (cb, str)
{
    if ( cb==undefined || 0 == cb || 0 == (""+str).length ) return "";
	if ( str==undefined ) str = " ";

    var sBld = (str + "");

    while (sBld.length * 2 < cb) sBld += sBld;

    if (sBld.length < cb)
        sBld += bldStr(cb - sBld.length, str);
    
    return sBld;
}


// Pads the base arg with as many pad arg's to make a string who's length is equal to n.
// sPad is optional, if not present, a space is used by default.
// Requires:  bldStr
function padStr (base, n, pad)
{
	var cb = n - ("" + base).length;

	if (pad == undefined) pad = " ";

	if (cb > 0 )
	    return bldStr(cb, pad) + base;

    return ""+base;
}


// Formats a string into cbBlk sized sections, delimited by sPad, from the right
// if fRight flag is set, from the left if not.  All args are optional except sSrc.
function fmtSBlock (sSrc, cbBlk, sPad, fRight)
{
    var i;
    // Default parameters
    if (fRight == undefined) fRight = false;
    if (sPad == undefined) sPad = " ";
    if (cbBlk == undefined || 0==cbBlk) cbBlk = sSrc.length;

    var cbMod = fRight ? sSrc.length%cbBlk : 0;

    var sRet = sSrc.substring(0,cbMod);
    
    for (i=0; i<sSrc.length-cbMod; i+=cbBlk)
        sRet += (sPad + sSrc.substring(i+cbMod,i+cbMod+cbBlk));

    return sRet.substring(!cbMod*sPad.length,sRet.length);
}


// Returns the string arg's chars randomly aranged in cbChunk sized chuncks
// cbChunk is optional, defaults to 1
function unsort (sIn, cbChunk)
{
    if (cbChunk == undefined) cbChunk = 1;
	var sOut="", iStrt=0, iEnd=0;
    while (sIn.length)
    {
        iStrt = Math.floor(Math.random()*Math.ceil(sIn.length/cbChunk))*cbChunk;
        iEnd = iStrt+cbChunk;
        sOut += sIn.substring(iStrt,iEnd);
        sIn = sIn.substring(0,iStrt) + (iEnd<sIn.length ? sIn.substring(iEnd, sIn.length) : "");
    }
    return sOut;
}


// Parameterized random string generator
// cb is the number of characters you want the string to return [optional, defaults to 100].
// bfType is a bit mask specifying the characters you want the string to contain [optional, 
// defaults to alphanumeric].  sDomain [optional] is a user supplied source string.
// Requires: chr
function genRndStr (cb, bfType, sDomain)
{
    var i;
    var sSource = "";
    if ( sDomain != undefined && typeof sDomain == "string")
        sSource = sDomain;

    if (bfType == undefined || bfType > 0)
    {
        if (cb == undefined) cb = 100;
        if (bfType == undefined) bfType = 0x01|0x02|0x04;

        var rgfWhichChars = new Array(256);
        for (i=1; i<rgfWhichChars.length; i++)
            rgfWhichChars[i] = 0;

        if (bfType & 0x01) // ucase alpha
            for (i=0x41; i<=0x5A; i++) rgfWhichChars[i]=1;

        if (bfType & 0x02) // lcase alpha
            for (i=0x61; i<=0x7A; i++) rgfWhichChars[i]=1;

        if (bfType & 0x04) // numbers
            for (i=0x30; i<=0x39; i++) rgfWhichChars[i]=1;

        if (bfType & 0x08) // all other printable chars (non-extended)
        {
            for (i=0x21; i<=0x2E; i++) rgfWhichChars[i]=1;
            for (i=0x3A; i<=0x40; i++) rgfWhichChars[i]=1;
            for (i=0x5B; i<=0x60; i++) rgfWhichChars[i]=1;
            for (i=0x7B; i<=0x7E; i++) rgfWhichChars[i]=1;
        }
        if (bfType & 0x10) // space
            rgfWhichChars[0x20]=1;

        if (bfType & 0x20) // tab
            rgfWhichChars[0x09]=1;

        if (bfType & 0x40) // newline
            rgfWhichChars[0x0A]=1;

        if (bfType & 0x80) // all other special chars
        {
            rgfWhichChars[0x08]=1; // backspace
            rgfWhichChars[0x0C]=1; // formfeed
            rgfWhichChars[0x0D]=1; // carriage return
        }
        if (bfType & 0x100) // non-printable non-extended chars
        {
            for (i=0x01; i<=0x07; i++) rgfWhichChars[i]=1;
                                           rgfWhichChars[0x0B]=1;
            for (i=0x0E; i<=0x1F; i++) rgfWhichChars[i]=1;
                                           rgfWhichChars[0x7F]=1;
        }
        if (bfType & 0x200) // extended chars (0x7f-0xff)
            for (i=0x80; i<=0xff; i++) rgfWhichChars[i]=1;

        // Create restricted char string
        for (i=0x00; i<0xff; i++)
            if (rgfWhichChars[i]) sSource += chr(i);
    }

    // Create random result string from restricted char string
    var sRet = "";
    for (i=0; i<cb; i++)
        sRet += sSource.charAt( Math.floor(Math.random()*sSource.length) );

    return sRet;
}

function baseDigi(digit,base) {
    var sDigits = "0123456789abcdefghijklmnopqrstuvwxyz";
    if (digit < base)
        return sDigits.charAt(digit);
    return ""
}

function decToBaseX(num,base)
{
    if (base>36) return null;

    if (num < 0)
        return "-"+decToBaseX((""+num).substring(1,(""+num).length));

    if (num < base)
        return baseDigi(num,base);
    else
        return decToBaseX(Math.floor(num/base),base) + baseDigi(num%base,base);
}

function verify(sCat, ob, sExp, bugNum) {
    if (bugNum == null) bugNum = "";

    var sAct = parseInt(ob,13);

    if (sAct + ""!= sExp + "")
        apLogFailInfo( m_scen+(sCat.length?"--"+sCat:"")+" failed", sExp, sAct,bugNum);
}


@if(!@aspx)
	function obFoo() {};
@else
	expando function obFoo() {};
@end

var m_scen = "";

function psint032() {
 @if(@_fast)
    var i, ich, iDec, num, sNum, cbNum, nRnd, sRnd, ExpNum, sDomain;
 @end
    apInitTest("psInt032 ");

    var INT_MIN =  -4294967295;
    var INT_MAX =  4294967295;

    //----------------------------------------------------------------------------
    apInitScenario("1. number, decimal, integer");

    m_scen = "number, decimal, integer";

    verify("min pos: 1",1,1, null);
    verify("min pos: +1",+1,1, null);
    verify("min pos < n < max pos: 42",42,54, null);
    verify("min pos < n < max pos: 148",148,229, null);
    verify("min pos < n < max pos: 1446",1446,2931, null);
    verify("min pos < n < max pos: 64688",64688,181280, null);
    verify("min pos < n < max pos: 983474",983474,3577487, null);
    verify("min pos < n < max pos: 8983524",8983524,42192063, null);
    verify("min pos < n < max pos: 25288939",25288939,150621298, null);
    verify("min pos < n < max pos: 463069357",463069357,3654086131, null);
    verify("min pos < n < max pos: 1234567890",1234567890,12445558281, null);
    verify("max pos: 2147483647",2147483647,22311232522, null);

    verify("max neg: -1",-1,-1, null);
    verify("min neg < n < max neg: -21",-21,-27, null);
    verify("min neg < n < max neg: -609",-609,-1023, null);
    verify("min neg < n < max neg: -6759",-6759,-14439, null);
    verify("min neg < n < max neg: -89429",-89429,-248972, null);
    verify("min neg < n < max neg: -401449",-401449,-1488106, null);
    verify("min neg < n < max neg: -7909313",-7909313,-37149596, null);
    verify("min neg < n < max neg: -47008400",-47008400,-284799983, null);
    verify("min neg < n < max neg: -404860839",-404860839,-3285373230, null);

    verify("min neg < n < max neg: -1234567890",-1234567890,-12445558281, null);
    verify("min neg: -2147483648",-2147483648,-22311232523, null);

    verify("pos zero",0,0, null);
    verify("neg zero",-0,0, null);
   

    //----------------------------------------------------------------------------
    apInitScenario("2. number, decimal, float");

    m_scen = "number, decimal, float";

    verify("min pos: 2.2250738585072014e-308",2.2250738585072014e-308, 2, null);

    // The following creates a numeric string for each of the possible
    // float lengths for nums between the min pos and the max pos floats

        // Loop through each possible string length (precision) for a float
        for (cbNum=1; cbNum<=15; cbNum++)
        {
            // Create a unique NS for each possible decimal place
            for (iDec=0; iDec<=cbNum+1; iDec++) // decimal places
            {
                sNum="";
                // Create the NS
                for (ich=0; ich<cbNum; ich++)
                {
                    // Get the current digit
                    for (;;)
                    {
                        nRnd = Math.floor(Math.random()*10);
                        if (ich>0 || iDec==1 || nRnd > 0) break;  // Note, relies on short-circuit logic
                    }

                    sNum += nRnd;
                }

                if (iDec != cbNum+1)
                    sNum = sNum.substring(0,iDec)+"."+sNum.substring(iDec,sNum.length);
                else if ( eval(sNum) < 2147483647) break;

                num = decToBaseX(sNum,13);
                
                ExpNum = sNum.substring(0,iDec);
                if (ExpNum=="") ExpNum = 0;

              //  if (eval(sNum)>2147483647)
              //      ExpNum = 2147483647;

                verify("min pos < n < max pos: "+sNum,num,ExpNum, null);
            }
        }


    verify("max pos",1.7976931348623158e308,1, null);
    verify("max pos",1.7976931348623158e+308,1, null);
    verify("> max pos float (1.#INF)",1.797693134862315807e309,Number.NaN, null);

    verify("max neg",-2.225073858507202e-308,-2, null);

    // The following creates a numeric string for each of the possible
    // float lengths for nums between the min neg and the max neg floats

        for (cbNum=1; cbNum<=15; cbNum++)
        {
            for (iDec=0; iDec<=cbNum+1; iDec++)
            {
                sNum="";
                for (ich=0; ich<cbNum; ich++)
                {
                    for (;;)
                    {
                        nRnd = Math.floor(Math.random()*10);
                        if (ich>0 || iDec==1 || nRnd > 0) break;
                    }
                    sNum += nRnd;
                }

                if (iDec != cbNum+1)
                    sNum = sNum.substring(0,iDec)+"."+sNum.substring(iDec,sNum.length);
                else if ( eval(sNum) < 2147483648) break;

                num = "-"+decToBaseX(sNum,13);
                
                ExpNum = "-"+sNum.substring(0,iDec);
                if (ExpNum=="-" || ExpNum=="-0") ExpNum = 0;

              //  if (eval(sNum)>2147483647)
              //      ExpNum = INT_MIN;

                verify("min pos < n < max pos: "+sNum,num,ExpNum, null);
            }
        }

    verify("min neg",-1.797693134862315807e308, -1, null);
    verify("< min neg float (-1.#INF)",-1.797693134862315807e309,Number.NaN, null);

    verify("pos zero",0.0,0, null);
    verify("neg zero",-0.0,0, null);


    //----------------------------------------------------------------------------
    apInitScenario("3. number, hexidecimal");

    m_scen = "number, hexidecimal";

    verify("min pos: 0x1",0x1,1, null);
    verify("min pos < n < max pos: 0xC",0xC,15, null);
    verify("min pos < n < max pos: 0x14",0x14,26, null);
    verify("min pos < n < max pos: 0x562",0x562,2803, null);
    verify("min pos < n < max pos: 0x9a40",0x9a40,106244, null);
    verify("min pos < n < max pos: 0xD4Bb9",0xD4Bb9,3173043, null);
    verify("min pos < n < max pos: 0x17020a",0x17020a,6700070, null);
    verify("min pos < n < max pos: 0x269aaf0",0x269aaf0,252699709, null);
    verify("min pos < n < max pos: 0xA7a864cf",0xA7a864cf,27810304230, null);
    verify("max pos: 0xffffffff",0xffffffff,44637031665, null);

    verify("max neg: -0x1",-0x1,-1, null);
    verify("min neg < n < max neg: -0x2",-0x2,-2, null);
    verify("min neg < n < max neg: -0xfC",-0xfC,-405, null);
    verify("min neg < n < max neg: -0xC07",-0xC07,-6691, null);
    verify("min neg < n < max neg: -0x54e6",-0x54e6,-60545, null);
    verify("min neg < n < max neg: -0x41061",-0x41061,-927687, null);
    verify("min neg < n < max neg: -0xb15870",-0xb15870,-69865460, null);
    verify("min neg < n < max neg: -0xf6BfE57",-0xf6BfE57,-1986515404, null);
    verify("min neg < n < max neg: -0xA347599c",-0xA347599c,-27152090606, null);
    verify("min neg: -0xffffffff",-0xffffffff,-44637031665, null);

    verify("pos zero",0x0,0, null);
    verify("neg zero",-0x0,0, null);


    //----------------------------------------------------------------------------
    apInitScenario("4. number, octal");

    m_scen = "number, octal";

    verify("min pos",01,1, null);
    verify("min pos < n < max pos: 02",02, 2, null);
    verify("min pos < n < max pos: 072",072, 73, null);
    verify("min pos < n < max pos: 0461",0461, 512, null);
    verify("min pos < n < max pos: 06706",06706, 7468, null);
    verify("min pos < n < max pos: 050661",050661, 58659, null);
    verify("min pos < n < max pos: 0302611",0302611, 278032, null);
    verify("min pos < n < max pos: 02303116",02303116, 2294012, null);
    verify("min pos < n < max pos: 042152134",042152134, 42143270, null);
    verify("min pos < n < max pos: 0324413222",0324413222, 340507045, null);
    verify("min pos < n < max pos: 05766757103",05766757103, 6538945002, null);
    verify("min pos < n < max pos: 021013604067",021013604067, 23364590796, null);
    verify("max pos: 037777777777",037777777777, 44637031665, null);

    verify("max neg: -01",-01, -1, null);
    verify("min neg < n < max neg: -07",-07, -7, null);
    verify("min neg < n < max neg: -021",-021, -20, null);
    verify("min neg < n < max neg: -0332",-0332, -359, null);
    verify("min neg < n < max neg: -07777",-07777, -8910, null);
    verify("min neg < n < max neg: -044221",-044221, -47080, null);
    verify("min neg < n < max neg: -0201615",-0201615, -185281, null);
    verify("min neg < n < max neg: -04506536",-04506536, -5610340, null);
    verify("min neg < n < max neg: -015257715",-015257715, -16239590, null);
    verify("min neg < n < max neg: -0521544023",-0521544023, -542526357, null);
    verify("min neg < n < max neg: -02330412042",-02330412042, -2597461665, null);
    verify("min neg < n < max neg: -035164014173",-035164014173, -39292901313, null);
    verify("min neg: -037777777777",-037777777777, -44637031665, null);

    verify("pos zero",00, 0, null);
    verify("neg zero",-00, 0, null);

    
    //----------------------------------------------------------------------------
    apInitScenario("5. num string, decimal, integer");

    m_scen = "number, decimal, integer";

    verify("min pos: 1","1",1, null);
    verify("min pos: +1","+1",1, null);
    verify("min pos < n < max pos: 42","42",54, null);
    verify("min pos < n < max pos: 148","148",229, null);
    verify("min pos < n < max pos: 1446","1446",2931, null);
    verify("min pos < n < max pos: 64688","64688",181280, null);
    verify("min pos < n < max pos: 983474","983474",3577487, null);
    verify("min pos < n < max pos: 8983524","8983524",42192063, null);
    verify("min pos < n < max pos: 25288939","25288939",150621298, null);
    verify("min pos < n < max pos: 463069357","463069357",3654086131, null);
    verify("min pos < n < max pos: 1234567890","1234567890",12445558281, null);
    verify("max pos: 2147483647","2147483647",22311232522, null);

    verify("max neg: -1","-1",-1, null);
    verify("min neg < n < max neg: -21","-21",-27, null);
    verify("min neg < n < max neg: -609","-609",-1023, null);
    verify("min neg < n < max neg: -6759","-6759",-14439, null);
    verify("min neg < n < max neg: -89429","-89429",-248972, null);
    verify("min neg < n < max neg: -401449","-401449",-1488106, null);
    verify("min neg < n < max neg: -7909313","-7909313",-37149596, null);
    verify("min neg < n < max neg: -47008400","-47008400",-284799983, null);
    verify("min neg < n < max neg: -404860839","-404860839",-3285373230, null);

    verify("min neg < n < max neg: -1234567890","-1234567890",-12445558281, null);
    verify("min neg: -2147483647","-2147483648",-22311232523, null);

    verify("pos zero","0",0, null);
    verify("neg zero","-0",0, null);


    //----------------------------------------------------------------------------
    apInitScenario("6. num string, decimal, float");

    m_scen = "num string, decimal, float";

    verify("min pos: 2.2250738585072014e-308","2.2250738585072014e-308", 2, null);

    // The following creates a numeric string for each of the possible
    // float lengths for nums between the min pos and the max pos floats

        // Loop through each possible string length (precision) for a float
        for (cbNum=1; cbNum<=15; cbNum++)
        {
            // Create a unique NS for each possible decimal place
            for (iDec=0; iDec<=cbNum+1; iDec++) // decimal places
            {
                sNum="";
                // Create the NS
                for (ich=0; ich<cbNum; ich++)
                {
                    // Get the current digit
                    for (;;)
                    {
                        nRnd = Math.floor(Math.random()*10);
                        if (ich>0 || iDec==1 || nRnd > 0) break;  // Note, relies on short-circuit logic
                    }

                    sNum += nRnd;
                }

                if (iDec != cbNum+1)
                    sNum = sNum.substring(0,iDec)+"."+sNum.substring(iDec,sNum.length);
                else if ( eval(sNum) < 2147483647) break;

                num = decToBaseX(sNum,13);
                
                ExpNum = sNum.substring(0,iDec);
                if (ExpNum=="") ExpNum = 0;

                //if (eval(sNum)>2147483647)
                //    ExpNum = 2147483647;

                verify("min pos < n < max pos: "+sNum,num,ExpNum, null);
            }
        }

    verify("max pos","1.7976931348623158e308",1, null);
    verify("max pos","1.7976931348623158e+308",1, null);
    verify("> max pos float (1.#INF)","1.797693134862315807e309",1, null);

    verify("max neg","-2.2250738585072012595e-308",-2, null);

    // The following creates a numeric string for each of the possible
    // float lengths for nums between the min neg and the max neg floats

        for (cbNum=1; cbNum<=15; cbNum++)
        {
            for (iDec=0; iDec<=cbNum+1; iDec++)
            {
                sNum="";
                for (ich=0; ich<cbNum; ich++)
                {
                    for (;;)
                    {
                        nRnd = Math.floor(Math.random()*10);
                        if (ich>0 || iDec==1 || nRnd > 0) break;
                    }
                    sNum += nRnd;
                }

                if (iDec != cbNum+1)
                    sNum = sNum.substring(0,iDec)+"."+sNum.substring(iDec,sNum.length);
                else if ( eval(sNum) < 2147483648) break;

                num = "-"+decToBaseX(sNum,13);
                
                ExpNum = "-"+sNum.substring(0,iDec);
                if (ExpNum=="-" || ExpNum=="-0") ExpNum = 0;

              //  if (eval(sNum)>2147483647)
              //      ExpNum = INT_MIN;

                verify("min pos < n < max pos: "+sNum,num,ExpNum, null);
            }
        }

    verify("min neg","-1.797693134862315807e308",-1, null);
    verify("< min neg float (-1.#INF)","-1.797693134862315807e309",-1, null);

    verify("pos zero","0.0",0, null);
    verify("neg zero","-0.0",0, null);

       
    //----------------------------------------------------------------------------
    apInitScenario("7. num string, hexidecimal");

    m_scen = "num string, hexidecimal";

    verify("min pos: 0x1","0x1",0, null);
    verify("min pos < n < max pos: 0xC","0xC",0, null);
    verify("min pos < n < max pos: 0x14","0x14",0, null);
    verify("min pos < n < max pos: 0x562","0x562",0, null);
    verify("min pos < n < max pos: 0x9a40","0x9a40",0, null);
    verify("min pos < n < max pos: 0xD4Bb9","0xD4Bb9",0, null);
    verify("min pos < n < max pos: 0x17020a","0x17020a",0, null);
    verify("min pos < n < max pos: 0x269aaf0","0x269aaf0",0, null);
    verify("min pos < n < max pos: 0xA7a864cf","0xA7a864cf",0, null);
    verify("max pos: 0xffffffff","0xffffffff",0, null);
    verify("max pos: 0xFFFFFFFF","0xFFFFFFFF",0, null);
    verify("max pos: 0xFFfFffFf","0xFFfFffFf",0, null);

    verify("max neg: -0x1","-0x1",0, null);
    verify("min neg < n < max neg: -0x2","-0x2",0, null);
    verify("min neg < n < max neg: -0xfC","-0xfC",0, null);
    verify("min neg < n < max neg: -0xC07","-0xC07",0, null);
    verify("min neg < n < max neg: -0x54e6","-0x54e6",0, null);
    verify("min neg < n < max neg: -0x41061","-0x41061",0, null);
    verify("min neg < n < max neg: -0xb15870","-0xb15870",0, null);
    verify("min neg < n < max neg: -0xf6BfE57","-0xf6BfE57",0, null);
    verify("min neg < n < max neg: -0xA347599c","-0xA347599c",0, null);
    verify("min neg: -0xffffffff","-0xffffffff",0, null);
    verify("min neg: -0xFFFFFFFF","-0xFFFFFFFF",0, null);
    verify("min neg: -0xffFffFFF","-0xffFffFFF",0, null);

    verify("pos zero","0x0",0, null);
    verify("neg zero","-0x0",0, null);


    //----------------------------------------------------------------------------
    apInitScenario("8. num string, octal");

    m_scen = "num string, octal";

    verify("min pos","01",1, null);
    verify("min pos < n < max pos: 02","02", 2, null);
    verify("min pos < n < max pos: 072","072", 93, null);
    verify("min pos < n < max pos: 0461","0461", 755, null);
    verify("min pos < n < max pos: 06706","06706", 14371, null);
    verify("min pos < n < max pos: 050661","050661", 143898, null);
    verify("min pos < n < max pos: 0302611","0302611", 1119301, null);
    verify("min pos < n < max pos: 02303116","02303116", 10774276, null);
    verify("min pos < n < max pos: 042152134","042152134", 261166390, null);
    verify("min pos < n < max pos: 0324413222","0324413222", 2593517123, null);
    verify("min pos < n < max pos: 05766757103","05766757103", 59140821275, null);
    verify("min pos < n < max pos: 021013604067","021013604067", 286400948646, null);
    verify("max pos: 037777777777","037777777777", 493992929125, null);

    verify("max neg: -01","-01", -1, null);
    verify("min neg < n < max neg: -07","-07", -7, null);
    verify("min neg < n < max neg: -021","-021", -27, null);
    verify("min neg < n < max neg: -0332","-0332", -548, null);
    verify("min neg < n < max neg: -07777","-07777", -16660, null);
    verify("min neg < n < max neg: -044221","-044221", -123397, null);
    verify("min neg < n < max neg: -0201615","-0201615", -745815, null);
    verify("min neg < n < max neg: -04506536","-04506536", -21177773, null);
    verify("min neg < n < max neg: -015257715","-015257715", -87784533, null);
    verify("min neg < n < max neg: -0521544023","-0521544023", -4210956974, null);
    verify("min neg < n < max neg: -02330412042","-02330412042", -23845954641, null);
    verify("min neg < n < max neg: -035164014173","-035164014173", -467809539083, null);
    verify("min neg: -037777777777","-037777777777", -493992929125, null);

    verify("pos zero",00, 0, null);
    verify("neg zero",-00, 0, null);


    //----------------------------------------------------------------------------
    apInitScenario("9. Alpha string");

    m_scen = "alpha string";

    verify("single space"," ",Number.NaN, null);
    verify("multiple spaces","                                                                   ",Number.NaN, null);
    verify("as false","false",Number.NaN, null);
    verify("as single word","foo",Number.NaN, null);
    verify("as single word, leading space"," foo",Number.NaN, null);
    verify("as single word, trailing space","foo ",Number.NaN, null);
    verify("as multiple word","foo bar",Number.NaN, null);
    verify("as multiple word, leading space"," foo bar",Number.NaN, null);
    verify("as multiple word, trailing space","foo bar ",Number.NaN, null);
    verify("zls","",Number.NaN, null);


    //----------------------------------------------------------------------------
    apInitScenario("10. random int number+string (printable chars only, no base-13 chars)");

    m_scen = "random number+string, printable chars only";
    sDomain = "tGHIJKLMNOPQRSTUVWXYZghijklmnopqrstuvwxyz<>?,./;:{[}]`~!@#$%^&*()-_=+\|";

    for (i=0; i<10; i++)
    {
        num = Math.floor(Math.random()*2147483647);
        sRnd = decToBaseX(num,13)+genRndStr(100,0,sDomain);
        verify(sRnd,sRnd,num,null);
    }


    //----------------------------------------------------------------------------
    apInitScenario("11. random int number+string (non-printable chars)");

    m_scen = "random number+string, non-printable chars";

    for (i=0; i<10; i++)
    {
        num = Math.floor(Math.random()*2147483647);
        sRnd = decToBaseX(num,13)+genRndStr(100,0x100|0x200);
        verify(sRnd,sRnd,num,null);
    }
    

    //----------------------------------------------------------------------------
    apInitScenario("12. random string(printable chars only)+number");

    m_scen = "random string(printable chars only)+number";

    for (i=0; i<10; i++)
    {
        num = Math.floor(Math.random()*2147483647);
        sRnd = genRndStr(100,0,sDomain)+decToBaseX(num,13);
        verify(sRnd,sRnd,Number.NaN,null);
    }


    //----------------------------------------------------------------------------
    apInitScenario("13. random string(non-printable chars)+number");

    m_scen = "random string(non-printable chars)+number";

    for (i=0; i<10; i++)
    {
        num = Math.floor(Math.random()*2147483647);
        sRnd = genRndStr(100,0x100|0x200)+decToBaseX(num,13);
        verify(sRnd,sRnd,Number.NaN,null);
    }
    

    //----------------------------------------------------------------------------
    apInitScenario("14. objects, built-in, non-exec");

    m_scen = "objects, built-in, non-exec";

    verify("Math",Math,Number.NaN, null);


    //----------------------------------------------------------------------------
    apInitScenario("15. objects, built-in, exec, not inst");

    m_scen = "objects, built-in, exec, not inst";

    verify("Array",Array,Number.NaN, null);
    verify("Boolean",Boolean,Number.NaN, null);
    verify("Date",Date,Number.NaN, null);
    verify("Number",Number,Number.NaN, null);
    verify("Object",Object,Number.NaN, null);


    //----------------------------------------------------------------------------
    apInitScenario("16. objects, built-in, exec, inst");

    m_scen = "objects, built-in, exec, inst";

    verify("new Array()",new Array(),Number.NaN, null);
    verify("new Boolean()",new Boolean(),Number.NaN, null);
    verify("new Date()",new Date(),Number.NaN, null);
    verify("new Number()",new Number(),0, null);
    verify("new Object()",new Object(),Number.NaN, null);


    //----------------------------------------------------------------------------
    apInitScenario("17. objects, user-def, not inst");

    m_scen = "objects, user-def, not inst";

    verify("obFoo",obFoo,Number.NaN, null);


    //----------------------------------------------------------------------------
    apInitScenario("18. objects, user-def, inst");

    m_scen = "objects, user-def, inst";

    verify("new obFoo()",new obFoo(),Number.NaN, null);


    //----------------------------------------------------------------------------
    apInitScenario("19. constants");

    m_scen = "constants";

    verify("true",true,Number.NaN, null);
    verify("false",false,Number.NaN, null);


    //----------------------------------------------------------------------------
    apInitScenario("20. null");

    m_scen = "null";

    verify("",null,Number.NaN, null);


    //----------------------------------------------------------------------------
    apInitScenario("21. undefined");
    
    m_scen = "undefined";

    var obUndef;
    verify("",obUndef,Number.NaN, null);


    apEndTest();

}


psint032();


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
