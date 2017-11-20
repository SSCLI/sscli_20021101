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
** Class:  SatelliteContractVersionAttribute
**
**                                                    
**
** Purpose: Specifies which version of a satellite assembly 
**          the ResourceManager should ask for.
**
** Date:  December 8, 2000
**
===========================================================*/

using System;

namespace System.Resources {
    
    /// <include file='doc\SatelliteContractVersionAttribute.uex' path='docs/doc[@for="SatelliteContractVersionAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple=false)]  
    public sealed class SatelliteContractVersionAttribute : Attribute 
    {
        private String _version;

        /// <include file='doc\SatelliteContractVersionAttribute.uex' path='docs/doc[@for="SatelliteContractVersionAttribute.SatelliteContractVersionAttribute"]/*' />
        public SatelliteContractVersionAttribute(String version)
        {
            if (version == null)
                throw new ArgumentNullException("version");
            _version = version;
        }

        /// <include file='doc\SatelliteContractVersionAttribute.uex' path='docs/doc[@for="SatelliteContractVersionAttribute.Version"]/*' />
        public String Version {
            get { return _version; }
        }
    }
}
