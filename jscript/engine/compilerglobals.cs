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
    using Microsoft.Vsa;
    using System;
    using System.Collections;
    using System.Configuration.Assemblies;
    using System.Diagnostics;
    using System.IO;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.CompilerServices;
    
    internal sealed class CompilerGlobals{
      internal static readonly Type iActivationObjectClass = typeof(IActivationObject);
      internal static readonly Type jsLocalFieldClass = typeof(JSLocalField);
      internal static readonly Type stackFrameClass = typeof(StackFrame);
    
      internal static readonly FieldInfo closureInstanceField = (typeof(StackFrame)).GetField("closureInstance");
      internal static readonly FieldInfo contextEngineField = (typeof(Globals)).GetField("contextEngine");
      internal static readonly FieldInfo engineField = (typeof(ScriptObject)).GetField("engine");
      internal static readonly FieldInfo localVarsField = (typeof(StackFrame)).GetField("localVars");
      internal static readonly FieldInfo missingField = (typeof(Missing)).GetField("Value");
      internal static readonly FieldInfo objectField = (typeof(LateBinding)).GetField("obj");
      internal static readonly FieldInfo undefinedField = (typeof(Empty)).GetField("Value");
      internal static readonly FieldInfo systemReflectionMissingField = (typeof(System.Reflection.Missing)).GetField("Value");
          
      internal static readonly ConstructorInfo bitwiseBinaryConstructor = (typeof(BitwiseBinary)).GetConstructor(new Type[]{Typeob.Int32});
      internal static readonly ConstructorInfo breakOutOfFinallyConstructor = (typeof(BreakOutOfFinally)).GetConstructor(new Type[]{Typeob.Int32});
      internal static readonly MethodInfo callValueMethod = (typeof(LateBinding)).GetMethod("CallValue", new Type[]{Typeob.Object, Typeob.Object, typeof(Object[]), Typeob.Boolean, Typeob.Boolean, Typeob.VsaEngine});
      internal static readonly MethodInfo callValue2Method = (typeof(LateBinding)).GetMethod("CallValue2", new Type[]{Typeob.Object, Typeob.Object, typeof(Object[]), Typeob.Boolean, Typeob.Boolean, Typeob.VsaEngine});
      internal static readonly MethodInfo callMethod = (typeof(LateBinding)).GetMethod("Call", new Type[]{typeof(Object[]), Typeob.Boolean, Typeob.Boolean, Typeob.VsaEngine});
      internal static readonly MethodInfo changeTypeMethod = typeof(System.Convert).GetMethod("ChangeType", new Type[]{Typeob.Object, typeof(TypeCode)});
      internal static readonly MethodInfo checkIfDoubleIsIntegerMethod = (typeof(Convert)).GetMethod("CheckIfDoubleIsInteger");
      internal static readonly MethodInfo checkIfSingleIsIntegerMethod = (typeof(Convert)).GetMethod("CheckIfSingleIsInteger");
      internal static readonly ConstructorInfo clsCompliantAttributeCtor = Typeob.CLSCompliantAttribute.GetConstructor(new Type[]{Typeob.Boolean});
      internal static readonly ConstructorInfo closureConstructor = (typeof(Closure)).GetConstructor(new Type[]{typeof(FunctionObject)});
      internal static readonly MethodInfo coerceTMethod = (typeof(Convert)).GetMethod("CoerceT");
      internal static readonly MethodInfo coerce2Method = (typeof(Convert)).GetMethod("Coerce2");
      internal static readonly ConstructorInfo contextStaticAttributeCtor = (typeof(ContextStaticAttribute)).GetConstructor(new Type[]{});
      internal static readonly MethodInfo constructArrayMethod = (typeof(ArrayConstructor)).GetMethod("ConstructArray");
      internal static readonly MethodInfo constructObjectMethod = (typeof(ObjectConstructor)).GetMethod("ConstructObject");
      internal static readonly ConstructorInfo continueOutOfFinallyConstructor = (typeof(ContinueOutOfFinally)).GetConstructor(new Type[]{Typeob.Int32});
      internal static readonly MethodInfo convertCharToStringMethod = (typeof(System.Convert)).GetMethod("ToString", new Type[]{Typeob.Char});
      internal static readonly MethodInfo createVsaEngine = (Typeob.VsaEngine).GetMethod("CreateEngine", new Type[]{});
      internal static readonly MethodInfo createVsaEngineWithType = (Typeob.VsaEngine).GetMethod("CreateEngineWithType", new Type[]{typeof(RuntimeTypeHandle)});
      internal static readonly ConstructorInfo dateTimeConstructor = (typeof(DateTime)).GetConstructor(new Type[]{Typeob.Int64});
      internal static readonly MethodInfo dateTimeToInt64Method = (typeof(DateTime)).GetProperty("Ticks").GetGetMethod();
      internal static readonly MethodInfo dateTimeToStringMethod = (typeof(DateTime)).GetMethod("ToString", new Type[]{});
      internal static readonly ConstructorInfo debuggerStepThroughAttributeCtor = typeof(DebuggerStepThroughAttribute).GetConstructor(new Type[]{});
      internal static readonly ConstructorInfo debuggerHiddenAttributeCtor = typeof(DebuggerHiddenAttribute).GetConstructor(new Type[]{});
      internal static readonly ConstructorInfo decimalConstructor = typeof(Decimal).GetConstructor(new Type[]{Typeob.Int32, Typeob.Int32, Typeob.Int32, Typeob.Boolean, Typeob.Byte});
      internal static readonly MethodInfo decimalToDoubleMethod = (typeof(Decimal)).GetMethod("ToDouble", new Type[]{typeof(Decimal)});
      internal static readonly MethodInfo decimalToInt32Method = (typeof(Decimal)).GetMethod("ToInt32", new Type[]{typeof(Decimal)});
      internal static readonly MethodInfo decimalToInt64Method = (typeof(Decimal)).GetMethod("ToInt64", new Type[]{typeof(Decimal)});
      internal static readonly MethodInfo decimalToStringMethod = (typeof(Decimal)).GetMethod("ToString", new Type[]{});
      internal static readonly MethodInfo decimalToUInt32Method = (typeof(Decimal)).GetMethod("ToUInt32", new Type[]{typeof(Decimal)});
      internal static readonly MethodInfo decimalToUInt64Method = (typeof(Decimal)).GetMethod("ToUInt64", new Type[]{typeof(Decimal)});
      internal static readonly ConstructorInfo defaultMemberAttributeCtor = (typeof(DefaultMemberAttribute)).GetConstructor(new Type[]{Typeob.String});
      internal static readonly MethodInfo deleteMethod = (typeof(LateBinding)).GetMethod("Delete");
      internal static readonly MethodInfo deleteMemberMethod = (typeof(LateBinding)).GetMethod("DeleteMember");
      internal static readonly MethodInfo doubleToBooleanMethod = (typeof(Convert)).GetMethod("ToBoolean", new Type[]{Typeob.Double});
      internal static readonly MethodInfo doubleToDecimalMethod = (typeof(Decimal)).GetMethod("op_Explicit", new Type[]{Typeob.Double});
      internal static readonly MethodInfo doubleToStringMethod = (typeof(Convert)).GetMethod("ToString", new Type[]{Typeob.Double});
      internal static readonly ConstructorInfo equalityConstructor = (typeof(Equality)).GetConstructor(new Type[]{Typeob.Int32});
      internal static readonly MethodInfo equalsMethod = (Typeob.Object).GetMethod("Equals", new Type[]{Typeob.Object});
      internal static readonly MethodInfo evaluateBitwiseBinaryMethod = (typeof(BitwiseBinary)).GetMethod("EvaluateBitwiseBinary");
      internal static readonly MethodInfo evaluateEqualityMethod = (typeof(Equality)).GetMethod("EvaluateEquality", new Type[]{Typeob.Object, Typeob.Object});
      internal static readonly MethodInfo evaluateNumericBinaryMethod = (typeof(NumericBinary)).GetMethod("EvaluateNumericBinary");
      internal static readonly MethodInfo evaluatePlusMethod = (typeof(Plus)).GetMethod("EvaluatePlus");
      internal static readonly MethodInfo evaluatePostOrPrefixOperatorMethod = (typeof(PostOrPrefixOperator)).GetMethod("EvaluatePostOrPrefix");
      internal static readonly MethodInfo evaluateRelationalMethod = (typeof(Relational)).GetMethod("EvaluateRelational");
      internal static readonly MethodInfo evaluateUnaryMethod = (typeof(NumericUnary)).GetMethod("EvaluateUnary");
      internal static readonly MethodInfo fastConstructArrayLiteralMethod = (typeof(Globals)).GetMethod("ConstructArrayLiteral");
      internal static readonly MethodInfo getCurrentMethod = (typeof(IEnumerator)).GetProperty("Current", Type.EmptyTypes).GetGetMethod();
      internal static readonly MethodInfo getDefaultThisObjectMethod = (typeof(IActivationObject)).GetMethod("GetDefaultThisObject");
      internal static readonly MethodInfo getEngineMethod = (typeof(INeedEngine)).GetMethod("GetEngine");
      internal static readonly MethodInfo getEnumeratorMethod = typeof(IEnumerable).GetMethod("GetEnumerator", Type.EmptyTypes);
      internal static readonly MethodInfo getFieldMethod = (typeof(IActivationObject)).GetMethod("GetField", new Type[]{Typeob.String, Typeob.Int32});
      internal static readonly MethodInfo getFieldValueMethod = (typeof(FieldInfo)).GetMethod("GetValue", new Type[]{Typeob.Object});
      internal static readonly MethodInfo getGlobalScopeMethod = (typeof(IActivationObject)).GetMethod("GetGlobalScope");
      internal static readonly MethodInfo getLenientGlobalObjectMethod = (Typeob.VsaEngine).GetProperty("LenientGlobalObject").GetGetMethod();
      internal static readonly MethodInfo getMemberValueMethod = (typeof(IActivationObject)).GetMethod("GetMemberValue", new Type[]{Typeob.String, Typeob.Int32});
      internal static readonly MethodInfo getMethodMethod = (typeof(Type)).GetMethod("GetMethod", new Type[]{Typeob.String});
      internal static readonly MethodInfo getNamespaceMethod = (typeof(Namespace)).GetMethod("GetNamespace");
      internal static readonly MethodInfo getNonMissingValueMethod = (typeof(LateBinding)).GetMethod("GetNonMissingValue");
      internal static readonly MethodInfo getOriginalArrayConstructorMethod = (Typeob.VsaEngine).GetMethod("GetOriginalArrayConstructor");
      internal static readonly MethodInfo getOriginalObjectConstructorMethod = (Typeob.VsaEngine).GetMethod("GetOriginalObjectConstructor");
      internal static readonly MethodInfo getOriginalRegExpConstructorMethod = (Typeob.VsaEngine).GetMethod("GetOriginalRegExpConstructor");
      internal static readonly MethodInfo getParentMethod = (typeof(ScriptObject)).GetMethod("GetParent");
      internal static readonly MethodInfo getTypeMethod = (typeof(Type)).GetMethod("GetType", new Type[]{Typeob.String});
      internal static readonly MethodInfo getTypeFromHandleMethod = (typeof(Type)).GetMethod("GetTypeFromHandle", new Type[]{typeof(RuntimeTypeHandle)});
      internal static readonly MethodInfo getValue2Method = (typeof(LateBinding)).GetMethod("GetValue2");
      internal static readonly ConstructorInfo GlobalScopeConstructor = typeof(GlobalScope).GetConstructor(new Type[]{typeof(GlobalScope), Typeob.VsaEngine});
      internal static readonly ConstructorInfo hashtableCtor = Typeob.SimpleHashtable.GetConstructor(new Type[]{Typeob.UInt32});
      internal static readonly MethodInfo hashTableGetEnumerator = Typeob.SimpleHashtable.GetMethod("GetEnumerator", Type.EmptyTypes);
      internal static readonly MethodInfo hashtableGetItem = Typeob.SimpleHashtable.GetMethod("get_Item", new Type[]{Typeob.Object});
      internal static readonly MethodInfo hashtableRemove = Typeob.SimpleHashtable.GetMethod("Remove", new Type[1]{Typeob.Object});
      internal static readonly MethodInfo hashtableSetItem = Typeob.SimpleHashtable.GetMethod("set_Item", new Type[]{Typeob.Object,Typeob.Object});
      internal static readonly MethodInfo isMissingMethod = (typeof(Binding)).GetMethod("IsMissing");
      internal static readonly MethodInfo int32ToDecimalMethod = (typeof(Decimal)).GetMethod("op_Implicit", new Type[]{Typeob.Int32});
      internal static readonly MethodInfo int32ToStringMethod = (Typeob.Int32).GetMethod("ToString", new Type[]{});
      internal static readonly MethodInfo int64ToDecimalMethod = (typeof(Decimal)).GetMethod("op_Implicit", new Type[]{Typeob.Int64});
      internal static readonly MethodInfo int64ToStringMethod = (Typeob.Int64).GetMethod("ToString", new Type[]{});  
      internal static readonly MethodInfo jScriptCompareMethod = (typeof(Relational)).GetMethod("JScriptCompare");
      internal static readonly MethodInfo jScriptEvaluateMethod = (typeof(Eval)).GetMethod("JScriptEvaluate");
      internal static readonly MethodInfo jScriptEqualsMethod = (typeof(Equality)).GetMethod("JScriptEquals");
      internal static readonly MethodInfo jScriptGetEnumeratorMethod = (typeof(ForIn)).GetMethod("JScriptGetEnumerator");
      internal static readonly MethodInfo jScriptFunctionDeclarationMethod = (typeof(FunctionDeclaration)).GetMethod("JScriptFunctionDeclaration");
      internal static readonly MethodInfo jScriptFunctionExpressionMethod = (typeof(FunctionExpression)).GetMethod("JScriptFunctionExpression");
      internal static readonly MethodInfo jScriptInMethod = (typeof(In)).GetMethod("JScriptIn");
      internal static readonly MethodInfo jScriptImportMethod = (typeof(Import)).GetMethod("JScriptImport");
      internal static readonly MethodInfo jScriptInstanceofMethod = (typeof(Instanceof)).GetMethod("JScriptInstanceof");
      internal static readonly MethodInfo jScriptExceptionValueMethod = (typeof(Try)).GetMethod("JScriptExceptionValue");
      internal static readonly MethodInfo jScriptPackageMethod = (typeof(Package)).GetMethod("JScriptPackage");
      internal static readonly MethodInfo jScriptStrictEqualsMethod = (typeof(StrictEquality)).GetMethod("JScriptStrictEquals", new Type[]{Typeob.Object, Typeob.Object});
      internal static readonly MethodInfo jScriptThrowMethod = (typeof(Throw)).GetMethod("JScriptThrow");
      internal static readonly MethodInfo jScriptTypeofMethod = (typeof(Typeof)).GetMethod("JScriptTypeof");
      internal static readonly MethodInfo jScriptWithMethod = (typeof(With)).GetMethod("JScriptWith");
      internal static readonly ConstructorInfo jsFunctionAttributeConstructor = (Typeob.JSFunctionAttribute).GetConstructor(new Type[] {typeof(JSFunctionAttributeEnum)});
      internal static readonly ConstructorInfo jsLocalFieldConstructor = (typeof(JSLocalField)).GetConstructor(new Type[]{Typeob.String, typeof(RuntimeTypeHandle), Typeob.Int32});
      internal static readonly ConstructorInfo lateBindingConstructor = (typeof(LateBinding)).GetConstructor(new Type[]{Typeob.String});
      internal static readonly ConstructorInfo lateBindingConstructor2 = (typeof(LateBinding)).GetConstructor(new Type[]{Typeob.String, Typeob.Object});
      internal static readonly MethodInfo moveNextMethod = (typeof(IEnumerator)).GetMethod("MoveNext", Type.EmptyTypes);
      internal static readonly ConstructorInfo numericBinaryConstructor = (typeof(NumericBinary)).GetConstructor(new Type[]{Typeob.Int32});
      internal static readonly ConstructorInfo numericUnaryConstructor = (typeof(NumericUnary)).GetConstructor(new Type[]{Typeob.Int32});
      internal static readonly ConstructorInfo plusConstructor = (typeof(Plus)).GetConstructor(new Type[]{});
      internal static readonly MethodInfo plusDoOpMethod = (typeof(Plus)).GetMethod("DoOp");
      internal static readonly MethodInfo numericbinaryDoOpMethod = (typeof(NumericBinary)).GetMethod("DoOp");
      internal static readonly ConstructorInfo postOrPrefixConstructor = (typeof(PostOrPrefixOperator)).GetConstructor(new Type[]{Typeob.Int32});
      internal static readonly MethodInfo popScriptObjectMethod = (Typeob.VsaEngine).GetMethod("PopScriptObject");
      internal static readonly MethodInfo pushScriptObjectMethod = (Typeob.VsaEngine).GetMethod("PushScriptObject");
      internal static readonly MethodInfo pushStackFrameForMethod = (typeof(StackFrame)).GetMethod("PushStackFrameForMethod");
      internal static readonly MethodInfo pushStackFrameForStaticMethod = (typeof(StackFrame)).GetMethod("PushStackFrameForStaticMethod");
      internal static readonly ConstructorInfo referenceAttributeConstructor = (Typeob.ReferenceAttribute).GetConstructor(new Type[]{Typeob.String});
      internal static readonly MethodInfo regExpConstructMethod = (typeof(RegExpConstructor)).GetMethod("Construct", new Type[]{Typeob.String, Typeob.Boolean, Typeob.Boolean, Typeob.Boolean});
      internal static readonly ConstructorInfo relationalConstructor = (typeof(Relational)).GetConstructor(new Type[]{Typeob.Int32});
      internal static readonly ConstructorInfo returnOutOfFinallyConstructor = (typeof(ReturnOutOfFinally)).GetConstructor(new Type[]{});
      internal static readonly ConstructorInfo scriptExceptionConstructor = (typeof(JScriptException)).GetConstructor(new Type[]{typeof(JSError)});
      internal static readonly MethodInfo scriptObjectStackTopMethod = (Typeob.VsaEngine).GetMethod("ScriptObjectStackTop");
      internal static readonly MethodInfo setEngineMethod = (typeof(INeedEngine)).GetMethod("SetEngine");
      internal static readonly MethodInfo setFieldValueMethod = (typeof(FieldInfo)).GetMethod("SetValue", new Type[]{Typeob.Object, Typeob.Object});
      internal static readonly MethodInfo setIndexedPropertyValueStaticMethod = (typeof(LateBinding)).GetMethod("SetIndexedPropertyValueStatic");
      internal static readonly MethodInfo setMemberValue2Method = (typeof(JSObject)).GetMethod("SetMemberValue2", new Type[]{Typeob.String, Typeob.Object});
      internal static readonly MethodInfo setValueMethod = (typeof(LateBinding)).GetMethod("SetValue");
      internal static readonly MethodInfo stringConcat2Method = Typeob.String.GetMethod("Concat", new Type[]{Typeob.String, Typeob.String});
      internal static readonly MethodInfo stringConcat3Method = Typeob.String.GetMethod("Concat", new Type[]{Typeob.String, Typeob.String, Typeob.String});
      internal static readonly MethodInfo stringConcat4Method = Typeob.String.GetMethod("Concat", new Type[]{Typeob.String, Typeob.String, Typeob.String, Typeob.String});
      internal static readonly MethodInfo stringConcatArrMethod = Typeob.String.GetMethod("Concat", new Type[]{typeof(String[])});
      internal static readonly MethodInfo stringEqualsMethod = Typeob.String.GetMethod("Equals", new Type[]{Typeob.String, Typeob.String});
      internal static readonly MethodInfo stringLengthMethod = Typeob.String.GetProperty("Length").GetGetMethod();
      internal static readonly MethodInfo toBooleanMethod = (typeof(Convert)).GetMethod("ToBoolean", new Type[]{Typeob.Object, Typeob.Boolean});  
      internal static readonly MethodInfo toForInObjectMethod = (typeof(Convert)).GetMethod("ToForInObject", new Type[]{Typeob.Object, Typeob.VsaEngine});  
      internal static readonly MethodInfo toInt32Method = (typeof(Convert)).GetMethod("ToInt32", new Type[]{Typeob.Object});
      internal static readonly MethodInfo toNumberMethod = (typeof(Convert)).GetMethod("ToNumber", new Type[]{Typeob.Object});
      internal static readonly MethodInfo toNativeArrayMethod = (typeof(Convert)).GetMethod("ToNativeArray");
      internal static readonly MethodInfo toObjectMethod = (typeof(Convert)).GetMethod("ToObject", new Type[]{Typeob.Object, Typeob.VsaEngine});  
      internal static readonly MethodInfo toObject2Method = (typeof(Convert)).GetMethod("ToObject2", new Type[]{Typeob.Object, Typeob.VsaEngine});  
      internal static readonly MethodInfo toStringMethod = (typeof(Convert)).GetMethod("ToString", new Type[]{Typeob.Object, Typeob.Boolean});
      internal static readonly MethodInfo uint32ToDecimalMethod = (typeof(Decimal)).GetMethod("op_Implicit", new Type[]{typeof(UInt32)});
      internal static readonly MethodInfo uint32ToStringMethod = (typeof(UInt32)).GetMethod("ToString", new Type[]{});
      internal static readonly MethodInfo uint64ToDecimalMethod = (typeof(Decimal)).GetMethod("op_Implicit", new Type[]{typeof(UInt64)});
      internal static readonly MethodInfo uint64ToStringMethod = (typeof(UInt64)).GetMethod("ToString", new Type[]{});
      internal static readonly ConstructorInfo vsaEngineConstructor = (Typeob.VsaEngine).GetConstructor(new Type[]{});
      internal static readonly MethodInfo writeMethod = (typeof(ScriptStream)).GetMethod("Write");
      internal static readonly MethodInfo writeLineMethod = (typeof(ScriptStream)).GetMethod("WriteLine");
      internal static readonly MethodInfo throwTypeMismatch = (typeof(Convert)).GetMethod("ThrowTypeMismatch");
      internal static readonly ConstructorInfo compilerGlobalScopeAttributeCtor = (typeof(CompilerGlobalScopeAttribute)).GetConstructor(new Type[]{});
      
      private int idCount = 0;
      
      internal Stack BreakLabelStack = new Stack();
      internal Stack ContinueLabelStack = new Stack();
      internal bool InsideProtectedRegion = false;
      internal bool InsideFinally = false;
      internal int FinallyStackTop = 0;
      
      internal ModuleBuilder module;
      internal AssemblyBuilder assemblyBuilder;
      internal TypeBuilder classwriter;
      internal SimpleHashtable documents = new SimpleHashtable(8);
      internal SimpleHashtable usedNames = new SimpleHashtable(32);

      internal CompilerGlobals(String assemName, String assemblyFileName, PEFileKinds PEFileKind, bool save, bool run, bool debugOn, bool isCLSCompliant, Version version, Globals globals) {
        String moduleFileName = null;
        String directory = null; //Default has assembly stored in current directory
        
        if (assemblyFileName != null){
          //The directory is an absolute path where the assembly is written
          try{
            directory = Path.GetDirectoryName(Path.GetFullPath(assemblyFileName));
          }catch(Exception e){
            throw new VsaException(VsaError.AssemblyNameInvalid, assemblyFileName, e);
          }
          
          //For a single file assembly module filename is set to assembly filename
          moduleFileName = Path.GetFileName(assemblyFileName);
          
          //If simple name is not specified, get it by extracting the basename from assembly name
          if (null == assemName || String.Empty == assemName){
            assemName = Path.GetFileName(assemblyFileName);
            if (Path.HasExtension(assemName))
              assemName = assemName.Substring(0, assemName.Length - Path.GetExtension(assemName).Length);
          }
        }
        
        //Setup default simple assembly name and module name for the case where no assemblyFileName is specified.
        if (assemName == null || assemName == String.Empty)
          assemName = "JScriptAssembly";
        if (moduleFileName == null) {
          if (PEFileKind == PEFileKinds.Dll)
            moduleFileName = "JScriptModule.dll";
          else
            moduleFileName = "JScriptModule.exe";
        }
        
        AssemblyName assemblyName = new AssemblyName();
        assemblyName.CodeBase = assemblyFileName;
        if (globals.assemblyCulture != null) assemblyName.CultureInfo = globals.assemblyCulture;
        assemblyName.Flags = AssemblyNameFlags.None;
        if ((globals.assemblyFlags & AssemblyFlags.PublicKey) != 0) assemblyName.Flags = AssemblyNameFlags.PublicKey;
        switch ((AssemblyFlags)(globals.assemblyFlags & AssemblyFlags.CompatibilityMask)){
          case AssemblyFlags.NonSideBySideAppDomain:
            assemblyName.VersionCompatibility = AssemblyVersionCompatibility.SameDomain; break;
          case AssemblyFlags.NonSideBySideMachine:
            assemblyName.VersionCompatibility = AssemblyVersionCompatibility.SameMachine; break;
          case AssemblyFlags.NonSideBySideProcess:
            assemblyName.VersionCompatibility = AssemblyVersionCompatibility.SameProcess; break;
          default:
            assemblyName.VersionCompatibility = (AssemblyVersionCompatibility)0; break;
        }

        assemblyName.HashAlgorithm = globals.assemblyHashAlgorithm;
        if (globals.assemblyKeyFileName != null){
          try{
            FileStream fs = new FileStream(globals.assemblyKeyFileName, FileMode.Open, FileAccess.Read);
            StrongNameKeyPair keyPair = new StrongNameKeyPair(fs);
            if (globals.assemblyDelaySign)
              if (fs.Length == 160){
                Byte[] pkey = new Byte[160];
                fs.Seek(0, SeekOrigin.Begin);
                int len = fs.Read(pkey, 0, 160);
                assemblyName.SetPublicKey(pkey);
              }else
                assemblyName.SetPublicKey(keyPair.PublicKey);
            else
              assemblyName.KeyPair = keyPair;
          }catch(Exception){}
        }else if (globals.assemblyKeyName != null){
          try{
            assemblyName.KeyPair = new StrongNameKeyPair(globals.assemblyKeyName);
          }catch(Exception){}
        }

        assemblyName.Name = assemName;
        if (version != null) assemblyName.Version = version;
        else if (globals.assemblyVersion != null) assemblyName.Version = globals.assemblyVersion;

        AssemblyBuilderAccess access = save ? (run ? AssemblyBuilderAccess.RunAndSave : AssemblyBuilderAccess.Save) : AssemblyBuilderAccess.Run;
        // Supply the evidence to assemblies built for VSA
        if (globals.engine.genStartupClass)
          this.assemblyBuilder = Thread.GetDomain().DefineDynamicAssembly(assemblyName, access, directory, globals.engine.Evidence);
        else
          this.assemblyBuilder = Thread.GetDomain().DefineDynamicAssembly(assemblyName, access, directory);
        if (save)
          this.module = this.assemblyBuilder.DefineDynamicModule("JScript Module", moduleFileName, debugOn);
        else
          this.module = this.assemblyBuilder.DefineDynamicModule("JScript Module", debugOn);
        if (isCLSCompliant)
          this.module.SetCustomAttribute(new CustomAttributeBuilder(clsCompliantAttributeCtor, new Object[]{isCLSCompliant}));
        if (debugOn){
          ConstructorInfo debuggableAttr = typeof(DebuggableAttribute).GetConstructor(new Type[] {Typeob.Boolean, Typeob.Boolean});
          assemblyBuilder.SetCustomAttribute(new CustomAttributeBuilder(debuggableAttr, 
            new Object[] {(globals.assemblyFlags & AssemblyFlags.EnableJITcompileTracking) != 0, 
                          (globals.assemblyFlags & AssemblyFlags.DisableJITcompileOptimizer) != 0}));
        }
        this.classwriter = null;
      }
    
      internal String GetId(){
        return "JScript Id "+(this.idCount++).ToString();
      }
      
      internal String GetId(String name){
        return name+(this.idCount++).ToString();
      }

    }  

}
