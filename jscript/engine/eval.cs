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

    using Microsoft.JScript.Vsa;
    using System;
    using System.Reflection;
    using System.Reflection.Emit;

    public sealed class Eval : AST{
      private AST operand;
      private FunctionScope enclosingFunctionScope;

      internal Eval(Context context, AST operand)
        : base(context) {
        this.operand = operand;
        ScriptObject enclosingScope = Globals.ScopeStack.Peek();
        ((IActivationObject)enclosingScope).GetGlobalScope().evilScript = true;
        if (enclosingScope is ActivationObject)
          ((ActivationObject)enclosingScope).isKnownAtCompileTime = this.Engine.doFast;
        if (enclosingScope is FunctionScope){
          this.enclosingFunctionScope = (FunctionScope)enclosingScope;
          this.enclosingFunctionScope.mustSaveStackLocals = true;
          ScriptObject scope = (ScriptObject)this.enclosingFunctionScope.GetParent();
          while (scope != null){
            FunctionScope fscope = scope as FunctionScope;
            if (fscope != null){
              fscope.mustSaveStackLocals = true;
              fscope.closuresMightEscape = true;
            }
            scope = (ScriptObject)scope.GetParent();
          }
        }else
          this.enclosingFunctionScope = null;
      }

      internal override void CheckIfOKToUseInSuperConstructorCall(){
        this.context.HandleError(JSError.NotAllowedInSuperConstructorCall);
      }

      internal override Object Evaluate(){
        if( VsaEngine.executeForJSEE )
          throw new JScriptException(JSError.NonSupportedInDebugger);
        Object v = this.operand.Evaluate();
        Globals.CallContextStack.Push(new CallContext(this.context, v));
        try{
          try{
            return JScriptEvaluate(v, this.Engine);
          }catch(JScriptException e){
            if (e.context == null){
              e.context = this.context;
            }
            throw e;
          }catch(Exception e){
            throw new JScriptException(e, this.context);
          }
        }finally{
          Globals.CallContextStack.Pop();
        }
      }

      public static Object JScriptEvaluate(Object source, VsaEngine engine){
        if (Convert.GetTypeCode(source) != TypeCode.String)
          return source;
        if (engine.doFast)
          engine.PushScriptObject(new BlockScope(engine.ScriptObjectStackTop()));
        try{
          Context context = new Context(new DocumentContext("eval code", engine), ((IConvertible)source).ToString());
          JSParser p = new JSParser(context);
          return ((Completion)p.ParseEvalBody().PartiallyEvaluate().Evaluate()).value;
        }finally{
          if (engine.doFast)
            engine.PopScriptObject();
        }
      }

      internal override AST PartiallyEvaluate(){
        VsaEngine engine = this.Engine;
        ScriptObject scope = Globals.ScopeStack.Peek();
        if (engine.doFast)
          engine.PushScriptObject(new BlockScope(scope));
        else{
          while (scope is WithObject || scope is BlockScope){
            if (scope is BlockScope)
              ((BlockScope)scope).isKnownAtCompileTime = false;
            scope = scope.GetParent();
          }
        }
        try{
          this.operand = this.operand.PartiallyEvaluate();
          if (this.enclosingFunctionScope != null && this.enclosingFunctionScope.owner == null)
            this.context.HandleError(JSError.NotYetImplemented);
          return this;
        }finally{
          if (engine.doFast)
            this.Engine.PopScriptObject();
        }
      }

      internal override void TranslateToIL(ILGenerator il, Type rtype){
        if (this.enclosingFunctionScope != null && this.enclosingFunctionScope.owner != null)
          this.enclosingFunctionScope.owner.TranslateToILToSaveLocals(il);
        this.operand.TranslateToIL(il, Typeob.Object);
        this.EmitILToLoadEngine(il);
        il.Emit(OpCodes.Call, CompilerGlobals.jScriptEvaluateMethod);
        Convert.Emit(this, il, Typeob.Object, rtype);
        if (this.enclosingFunctionScope != null && this.enclosingFunctionScope.owner != null)
          this.enclosingFunctionScope.owner.TranslateToILToRestoreLocals(il);
      }

      internal override void TranslateToILInitializer(ILGenerator il){
        this.operand.TranslateToILInitializer(il);
      }
    }
}
