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
	using System.Text;
	using System.Runtime.CompilerServices;
    //FrameSecurityDescriptor.cs
    //
    // Internal use only.
    // DO NOT DOCUMENT
    //
	using System;
	[Serializable()]
    sealed internal class FrameSecurityDescriptor : System.Object
    {
    
    	/*	EE looks up the following three fields using the field names.
    	*	If the names are ever changed, make corresponding changes to constants 
    	*	defined in COMSecurityRuntime.cpp	
    	*/
        private PermissionSet       m_assertions;
        private PermissionSet       m_denials;
        private PermissionSet       m_restriction;
        private bool                m_assertAllPossible;
        
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private static extern void IncrementOverridesCount();
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		private static extern void DecrementOverridesCount();

        // Default constructor.
        internal FrameSecurityDescriptor()
        {
            //m_flags = 0;
        }
    
        internal FrameSecurityDescriptor Copy()
        {
            FrameSecurityDescriptor desc = new FrameSecurityDescriptor();
            if (this.m_assertions != null)
                desc.m_assertions = this.m_assertions.Copy();

            if (this.m_denials != null)
                desc.m_denials = this.m_denials.Copy();

            if (this.m_restriction != null)
                desc.m_restriction = this.m_restriction.Copy();

            desc.m_assertAllPossible = this.m_assertAllPossible;

            return desc;
        }
        
        //-----------------------------------------------------------+
        // H E L P E R
        //-----------------------------------------------------------+
        
        private PermissionSet CreateSingletonSet(IPermission perm)
        {
            PermissionSet permSet = new PermissionSet(false);
            permSet.AddPermission(perm.Copy());
            return permSet;
        }
    
        //-----------------------------------------------------------+
        // A S S E R T
        //-----------------------------------------------------------+
    
        internal void SetAssert(IPermission perm)
        {
            m_assertions = CreateSingletonSet(perm);
        }
        
        internal void SetAssert(PermissionSet permSet)
        {
            m_assertions = permSet.Copy();
        }
        
        internal PermissionSet GetAssertions()
        {
            return m_assertions;
        }

        internal void SetAssertAllPossible()
        {
            m_assertAllPossible = true;
        }

        internal bool GetAssertAllPossible()
        {
            return m_assertAllPossible;
        }
        
        //-----------------------------------------------------------+
        // D E N Y
        //-----------------------------------------------------------+
    
        internal void SetDeny(IPermission perm)
        {
			IncrementOverridesCount();
            m_denials = CreateSingletonSet(perm);
        }
        
        internal void SetDeny(PermissionSet permSet)
        {
			IncrementOverridesCount();
            m_denials = permSet.Copy();
        }
    
        internal PermissionSet GetDenials()
        {
            return m_denials;
        }
        
        //-----------------------------------------------------------+
        // R E S T R I C T
        //-----------------------------------------------------------+
    
        internal void SetPermitOnly(IPermission perm)
        {
			IncrementOverridesCount();
            m_restriction = CreateSingletonSet(perm);
        }
        
        internal void SetPermitOnly(PermissionSet permSet)
        {
			IncrementOverridesCount();
            // permSet must not be null
            m_restriction = permSet.Copy();
        }
        
        internal PermissionSet GetPermitOnly()
        {
            return m_restriction;
        }
        
        //-----------------------------------------------------------+
        // R E V E R T
        //-----------------------------------------------------------+
    
        internal void RevertAssert()
        {
            m_assertions = null;
        }
        
        internal void RevertAssertAllPossible()
        {
            m_assertAllPossible = false;
        }

        internal void RevertDeny()
        {
			if (m_denials != null)
				DecrementOverridesCount();
            m_denials = null;
        }
        
        internal void RevertPermitOnly()
        {
			if (m_restriction != null)
				DecrementOverridesCount();
            m_restriction = null;
        }

        internal void RevertAll()
        {
            RevertAssert();
            RevertAssertAllPossible();
			RevertDeny();
			RevertPermitOnly();
        }
    
    }
}
