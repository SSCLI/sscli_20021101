//------------------------------------------------------------------------------
// <copyright file="ConstraintStruct.cs" company="Microsoft">
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
    using System.Text;
    using System.Collections;
    using System.Diagnostics;
    using System.Xml.XPath;

    internal sealed class ConstraintStruct {
        // for each constraint
        internal CompiledIdentityConstraint constraint;     // pointer to constraint
        internal ArrayList  constraintTable;                // Add KeySequence each time
        internal ActiveAxis axisSelector;
        internal ArrayList  axisFields;                     // Add tableDim * LocatedActiveAxis in a loop
        internal Hashtable qualifiedTable;                  // Checking confliction
        internal ArrayList keyrefTables;                    // several keyref tables having connections to this one is possible
        private int tableDim;                               // dimension of table = numbers of fields;
        private int currentRow;                             // the current pointer for the table

        internal int TableDim {
            get { return this.tableDim; }
        }

        internal ConstraintStruct (CompiledIdentityConstraint constraint) {
            this.constraint = constraint;
            this.tableDim = constraint.Fields.Length;
            this.constraintTable = new ArrayList();         // empty table
            this.currentRow = 0;
            this.axisFields = new ArrayList();              // empty fields
            this.axisSelector = new ActiveAxis (constraint.Selector);
            this.qualifiedTable = new Hashtable();
        
        }

        internal int AddToTable (KeySequence ks) {
            this.constraintTable.Add(ks);
            return (this.currentRow ++);
        }

    } 

    // ActiveAxis plus the location plus the state of matching in the constraint table : only for field
    internal class LocatedActiveAxis : ActiveAxis {
        private int         row;                        // the row in the table (which is assigned by selector)
        private int         column;                     // the column in the table (the field sequence)
        internal bool       isMatched;                  // if it's matched, then fill value in the validator later

        internal int Row {
            get { return this.row; }
        }
        internal int Column {
            get { return this.column; }
        }

        internal LocatedActiveAxis (Asttree astfield, int row, int column) : base (astfield) {
            this.row = row;
            this.column = column;
            this.isMatched = false;
        }

    }

    internal class TypedObject {

        private class DecimalStruct {
            bool isDecimal = false;         // rare case it will be used...
            decimal[] dvalue;               // to accelerate equals operation.  array <-> list

            public bool IsDecimal {
                get { return this.isDecimal; }
                set { this.isDecimal = value; }
            }

            public int Dim {
                get { return this.dvalue.Length; }
            }

            public decimal[] Dvalue {
                get { return this.dvalue; }
                set { this.dvalue = value; }
            }
            public DecimalStruct () {
                this.dvalue = new decimal[1];
            }
            //list
            public DecimalStruct (int dim) {
                this.dvalue = new decimal[dim];
            }
        }

        DecimalStruct dstruct = null; 
        object ovalue;
        string svalue;      // only for output
        XmlSchemaDatatype xsdtype;
        int dim = 1; 
        bool isList = false;

        public int Dim {
            get { return this.dim; }
        }

        public bool IsList {
            get { return this.isList; }
        }

        public bool IsDecimal {
            get { 
                Debug.Assert (this.dstruct != null);
                return this.dstruct.IsDecimal; 
            }
        }
        public decimal[] Dvalue {
            get {
                Debug.Assert (this.dstruct != null);
                return this.dstruct.Dvalue; 
            }
        }
        
        public object Value {
            get {return ovalue; }
            set {ovalue = value; }
        }

        public XmlSchemaDatatype Type {
            get {return xsdtype; }
            set {xsdtype = value; }
        }

        public TypedObject (object obj, string svalue, XmlSchemaDatatype xsdtype) {
            this.ovalue = obj;
            this.svalue = svalue;
            this.xsdtype = xsdtype;
            if (xsdtype.Variety == XmlSchemaDatatypeVariety.List) {
                this.isList = true;
                this.dim = ((Array)obj).Length;
            }
        }

        public override string ToString() {
            // only for exception
            return this.svalue;
        }

        public void SetDecimal () {

            if (this.dstruct != null) {
                return; 
            }

            //list
            // can't use switch-case for type
            // from derived to base for safe, does it really affect?
            if (this.isList) {
                this.dstruct = new DecimalStruct(this.dim);
                // Debug.Assert(!this.IsDecimal);
                if ((xsdtype is Datatype_byte) || (xsdtype is Datatype_unsignedByte)
                	||(xsdtype is Datatype_short) ||(xsdtype is Datatype_unsignedShort)
                	||(xsdtype is Datatype_int) ||(xsdtype is Datatype_unsignedInt)
                	||(xsdtype is Datatype_long) ||(xsdtype is Datatype_unsignedLong)
                	||(xsdtype is Datatype_decimal) || (xsdtype is Datatype_integer) 
                     ||(xsdtype is Datatype_positiveInteger) || (xsdtype is Datatype_nonNegativeInteger) 
                     ||(xsdtype is Datatype_negativeInteger) ||(xsdtype is Datatype_nonPositiveInteger)) {
                    for (int i = 0; i < this.dim; i ++) {
                        this.dstruct.Dvalue[i] = Convert.ToDecimal (((Array) this.ovalue).GetValue(i));
                    }
                    this.dstruct.IsDecimal = true;
                }
            }
            else {  //not list
                this.dstruct = new DecimalStruct();
                if ((xsdtype is Datatype_byte) || (xsdtype is Datatype_unsignedByte)
                	||(xsdtype is Datatype_short) ||(xsdtype is Datatype_unsignedShort)
                	||(xsdtype is Datatype_int) ||(xsdtype is Datatype_unsignedInt)
                	||(xsdtype is Datatype_long) ||(xsdtype is Datatype_unsignedLong)
                	||(xsdtype is Datatype_decimal) || (xsdtype is Datatype_integer) 
                     ||(xsdtype is Datatype_positiveInteger) || (xsdtype is Datatype_nonNegativeInteger) 
                     ||(xsdtype is Datatype_negativeInteger) ||(xsdtype is Datatype_nonPositiveInteger)) {
                   //possibility of list of length 1.
                    this.dstruct.Dvalue[0] = Convert.ToDecimal (this.ovalue);
                    this.dstruct.IsDecimal = true;
                }
            }
        }

        private bool ListDValueEquals (TypedObject other) {
            for (int i = 0; i < this.Dim; i ++) {
                if (this.Dvalue[i] != other.Dvalue[i]) {
                    return false;
                }                
            }
            return true;
        }

        public bool Equals (TypedObject other) {
            // ? one is list with one member, another is not list -- still might be equal
            if (this.Dim != other.Dim) {
                return false;
            }

            if (this.Type != other.Type) {              
                bool thisfromother = this.Type.IsDerivedFrom (other.Type); 
                bool otherfromthis = other.Type.IsDerivedFrom (this.Type);

                if (! (thisfromother || otherfromthis)) {       // second normal case
                    return false;
                }

                if (thisfromother) {
                    // can't use cast and other.Type.IsEqual (value1, value2)
                    other.SetDecimal();
                    if (other.IsDecimal) {
                        this.SetDecimal();
			   return this.ListDValueEquals(other);                    
		      }
                    // deal else (not decimal) in the end 
                }
                else {
                    this.SetDecimal();
                    if (this.IsDecimal) {
                        other.SetDecimal();
                        return this.ListDValueEquals(other);
                    }
                    // deal else (not decimal) in the end 
                }
            }

            // not-Decimal derivation or type equal
            if ((! this.IsList) && (! other.IsList)) {      // normal case
                return this.Value.Equals (other.Value);
            }
            else if ((this.IsList) && (other.IsList)){      // second normal case
                for (int i = 0; i < this.Dim; i ++) {
                    if (! ((Array)this.Value).GetValue(i).Equals(((Array)other.Value).GetValue(i))) {
                        return false;
                    }
                }
                return true;
            }
            else if (((this.IsList) && (((Array)this.Value).GetValue(0).Equals(other.Value)))
                || ((other.IsList) && (((Array)other.Value).GetValue(0).Equals(this.Value)))) { // any possibility?
                return true;
            }

            return false;
        }
    }

    internal class KeySequence {
        TypedObject[] ks;
        int dim;
        int posline, poscol;            // for error reporting

        internal KeySequence (int dim, int line, int col) {
            Debug.Assert(dim > 0);
            this.dim = dim;
            this.ks = new TypedObject[dim];
            this.posline = line;
            this.poscol = col;
        }

        public int PosLine {
            get { return this.posline; }
        }

        public int PosCol {
            get { return this.poscol; }
        }

        public KeySequence(TypedObject[] ks) {
            this.ks = ks;
            this.dim = ks.Length;
            this.posline = this.poscol = 0;
        }

        public object this[int index] {
            get {
                object result = ks[index];
                return result;
            }
            set {
                ks[index] = (TypedObject) value;
            } 
        }

        // return true if no null field
        internal bool IsQualified() {
            foreach (TypedObject tobj in this.ks) {
                if ((tobj == null) || (tobj.Value == null)) return false;
            }
            return true;
        }

        public override int GetHashCode() {
            int hashcode = 0;
            for (int i = 0; i < this.ks.Length; i ++) {
                hashcode += this.ks[i].GetHashCode();
            }
            return hashcode;
        }

        // considering about derived type
        public override bool Equals(object other) {
            // each key sequence member can have different type
            KeySequence keySequence = (KeySequence)other;
            for (int i = 0; i < this.ks.Length; i ++) {
                if (! this.ks[i].Equals (keySequence.ks[i])) {
                    return false;
                }
            }
            return true;
        }

        public override string ToString() {
            StringBuilder sb = new StringBuilder();
            sb.Append(this.ks[0].ToString());
            for (int i = 1; i < this.ks.Length; i ++) {
                sb.Append(" ");
                sb.Append(this.ks[i].ToString());
            }
            return sb.ToString();
        }
    }

}
