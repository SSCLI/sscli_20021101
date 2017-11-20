//------------------------------------------------------------------------------
// <copyright file="ICodeCompiler.cs" company="Microsoft">
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

    using System.Diagnostics;
    using System.IO;

    /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a
    ///       code compilation
    ///       interface.
    ///    </para>
    /// </devdoc>
    public interface ICodeCompiler {

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromDom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an assembly based on options, with the information from
        ///       e.
        ///    </para>
        /// </devdoc>
        CompilerResults CompileAssemblyFromDom(CompilerParameters options, CodeCompileUnit compilationUnit);

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromFile"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an assembly based on options, with the contents of
        ///       fileName.
        ///    </para>
        /// </devdoc>
        CompilerResults CompileAssemblyFromFile(CompilerParameters options, string fileName);

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromSource"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an assembly based on options, with the information from
        ///       source.
        ///    </para>
        /// </devdoc>
        CompilerResults CompileAssemblyFromSource(CompilerParameters options, string source);

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromDomBatch"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compiles an assembly based on the specified options and
        ///       information.
        ///    </para>
        /// </devdoc>
        CompilerResults CompileAssemblyFromDomBatch(CompilerParameters options, CodeCompileUnit[] compilationUnits);

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromFileBatch"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compiles
        ///       an
        ///       assembly based on the specified options and contents of the specified
        ///       filenames.
        ///    </para>
        /// </devdoc>
        CompilerResults CompileAssemblyFromFileBatch(CompilerParameters options, string[] fileNames);

        /// <include file='doc\ICodeCompiler.uex' path='docs/doc[@for="ICodeCompiler.CompileAssemblyFromSourceBatch"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compiles an assembly based on the specified options and information from the specified
        ///       sources.
        ///    </para>
        /// </devdoc>
        CompilerResults CompileAssemblyFromSourceBatch(CompilerParameters options, string[] sources);

    }
}
