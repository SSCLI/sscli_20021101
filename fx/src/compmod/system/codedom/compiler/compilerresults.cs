//------------------------------------------------------------------------------
// <copyright file="CompilerResults.cs" company="Microsoft">
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

namespace System.CodeDom.Compiler {
    using System;
    using System.CodeDom;
    using System.Reflection;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Security.Permissions;


    /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the results
    ///       of compilation from the compiler.
    ///    </para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class CompilerResults {
        private CompilerErrorCollection errors = new CompilerErrorCollection();
        private StringCollection output = new StringCollection();
        private Assembly compiledAssembly;
        private string pathToAssembly;
        private int nativeCompilerReturnValue;
        private TempFileCollection tempFiles;

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.CompilerResults"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.Compiler.CompilerResults'/>
        ///       that uses the specified
        ///       temporary files.
        ///    </para>
        /// </devdoc>
        public CompilerResults(TempFileCollection tempFiles) {
            this.tempFiles = tempFiles;
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.TempFiles"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the temporary files to use.
        ///    </para>
        /// </devdoc>
        public TempFileCollection TempFiles {
            get {
                return tempFiles;
            }
            set {
                tempFiles = value;
            }
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.CompiledAssembly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The compiled assembly.
        ///    </para>
        /// </devdoc>
        public Assembly CompiledAssembly {
            get {
                if (compiledAssembly == null && pathToAssembly != null) {
                    AssemblyName assemName = new AssemblyName();
                    assemName.CodeBase = pathToAssembly;
                    compiledAssembly = Assembly.Load(assemName);
                }
                return compiledAssembly;
            }
            set {
                compiledAssembly = value;
            }
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.Errors"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the collection of compiler errors.
        ///    </para>
        /// </devdoc>
        public CompilerErrorCollection Errors {
            get {
                return errors;
            }
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.Output"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the compiler output messages.
        ///    </para>
        /// </devdoc>
        public StringCollection Output {
            get {
                return output;
            }
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.PathToAssembly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the path to the assembly.
        ///    </para>
        /// </devdoc>
        public string PathToAssembly {
            get {
                return pathToAssembly;
            }
            set {
                pathToAssembly = value;
            }
        }

        /// <include file='doc\CompilerResults.uex' path='docs/doc[@for="CompilerResults.NativeCompilerReturnValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the compiler's return value.
        ///    </para>
        /// </devdoc>
        public int NativeCompilerReturnValue {
            get {
                return nativeCompilerReturnValue;
            }
            set {
                nativeCompilerReturnValue = value;
            }
        }
    }
}

