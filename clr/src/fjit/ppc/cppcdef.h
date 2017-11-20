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
//
// Desription:
//
// Defines the layout of the PowerPC machine instructions using Cmd structures.
// Note that the code here ONLY deals with PowerPC specific information
//


#ifndef _CPPCDEF_H
#define _CPPCDEF_H 

#ifdef _PPCDEF_H
#error "Can not include both ppcdef.h and cppcdef.h"
#endif

#undef Fjit_pass
#ifdef FjitCompile
#define Fjit_pass compile
#endif
#ifdef FjitMap
#define Fjit_pass map
#endif

#ifndef Fjit_pass
#error "Either FjitCompile or FjitMap must be defined"
#endif

#define expNum(val)			(val)
#define expBits(exp, width, pos) 	(/*_ASSERTE((exp) < (1 << width)), */ (exp) << pos)
#define expMap(exp, map)  		(/*_ASSERTE((exp) < 32), */((char*) map)[exp])
#define expOr2(exp1, exp2)  		((exp1) | (exp2))
#define expOr3(exp1, exp2, exp3) 	((exp1) | (exp2) | (exp3))
#define expOr4(exp1, exp2, exp3, exp4)	((exp1) | (exp2) | (exp3) | (exp4))

	// Convention outPtr is the output pointer

#ifdef FjitCompile // output the instructions
#define cmdNull() 	  			0
#define cmdByte(exp)			   	(*((unsigned char*&)(outPtr))++ = (unsigned char)(exp))
#define cmdWord(exp)  				(*((unsigned short*&) (outPtr))++ = (exp))
#define cmdDWord(exp)				(*((unsigned int *&)(outPtr))++ = (exp))
#endif //FjitCompile

#ifdef FjitMap	// size the instructions, do not ouput them
#define cmdNull() 	  			
#define cmdByte(exp)			   	(outPtr += 1)
#define cmdWord(exp)  				(outPtr += 2)
#define cmdDWord(exp)				(outPtr += 4)
#endif //FjitMap

#define cmdBlock2(cmd0, cmd1) 			(cmd0, cmd1)
#define cmdBlock3(cmd0, cmd1, cmd2) 		(cmd0, cmd1, cmd2)
#define cmdBlock4(cmd0, cmd1, cmd2, cmd3) 	(cmd0, cmd1, cmd2, cmd3)
#define cmdReloc(type, exp)			_ASSERTE(0)

#include "ppcdef.h"

#endif
