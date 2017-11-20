.assembly extern mscorlib {}

.module 'anotherprivatescopediffassem.dll'

.assembly 'anotherprivatescopediffassem' // as "anotherprivatescopediffassem.dll"
 { 
.custom instance void [mscorlib]System.Security.AllowPartiallyTrustedCallersAttribute::.ctor() = ( 01 00 00 00 )
 
  .hash algorithm 0x00008004
  .ver 0:0:0:0
}
 

  .field static public int32 iPublic at MyLabel1
  .data MyLabel1 = int32(12)

  .field static assembly int32 iAssmbly at MyLabel2
  .data MyLabel2 = int32(12)

  .field static int32 iPrivateScope at MyLabel3
  .data MyLabel3 = int32(12)

  .method  static public void PublicFoo() il managed
  {
    // Code size       11 (0xb)
    .maxstack  8
    IL_0000:  ldstr      "This is PublicFoo"
    IL_0005:  call       void [mscorlib]System.Console::WriteLine(class ['mscorlib']System.String)
    IL_000a:  ret
  } // end of method 'PublicFoo'



//  .method static assembly void AssemblyFoo() il managed
  //{
    // Code size       11 (0xb)
    //.maxstack  8
    //IL_0000:  ldstr      "This is AssemblyFoo"
    //IL_0005:  call       void [mscorlib]System.Console::WriteLine(class ['mscorlib']System.String)
    //IL_000a:  ret
  //} // end of method 'AssemblyFoo'

  .method  static void PrivateScopeFoo() il managed
  {
    // Code size       11 (0xb)
    .maxstack  8
    IL_0000:  ldstr      "This is PrivateScopeFoo."
    IL_0005:  call       void [mscorlib]System.Console::WriteLine(class ['mscorlib']System.String)
    IL_000a:  ret
  } // end of method 'PrivateScopeFoo'



//*********** DISASSEMBLY COMPLETE ***********************
