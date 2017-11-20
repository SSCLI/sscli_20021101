//------------------------------------------------------------------------------
// <copyright file="IndentedWriter.cs" company="Microsoft">
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

namespace System.Xml.Serialization {

    using System.IO;
    
    /// <include file='doc\IndentedWriter.uex' path='docs/doc[@for="IndentedWriter"]/*' />
    /// <devdoc>
    ///     This class will write to a stream and manage indentation.
    /// </devdoc>
    internal class IndentedWriter {
        TextWriter writer;
        bool needIndent;
        int indentLevel;
        bool compact;
        
        public IndentedWriter(TextWriter writer, bool compact) {
            this.writer = writer;
            this.compact = compact;
        }

        public int Indent {
            get {
                return indentLevel;
            }
            set {
                indentLevel = value;
            }
        }
        
        public void Write(string s) {
            if (needIndent) WriteIndent();
            writer.Write(s);
        }
        
        public void Write(char c) {
            if (needIndent) WriteIndent();
            writer.Write(c);
        }
        
        public void WriteLine(string s) {
            if (needIndent) WriteIndent();
            writer.WriteLine(s);
            needIndent = true;
        }
        
        public void WriteLine() {
            writer.WriteLine();
            needIndent = true;
        }

        public void WriteIndent() {
            needIndent = false;
            if (!compact) {
                for (int i = 0; i < indentLevel; i++) {
                    writer.Write("    ");
                }
            }
        }
    }
}
