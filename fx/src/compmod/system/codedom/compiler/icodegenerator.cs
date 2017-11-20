//------------------------------------------------------------------------------
// <copyright file="ICodeGenerator.cs" company="Microsoft">
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

    /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides an
    ///       interface for code generation.
    ///    </para>
    /// </devdoc>
    public interface ICodeGenerator {
        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.IsValidIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether
        ///       the specified value is a valid identifier for this language.
        ///    </para>
        /// </devdoc>
        bool IsValidIdentifier(string value);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.ValidateIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Throws an exception if value is not a valid identifier.
        ///    </para>
        /// </devdoc>
        void ValidateIdentifier(string value);
        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.CreateEscapedIdentifier"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        string CreateEscapedIdentifier(string value);
        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.CreateValidIdentifier"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        string CreateValidIdentifier(string value);
        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GetTypeOutput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        string GetTypeOutput(CodeTypeReference type);
        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.Supports"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        bool Supports(GeneratorSupport supports);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GenerateCodeFromExpression"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Generates code from the specified expression and
        ///       outputs it to the specified textwriter.
        ///    </para>
        /// </devdoc>
        void GenerateCodeFromExpression(CodeExpression e, TextWriter w, CodeGeneratorOptions o);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GenerateCodeFromStatement"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        void GenerateCodeFromStatement(CodeStatement e, TextWriter w, CodeGeneratorOptions o);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GenerateCodeFromNamespace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        void GenerateCodeFromNamespace(CodeNamespace e, TextWriter w, CodeGeneratorOptions o);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GenerateCodeFromCompileUnit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        void GenerateCodeFromCompileUnit(CodeCompileUnit e, TextWriter w, CodeGeneratorOptions o);

        /// <include file='doc\ICodeGenerator.uex' path='docs/doc[@for="ICodeGenerator.GenerateCodeFromType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Outputs the language specific representaion of the CodeDom tree
        ///       refered to by e, into w.
        ///    </para>
        /// </devdoc>
        void GenerateCodeFromType(CodeTypeDeclaration e, TextWriter w, CodeGeneratorOptions o);

    }
}
