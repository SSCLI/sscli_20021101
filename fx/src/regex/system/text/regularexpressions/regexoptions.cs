//------------------------------------------------------------------------------
// <copyright file="RegexOptions.cs" company="Microsoft">
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
#define ECMA


namespace System.Text.RegularExpressions {

using System;

    /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Flags]
    public enum RegexOptions {
        /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None =                     0x0000,  

        /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions.IgnoreCase"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        IgnoreCase =               0x0001,      // "i"

        /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions.Multiline"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Multiline =                0x0002,      // "m"
        
        /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions.ExplicitCapture"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ExplicitCapture =          0x0004,      // "n"
        
        /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions.Compiled"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Compiled =                 0x0008,      // "c"
        
        /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions.Singleline"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Singleline =               0x0010,      // "s"
        
        /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions.IgnorePatternWhitespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        IgnorePatternWhitespace =  0x0020,      // "x"
        
        /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions.RightToLeft"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        RightToLeft =              0x0040,      // "r"

#if DBG
        /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions.Debug"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Debug=                     0x0080,      // "d"
#endif

        /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions.ECMAScript"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ECMAScript =                  0x0100,      // "e"

        /// <include file='doc\RegexOptions.uex' path='docs/doc[@for="RegexOptions.CultureInvariant"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        CultureInvariant =                  0x0200,
    }


}

