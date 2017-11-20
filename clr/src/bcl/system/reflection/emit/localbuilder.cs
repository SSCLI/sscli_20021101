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
** Class:  LocalBuilder
**
**                                     
**
** LocalBuilder represent a local within a Method. Get to LocalBuilder from ILGenerator:DeclareLocal
**
** Date:  Feb, 2000
** 
===========================================================*/

namespace System.Reflection.Emit {
    
	using System;
	using System.Reflection;
    /// <include file='doc\LocalBuilder.uex' path='docs/doc[@for="LocalBuilder"]/*' />
    public sealed class LocalBuilder
    { 

    	/***************************************
    	 * Set the current local variable's name
    	**********************************************/
        /// <include file='doc\LocalBuilder.uex' path='docs/doc[@for="LocalBuilder.SetLocalSymInfo"]/*' />
        public void SetLocalSymInfo(	
            String 	    name)
        {
            SetLocalSymInfo(name, 0, 0);
        }
    
    	/***************************************
    	 * Set current local variable's name and also the variable scope
    	**********************************************/
        /// <include file='doc\LocalBuilder.uex' path='docs/doc[@for="LocalBuilder.SetLocalSymInfo1"]/*' />
        public void SetLocalSymInfo(
            String 	    name, 
            int 		startOffset,            // this pair of offset declare the scope of the local
            int 		endOffset)
        {
            ModuleBuilder   dynMod;
            SignatureHelper sigHelp;
            int             sigLength;
            byte[]          signature;
            byte[]          mungedSig;
            int             index;

            dynMod = (ModuleBuilder) m_methodBuilder.DeclaringType.Module;
            if (m_methodBuilder.IsTypeCreated())
            {
                // cannot change method after its containing type has been created
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_TypeHasBeenCreated"));
            }
    
            // set the name and range of offset for the local
            if (dynMod.GetSymWriter() == null)
            {
                // cannot set local name if not debug module
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_NotADebugModule"));
            }
    
            sigHelp = SignatureHelper.GetFieldSigHelper(dynMod);
            sigHelp.AddArgument(m_localType);
            signature = sigHelp.InternalGetSignature(out sigLength);
    
            // The symbol store doesn't want the calling convention on the
            // front of the signature, but InternalGetSignature returns
            // the callinging convention. So we strip it off. This is a
            // bit unfortunate, since it means that we need to allocate
            // yet another array of bytes... ugh...
            mungedSig = new byte[sigLength - 1];
            Array.Copy(signature, 1, mungedSig, 0, sigLength - 1);
            
            index = m_methodBuilder.GetILGenerator().m_ScopeTree.GetCurrentActiveScopeIndex();
            if (index == -1)
            {
                // top level scope information is kept with methodBuilder
                m_methodBuilder.m_localSymInfo.AddLocalSymInfo(
                     name,
                     mungedSig,
                     m_localIndex,   
                     startOffset,
                     endOffset);
            }
            else
            {
                m_methodBuilder.GetILGenerator().m_ScopeTree.AddLocalSymInfoToCurrentScope(
                     name,
                     mungedSig,
                     m_localIndex,   
                     startOffset,
                     endOffset);
            }
        }

        internal int GetLocalIndex() 
        {
            return m_localIndex;
        }

        internal MethodBuilder GetMethodBuilder() 
        {
            return m_methodBuilder;
        }
    	/// <include file='doc\LocalBuilder.uex' path='docs/doc[@for="LocalBuilder.LocalType"]/*' />
    	public Type LocalType {
    		get { return m_localType; }
    	}
        
    
        //*******************************
    	// Make a private constructor so these cannot be constructed externally.
        //*******************************
        private LocalBuilder() {}

        
        //*******************************
        // Internal constructor.
        //*******************************
        internal LocalBuilder(int localIndex, Type localType, MethodBuilder methodBuilder) 
        {
            m_localIndex = localIndex;
            m_localType = localType;
            m_methodBuilder = methodBuilder;
        }

        private int         m_localIndex;                   // index in the local signature
        private Type        m_localType;
        private MethodBuilder m_methodBuilder;
    }
}

