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
    internal interface ISecurityRuntime
    {
    	void Assert(PermissionSet permSet, ref StackCrawlMark stackMark);
    	
    	void Deny(PermissionSet permSet, ref StackCrawlMark stackMark);
    	
    	void PermitOnly(PermissionSet permSet, ref StackCrawlMark stackMark);
        
        void RevertAssert(ref StackCrawlMark stackMark);
    
        void RevertDeny(ref StackCrawlMark stackMark);
    
        void RevertPermitOnly(ref StackCrawlMark stackMark);
    
        void RevertAll(ref StackCrawlMark stackMark);
    
        PermissionSet GetDeclaredPermissions(Object cl, int type);
    
    }
}
