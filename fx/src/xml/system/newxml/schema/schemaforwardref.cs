//------------------------------------------------------------------------------
// <copyright file="schemaforwardref.cs" company="Microsoft">
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
    using System.Diagnostics;

    internal sealed class ForwardRef {
        public enum Type {
            ID,
            NOTATION
        };

        internal String             Name;
        internal String             Prefix;
        internal String             ID;
        internal int                Line;
        internal int                Col;
        internal bool               Implied;
        internal ForwardRef.Type    RefType;
        internal ForwardRef         Next;


        internal ForwardRef(ForwardRef next, String name, String prefix, String id, int line, int col, bool implied, ForwardRef.Type type) {
            Next = next;
            Name = name;
            Prefix = prefix;
            ID = id;
            Line = line;
            Col = col;
            Implied = implied;
            RefType = type;
        }

        internal void Check(SchemaInfo sinfo, Validator validator, ValidationEventHandler eventhandler) {
            Object o = null;
            string code = null;

            switch (RefType) {
            case ForwardRef.Type.ID:
                o = validator.FindID(ID);
                code = Res.Sch_UndeclaredId;
                break;
            case ForwardRef.Type.NOTATION:
                o = sinfo.Notations[ID];
                code = Res.Sch_UndeclaredNotation;
                break;
            }

            if (o == null) {
                string baseuri = (null != validator && null != validator.BaseUri ? validator.BaseUri.AbsolutePath : string.Empty);
                eventhandler(this, new ValidationEventArgs(new XmlSchemaException(code, ID, baseuri, Line, Col)));
            }
        }
    };

}
