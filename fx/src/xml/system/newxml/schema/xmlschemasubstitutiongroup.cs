//------------------------------------------------------------------------------
// <copyright file="XmlSchemaSubstitutionGroup.cs" company="Microsoft">
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

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    internal class XmlSchemaSubstitutionGroup : XmlSchemaObject {
        Hashtable members = new Hashtable();
        XmlQualifiedName examplar = XmlQualifiedName.Empty;
        XmlSchemaChoice choice = new XmlSchemaChoice();
        bool validating = false;

        [XmlIgnore]
        internal Hashtable Members {
            get { return members; }
        } 

        [XmlIgnore]
        internal XmlQualifiedName Examplar {
            get { return examplar; }
            set { examplar = value; }
        }
        
        [XmlIgnore]
        internal XmlSchemaChoice Choice {
            get { return choice; }
            set { choice = value; }
        }          

        [XmlIgnore]
        internal bool Validating {
            get { return validating; }
            set { validating = value; }
        } 
    }
}
