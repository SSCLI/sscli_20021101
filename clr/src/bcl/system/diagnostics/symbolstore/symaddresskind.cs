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
/*============================================================
**
** Class:  SymAddressKind
**
**                                               
**
** Represents address Kinds used with local variables, parameters, and
** fields.
**
** Date:  Tue Aug 24 02:13:09 1999
** 
===========================================================*/
namespace System.Diagnostics.SymbolStore {
	// Only statics, does not need to be marked with the serializable attribute    
    using System;

    /// <include file='doc\SymAddressKind.uex' path='docs/doc[@for="SymAddressKind"]/*' />
	[Serializable()]
    public enum SymAddressKind
    {
        // ILOffset: addr1 = IL local var or param index.
        /// <include file='doc\SymAddressKind.uex' path='docs/doc[@for="SymAddressKind.ILOffset"]/*' />
        ILOffset = 1,
    
        // NativeRVA: addr1 = RVA into module.
        /// <include file='doc\SymAddressKind.uex' path='docs/doc[@for="SymAddressKind.NativeRVA"]/*' />
        NativeRVA = 2,
    
        // NativeRegister: addr1 = register the var is stored in.
        /// <include file='doc\SymAddressKind.uex' path='docs/doc[@for="SymAddressKind.NativeRegister"]/*' />
        NativeRegister = 3,
    
        // NativeRegisterRelative: addr1 = register, addr2 = offset.
        /// <include file='doc\SymAddressKind.uex' path='docs/doc[@for="SymAddressKind.NativeRegisterRelative"]/*' />
        NativeRegisterRelative = 4,
    
        // NativeOffset: addr1 = offset from start of parent.
        /// <include file='doc\SymAddressKind.uex' path='docs/doc[@for="SymAddressKind.NativeOffset"]/*' />
        NativeOffset = 5,
    
        // NativeRegisterRegister: addr1 = reg low, addr2 = reg high.
        /// <include file='doc\SymAddressKind.uex' path='docs/doc[@for="SymAddressKind.NativeRegisterRegister"]/*' />
        NativeRegisterRegister = 6,
    
        // NativeRegisterStack: addr1 = reg low, addr2 = reg stk, addr3 = offset.
        /// <include file='doc\SymAddressKind.uex' path='docs/doc[@for="SymAddressKind.NativeRegisterStack"]/*' />
        NativeRegisterStack = 7,
    
        // NativeStackRegister: addr1 = reg stk, addr2 = offset, addr3 = reg high.
        /// <include file='doc\SymAddressKind.uex' path='docs/doc[@for="SymAddressKind.NativeStackRegister"]/*' />
        NativeStackRegister = 8,
    
        // BitField: addr1 = field start, addr = field length.
        /// <include file='doc\SymAddressKind.uex' path='docs/doc[@for="SymAddressKind.BitField"]/*' />
        BitField = 9,
    }
}
