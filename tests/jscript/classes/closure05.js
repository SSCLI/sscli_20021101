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


var iTestID = 227435;

//////////////////////////////////////////////////////////////////////////////
//		Trying to create method closures
//
//

//		
//
//


class cls1{
  var x = 5
  function fun1(){
    return x
  }
  function fun2(){
    return this.fun1
  }
}

function basefun1(){
  var x = 6
  function nestfun1(){
    return x
  }
  return nestfun1
}

class cls3{
  var q
  var _x
  function get x(){
    return _x
  }
  function set x(invar:Object){
    function default1(){
      return q
    }
    if (invar == null) _x = default1
    else _x = invar
  }
  function GetGet(){
    return x
  }
}

class cls4{
  var q = 6
  function CloseClass(){
    function nestfun1(){
      return q
    }
    return nestfun1
  }
  function CloseMethod(){
    var r = 15
    function nestfun2(){
      return r
    }
    return nestfun2
  }
  function CloseClass2(){
    function nestfun3(){
      return this
    }
    return nestfun3
  }
  function CloseClass3(){
    var inst = this
    function nestfun4(){
      return inst
    }
    return nestfun4
  }
}
var xyz = 123

//  call  apply



function closure05() {


  apInitTest("closure05");
  var a, x

  

  apInitScenario("functionWrapper of one method another")
  var c1 = new cls1
  var f1 = c1.fun2()
  if (f1() != 5) apLogFailInfo("early/early bound closure of method failed", 5, f1(), "")
  x = c1.fun2()
  if (x() != 5) apLogFailInfo("early/late bound closure of method failed", 5, x(), "")

  a = new cls1
  var f2 = a.fun2()  
  if (f2() != 5) apLogFailInfo("late/early bound closure of method failed", 5, f2(), "")
  x = a.fun2()
  if (x() != 5) apLogFailInfo("late/late bound closure of method failed", 5, x(), "")



  apInitScenario("closure around a function from nested function")
  var f3 = basefun1()
  x = basefun1()
  if (f3() != 6) apLogFailInfo("early bound closure of method failed", 6, f3(), "")
  if (x() != 6) apLogFailInfo("late bound closure of method failed", 6, x(), "")



  apInitScenario("closure within property")
  var c3 = new cls3
  c3.x = null
  c3.q = 5
  if (c3.x() != 5) apLogFailInfo("closure within property failed", 5, c3.x(), "")
  c3.q = 3
  function funny(){return this.q +1}
  c3.x = funny
  if (c3.x() != 4) apLogFailInfo("closure within property failed", 4, c3.x(), "")
  c3.x = null
  if (c3.x() != 3) apLogFailInfo("closure within property failed", 3, c3.x(), "")



  apInitScenario("closure of property getter")
  var fun3 = c3.GetGet()
  c3.q = 9
  if (fun3() != 9) apLogFailInfo("closure of property getter failed", 9, fun3(), "")
  if (fun3.ToString() != "function default1(){" + Environment.NewLine + "      return q" + Environment.NewLine + "    }")
    apLogFailInfo("closure of property getter failed: code", 
    "function default1(){" + Environment.NewLine + "      return q" + Environment.NewLine + "    }", fun3.ToString(), "")
  if (fun3.GetType() != "Microsoft.JScript.Closure")
    apLogFailInfo("closure of property getter failed: type", 3, fun3.GetType(), "")
  
  /*  PGMTODOPGM
  var fun4:Microsoft.JScript.Closure = c3.GetGet()
  c3.q = 9
  if (fun4() != 9) apLogFailInfo("closure of property getter failed", 9, fun4(), "")
  if (fun4.ToString() != "function default1(){" + Environment.NewLine + "      return q" + Environment.NewLine + "    }")
    apLogFailInfo("closure of property getter failed: code", 
    "function default1(){" + Environment.NewLine + "      return q" + Environment.NewLine + "    }", fun4.ToString(), "")
  if (fun4.GetType() != "Microsoft.JScript.Closure")
    apLogFailInfo("closure of property getter failed: type", 3, fun4.GetType(), "")
*/

  apInitScenario("closure within a method")
  var c4 : cls4 = new cls4
  var f4 = c4.CloseClass()
  if (f4() != 6) apLogFailInfo("closure of property getter failed", 6, f4(), "")
  var f4 = c4.CloseMethod()
  if (f4() != 15) apLogFailInfo("closure of property getter failed", 15, f4(), "")

  var f5 = c4.CloseClass2()
  if (f5().xyz != 123) apLogFailInfo("closure of property getter failed", 123, f5().xyz, "")

  var f6 = c4.CloseClass3()
  if (f6().q != 6) apLogFailInfo("closure of property getter failed", 6, f6().q, "")



  apEndTest();


}


closure05();


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
