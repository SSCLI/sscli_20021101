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
    
    public abstract class FieldAccessor{

      public abstract Object GetValue(Object thisob);
      public abstract void SetValue(Object thisob, Object value);
        
      private static SimpleHashtable accessorFor;
      private static AssemblyBuilder assembly;
      private static ModuleBuilder module;
      private static int count = 0;
      
      [ReflectionPermission(SecurityAction.Assert, ReflectionEmit = true), FileIOPermission(SecurityAction.Assert, Unrestricted = true)]
      static FieldAccessor(){
        FieldAccessor.accessorFor = new SimpleHashtable(32);
        AssemblyName name = new AssemblyName();
        name.Name = "JScript FieldAccessor Assembly";
        FieldAccessor.assembly = Thread.GetDomain().DefineDynamicAssembly(name, AssemblyBuilderAccess.Run);
        FieldAccessor.module = FieldAccessor.assembly.DefineDynamicModule("JScript FieldAccessor Module");
      }
      
      internal static FieldAccessor GetAccessorFor(FieldInfo field){
        FieldAccessor accessor = FieldAccessor.accessorFor[field] as FieldAccessor;
        if (accessor != null) return accessor;
        lock(FieldAccessor.accessorFor){
          accessor = FieldAccessor.accessorFor[field] as FieldAccessor;
          if (accessor != null) return accessor;
          accessor = FieldAccessor.SpitAndInstantiateClassFor(field);
          FieldAccessor.accessorFor[field] = accessor;
        }
        return accessor;
      }
      
      [ReflectionPermission(SecurityAction.Assert, Unrestricted = true)]
      private static FieldAccessor SpitAndInstantiateClassFor(FieldInfo field){
        Type ft = field.FieldType;
        TypeBuilder tb = FieldAccessor.module.DefineType("accessor"+FieldAccessor.count++, TypeAttributes.Public, typeof(FieldAccessor));
        
        MethodBuilder mb = tb.DefineMethod("GetValue", MethodAttributes.Public|MethodAttributes.Virtual|MethodAttributes.ReuseSlot, 
          Typeob.Object, new Type[]{Typeob.Object});
        ILGenerator il = mb.GetILGenerator();
        if (field.IsLiteral)
          (new ConstantWrapper(field.GetValue(null), null)).TranslateToIL(il, ft);
        else if (field.IsStatic)
          il.Emit(OpCodes.Ldsfld, field);
        else{
          il.Emit(OpCodes.Ldarg_1);
          il.Emit(OpCodes.Ldfld, field);
        }
        if (ft.IsValueType)
          il.Emit(OpCodes.Box, ft);
        il.Emit(OpCodes.Ret);
        
        
        mb = tb.DefineMethod("SetValue", MethodAttributes.Public|MethodAttributes.Virtual|MethodAttributes.ReuseSlot, 
          Typeob.Void, new Type[]{Typeob.Object, Typeob.Object});
        il = mb.GetILGenerator();
        if (!field.IsLiteral){
          if (!field.IsStatic)
            il.Emit(OpCodes.Ldarg_1);
          il.Emit(OpCodes.Ldarg_2);
          if (ft.IsValueType)
            Convert.EmitUnbox(il, ft, Type.GetTypeCode(ft));
          if (field.IsStatic)
            il.Emit(OpCodes.Stsfld, field);
          else
            il.Emit(OpCodes.Stfld, field);
        }
        il.Emit(OpCodes.Ret);
        
        Type t = tb.CreateType();
        return (FieldAccessor)Activator.CreateInstance(t);
      }
        
    }
    
}
