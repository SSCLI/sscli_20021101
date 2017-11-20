//  ==++==
//
//    
//     Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//    
//     The use and distribution terms for this software are contained in the file
//     named license.txt, which can be found in the root of this distribution.
//     By using this software in any fashion, you are agreeing to be bound by the
//     terms of this license.
//    
//     You must not remove this notice, or any other, from this software.
//    
//
//  ==--==
//
//   *** NOTE:	  If you make changes to this file, propagate the changes to
//                x86fjitasm.s in this directory

	.intel_syntax
	.arch i586


//  void __stdcall SWITCH_helper(void)
.global SWITCH_helper
.func SWITCH_helper
SWITCH_helper:	
        pop   %eax       // return address
        pop   %ecx       // limit
        pop   %edx       // index
        push  %eax
        cmp   %edx, %ecx
        jbe   LRangeOK
        mov   %edx, %ecx
LRangeOK:	
	lea   %edx, [%edx*4+%edx+2]  // +2 is the size of the "jmp eax"
                                  // instruction just before the switch table
	                          // this is being done only for the
	                          // convenience of the debugger, which
	                          // currently cannot handle functions that do
	                          // a jmp out.
        add   %eax, %edx  // since eax+edx*5 is not allowed
        ret
.endfunc

	.end

