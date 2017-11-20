//------------------------------------------------------------------------------
// <copyright file="Types.cs" company="Microsoft">
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

namespace System.Xml.Serialization {

    using System;
    using System.Reflection;
    using System.Collections;
    using System.Xml.Schema;
    using System.Xml;
    using System.IO;
    using System.ComponentModel;

    // These classes provide a higher level view on reflection specific to 
    // Xml serialization, for example:
    // - allowing one to talk about types w/o having them compiled yet
    // - abstracting collections & arrays
    // - abstracting classes, structs, interfaces
    // - knowing about XSD primitives
    // - dealing with Serializable and xmlelement
    // and lots of other little details

    internal enum TypeKind {
        Root,
        Primitive,
        Enum,
        Struct,
        Class,
        Interface,
        Array,
        Collection,
        Enumerable,
        Void,
        Node,
        Attribute,
        Serializable
    }

    internal enum TypeFlags {
        Abstract = 0x1,
        Reference = 0x2,
        Special = 0x4,
        CanBeAttributeValue = 0x8,
        CanBeTextValue = 0x10,
        CanBeElementValue = 0x20,
        HasCustomFormatter = 0x40,
        AmbiguousDataType = 0x80,
        IgnoreDefault = 0x200,
        HasIsEmpty = 0x400,
        HasDefaultConstructor = 0x800,
        XmlEncodingNotRequired = 0x1000,
    }

    internal class TypeDesc {
        string name;
        string fullName;
        TypeDesc arrayElementTypeDesc;
        TypeDesc arrayTypeDesc;
        TypeKind kind;
        XmlSchemaSimpleType dataType;
        TypeDesc baseTypeDesc;
        TypeFlags flags;
        string formatterName;
        bool isXsdType;
        bool isMixed;
        
        public TypeDesc(string name, string fullName, TypeKind kind, TypeDesc baseTypeDesc, TypeFlags flags) {
            this.name = name.Replace('+', '.');
            this.fullName = fullName.Replace('+', '.');
            this.kind = kind;
            if (baseTypeDesc != null) {
                this.baseTypeDesc = baseTypeDesc;
            }
            this.flags = flags;
            this.isXsdType = kind == TypeKind.Primitive;
        }
        
        public TypeDesc(string name, string fullName, bool isXsdType, XmlSchemaSimpleType dataType, string formatterName, TypeFlags flags) : this(name, fullName, TypeKind.Primitive, (TypeDesc)null, flags) {
            this.dataType = dataType;
            this.formatterName = formatterName;
            this.isXsdType = isXsdType;
        }
        
        public TypeDesc(string name, string fullName, TypeKind kind, TypeDesc baseTypeDesc, TypeFlags flags, TypeDesc arrayElementTypeDesc) : this(name, fullName, kind, baseTypeDesc, flags) {
            this.arrayElementTypeDesc = arrayElementTypeDesc;
        }

        public override string ToString() {
            return fullName;
        }
        
        public bool IsXsdType {
            get { return isXsdType; }
        }

        public string Name { 
            get { return name; }
        }
        
        public string FullName {
            get { return fullName; }
        }
        
        public XmlSchemaSimpleType DataType {
            get { return dataType; }
        }

        public string FormatterName {    
            get { return formatterName; }
        }
        
        public TypeKind Kind {
            get { return kind; }
        }

        public bool IsValueType {
            get { return (flags & TypeFlags.Reference) == 0; }
        }

        public bool CanBeAttributeValue {
            get { return (flags & TypeFlags.CanBeAttributeValue) != 0; }
        }

        public bool XmlEncodingNotRequired {
            get { return (flags & TypeFlags.XmlEncodingNotRequired) != 0; }
        }
        
        public bool CanBeElementValue {
            get { return (flags & TypeFlags.CanBeElementValue) != 0; }
        }

        public bool CanBeTextValue {
            get { return (flags & TypeFlags.CanBeTextValue) != 0; }
        }

        public bool IsMixed {
            get { return isMixed || CanBeTextValue; }
            set { isMixed = value; }
        }

        public bool IsSpecial {
            get { return (flags & TypeFlags.Special) != 0; }
        }

        public bool IsAmbiguousDataType {
            get { return (flags & TypeFlags.AmbiguousDataType) != 0; }
        }

        public bool HasCustomFormatter {
            get { return (flags & TypeFlags.HasCustomFormatter) != 0; }
        }

        public bool HasDefaultSupport {
            get { return (flags & TypeFlags.IgnoreDefault) == 0; }
        }

        public bool HasIsEmpty {
            get { return (flags & TypeFlags.HasIsEmpty) != 0; }
        }

        public bool HasDefaultConstructor {
            get { return (flags & TypeFlags.HasDefaultConstructor) != 0; }
        }

        public bool IsAbstract {
            get { return (flags & TypeFlags.Abstract) != 0; }
        }

        public bool IsVoid {
            get { return kind == TypeKind.Void; }
        }
        
        public bool IsStruct {
            get { return kind == TypeKind.Struct; }
        }
        
        public bool IsClass {
            get { return kind == TypeKind.Class; }
        }

        public bool IsInterface {
            get { return kind == TypeKind.Interface; }
        }
        
        public bool IsStructLike {
            get { return kind == TypeKind.Struct || kind == TypeKind.Class || kind == TypeKind.Interface; }
        }
        
        public bool IsArrayLike {
            get { return kind == TypeKind.Array || kind == TypeKind.Collection || kind == TypeKind.Enumerable; }
        }
        
        public bool IsCollection {
            get { return kind == TypeKind.Collection; }
        }
        
        public bool IsEnumerable {
            get { return kind == TypeKind.Enumerable; }
        }

        public bool IsArray {
            get { return kind == TypeKind.Array; }
        }
        
        public bool IsPrimitive {
            get { return kind == TypeKind.Primitive; }
        }
        
        public bool IsEnum {
            get { return kind == TypeKind.Enum; }
        }

        public bool IsNullable {
            get { return !IsValueType; }
        }

        public bool IsRoot {
            get { return kind == TypeKind.Root; }
        }
        
        public string ArrayLengthName {
            get { return kind == TypeKind.Array ? "Length" : "Count"; }
        }
        
        public TypeDesc ArrayElementTypeDesc {
            get { return arrayElementTypeDesc; }
        }

        public TypeDesc CreateArrayTypeDesc() {
            if (arrayTypeDesc == null)
                arrayTypeDesc = new TypeDesc(name + "[]", fullName + "[]", TypeKind.Array, null, TypeFlags.Reference, this);
            return arrayTypeDesc;
        }

        public TypeDesc BaseTypeDesc {
            get { return baseTypeDesc; }
            set { baseTypeDesc = value; }
        }

        public int GetDerivationLevels() {
            int count = 0;
            TypeDesc typeDesc = this;
            while (typeDesc != null) {
                count++;
                typeDesc = typeDesc.BaseTypeDesc;
            }
            return count;
        }

        public bool IsDerivedFrom(TypeDesc baseTypeDesc) {
            TypeDesc typeDesc = this;
            while (typeDesc != null) {
                if (typeDesc == baseTypeDesc) return true;
                typeDesc = typeDesc.BaseTypeDesc;
            }
            return baseTypeDesc.IsRoot;
        }

        public static TypeDesc FindCommonBaseTypeDesc(TypeDesc[] typeDescs) {
            if (typeDescs.Length == 0) return null;
            TypeDesc leastDerivedTypeDesc = null;
            int leastDerivedLevel = int.MaxValue;
            for (int i = 0; i < typeDescs.Length; i++) {
                int derivationLevel = typeDescs[i].GetDerivationLevels();
                if (derivationLevel < leastDerivedLevel) {
                    leastDerivedLevel = derivationLevel;
                    leastDerivedTypeDesc = typeDescs[i];
                }
            }
            while (leastDerivedTypeDesc != null) {
                int i;
                for (i = 0; i < typeDescs.Length; i++) {
                    if (!typeDescs[i].IsDerivedFrom(leastDerivedTypeDesc)) break;
                }
                if (i == typeDescs.Length) break;
                leastDerivedTypeDesc = leastDerivedTypeDesc.BaseTypeDesc;
            }
            return leastDerivedTypeDesc;
        }
    }
   
    internal class TypeScope {
        Hashtable typeDescs = new Hashtable();
        Hashtable arrayTypeDescs = new Hashtable();
        ArrayList typeMappings = new ArrayList();

        static Hashtable primitiveTypes = new Hashtable();
        static Hashtable primitiveDataTypes = new Hashtable();
        static Hashtable primitiveNames = new Hashtable();
        static string[] unsupportedTypes = new string[] {
            "anyURI",
            "duration",
            "ENTITY",
            "ENTITIES",
            "gDay",
            "gMonth",
            "gMonthDay",
            "gYear",
            "gYearMonth",
            "ID",
            "IDREF",
            "IDREFS",
            "integer",
            "language",
            "negativeInteger",
            "nonNegativeInteger",
            "nonPositiveInteger",
            "normalizedString",
            "NOTATION",
            "positiveInteger",
            "token"
        };

        static TypeScope() {
            AddPrimitive(typeof(string), "string", "String", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.CanBeTextValue | TypeFlags.Reference);
            AddPrimitive(typeof(int), "int", "Int32", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(bool), "boolean", "Boolean", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(short), "short", "Int16", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(long), "long", "Int64", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(float), "float", "Single", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(double), "double", "Double", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(decimal), "decimal", "Decimal", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(DateTime), "dateTime", "DateTime", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(XmlQualifiedName), "QName", "XmlQualifiedName", TypeFlags.CanBeAttributeValue | TypeFlags.HasCustomFormatter | TypeFlags.HasIsEmpty | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(byte), "unsignedByte", "Byte", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(SByte), "byte", "SByte", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(UInt16), "unsignedShort", "UInt16", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(UInt32), "unsignedInt", "UInt32", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(UInt64), "unsignedLong", "UInt64", TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);

            // Types without direct mapping (ambigous)
            AddPrimitive(typeof(DateTime), "date", "Date", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(DateTime), "time", "Time", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);

            AddPrimitive(typeof(string), "Name", "XmlName", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddPrimitive(typeof(string), "NCName", "XmlNCName", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddPrimitive(typeof(string), "NMTOKEN", "XmlNmToken", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddPrimitive(typeof(string), "NMTOKENS", "XmlNmTokens", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);

            AddPrimitive(typeof(byte[]), "base64Binary", "ByteArrayBase64", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference | TypeFlags.IgnoreDefault | TypeFlags.XmlEncodingNotRequired);
            AddPrimitive(typeof(byte[]), "hexBinary", "ByteArrayHex", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference  | TypeFlags.IgnoreDefault | TypeFlags.XmlEncodingNotRequired);

            XmlSchemaPatternFacet guidPattern = new XmlSchemaPatternFacet();
            guidPattern.Value = "[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}";

            AddNonXsdPrimitive(typeof(Guid), "guid", UrtTypes.Namespace, "Guid", new XmlQualifiedName("string", XmlSchema.Namespace), new XmlSchemaFacet[] { guidPattern }, TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddNonXsdPrimitive(typeof(char), "char", UrtTypes.Namespace, "Char", new XmlQualifiedName("unsignedShort", XmlSchema.Namespace), new XmlSchemaFacet[0], TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter);

            AddSoapEncodedTypes(Soap.Encoding);

            // Unsuppoted types that we map to string, if in the future we decide 
            // to add support for them we would need to create custom formatters for them
            for (int i = 0; i < unsupportedTypes.Length; i++) {
                AddPrimitive(typeof(string), unsupportedTypes[i], "String", TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.CanBeTextValue | TypeFlags.Reference);
            }
        }

        static void AddSoapEncodedTypes(string ns) {
            for (int i = 0; i < unsupportedTypes.Length; i++) {
                AddSoapEncodedPrimitive(typeof(string), unsupportedTypes[i], ns, "String", new XmlQualifiedName(unsupportedTypes[i], XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.Reference);
            }

            AddSoapEncodedPrimitive(typeof(string), "string", ns, "String", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.CanBeTextValue | TypeFlags.Reference);
            AddSoapEncodedPrimitive(typeof(int), "int", ns, "Int32", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(bool), "boolean", ns, "Boolean", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(short), "short", ns, "Int16", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(long), "long", ns, "Int64", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(float), "float", ns, "Single", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(double), "double", ns, "Double", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(decimal), "decimal", ns, "Decimal", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(DateTime), "dateTime", ns, "DateTime", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(XmlQualifiedName), "QName", ns, "XmlQualifiedName", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.HasCustomFormatter | TypeFlags.HasIsEmpty | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(byte), "unsignedByte", ns, "Byte", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(SByte), "byte", ns, "SByte", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(UInt16), "unsignedShort", ns, "UInt16", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(UInt32), "unsignedInt", ns, "UInt32", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(UInt64), "unsignedLong", ns, "UInt64", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.XmlEncodingNotRequired);

            // Types without direct mapping (ambigous)
            AddSoapEncodedPrimitive(typeof(DateTime), "date", ns, "Date", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(DateTime), "time", ns, "Time", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.XmlEncodingNotRequired);

            AddSoapEncodedPrimitive(typeof(string), "Name", ns, "XmlName", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddSoapEncodedPrimitive(typeof(string), "NCName", ns, "XmlNCName", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddSoapEncodedPrimitive(typeof(string), "NMTOKEN", ns, "XmlNmToken", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);
            AddSoapEncodedPrimitive(typeof(string), "NMTOKENS", ns, "XmlNmTokens", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference);

            AddSoapEncodedPrimitive(typeof(byte[]), "base64Binary", ns, "ByteArrayBase64", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference | TypeFlags.IgnoreDefault | TypeFlags.XmlEncodingNotRequired);
            AddSoapEncodedPrimitive(typeof(byte[]), "hexBinary", ns, "ByteArrayHex", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.AmbiguousDataType | TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue | TypeFlags.HasCustomFormatter | TypeFlags.Reference  | TypeFlags.IgnoreDefault | TypeFlags.XmlEncodingNotRequired);

            AddSoapEncodedPrimitive(typeof(string), "arrayCoordinate", ns, "String", new XmlQualifiedName("string", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue);
            AddSoapEncodedPrimitive(typeof(byte[]), "base64", ns, "ByteArrayBase64", new XmlQualifiedName("base64Binary", XmlSchema.Namespace), TypeFlags.CanBeAttributeValue | TypeFlags.CanBeElementValue);
        }

        static void AddPrimitive(Type type, string dataTypeName, string formatterName, TypeFlags flags) {
            XmlSchemaSimpleType dataType = new XmlSchemaSimpleType();
            dataType.Name = dataTypeName;
            TypeDesc typeDesc = new TypeDesc(type.Name, type.FullName, true, dataType, formatterName, flags);
            if (primitiveTypes[type] == null)
                primitiveTypes.Add(type, typeDesc);
            primitiveDataTypes.Add(dataType, typeDesc);
            primitiveNames.Add((new XmlQualifiedName(dataTypeName, XmlSchema.Namespace)).ToString(), typeDesc);
        }

        static void AddNonXsdPrimitive(Type type, string dataTypeName, string ns, string formatterName, XmlQualifiedName baseTypeName, XmlSchemaFacet[] facets, TypeFlags flags) {
            XmlSchemaSimpleType dataType = new XmlSchemaSimpleType();
            dataType.Name = dataTypeName;
            XmlSchemaSimpleTypeRestriction restriction = new XmlSchemaSimpleTypeRestriction();
            restriction.BaseTypeName = baseTypeName;
            foreach (XmlSchemaFacet facet in facets) {
                restriction.Facets.Add(facet);
            }
            dataType.Content = restriction;
            TypeDesc typeDesc = new TypeDesc(type.Name, type.FullName, false, dataType, formatterName, flags);
            if (primitiveTypes[type] == null)
                primitiveTypes.Add(type, typeDesc);
            primitiveDataTypes.Add(dataType, typeDesc);
            primitiveNames.Add((new XmlQualifiedName(dataTypeName, ns)).ToString(), typeDesc);
        }

        static void AddSoapEncodedPrimitive(Type type, string dataTypeName, string ns, string formatterName, XmlQualifiedName baseTypeName, TypeFlags flags) {
            AddNonXsdPrimitive(type, dataTypeName, ns, formatterName, baseTypeName, new XmlSchemaFacet[0], flags);
        }

        public TypeDesc GetTypeDesc(XmlQualifiedName name) {
            return (TypeDesc)primitiveNames[name.ToString()];
        }
        
        public TypeDesc GetTypeDesc(XmlSchemaSimpleType dataType) {
            return (TypeDesc)primitiveDataTypes[dataType];
        }
        
        public TypeDesc GetTypeDesc(Type type) {
            return GetTypeDesc(type, null, true);
        }

        public TypeDesc GetTypeDesc(Type type, MemberInfo source) {
            return GetTypeDesc(type, source, true);
        }

        TypeDesc GetTypeDesc(Type type, MemberInfo source, bool throwOnDefaultCtor) {
            TypeDesc typeDesc = (TypeDesc)typeDescs[type];
            if (typeDesc == null) {
                typeDesc = ImportTypeDesc(type, true, throwOnDefaultCtor, source);
                typeDescs.Add(type, typeDesc);
            }
            else {
                if (throwOnDefaultCtor && typeDesc.IsClass && !typeDesc.IsAbstract && !typeDesc.HasDefaultConstructor) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlConstructorInaccessible, typeDesc.FullName));
                }
            }
            return typeDesc;
        }

        public TypeDesc GetArrayTypeDesc(Type type) {
            TypeDesc typeDesc = (TypeDesc) arrayTypeDescs[type];
            if (typeDesc == null) {
                typeDesc = GetTypeDesc(type);
                if (!typeDesc.IsArrayLike)
                    typeDesc = ImportTypeDesc(type, false, true, null);
                arrayTypeDescs.Add(type, typeDesc);
            }
            return typeDesc;
        }

        TypeDesc ImportTypeDesc(Type type, bool canBePrimitive, bool throwOnNoDefaultCtor, MemberInfo memberInfo) {
            TypeDesc typeDesc = null;
            if (canBePrimitive) {
                typeDesc = (TypeDesc)primitiveTypes[type];
                if (typeDesc != null) return typeDesc;
            }
            TypeKind kind;
            Type arrayElementType = null;
            Type baseType = null;
            TypeFlags flags = 0;
            if (!type.IsValueType)
                flags |= TypeFlags.Reference;
            if (type.IsNotPublic) {
                throw new InvalidOperationException(Res.GetString(Res.XmlTypeInaccessible, type.FullName));
            }
            else if (type == typeof(object)) {
                kind = TypeKind.Root;
                flags |= TypeFlags.HasDefaultConstructor;
            }
            else if (type == typeof(ValueType)) {
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedType, type.FullName));
            }
            else if (type == typeof(void)) {
                kind = TypeKind.Void;
            }
            else if (typeof(IXmlSerializable).IsAssignableFrom(type)) {
                kind = TypeKind.Serializable;
                flags |= TypeFlags.Special | TypeFlags.CanBeElementValue;
            }
            else if (type.IsArray) {
                if (type.GetArrayRank() > 1) {
                    throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedRank, type.FullName));
                }
                kind = TypeKind.Array;
                arrayElementType = type.GetElementType();
                flags |= TypeFlags.HasDefaultConstructor;
            }
            else if (typeof(ICollection).IsAssignableFrom(type)) {
                arrayElementType = GetCollectionElementType(type);
                kind = TypeKind.Collection;
                if (type.GetConstructor(new Type[0]) != null) {
                    flags |= TypeFlags.HasDefaultConstructor;
                }
            }
            else if (type == typeof(XmlQualifiedName)) {
                kind = TypeKind.Primitive;
            }
            else if (type.IsPrimitive) {
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedType, type.FullName));
            }
            else if (type.IsEnum) {
                kind = TypeKind.Enum;
            }
            else if (type.IsValueType) {
                kind = TypeKind.Struct;
                baseType = type.BaseType;
                if (type.IsAbstract) flags |= TypeFlags.Abstract;
            }
            else if (type.IsClass) {
                if (type == typeof(XmlAttribute)) {
                    kind = TypeKind.Attribute;
                    flags |= TypeFlags.Special | TypeFlags.CanBeAttributeValue;
                }
                else if (typeof(XmlNode).IsAssignableFrom(type)) {
                    kind = TypeKind.Node;
                    flags |= TypeFlags.Special | TypeFlags.CanBeElementValue | TypeFlags.CanBeTextValue;
                    if (typeof(XmlText).IsAssignableFrom(type))
                        flags &= ~TypeFlags.CanBeElementValue;
                    else if (typeof(XmlElement).IsAssignableFrom(type))
                        flags &= ~TypeFlags.CanBeTextValue;
                    else if (type.IsAssignableFrom(typeof(XmlAttribute)))
                        flags |= TypeFlags.CanBeAttributeValue;

                    //type = typeof(XmlNode);
                }
                else {
                    kind = TypeKind.Class;
                    baseType = type.BaseType;
                    if (type.IsAbstract)
                        flags |= TypeFlags.Abstract;
                }
            }
            else if (type.IsInterface) {
                if (memberInfo == null) {
                    throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedInterface, type.FullName));
                }
                else {
                    throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedInterfaceDetails, memberInfo.DeclaringType.FullName + "." + memberInfo.Name, type.FullName));
                }
            }
            else {
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedType, type.FullName));
            }

            // check to see if the type has public default constructor for classes
            if (kind == TypeKind.Class && !type.IsAbstract) {
                ConstructorInfo ctor = type.GetConstructor(new Type[0]);
                if (ctor != null) {
                    if ((ctor.Attributes & MethodAttributes.HasSecurity) != 0 || (type.Attributes & TypeAttributes.HasSecurity) != 0)
                        throw new InvalidOperationException(Res.GetString(Res.XmlConstructorHasSecurityAttributes, type.FullName));
                    flags |= TypeFlags.HasDefaultConstructor;
                }
                else if (throwOnNoDefaultCtor) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlConstructorInaccessible, type.FullName));
                }
            }

            // check if a struct-like type is enumerable

            if (kind == TypeKind.Struct || kind == TypeKind.Class || kind == TypeKind.Interface){
                if (typeof(IEnumerable).IsAssignableFrom(type)) {
                    kind = TypeKind.Enumerable;
                    arrayElementType = GetEnumeratorElementType(type);

                    if (type.GetConstructor(new Type[0]) != null) {
                        flags |= TypeFlags.HasDefaultConstructor;
                    }
                }
            }

            TypeDesc arrayElementTypeDesc;
            if (arrayElementType != null) {
                arrayElementTypeDesc = GetTypeDesc(arrayElementType, null, throwOnNoDefaultCtor);
            }
            else
                arrayElementTypeDesc = null;
            TypeDesc baseTypeDesc;
            if (baseType != null && baseType != typeof(object) && baseType != typeof(ValueType))
                baseTypeDesc = GetTypeDesc(baseType, null, throwOnNoDefaultCtor);
            else
                baseTypeDesc = null;

            string name = type.Name;
            string fullName = type.FullName;
            if (type.IsNestedPublic) {
                for (Type t = type.DeclaringType; t != null; t = t.DeclaringType)
                    GetTypeDesc(t, null, false);
                int plus = name.LastIndexOf('+');
                if (plus >= 0) {
                    name = name.Substring(plus + 1);
                    fullName = fullName.Replace('+', '.');
                }
            }

            return new TypeDesc(name, fullName, kind, baseTypeDesc, flags, arrayElementTypeDesc);
        }
        
        public static Type GetArrayElementType(Type type) {
            if (type.IsArray)
                return type.GetElementType();
            else if (typeof(ICollection).IsAssignableFrom(type))
                return GetCollectionElementType(type);
            else if (typeof(IEnumerable).IsAssignableFrom(type))
                return GetEnumeratorElementType(type);
            else
                return null;
        }

        public static MemberMapping[] GetAllMembers(StructMapping mapping) {
            if (mapping.BaseMapping == null)
                return mapping.Members;
            ArrayList list = new ArrayList();
            GetAllMembers(mapping, list);
            return (MemberMapping[])list.ToArray(typeof(MemberMapping));
        }
       
        public static void GetAllMembers(StructMapping mapping, ArrayList list) {
            if (mapping.BaseMapping != null) {
                GetAllMembers(mapping.BaseMapping, list);
            }
            for (int i = 0; i < mapping.Members.Length; i++) {
                list.Add(mapping.Members[i]);
            }
        }
       
        static Type GetEnumeratorElementType(Type type) {
            if (typeof(IEnumerable).IsAssignableFrom(type)) {
                MethodInfo e = type.GetMethod("GetEnumerator", new Type[0]);
                XmlAttributes methodAttrs = new XmlAttributes(e);
                if (methodAttrs.XmlIgnore) return null;

                PropertyInfo p = e.ReturnType.GetProperty("Current");
                Type currentType = (p == null ? typeof(object) : p.PropertyType);

                MethodInfo addMethod = type.GetMethod("Add", new Type[] { currentType });
                if (addMethod == null && currentType != typeof(object)) {
                    currentType = typeof(object);
                    addMethod = type.GetMethod("Add", new Type[] { currentType });
                }
                if (addMethod == null) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlNoAddMethod, type.FullName, currentType, "IEnumerable"));
                }
                return currentType;
            }
            else {
                return null;
            }
        }

        static Type GetCollectionElementType(Type type) {
            if (typeof(IDictionary).IsAssignableFrom(type)) {
                throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedIDictionary, type.FullName));
            }

            PropertyInfo indexer = null;
            for (Type t = type; t != null; t = t.BaseType) {
                MemberInfo[] defaultMembers = type.GetDefaultMembers();
                if (defaultMembers != null) {
                    for (int i = 0; i < defaultMembers.Length; i++) {
                        if (defaultMembers[i] is PropertyInfo) {
                            PropertyInfo defaultProp = (PropertyInfo)defaultMembers[i];
                            if (!defaultProp.CanRead) continue;
                            MethodInfo getMethod = defaultProp.GetGetMethod();
                            ParameterInfo[] parameters = getMethod.GetParameters();
                            if (parameters.Length == 1 && parameters[0].ParameterType == typeof(int)) {
                                indexer = defaultProp;
                                break;
                            }
                        }
                    }
                }
                if (indexer != null) break;
            }

            if (indexer == null) {
                throw new InvalidOperationException(Res.GetString(Res.XmlNoDefaultAccessors, type.FullName));
            }
            
            MethodInfo addMethod = type.GetMethod("Add", new Type[] { indexer.PropertyType });
            if (addMethod == null) {
                throw new InvalidOperationException(Res.GetString(Res.XmlNoAddMethod, type.FullName, indexer.PropertyType, "ICollection"));
            }

            return indexer.PropertyType;
        }
        
        static public XmlQualifiedName ParseWsdlArrayType(string type, out string dims) {
            return ParseWsdlArrayType(type, out dims, null);
        }

        static public XmlQualifiedName ParseWsdlArrayType(string type, out string dims, XmlSchemaObject parent) {
            string ns;
            string name;

            int nsLen = type.LastIndexOf(':');

            if (nsLen <= 0) {
                ns = "";
            }
            else {
                ns = type.Substring(0, nsLen);
            }
            int nameLen = type.IndexOf('[', nsLen + 1);

            if (nameLen <= nsLen) {
                throw new InvalidOperationException(Res.GetString(Res.XmlInvalidArrayTypeSyntax, type));
            }
            name = type.Substring(nsLen + 1, nameLen - nsLen - 1);
            dims = type.Substring(nameLen);

            // parent is not null only in the case when we used XmlSchema.Read(), 
            // in which case we need to fixup the wsdl:arayType attribute value
            while (parent != null) {
                if (parent.Namespaces != null) {
                    string wsdlNs = (string)parent.Namespaces.Namespaces[ns];
                    if (wsdlNs != null) {
                        ns = wsdlNs;
                        break;
                    }
                }
                parent = parent.Parent;
            }
            return new XmlQualifiedName(name, ns);
        }

        public ICollection Types {
            get { return this.typeDescs.Keys; }
        }

        public void AddTypeMapping(TypeMapping typeMapping) {
            typeMappings.Add(typeMapping);
        }

        public ICollection TypeMappings {
            get { return typeMappings; }
        }

        public bool IsValidXsdDataType(string dataType) {
            return (null != GetTypeDesc(new XmlQualifiedName(dataType, XmlSchema.Namespace)));
        }
    }

    internal class Soap {
        public const string Encoding = "http://schemas.xmlsoap.org/soap/encoding/";
        public const string UrType = "anyType";
        public const string Array = "Array";
        public const string ArrayType = "arrayType";
    }

    internal class Wsdl {
        public const string Namespace = "http://schemas.xmlsoap.org/wsdl/";
        public const string ArrayType = "arrayType";
    }

    internal class UrtTypes {
        public const string Namespace = "http://microsoft.com/wsdl/types/";
    }
}

