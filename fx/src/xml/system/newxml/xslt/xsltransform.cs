//------------------------------------------------------------------------------
// <copyright file="XslTransform.cs" company="Microsoft">
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
    using System.Reflection;
    using System.Diagnostics;
    using System.IO;
    using System.Xml;
    using System.Xml.XPath;
    using System.Collections;
    using System.Xml.Xsl.Debugger;
    using System.Security.Permissions;

    /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform"]/*' />
    /// <devdoc>
    ///    <para>Transforms XML data using an XSL stylesheet.</para>
    /// </devdoc>

    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    public sealed class XslTransform {
        //
        // XmlResolver
        //

        private XmlResolver _XmlResolver;

        //
        // Compiled stylesheet state
        //

        private Stylesheet  _CompiledStylesheet;
        private ArrayList   _QueryStore;
        private RootAction  _RootAction;


        private IXsltDebugger debugger;
    
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.XslTransform"]/*' />
        public XslTransform() {}

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform4"]/*' />
        public XmlReader Transform(IXPathNavigable input, XsltArgumentList args) {
            return Transform( input.CreateNavigator(), args );
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform5"]/*' />
        public void Transform(IXPathNavigable input, XsltArgumentList args, TextWriter output) {
            Transform( input.CreateNavigator(), args, output );
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform6"]/*' />
        public void Transform(IXPathNavigable input, XsltArgumentList args, Stream output) {
            Transform( input.CreateNavigator(), args, output );
        }
 
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform7"]/*' />
        public void Transform(IXPathNavigable input, XsltArgumentList args, XmlWriter output) {
            Transform( input.CreateNavigator(), args, output );
        }

        //
        // XmlResolver methods
        //

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.XmlResolver"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the XmlResolver used by this XslTransform to resolve external
        ///       resources.</para>
        /// </devdoc>
        public XmlResolver XmlResolver {
            set {
                _XmlResolver = value;
            }
        }

        internal static XPathDocument LoadDocument(XmlTextReader reader) {
            XmlValidatingReader vr = new XmlValidatingReader(reader); {
	            vr.ValidationType = ValidationType.None;
	            vr.EntityHandling = EntityHandling.ExpandEntities;
            }
            try {
                return new XPathDocument(vr, XmlSpace.Preserve);
            }
            finally {
                vr.Close();
                reader.Close();
            }
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load"]/*' />
        /// <devdoc>
        ///    <para>Loads the XSL stylesheet specified by a URL.</para>
        /// </devdoc>
        public void Load(string url) {
            Load( url, null );
        }
        
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load4"]/*' />
        public void Load(string url, XmlResolver resolver) {
            Load( new XPathDocument(url, XmlSpace.Preserve ) , resolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load1"]/*' />
        /// <devdoc>
        ///    <para>Loads
        ///       the XSL stylesheet contained in the specified XmlReader.</para>
        /// </devdoc>
        public void Load(XmlReader stylesheet) {
            Load( stylesheet, null );
        }
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load5"]/*' />
        public void Load(XmlReader stylesheet, XmlResolver resolver) {
            Load( new XPathDocument(stylesheet, XmlSpace.Preserve ) , resolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load2"]/*' />
        /// <devdoc>
        ///    <para>Loads the XSL stylesheet contained in the specified XPathNavigator.</para>
        /// </devdoc>

        public void Load(XPathNavigator stylesheet) {
            Load( stylesheet, null );
        }
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load6"]/*' />
        public void Load(XPathNavigator stylesheet, XmlResolver resolver) {
            Compile( stylesheet, resolver );
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load3"]/*' />
        public void Load(IXPathNavigable stylesheet) {
            Load(stylesheet.CreateNavigator(), null);
        }
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load7"]/*' />
        public void Load(IXPathNavigable stylesheet, XmlResolver resolver) {
            Load(stylesheet.CreateNavigator(), resolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform"]/*' />
        /// <devdoc>
        ///    <para>Transforms the XML data in the specified XPathNavigator and
        ///       outputs the result to an XmlReader.</para>
        /// </devdoc>

        public XmlReader Transform(XPathNavigator input, XsltArgumentList args) {
            Processor    processor = new Processor(this, input, args, _XmlResolver);
            return processor.Reader;
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform1"]/*' />
        /// <devdoc>
        ///    <para>Transforms the XML data in the specified XPathNavigator and
        ///       outputs the result to an XmlWriter.</para>
        /// </devdoc>

        public void Transform(XPathNavigator input, XsltArgumentList args, XmlWriter output) {
            Processor    processor = new Processor(this, input, args, _XmlResolver, output);
            processor.Execute();        
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform2"]/*' />
        /// <devdoc>
        ///    <para>Transforms the XML data in the specified XPathNavigator and outputs the result
        ///       to a Stream.</para>
        /// </devdoc>

        public void Transform(XPathNavigator input, XsltArgumentList args, Stream output) {
            Processor    processor = new Processor(this, input, args, _XmlResolver, output);
            processor.Execute();        
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform3"]/*' />
        /// <devdoc>
        ///    <para>Transforms the XML data in the specified XPathNavigator and outputs the result
        ///       to a TextWriter.</para>
        /// </devdoc>

        public void Transform(XPathNavigator input, XsltArgumentList args, TextWriter output) {
            Processor    processor = new Processor(this, input, args, _XmlResolver, output);
            processor.Execute();        
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform8"]/*' />
        public void Transform(String inputfile, String outputfile) { 
            FileStream fs = null;
            try {
                fs = new FileStream(outputfile, FileMode.Create, FileAccess.ReadWrite);
                Transform(new XPathDocument(inputfile), null , fs);
            }
            finally {
                fs.Close();
            }
        }                

        //
        // Implementation
        //

        private void StoreCompiledStylesheet(Stylesheet compiledStylesheet, ArrayList queryStore, RootAction rootAction) {
            Debug.Assert(queryStore != null);
            Debug.Assert(compiledStylesheet != null);
            Debug.Assert(rootAction != null);

            //
            // Set the new state atomically
            //

           // lock(this) {
                _CompiledStylesheet = compiledStylesheet;
                _QueryStore         = queryStore;
                _RootAction         = rootAction;
            //    }
        }

        internal void LoadCompiledStylesheet(out Stylesheet compiledStylesheet, out XPathExpression[] querylist, out ArrayList queryStore, out RootAction rootAction, XPathNavigator input) {
            //
            // Extract the state atomically
            //
            if (_CompiledStylesheet == null || _QueryStore == null || _RootAction == null) {
                throw new XsltException(Res.Xslt_NoStylesheetLoaded);
            }

            compiledStylesheet  = _CompiledStylesheet;
            queryStore          = _QueryStore;
            rootAction          = _RootAction;
            int queryCount      = _QueryStore.Count;
            querylist           = new XPathExpression[queryCount]; {
                bool canClone = input is DocumentXPathNavigator || input is XPathDocumentNavigator;
                for(int i = 0; i < queryCount; i ++) {
                    XPathExpression query = ((TheQuery)_QueryStore[i]).CompiledQuery;
                    querylist[i] = canClone ? query.Clone() : input.Compile(query.Expression);
                }
            }
        }

        private void Compile(XPathNavigator stylesheet, XmlResolver resolver) {
            Debug.CompilingMessage();

            Compiler  compiler = (Debugger == null) ? new Compiler() : new DbgCompiler(this.Debugger);
            NavigatorInput input    = new NavigatorInput(stylesheet);
            compiler.Compile(input, resolver);                 
            StoreCompiledStylesheet(compiler.CompiledStylesheet, compiler.QueryStore, compiler.RootAction);
            TraceCompiledState(compiler);
        }

        [System.Diagnostics.Conditional("DEBUG")]
        void TraceCompiledState(Compiler compiler) {
            Debug.TracingMessage();
            compiler.CompiledStylesheet.Trace();
            Debug.Indent(0);
            Debug.WriteLine("RootAction:");
            compiler.RootAction.Trace();
        }

        internal IXsltDebugger Debugger {
            get {return this.debugger;}
        }


        internal XslTransform(object debugger) {
            if (debugger != null) {
                this.debugger = new DebuggerAddapter(debugger);
            }
        }

        private class DebuggerAddapter : IXsltDebugger {
            private object unknownDebugger;
            private MethodInfo getBltIn;
            private MethodInfo onCompile;
            private MethodInfo onExecute;
            public DebuggerAddapter(object unknownDebugger) {
                this.unknownDebugger = unknownDebugger;
                BindingFlags flags = BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static;
                Type unknownType = unknownDebugger.GetType();
                getBltIn  = unknownType.GetMethod("GetBuiltInTemplatesUri", flags);
                onCompile = unknownType.GetMethod("OnInstructionCompile"  , flags);
                onExecute = unknownType.GetMethod("OnInstructionExecute"  , flags);
            }
            // ------------------ IXsltDebugger ---------------
            public string GetBuiltInTemplatesUri() {
                if (getBltIn == null) {
                    return null;
                }
                return (string) getBltIn.Invoke(unknownDebugger, new object[] {});
            }
            public void OnInstructionCompile(XPathNavigator styleSheetNavigator) {
                if (onCompile != null) {
                    onCompile.Invoke(unknownDebugger, new object[] { styleSheetNavigator });
                }
            }
            public void OnInstructionExecute(IXsltProcessor xsltProcessor) {
                if (onExecute != null) {
                    onExecute.Invoke(unknownDebugger, new object[] { xsltProcessor });
                }
            }
        }
    }
}
