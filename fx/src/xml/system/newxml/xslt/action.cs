//------------------------------------------------------------------------------
// <copyright file="Action.cs" company="Microsoft">
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
    using System.Xml;
    using System.Xml.XPath;
    using System.Xml.Xsl.Debugger;

    internal abstract class Action {
        internal const int Initialized  =  0;
        internal const int Finished     = -1;

        internal abstract void Execute(Processor processor, ActionFrame frame);

        internal virtual String Select {
            get { return null; }
        }

        internal virtual void ReplaceNamespaceAlias(Compiler compiler){}

        [System.Diagnostics.Conditional("DEBUG")]
        internal virtual void Trace(int tab) {}


        internal virtual DbgData GetDbgData(ActionFrame frame) { return DbgData.Empty; }
    }
}
