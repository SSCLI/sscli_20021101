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
namespace System.Security {
    
    using System;
    using System.Threading;

    internal interface ICodeAccessSecurityEngine
    {
    	void Check(CodeAccessPermission cap, ref StackCrawlMark stackMark);
    	
    	void CheckImmediate(CodeAccessPermission cap, ref StackCrawlMark stackMark);
        
        void Check(PermissionSet permSet, ref StackCrawlMark stackMark);
        
        void CheckImmediate(PermissionSet permSet, ref StackCrawlMark stackMark);
        
    	void Assert(CodeAccessPermission cap, ref StackCrawlMark stackMark);
    	
    	void Deny(CodeAccessPermission cap, ref StackCrawlMark stackMark);
    	
    	void PermitOnly(CodeAccessPermission cap, ref StackCrawlMark stackMark);
    	
    	//boolean IsGranted(CodeAccessPermission cap);
        //  Not needed, although it might be useful to have:
        //    boolean IsGranted(CodeAccessPermission cap, Object obj);
    	
    	PermissionListSet GetCompressedStack(ref StackCrawlMark stackMark);
        
        PermissionSet GetPermissions(Object cl, out PermissionSet denied);
        
    }
}
