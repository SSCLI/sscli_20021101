//------------------------------------------------------------------------------
// <copyright file="AttributeSetAction.cs" company="Microsoft">
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
    using System.Collections;

    internal class AttributeSetAction : ContainerAction {
        internal XmlQualifiedName            name;

        internal XmlQualifiedName Name {
            get { return this.name; }
        }

        internal override void Compile(Compiler compiler) {
            CompileAttributes(compiler);
            CheckRequiredAttribute(compiler, this.name, Keywords.s_Name);
            CompileContent(compiler);
        }

        internal override bool CompileAttribute(Compiler compiler) {
            string name   = compiler.Input.LocalName;
            string value  = compiler.Input.Value;
            if (Keywords.Equals(name, compiler.Atoms.Name)) {
                Debug.Assert(this.name == null);
                this.name = compiler.CreateXPathQName(value);
                Debug.WriteLine("name attribute found: " + this.name);
            }
            else if (Keywords.Equals(name, compiler.Atoms.UseAttributeSets)) {
                // create a UseAttributeSetsAction
                // sets come before xsl:attributes
                AddAction(compiler.CreateUseAttributeSetsAction());
            }
            else {
                return false;
            }

            return true;
        }

        private void CompileContent(Compiler compiler) {
            NavigatorInput input = compiler.Input;

            if (compiler.Recurse()) {
                do {
                    Debug.Trace(input);

                    switch(input.NodeType) {
                    case XPathNodeType.Element:
                        compiler.PushNamespaceScope();

                        string nspace = input.NamespaceURI;
                        string name   = input.LocalName;

                        if (Keywords.Equals(nspace, input.Atoms.XsltNamespace) && Keywords.Equals(name, input.Atoms.Attribute)) {
                            // found attribute so add it
                            AddAction(compiler.CreateAttributeAction());
                        }
                        else {
                            throw XsltException.UnexpectedKeyword(compiler);
                        }
                        compiler.PopScope();
                        break;

                    case XPathNodeType.Comment:
                    case XPathNodeType.ProcessingInstruction:
                    case XPathNodeType.Whitespace:
                    case XPathNodeType.SignificantWhitespace:
                        break;

                    default:
                        throw new XsltException(Res.Xslt_InvalidContents, Keywords.s_AttributeSet);
                    }
                }
                while(compiler.Advance());

                compiler.ToParent();
            }
        }

        internal void Merge(AttributeSetAction attributeAction) {
            // add the contents of "attributeAction" to this action
            // place them at the end
            Action  action;
            int     i = 0;

            while((action = attributeAction.GetAction(i)) != null) {
                AddAction(action);
                i++;
            }
        }

        internal override void Trace(int tab) {
            Debug.Indent(tab);
            Debug.WriteLine("<xsl:attribute-set name=\"" + this.name + "\">");
            base.Trace(tab + 1);
            Debug.Indent(tab);
            Debug.WriteLine("</xsl:attribute-set>");
        }
    }
}
