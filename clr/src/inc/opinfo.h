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
/***************************************************************************/
/*                                OpInfo.h                                 */
/***************************************************************************/

/* contains OpInfo, a wrapper that allows you to get useful information
   about IL opcodes, and how to decode them */

/***************************************************************************/

#ifndef OpInfo_h
#define OpInfo_h

#include "openum.h"

	// Decribes the flow of control properties of the instruction
enum OpFlow {
	META,			// not a real instuction
	CALL,			// a call instruction
	BRANCH,			// unconditional branch, does not fall through
	COND_BRANCH,	// may fall through
	PHI,			
	THROW,
	BREAK,
	RETURN,		
	NEXT,			// flows into next instruction (none of the above)
};

	// These are all the possible arguments for the instruction
/****************************************************************************/
union OpArgsVal {
	__int32  i;
	__int64 i8;
	double   r;
	struct {
		unsigned count;
		int* targets;   // targets are pcrelative displacements (little-endian)
		} switch_;
	struct {
		unsigned count;
		unsigned short* vars;
		} phi;
};

/***************************************************************************/
#define BYTE unsigned char

	// OpInfo parses a il instrution into an opcode, and a arg and updates the IP
class OpInfo {
public:
	OpInfo()			  { data = 0; }
	OpInfo(OPCODE opCode) { _ASSERTE(opCode < CEE_COUNT); data = &table[opCode]; } 

		// fetch instruction at 'instrPtr, fills in 'args' returns pointer 
		// to next instruction 
	const BYTE* fetch(const BYTE* instrPtr, OpArgsVal* args);	

	const char* getName() 	 	{ return(data->name); }
	OPCODE_FORMAT getArgsInfo()	{ return(OPCODE_FORMAT(data->format & PrimaryMask)); }
	OpFlow 		getFlow()	 	{ return(data->flow); }
	OPCODE 		getOpcode()	 	{ return((OPCODE) (data-table)); }
    int         getNumPop()     { return(data->numPop); }
    int         getNumPush()    { return(data->numPush); }

private:
	struct OpInfoData {
        const char* name;
        OPCODE_FORMAT format  	: 8;
		OpFlow     	flow		: 8;
		int     	numPop		: 3;	// < 0 means depends on instr args
		int       	numPush		: 3;	// < 0 means depends on instr args
        OPCODE      opcode      : 10;  	// This is the same as the index into the table
    };

	static OpInfoData table[];
private:
	OpInfoData* data;
};

#endif
