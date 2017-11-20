//------------------------------------------------------------------------------
// <copyright file="validationstate.cs" company="Microsoft">
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

namespace System.Xml.Schema {
    using System;
    using System.Collections;

    internal sealed class ValidationState {
        public bool              HasMatched;       // whether the element has been verified correctly
        public SchemaElementDecl ElementDecl;            // ElementDecl
        public bool              IsNill;
        public int               Depth;         // The validation state  
        public int               State;         // state of the content model checking
        public bool              NeedValidateChildren;  // whether need to validate the children of this element   
        public XmlQualifiedName  Name;
        public string            Prefix;
        public ContentNode       CurrentNode;
        public Hashtable         MinMaxValues;
        public int               HasNodeMatched;
        public BitSet            AllElementsSet;
        public XmlSchemaContentProcessing ProcessContents;
        public ConstraintStruct[] Constr;
    };

}
