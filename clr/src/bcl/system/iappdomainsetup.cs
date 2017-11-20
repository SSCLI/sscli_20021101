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
** Interface:  IAppDomainSetup
**
** Author: 
**
** Purpose: Properties exposed to COM
**
** Date:  
** 
===========================================================*/
namespace System {

    using System.Runtime.InteropServices;

    /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup"]/*' />
    public interface IAppDomainSetup
    {
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.ApplicationBase"]/*' />
        String ApplicationBase {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.ApplicationName"]/*' />

        String ApplicationName
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.CachePath"]/*' />

        String CachePath
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.ConfigurationFile"]/*' />

        String ConfigurationFile {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.DynamicBase"]/*' />

        String DynamicBase
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.LicenseFile"]/*' />

        String LicenseFile
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.PrivateBinPath"]/*' />

        String PrivateBinPath
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.PrivateBinPathProbe"]/*' />

        String PrivateBinPathProbe
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.ShadowCopyDirectories"]/*' />

        String ShadowCopyDirectories
        {
            get;
            set;
        }
        /// <include file='doc\IAppDomainSetup.uex' path='docs/doc[@for="IAppDomainSetup.ShadowCopyFiles"]/*' />

        String ShadowCopyFiles
        {
            get;
            set;
        }

    }
}
