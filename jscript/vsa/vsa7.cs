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
[assembly:System.CLSCompliant(true)]
namespace Microsoft.Vsa
{
    using System.CodeDom;
    using System;
    using System.Runtime.Serialization;

    /* VsaItemType is also defined in vs\src\vsa\vbapkg\src\vbapkg.idl
       If you want to change the definition here, you have to make changes there too.
       See comment in vs\src\vsa\vbapkg\src\vbapkg.idl which explain why we need
       this definition twice */  
    public enum VsaItemType
    {
        Reference,             // IVsaReferenceItem - COM+ Assembly or RCW
        AppGlobal,             // IVsaItem - application object
        Code,                  // IVsaCodeItem - vsa code
    }
    
    public enum VsaItemFlag
    {
        None,                  // No special flags required or flags not accepted
        Module,                // Global Module
        Class,                 // Instantiable COM+ class.
    }

    public enum VsaIDEMode
    {
        Break,                  // Break mode - debugger is attached and in break mode
        Design,                 // Design mode - debugger is not attached
        Run,                    // Run mode - debugger is attached
    }

    public interface IVsaEngine
    {
        // === Site ===
        IVsaSite Site { get; set; }
    
        // === Properties ===
        // Name, RootMoniker, RootNameSpace are first class properties of the engine
        // Never assume get, set operations of first class properties can be the
        // equivalent of calling SetOption and GetOption
    
        string Name { get; set; }
        string RootMoniker { get; set; }        
        // Default namespace used by the engine
        string RootNamespace { get; set; }
    
        int LCID { get; set; }

        bool GenerateDebugInfo {get; set; }

        System.Security.Policy.Evidence Evidence { get; set; }

        // collection of items
        IVsaItems Items { get; }
    
        // returns TRUE if any item in the engine has been dirtied
        bool IsDirty { get; }
    
        // return the programming language of the engine (e.g. VB or JScript)
        string Language { get; }
    
        // returns the engine version
        string Version { get; }

        // engine options (methods)
        object GetOption(string name);
        void SetOption(string name, object value);
    
        // === State management ===
        bool Compile();
        void Run();
        void Reset();
        void Close();

        bool IsRunning { get; }
        bool IsCompiled { get; }

        void RevokeCache();
    
        // === Source State ===
        void SaveSourceState(IVsaPersistSite site);
        void LoadSourceState(IVsaPersistSite site);

        // === Compiled State ===
        void SaveCompiledState(out byte[] pe, out byte[] pdb);
    
        // Called if the engine state will be set by calls (rather than a call to by LoadSourceState)
        void InitNew();

        // returns TRUE if the identifier if legal for the language of the engine
        bool IsValidIdentifier(string identifier);

        // === Assembly === 
        System.Reflection.Assembly Assembly { get; }
    } // IVsaEngine

    public interface IVsaSite
    {
        void GetCompiledState(out byte[] pe, out byte[] debugInfo);
    
        bool OnCompilerError(IVsaError error);       
    
        object GetGlobalInstance( string name );

        object GetEventSourceInstance( string itemName,
                                       string eventSourceName);
     
        void Notify(string notify, object info);

    } // IVSaSite
  
    public interface IVsaPersistSite
    {
      void SaveElement(string name, string source);
      string LoadElement(string name);
    } // IVsaPersistSite
      
    public interface IVsaError
    {
        int Line { get; }        
        int Severity { get; }        
        string Description { get; }
        string LineText { get; }
        IVsaItem SourceItem { get; }   
        // returns the ending column of the text creating the error
        int EndColumn { get; }    
        // returns the beginning column of the text creating the error
        int StartColumn { get; }
        // Error number
        int Number { get; }
        // (fully qualified?) moniker of item with error
        string SourceMoniker { get; }
    } // IVsaError

    public interface IVsaItem
    {   
        string Name { get; set; }    
        VsaItemType ItemType { get; }
        // returns TRUE if item is dirty
        bool IsDirty { get; }
        // item option (property)
        object GetOption(string name);
        void SetOption(string name, object value);
    } // IVsaItem
    
    public interface IVsaItems : System.Collections.IEnumerable
    {
        int Count { get; }
        IVsaItem this[string name] { get; }
        IVsaItem this[int index] { get; }
        // Create an item
        IVsaItem CreateItem(string name, VsaItemType itemType, VsaItemFlag itemFlag);
        void Remove(string name);
        void Remove(int index);
    } // IVsaItems
          
    public interface IVsaReferenceItem : IVsaItem
    { 
        string AssemblyName { get; set; }
    } // IVsaReferenceItem
    
    public interface IVsaCodeItem : IVsaItem
    {
        string SourceText { get; set; }
    
        // returns a CodeDom tree for this code item.  The CodeObject returned is of type
        // System.CodeDom.CodeCompileUnit
        System.CodeDom.CodeObject CodeDOM { get; }
    
        void AppendSourceText(string text);
    
        void AddEventSource( string eventSourceName,
                             string eventSourceType);
        
        void RemoveEventSource( string eventSourceName);
        
    } // IVsaCodeItem
      
    
    public interface IVsaGlobalItem : IVsaItem
    {
        string TypeString { set; }
    
        bool ExposeMembers { get; set; }
    } // IVsaGlobalItem
    




    // Make it contiguous so that it's easy to determine whether this is a VsaError.
    // WARNING: If you change the error code below, you MUST make the corresponding changes in vs\src\common\inc\vsa\vsares.h
    // Note we are using the int (as opposed to unsigned hex) values for CLS Compliance
    public enum VsaError : int
    {
        AppDomainCannotBeSet        = -2146226176, // 0x80133000
        AppDomainInvalid            = -2146226175, // 0x80133001
        ApplicationBaseCannotBeSet  = -2146226174, // 0x80133002
        ApplicationBaseInvalid      = -2146226173, // 0x80133003
        AssemblyExpected            = -2146226172, // 0x80133004
        AssemblyNameInvalid         = -2146226171, // 0x80133005
        BadAssembly                 = -2146226170, // 0x80133006
        CachedAssemblyInvalid       = -2146226169, // 0x80133007
        CallbackUnexpected          = -2146226168, // 0x80133008
        CodeDOMNotAvailable         = -2146226167, // 0x80133009
        CompiledStateNotFound       = -2146226166, // 0x8013300A
        DebugInfoNotSupported       = -2146226165, // 0x8013300B
        ElementNameInvalid          = -2146226164, // 0x8013300C
        ElementNotFound             = -2146226163, // 0x8013300D
        EngineBusy                  = -2146226162, // 0x8013300E
        EngineCannotClose           = -2146226161, // 0x8013300F
        EngineCannotReset           = -2146226160, // 0x80133010
        EngineClosed                = -2146226159, // 0x80133011
        EngineEmpty                 = -2146226159, // 0x80133012
        EngineInitialized           = -2146226157, // 0x80133013
        EngineNameInUse             = -2146226156, // 0x80133014
        EngineNotCompiled           = -2146226155, // 0x80133015
        EngineNotInitialized        = -2146226154, // 0x80133016
        EngineNotRunning            = -2146226153, // 0x80133017
        EngineRunning               = -2146226152, // 0x80133018
        EventSourceInvalid          = -2146226151, // 0x80133019
        EventSourceNameInUse        = -2146226150, // 0x8013301A
        EventSourceNameInvalid      = -2146226149, // 0x8013301B
        EventSourceNotFound         = -2146226148, // 0x8013301C
        EventSourceTypeInvalid      = -2146226147, // 0x8013301D
        GetCompiledStateFailed      = -2146226146, // 0x8013301E
        GlobalInstanceInvalid       = -2146226145, // 0x8013301F
        GlobalInstanceTypeInvalid   = -2146226144, // 0x80133020
        InternalCompilerError       = -2146226143, // 0x80133021
        ItemCannotBeRemoved         = -2146226142, // 0x80133022
        ItemFlagNotSupported        = -2146226141, // 0x80133023
        ItemNameInUse               = -2146226140, // 0x80133024
        ItemNameInvalid             = -2146226139, // 0x80133025
        ItemNotFound                = -2146226138, // 0x80133026
        ItemTypeNotSupported        = -2146226137, // 0x80133027
        LCIDNotSupported            = -2146226136, // 0x80133028
        LoadElementFailed           = -2146226135, // 0x80133029
        NotificationInvalid         = -2146226134, // 0x8013302A
        OptionInvalid               = -2146226133, // 0x8013302B
        OptionNotSupported          = -2146226132, // 0x8013302C
        RevokeFailed                = -2146226131, // 0x8013302D
        RootMonikerAlreadySet       = -2146226130, // 0x8013302E
        RootMonikerInUse            = -2146226129, // 0x8013302F
        RootMonikerInvalid          = -2146226128, // 0x80133030
        RootMonikerNotSet           = -2146226127, // 0x80133031
        RootMonikerProtocolInvalid  = -2146226126, // 0x80133032
        RootNamespaceInvalid        = -2146226125, // 0x80133033
        RootNamespaceNotSet         = -2146226124, // 0x80133034
        SaveCompiledStateFailed     = -2146226123, // 0x80133035
        SaveElementFailed           = -2146226122, // 0x80133036
        SiteAlreadySet              = -2146226121, // 0x80133037
        SiteInvalid                 = -2146226120, // 0x80133038
        SiteNotSet                  = -2146226119, // 0x80133039
        SourceItemNotAvailable      = -2146226118, // 0x8013303A
        SourceMonikerNotAvailable   = -2146226117, // 0x8013303B
        URLInvalid                  = -2146226116, // 0x8013303C
        
        BrowserNotExist             = -2146226115, // 0x8013303D
        DebuggeeNotStarted          = -2146226114, // 0x8013303E
        EngineNameInvalid           = -2146226113, // 0x8013303F
        EngineNotExist              = -2146226112, // 0x80133040
        FileFormatUnsupported       = -2146226111, // 0x80133041
        FileTypeUnknown             = -2146226110, // 0x80133042
        ItemCannotBeRenamed         = -2146226109, // 0x80133043
        MissingSource               = -2146226108, // 0x80133044
        NotInitCompleted            = -2146226107, // 0x80133045
        NameTooLong                 = -2146226106, // 0x80133046
        ProcNameInUse               = -2146226105, // 0x80133047
        ProcNameInvalid             = -2146226104, // 0x80133048
        VsaServerDown               = -2146226103, // 0x80133049   
        MissingPdb                  = -2146226102, // 0x8013304A
        NotClientSideAndNoUrl       = -2146226101, // 0x8013304B
        CannotAttachToWebServer     = -2146226100, // 0x8013304C
        EngineNameNotSet            = -2146226099, // 0x8013304D
                       
        UnknownError                = -2146225921, // 0x801330FF
    } // VsaErr

    [System.SerializableAttribute]
    public class VsaException : Exception
    {
        //Serialization constructor
        public VsaException (SerializationInfo info, StreamingContext context)  
        {
          //deserialize value
          HResult = (Int32)info.GetValue("VsaException_HResult", typeof(Int32));
          HelpLink = (String)info.GetValue("VsaException_HelpLink", typeof(String));
          Source = (String)info.GetValue("VsaException_Source", typeof(String));
        }

	//serializes the object
        public override void GetObjectData(SerializationInfo info, StreamingContext context) 
        {  
           //serialize value 
           info.AddValue("VsaException_HResult", HResult);  
           info.AddValue("VsaException_HelpLink", HelpLink);
           info.AddValue("VsaException_Source", Source);
        }

        public override String ToString()
        {
            if ((null != this.Message) && ("" != this.Message))
                return "Microsoft.Vsa.VsaException: " + System.Enum.GetName(((VsaError)HResult).GetType(), (VsaError)HResult) + " (0x" + String.Format("{0,8:X}", HResult) + "): " + Message;    
            else
                return "Microsoft.Vsa.VsaException: " + System.Enum.GetName(((VsaError)HResult).GetType(), (VsaError)HResult) + " (0x" + String.Format("{0,8:X}", HResult) + ").";    
        }

        public VsaException(VsaError error) : this(error, "", null)
        {
        }

        public VsaException(VsaError error, string message) : this(error, message, null)
        {
        }

        public VsaException(VsaError error, string message, Exception innerException) : base(message, innerException)
        {
           HResult = (int)error;
        }

       public VsaError ErrorCode
        {
            get { return (VsaError)HResult; }
        }
    } // VsaException

    // A custom attribute to identify this assembly as being generated by VSA
    [type:System.AttributeUsageAttribute(System.AttributeTargets.All)]
    public class VsaModule : System.Attribute
    {
        public VsaModule(bool bIsVsaModule)
        {
            IsVsaModule = bIsVsaModule;
        }
        
        public bool IsVsaModule
        {
            get 
            { return isVsaModule; }
            set 
            { isVsaModule = value; }
        }
        
        private bool isVsaModule;    
    } // VsaModule
    
} // Microsoft.Vsa
