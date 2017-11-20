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
** File:    __TransparentProxy.cs
**
**                                                
**
** Purpose: Defines Transparent proxy
**
** Date:    Feb 16, 1998
**
===========================================================*/
namespace System.Runtime.Remoting.Proxies {
	using System.Runtime.Remoting;
    // Transparent proxy and Real proxy are vital pieces of the
    // remoting data structures. Transparent proxy magically
    // creates a message that represents a call on it and delegates
    // to the Real proxy to do the real remoting work.
	using System;
    internal sealed class __TransparentProxy {
        // Created inside EE
        private __TransparentProxy() {
           throw new NotSupportedException(Environment.GetResourceString(ResId.NotSupported_Constructor));
        }
        
        // Private members
        private RealProxy _rp;          // Reference to the real proxy
        private Object    _stubData;    // Data used by stubs to decide whether to short circuit calls or not
        private IntPtr _pMT;            // Method table of the class this proxy represents
        private IntPtr _pInterfaceMT;   // Cached interface method table		
        private IntPtr _stub;           // Unmanaged code that decides whether to short circuit calls or not
     
 		// This method should never be called.  Its sole purpose is to shut up the compiler
		//	because it warns about private fields that are never used.  Most of these fields
		//	are used in unmanaged code.
#if _DEBUG
		private int NeverCallThis()
		{
			BCLDebug.Assert(false,"NeverCallThis");
			_rp = null;
			_pMT = new IntPtr(RemotingServices.TrashMemory);
			_pInterfaceMT = new IntPtr(RemotingServices.TrashMemory);
            _stub = new IntPtr(RemotingServices.TrashMemory);
			RealProxy rp = _rp;
			return _pInterfaceMT.ToInt32();
		}
#endif
    }

}

