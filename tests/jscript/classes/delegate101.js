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


var iTestID = 220518;

//////////////////////////////////////////////////////////////////////////////////////
//

//      			
//
//////////////////////////////////////////////////////////////////////////////////////

@if(!@aspx)
	import System.Threading
@else
	</script>
	<%@ import namespace="System.Threading" %>
	<script language=jscript runat=server>
@end


class del1{
  var info = 0
  static var sinfo = 0
  function th1(){
    info = 1
  }
  static function th2(){
    del1.sinfo = 3
  }
  protected function th3(){
    info = 5
  }
}

class del1exp extends del1{
  function getfun():Thread{
    return new Thread(th3)
  }
}


package pkg1{
  class del2{
    var info = 0
    static var sinfo = 0
    function th1(){
      info = 1
    }
    static function th2(){
      del2.sinfo = 3
    }
    protected function th3(){
      info = 5
    }
  }

  class del2exp extends del2{
    function getfun():Thread{
      return new Thread(th3)
    }
  }
}












function delegate101() {

  apInitTest("delegate101");
  var th :Thread
  var x
  var cls1 :del1
  var cls2 :pkg1.del2

@if(@_fast)
  apInitScenario("create a Thread using a method")
  cls1 = new del1
  th = new Thread(cls1.th1)
  th.Start()
  Thread.Sleep(500)
  if (cls1.info != 1) apLogFailInfo("thread did not report", 1, cls1.info, "")

/*  TODOPGM
  x = new del1
  th = new Thread(x.th1)
  th.Start()
  Thread.Sleep(500)
  if (x.info != 1) apLogFailInfo("thread did not report", 1, x.info, "")
*/



  apInitScenario("create a Thread using a static method")
  th = new Thread(del1.th2)
  th.Start()
  Thread.Sleep(500)
  if (del1.sinfo != 3) apLogFailInfo("thread did not report", 3, del1.sinfo, "")




  apInitScenario("create a Thread using an protected method")
  var cls1exp = new del1exp
  th = cls1exp.getfun()
  th.Start()
  Thread.Sleep(500)
  if (cls1exp.info != 5) apLogFailInfo("thread did not report", 5, cls1exp.info, "")


/*  TODOPGM
  x = new del1
  th = new Thread(x.th1)
  th.Start()
  Thread.Sleep(500)
  if (x.info != 1) apLogFailInfo("thread did not report", 1, x.info, "")
*/





  apInitScenario("create a Thread using a method (package)")
  cls2 = new pkg1.del2
  th = new Thread(cls2.th1)
  th.Start()
  Thread.Sleep(500)
  if (cls2.info != 1) apLogFailInfo("thread did not report", 1, cls2.info, "")

/*  TODOPGM
  x = new del1
  th = new Thread(x.th1)
  th.Start()
  Thread.Sleep(500)
  if (x.info != 1) apLogFailInfo("thread did not report", 1, x.info, "")
*/



  apInitScenario("create a Thread using a static method (package)")
  th = new Thread(pkg1.del2.th2)
  th.Start()
  Thread.Sleep(500)
  if (pkg1.del2.sinfo != 3) apLogFailInfo("thread did not report", 3, pkg1.del2.sinfo, "")




  apInitScenario("create a Thread using an protected method (package)")
  var cls2exp = new pkg1.del2exp
  th = cls2exp.getfun()
  th.Start()
  Thread.Sleep(500)
  if (cls2exp.info != 5) apLogFailInfo("thread did not report", 5, cls2exp.info, "")


/*  TODOPGM
  x = new del1
  th = new Thread(x.th1)
  th.Start()
  Thread.Sleep(500)
  if (x.info != 1) apLogFailInfo("thread did not report", 1, x.info, "")
*/
@else
/**/apInitScenario("1) create a Thread using a method")
/**/apInitScenario("2) create a Thread using a static method")
/**/apInitScenario("3) create a Thread using an protected method")
/**/apInitScenario("4) create a Thread using a method (package)")
/**/apInitScenario("5) create a Thread using a static method (package)")
/**/apInitScenario("6) create a Thread using an protected method (package)")
@end     

  apEndTest()
}


delegate101();


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
