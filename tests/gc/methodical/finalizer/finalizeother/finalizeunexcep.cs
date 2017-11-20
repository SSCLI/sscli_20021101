// ==++==
//
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
//
// ==--==

// Tests Unhandled Exception in Finalize() 
// All exceptions on the finalizer thread are handled by EE.  
// There is a try/catch before a call to the finalize method.


using System;

public class Test {

	public class List {
		public int val;
		public List next;
	}
	public class Dummy {

		public static bool visited;
	
		~Dummy() {
			List lst = new List();
			Console.WriteLine("In Finalize() of Dummy");
			int i=lst.next.val;    // should throw nullreference exception
			visited=true;		// <-------- should not reach here
		}
	}

	public static void Main() {

		Dummy obj = new Dummy();
		obj=null;

		GC.Collect();
		GC.WaitForPendingFinalizers();  // makes sure Finalize() is called.

		if(Dummy.visited == false) {
			Environment.ExitCode = 0;
			Console.WriteLine("Test for Unhandled Exception in Finalize() passed!");
		}
		else {
			Environment.ExitCode = 1;
			Console.WriteLine("Test for Unhandled Exception in Finalize() failed!");
		}
	}
}
