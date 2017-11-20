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
    using System.Security.Permissions;
    using System.Threading;
    using System.Diagnostics;
    
    public abstract class MethodInvoker{

#if !DEBUG
      [DebuggerStepThroughAttribute]
      [DebuggerHiddenAttribute]
#endif      
      public abstract Object Invoke(Object thisob, Object[] parameters);
        
      private static SimpleHashtable invokerFor;
      private static AssemblyBuilder assembly;
      private static ModuleBuilder module;
      private static int count = 0;
      
      [ReflectionPermission(SecurityAction.Assert, ReflectionEmit = true), FileIOPermission(SecurityAction.Assert, Unrestricted = true)]
      static MethodInvoker(){
        MethodInvoker.invokerFor = new SimpleHashtable(64);
        AssemblyName name = new AssemblyName();
        name.Name = "JScript MethodInvoker Assembly";
        MethodInvoker.assembly = Thread.GetDomain().DefineDynamicAssembly(name, AssemblyBuilderAccess.Run);
        MethodInvoker.module = MethodInvoker.assembly.DefineDynamicModule("JScript MethodInvoker Module");
      }
      
      internal static MethodInvoker GetInvokerFor(MethodInfo method){
        // .NET security does not allow Deny, Assert or PermitOnly to be called via
        // Reflection.  We are effectively creating our own Reflection layer, so
        // we must do the same thing.

        if (method.DeclaringType == typeof(System.Security.CodeAccessPermission) && (method.Name == "Deny" || method.Name == "Assert" || method.Name == "PermitOnly"))
            throw new JScriptException(JSError.CannotCallSecurityMethodLateBound);
        
        MethodInvoker invoker = MethodInvoker.invokerFor[method] as MethodInvoker;
        if (invoker != null) return invoker;
        lock(MethodInvoker.invokerFor){
          invoker = MethodInvoker.invokerFor[method] as MethodInvoker;
          if (invoker != null) return invoker;
          invoker = MethodInvoker.SpitAndInstantiateClassFor(method);
          MethodInvoker.invokerFor[method] = invoker;
        }
        return invoker;
      }
      
      [ReflectionPermission(SecurityAction.Assert, Unrestricted = true)]
      private static MethodInvoker SpitAndInstantiateClassFor(MethodInfo method){
        TypeBuilder tb = MethodInvoker.module.DefineType("invoker"+MethodInvoker.count++, TypeAttributes.Public, typeof(MethodInvoker));
        MethodBuilder mb = tb.DefineMethod("Invoke", MethodAttributes.Public|MethodAttributes.Virtual|MethodAttributes.ReuseSlot, 
          typeof(Object), new Type[]{typeof(Object), typeof(Object[])});
#if !DEBUG
        mb.SetCustomAttribute(new CustomAttributeBuilder(CompilerGlobals.debuggerStepThroughAttributeCtor, new Object[]{}));
        mb.SetCustomAttribute(new CustomAttributeBuilder(CompilerGlobals.debuggerHiddenAttributeCtor, new Object[]{}));
#endif      
        ILGenerator il = mb.GetILGenerator();
        if (!method.DeclaringType.IsPublic) method = method.GetBaseDefinition();
        Type obT = method.DeclaringType;
        if (!method.IsStatic){
          il.Emit(OpCodes.Ldarg_1);
          if (obT.IsValueType){
            Convert.EmitUnbox(il, obT, Type.GetTypeCode(obT));
            Convert.EmitLdloca(il, obT);
          }else{
            il.Emit(OpCodes.Castclass, obT);
          }
        }
        ParameterInfo[] parameters = method.GetParameters();
        LocalBuilder[] outTemps = null;
        for (int i = 0, n = parameters.Length; i < n; i++){
          il.Emit(OpCodes.Ldarg_2);
          ConstantWrapper.TranslateToILInt(il, i);
          Type pt = parameters[i].ParameterType;
          if (pt.IsByRef){
            pt = pt.GetElementType();
            if (outTemps == null) outTemps = new LocalBuilder[n];
            outTemps[i] = il.DeclareLocal(pt);
            il.Emit(OpCodes.Ldelem_Ref);
            if (pt.IsValueType)
              Convert.EmitUnbox(il, pt, Type.GetTypeCode(pt));
            il.Emit(OpCodes.Stloc, outTemps[i]);
            il.Emit(OpCodes.Ldloca, outTemps[i]);
          }else{
            il.Emit(OpCodes.Ldelem_Ref);
            if (pt.IsValueType)
              Convert.EmitUnbox(il, pt, Type.GetTypeCode(pt));
          }
        }
        if (!method.IsStatic && method.IsVirtual && !method.IsFinal && !obT.IsSealed)
          il.Emit(OpCodes.Callvirt, method);
        else
          il.Emit(OpCodes.Call, method);
        Type rt = method.ReturnType;
        if (rt == Typeob.Void)
          il.Emit(OpCodes.Ldnull);
        else if (rt.IsValueType)
          il.Emit(OpCodes.Box, rt);
        if (outTemps != null){
          for (int i = 0, n = parameters.Length; i < n; i++){
            LocalBuilder loc = outTemps[i];
            if (loc != null){
              il.Emit(OpCodes.Ldarg_2);
              ConstantWrapper.TranslateToILInt(il, i);
              il.Emit(OpCodes.Ldloc, loc);
              Type pt = parameters[i].ParameterType.GetElementType();
              if (pt.IsValueType)
                il.Emit(OpCodes.Box, pt);
              il.Emit(OpCodes.Stelem_Ref);
            }
          }
        }
        il.Emit(OpCodes.Ret);
        Type t = tb.CreateType();
        return (MethodInvoker)Activator.CreateInstance(t);
      }
        
    }
    
}


      

