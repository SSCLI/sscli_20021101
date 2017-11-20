//------------------------------------------------------------------------------
// <copyright file="Models.cs" company="Microsoft">
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
    using System.Diagnostics;

    // These classes define the abstract serialization model, e.g. the rules for WHAT is serialized.  
    // The answer of HOW the values are serialized is answered by a particular reflection importer 
    // by looking for a particular set of custom attributes specific to the serialization format
    // and building an appropriate set of accessors/mappings.

    internal class ModelScope {
        TypeScope typeScope;
        Hashtable models = new Hashtable();
        Hashtable arrayModels = new Hashtable();

        public ModelScope(TypeScope typeScope) {
            this.typeScope = typeScope;
        }

        public TypeScope TypeScope {
            get { return typeScope; }
        }

        public TypeModel GetTypeModel(Type type) {
            TypeModel model = (TypeModel)models[type];
            if (model != null) return model;
            TypeDesc typeDesc = typeScope.GetTypeDesc(type);

            switch (typeDesc.Kind) {
                case TypeKind.Enum: 
                    model = new EnumModel(type, typeDesc, this);
                    break;
                case TypeKind.Primitive:
                    model = new PrimitiveModel(type, typeDesc, this);
                    break;
                case TypeKind.Array:
                case TypeKind.Collection:
                case TypeKind.Enumerable:
                    model = new ArrayModel(type, typeDesc, this);
                    break;
                case TypeKind.Root:
                case TypeKind.Class:
                case TypeKind.Struct:
                case TypeKind.Interface:
                    model = new StructModel(type, typeDesc, this);
                    break;
                default:
                    if (!typeDesc.IsSpecial) throw new NotSupportedException(Res.GetString(Res.XmlUnsupportedTypeKind, type.FullName));
                    model = new SpecialModel(type, typeDesc, this);
                    break;
            }

            models.Add(type, model);
            return model;
        }

        public ArrayModel GetArrayModel(Type type) {
            TypeModel model = (TypeModel) arrayModels[type];
            if (model == null) {
                model = GetTypeModel(type);
                if (!(model is ArrayModel)) {
                    TypeDesc typeDesc = typeScope.GetArrayTypeDesc(type);
                    model = new ArrayModel(type, typeDesc, this);
                }
                arrayModels.Add(type, model);
            }
            return (ArrayModel) model;
        }
    }

    internal abstract class TypeModel {
        TypeDesc typeDesc;
        Type type;
        ModelScope scope;

        protected TypeModel(Type type, TypeDesc typeDesc, ModelScope scope) {
            this.scope = scope;
            this.type = type;
            this.typeDesc = typeDesc;
        }

        public Type Type {
            get { return type; }
        }

        public ModelScope ModelScope {
            get { return scope; }
        }

        public TypeDesc TypeDesc {
            get { return typeDesc; }
        }
    }

    internal class ArrayModel : TypeModel {
        public ArrayModel(Type type, TypeDesc typeDesc, ModelScope scope) : base(type, typeDesc, scope) { }

        public TypeModel Element {
            get { return ModelScope.GetTypeModel(TypeScope.GetArrayElementType(Type)); }
        }
    }

    internal class PrimitiveModel : TypeModel {
        public PrimitiveModel(Type type, TypeDesc typeDesc, ModelScope scope) : base(type, typeDesc, scope) { }
    }

    internal class SpecialModel : TypeModel {
        public SpecialModel(Type type, TypeDesc typeDesc, ModelScope scope) : base(type, typeDesc, scope) { }
    }

    internal class StructModel : TypeModel {

        public StructModel(Type type, TypeDesc typeDesc, ModelScope scope) : base(type, typeDesc, scope) { }

        public MemberInfo[] GetMemberInfos() {
            return Type.GetMembers(BindingFlags.DeclaredOnly | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
        }

        public FieldModel GetFieldModel(MemberInfo memberInfo) {
            FieldModel model = null;
            if (memberInfo is FieldInfo)
                model = GetFieldModel((FieldInfo)memberInfo);
            else if (memberInfo is PropertyInfo)
                model = GetPropertyModel((PropertyInfo)memberInfo);
            
            if (model != null) {
                if (model.ReadOnly && model.FieldTypeDesc.Kind != TypeKind.Collection && model.FieldTypeDesc.Kind != TypeKind.Enumerable)
                    return null;
            }
            return model;
        }

        FieldModel GetFieldModel(FieldInfo fieldInfo) {
            if (fieldInfo.IsStatic) return null;
            if (fieldInfo.DeclaringType != Type) return null;
            TypeDesc typeDesc = ModelScope.TypeScope.GetTypeDesc(fieldInfo.FieldType, fieldInfo);
            return new FieldModel(fieldInfo, fieldInfo.FieldType, typeDesc);
        }

        FieldModel GetPropertyModel(PropertyInfo propertyInfo) {
            if (propertyInfo.DeclaringType != Type) return null;
            TypeDesc typeDesc = ModelScope.TypeScope.GetTypeDesc(propertyInfo.PropertyType, propertyInfo);

            if (!propertyInfo.CanRead) return null;

            foreach (MethodInfo accessor in propertyInfo.GetAccessors())
                if ((accessor.Attributes & MethodAttributes.HasSecurity) != 0)
                    throw new InvalidOperationException(Res.GetString(Res.XmlPropertyHasSecurityAttributes, propertyInfo.Name, Type.FullName));
            if ((propertyInfo.DeclaringType.Attributes & TypeAttributes.HasSecurity) != 0)
                throw new InvalidOperationException(Res.GetString(Res.XmlPropertyHasSecurityAttributes, propertyInfo.Name, Type.FullName));
                
            MethodInfo getMethod = propertyInfo.GetGetMethod();
            ParameterInfo[] parameters = getMethod.GetParameters();
            if (parameters.Length > 0) return null;

            return new FieldModel(propertyInfo, propertyInfo.PropertyType, typeDesc);
        }
    }


    internal class FieldModel {
        bool checkSpecified;
        bool checkShouldPersist;
        bool readOnly = false;
        Type fieldType;
        string name;
        TypeDesc fieldTypeDesc;

        public FieldModel(string name, Type fieldType, TypeDesc fieldTypeDesc, bool checkSpecified, bool checkShouldPersist) : 
            this(name, fieldType, fieldTypeDesc, checkSpecified, checkShouldPersist, false) {
        }
        public FieldModel(string name, Type fieldType, TypeDesc fieldTypeDesc, bool checkSpecified, bool checkShouldPersist, bool readOnly) {
            this.fieldTypeDesc = fieldTypeDesc;
            this.name = name;
            this.fieldType = fieldType;
            this.checkSpecified = checkSpecified;
            this.checkShouldPersist = checkShouldPersist;
            this.readOnly = readOnly;
        }

        public FieldModel(MemberInfo memberInfo, Type fieldType, TypeDesc fieldTypeDesc) {
            this.name = memberInfo.Name;
            this.fieldType = fieldType;
            this.fieldTypeDesc = fieldTypeDesc;
            this.checkShouldPersist = memberInfo.DeclaringType.GetMethod("ShouldSerialize" + memberInfo.Name, new Type[0]) != null;
            this.checkSpecified = memberInfo.DeclaringType.GetField(memberInfo.Name + "Specified") != null;
            if (!this.checkSpecified) {
                this.checkSpecified = memberInfo.DeclaringType.GetProperty(memberInfo.Name + "Specified") != null;
            }
            if (memberInfo is PropertyInfo) {
                readOnly = !((PropertyInfo)memberInfo).CanWrite;
            }
            else if (memberInfo is FieldInfo) {
                readOnly = ((FieldInfo)memberInfo).IsInitOnly;
            }
        }

        public string Name {
            get { return name; }
        }

        public Type FieldType {
            get { return fieldType; }
        }

        public TypeDesc FieldTypeDesc {
            get { return fieldTypeDesc; }
        }

        public bool CheckShouldPersist {
            get { return checkShouldPersist; }
        }

        public bool CheckSpecified {
            get { return checkSpecified; }
        }

        internal bool ReadOnly {
            get { return readOnly; }
        }
    }

    internal class ConstantModel {
        FieldInfo fieldInfo;
        long value;

        public ConstantModel(FieldInfo fieldInfo, long value) {
            this.fieldInfo = fieldInfo;
            this.value = value;
        }

        public string Name {
            get { return fieldInfo.Name; }
        }

        public long Value {
            get { return value; }
        }

        public FieldInfo FieldInfo {
            get { return fieldInfo; }
        }
    }

    internal class EnumModel : TypeModel {
        ConstantModel[] constants;

        public EnumModel(Type type, TypeDesc typeDesc, ModelScope scope) : base(type, typeDesc, scope) { }

        public ConstantModel[] Constants {
            get {
                if (constants == null) {
                    ArrayList list = new ArrayList();
                    FieldInfo[] fields = Type.GetFields();
                    for (int i = 0; i < fields.Length; i++) {
                        FieldInfo field = fields[i];
                        ConstantModel constant = GetConstantModel(field);
                        if (constant != null) list.Add(constant);
                    }
                    constants = (ConstantModel[])list.ToArray(typeof(ConstantModel));
                }
                return constants;
            }

        }

        ConstantModel GetConstantModel(FieldInfo fieldInfo) {
            if (fieldInfo.IsSpecialName) return null;
            return new ConstantModel(fieldInfo, ((IConvertible)fieldInfo.GetValue(null)).ToInt64(null));
        }
    }
}

