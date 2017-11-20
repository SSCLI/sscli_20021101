//------------------------------------------------------------------------------
// <copyright file="ForwardOnlyXpathExpr.cs" company="Microsoft">
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

namespace System.Xml.XPath {

    using System;
    using System.Xml;
    using System.Collections;

    internal interface ForwardOnlyXpathExpr {
        int CurrentDepth { get; }

        // this is for checking the restrictly xpath in grammer, and generate the basic tree
        // leave it commentted bcoz interface doesn't allow static function
        // static ForwardOnlyXpathExpr CompileXPath (string xPath, bool isField);
        
        //******************************* Firstly only think about selector interface ***************************
        bool MoveToStartElement (string NCName, string URN);
        void EndElement (string NCName, string URN);

        //******************************* Secondly field interface ***************************
        bool MoveToAttribute (string NCName, string URN);
        
    }

}
