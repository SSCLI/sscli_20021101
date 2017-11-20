//------------------------------------------------------------------------------
// <copyright file="VariableQuery.cs" company="Microsoft">
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
    using System.Xml.Xsl;

    using System.Collections;

    internal  class VariableQuery : IQuery {
        private String Name;
        private String Prefix;
        private IQuery _VarQuery;
        private IXsltContextVariable xsltContextVariable;
        
        internal VariableQuery(string name, string prefix) {
            this.Name = name;
            this.Prefix = prefix;
        }

        private VariableQuery( string name, string prefix, IQuery varQuery) {
            this.Name = name;
            this.Prefix = prefix;
            this._VarQuery = varQuery;
        }
        
        internal override void SetXsltContext(XsltContext context) {
            if (context == null) {
                throw new XPathException(Res.Xp_UndefinedXsltContext, Prefix, Name);            }

            if(xsltContextVariable == null) {
                xsltContextVariable = context.ResolveVariable(Prefix, Name);
                Debug.Assert(xsltContextVariable != null, "XSLT has to resolve it or throw exception");
            }
            else {
                Debug.Assert(xsltContextVariable == context.ResolveVariable(Prefix, Name), "ResolveVariable supposed to return the same VariableAction each time");
            }
            Object result = xsltContextVariable.Evaluate(context);
            if (result is double) {
                _VarQuery = new OperandQuery(result, XPathResultType.Number);
            }
            else if (result is String) {
                _VarQuery = new OperandQuery(result, XPathResultType.String);
            }
            else if (result is Boolean) {
                _VarQuery = new OperandQuery(result, XPathResultType.Boolean);
            }
            else if (result is XPathNavigator) {
                _VarQuery = new NavigatorQuery((XPathNavigator)result);
            }            
            else if (result is ResetableIterator) {
                ResetableIterator it = result as ResetableIterator;
                _VarQuery = new XmlIteratorQuery( (ResetableIterator) it.Clone() );
            }
            else if (result is XPathNodeIterator) {
                Debug.Assert(false, "Unexpected type of XPathNodeIterator");
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedType, result.GetType().FullName));             
            }
            else if (result is int) {
                _VarQuery = new OperandQuery(XmlConvert.ToXPathDouble(result), XPathResultType.Number);
            }
            else {
                if (result == null) {
                    throw new XPathException(Res.Xslt_InvalidVariable, Name);
                }
                Debug.Assert(false, "Unexpected variable type");
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedType, result.GetType().FullName));
            }
            Debug.Assert(_VarQuery != null);           
        }
        
        internal override XPathNavigator advance() {
            if (_VarQuery != null )
                return _VarQuery.advance();
            throw new XPathException(Res.Xp_UndefinedXsltContext, Prefix, Name);
        } // Advance

        internal override Object getValue(IQuery qy) {
            if (_VarQuery != null )
                return _VarQuery.getValue(qy);
            throw new XPathException(Res.Xp_UndefinedXsltContext, Prefix, Name);
        } 

        internal override Object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            if (_VarQuery != null )
                return _VarQuery.getValue(qy, iterator);
            return null;
        } 
        
        internal override IQuery Clone() {            if (_VarQuery == null) {
                return new VariableQuery(Name, Prefix);
            }
            else {
                return new VariableQuery(Name, Prefix, _VarQuery.Clone());
            }
        }
        
        internal override XPathResultType  ReturnType() {
            if (_VarQuery != null)
                return _VarQuery.ReturnType();            throw new XPathException(Res.Xp_UndefinedXsltContext, Prefix, Name);
        }
        
        internal override Querytype getName() {
            if (_VarQuery != null)
                return _VarQuery.getName();
            return Querytype.None;
        }

        internal override void reset() {
            if (_VarQuery != null )
                _VarQuery.reset();
        } 

        internal override XPathNavigator peekElement() {
            if (_VarQuery != null )
                return _VarQuery.peekElement();
            throw new XPathException(Res.Xp_UndefinedXsltContext, Prefix, Name);
        }

        internal override int Position {
            get {
                Debug.Assert(_VarQuery != null , " Result Query is null in Position");
                return _VarQuery.Position; 
            } 
        }
    }
}
