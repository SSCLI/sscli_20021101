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
//This class is a compile time helper only. It is used by class definitions marked as expando. The evaluator never sees instances of this class.

namespace Microsoft.JScript 
{
    
  using System;
  using System.Reflection;
  using System.Globalization;
  using System.Diagnostics;
  
  internal sealed class JSExpandoIndexerMethod : JSMethod{
    private ClassScope classScope;
    private bool isGetter;
    private MethodInfo token;

    private static ParameterInfo[] GetterParams = new ParameterInfo[]{new ParameterDeclaration(Typeob.String, "field")};
    private static ParameterInfo[] SetterParams = new ParameterInfo[]{new ParameterDeclaration(Typeob.String, "field"), 
                                                                      new ParameterDeclaration(Typeob.Object, "value")};
  
    internal JSExpandoIndexerMethod(ClassScope classScope, bool isGetter)
      : base(null){ //The object is never used, but it is convenient to have the field on JSMethod.
      this.isGetter = isGetter;
      this.classScope = classScope;
    }

    internal override Object Construct(Object[] args){
      throw new JScriptException(JSError.InvalidCall);
    }
    
    public override MethodAttributes Attributes{
      get{
        return MethodAttributes.Public;
      }
    }

    public override Type DeclaringType{
      get{
        return this.classScope.GetTypeBuilderOrEnumBuilder();
      }
    }

    internal ScriptObject EnclosingScope(){
      return this.classScope;
    }
    
    public override ParameterInfo[] GetParameters(){
      if (this.isGetter)
        return JSExpandoIndexerMethod.GetterParams;
      else
        return JSExpandoIndexerMethod.SetterParams;
    }
    
    internal override MethodInfo GetMethodInfo(CompilerGlobals compilerGlobals){
      if (this.isGetter){
        if (this.token == null)
          this.token = this.classScope.owner.GetExpandoIndexerGetter();
      }else{
        if (this.token == null)
          this.token = this.classScope.owner.GetExpandoIndexerSetter();
      }
      return this.token;
    }
    
#if !DEBUG
      [DebuggerStepThroughAttribute]
      [DebuggerHiddenAttribute]
#endif      
    internal override Object Invoke(Object obj, Object thisob, BindingFlags options, Binder binder, Object[] parameters, CultureInfo culture){
      throw new JScriptException(JSError.InvalidCall);
    }     
    
    public override String Name{
      get{
        if (this.isGetter)
          return "get_Item";
        else
          return "set_Item";
      }
    }
  
    public override Type ReturnType{
      get{
        if (this.isGetter)
          return Typeob.Object;
        else
          return Typeob.Void;
      }
    }
    
    internal IReflect ReturnIR(){
      return this.ReturnType;
    }
    
  }

}
