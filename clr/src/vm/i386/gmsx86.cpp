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
/**************************************************************/
/*                       gmsx86.cpp                           */
/**************************************************************/

#include "common.h"
#include "gmscpu.h"

/***************************************************************/
/* setMachState figures out what the state of the CPU will be
   when the function that calls 'setMachState' returns.  It stores
   this information in 'frame'

   setMachState works by simulating the execution of the
   instructions starting at the instruction following the
   call to 'setMachState' and continuing until a return instruction
   is simulated.  To avoid having to process arbitrary code, the
   call to 'setMachState' should be called as follows

      if (machState.setMachState != 0) return;

   setMachState is guarnenteed to return 0 (so the return
   statement will never be executed), but the expression above
   insures insures that there is a 'quick' path to epilog
   of the function.  This insures that setMachState will only
   have to parse a limited number of X86 instructions.   */


/***************************************************************/
#ifndef POISONC
#define POISONC ((sizeof(int *) == 4)?0xCCCCCCCCU:UI64(0xCCCCCCCCCCCCCCCC))
#endif

void MachState::Init(void** aPEdi, void** aPEsi, void** aPEbx, void** aPEbp, void* aEsp, void** aPRetAddr) {

#ifdef _DEBUG
        _edi = (void*)POISONC;
        _esi = (void*)POISONC;
        _ebx = (void*)POISONC;
        _ebp = (void*)POISONC;
#endif
        _esp = aEsp;
        _pRetAddr = aPRetAddr;
        _pEdi = aPEdi;
        _pEsi = aPEsi;
        _pEbx = aPEbx;
        _pEbp = aPEbp;
}


/***************************************************************/
void LazyMachState::getState(int funCallDepth, TestFtn testFtn) {

	if (isValid())          // already in valid state, can return
			return;

	// currently we only do this for depth 1 through 4
	_ASSERTE(1 <= funCallDepth && funCallDepth <= 4);

			// Work with a copy so that we only write the values once.
			// this avoids race conditions.
	MachState copy;
	copy._edi = _edi;
	copy._esi = _esi;
	copy._ebx = _ebx;
	copy._ebp = captureEbp;
	copy._pEdi = &_edi;
	copy._pEsi = &_esi;
	copy._pEbx = &_ebx;
	copy._pEbp = &_ebp;

		// We have caputred the state of the registers as they exist in 'captureState'
		// we need to simulate execution from the return address caputred in 'captureState
		// until we return from the caller of caputureState.

	unsigned __int8* ip = captureEip;
    void** ESP = captureEsp;
	ESP++;                                                                          // pop caputureState's return address


		// VC now has small helper calls that it uses in epilogs.  We need to walk into these
		// helpers if we are to decode the stack properly.  After we walk the helper we need
		// to return and continue walking the epiliog.  This varaible remembers were to return to
	unsigned __int8* epilogCallRet = 0;
    BOOL bFirstCondJmp = TRUE;

    int datasize; // helper variable for decoding of r/m

#ifdef _DEBUG
	int count = 0;
#endif
    for(;;)
    {
		_ASSERTE(count++ < 1000);		// we should never walk more than 1000 instructions!
        switch(*ip)
        {
            case 0x90:              // nop 
            case 0x64:              // FS: prefix
            incIp1:
                ip++;
                break;
            case 0x5B:              // pop EBX
                copy._pEbx = ESP;
                copy._ebx  = *ESP++;
                goto incIp1;
            case 0x5D:              // pop EBP
                copy._pEbp = ESP;
                copy._ebp  = *ESP++;
                goto incIp1;
            case 0x5E:              // pop ESI
                copy._pEsi = ESP;
                copy._esi = *ESP++;
                goto incIp1;
            case 0x5F:              // pop EDI
                copy._pEdi = ESP;
                copy._edi = *ESP++;
                goto incIp1;

            case 0x58:              // pop EAX
            case 0x59:              // pop ECX
            case 0x5A:              // pop EDX
                ESP++;
                goto incIp1;

            case 0xEB:              // jmp <disp8>
                ip += (signed __int8) ip[1] + 2;
                break;

            case 0xE8:              // call <disp32>
                ip += 5;
                // Normally we simply skip calls since we should only run into descructor calls, and they leave 
                // the stack unchanged.  VC emits special epilog helpers which we do need to walk into
                // we determine that they are one of these because they are followed by a 'ret' (this is
                // just a heuristic, but it works for now)

                if (epilogCallRet == 0 && (*ip == 0xC2 || *ip == 0xC3)) {   // is next instr a ret or retn?
                    // Yes.  we found a call we need to walk into.
                    epilogCallRet = ip;             // remember our return address
                    --ESP;                          // simulate pushing the return address
                    ip += *((__int32*) &ip[-4]);    // goto the call

                    // handle jmp [addr] in case SEH_Epilog is imported
                    if (*ip == 0xFF && *(ip+1) == 0x25)
                    {
                        ip = **((unsigned __int8***) &ip[2]);
                    }
                }
                break;

            case 0xE9:              // jmp <disp32>
                ip += *((__int32*) &ip[1]) + 5;
                break;

            case 0x0f:              // follow non-zero jumps:
                if (ip[1] == 0x85)  // jne <disp32>
                    ip += *((__int32*) &ip[2]) + 6;
                else
                if (ip[1] == 0x84)  // je <disp32>
                    ip += 6;
                else
                    goto badOpcode;
                break;

                // This is here because VC seems no not always optimize
                // away a test for a literal constant
            case 0x6A:              // push 0xXX
                ip += 2;
                --ESP;
                break;

            // Added to handle VC7 generated code
            case 0x50:              // push EAX
            case 0x51:              // push ECX
            case 0x52:              // push EDX
            case 0x53:              // push EBX
            case 0x55:              // push EBP
            case 0x56:              // push ESI
            case 0x57:              // push EDI
                --ESP;
            case 0x40:              // inc EAX
                goto incIp1;

            case 0x68:              // push 0xXXXXXXXX
                if ((ip[5] == 0xFF) && (ip[6] == 0x15)) {
                    ip += 11;
                }
                else
                    ip += 5;
                break;

            case 0x75:              // jnz <target>
                // Except the first jump, we always follow forward jump to avoid possible looping.
                if (bFirstCondJmp) {
                    bFirstCondJmp = FALSE;
                    ip += (signed __int8) ip[1] + 2;   // follow the non-zero path
                }
                else {
                    unsigned __int8* tmpIp = ip + (signed __int8) ip[1] + 2;
                    if (tmpIp > ip) {
                        ip = tmpIp;
                    }
                    else {
                        ip += 2;
                    }
                }
                break;

            case 0x74:              // jz <target>
                if (bFirstCondJmp) {
                    bFirstCondJmp = FALSE;
                    ip += 2;            // follow the non-zero path
                }
                else {
                    unsigned __int8* tmpIp = ip + (signed __int8) ip[1] + 2;
                    if (tmpIp > ip) {
                        ip = tmpIp;
                    }
                    else {
                        ip += 2;
                    }
                }
                break;

            case 0x85:
                if ((ip[1] & 0xC0) != 0xC0)  // TEST reg1, reg2
                    goto badOpcode;
                ip += 2;
                break;

            case 0x31:
            case 0x32:
            case 0x33:
                if ((ip[1] & 0xC0) == 0xC0) // mod bits are 11
                {
                    // XOR reg1, reg2

                    // VC generates a silly sequence in some code
                    // xor reg, reg
                    // test reg reg
                    // je 	<target>
                    // This is just an unconditional branch, so to it
                    if ((ip[1] & 7) == ((ip[1] >> 3) & 7)) {
                        if (ip[2] == 0x85 && ip[3] == ip[1]) {		// TEST reg, reg
                            if (ip[4] == 0x74) {
                                ip += (signed __int8) ip[5] + 6;   // follow the non-zero path
                                break;
                            }
                            _ASSERTE(ip[4] != 0x0f); // If this goes off, we need the big jumps
                        }
                    }
                    ip += 2;
                }
                else if ((ip[1] & 0xC0) == 0x40) // mod bits are 01
                {
                    // XOR reg1, [reg+offs8]
                    // Used by the /GS flag for call to __security_check_cookie()
                    // Should only be XOR ECX,[EBP+4]
                    _ASSERTE((((ip[1] >> 3) & 0x7) == 0x1) && ((ip[1] & 0x7) == 0x5) && (ip[2] == 4));
                    ip += 3;
                }
                else if ((ip[1] & 0xC0) == 0x80) // mod bits are 10
                {
                    // XOR reg1, [reg+offs32]
                    // Should not happen but may occur with __security_check_cookie()
                    _ASSERTE(!"Unexpected XOR reg1, [reg+offs32]");
                    ip += 6;
                }
                else
                {
                    goto badOpcode;
                }
                break;

            case 0x39:
                if ((ip[1] & 0xC7) == 0x85) // CMP [ebp+imm32], reg
                    ip += 6;
                else
                    goto badOpcode;
                break;

            case 0xFF:
                if (ip[1] == 0xb5)
                {
                    --ESP;      // push dword ptr [ebp+imm32]
                    ip += 6;
                }
                else
                    goto badOpcode;
                break;

            case 0x3B:
                if ((ip[1] & 0xC0) != 0xC0)  // CMP reg1, reg2
                    goto badOpcode;
                ip += 2;
                break;

            case 0x89:                          // MOV (DWORD)
                if (ip[1] == 0xEC)              // MOV ESP, EBP
                    goto mov_esp_ebp;

                // FALL THROUGH  
            case 0x88:                          // MOV (BYTE)
                datasize = 0;

            DecodeRM:
                if ((ip[1] & 0xC0) == 0x0) {    // MOV [REG], REG
                    if ((ip[1] & 7) == 5)       // MOV [mem], REG
                        ip += datasize + 6;
                    else
                    if ((ip[1] & 7) == 4)       // SIB byte
                        ip += datasize + 3;
                    else
                        ip += datasize + 2;
                    break;
                }
                if ((ip[1] & 0xC0) == 0x40) {   // MOV [REG+XX], REG
                    if ((ip[1] & 7) == 4)       // SIB byte
                        ip += datasize + 4;
                    else
                        ip += datasize + 3;
                    break;
                }
                if ((ip[1] & 0xC0) == 0x80) {   // MOV [REG+XXXX], REG
                    if ((ip[1] & 7) == 4)       // SIB byte
                        ip += datasize + 7;
                    else
                        ip += datasize + 6;
                    break;
                }
                if ((ip[1] & 0xC0) == 0xC0) {    // MOV EAX, REG
                    ip += datasize + 2;
                    break;
                }
                goto badOpcode;

            case 0x80:                           // OP r/m8, <imm8>
                datasize = 1;
                goto DecodeRM;

            case 0x81:                           // OP r/m32, <imm32>
                if (ip[1] == 0xC4) {             // ADD ESP, <imm32>
                    ESP = (void**) ((__int8*) ESP + *((__int32*) &ip[2]));
                    ip += 6;
                    break;
                }

                datasize = 4;
                goto DecodeRM;

            case 0x83:                           // OP r/m32, <imm8>
                if (ip[1] == 0xC4)  {            // ADD ESP, <imm8>
                    ESP = (void**) ((__int8*) ESP + (signed __int8) ip[2]);
                    ip += 3;
                    break;
                }
                if (ip[1] == 0xc5) {            // ADD EBP, <imm8>
                    copy._ebp  = (void*)((size_t)copy._ebp + (signed __int8) ip[2]);
                    ip += 3;
                    break;
                }

                datasize = 1;
                goto DecodeRM;

            case 0x8B:                                  // MOV (DWORD)
                if (ip[1] == 0xE5) {                    // MOV ESP, EBP
                mov_esp_ebp:
                    ESP = (void**) copy._ebp;
                    ip += 2;
                    break;
                }

                if ((ip[1] & 0xC7) == 0x45) {   // MOV reg, [EBP + imm8]
                    // gcc sometimes restores callee-preserved registers
                    // via 'mov reg, [ebp-xx]' instead of 'pop reg'
                    if ( ip[1] == 0x5D ) {  // MOV EBX, [EBP+XX]
                      copy._pEbx = (void**)((__int8*)copy._ebp + (signed __int8)ip[2]);
                      copy._ebx =  *copy._pEbx ;
                    }
                    else if ( ip[1] == 0x75 ) {  // MOV ESI, [EBP+XX]
                      copy._pEsi = (void**)((__int8*)copy._ebp + (signed __int8)ip[2]);
                      copy._esi =  *copy._pEsi;
                    }
                    else if ( ip[1] == 0x7D ) {  // MOV EDI, [EBP+XX]
                      copy._pEdi = (void**)((__int8*)copy._ebp + (signed __int8)ip[2]);
                      copy._edi =   *copy._pEdi;
                    }
                    else if ( ip[1] == 0x65 /*ESP*/ || ip[1] == 0x6D /*EBP*/)
                      goto badOpcode;

                    /*EAX,ECX,EDX*/
                    ip += 3;
                    break;
                }

                if ((ip[1] & 0xC7) == 0x85) {   // MOV E*X, [EBP+imm32]
                    // gcc sometimes restores callee-preserved registers
                    // via 'mov reg, [ebp-xx]' instead of 'pop reg'
                    if ( ip[1] == 0xDD ) {  // MOV EBX, [EBP+XXXXXXXX]
                      copy._pEbx = (void**)((__int8*)copy._ebp + *(signed __int32*)&ip[2]);
                      copy._ebx =  *copy._pEbx ;
                    }
                    else if ( ip[1] == 0xF5 ) {  // MOV ESI, [EBP+XXXXXXXX]
                      copy._pEsi = (void**)((__int8*)copy._ebp + *(signed __int32*)&ip[2]);
                      copy._esi =  *copy._pEsi;
                    }
                    else if ( ip[1] == 0xFD ) {  // MOV EDI, [EBP+XXXXXXXX]
                      copy._pEdi = (void**)((__int8*)copy._ebp + *(signed __int32*)&ip[2]);
                      copy._edi =   *copy._pEdi;
                    }
                    else if ( ip[1] == 0xE5 /*ESP*/ || ip[1] == 0xED /*EBP*/)
                      goto badOpcode;  // Add more registers

                    /*EAX,ECX,EDX*/
                    ip += 6;
                    break;
                }

                // FALL THROUGH
            case 0x8A:                  // MOV (byte)
                datasize = 0;
                goto DecodeRM;

            case 0x8D:                          // LEA
                if ((ip[1] & 0x38) == 0x20) {                       // Don't allow ESP to be updated
                    if (ip[1] == 0xA5)          // LEA ESP, [EBP+XXXX]
                        ESP = (void**) ((__int8*) copy._ebp + *((signed __int32*) &ip[2])); 
                    else if (ip[1] == 0x65)     // LEA ESP, [EBP+XX]
                        ESP = (void**) ((__int8*) copy._ebp + (signed __int8) ip[2]); 
                    else
                        goto badOpcode;
                }

                datasize = 0;
                goto DecodeRM;

            case 0xB0:  // MOV AL, imm8
                ip += 2;
                break;
            case 0xB8:  // MOV EAX, imm32
            case 0xB9:  // MOV ECX, imm32
            case 0xBA:  // MOV EDX, imm32
            case 0xBB:  // MOV EBX, imm32
            case 0xBE:  // MOV ESI, imm32
            case 0xBF:  // MOV EDI, imm32
                ip += 5;
                break;

            case 0xC2:                  // ret N
                {
                unsigned __int16 disp = *((unsigned __int16*) &ip[1]);
                                ip = (unsigned __int8*) (*ESP);
                copy._pRetAddr = ESP++;
                _ASSERTE(disp < 64);    // sanity check (although strictly speaking not impossible)
                ESP = (void**)((size_t) ESP + disp);           // pop args
                goto ret;
                }
            case 0xC3:                  // ret
                ip = (unsigned __int8*) (*ESP);
                copy._pRetAddr = ESP++;

                if (epilogCallRet != 0) {       // we are returning from a special epilog helper
                    ip = epilogCallRet;
                    epilogCallRet = 0;
                    break;                      // this does not count toward funcCallDepth
                }
            ret:
                --funCallDepth;
                if (funCallDepth <= 0 || (testFtn != 0 && (*testFtn)(*copy.pRetAddr())))
                    goto done;
                bFirstCondJmp = TRUE;
                break;

            case 0xC6:                  // MOV r/m8, imm8
                datasize = 1;
                goto DecodeRM;

            case 0xC7:                  // MOV r/m32, imm32
                datasize = 4;
                goto DecodeRM;

            case 0xC9:                  // leave
                ESP = (void**) (copy._ebp);
                copy._pEbp = ESP;
                copy._ebp = *ESP++;
                ip++;
                                break;

            case 0xCC:
                *((int*) 0) = 1;        // If you get at this error, it is because yout
                                        // set a breakpoint in a helpermethod frame epilog
                                        // you can't do that unfortunately.  Just move it
                                        // into the interior of the method to fix it

                goto done;

            case 0xD9:  // single prefix
                if (0xEE == ip[1])
                {
                    ip += 2;            // FLDZ
                    break;
                }
                //
                // INTENTIONAL FALL THRU
                //
            case 0xDD:  // double prefix
                switch (ip[1])
                {
                case 0x5D:  ip += 3; break; // FSTP {d|q}word ptr [EBP+disp8]
                case 0x9D:  ip += 6; break; // FSTP {d|q}word ptr [EBP+disp32]
                case 0x45:  ip += 3; break; // FLD  {d|q}word ptr [EBP+disp8]
                case 0x85:  ip += 6; break; // FLD  {d|q}word ptr [EBP+disp32]
                case 0x05:  ip += 6; break; // FLD  {d|q}word ptr [XXXXXXXX]
                default:    goto badOpcode; 
                }
                break;

            default:
            badOpcode:
                _ASSERTE(!"Bad opcode");
                // FIX what to do here?
                *((unsigned __int8**) 0) = ip;  // cause an access violation (Free Build assert)
                goto done;
        }
    }
    done:
        _ASSERTE(epilogCallRet == 0);

    // At this point the fields in 'frame' coorespond exactly to the register
    // state when the the helper returns to its caller.
        copy._esp = ESP;


        // _pRetAddr has to be the last thing updated when we make the copy (because its
        // is the the _pRetAddr becoming non-zero that flips this from invalid to valid.
        // we assert that it is the last field in the struct.

#ifdef _DEBUG
                        // Make certain _pRetAddr is last and struct copy happens lowest to highest
        static int once = 0;
        if (!once) {
                _ASSERTE(offsetof(MachState, _pRetAddr) == sizeof(MachState) - sizeof(void*));
                void* buff[sizeof(MachState) + sizeof(void*)];
                MachState* from = (MachState*) &buff[0];        // Set up overlapping buffers
                MachState* to = (MachState*) &buff[1];
                memset(to, 0xCC, sizeof(MachState));
                from->_pEdi = 0;
                *to = *from;                                                            // If lowest to highest, 0 gets propageted everywhere
                _ASSERTE(to->_pRetAddr == 0);
                once = 1;
        }
#endif

        *((MachState *) this) = copy;
}
