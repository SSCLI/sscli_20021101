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


var iTestID = 221448;

/*----------------------------
Test: dotnet11
Product: JScript
Area: System
Purpose: check using System delegate type.
Notes:

-------------------------------------------------*/

var sAct="";
var sExp = "";
var sActErr = "";
var sExpErr = "";
var sErrThr = false;

function verify(sRes, sExp, sMsg, sBug){
	if (sRes != sExp)
		apLogFailInfo(sMsg, sExp, sRes, sBug);
}


@if(@aspx)
	</script>
	<%@ import namespace="System.Threading" %>
	<script language=jscript runat=server>
@else
	import System
	import System.Threading
@end


//PGMTODOPGM
/*
class strDel extends MulticastDelegate
{
}
*/  

class A 
{
	var e: EventHandler;
	var x: int = 1;
	static var y: int = -100;
	function test(o:Object, e:EventArgs):void 	
	{
		x += 100;
	}
}

class B 
{	
	var e: EventHandler;
	function test(o:Object, e:EventArgs): void
	{
		A.y -= 100;
	}
}

// put this into a separate package.

package p_event
{
	class C 
	{
		var e: EventHandler;
		static var s: String = "class c";
		function test(o:Object, e: EventArgs) : void
		{
			s = "class c in package";
		}
	}
}

import p_event;

class D
{
	var s: String = "abc";
	
	function anything() 
	{
		print(s);
	}
}	


class ServerClass 
{
	public function InstanceMethod() 
	{
		//print("ServerClass.InstanceMethod is running on another thread");
		// Pause for a moment to provide a delay to make threads more apparent.
      		Thread.Sleep(1000);
		//print("The instance method called by the worker thread has ended");
	}
	public static function StaticMethod()
	{
		//print("ServerClass.StaticMethod is running on another thread");
		Thread.Sleep(3000);
		//print("The static method called by the worker thread has ended");
	}
}

class simple 
{
	public static function Main() : int
	{
		//print("Thread Simple Sample");
		var serverObject: ServerClass = new ServerClass();
		var instanceCaller : Thread = new Thread(serverObject.InstanceMethod);
		instanceCaller.Start();
		//print("The Main() thread calls this after starting the new InstanceCaller thread");
		var staticCaller : Thread = new Thread(ServerClass.StaticMethod);
		staticCaller.Start();
		//print("The Main() thread calls this after starting the new StaticCaller thread");
		return 0;
	}
}
		
		

function dotnet11() 
{

	apInitTest (" dotnet11 : test using System delegate type");

	apInitScenario ("1. EventHandler in function ");
	var a: A = new A();
	var b: B = new B();
@if (@_fast) //bug 314986
	var d1: EventHandler = a.test;
	var d2: EventHandler = b.test;
	d1(null,null);
	if (a.x != 101)  apLogFailInfo ("1. EventHandler in function ", 101, a.x,"")
	d2(null,null);
	if (A.y != -200)  apLogFailInfo ("1. EventHandler in function ", -200, A.y,"")
@end
	apInitScenario ("2. EventHandler in class ");
@if (@_fast)
	a.e = b.test;
	b.e = a.test;
	a.e(null,null);
	if (A.y != -300) apLogFailInfo ("2. EventHandler in class ", -300, A.y,"")
	b.e(null,null);
	if (a.x != 201)  apLogFailInfo ("2. EventHandler in class ", 201, a.x,"")
@end
	apInitScenario ("3. Event handler in package");
@if (@_fast)
	var c: C = new p_event.C();
	a.e = c.test;
	a.e(null,null);
	if (p_event.C.s != "class c in package") apLogFailInfo ("3. Event handler in package","class c in package",p_event.C.s,"")
	c.e = a.test;
	c.e(null,null);
	if (a.x != 301)  apLogFailInfo ("3. Event handler in package ", 301, a.x,"")
@end
	
	apInitScenario ("4. using ThreadStart");
	try {
		eval (" var d: D = new D();"+
		"var thStat: ThreadStart = d.anything;"+
		"var thread: Thread = new Thread(thStat);"+
 		"thread.Start();");
	}
	catch (error)
	{
		sErrThr = true;
		sActErr = error;
	}
	if (sErrThr)
		apLogFailInfo ("4. using ThreadStart", "should not have error", sActErr, "");
	
	
	apInitScenario ("5. creating thread");
	sErrThr = false;
	sActErr = "";
	try {
		eval ("simple.Main();")
	}
	catch (error)
	{
		sErrThr = true;
		sActErr = error;
	}
@if (@_fast)
	if (sErrThr)
		apLogFailInfo ("5. creating thread", "should not have error", sActErr, "");
@end	

      apEndTest()
}



dotnet11();


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
