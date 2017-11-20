//------------------------------------------------------------------------------
// <copyright file="OperandQuery.cs" company="Microsoft">
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
    using System.Xml; 

    using System.Collections;

    internal sealed class OperandQuery : IQuery {
        private object _Var;
        private XPathResultType _Type;

        internal override Querytype getName() {
            return Querytype.Constant;
        }

        internal override object getValue(IQuery qy) {
            return _Var;
        }
        internal override object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            return _Var;
        }
        internal override XPathResultType ReturnType() {
            return _Type;
        }
        internal OperandQuery(object var,XPathResultType type) {
            _Var = var;
            _Type = type;
        }

        internal override IQuery Clone() {
            return new OperandQuery(_Var,_Type);
        }


    }
}
