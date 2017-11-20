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
/*=============================================================================
**
** Class: SecurityException
**
**                                             
**
** Purpose: Exception class for security
**
** Date: March 22, 1998
**
=============================================================================*/

namespace System.Security {
	using System.Security;
	using System;
	using System.Runtime.Serialization;
    using System.Security.Permissions;

    /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException"]/*' />
    [Serializable] public class SecurityException : SystemException {

        [NonSerialized] private Type permissionType;
        private String permissionState;

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException"]/*' />
        public SecurityException() 
            : base(Environment.GetResourceString("Arg_SecurityException")) {
    		SetErrorCode(__HResults.COR_E_SECURITY);
        }
    
        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException1"]/*' />
        public SecurityException(String message) 
            : base(message)
        {
    		SetErrorCode(__HResults.COR_E_SECURITY);
        }

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException2"]/*' />
        public SecurityException(String message, Type type ) 
            : base(message)
        {
    		SetErrorCode(__HResults.COR_E_SECURITY);
            permissionType = type;
        }
    
        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException4"]/*' />
        public SecurityException(String message, Type type, String state ) 
            : base(message)
        {
    		SetErrorCode(__HResults.COR_E_SECURITY);
            permissionType = type;
            permissionState = state;
        }

    
        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException3"]/*' />
        public SecurityException(String message, Exception inner) 
            : base(message, inner) {
    		SetErrorCode(__HResults.COR_E_SECURITY);
        }

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.SecurityException5"]/*' />
        protected SecurityException(SerializationInfo info, StreamingContext context) : base (info, context) {
            if (info==null)
                throw new ArgumentNullException("info");

            try
            {
                permissionState = (String)info.GetValue("PermissionState",typeof(String));
            }
            catch (Exception)
            {
                permissionState = null;
            }
        }

		/// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.PermissionType"]/*' />
		public Type PermissionType
		{
			get
			{
				return permissionType;
			}
		}

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.PermissionState"]/*' />
        public String PermissionState
        {
            [SecurityPermissionAttribute( SecurityAction.Demand, Flags = SecurityPermissionFlag.ControlEvidence | SecurityPermissionFlag.ControlPolicy)]
            get
            {
                return permissionState;
            }
        }

        internal void SetPermissionState( String state )
        {
            permissionState = state;
        }

		/// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.ToString"]/*' />
		public override String ToString() 
		{
			String RetVal=base.ToString();
			try
			{
                if (PermissionState!=null)
                {
                   if (RetVal==null)
                        RetVal=" ";
                   RetVal+=Environment.NewLine;
                   RetVal+=Environment.NewLine;
                   RetVal+=Environment.GetResourceString( "Security_State" );
                   RetVal+=" " + Environment.NewLine;
                   RetVal+=PermissionState;
                }

			}
			catch(SecurityException)
			{
			
			}
			return RetVal;
		}

        /// <include file='doc\SecurityException.uex' path='docs/doc[@for="SecurityException.GetObjectData"]/*' />
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            if (info==null) {
                throw new ArgumentNullException("info");
            }

            base.GetObjectData( info, context );

            try
            {
                info.AddValue("PermissionState", PermissionState, typeof(String));
            }
            catch (SecurityException)
            {
            }
        }
            
    }

}
