//------------------------------------------------------------------------------
// <copyright file="RegexCompilationInfo.cs" company="Microsoft">
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

namespace System.Text.RegularExpressions {
    using System;

    /// <include file='doc\RegexCompilationInfo.uex' path='docs/doc[@for="RegexCompilationInfo"]/*' />
    /// <devdoc>
    ///    <para>
    ///       [To be supplied]
    ///    </para>
    /// </devdoc>
    [ Serializable() ] 
    public class RegexCompilationInfo { 
        private String           pattern;
        private RegexOptions     options;
        private String           name;
        private String           nspace;
        private bool             isPublic;

        /// <include file='doc\RegexCompilationInfo.uex' path='docs/doc[@for="RegexCompilationInfo.RegexCompilationInfo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       [To be supplied]
        ///    </para>
        /// </devdoc>
        public RegexCompilationInfo(String pattern, RegexOptions options, String name, String fullnamespace, bool ispublic) {
            Pattern = pattern;
            Name = name;
            Namespace = fullnamespace;
            this.options = options;
            isPublic = ispublic;
        }

        /// <include file='doc\RegexCompilationInfo.uex' path='docs/doc[@for="RegexCompilationInfo.Pattern"]/*' />
        /// <devdoc>
        ///    <para>
        ///       [To be supplied]
        ///    </para>
        /// </devdoc>
        public String Pattern {
            get { return pattern; }
            set { 
                if (value == null)
                    throw new ArgumentNullException("value");
                pattern = value;
            }
        }

        /// <include file='doc\RegexCompilationInfo.uex' path='docs/doc[@for="RegexCompilationInfo.Options"]/*' />
        /// <devdoc>
        ///    <para>
        ///       [To be supplied]
        ///    </para>
        /// </devdoc>
        public RegexOptions Options {
            get { return options; }
            set { options = value;}
        }

        /// <include file='doc\RegexCompilationInfo.uex' path='docs/doc[@for="RegexCompilationInfo.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       [To be supplied]
        ///    </para>
        /// </devdoc>
        public String Name {
            get { return name; }
            set { 
                if (value == null)
                    throw new ArgumentNullException("value");
                name = value;
            }
        }

        /// <include file='doc\RegexCompilationInfo.uex' path='docs/doc[@for="RegexCompilationInfo.Namespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       [To be supplied]
        ///    </para>
        /// </devdoc>
        public String Namespace {
            get { return nspace; }
            set { 
                if (value == null)
                    throw new ArgumentNullException("value");
                nspace = value;
            }
        }

        /// <include file='doc\RegexCompilationInfo.uex' path='docs/doc[@for="RegexCompilationInfo.IsPublic"]/*' />
        /// <devdoc>
        ///    <para>
        ///       [To be supplied]
        ///    </para>
        /// </devdoc>
        public bool IsPublic {
            get { return isPublic; }
            set { isPublic = value;}
        }
    }
}


