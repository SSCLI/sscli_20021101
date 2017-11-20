.assembly extern mscorlib {}

.module 'anotherstruct.dll'

.assembly 'anotherstruct' // as "anotherstruct.dll"
 { 
.custom instance void [mscorlib]System.Security.AllowPartiallyTrustedCallersAttribute::.ctor() = ( 01 00 00 00 )
 
  .hash algorithm 0x00008004
  .ver 0:0:0:0
}
 

.namespace anotherStruct
{
  .class value public auto ansi sealed CompilerSource extends [mscorlib]System.ValueType
  {
    .field public int32 i
    .field public int32 j
  } // end of class 'CompilerSource'

} // end of namespace 'anotherStruct'

//*********** DISASSEMBLY COMPLETE ***********************
