//------------------------------------------------------------------------------
// <copyright file="IndentTextWriter.cs" company="Microsoft">
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
    using System;
    using System.IO;
    using System.Text;
    using System.Security.Permissions;

    /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter"]/*' />
    /// <devdoc>
    ///    <para>Provides a text writer that can indent new lines by a tabString token.</para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public class IndentedTextWriter : TextWriter {
        private TextWriter writer;
        private int indentLevel;
        private bool tabsPending;
        private string tabString;

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.DefaultTabString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public const string DefaultTabString = "    ";

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.IndentedTextWriter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.Compiler.IndentedTextWriter'/> using the specified
        ///       text writer and default tab string.
        ///    </para>
        /// </devdoc>
        public IndentedTextWriter(TextWriter writer) : this(writer, DefaultTabString) {
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.IndentedTextWriter1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.CodeDom.Compiler.IndentedTextWriter'/> using the specified
        ///       text writer and tab string.
        ///    </para>
        /// </devdoc>
        public IndentedTextWriter(TextWriter writer, string tabString) {
            this.writer = writer;
            this.tabString = tabString;
            indentLevel = 0;
            tabsPending = false;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Encoding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Encoding Encoding {
            get {
                return writer.Encoding;
            }
        }
                                                
        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.NewLine"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the new line character to use.
        ///    </para>
        /// </devdoc>
        public override string NewLine {
            get {
                return writer.NewLine;
            }

            set {
                writer.NewLine = value;
            }
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Indent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the number of spaces to indent.
        ///    </para>
        /// </devdoc>
        public int Indent {
            get {
                return indentLevel;
            }
            set {
                Debug.Assert(value >= 0, "Bogus Indent... probably caused by mismatched Indent++ and Indent--");
                if (value < 0) {
                    value = 0;
                }
                indentLevel = value;
            }
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.InnerWriter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the TextWriter to use.
        ///    </para>
        /// </devdoc>
        public TextWriter InnerWriter {
            get {
                return writer;
            }
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes the document being written to.
        ///    </para>
        /// </devdoc>
        public override void Close() {
            writer.Close();
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Flush"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void Flush() {
            writer.Flush();
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.OutputTabs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OutputTabs() {
            if (tabsPending) {
                for (int i=0; i < indentLevel; i++) {
                    writer.Write(tabString);
                }
                tabsPending = false;
            }
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes a string
        ///       to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(string s) {
            OutputTabs();
            writer.Write(s);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of a Boolean value to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(bool value) {
            OutputTabs();
            writer.Write(value);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes a character to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(char value) {
            OutputTabs();
            writer.Write(value);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes a
        ///       character array to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(char[] buffer) {
            OutputTabs();
            writer.Write(buffer);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes a subarray
        ///       of characters to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(char[] buffer, int index, int count) {
            OutputTabs();
            writer.Write(buffer, index, count);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of a Double to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(double value) {
            OutputTabs();
            writer.Write(value);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of
        ///       a Single to the text
        ///       stream.
        ///    </para>
        /// </devdoc>
        public override void Write(float value) {
            OutputTabs();
            writer.Write(value);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write7"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of an integer to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(int value) {
            OutputTabs();
            writer.Write(value);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write8"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of an 8-byte integer to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(long value) {
            OutputTabs();
            writer.Write(value);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write9"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of an object
        ///       to the text stream.
        ///    </para>
        /// </devdoc>
        public override void Write(object value) {
            OutputTabs();
            writer.Write(value);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write10"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out a formatted string, using the same semantics as specified.
        ///    </para>
        /// </devdoc>
        public override void Write(string format, object arg0) {
            OutputTabs();
            writer.Write(format, arg0);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write11"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out a formatted string,
        ///       using the same semantics as specified.
        ///    </para>
        /// </devdoc>
        public override void Write(string format, object arg0, object arg1) {
            OutputTabs();
            writer.Write(format, arg0, arg1);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.Write12"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes out a formatted string,
        ///       using the same semantics as specified.
        ///    </para>
        /// </devdoc>
        public override void Write(string format, params object[] arg) {
            OutputTabs();
            writer.Write(format, arg);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLineNoTabs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the specified
        ///       string to a line without tabs.
        ///    </para>
        /// </devdoc>
        public void WriteLineNoTabs(string s) {
            writer.WriteLine(s);
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the specified string followed by
        ///       a line terminator to the text stream.
        ///    </para>
        /// </devdoc>
        public override void WriteLine(string s) {
            OutputTabs();
            writer.WriteLine(s);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes a line terminator.
        ///    </para>
        /// </devdoc>
        public override void WriteLine() {
            OutputTabs();
            writer.WriteLine();
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the text representation of a Boolean followed by a line terminator to
        ///       the text stream.
        ///    </para>
        /// </devdoc>
        public override void WriteLine(bool value) {
            OutputTabs();
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(char value) {
            OutputTabs();
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(char[] buffer) {
            OutputTabs();
            writer.WriteLine(buffer);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(char[] buffer, int index, int count) {
            OutputTabs();
            writer.WriteLine(buffer, index, count);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(double value) {
            OutputTabs();
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(float value) {
            OutputTabs();
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine8"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(int value) {
            OutputTabs();
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine9"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(long value) {
            OutputTabs();
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine10"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(object value) {
            OutputTabs();
            writer.WriteLine(value);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine11"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(string format, object arg0) {
            OutputTabs();
            writer.WriteLine(format, arg0);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine12"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(string format, object arg0, object arg1) {
            OutputTabs();
            writer.WriteLine(format, arg0, arg1);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine13"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void WriteLine(string format, params object[] arg) {
            OutputTabs();
            writer.WriteLine(format, arg);
            tabsPending = true;
        }

        /// <include file='doc\IndentTextWriter.uex' path='docs/doc[@for="IndentedTextWriter.WriteLine14"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [CLSCompliant(false)]
        public override void WriteLine(UInt32 value) {
            OutputTabs();
            writer.WriteLine(value);
            tabsPending = true;
        }
    }

}
