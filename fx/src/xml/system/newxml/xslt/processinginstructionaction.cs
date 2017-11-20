//------------------------------------------------------------------------------
// <copyright file="ProcessingInstructionAction.cs" company="Microsoft">
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

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;

    internal class ProcessingInstructionAction : ContainerAction {
        private const int NameEvaluated = 2;
        private const int NameReady     = 3;

        private Avt       nameAvt;
        // Compile time precalculated AVT
        private string    name; 

        private const char CharX = 'X';
        private const char Charx = 'x';
        private const char CharM = 'M';
        private const char Charm = 'm';
        private const char CharL = 'L';
        private const char Charl = 'l';

        internal ProcessingInstructionAction() {}

        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);
            CheckRequiredAttribute(compiler, this.nameAvt, Keywords.s_Name);

            if(this.nameAvt.IsConstant) {
                this.name    = this.nameAvt.Evaluate(null, null);
                this.nameAvt = null;
                if (! IsProcessingInstructionName(this.name)) {
                    // For Now: set to null to ignore action late;
                   this.name = null;
               }
            }

            if (compiler.Recurse()) {
                CompileTemplate(compiler);
                compiler.ToParent();
            }
        }

        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;
            if (Keywords.Equals(name, compiler.Atoms.Name)) {
                this.nameAvt = Avt.CompileAvt(compiler, value);
                Debug.WriteLine("name = \"" + value + "\"");
            }
            else {
                return false;
            }

            return true;
        }

        internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);

            switch (frame.State) {
            case Initialized:
                if(this.nameAvt == null) {
                    frame.StoredOutput = this.name;
                    if(this.name == null) {
                        // name was static but was bad;
                        frame.Finished();
                        break;
                    }
                }
                else {
                    frame.StoredOutput = this.nameAvt.Evaluate(processor, frame);
                    if (! IsProcessingInstructionName(frame.StoredOutput)) {
                        Debug.WriteLine("Invalid Processing instruction naame: \"" + name + "\"");
                        frame.Finished();
                        break;
                    }
                }
                goto case NameReady;

            case NameReady:
                Debug.Assert(frame.StoredOutput != null);
                if (processor.BeginEvent(XPathNodeType.ProcessingInstruction, string.Empty, frame.StoredOutput, string.Empty, false) == false) {
                    // Come back later
                    frame.State = NameReady;
                    break;
                }
                processor.PushActionFrame(frame);
                frame.State = ProcessingChildren;
                break;                              // Allow children to run

            case ProcessingChildren:
                if (processor.EndEvent(XPathNodeType.ProcessingInstruction) == false) {
                    Debug.WriteLine("Cannot end event, breaking, will restart");
                    frame.State = ProcessingChildren;
                    break;
                }
                frame.Finished();
                break;
            default:
                Debug.Fail("Invalid ElementAction execution state");
                frame.Finished();
                break;
            }
        }


        internal static bool IsProcessingInstructionName(string name) {
            if (name == null) {
                return false;
            }

            int nameLength = name.Length;
            int position   = 0;

            while (position < nameLength && XmlCharType.IsWhiteSpace(name[position])) {
                position ++;
            }

            if (position >= nameLength) {
                return false;
            }

            if (position < nameLength && ! XmlCharType.IsStartNCNameChar(name[position])) {
                return false;
            }

            while (position < nameLength && XmlCharType.IsNCNameChar(name[position])) {
                position ++;
            }

            while (position < nameLength && XmlCharType.IsWhiteSpace(name[position])) {
                position ++;
            }

            if (position < nameLength) {
                return false;
            }

            if (nameLength == 3 &&
                (name[0] == CharX || name[0] == Charx) &&
                (name[1] == CharM || name[1] == Charm) &&
                (name[2] == CharL || name[2] == Charl)
            ) {
                return false;
            }

            return true;
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:processing-instruction name=\"" + this.name + "\"");
            base.Trace(tab);
            Debug.Indent(tab);
            Debug.WriteLine("</xsl:processing-instruction>");
        }
    }
}
