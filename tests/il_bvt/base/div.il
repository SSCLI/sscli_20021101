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
.assembly div.exe{}
.class public _div {
.method public static int32 i4(int32,int32) {
.maxstack	2
	ldarg	0
	ldarg	1
	div
	ret
}
.method public static int32 u4(int32,int32) {
.maxstack	2
	ldarg	0
	ldarg	1
	div.un
	ret
}
.method public static int64 i8(int64,int64) {
.maxstack	2
	ldarg	0
	ldarg	1
	div
	ret
}
.method public static int64 u8(int64,int64) {
.maxstack	2
	ldarg	0
	ldarg	1
	div.un
	ret
}
.method public static float32 r4(float32,float32) {
.maxstack	2
	ldarg	0
	ldarg	1
	div
	ret
}
.method public static float64 r8(float64,float64) {
.maxstack	2
	ldarg	0
	ldarg	1
	div
	ret
}
.method public static int32 main(class [mscorlib]System.String[]) {
.entrypoint
.maxstack	10
        ldc.i4          0xF0F0F0F0
        ldc.i4          0x01010101
        call    int32 _div::i4(int32,int32)
        ldc.i4          0xFFFFFFF1
        ceq
        brfalse         FAIL
        ldc.i4          0xFFFFFFFF
        ldc.i4          0x00000002
        call    int32 _div::u4(int32,int32)
        ldc.i4          0x7FFFFFFF
        ceq
        brfalse         FAIL
        ldc.i8          0xF0F0F0F0F0F0F0F0
        ldc.i8          0x0101010101010101
        call    int64 _div::i8(int64,int64)
        ldc.i8          0xFFFFFFFFFFFFFFF1
        ceq
        brfalse         FAIL
        ldc.i8          0xFFFFFFFFFFFFFFFF
        ldc.i8          0x0000000000000002
        call    int64 _div::u8(int64,int64)
        ldc.i8          0x7FFFFFFFFFFFFFFF
        ceq
        brfalse         FAIL
        ldc.r4          float32(0x3F800000)
        ldc.r4          float32(0xBF800000)
        call    float32 _div::r4(float32,float32)
        ldc.r4          float32(0xBF800000)
        ceq
        brfalse         FAIL
        ldc.r8          float64(0x3FF0000000000000)
        ldc.r8          float64(0xBFF0000000000000)
        call    float64 _div::r8(float64,float64)
        ldc.r8          float64(0xBFF0000000000000)
        ceq
        brfalse         FAIL
PASS:
	ldc.i4	0x0000
	ret
FAIL:
	ldc.i4	0x0001
	ret
}
}
