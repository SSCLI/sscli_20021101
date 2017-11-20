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
** Class:  NeutralResourcesLanguageAttribute
**
**                                                    
**
** Purpose: Tells the ResourceManager what language your main
**          assembly's resources are written in.  The 
**          ResourceManager won't try loading a satellite
**          assembly for that culture, which helps perf.
**
** Date:  March 14, 2001
**
===========================================================*/

using System;

namespace System.Resources {
    
    /// <include file='doc\NeutralResourcesLanguageAttribute.uex' path='docs/doc[@for="NeutralResourcesLanguageAttribute"]/*' />
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple=false)]  
    public sealed class NeutralResourcesLanguageAttribute : Attribute 
    {
        private String _culture;

        /// <include file='doc\NeutralResourcesLanguageAttribute.uex' path='docs/doc[@for="NeutralResourcesLanguageAttribute.NeutralResourcesLanguageAttribute"]/*' />
        public NeutralResourcesLanguageAttribute(String cultureName)
        {
            if (cultureName == null)
                throw new ArgumentNullException("cultureName");
            _culture = cultureName;
        }

        /// <include file='doc\NeutralResourcesLanguageAttribute.uex' path='docs/doc[@for="NeutralResourcesLanguageAttribute.CultureName"]/*' />
        public String CultureName {
            get { return _culture; }
        }
    }
}
