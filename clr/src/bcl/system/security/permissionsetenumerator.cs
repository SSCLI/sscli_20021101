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
    //PermissionSetEnumerator.cs
    
	using System;
	using TokenBasedSetEnumerator = System.Security.Util.TokenBasedSetEnumerator;
    internal class PermissionSetEnumerator : TokenBasedSetEnumerator
    {
        private bool m_first;
        internal PermissionSet m_permSet;
        
        internal PermissionSetEnumerator(PermissionSet permSet)
            : base(permSet.m_unrestrictedPermSet)
        {
            m_first = true;
            m_permSet = permSet;
        }
        
        public override bool MoveNext()
        {
            if (!base.MoveNext())
            {
                if (m_first)
                {
                    this.SetData(m_permSet.m_normalPermSet);
                    m_first = false;
                    return base.MoveNext();
                }
                else
                {
                    return false;
                }
            }
            return true;
        }
        
        public virtual IPermission GetNextPermission()
        {
            if (MoveNext())
                return (IPermission) Current;
            else
                return null;
        }
        
    }
}
