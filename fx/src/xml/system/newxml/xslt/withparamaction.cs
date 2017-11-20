//------------------------------------------------------------------------------
// <copyright file="WithParamAction.cs" company="Microsoft">
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
    using System.Collections;
    using System.Xml;
    using System.Xml.XPath;

    internal class WithParamAction : VariableAction {
        internal WithParamAction() : base(VariableType.WithParameter) {}

        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);
            CheckRequiredAttribute(compiler, this.name, Keywords.s_Name);
            if (compiler.Recurse()) {
                CompileTemplate(compiler);
                compiler.ToParent();

                if (this.select != null && this.containedActions != null) {
                    throw new XsltException(Res.Xslt_VariableCntSel, this.nameStr, this.select);
                }
            }
        }
        
        internal override void Execute(Processor processor, ActionFrame frame) {
            Debug.Assert(processor != null && frame != null);
            object ParamValue;
            switch(frame.State) {
            case Initialized:           
                if (this.selectKey != Compiler.InvalidQueryKey) {
                    ParamValue = processor.RunQuery(frame, this.selectKey);
                    processor.SetParameter(this.name, ParamValue);
                    frame.Finished();
                }
                else {
                    if (this.containedActions == null) {
                        processor.SetParameter(this.name, String.Empty);
                        frame.Finished();
                        break;
                    }
                    NavigatorOutput output = new NavigatorOutput();
                    processor.PushOutput(output);
                    processor.PushActionFrame(frame);
                    frame.State = ProcessingChildren;
                }
                break;
            case ProcessingChildren:
                RecordOutput recOutput = processor.PopOutput();
                Debug.Assert(recOutput is NavigatorOutput);
                processor.SetParameter(this.name,((NavigatorOutput)recOutput).Navigator);
                frame.Finished();
                break;
            default:
                Debug.Fail("Invalid execution state inside VariableAction.Execute");
		    break;
            }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:variable select=\"" + this.select + "\" name=\"" + this.name + "\">");
            base.Trace(tab);
            Debug.Indent(tab);
            Debug.WriteLine("</xsl:variable>");
        }
    }
}
