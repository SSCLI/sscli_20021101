//------------------------------------------------------------------------------
// <copyright file="Compiler.cs" company="Microsoft">
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
    using System.Globalization;
    using System.Xml;
    using System.Xml.XPath;
    using System.Xml.Xsl.Debugger;
    using System.Text;
    using System.Collections;
    using System.IO;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using System.Reflection;

    internal class Sort {
        internal int          select;
        internal string       lang;
        internal XmlDataType  dataType;
        internal XmlSortOrder order;
        internal XmlCaseOrder caseOrder;

        public Sort(int sortkey, String xmllang, XmlDataType datatype, XmlSortOrder xmlorder, XmlCaseOrder xmlcaseorder) {
            select    = sortkey;
            lang      = xmllang;
            dataType  = datatype;
            order     = xmlorder;
            caseOrder = xmlcaseorder;
        }
    }

    internal enum ScriptingLanguage {
        JScript,
        VisualBasic,
        CSharp
    }

    internal class DecimalFormat {
        internal NumberFormatInfo info;
        internal char digit;
        internal char zeroDigit;
        internal char patternSeparator;

        internal DecimalFormat(NumberFormatInfo Info, char Digit,char ZeroDigit, char PatternSeparator) {
            this.info = Info;
            this.digit = Digit;
            this.zeroDigit = ZeroDigit;
            this.patternSeparator = PatternSeparator;
        }
    }
    
    internal class Compiler {
        //
        // Predefined query keys
        //
        
        internal const int    InvalidQueryKey   = -1;
        internal const int    RootQueryKey      = 0;
        internal const int    SelfQueryKey      = 1;
        internal const int    PrecedingSiblingKey = 2;
        internal const int    DescendantKey     = 3;
        internal const int    DescendantTextKey = 4;

        internal const string RootQuery         = "/";
        internal const string SelfQuery         = ".";
        internal const string PrecedingSibling  = "preceding-sibling::node()";
        internal const string Descendant        = "descendant::node()";
        internal const string DescendantText    = "descendant::text()";

        internal const double RootPriority   = 0.5;

        // cached StringBuilder for AVT parseing
        internal StringBuilder AvtStringBuilder = new StringBuilder();

        private int                 stylesheetid; // Root stylesheet has id=0. We are using this in CompileImports to compile BuiltIns
        private InputScope          rootScope;

        //
        // Object members
        private XmlResolver         xmlResolver;

        //
        // Template being currently compiled
        private TemplateBaseAction  currentTemplate;
        private XmlQualifiedName    currentMode;
        private Hashtable           globalNamespaceAliasTable;
   
        //
        // Current import stack
        private Stack               stylesheets;

        private Hashtable           documentURIs;

        // import/include documents, who is here has its URI in this.documentURIs
        private NavigatorInput      input;

        // Atom table & InputScopeManager - cached top of the this.input stack
        private Keywords            atoms;
        private InputScopeManager   scopeManager;

        //
        // Compiled stylesheet state
        internal Stylesheet         stylesheet;
        internal Stylesheet         rootStylesheet;
        private RootAction          rootAction;
        private ArrayList           queryStore;
        private QueryBuilder        queryBuilder = new QueryBuilder();

        // Used to load Built In templates
        public  bool                AllowBuiltInMode;
        public  static XmlQualifiedName BuiltInMode = new XmlQualifiedName("*", string.Empty);

        internal Keywords Atoms {
            get {
                Debug.Assert(this.atoms != null);
                return this.atoms;
            }
        }

        internal int Stylesheetid {
            get{ return this.stylesheetid; }
            set { this.stylesheetid = value; }
        }
        
        internal NavigatorInput Document {
            get { return this.input; }
        }

        internal NavigatorInput Input {
            get { return this.input; }
        }

        internal bool Advance() {
            Debug.Assert(Document != null);
            return Document.Advance();
        }

        internal bool Recurse() {
            Debug.Assert(Document != null);
            return Document.Recurse();
        }

        internal bool ToParent() {
            Debug.Assert(Document != null);
            return Document.ToParent();
        }

        internal Stylesheet CompiledStylesheet {
            get { return this.stylesheet; }
        }

        internal RootAction RootAction {
            get { return this.rootAction; }
            set { 
                Debug.Assert(this.rootAction == null);
                this.rootAction = value; 
                Debug.Assert(this.currentTemplate == null);
                this.currentTemplate = this.rootAction;
            }
        }

        internal ArrayList QueryStore {
            get { return this.queryStore; }
        }

        public virtual IXsltDebugger Debugger { get { return null; } }

        //
        // The World of Compile
        //

        internal void Compile(NavigatorInput input, XmlResolver xmlResolver) {
            Debug.Assert(this.input == null && this.atoms == null);
            this.xmlResolver = xmlResolver == null ? new XmlUrlResolver() : xmlResolver;

            PushInputDocument(input, input.BaseURI);
            this.rootScope  = this.scopeManager.PushScope();
            this.queryStore = new ArrayList();
            AddSpecialQueries();

            try {
                this.rootStylesheet = new Stylesheet();
                PushStylesheet(this.rootStylesheet);

                Debug.Assert(this.input != null && this.atoms != null);

                try {
                    this.CreateRootAction();
                }
                catch(Exception e) {
                    throw new XsltCompileException(e, this.Input.BaseURI, this.Input.LineNumber, this.Input.LinePosition);
                }

                this.stylesheet.ProcessTemplates();
                this.rootAction.PorcessAttributeSets(this.rootStylesheet);
                this.stylesheet.SortWhiteSpace();
                CompileScript();
                this.rootAction.SortVariables();

                if (this.globalNamespaceAliasTable != null ) {
                    this.stylesheet.ReplaceNamespaceAlias(this);
                    this.rootAction.ReplaceNamespaceAlias(this);
		        }  
	        }
            finally {
                PopInputDocument();
            }

            Debug.Assert(this.rootAction != null);
            Debug.Assert(this.stylesheet != null);
            Debug.Assert(this.queryStore != null);
            Debug.Assert(this.input == null && this.atoms == null);
        }

        //
        // Input scope compiler's support
        //

        internal bool ForwardCompatibility {
            get { return this.scopeManager.CurrentScope.ForwardCompatibility;  }
            set { this.scopeManager.CurrentScope.ForwardCompatibility = value; }
        }
        
        internal bool CanHaveApplyImports {
            get { return this.scopeManager.CurrentScope.CanHaveApplyImports;  }
            set { this.scopeManager.CurrentScope.CanHaveApplyImports = value; }
        }
        
        internal void InsertExtensionNamespace(string value) {
            if (value == null) {
                return ;
            }
            String[] prefixes = SplitString(value);

            for (int i= 0; i < prefixes.Length; i++) {
                this.scopeManager.InsertExtensionNamespace(
                    prefixes[i] == Keywords.s_HashDefault ? DefaultNamespace : ResolveXmlNamespace(prefixes[i])
                );
            }       
        }

        internal void InsertExcludedNamespace(string value) {
            if (value == null) {
                return ;
            }
            String[] prefixes = SplitString(value);

            for (int i= 0; i < prefixes.Length; i++) {
                this.scopeManager.InsertExcludedNamespace(
                    prefixes[i] == Keywords.s_HashDefault ? DefaultNamespace : ResolveXmlNamespace(prefixes[i])
                );
            }
        }

        internal void InsertExtensionNamespace() {
            InsertExtensionNamespace(Input.Navigator.GetAttribute(Input.Atoms.ExtensionElementPrefixes, Input.Atoms.XsltNamespace));
        }

        internal void InsertExcludedNamespace() {
            InsertExcludedNamespace(Input.Navigator.GetAttribute(Input.Atoms.ExcludeResultPrefixes, Input.Atoms.XsltNamespace));
        }
        
        internal bool IsExtensionNamespace(String nspace) {
            return this.scopeManager.IsExtensionNamespace(nspace);
        }

        internal bool IsExcludedNamespace(String nspace) {
            return this.scopeManager.IsExcludedNamespace(nspace);
        }
        
        internal void PushLiteralScope() {
            PushNamespaceScope();
            string value = Input.Navigator.GetAttribute(Atoms.Version, Atoms.XsltNamespace);
            if (value != string.Empty) {
                ForwardCompatibility = (value != Keywords.s_Version10);
            }
        }

        internal void PushNamespaceScope() {
            this.scopeManager.PushScope();
            NavigatorInput input  = Input;

            if (input.MoveToFirstNamespace()) {
                do {
                    this.scopeManager.PushNamespace(input.LocalName, input.Value);
                }
                while(input.MoveToNextNamespace());
                input.ToParent();
            }
        }

        protected InputScopeManager  ScopeManager {
            get { return this.scopeManager; }
        }

        internal virtual void PopScope() {
            this.currentTemplate.ReleaseVariableSlots(this.scopeManager.CurrentScope.GetVeriablesCount());
            this.scopeManager.PopScope();
        }

        internal InputScopeManager CloneScopeManager() {
            return this.scopeManager.Clone();
        }

        //
        // Variable support
        //

        internal int InsertVariable(VariableAction variable) {
            InputScope varScope;
            if (variable.IsGlobal) {
                Debug.Assert(this.rootAction != null);
                varScope = this.rootScope;
            }
            else {
                Debug.Assert(this.currentTemplate != null);
                Debug.Assert(variable.VarType == VariableType.LocalVariable || variable.VarType == VariableType.LocalParameter || variable.VarType == VariableType.WithParameter);
                varScope = this.scopeManager.VariableScope;
            }

            VariableAction oldVar = varScope.ResolveVariable(variable.Name);
            if (oldVar != null) {
                // Other variable with this name is visible in this scope
                if (oldVar.IsGlobal) {
                    if (variable.IsGlobal) {
                        // Global Vars replace each other base on import presidens odred
                        if(variable.Stylesheetid == oldVar.Stylesheetid) {
                            // Both vars are in the same stylesheet
                            throw new XsltException(Res.Xslt_DupVarName, variable.NameStr);
                        }
                        else if(variable.Stylesheetid < oldVar.Stylesheetid) {
                            // newly defined var is more importent
                            varScope.InsertVariable(variable);
                            return oldVar.VarKey;
                        }
                        else {
                            // we egnore new variable
                            return InvalidQueryKey; // We didn't add this var, so doesn't matter what VarKey we return;
                        }
                    }
                    else {
                        // local variable can shadow global
                    }
                }
                else {
                    // Local variable never can be "shadowed"
                    throw new XsltException(Res.Xslt_DupVarName, variable.NameStr);
                }
            }

            varScope.InsertVariable(variable);
            return this.currentTemplate.AllocateVariableSlot();
        }
        
        internal void AddNamespaceAlias(String StylesheetURI, NamespaceInfo AliasInfo){
            if (this.globalNamespaceAliasTable == null) {
                this.globalNamespaceAliasTable = new Hashtable();            
            }
            NamespaceInfo duplicate = this.globalNamespaceAliasTable[StylesheetURI] as NamespaceInfo;
            if (duplicate == null || AliasInfo.stylesheetId <= duplicate.stylesheetId) {
                this.globalNamespaceAliasTable[StylesheetURI] = AliasInfo;
            }
        }
        
        internal bool IsNamespaceAlias(String StylesheetURI){
            if (this.globalNamespaceAliasTable == null) {
                return false;
            }
            return this.globalNamespaceAliasTable.Contains(StylesheetURI);
        }
        
        internal NamespaceInfo FindNamespaceAlias(String StylesheetURI) {
            if (this.globalNamespaceAliasTable != null) {
               return (NamespaceInfo)this.globalNamespaceAliasTable[StylesheetURI]; 
            }   
            return null;
        }

        internal String ResolveXmlNamespace(String prefix) {
            return this.scopeManager.ResolveXmlNamespace(prefix);
        }

        internal String ResolveXPathNamespace(String prefix) {
            return this.scopeManager.ResolveXPathNamespace(prefix);
        }

        internal String DefaultNamespace {
            get{ return this.scopeManager.DefaultNamespace; }
        }

        internal void InsertKey(XmlQualifiedName name, int MatchKey, int UseKey) {
            this.rootAction.InsertKey(name, MatchKey, UseKey);
        }

        internal void AddDecimalFormat(XmlQualifiedName name, DecimalFormat formatinfo) {
            this.rootAction.AddDecimalFormat(name, formatinfo);
        }
                
        //
        // Attribute parsing support
        //

        internal bool GetYesNo(string value) {
            Debug.Assert(value != null);
            Debug.Assert((object)value == (object)Input.Value); // this is always true. Why we passing value to this function.
//            if (value == null)
//                return false;  // in XSLT it happens that default value was always false;
            if (value.Equals(Atoms.Yes)) {
                return true;
            }
            if (value.Equals(Atoms.No)) {
                return false;
            }
            throw XsltException.InvalidAttrValue(Input.LocalName, value);
        }

        internal string GetSingleAttribute(string attributeAtom) {
            NavigatorInput input   = Input;
            string         element = input.LocalName;
            string         value   = null;

            if (input.MoveToFirstAttribute()) {
                do {
                    Debug.TraceAttribute(input);
                    string nspace = input.NamespaceURI;
                    string name   = input.LocalName;

                    if (! Keywords.Equals(nspace, Atoms.Empty)) continue;

                    if (Keywords.Equals(name, attributeAtom)) {
                        value = input.Value;
                    }
                    else {
                        if (! this.ForwardCompatibility) {
                             throw XsltException.InvalidAttribute(element, name);
                        }
                    }
                }
                while (input.MoveToNextAttribute());
                input.ToParent();
            }

            if (value == null) {
                throw new XsltException(Res.Xslt_MissingAttribute, attributeAtom);
            }
            return value;
        }

        internal XmlQualifiedName CreateXPathQName(string qname) {
            string prefix, local;
            PrefixQName.ParseQualifiedName(qname, out prefix, out local);

            return new XmlQualifiedName(local, this.scopeManager.ResolveXPathNamespace(prefix));
        }

        internal XmlQualifiedName CreateXmlQName(string qname) {
            string prefix, local;
            PrefixQName.ParseQualifiedName(qname, out prefix, out local);

            return new XmlQualifiedName(local, this.scopeManager.ResolveXmlNamespace(prefix));
        }

        //
        // Input documents management
        //

        private void AddDocumentURI(string href) {
            if (this.documentURIs == null) {
                this.documentURIs = new Hashtable();
            }

            if (this.documentURIs.Contains(href)) {
                throw new XsltException(Res.Xslt_CircularInclude, href);
            }

            this.documentURIs.Add(href, null);
        }

        private void RemoveDocumentURI(string href) {
            if (this.documentURIs != null) {
                Debug.Assert(this.documentURIs.Contains(href));
                this.documentURIs.Remove(href);
            }
        }

        internal NavigatorInput ResolveDocument(string relativeUri) {
            Debug.Assert(this.xmlResolver != null);
            string baseUri = this.Input.BaseURI;
            Uri ruri = this.xmlResolver.ResolveUri((baseUri != String.Empty) ? this.xmlResolver.ResolveUri(null, baseUri) : null, relativeUri);
            object input = this.xmlResolver.GetEntity(ruri, null, null);
            string resolved = ruri.ToString();

            if (input is Stream) {
                XmlTextReader tr  = new XmlTextReader(resolved, (Stream) input); {
                    tr.XmlResolver = this.xmlResolver;
                }
                // reader is closed by XslTransform.LoadDocument()
                return new NavigatorInput(XslTransform.LoadDocument(tr), resolved, rootScope);
            }
            else if (input is XPathNavigator) {
                return new NavigatorInput((XPathNavigator) input, resolved, rootScope);
            }
            else {
                throw new XsltException(Res.Xslt_CantResolve, relativeUri);
            }
        }

        internal void PushInputDocument(NavigatorInput newInput, string inputUri) {
            Debug.Assert(newInput != null);
            Debug.WriteLine("Pushing document \"" + inputUri + "\"");

            AddDocumentURI(inputUri);

            newInput.Next     = this.input;
            this.input        = newInput;
            this.atoms        = this.input.Atoms;
            this.scopeManager = this.input.InputScopeManager;
        }

        internal void PopInputDocument() {
            Debug.Assert(this.input != null);
            Debug.Assert(this.input.Atoms == this.atoms);

            NavigatorInput lastInput = this.input;

            this.input     = lastInput.Next;
            lastInput.Next = null;

            if (this.input != null) {
                this.atoms        = this.input.Atoms;
                this.scopeManager = this.input.InputScopeManager;
            }
            else {
                this.atoms        = null;
                this.scopeManager = null;
            }

            RemoveDocumentURI(lastInput.Href);
            lastInput.Close();
        }

        //
        // Stylesheet management
        //

        internal void PushStylesheet(Stylesheet stylesheet) {
            if (this.stylesheets == null) {
                this.stylesheets = new Stack();
            }
            Debug.Assert(this.stylesheets != null);

            this.stylesheets.Push(stylesheet);
            this.stylesheet = stylesheet;
        }

        internal Stylesheet PopStylesheet() {
            Debug.Assert(this.stylesheet == this.stylesheets.Peek());
            Stylesheet stylesheet = (Stylesheet) this.stylesheets.Pop();
            this.stylesheet = (Stylesheet) this.stylesheets.Peek();
            return stylesheet;
        }

        //
        // Attribute-Set management
        //

        internal void AddAttributeSet(AttributeSetAction attributeSet) {
            Debug.Assert(this.stylesheet == this.stylesheets.Peek());
            this.stylesheet.AddAttributeSet(attributeSet);
        }

        //
        // Template management
        //

        internal void AddTemplate(TemplateAction template) {
            Debug.Assert(this.stylesheet == this.stylesheets.Peek());
            this.stylesheet.AddTemplate(template);
        }

        internal void BeginTemplate(TemplateAction template) {
            Debug.Assert(this.currentTemplate != null);
            this.currentTemplate = template;
            this.currentMode     = template.Mode;
            this.CanHaveApplyImports = template.MatchKey != Compiler.InvalidQueryKey;
        }

        internal void EndTemplate() {
            Debug.Assert(this.currentTemplate != null);
            this.currentTemplate = this.rootAction;
        }

        internal XmlQualifiedName CurrentMode {
            get { return this.currentMode; }
        }

        //
        // Query management
        //

        private void AddSpecialQueries() {
            Debug.Assert(this.queryStore.Count == 0);

            if (
                AddQuery(RootQuery) != RootQueryKey ||
                AddQuery(SelfQuery) != SelfQueryKey ||
                AddQuery(PrecedingSibling) != PrecedingSiblingKey ||
                AddQuery(Descendant) != DescendantKey ||
                AddQuery(DescendantText) != DescendantTextKey
            ){
                Debug.Assert(false, "Internal Error: Predefined quiery mismatch.");
            }
        }

        internal int AddQuery(string xpathQuery) {
            return AddQuery(xpathQuery, /*allowVars:*/true, /*allowKey*/true);
        }

        internal int AddQuery(string xpathQuery, bool allowVar, bool allowKey) {
            Debug.Assert(this.queryStore != null);

            try {
                TheQuery theQuery = new TheQuery(new CompiledXpathExpr(this.queryBuilder.Build(xpathQuery, allowVar, allowKey), xpathQuery, false) , this.scopeManager);
                int      theKey   = this.queryStore.Add(theQuery);
                Debug.Assert(this.queryStore[theKey] == theQuery);
                Debug.Assert(theKey == this.queryStore.Count - 1);

                return theKey;
            }
            catch (XPathException e) {
                if (ForwardCompatibility) {
                    return InvalidQueryKey;
                }
                throw new XsltException(Res.Xslt_InvalidXPath,  new string[] { xpathQuery }, e);
                
            }
        }

        internal int AddPatternQuery(string xpathQuery, bool allowVar, bool allowKey) {
            Debug.Assert(this.queryStore != null);
            try {
                TheQuery theQuery = new TheQuery(new CompiledXpathExpr(this.queryBuilder.BuildPatternQuery(xpathQuery, allowVar, allowKey), xpathQuery, false) , this.scopeManager);
                int      theKey   = this.queryStore.Add(theQuery);
                Debug.Assert(this.queryStore[theKey] == theQuery);
                Debug.Assert(theKey == this.queryStore.Count - 1);
                return theKey;
            }
            catch (XPathException e) {
                if (ForwardCompatibility) {
                    return InvalidQueryKey;
                }
                throw new XsltException(Res.Xslt_InvalidXPath,  new string[] { xpathQuery }, e);
            }
        }

        internal int AddStringQuery(string xpathQuery) {
            string modifiedQuery = XmlCharType.IsOnlyWhitespace(xpathQuery) ? xpathQuery : "string(" + xpathQuery + ")";
            return AddQuery(modifiedQuery);
        }

        internal int AddBooleanQuery(string xpathQuery) {
            string modifiedQuery = XmlCharType.IsOnlyWhitespace(xpathQuery) ? xpathQuery : "boolean(" + xpathQuery + ")";
            return AddQuery(modifiedQuery);
        }

        //
        // Script support
        //
        Hashtable[] _typeDeclsByLang = new Hashtable[] { new Hashtable(), new Hashtable(), new Hashtable() };
        // Namespaces we always import when compiling
        private static string[] _defaultNamespaces = new string[] {
            "System",
            "System.Collections",
            "System.Text",
            "System.Text.RegularExpressions",
            "System.Xml",
            "System.Xml.Xsl",
            "System.Xml.XPath",
        };

        private static int scriptClassCounter = 0;
        private static string GenerateUniqueClassName() {
            return "ScriptClass_" + (++scriptClassCounter);
        }

        internal void AddScript(string source, ScriptingLanguage lang, string ns, string fileName, int lineNumber) {
            XsltArgumentList.ValidateExtensionNamespace(ns);
            for (ScriptingLanguage langTmp = ScriptingLanguage.JScript; langTmp <= ScriptingLanguage.CSharp; langTmp ++) {
                Hashtable typeDecls = _typeDeclsByLang[(int)langTmp];
                if (lang == langTmp) {
                    CodeTypeDeclaration scriptClass = (CodeTypeDeclaration)typeDecls[ns];
                    if (scriptClass == null) {
                        scriptClass = new CodeTypeDeclaration(GenerateUniqueClassName());
                        scriptClass.TypeAttributes = TypeAttributes.Public;
                        typeDecls.Add(ns, scriptClass);
                    }
                    CodeSnippetTypeMember scriptSnippet = new CodeSnippetTypeMember(source);
                    if (lineNumber > 0) {
                        scriptSnippet.LinePragma = new CodeLinePragma(fileName, lineNumber); 
                    }
                    scriptClass.Members.Add(scriptSnippet);
                } 
                else if (typeDecls.Contains(ns)) {
                    throw new XsltException(Res.Xslt_ScriptMixLang);
                }
            }
        }

        CodeDomProvider GetCodeDomProvider(ScriptingLanguage lang) {
            switch(lang) {
            case ScriptingLanguage.JScript:
                return  (CodeDomProvider) Activator.CreateInstance(Type.GetType("Microsoft.JScript.JScriptCodeProvider, " + AssemblyRef.MicrosoftJScript), 
                    BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null, null);
            case ScriptingLanguage.CSharp: 
            default:
                return new Microsoft.CSharp.CSharpCodeProvider();
            }
        }

        private void CompileScript() {
            for (ScriptingLanguage lang = ScriptingLanguage.JScript; lang <= ScriptingLanguage.CSharp; lang ++) {
                int idx = (int)lang;
                if(_typeDeclsByLang[idx].Count > 0) {
                    CompileAssembly(GetCodeDomProvider(lang), _typeDeclsByLang[idx], lang.ToString());
                }
            }
        }

        private void CompileAssembly(CodeDomProvider provider, Hashtable typeDecls, string nsName) {
            nsName = "Microsoft.Xslt.CompiledScripts." + nsName;
            CodeNamespace msXslt = new CodeNamespace(nsName);            

            // Add all the default namespaces
            foreach(string ns in _defaultNamespaces) {
                msXslt.Imports.Add(new CodeNamespaceImport(ns));
            }

            foreach(CodeTypeDeclaration scriptClass in typeDecls.Values) {
                msXslt.Types.Add(scriptClass);
            }

            CodeCompileUnit unit = new CodeCompileUnit(); {
                unit.Namespaces.Add(msXslt);
                // This settings have sense for Visual Basic only. 
                // We can consider in future to allow user set them in <xsl:script option="???"> attribute.
                unit.UserData["AllowLateBound"]             = true;  // Allow variables to be undeclared
                unit.UserData["RequireVariableDeclaration"] = false; // Allow variables to be declared untyped
            }
            CompilerParameters compilParams = new CompilerParameters(); {
                compilParams.GenerateInMemory = true;
                compilParams.ReferencedAssemblies.Add(typeof(XPathNavigator).Module.FullyQualifiedName);
                compilParams.ReferencedAssemblies.Add("system.dll");
//                compilParams.ReferencedAssemblies.Add("microsoft.visualbasic.dll"); This is not requied somehow.
            }
            CompilerResults results = provider.CreateCompiler().CompileAssemblyFromDom(compilParams, unit);
            if (results.Errors.HasErrors) {
                StringWriter stringWriter = new StringWriter();
                foreach (CompilerError e in results.Errors) {
                    stringWriter.WriteLine(e.ToString());
                }
                System.Diagnostics.Debug.WriteLine(stringWriter.ToString());
                throw new XsltException(Res.Xslt_ScriptCompileErrors, stringWriter.ToString());
            }
            Assembly assembly = results.CompiledAssembly;
            foreach(DictionaryEntry entry in typeDecls) {
                string ns = (string)entry.Key;
                CodeTypeDeclaration scriptClass = (CodeTypeDeclaration)entry.Value;
                this.stylesheet.ScriptObjectTypes.Add(ns, assembly.GetType(nsName + "." + scriptClass.Name));
            }
        }

        public string GetNsAlias(ref string prefix) {
            Debug.Assert(
                Keywords.Equals(this.input.LocalName, this.input.Atoms.StylesheetPrefix) || 
                Keywords.Equals(this.input.LocalName, this.input.Atoms.ResultPrefix)
            );
            if (Keywords.Compare(this.input.Atoms.HashDefault, prefix)) {
                prefix = string.Empty;
                return this.DefaultNamespace;
            }
            else {
                if(! PrefixQName.ValidatePrefix(prefix) || prefix.Length == 0) {
                    throw XsltException.InvalidAttrValue(this.input.LocalName, prefix);
                }
                return this.ResolveXPathNamespace(prefix);
            }
        }

        // AVT's compilation.
        // CompileAvt() returns ArrayList of TextEvent & AvtEvent

        private static void getTextLex(string avt, ref int start, StringBuilder lex) {
            Debug.Assert(avt.Length != start, "Empty string not supposed here");
            Debug.Assert(lex.Length == 0    , "Builder shoud to be reset here");
            int avtLength = avt.Length;
            int i;
            for (i = start; i < avtLength; i ++) {
                char ch = avt[i];
                if (ch == '{') {
                    if(i + 1 < avtLength && avt[i + 1] == '{') { // "{{"
                        i ++;                         
                    }
                    else {
                        break;
                    }
                }
                else if (ch == '}') {
                    if(i + 1 < avtLength && avt[i + 1] == '}') { // "}}"
                        i ++;                         
                    }
                    else {
                        throw new XsltException(Res.Xslt_SingleRightAvt, avt);
                    }
                }
                lex.Append(ch);
            }
            start = i;
        }

        private static void getXPathLex(string avt, ref int start, StringBuilder lex) {
            // AVT parser states
            const int InExp       = 0;      // Inside AVT expression
            const int InLiteralA  = 1;      // Inside literal in expression, apostrophe delimited
            const int InLiteralQ  = 2;      // Inside literal in expression, quote delimited
            Debug.Assert(avt.Length != start, "Empty string not supposed here");
            Debug.Assert(lex.Length == 0    , "Builder shoud to be reset here");
            Debug.Assert(avt[start] == '{'  , "We calling getXPathLex() only after we realy meet {");
            int avtLength = avt.Length;
            int state  = InExp;
            for (int i = start + 1; i < avtLength; i ++) {
                char ch = avt[i];
                switch(state) {
                case InExp :
                    switch (ch) {
                    case '{' :
                        throw new XsltException(Res.Xslt_NestedAvt, avt);
                    case '}' :
                        i ++; // include '}'
                        if(i == start + 2) { // empty XPathExpresion
                            throw new XsltException(Res.Xslt_EmptyAvtExpr, avt);
                        }
                        lex.Append(avt, start + 1, i - start - 2); // avt without {}
                        start = i;
                        return;
                    case '\'' :
                        state = InLiteralA;
                        break;
                    case '"' :
                        state = InLiteralQ;
                        break;
                    }
                    break;
                case InLiteralA :
                    if (ch == '\'') {
                        state = InExp;
                    }
                    break;
                case InLiteralQ :
                    if (ch == '"') {
                        state = InExp;
                    }
                    break;
                }
            }

            // if we meet end of string before } we have an error
            throw new XsltException(state == InExp ? Res.Xslt_OpenBracesAvt : Res.Xslt_OpenLiteralAvt, avt);
        }

        private static bool GetNextAvtLex(string avt, ref int start, StringBuilder lex, out bool isAvt) {
            Debug.Assert(start <= avt.Length);
#if DEBUG
            int saveStart = start;
#endif
            isAvt = false;
            if (start == avt.Length) {
                return false;
            }
            lex.Length = 0;
            getTextLex(avt, ref start, lex);
            if(lex.Length == 0) {
                isAvt = true;
                getXPathLex(avt, ref start, lex);
            }
#if DEBUG
            Debug.Assert(saveStart < start, "We have to read something. Otherwise it's dead loop.");
#endif
            return true;
        }

        internal ArrayList CompileAvt(string avtText, out bool constant) {
            Debug.Assert(avtText != null);
            ArrayList list = new ArrayList();

            constant = true;
            /* Parse input.Value as AVT */ {
                int  pos = 0;
                bool isAvt;
                while (GetNextAvtLex(avtText, ref pos, this.AvtStringBuilder, out isAvt)) {
                    string lex = this.AvtStringBuilder.ToString();
                    if (isAvt) {
                        list.Add(new AvtEvent(this.AddStringQuery(lex)));
                        constant = false;
                    }
                    else {
                        list.Add(new TextEvent(lex));
                    }
                }
            }
            if(constant) {
                Debug.Assert(list.Count <= 1, "We can't have more then 1 text event if now {avt} found");
            }
            return list;
        }

        internal ArrayList CompileAvt(string avtText) {
            bool constant;
            return CompileAvt(avtText, out constant);
        }

        internal static string[] SplitString(string source) {
            Debug.Assert(source != null);
            string[] array = source.Split(null);

            int realLength = 0; {
                for (int i = 0; i < array.Length; i++) {
                    if (array[i].Length != 0) {
                        realLength ++;
                    }
                }
            }
            if (realLength == array.Length) {
                return array;
            }

            string[] realArray = new string[realLength]; {
                int realPos = 0;
                for (int i = 0; i < array.Length; i++) {
                    if (array[i].Length != 0) {
                        realArray[realPos ++] = array[i];
                    }
                }
                Debug.Assert(realPos == realLength);
            }
            return realArray;
        }

        // Compiler is a class factory for some actions:
        // CompilerDbg override all this methods:

        public virtual ApplyImportsAction CreateApplyImportsAction() {
            ApplyImportsAction action = new ApplyImportsAction();
            action.Compile(this);
            return action;
        }

        public virtual ApplyTemplatesAction CreateApplyTemplatesAction() {
            ApplyTemplatesAction action = new ApplyTemplatesAction();
            action.Compile(this);
            return action;
        }

        public virtual AttributeAction CreateAttributeAction() {
            AttributeAction action = new AttributeAction();
            action.Compile(this);
            return action;
        }

        public virtual AttributeSetAction CreateAttributeSetAction() {
            AttributeSetAction action = new AttributeSetAction();
            action.Compile(this);
            return action;
        }

        public virtual CallTemplateAction CreateCallTemplateAction() {
            CallTemplateAction action = new CallTemplateAction();
            action.Compile(this);
            return action;
        }

        public virtual ChooseAction CreateChooseAction() {//!!! don't need to be here
            ChooseAction action = new ChooseAction();
            action.Compile(this);
            return action;
        }

        public virtual CommentAction CreateCommentAction() {
            CommentAction action = new CommentAction();
            action.Compile(this);
            return action;
        }

        public virtual CopyAction CreateCopyAction() {
            CopyAction action = new CopyAction();
            action.Compile(this);
            return action;
        }

        public virtual CopyOfAction CreateCopyOfAction() {
            CopyOfAction action = new CopyOfAction();
            action.Compile(this);
            return action;
        }

        public virtual ElementAction CreateElementAction() {
            ElementAction action = new ElementAction();
            action.Compile(this);
            return action;
        }

        public virtual ForEachAction CreateForEachAction() {
            ForEachAction action = new ForEachAction();
            action.Compile(this);
            return action;
        }

        public virtual IfAction CreateIfAction(IfAction.ConditionType type) {
            IfAction action = new IfAction(type);
            action.Compile(this);
            return action;
        }

        public virtual MessageAction CreateMessageAction() {
            MessageAction action = new MessageAction();
            action.Compile(this);
            return action;
        }

        public virtual NewInstructionAction CreateNewInstructionAction() {
            NewInstructionAction action = new NewInstructionAction();
            action.Compile(this);
            return action;
        }

        public virtual NumberAction CreateNumberAction() {
            NumberAction action = new NumberAction();
            action.Compile(this);
            return action;
        }

        public virtual ProcessingInstructionAction CreateProcessingInstructionAction() {
            ProcessingInstructionAction action = new ProcessingInstructionAction();
            action.Compile(this);
            return action;
        }

        public virtual void CreateRootAction() {
            this.RootAction = new RootAction();
            this.RootAction.Compile(this);
        }

        public virtual SortAction CreateSortAction() {
            SortAction action = new SortAction();
            action.Compile(this);
            return action;
        }

        public virtual TemplateAction CreateTemplateAction() {
            TemplateAction action = new TemplateAction();
            action.Compile(this);
            return action;
        }

        public virtual TemplateAction CreateSingleTemplateAction() {
            TemplateAction action = new TemplateAction();
            action.CompileSingle(this);
            return action;
        }

        public virtual TextAction CreateTextAction() {
            TextAction action = new TextAction();
            action.Compile(this);
            return action;
        }

        public virtual UseAttributeSetsAction CreateUseAttributeSetsAction() {
            UseAttributeSetsAction action = new UseAttributeSetsAction();
            action.Compile(this);
            return action;
        }

        public virtual ValueOfAction CreateValueOfAction() {
            ValueOfAction action = new ValueOfAction();
            action.Compile(this);
            return action;
        }

        public virtual VariableAction CreateVariableAction(VariableType type) {
            VariableAction action = new VariableAction(type);
            action.Compile(this);
            if ( action.VarKey != InvalidQueryKey ) {
                return action;
            }
            else {
                return null;
            }
        }

        public virtual WithParamAction CreateWithParamAction() {
            WithParamAction action = new WithParamAction();
            action.Compile(this);
            return action;
        }

        // Compiler is a class factory for some events:
        // CompilerDbg override all this methods:

        public virtual BeginEvent CreateBeginEvent() {
            return new BeginEvent(this);
        }

        public virtual TextEvent CreateTextEvent() {
            return new TextEvent(this);
        }
    }
}
