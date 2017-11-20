// ==++==
//
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
//
// ==--==
namespace Microsoft.JScript 
{
    
    using System;
    using System.Collections;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Text;
    
    internal sealed class TypeExpression : AST{
      internal AST expression;
      internal bool isArray;
      internal int rank;
      private bool recursive;
      private IReflect cachedIR;
      
      internal static readonly SimpleHashtable PredefinedType = new SimpleHashtable(32);
      
      static TypeExpression(){
        PredefinedType["boolean"] = Typeob.Boolean;
        PredefinedType["byte"] = Typeob.Byte;
        PredefinedType["char"] = Typeob.Char;
        PredefinedType["decimal"] = Typeob.Decimal;
        PredefinedType["double"] = Typeob.Double;
        PredefinedType["float"] = Typeob.Single;
        PredefinedType["int"] = Typeob.Int32;
        PredefinedType["long"] = Typeob.Int64;
        PredefinedType["sbyte"] = Typeob.SByte;
        PredefinedType["short"] = Typeob.Int16;
        PredefinedType["void"] = Typeob.Void;
        PredefinedType["uint"] = Typeob.UInt32;
        PredefinedType["ulong"] = Typeob.UInt64;
        PredefinedType["ushort"] = Typeob.UInt16;

        PredefinedType["ActiveXObject"] = Typeob.Object;
        PredefinedType["Array"] = typeof(ArrayObject);
        PredefinedType["Boolean"] = Typeob.Boolean;
        PredefinedType["Date"] = Typeob.DateObject;
        PredefinedType["Enumerator"] = typeof(EnumeratorObject);
        PredefinedType["Error"] = typeof(ErrorObject);
        PredefinedType["EvalError"] = typeof(EvalErrorObject);
        PredefinedType["Function"] = typeof(ScriptFunction);
        PredefinedType["Number"] = Typeob.Double;
        PredefinedType["Object"] = Typeob.Object;
        PredefinedType["RangeError"] = typeof(RangeErrorObject);
        PredefinedType["ReferenceError"] = typeof(ReferenceErrorObject);
        PredefinedType["RegExp"] = typeof(RegExpObject);
        PredefinedType["String"] = Typeob.String;
        PredefinedType["SyntaxError"] = typeof(SyntaxErrorObject);
        PredefinedType["Type"] = Typeob.Type;
        PredefinedType["TypeError"] = typeof(TypeErrorObject);
        PredefinedType["URIError"] = typeof(URIErrorObject);
        PredefinedType["VBArray"] = typeof(VBArrayObject);
      }
      
      internal TypeExpression(AST expression)
        : base(expression.context) {
        this.expression = expression;
        this.isArray = false;
        this.rank = 0;
        this.recursive = false;
        this.cachedIR = null;
        if (expression is Lookup){
          String name = expression.ToString();
          Object ptype = PredefinedType[name];
          if (ptype != null)
            this.expression = new ConstantWrapper(ptype, expression.context);
        }
      }
    
      internal override Object Evaluate(){
        return this.ToIReflect();
      }
      
      internal override IReflect InferType(JSField inference_target){
        return this.ToIReflect();
      }
    
      // has to be called after partially evaluate
      internal bool IsCLSCompliant(){
        Object value = this.expression.Evaluate();
        return TypeExpression.TypeIsCLSCompliant(value);
      }
        
      internal override AST PartiallyEvaluate(){
        if (this.recursive){
          if (this.expression is ConstantWrapper) return this;
          this.expression = new ConstantWrapper(Typeob.Object, this.context);
          return this;
        }
        Member member = this.expression as Member;
        if (member != null){
          //Dealing with a qualified name. See if there is a type/class with such a name, bypassing normal scope lookup.
          Object type = member.EvaluateAsType();
          if (type != null){
            this.expression = new ConstantWrapper(type, member.context);
            return this;
          }
        }
        this.recursive = true;
        this.expression = this.expression.PartiallyEvaluate();
        this.recursive = false;
        if (this.expression is TypeExpression)
          return this;
        //Make sure that the expression represents a Type
        Type t = null;
        if (this.expression is ConstantWrapper){
          Object val = this.expression.Evaluate();
          if (val == null){
            this.expression.context.HandleError(JSError.NeedType);
            this.expression = new ConstantWrapper(Typeob.Object, this.context);
            return this;
          }
          t = val.GetType();
          Binding.WarnIfObsolete(val as Type, this.expression.context);
        }else{
          if (this.expression.OkToUseAsType())
            t = this.expression.Evaluate().GetType();
          else{
            this.expression.context.HandleError(JSError.NeedCompileTimeConstant);
            this.expression = new ConstantWrapper(Typeob.Object, this.expression.context);
            return this;
          }
        }
        if (t == null || (t != Typeob.ClassScope && t != Typeob.TypedArray && !typeof(Type).IsAssignableFrom(t))){
          this.expression.context.HandleError(JSError.NeedType);
          this.expression = new ConstantWrapper(Typeob.Object, this.expression.context);
        }
        return this;
      }
      
      internal IReflect ToIReflect(){
        if (!(this.expression is ConstantWrapper))
          this.PartiallyEvaluate();
        IReflect result = this.cachedIR;
        if (result != null) return result;
        Object value = this.expression.Evaluate();
        if (value is ClassScope || value is TypedArray || this.context == null)
          result = (IReflect)value;
        else
          result = Convert.ToIReflect((Type)value, this.Engine);
        if (this.isArray)
          return this.cachedIR = new TypedArray(result, this.rank);
        else
          return this.cachedIR = result;
      }
      
      internal Type ToType(){
        if (!(this.expression is ConstantWrapper))
          this.PartiallyEvaluate();
        Object value = this.expression.Evaluate();
        Type result = null;
        if (value is ClassScope)
          result = ((ClassScope)value).GetTypeBuilderOrEnumBuilder();
        else if (value is TypedArray)
          result = Convert.ToType((TypedArray)value);
        else
          result = (Type)value;
        if (this.isArray){
          return Convert.ToType(TypedArray.ToRankString(this.rank), result);
        }else
          return result;
      }
      
      internal override void TranslateToIL(ILGenerator il, Type rtype){
        this.expression.TranslateToIL(il, rtype);
      }
      
      internal override void TranslateToILInitializer(ILGenerator il){
        this.expression.TranslateToILInitializer(il);
      }
      
      internal static bool TypeIsCLSCompliant(Object type){
        if (type is ClassScope)
          return ((ClassScope)type).IsCLSCompliant();
        else if (type is TypedArray){
          Object et = ((TypedArray)type).elementType;
          if (et is TypedArray || (et is Type && ((Type)et).IsArray)) return false;
          return TypeExpression.TypeIsCLSCompliant(et);
        }else{
          Type t = (Type)type;
          if (t.IsPrimitive)
            if (t == Typeob.Boolean ||
                t == Typeob.Byte ||
                t == Typeob.Char ||
                t == Typeob.Double ||
                t == Typeob.Int16 ||
                t == Typeob.Int32 ||
                t == Typeob.Int64 ||
                t == Typeob.Single)
              return true;
            else
              return false;
          else{
            if (t.IsArray){
              Type et = t.GetElementType();
              if (et.IsArray) return false;
              return TypeExpression.TypeIsCLSCompliant(t);
            }
            Object[] attr = t.GetCustomAttributes(Typeob.CLSCompliantAttribute, false);
            if (attr.Length > 0)
              return ((CLSCompliantAttribute)attr[0]).IsCompliant;
            else{
              Module m = t.Module;
              attr = m.GetCustomAttributes(Typeob.CLSCompliantAttribute, false);
              if (attr.Length > 0)
                return ((CLSCompliantAttribute)attr[0]).IsCompliant;
              else{
                Assembly a = m.Assembly;
                attr = a.GetCustomAttributes(Typeob.CLSCompliantAttribute, false);
                if (attr.Length > 0)
                  return ((CLSCompliantAttribute)attr[0]).IsCompliant;
                else                
                  return false;
              }
            }
          }
        }
      }

    }
 }
