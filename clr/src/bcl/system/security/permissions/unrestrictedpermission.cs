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
// UnrestrictedPermissionBase.cs
//

namespace System.Security.Permissions {
	using System.Security;
	using System;
	using System.IO;
	using System.Security.Util;
    internal class UnrestrictedPermission
    {
        // Returns true if OK to return from check, or false if
        // permission-specific information must be checked.
        internal static bool CheckUnrestricted(IUnrestrictedPermission grant, CodeAccessPermission demand)
        {
            // We return true here because we're defining a demand of null to
            // automatically pass.
            if (demand == null)
                return true;
    
            if (demand.GetType() != grant.GetType())
                return false;
            if (grant.IsUnrestricted())
                return true;
            if (((IUnrestrictedPermission)demand).IsUnrestricted())
                throw new SecurityException(String.Format(Environment.GetResourceString("Security_Generic"), demand.GetType().FullName), demand.GetType(), demand.ToXml().ToString());
            return false;
        } 
    }
}
