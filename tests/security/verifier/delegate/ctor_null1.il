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
// ctor_null1.il
//
// NEGATIVE
// pass null as 'this' argument
.assembly extern mscorlib { }
.assembly ctor_null1 { }
.class private auto ansi sealed MyDelegate
       extends [mscorlib]System.MulticastDelegate
{
  .method public hidebysig specialname rtspecialname 
          instance void  .ctor(object 'object',
                               native unsigned int 'method') runtime managed { }
  .method public hidebysig virtual instance void
          Invoke() runtime managed { }
  .method public hidebysig newslot virtual 
          instance class [mscorlib]System.IAsyncResult 
          BeginInvoke(class [mscorlib]System.AsyncCallback callback,
                      object 'object') runtime managed { }
  .method public hidebysig newslot virtual 
          instance void  EndInvoke(class [mscorlib]System.IAsyncResult result) runtime managed { }
}
.class public auto ansi beforefieldinit TestClass
       extends [mscorlib]System.Object
{
  .method public hidebysig instance void DelegateFunc() il managed
  {
    .maxstack  8
    IL_0000:  ret
  }
  .method public hidebysig static void  Main(class System.String[] args) il managed
  { 
    .entrypoint
    .maxstack  3
    .locals init (class MyDelegate V_0, int32 V_1)
    .try {
      .try {
           IL_0000:  ldnull
           IL_0001:  ldftn      instance void TestClass::DelegateFunc()
           IL_0007:  newobj     instance void MyDelegate::.ctor(object,
                                                                native unsigned int)
           IL_000c:  stloc.0
                     leave.s EndTry
           }
      catch ['mscorlib']System.NullReferenceException 
           {
               pop
               ldstr      "TEST PASSED"
               call       void ['mscorlib']System.Console::WriteLine(string)
               ldc.i4.0
               stloc      V_1
               leave.s    EndTry
           }  
      EndTry: leave.s EndIL
    }  
    catch ['mscorlib']System.Exception 
    {
        pop
        ldstr      "TEST FAILED"
        call       void ['mscorlib']System.Console::WriteLine(string)
        ldc.i4.1
        stloc      V_1
        leave.s    EndIL
    }  
    EndIL:    ldloc V_1
              call       void [mscorlib]System.Environment::set_ExitCode(int32)
    IL_000d:  ret
  }
  .method public hidebysig specialname rtspecialname 
          instance void .ctor() il managed
  {
    .maxstack  8
    IL_0000:  ldarg.0
    IL_0001:  call       instance void [mscorlib]System.Object::.ctor()
    IL_0006:  ret
  }
}
