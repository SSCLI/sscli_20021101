//------------------------------------------------------------------------------
// <copyright file="IConfigurationSystem.cs" company="Microsoft">
//     
//      Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//     
//      The use and distribution terms for this software are contained in the file
//      named license.txt, which can be found in the root of this distribution.
//      By using this software in any fashion, you are agreeing to be bound by the
//      terms of this license.
//     
//      You must not remove this notice, or any other, from this software.
//     
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Configuration {

    /// <include file='doc\IConfigurationSystem.uex' path='docs/doc[@for="IConfigurationSystem"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// <para>
    /// The IConfigurationSystem interface defines the contract that a configuration
    /// system must implement.
    /// </para>
    /// </devdoc>
    public interface IConfigurationSystem {

        /// <include file='doc\IConfigurationSystem.uex' path='docs/doc[@for="IConfigurationSystem.GetConfig"]/*' />
        /// <devdoc>
        /// <para>
        /// Returns the config object for the specified key.
        /// </para>
        /// </devdoc>
        object GetConfig(string configKey);

        /// <include file='doc\IConfigurationSystem.uex' path='docs/doc[@for="IConfigurationSystem.Init"]/*' />
        /// <devdoc>
        /// <para>
        /// Initializes the configuration system.
        /// </para>
        /// </devdoc>
        void Init();
    }
}
