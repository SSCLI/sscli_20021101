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
namespace Microsoft.JScript.Vsa
{

    using Microsoft.JScript;
    using Microsoft.Vsa;
    using System;
    using System.Collections;
    using System.IO;
    using System.Globalization;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Resources;
    using System.Text;
    using System.Threading;
    using System.Runtime.Remoting.Messaging;

    public class VsaEngine : BaseVsaEngine, IRedirectOutput{
      internal bool alwaysGenerateIL;
      private bool autoRef;
      private Hashtable Defines;
      internal bool doCRS;
      internal bool doFast;
      internal bool doPrint;
      internal bool doSaveAfterCompile;
      private bool doWarnAsError;
      private int nWarningLevel;
      internal bool genStartupClass;
      internal bool isCLSCompliant;
      internal bool versionSafe;
      private String PEFileName;
      internal PEFileKinds PEFileKind;
      private Version versionInfo;

      private CultureInfo errorCultureInfo;

    
      //In appdomain used to executing JScript debugger expression evaluator
      static internal bool executeForJSEE = false;
      
      private String libpath;
      private String[] libpathList;

      private bool isCompilerSet;

      internal VsaScriptScope globalScope;
      private ArrayList packages;
      private ArrayList scopes;
      private ArrayList implicitAssemblies;
      private SimpleHashtable implicitAssemblyCache;
      private ICollection managedResources;
      private string debugDirectory;
      private Random randomNumberGenerator;
      private string cachedPEFileName;
      internal int classCounter;

      private SimpleHashtable cachedTypeLookups;

      internal Thread runningThread;
      private CompilerGlobals compilerGlobals;
      private Globals globals;

      // Used only during VsaEngine.Compile. It is reset at the beginning of the 
      // function. It is incremented as errors are reported via OnCompilerError.
      private int numberOfErrors;

      private String runtimeDirectory;
      private static readonly Version CurrentProjectVersion = new Version("1.0");

      private Hashtable typenameTable; // for checking CLS compliance
      
      public VsaEngine()
        :this(true){
      }
      
      public VsaEngine(bool fast) : base("JScript", "7.0.3300.0", true){
        this.alwaysGenerateIL = false;
        this.autoRef = false;
        this.doCRS = false;
        this.doFast = fast;
        this.cachedPEFileName = "";
        this.genDebugInfo = false;
        this.genStartupClass = true;
        this.doPrint = false;
        this.doWarnAsError = false;
        this.nWarningLevel = 4;
        this.isCLSCompliant = false;
        this.versionSafe = false;
        this.PEFileName = null;
        this.PEFileKind = PEFileKinds.Dll;
        this.errorCultureInfo = null;
        this.libpath = null;
        this.libpathList = null;

        this.globalScope = null;
        this.vsaItems = new VsaItems(this);
        this.packages = null;
        this.scopes = null;
        this.classCounter = 0;
        this.implicitAssemblies = null;
        this.implicitAssemblyCache = null;
        this.cachedTypeLookups = null;

        this.isEngineRunning = false;
        this.isEngineCompiled = false;
        this.isCompilerSet = false;
        this.isClosed = false;

        this.runningThread = null;
        this.compilerGlobals = null;
        this.globals = null;
        this.runtimeDirectory = null;
        Globals.contextEngine = this;
        this.runtimeAssembly = null;
        this.typenameTable = null;
      }

      private Assembly runtimeAssembly;
      private static Hashtable assemblyReferencesTable = null;
      // This constructor is called at run time to instantiate an engine for a given assembly
      private VsaEngine(Assembly runtimeAssembly) : this(true){
        this.runtimeAssembly = runtimeAssembly;
      }

      
      internal void AddPackage(PackageScope pscope){
        if (this.packages == null)
          this.packages = new ArrayList(8);
        IEnumerator e = this.packages.GetEnumerator();
        while (e.MoveNext()){
          PackageScope cps = (PackageScope)e.Current;
          if (cps.name.Equals(pscope.name)){
            cps.owner.MergeWith(pscope.owner);
            return;
          }
        }
        this.packages.Add(pscope);
      }
      
      internal void CheckForErrors(){
        if (!this.isClosed && !this.isEngineCompiled){
          SetUpCompilerEnvironment();
          Globals.ScopeStack.Push(this.GetGlobalScope().GetObject());
          try{
            foreach (Object item in this.vsaItems){
              if (item is VsaReference)
                ((VsaReference)item).Compile(); //Load the assembly into memory. 
            }
            if (this.vsaItems.Count > 0) 
              this.SetEnclosingContext(new WrappedNamespace("", this)); //Provide a way to find types that are not inside of a name space
            foreach (Object item in this.vsaItems){
              if (!(item is VsaReference))
                ((VsaItem)item).CheckForErrors();
            }
            if (null != this.globalScope) 
              this.globalScope.CheckForErrors(); //In case the host added items to the global scope. 
          }finally{
            Globals.ScopeStack.Pop();
          }
        }
        this.globalScope = null;
      }

      public virtual IVsaEngine Clone(AppDomain domain){
        throw new NotImplementedException();
      }
      
      public virtual bool CompileEmpty(){
        this.TryObtainLock();
        try{
          return this.DoCompile();
        }finally{
          this.ReleaseLock();
        }
      }


      public virtual void ConnectEvents(){
      }

      internal CompilerGlobals CompilerGlobals{
        get{
          if (this.compilerGlobals == null)
            this.compilerGlobals = new CompilerGlobals(this.Name, this.PEFileName, this.PEFileKind, 
              this.doSaveAfterCompile || this.genStartupClass, !this.doSaveAfterCompile || this.genStartupClass, this.genDebugInfo, this.isCLSCompliant, 
              this.versionInfo, this.globals);
          return this.compilerGlobals;
        }
      }

      public static GlobalScope CreateEngineAndGetGlobalScope(bool fast, String[] assemblyNames){
        VsaEngine engine = new VsaEngine(fast);
        engine.InitVsaEngine("JScript.Vsa.VsaEngine://Microsoft.JScript.VsaEngine.Vsa", new DefaultVsaSite());
        engine.doPrint = true;
        engine.SetEnclosingContext(new WrappedNamespace("", engine));
        foreach (String assemblyName in assemblyNames){
          VsaReference r = (VsaReference)engine.vsaItems.CreateItem(assemblyName, VsaItemType.Reference, VsaItemFlag.None);
          r.AssemblyName = assemblyName;
        }
        VsaEngine.exeEngine = engine;
        GlobalScope scope = (GlobalScope)engine.GetGlobalScope().GetObject();
        scope.globalObject = engine.Globals.globalObject;
        return scope;
      }

      public static GlobalScope CreateEngineAndGetGlobalScopeWithType(bool fast, String[] assemblyNames, RuntimeTypeHandle callingTypeHandle){
        VsaEngine engine = new VsaEngine(fast);
        engine.InitVsaEngine("JScript.Vsa.VsaEngine://Microsoft.JScript.VsaEngine.Vsa", new DefaultVsaSite());
        engine.doPrint = true;
        engine.SetEnclosingContext(new WrappedNamespace("", engine));
        foreach (String assemblyName in assemblyNames){
          VsaReference r = (VsaReference)engine.vsaItems.CreateItem(assemblyName, VsaItemType.Reference, VsaItemFlag.None);
          r.AssemblyName = assemblyName;
        }
        // Put the engine in the CallContext so that calls to CreateEngineWithType will return this engine
        Type callingType = Type.GetTypeFromHandle(callingTypeHandle);
        Assembly callingAssembly = callingType.Assembly;
        System.Runtime.Remoting.Messaging.CallContext.SetData("JScript:" + callingAssembly.FullName, engine);
        // Get and return the global scope
        GlobalScope scope = (GlobalScope)engine.GetGlobalScope().GetObject();
        scope.globalObject = engine.Globals.globalObject;
        return scope;
      }

      private static volatile VsaEngine exeEngine;    // Instance of VsaEngine used by JScript EXEs

      // This factory method is called by EXE code only
      public static VsaEngine CreateEngine(){
        if (VsaEngine.exeEngine == null){
          VsaEngine e = new VsaEngine(true);
          e.InitVsaEngine("JScript.Vsa.VsaEngine://Microsoft.JScript.VsaEngine.Vsa", new DefaultVsaSite());
          VsaEngine.exeEngine = e;
        }
        return VsaEngine.exeEngine;
      }

      internal static VsaEngine CreateEngineForDebugger(){
        VsaEngine engine = new VsaEngine(true);
        engine.InitVsaEngine("JScript.Vsa.VsaEngine://Microsoft.JScript.VsaEngine.Vsa", new DefaultVsaSite());
        GlobalScope scope = (GlobalScope)engine.GetGlobalScope().GetObject();
        scope.globalObject = engine.Globals.globalObject;
        return engine;
      }

      // This factory method is called by DLL code only
      public static VsaEngine CreateEngineWithType(RuntimeTypeHandle callingTypeHandle){
        Type callingType = Type.GetTypeFromHandle(callingTypeHandle);
        Assembly callingAssembly = callingType.Assembly;
        Object o = System.Runtime.Remoting.Messaging.CallContext.GetData("JScript:" + callingAssembly.FullName);
        if (o != null){
          VsaEngine e = o as VsaEngine;
          if (e != null)
            return e;
        }

        VsaEngine engine = new VsaEngine(callingAssembly);
        engine.InitVsaEngine("JScript.Vsa.VsaEngine://Microsoft.JScript.VsaEngine.Vsa", new DefaultVsaSite());

        GlobalScope scope = (GlobalScope)engine.GetGlobalScope().GetObject();
        scope.globalObject = engine.Globals.globalObject;

        // for every global class generated in this assembly make an instance and call the global code method
        int i = 0;
        Type globalClassType = null;
        do{
          String globalClassName = "JScript " + i.ToString();
          globalClassType = callingAssembly.GetType(globalClassName, false);
          if (globalClassType != null){
            engine.SetEnclosingContext(new WrappedNamespace("", engine));
            ConstructorInfo globalScopeConstructor = globalClassType.GetConstructor(new Type[]{typeof(GlobalScope)});
            MethodInfo globalCode = globalClassType.GetMethod("Global Code");
            Object globalClassInstance = globalScopeConstructor.Invoke(new Object[]{scope});
            globalCode.Invoke(globalClassInstance, new Object[0]);
          }
          i++;
        }while (globalClassType != null);

        if (o == null)
          System.Runtime.Remoting.Messaging.CallContext.SetData("JScript:" + callingAssembly.FullName, engine);
        return engine;
      }

      private void AddReferences(){
        if (VsaEngine.assemblyReferencesTable == null) {
          // Create the cache
          Hashtable h = new Hashtable();
          VsaEngine.assemblyReferencesTable = Hashtable.Synchronized(h);
        }

        String[] references = VsaEngine.assemblyReferencesTable[this.runtimeAssembly.FullName] as String[];
        if (references != null){
          for (int i = 0; i < references.Length; i++){
            VsaReference r = (VsaReference)this.vsaItems.CreateItem(references[i], VsaItemType.Reference, VsaItemFlag.None);
            r.AssemblyName = references[i];
          }
        }else{
          // Read the references from the custom attribute on the assembly and create VsaReferences for each
          // of them.
          Object[] attrs = this.runtimeAssembly.GetCustomAttributes(Typeob.ReferenceAttribute, false);
          String[] references1 = new String[attrs.Length];
          for (int i = 0; i < attrs.Length; i++){
            String assemblyName = ((ReferenceAttribute)attrs[i]).reference;
            VsaReference r = (VsaReference)this.vsaItems.CreateItem(assemblyName, VsaItemType.Reference, VsaItemFlag.None);
            r.AssemblyName = assemblyName;
            references1[i] = assemblyName;
          }
          VsaEngine.assemblyReferencesTable[this.runtimeAssembly.FullName] = references1;
        }
      }

      private void EmitReferences() 
      {
        SimpleHashtable emitted = new SimpleHashtable((uint)this.vsaItems.Count + (this.implicitAssemblies == null ? 0 : (uint)this.implicitAssemblies.Count));
        foreach (Object item in this.vsaItems){
          if (item is VsaReference){
            String referenceName = ((VsaReference)item).Assembly.GetName().FullName;
            // do not write duplicate assemblies
            if (emitted[referenceName] == null){
              CustomAttributeBuilder cab = new CustomAttributeBuilder(CompilerGlobals.referenceAttributeConstructor, new Object[1] {referenceName});
              this.CompilerGlobals.assemblyBuilder.SetCustomAttribute(cab);
              emitted[referenceName] = item;
            }
          }
        }
        if (this.implicitAssemblies != null){
          foreach (Object item in this.implicitAssemblies){
            Assembly a = item as Assembly;
            if (a != null){
              String referenceName = a.GetName().FullName;
              // do not write duplicate assemblies
              if (emitted[referenceName] == null){
                CustomAttributeBuilder cab = new CustomAttributeBuilder(CompilerGlobals.referenceAttributeConstructor, new Object[1] {referenceName});
                this.CompilerGlobals.assemblyBuilder.SetCustomAttribute(cab);
                emitted[referenceName] = item;
              }
            }
          }
        }
      }

      private void CreateMain(){
        // define a class that will hold the main method
        TypeBuilder mainClass = this.CompilerGlobals.module.DefineType("JScript Main", TypeAttributes.Public);
      
        // create a function with the following signature void Main(String[] args)
        MethodBuilder main = mainClass.DefineMethod("Main", MethodAttributes.Public | MethodAttributes.Static, Typeob.Void, new Type[]{typeof(String[])});
        ILGenerator il = main.GetILGenerator();

        // emit code for main method
        this.CreateEntryPointIL(il, null /* site */);

        // cook method and class
        mainClass.CreateType();
        // define the Main() method as the entry point for the exe
        this.CompilerGlobals.assemblyBuilder.SetEntryPoint(main, this.PEFileKind);
      }

      private void CreateStartupClass(){
        // define _Startup class for VSA (in the RootNamespace)
        Debug.Assert(this.rootNamespace != null && this.rootNamespace.Length > 0);
        TypeBuilder startupClass = this.CompilerGlobals.module.DefineType(this.rootNamespace + "._Startup", TypeAttributes.Public, typeof(BaseVsaStartup));
        FieldInfo site = typeof(BaseVsaStartup).GetField("site", BindingFlags.NonPublic | BindingFlags.Instance);
        // create a function with the following signature: public virtual void Startup()
        MethodBuilder startup = startupClass.DefineMethod("Startup", MethodAttributes.Public | MethodAttributes.Virtual, Typeob.Void, Type.EmptyTypes);
        this.CreateEntryPointIL(startup.GetILGenerator(), site, startupClass);
        // create a function with the following signature: public virtual void Shutdown()
        MethodBuilder shutdown = startupClass.DefineMethod("Shutdown", MethodAttributes.Public | MethodAttributes.Virtual, Typeob.Void, Type.EmptyTypes);
        this.CreateShutdownIL(shutdown.GetILGenerator());

        // cook method and class
        startupClass.CreateType();
      }

      void CreateEntryPointIL(ILGenerator il, FieldInfo site){
        this.CreateEntryPointIL(il, site, null);
      }

      void CreateEntryPointIL(ILGenerator il, FieldInfo site, TypeBuilder startupClass){
        LocalBuilder globalScope = il.DeclareLocal(typeof(GlobalScope));

        //Emit code to create an engine. We do this explicitly so that we can control fast mode.
        if (this.doFast)
          il.Emit(OpCodes.Ldc_I4_1);
        else
          il.Emit(OpCodes.Ldc_I4_0);
        //Run through the list of references and emit code to create an array of strings representing them
        //but do not emit duplicates.
        SimpleHashtable uniqueReferences = new SimpleHashtable((uint)this.vsaItems.Count);
        ArrayList references = new ArrayList();
        foreach (Object item in this.vsaItems){
          if (item is VsaReference){
            string assemblyName = ((VsaReference)item).Assembly.GetName().FullName;
            if (uniqueReferences[assemblyName] == null){
              references.Add(assemblyName);
              uniqueReferences[assemblyName] = item;
            }
          }
        }
        if (this.implicitAssemblies != null){
          foreach (Object item in this.implicitAssemblies){
            Assembly a = item as Assembly;
            if (a != null){
              String assemblyName = a.GetName().FullName;
              if (uniqueReferences[assemblyName] == null){
                references.Add(assemblyName);
                uniqueReferences[assemblyName] = item;
              }
            }
          }
        }

        ConstantWrapper.TranslateToILInt(il, references.Count);
        il.Emit(OpCodes.Newarr, Typeob.String);
        int num = 0;
        foreach (string referenceName in references){
          il.Emit(OpCodes.Dup);
          ConstantWrapper.TranslateToILInt(il, num++);
          il.Emit(OpCodes.Ldstr, referenceName);
          il.Emit(OpCodes.Stelem_Ref);
        }
        if (startupClass != null){
          MethodInfo createEngineAndGetGlobalScopeWithType = this.GetType().GetMethod("CreateEngineAndGetGlobalScopeWithType");
          il.Emit(OpCodes.Ldtoken, startupClass);
          il.Emit(OpCodes.Call, createEngineAndGetGlobalScopeWithType);
        }else{
          MethodInfo createEngineAndGetGlobalScope = this.GetType().GetMethod("CreateEngineAndGetGlobalScope");
          il.Emit(OpCodes.Call, createEngineAndGetGlobalScope);
        }
        il.Emit(OpCodes.Stloc, globalScope);

        // get global object instances and event source instances (CreateStartupClass scenario only)
        if (site != null) this.CreateHostCallbackIL(il, site);

        bool setUserEntryPoint = this.genDebugInfo;
        // for every generated class make an instance and call the main routine method
        foreach (Object item in this.vsaItems){
          Type compiledType = ((VsaItem)item).GetCompiledType();
          if (null != compiledType){
            ConstructorInfo globalScopeConstructor = compiledType.GetConstructor(new Type[]{typeof(GlobalScope)});
            MethodInfo globalCode = compiledType.GetMethod("Global Code");
            if (setUserEntryPoint){
              //Set the Global Code method of the first code item to be the place where the debugger breaks for the first step into
              this.CompilerGlobals.module.SetUserEntryPoint(globalCode);
              setUserEntryPoint = false; //Do it once only
            }
            il.Emit(OpCodes.Ldloc, globalScope);
            il.Emit(OpCodes.Newobj, globalScopeConstructor);
            il.Emit(OpCodes.Call, globalCode);
            il.Emit(OpCodes.Pop);
          }
        }
        // a method needs a return opcode
        il.Emit(OpCodes.Ret);
      }

      private void CreateHostCallbackIL(ILGenerator il, FieldInfo site){
        // Do callbacks to the host for global object instances
        MethodInfo getGlobalInstance = site.FieldType.GetMethod("GetGlobalInstance");
        MethodInfo getEventSourceInstance = site.FieldType.GetMethod("GetEventSourceInstance");
        foreach (Object item in this.vsaItems){
          if (item is VsaHostObject){
            VsaHostObject hostObject = (VsaHostObject)item;
            // get global item instance from site
            il.Emit(OpCodes.Ldarg_0);
            il.Emit(OpCodes.Ldfld, site);
            il.Emit(OpCodes.Ldstr, hostObject.Name);
            il.Emit(OpCodes.Callvirt, getGlobalInstance);
            // cast instance to the correct type and store into the global field
            Type target_type = hostObject.Field.FieldType;
            il.Emit(OpCodes.Ldtoken, target_type);
            il.Emit(OpCodes.Call, CompilerGlobals.getTypeFromHandleMethod);
            ConstantWrapper.TranslateToILInt(il, 0);
            il.Emit(OpCodes.Call, CompilerGlobals.coerceTMethod);
            if (target_type.IsValueType)
              Microsoft.JScript.Convert.EmitUnbox(il, target_type, Type.GetTypeCode(target_type));
            else
              il.Emit(OpCodes.Castclass, target_type);
            il.Emit(OpCodes.Stsfld, hostObject.Field);
          }
        }
      }

      private void CreateShutdownIL(ILGenerator il){
        // release references to global instances
        foreach (Object item in this.vsaItems){
          if (item is VsaHostObject){
            il.Emit(OpCodes.Ldnull); 
            il.Emit(OpCodes.Stsfld, ((VsaHostObject)item).Field);
          }
        }
        il.Emit(OpCodes.Ret);
      }

      private void DeleteCachedCompiledState(){
        if (this.cachedPEFileName.Length > 0){
          try{
            if (File.Exists(this.cachedPEFileName))
              File.Delete(this.cachedPEFileName);
            string debugFileName = Path.ChangeExtension(this.cachedPEFileName, ".ildb");
            if (File.Exists(debugFileName))
              File.Delete(debugFileName);
          }catch(Exception){
            // swallow exceptions
          }
          this.cachedPEFileName = "";
        }
      }

      public virtual void DisconnectEvents(){
      }

      protected override void DoClose(){
        ((VsaItems)this.vsaItems).Close();
        if (null != this.globalScope)
          this.globalScope.Close();
        this.vsaItems = null;
        this.engineSite = null;
        this.globalScope = null;
        this.runningThread = null;
        this.compilerGlobals = null;
        this.globals = null;
        ScriptStream.Out = Console.Out;
        ScriptStream.Error = Console.Error;
        this.DeleteCachedCompiledState();
        this.isClosed = true;

        // The following calls ensure that all pointers to unmanaged objects get properly released
        // and avoid memory leaks on shutdown. GC.Collect() should be enough by itself when COM+
        // takes care of the second automatically.
        GC.Collect();
      }

      protected override bool DoCompile(){
        if (!this.isClosed && !this.isEngineCompiled){
          this.SetUpCompilerEnvironment();
          if (this.PEFileName == null){
            // we use random default names to avoid overwriting cached assembly files when debugging VSA
            this.PEFileName = this.GenerateRandomPEFileName();
          }
          this.SaveSourceForDebugging();  // Save sources needed for debugging (does nothing if no debug info)
          this.numberOfErrors = 0;        // Records number of errors during compilation.
          this.isEngineCompiled = true;         // OnCompilerError sets to false if it encounters an unrecoverable error.
          Globals.ScopeStack.Push(this.GetGlobalScope().GetObject());
          try{
            try{
              foreach (Object item in this.vsaItems){
                Debug.Assert(item is VsaReference || item is VsaStaticCode || item is VsaHostObject);
                if (item is VsaReference)
                  ((VsaReference)item).Compile(); //Load the assembly into memory.
              }
              if (this.vsaItems.Count > 0)
                this.SetEnclosingContext(new WrappedNamespace("", this)); //Provide a way to find types that are not inside of a name space
              // Add VSA global items to the global scope
              foreach (Object item in this.vsaItems){
                if (item is VsaHostObject)
                  ((VsaHostObject)item).Compile();
              }
              foreach (Object item in this.vsaItems){
                if (item is VsaStaticCode)
                  ((VsaStaticCode)item).Parse();
              }
              foreach (Object item in this.vsaItems){
                if (item is VsaStaticCode)
                  ((VsaStaticCode)item).ProcessAssemblyAttributeLists();
              }
              foreach (Object item in this.vsaItems){
                if (item is VsaStaticCode)
                  ((VsaStaticCode)item).PartiallyEvaluate();
              }
              foreach (Object item in this.vsaItems){
                if (item is VsaStaticCode)
                  ((VsaStaticCode)item).TranslateToIL();
              }
              foreach (Object item in this.vsaItems){
                if (item is VsaStaticCode)
                  ((VsaStaticCode)item).GetCompiledType();
              }
              if (null != this.globalScope)
                this.globalScope.Compile(); //In case the host added items to the global scope
            }catch(JScriptException se){
              // It's a bit strange because we may be capturing an exception 
              // thrown by VsaEngine.OnCompilerError (in the case where 
              // this.engineSite is null. This is fine though. All we end up doing
              // is capturing and then rethrowing the same error.
              this.OnCompilerError(se);
            }catch(System.IO.FileLoadException e){
              JScriptException se = new JScriptException(JSError.ImplicitlyReferencedAssemblyNotFound);
              se.value = e.FileName;
              this.OnCompilerError(se);
              this.isEngineCompiled = false;
            }catch(EndOfFile){
              // an error was reported during PartiallyEvaluate and the host decided to abort
              // swallow the exception and keep going
            }catch(Exception){
              // internal compiler error -- make sure we don't claim to have compiled, then rethrow
              this.isEngineCompiled = false;
              throw;
            }
          }finally{
            Globals.ScopeStack.Pop();
          }
          if (this.isEngineCompiled){
            // there were no unrecoverable errors, but we want to return true only if there is IL
            this.isEngineCompiled = (this.numberOfErrors == 0 || this.alwaysGenerateIL);
          }
        }

        if (this.managedResources != null){
          foreach (ResInfo managedResource in this.managedResources){
            if (managedResource.isLinked){
              this.CompilerGlobals.assemblyBuilder.AddResourceFile(managedResource.name,
                                                              Path.GetFileName(managedResource.filename),
                                                              managedResource.isPublic?
                                                              ResourceAttributes.Public:
                                                              ResourceAttributes.Private);
            }else{
              ResourceReader reader = null;
              try{
                reader = new ResourceReader(managedResource.filename);
              }catch(System.ArgumentException){
                JScriptException se = new JScriptException(JSError.InvalidResource);
                se.value = managedResource.filename;
                this.OnCompilerError(se);
                this.isEngineCompiled = false;
                return false;
              }
              IResourceWriter writer = this.CompilerGlobals.module.DefineResource(managedResource.name,
                                                                                  managedResource.filename,
                                                                                  managedResource.isPublic?
                                                                                  ResourceAttributes.Public:
                                                                                  ResourceAttributes.Private);
              foreach (DictionaryEntry resource in reader)
                writer.AddResource((string)resource.Key, resource.Value);
              reader.Close();
            }
          }
        }

        if (this.isEngineCompiled)
          this.EmitReferences();

        // Save things out to a local PE file when doSaveAfterCompile is set; this is set when an
        // output name is given (allows JSC to avoid IVsaEngine.SaveCompiledState).  The proper
        // values for VSA are doSaveAfterCompile == false and genStartupClass == true.  We allow
        // genStartupClass to be false for scenarios like JSTest and the Debugger
        if (this.isEngineCompiled){
          if (this.doSaveAfterCompile){
            if (this.PEFileKind != PEFileKinds.Dll)
              this.CreateMain();
            try {
              // After executing this code path, it is an error to call SaveCompiledState (until the engine is recompiled)
              compilerGlobals.assemblyBuilder.Save(Path.GetFileName(this.PEFileName));
            }catch(Exception e){
              throw new VsaException(VsaError.SaveCompiledStateFailed, e.Message, e);
            }
          }else if (this.genStartupClass){
            // this is generated for VSA hosting
            this.CreateStartupClass();
          }
        }
        return this.isEngineCompiled;
      }

      internal CultureInfo ErrorCultureInfo{
        get{
          if (this.errorCultureInfo == null || this.errorCultureInfo.LCID != this.errorLocale)
            this.errorCultureInfo = new CultureInfo(this.errorLocale);
          return this.errorCultureInfo;
        }
      }

      private string GenerateRandomPEFileName(){
        if (this.randomNumberGenerator == null)
          this.randomNumberGenerator = new Random((int)System.DateTime.Now.ToFileTime());
        string filename = "tmp" + this.randomNumberGenerator.Next() + (this.PEFileKind == PEFileKinds.Dll? ".dll": ".exe");
        return System.IO.Path.GetTempPath() + filename;
      }

      public virtual Assembly GetAssembly(){
        this.TryObtainLock();
        try{
          if (null != this.PEFileName)
            return Assembly.LoadFrom(this.PEFileName);
          else
            return compilerGlobals.assemblyBuilder;
        }finally{
          this.ReleaseLock();
        }
      }

      internal ClassScope GetClass(String className){
        if (this.packages != null)
          for (int i = 0, n = this.packages.Count; i < n; i++){
            PackageScope pscope = (PackageScope)this.packages[i];
            Object pval = pscope.GetMemberValue(className, 1);
            if (!(pval is Microsoft.JScript.Missing)){
              ClassScope csc = (ClassScope)pval;
              return csc;
            }
          }
        return null;
      }




      public virtual IVsaScriptScope GetGlobalScope(){
        if (null == this.globalScope){
          this.globalScope = new VsaScriptScope(this, "Global", null);
          GlobalScope scope = (GlobalScope)this.globalScope.GetObject();
          scope.globalObject = this.Globals.globalObject;
          scope.fast = this.doFast;
          scope.isKnownAtCompileTime = this.doFast;
        }
        return this.globalScope;
      }

      // Called by the debugger to get hold of the global scope from within a class method
      public virtual GlobalScope GetMainScope(){
        ScriptObject o = ScriptObjectStackTop();
        while (o != null && !(o is GlobalScope))
          o = o.GetParent();
        return (GlobalScope)o;
      }

      public virtual Module GetModule(){
        if (null != this.PEFileName){
          Assembly a = GetAssembly();
          Module[] modules = a.GetModules();
          return modules[0];
        }else
          return this.CompilerGlobals.module;
      }

      public ArrayConstructor GetOriginalArrayConstructor(){
        return this.Globals.globalObject.originalArray;
      }

      public ObjectConstructor GetOriginalObjectConstructor(){
        return this.Globals.globalObject.originalObject;
      }
      
      public RegExpConstructor GetOriginalRegExpConstructor(){
        return this.Globals.globalObject.originalRegExp;
      }
      
      protected override Object GetCustomOption(String name){
        if (String.Compare(name, "CLSCompliant", true, CultureInfo.InvariantCulture) == 0)
          return this.isCLSCompliant;
        else if (String.Compare(name, "fast", true, CultureInfo.InvariantCulture) == 0)
          return this.doFast;
        else if (String.Compare(name, "output", true, CultureInfo.InvariantCulture) == 0)
          return this.PEFileName;
        else if (String.Compare(name, "PEFileKind", true, CultureInfo.InvariantCulture) == 0)
          return this.PEFileKind;
        else if (String.Compare(name, "print", true, CultureInfo.InvariantCulture) == 0)
          return this.doPrint;
        else if (String.Compare(name, "UseContextRelativeStatics", true, CultureInfo.InvariantCulture) == 0)
          return this.doCRS;
        // the next two are needed because of the ICompiler interface. They should not fail even though they may not do anything
        else if (String.Compare(name, "optimize", true, CultureInfo.InvariantCulture) == 0)
          return null;
        else if (String.Compare(name, "define", true, CultureInfo.InvariantCulture) == 0)
          return null;
        else if (String.Compare(name, "defines", true, CultureInfo.InvariantCulture) == 0)
          return this.Defines;
        else if (String.Compare(name, "ee", true, CultureInfo.InvariantCulture) == 0)
          return VsaEngine.executeForJSEE;
        else if (String.Compare(name, "version", true, CultureInfo.InvariantCulture) == 0)
          return this.versionInfo;
        else if (String.Compare(name, "VersionSafe", true, CultureInfo.InvariantCulture) == 0)
          return this.versionSafe;
        else if (String.Compare(name, "warnaserror", true, CultureInfo.InvariantCulture) == 0)
          return this.doWarnAsError;
        else if (String.Compare(name, "WarningLevel", true, CultureInfo.InvariantCulture) == 0)
          return this.nWarningLevel;
        else if (String.Compare(name, "managedResources", true, CultureInfo.InvariantCulture) == 0)
          return this.managedResources;
        else if (String.Compare(name, "alwaysGenerateIL", true, CultureInfo.InvariantCulture) == 0)
          return this.alwaysGenerateIL;
        else if (String.Compare(name, "DebugDirectory", true, CultureInfo.InvariantCulture) == 0)
          return this.debugDirectory;
        else if (String.Compare(name, "AutoRef", true, CultureInfo.InvariantCulture) == 0)
          return this.autoRef;
        else
          throw new VsaException(VsaError.OptionNotSupported);
      }

      internal int GetStaticCodeBlockCount(){
        return ((VsaItems)this.vsaItems).staticCodeBlockCount;
      }

      internal Type GetType(String typeName){
        if (this.cachedTypeLookups == null)
          this.cachedTypeLookups = new SimpleHashtable(1000);
        object cacheResult = this.cachedTypeLookups[typeName];
        if (cacheResult == null){
          // proceed with lookup
          for (int i = 0, n = this.Scopes.Count; i < n; i++){
            GlobalScope scope = (GlobalScope)this.scopes[i];
            Type result = scope.GetType().Module.Assembly.GetType(typeName, false);
            if (result != null){
              this.cachedTypeLookups[typeName] = result;
              return result;
            }
          }

          if (this.runtimeAssembly != null) {
            this.AddReferences();
            this.runtimeAssembly = null;
          }

          for (int i = 0, n = this.vsaItems.Count; i < n; i++){
            object item = this.vsaItems[i];
            if (item is VsaReference){
              Type result = ((VsaReference)item).GetType(typeName);
              if (result != null){
                this.cachedTypeLookups[typeName] = result;
                return result;
              }
            }
          }
          if (this.implicitAssemblies == null){
            this.cachedTypeLookups[typeName] = false;
            return null;
          }
          for (int i = 0, n = this.implicitAssemblies.Count; i < n; i++){
            Assembly assembly = (Assembly)this.implicitAssemblies[i];
            Type result = assembly.GetType(typeName, false);
            if (result != null){
              if (!result.IsPublic || result.IsDefined(typeof(System.Runtime.CompilerServices.RequiredAttributeAttribute), true))
                result = null; //Suppress the type if it is not public or if it is a funky C++ type.
              else{
                this.cachedTypeLookups[typeName] = result;
                return result;
              }
            }
          }
          this.cachedTypeLookups[typeName] = false;
          return null;
        }
        return (cacheResult as Type);
      }

      internal Globals Globals{
        get{
          if (this.globals == null)
            this.globals = new Globals(this.doFast, this);
          return this.globals;
        }
      }

      internal bool HasErrors{
        get{
          return this.numberOfErrors != 0;
        }
      }

      // GetScannerInstance is used by IsValidNamespaceName and IsValidIdentifier to validate names.
      // We return an instance of the scanner only if there is no whitespace in the name text, since
      // we do not want to allow, say, "not. valid" to be a valid namespace name even though that
      // would produce a valid sequence of tokens.
      private JSScanner GetScannerInstance(string name){
        // make sure there's no whitespace in the name (values copied from documentation on String.Trim())
        char[] anyWhiteSpace = {
          (char)0x0009, (char)0x000A, (char)0x000B, (char)0x000C, (char)0x000D, (char)0x0020, (char)0x00A0,
          (char)0x2000, (char)0x2001, (char)0x2002, (char)0x2003, (char)0x2004, (char)0x2005, (char)0x2006,
          (char)0x2007, (char)0x2008, (char)0x2009, (char)0x200A, (char)0x200B, (char)0x3000, (char)0xFEFF
        };
        if (name == null || name.IndexOfAny(anyWhiteSpace) > -1)
          return null;
        // Create a code item whose source is the given text
        VsaItem item = new VsaStaticCode(this, "itemName", VsaItemFlag.None);
        Context context = new Context(new DocumentContext(item), name);
        context.errorReported = -1;
        JSScanner scanner = new JSScanner(); //Use this constructor to avoid allocating a Globals instance
        scanner.SetSource(context);
        return scanner;
      }

      // Use this method to initialize the engine for non-VSA use.  This includes JSC, JSTest, and the JSEE.
      public void InitVsaEngine(string rootMoniker, IVsaSite site){
        this.genStartupClass = false;
        this.engineMoniker = rootMoniker;
        this.engineSite = site;
        this.isEngineInitialized = true;
        this.rootNamespace = "JScript.DefaultNamespace";
        this.isEngineDirty = true;
        this.isEngineCompiled = false;
      }

      public virtual void Interrupt(){
        if (this.runningThread != null){
          this.runningThread.Abort();    
          this.runningThread = null;
        }
      }

      protected override bool IsValidNamespaceName(string name){
        JSScanner scanner = this.GetScannerInstance(name);
        if (scanner == null)
          return false;
        while(true){
          if (scanner.PeekToken() != JSToken.Identifier)
            return false;
          scanner.GetNextToken();
          if (scanner.PeekToken() == JSToken.EndOfFile)
            break;
          if (scanner.PeekToken() != JSToken.AccessField)
            return false;
          scanner.GetNextToken();
        }
        return true;
      }

      public override bool IsValidIdentifier(string ident){
        JSScanner scanner = this.GetScannerInstance(ident);
        if (scanner == null)
          return false;
        if (scanner.PeekToken() != JSToken.Identifier)
          return false;
        scanner.GetNextToken();
        if (scanner.PeekToken() != JSToken.EndOfFile)
            return false;
        return true;
      }

      public LenientGlobalObject LenientGlobalObject{
        get{
          return (LenientGlobalObject)this.Globals.globalObject;
        }
      }

      protected override Assembly LoadCompiledState(){
        Debug.Assert(this.haveCompiledState);
        byte[] pe;
        byte[] pdb;
        if (!this.genDebugInfo)
          return this.compilerGlobals.assemblyBuilder;
        // we need to save/reload to properly associate debug symbols with the assembly
        this.DoSaveCompiledState(out pe, out pdb);
        return Assembly.Load(pe, pdb, this.executionEvidence);
      }

      protected override void DoLoadSourceState(IVsaPersistSite site){
      }


      internal bool OnCompilerError(JScriptException se){
        if (se.Severity == 0 || (this.doWarnAsError && se.Severity <= this.nWarningLevel))
          this.numberOfErrors++;
        bool canRecover = this.engineSite.OnCompilerError(se); //true means carry on with compilation.
        if (!canRecover)
          this.isEngineCompiled = false;
        return canRecover;
      }

      public ScriptObject PopScriptObject(){
        return (ScriptObject)this.Globals.ScopeStack.Pop();
      }
      
      public void PushScriptObject(ScriptObject obj){
        this.Globals.ScopeStack.Push(obj);
      }
      
      public virtual void RegisterEventSource(String name){
      }

      public override void Reset(){
        if (this.genStartupClass)
          base.Reset();
        else
          this.ResetCompiledState();
      }

      protected override void ResetCompiledState(){
        if (this.globalScope != null){
          this.globalScope.Reset();
          this.globalScope = null;
        }
        this.classCounter = 0;
        this.haveCompiledState = false;
        this.failedCompilation = true;
        this.compiledRootNamespace = null;
        this.startupClass = null;
        this.compilerGlobals = null;
        this.globals = null;
        foreach (Object item in this.vsaItems)
          ((VsaItem)item).Reset();
        this.implicitAssemblies = null;
        this.implicitAssemblyCache = null;
        this.cachedTypeLookups = null;
        this.isEngineCompiled = false;
        this.isEngineRunning = false;
        this.isCompilerSet = false;
        this.packages = null;
        this.DeleteCachedCompiledState();
      }

      // the debugger restart the engine to run different expression evaluation
      public virtual void Restart(){
        this.TryObtainLock();
        try{
          ((VsaItems)this.vsaItems).Close();
          if (null != this.globalScope)
            this.globalScope.Close();
          this.globalScope = null;
          this.vsaItems = new VsaItems(this);
          this.isEngineRunning = false;
          this.isEngineCompiled = false;
          this.isCompilerSet = false;
          this.isClosed = false;
          this.runningThread = null;
          this.globals = null;

          // These are the same as the calls in Close(). Ideally, they should be necessary only in Close(),
          // but actually the debugger leaks memory on shutdown if they are not present at both places.
          GC.Collect();
        }finally{
          this.ReleaseLock();
        }
      }

      public virtual void RunEmpty(){
        this.TryObtainLock();
        try{
          Preconditions(Pre.EngineNotClosed | Pre.RootMonikerSet | Pre.SiteSet);
          this.isEngineRunning = true;
          // save the current thread so it can be interrupted
          this.runningThread = Thread.CurrentThread;
          if (null != this.globalScope)
            this.globalScope.Run();
          foreach (Object item in this.vsaItems)
            ((VsaItem)item).Run();
        }finally{
          this.runningThread = null;
          this.ReleaseLock();
        }
      }

      public virtual void Run(AppDomain domain){
        // managed engines are not supporting Run in user-provided AppDomains
        throw new System.NotImplementedException();
      }

      protected override void DoSaveCompiledState(out byte[] pe, out byte[] pdb){
        pe = null;
        pdb = null;
        try{
          // Save things out to a local PE file, then read back into memory
          // PEFileName was set in the Compile method and we must have compiled in order to be calling this
          string tempAssemblyName = this.PEFileName;
          if (this.cachedPEFileName.Length == 0){
            this.compilerGlobals.assemblyBuilder.Save(Path.GetFileName(tempAssemblyName));
          }else
            tempAssemblyName = this.cachedPEFileName;
          FileStream stream = new FileStream(this.PEFileName, FileMode.Open, FileAccess.Read, FileShare.Read);
          pe = new byte[(int)stream.Length];
          stream.Read(pe, 0, pe.Length);
          stream.Close();
          this.cachedPEFileName = tempAssemblyName;
          if (this.genDebugInfo){
            string tempPDBName = Path.ChangeExtension(tempAssemblyName, ".pdb");
            // genDebugInfo could have been changed since we compiled, so check to make sure the symbols are there
            if (File.Exists(tempPDBName)){
              stream = new FileStream(tempPDBName, FileMode.Open, FileAccess.Read, FileShare.Read);
              try{
                pdb = new byte[(int)stream.Length];
                stream.Read(pdb, 0, pdb.Length);
              }finally{
                stream.Close();
              }
            }
          }
        }catch(Exception e){
          throw new VsaException(VsaError.SaveCompiledStateFailed, e.ToString(), e);
        }
      }

      protected override void DoSaveSourceState(IVsaPersistSite site){
      }

      private void SaveSourceForDebugging(){
        if (!this.GenerateDebugInfo || this.debugDirectory == null || !this.isEngineDirty)
          return;
        foreach (VsaItem item in this.vsaItems){
          if (item is VsaStaticCode){
            string fileName = this.debugDirectory + item.Name + ".js";
            try{
              FileStream file = new FileStream(fileName, FileMode.Create, FileAccess.Write);
              try{
                StreamWriter sw = new StreamWriter(file);
                sw.Write(((VsaStaticCode)item).SourceText);
                sw.Close();
                item.SetOption("codebase", fileName);
              }finally{
                file.Close();
              }
            }catch(Exception){
              // swallow any file creation exceptions
            }
          }
        }
      }


      internal ArrayList Scopes{
        get{
          if (this.scopes == null)
            this.scopes = new ArrayList(8);
          return this.scopes;
        }
      }

      public ScriptObject ScriptObjectStackTop(){
        return (ScriptObject)this.Globals.ScopeStack.Peek();
      }

      internal void SetEnclosingContext(ScriptObject ob){
        ScriptObject s = this.Globals.ScopeStack.Peek();
        while (s.GetParent() != null)
          s = s.GetParent();
        s.SetParent(ob);
      }

      public virtual void SetOutputStream(IMessageReceiver output){
        COMCharStream stream = new COMCharStream(output);
        System.IO.StreamWriter writer = new System.IO.StreamWriter(stream, Encoding.Default);
        writer.AutoFlush = true;
        ScriptStream.Out = writer;
        ScriptStream.Error = writer;
      }

      protected override void SetCustomOption(String name, Object value){
        try{
          if (String.Compare(name, "CLSCompliant", true, CultureInfo.InvariantCulture) == 0)
            this.isCLSCompliant = (bool)value;
          else if (String.Compare(name, "fast", true, CultureInfo.InvariantCulture) == 0)
            this.doFast = (bool)value;
          else if (String.Compare(name, "output", true, CultureInfo.InvariantCulture) == 0){
            if (value is String){
              this.PEFileName = (String)value;
              this.doSaveAfterCompile = true;
            }
          }else if (String.Compare(name, "PEFileKind", true, CultureInfo.InvariantCulture) == 0)
            this.PEFileKind = (PEFileKinds)value;
          else if (String.Compare(name, "print", true, CultureInfo.InvariantCulture) == 0)
            this.doPrint = (bool)value;
          else if (String.Compare(name, "UseContextRelativeStatics", true, CultureInfo.InvariantCulture) == 0)
            this.doCRS = (bool)value;
            // the next two are needed because of the ICompiler interface. They should not fail even though they may not do anything
          else if (String.Compare(name, "optimize", true, CultureInfo.InvariantCulture) == 0)
            return;
          else if (String.Compare(name, "define", true, CultureInfo.InvariantCulture) == 0)
            return;
          else if (String.Compare(name, "defines", true, CultureInfo.InvariantCulture) == 0)
            this.Defines = (Hashtable)value;
          else if (String.Compare(name, "ee", true, CultureInfo.InvariantCulture) == 0)
            VsaEngine.executeForJSEE = (Boolean)value;
          else if (String.Compare(name, "version", true, CultureInfo.InvariantCulture) == 0)
            this.versionInfo = (Version)value;
          else if (String.Compare(name, "VersionSafe", true, CultureInfo.InvariantCulture) == 0)
            this.versionSafe = (Boolean)value;
          else if (String.Compare(name, "libpath", true, CultureInfo.InvariantCulture) == 0)
            this.libpath = (String)value;
          else if (String.Compare(name, "warnaserror", true, CultureInfo.InvariantCulture) == 0)
            this.doWarnAsError = (bool)value;
          else if (String.Compare(name, "WarningLevel", true, CultureInfo.InvariantCulture) == 0)
            this.nWarningLevel = (int)value;
          else if (String.Compare(name, "managedResources", true, CultureInfo.InvariantCulture) == 0)
            this.managedResources = (ICollection)value;
          else if (String.Compare(name, "alwaysGenerateIL", true, CultureInfo.InvariantCulture) == 0)
            this.alwaysGenerateIL = (bool)value;
          else if (String.Compare(name, "DebugDirectory", true, CultureInfo.InvariantCulture) == 0){
            // use null to turn off SaveSourceState source generation
            if (value == null){
              this.debugDirectory = null;
              return;
            }
            string dir = value as string;
            if (dir == null)
              throw new VsaException(VsaError.OptionInvalid);
            try{
              dir = Path.GetFullPath(dir + Path.DirectorySeparatorChar);
              if (!Directory.Exists(dir))
                Directory.CreateDirectory(dir);
            }catch(Exception e){
              // we couldn't create the specified directory
              throw new VsaException(VsaError.OptionInvalid, "", e);
            }
            this.debugDirectory = dir;
          }else if (String.Compare(name, "AutoRef", true, CultureInfo.InvariantCulture) == 0)
            this.autoRef = (bool)value;
          else
            throw new VsaException(VsaError.OptionNotSupported);
        }catch(VsaException){
          throw;
        }catch(Exception){
          throw new VsaException(VsaError.OptionInvalid);
        }
      }

      internal void SetUpCompilerEnvironment(){
        if (!this.isCompilerSet){
          this.globals = this.Globals;
          this.isCompilerSet = true;
        }
      }

      internal void TryToAddImplicitAssemblyReference(String name){
        if (!this.autoRef) return;
        
        String key;
        SimpleHashtable implictAssemblyCache = this.implicitAssemblyCache;
        if (implicitAssemblyCache == null) {
          //Populate cache with things that should not be autoref'd. Canonical form is lower case without extension.
          implicitAssemblyCache = new SimpleHashtable(50);
          
          //PEFileName always includes an extension and is never NULL.
          implicitAssemblyCache[Path.GetFileNameWithoutExtension(this.PEFileName).ToLower(CultureInfo.InvariantCulture)] = true;
          
          foreach (Object item in this.vsaItems){
            VsaReference assemblyReference = item as VsaReference;
            if (assemblyReference == null || assemblyReference.AssemblyName == null) continue;
            key = Path.GetFileName(assemblyReference.AssemblyName).ToLower(CultureInfo.InvariantCulture);
            if (key.EndsWith(".dll"))
              key = key.Substring(0, key.Length-4);
            implicitAssemblyCache[key] = true;
          }          
          this.implicitAssemblyCache = implicitAssemblyCache;          
        }
        
        key = name.ToLower(CultureInfo.InvariantCulture);
        if (implicitAssemblyCache[key] != null) return;
        implicitAssemblyCache[key] = true;
        
        try{
          VsaReference assemblyReference = new VsaReference(this, name + ".dll");
          if (assemblyReference.Compile(false)){
            ArrayList implicitAssemblies = this.implicitAssemblies;
            if (implicitAssemblies == null) {
               implicitAssemblies = new ArrayList();
               this.implicitAssemblies = implicitAssemblies;
            }
            implicitAssemblies.Add(assemblyReference.Assembly);
          }
        }catch(VsaException){
        }
      }

      internal String RuntimeDirectory{
        get{
          if (this.runtimeDirectory == null){
            //Get the path to mscorlib.dll
            String s = typeof(Object).Module.FullyQualifiedName;
            //Remove the file part to get the directory
            this.runtimeDirectory = Path.GetDirectoryName(s);
          }
          return this.runtimeDirectory;
        }
      }

      internal String[] LibpathList{
        get{
          if (this.libpathList == null){
            if (this.libpath == null)
              this.libpathList = new String[]{Typeob.Object.Module.Assembly.Location};
            else
              this.libpathList = this.libpath.Split(new Char[] {Path.PathSeparator});
          }
          return this.libpathList;
        }
      }

      internal String FindAssembly(String name){
        String path = name;
        if (Path.GetFileName(name) == name){ // just the filename, no path
          // Look in current directory
          if (!File.Exists(name)){
            // Look in COM+ runtime directory
            String path1 = this.RuntimeDirectory + Path.DirectorySeparatorChar + name;
            if (File.Exists(path1))
              path = path1;
            else{
              // Look on the LIBPATH
              String[] libpathList = this.LibpathList;
              foreach( String l in libpathList ){
                if (l.Length > 0){
                  path1 = l + Path.DirectorySeparatorChar + name;
                  if (File.Exists(path1)){
                    path = path1;
                    break;
                  }
                }
              }
            }
          }
        }
        if (!File.Exists(path))
          return null;
        return path;
      }

      protected override void ValidateRootMoniker(string rootMoniker){
        // We override this method to avoid reading the registry in a non-VSA scenario
        if (this.genStartupClass)
          base.ValidateRootMoniker(rootMoniker);
        else if (rootMoniker == null || rootMoniker.Length == 0)
          throw new VsaException(VsaError.RootMonikerInvalid);
      }

      internal static bool CheckIdentifierForCLSCompliance(String name){
        if (name[0] == '_')
          return false;
        for (int i = 0; i < name.Length; i++){
          if (name[i] == '$')
            return false;
        }
        return true;
      }

      internal void CheckTypeNameForCLSCompliance(String name, String fullname, Context context){
        if (!this.isCLSCompliant)
          return;
        if (name[0] == '_'){
          context.HandleError(JSError.NonCLSCompliantType);
          return;
        }
        if (!VsaEngine.CheckIdentifierForCLSCompliance(fullname)){
          context.HandleError(JSError.NonCLSCompliantType);
          return;
        }
        if (this.typenameTable == null)
          this.typenameTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
        if (this.typenameTable[fullname] == null)
          this.typenameTable[fullname] = fullname;
        else
          context.HandleError(JSError.NonCLSCompliantType);
      }
    }

    // The VSA spec requires that every IVsaEngine has a host property that is non-null.
    // Since every assembly that we generate creates an engine (see CreateEngineAndGetGlobalScope)
    // we must provide a default site for it.

    class DefaultVsaSite: BaseVsaSite{
      public override bool OnCompilerError(IVsaError error){
        // We expect only JScriptExceptions here, and we throw them to be caught by the host
        throw (JScriptException)error;
      }
    }
}
