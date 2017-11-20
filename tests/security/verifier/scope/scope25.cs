using System;
using System.Security;
[assembly:AllowPartiallyTrustedCallersAttribute] 

public class TestClass{

	public static int Main(string[] args){
          try {
          Console.WriteLine("initial field value: " + A.getField()) ;
          B.C.foo(5);		
          Console.WriteLine("final field value: " + A.getField()) ;
          return 0;
          }
          catch {
          return 1;
          }
	}

}

public class A{

protected static int protInt=0;
public static int getField(){
	return protInt;
}

}

public class B : A{


public class C{

  public static void foo(int i){
	A.protInt=i;
  }

}

}

