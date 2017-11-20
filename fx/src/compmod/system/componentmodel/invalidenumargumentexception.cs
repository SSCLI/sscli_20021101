//------------------------------------------------------------------------------
// <copyright file="InvalidEnumArgumentException.cs" company="Microsoft">
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

namespace System.ComponentModel {
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;

    /// <include file='doc\InvalidEnumArgumentException.uex' path='docs/doc[@for="InvalidEnumArgumentException"]/*' />
    /// <devdoc>
    ///    <para>The exception that is thrown when using invalid arguments that are enumerators.</para>
    /// </devdoc>
    public class InvalidEnumArgumentException : ArgumentException {

        /// <include file='doc\InvalidEnumArgumentException.uex' path='docs/doc[@for="InvalidEnumArgumentException.InvalidEnumArgumentException"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.InvalidEnumArgumentException'/> class without a message.</para>
        /// </devdoc>
        public InvalidEnumArgumentException() : this(null) {
        }

        /// <include file='doc\InvalidEnumArgumentException.uex' path='docs/doc[@for="InvalidEnumArgumentException.InvalidEnumArgumentException1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.InvalidEnumArgumentException'/> class with 
        ///    the specified message.</para>
        /// </devdoc>
        public InvalidEnumArgumentException(string message)
            : base(message) {
        }

        /// <include file='doc\InvalidEnumArgumentException.uex' path='docs/doc[@for="InvalidEnumArgumentException.InvalidEnumArgumentException2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.InvalidEnumArgumentException'/> class with a 
        ///    message generated from the argument, invalid value, and enumeration
        ///    class.</para>
        /// </devdoc>
        public InvalidEnumArgumentException(string argumentName, int invalidValue, Type enumClass)
            : base(SR.GetString(SR.InvalidEnumArgument,
                                argumentName,
                                invalidValue.ToString(),
                                enumClass.Name), argumentName) {
        }
    }
}
