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
.assembly bge.exe{}
.method public	static int32 main(class [mscorlib]System.String[]) {
.locals		(class [mscorlib]System.String)
.entrypoint
	ldc.i4		0x55555555
	ldc.i4		0x44444444
	bge			next0
	br			fail
next0:
	ldc.i4		0x66666666
	ldc.i4		0x66666666
	bge			next1
	br			fail
next1:
        ldc.i4          0x44444444
        ldc.i4          0x55555555
        bge                     fail
        br                      next2
next2:
	ldc.r4		float32(0x3F800000)
	ldc.r4		float32(0xBF800000)
	bge			next3
	br			fail
next3:
	ldc.r4		float32(0x3F800000)
	ldc.r4		float32(0x3F800000)
	bge			next4
	br			fail
next4:
	ldc.r4		float32(0xBF800000)
	ldc.r4		float32(0x3F800000)
	bge			fail
	br			next5
next5:
	ldc.r4		float32(0x3F800000)
	ldc.r4		float32(0xBF800000)
	bge.un		next6
	br			fail
next6:
	ldc.r4		float32(0x3F800000)
	ldc.r4		float32(0x3F800000)
	bge.un		next7
	br			fail
next7:
	ldc.r4		float32(0xBF800000)
	ldc.r4		float32(0x3F800000)
	bge.un		fail
	br			next8
next8:
	ldc.r4		float32(0x7FC00000)
	ldc.r4		float32(0x7FC00000)
	bge.un		next9
	br			fail
next9:
	ldc.r4		float32(0x7FC00000)
	ldc.r4		float32(0x3F800000)
	bge.un		next10
	br			fail
next10:
        ldc.r4          float32(0xBF800000)
        ldc.r4          float32(0x7FC00000)
        bge.un          pass
        br                      fail
pass:
	ldc.i4		0x0000
	br			end
fail:
	ldc.i4		0x0001
	br			end
end:
	ret
}
