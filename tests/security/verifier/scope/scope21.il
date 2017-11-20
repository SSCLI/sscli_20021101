//-desc: access to public nested B defined inside private A
//  Microsoft (R) .NET Framework IL Disassembler.  Version 1.0.2729.0
//  Copyright (C) Microsoft Corp. 1998-2001. All rights reserved.

// VTableFixup Directory:
// No data.
.assembly extern mscorlib
{
  .publickeytoken = (B7 7A 5C 56 19 34 E0 89 )                         // .z\V.4..
  .ver 1:0:2411:0
}
.assembly scope21
 { 
.custom instance void [mscorlib]System.Security.AllowPartiallyTrustedCallersAttribute::.ctor() = ( 01 00 00 00 )
 
  // --- The following custom attribute is added automatically, do not uncomment -------
  //  .custom instance void [mscorlib]System.Diagnostics.DebuggableAttribute::.ctor(bool,
  //                                                                                bool) = ( 01 00 00 01 00 00 ) 
  .hash algorithm 0x00008004
  .ver 0:0:0:0
}
 
.module scope21.exe
// MVID: {959F307A-CF8E-4474-ABA2-77B5B2B2DB8F}
.subsystem 0x00000003
.file alignment 512
.corflags 0x00000001
// Image base: 0x03540000
.class public auto ansi beforefieldinit TestClass
       extends [mscorlib]System.Object
{
  .method public hidebysig static void  Main(string[] args) cil managed
  {
    .entrypoint
    // Code size       7 (0x7)
    .maxstack  1
    .locals init (class A/B V_0)
    IL_0000:  newobj     instance void A/B::.ctor()
    IL_0005:  stloc.0
    IL_0006:  ret
  } // end of method TestClass::Main

  .method public hidebysig specialname rtspecialname 
          instance void  .ctor() cil managed
  {
    // Code size       7 (0x7)
    .maxstack  8
    IL_0000:  ldarg.0
    IL_0001:  call       instance void [mscorlib]System.Object::.ctor()
    IL_0006:  ret
  } // end of method TestClass::.ctor

} // end of class TestClass

.class private auto ansi beforefieldinit A
       extends [mscorlib]System.Object
{
  .class auto ansi nested public beforefieldinit B
         extends [mscorlib]System.Object
  {
    .method public hidebysig specialname rtspecialname 
            instance void  .ctor() cil managed
    {
      // Code size       7 (0x7)
      .maxstack  8
      IL_0000:  ldarg.0
      IL_0001:  call       instance void [mscorlib]System.Object::.ctor()
      IL_0006:  ret
    } // end of method B::.ctor

  } // end of class B

  .method public hidebysig specialname rtspecialname 
          instance void  .ctor() cil managed
  {
    // Code size       7 (0x7)
    .maxstack  8
    IL_0000:  ldarg.0
    IL_0001:  call       instance void [mscorlib]System.Object::.ctor()
    IL_0006:  ret
  } // end of method A::.ctor

} // end of class A

//*********** DISASSEMBLY COMPLETE ***********************
// WARNING: Created Win32 resource file scope21.res
