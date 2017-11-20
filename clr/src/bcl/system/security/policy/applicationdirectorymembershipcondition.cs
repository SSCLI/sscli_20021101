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
//  ApplicationDirectoryMembershipCondition.cs
//
//  Implementation of membership condition for "application directories"
//

namespace System.Security.Policy {   
	using System;
	using SecurityElement = System.Security.SecurityElement;
	using System.Security.Policy;
	using URLString = System.Security.Util.URLString;
    using System.Collections;

    /// <include file='doc\ApplicationDirectoryMembershipCondition.uex' path='docs/doc[@for="ApplicationDirectoryMembershipCondition"]/*' />
    [Serializable]
    sealed public class ApplicationDirectoryMembershipCondition : IMembershipCondition, IConstantMembershipCondition
    {
        //------------------------------------------------------
        //
        // PRIVATE STATE DATA
        //
        //------------------------------------------------------
        
        //------------------------------------------------------
        //
        // PUBLIC CONSTRUCTORS
        //
        //------------------------------------------------------
    
        /// <include file='doc\ApplicationDirectoryMembershipCondition.uex' path='docs/doc[@for="ApplicationDirectoryMembershipCondition.ApplicationDirectoryMembershipCondition"]/*' />
        public ApplicationDirectoryMembershipCondition()
        {
        }
        
        //------------------------------------------------------
        //
        // IMEMBERSHIPCONDITION IMPLEMENTATION
        //
        //------------------------------------------------------
    
        /// <include file='doc\ApplicationDirectoryMembershipCondition.uex' path='docs/doc[@for="ApplicationDirectoryMembershipCondition.Check"]/*' />
        public bool Check( Evidence evidence )
        {
            if (evidence == null)
                return false;
        
            IEnumerator enumerator = evidence.GetHostEnumerator();
            while (enumerator.MoveNext())
            {
                Object obj = enumerator.Current;
            
                if (obj is ApplicationDirectory)
                {
                    ApplicationDirectory dir = (ApplicationDirectory)obj;
                
                    IEnumerator innerEnumerator = evidence.GetHostEnumerator();
                    
                    while (innerEnumerator.MoveNext())
                    {
                        Object innerObj = innerEnumerator.Current;
                        
                        if (innerObj is Url)
                        {
                            // We need to add a wildcard at the end because IsSubsetOf
                            // keys off of it.
                            
                            String appDir = dir.Directory;
                            
                            if (appDir != null && appDir.Length > 1)
                            {
                                if (appDir[appDir.Length-1] == '/')
                                    appDir += "*";
                                else
                                    appDir += "/*";
                                
                                URLString appDirString = new URLString( appDir );
                            
                                if (((Url)innerObj).GetURLString().IsSubsetOf( appDirString ))
                                {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
            
            return false;
        }
        
        /// <include file='doc\ApplicationDirectoryMembershipCondition.uex' path='docs/doc[@for="ApplicationDirectoryMembershipCondition.Copy"]/*' />
        public IMembershipCondition Copy()
        {
            return new ApplicationDirectoryMembershipCondition();
        }
        
        /// <include file='doc\ApplicationDirectoryMembershipCondition.uex' path='docs/doc[@for="ApplicationDirectoryMembershipCondition.ToXml"]/*' />
        public SecurityElement ToXml()
        {
            return ToXml( null );
        }
    
        /// <include file='doc\ApplicationDirectoryMembershipCondition.uex' path='docs/doc[@for="ApplicationDirectoryMembershipCondition.FromXml"]/*' />
        public void FromXml( SecurityElement e )
        {
            FromXml( e, null );
        }
        
        /// <include file='doc\ApplicationDirectoryMembershipCondition.uex' path='docs/doc[@for="ApplicationDirectoryMembershipCondition.ToXml1"]/*' />
        public SecurityElement ToXml( PolicyLevel level )
        {
            SecurityElement root = new SecurityElement( "IMembershipCondition" );
            System.Security.Util.XMLUtil.AddClassAttribute( root, this.GetType() );
            root.AddAttribute( "version", "1" );
            
            return root;
        }
    
        /// <include file='doc\ApplicationDirectoryMembershipCondition.uex' path='docs/doc[@for="ApplicationDirectoryMembershipCondition.FromXml1"]/*' />
        public void FromXml( SecurityElement e, PolicyLevel level )
        {
            if (e == null)
                throw new ArgumentNullException("e");
        
            if (!e.Tag.Equals( "IMembershipCondition" ))
            {
                throw new ArgumentException( Environment.GetResourceString( "Argument_MembershipConditionElement" ) );
            }
        }
        
        /// <include file='doc\ApplicationDirectoryMembershipCondition.uex' path='docs/doc[@for="ApplicationDirectoryMembershipCondition.Equals"]/*' />
        public override bool Equals( Object o )
        {
            return (o is ApplicationDirectoryMembershipCondition);
        }
        
        /// <include file='doc\ApplicationDirectoryMembershipCondition.uex' path='docs/doc[@for="ApplicationDirectoryMembershipCondition.GetHashCode"]/*' />
        public override int GetHashCode()
        {
            return typeof( ApplicationDirectoryMembershipCondition ).GetHashCode();
        }
        
        /// <include file='doc\ApplicationDirectoryMembershipCondition.uex' path='docs/doc[@for="ApplicationDirectoryMembershipCondition.ToString"]/*' />
        public override String ToString()
        {
            return Environment.GetResourceString( "ApplicationDirectory_ToString" );
        }   
    }
}
