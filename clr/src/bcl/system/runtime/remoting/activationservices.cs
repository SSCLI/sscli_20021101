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
/*============================================================
**
**  File:    ActivationServices.cs
**
**  Author(s):  
**                                                     
**                                                      
**
**  Purpose:           
**
**  Date:    Jun 26, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Activation {

    using System;
    using System.Security;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Contexts;
    using System.Runtime.Remoting.Proxies;
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.Remoting.Metadata;
    using System.Collections;
    using System.Reflection;
    using System.IO;
    using System.Runtime.Serialization;    
    using IList       = System.Collections.IList;
    using ArrayList  = System.Collections.ArrayList;
    using IEnumerator = System.Collections.IEnumerator;
    using System.Security.Permissions;
    using System.Runtime.CompilerServices;

    // Implements various activation services
    internal sealed class ActivationServices
    {

        private static IActivator activator = null;     
        
        private static Hashtable _proxyTable = new Hashtable();
        private static Type proxyAttributeType = typeof(System.Runtime.Remoting.Proxies.ProxyAttribute); 
        private static ProxyAttribute _proxyAttribute = new ProxyAttribute();
        [ThreadStaticAttribute()]
        private static Object[] _activationAttributes;       

        internal const String ActivationServiceURI = "RemoteActivationService.rem";

        internal const String RemoteActivateKey = "Remote";
        internal const String PermissionKey = "Permission";
        internal const String ConnectKey = "Connect";


        //1 private static LocalActivator  localActivator = null;

        // ActivationListener is the object that listens to incoming
        // activation requests. It delegates the incoming request to 
        // the local activator.
        //2 private static ActivationListener ActivationListener = null;

        //3 private static Object staticSyncObject = new Object();
        //4 private static bool bInitializing = false;

        // This gets called upon the first attempt to activate
        // anything that is ContextBound or MarshalByRef
        private static void Startup()
        {            
            DomainSpecificRemotingData remData = Thread.GetDomain().RemotingData;

            // wait on the lock if a)activation has not been initialized yet
            // or b) activation is being initialized by another thread!
            if ((!remData.ActivationInitialized) 
               || remData.InitializingActivation)
            {
                lock(remData.ConfigLock)
                {
                    remData.InitializingActivation = true;
                    // Ensure that some other thread did not complete
                    // the work while we were waiting on the lock.
                    if (!remData.ActivationInitialized)
                    {
                        // Startup the activation service
                        BCLDebug.Trace("REMOTE","Starting up activation service ",Thread.CurrentContext);

                        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        // NOTE: This should be the first step in Startup!
                        // Otherwise activation will recurse, when we try
                        // to create the ActivationListener (which is MarshalByRef)
                        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        remData.LocalActivator = new LocalActivator();

                        // Add the Lifetime service property to the appdomain.
                        // For now we are assuming that this is the only property
                        // If there are more properties, then an existing array
                        // will need to be expanded to add this property

                        // This is activated in RemotingServices.DomainSpecificRemotingData()
                        // IContextProperty[] contextProperties = new IContextProperty[1];
                        // contextProperties[0] = new System.Runtime.Remoting.LeaseLifeTimeServiceProperty();
                        // Thread.GetDomain().RemotingData.AppDomainContextProperties = contextProperties;

                        remData.ActivationListener = new ActivationListener();
                        remData.ActivationInitialized = true;

                    }
                    
                    remData.InitializingActivation = false;
                } //lock (remData)
            }
        }            
           
        private static void InitActivationServices()
        {
            // If activation services has not been loaded do it now and create 
            // the instace that will service the activation requests.
            if (null == activator)
            {
                activator = GetActivator();
                if (null == activator)
                {
                    Message.DebugOut("Fatal Error... Could not create activator\n");                
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_BadInternalState"), 
                            "Activation service failed to initialize"));
                }
            }
        }

        // Determine whether the current context is ok for the activation.  
        private static MarshalByRefObject IsCurrentContextOK(Type serverType, 
                                                             Object[] props)
        {
            BCLDebug.Assert(!serverType.IsInterface,"!serverType.IsInterface");

            MarshalByRefObject retObj = null;
            
            // Initialize activation services if needed.
            //   (we temporary null out the activation attributes in case
            //    InitActivationServices creates an MBR).
            Object[] activationAttrs = _activationAttributes;
            _activationAttributes = null;
            InitActivationServices();
            _activationAttributes = activationAttrs;
            
            // Obtain the method info which will create an instance 
            // of type RealProxy
            ProxyAttribute pa = GetProxyAttribute(serverType);
            BCLDebug.Assert(null != pa, "null != pa");

            if (Object.ReferenceEquals(pa, DefaultProxyAttribute))
                retObj = pa.CreateInstanceInternal(serverType);
            else
                retObj = CreateInstanceWithLinkDemand(pa,serverType);
            BCLDebug.Assert(null != retObj, "null != retObj");
            return retObj;
        }

        // This has to be a helper method for IsCurrentContextOK because
        // referencing ProxyAttribute from IsCurrentContextOK above will 
        // cause infinite recursion in the evaluation of the LinkDemand on
        // ProxyAttribute.

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static MarshalByRefObject CreateInstanceWithLinkDemand(
            ProxyAttribute pa, Type serverType)
        {
            return pa.CreateInstance(serverType);
        }


        // For types with no proxy attribute, we take the default route of 
        // querying attributes if the current context is suitable for 
        // activation.
        private static bool IsCurrentContextOK(Type serverType, Object[] props, 
                                               ref ConstructorCallMessage ctorCallMsg)
        {
            //Get callSite attributes
            Object[] callSiteAttr = ActivationAttributes;

            // Set it to null
            ActivationAttributes = null;
            
            Object[] womAttr = new Object[1];
            womAttr[0] = GetGlobalAttribute();

            // Get the type context attributes
            Object[] typeAttr = GetContextAttributesForType(serverType);

            // Get the client context (current context)                                                             
            Context cliCtx = Thread.CurrentContext;

            // Create a ctorCallMsg with the reqd info 
            ctorCallMsg = 
                new ConstructorCallMessage(
                    callSiteAttr,
                    womAttr,
                    typeAttr,
                    serverType);

            // This is the activator that handles activation in *all* cases 
            // Based on whether what the activation attributes do.... other
            // activators may get chained ahead of this one and may take
            // over the activation process... (possibly) delegating to this
            // only in the last stage.
            // Note: currently, this does not get used in the same context (MBR)
            // scenarios ... because of the 2-step activation model of JIT.
            ctorCallMsg.Activator = new ConstructionLevelActivator();


            // Ask all attributes if they are happy with the current context
            // NOTE: if someone says no, we do not ask the rest of the attributes
            // This is why, womAttr (which is the global activation service
            // attribute) *must* be the first one we query.
            bool bCtxOK = QueryAttributesIfContextOK(cliCtx, 
                                                     ctorCallMsg,
                                                     womAttr);
            if (bCtxOK == true)
            {
                bCtxOK = QueryAttributesIfContextOK(cliCtx, 
                                                    ctorCallMsg,
                                                    callSiteAttr);
                if (bCtxOK == true)
                {
                    bCtxOK = QueryAttributesIfContextOK(cliCtx, 
                                                        ctorCallMsg,
                                                        typeAttr);
                }
            }

            return bCtxOK;
        }

	private static void CheckForInfrastructurePermission(Assembly asm)
	{
	    // Make a security check to ensure that the context attribute
	    // is from a trusted assembly!
	    if (asm != RemotingServices.s_MscorlibAssembly)
	    {
		CodeAccessSecurityEngine.CheckAssembly(
		    asm, 
		    RemotingServices.s_RemotingInfrastructurePermission);
	    }
	}

        private static bool QueryAttributesIfContextOK(
            Context ctx, IConstructionCallMessage ctorMsg, Object[] attributes) 
        {       
            bool bCtxOK = true;
            if (attributes != null)
            {
                for (int i=0; i<attributes.Length; i++)
                {
                    // All the attributes, including callsite attributes should 
                    // expose IContextAttribute. If not, then we throw an 
                    // exception.
                    IContextAttribute attr = attributes[i] as IContextAttribute;
                    if(null != attr)
                    { 
                        Assembly asm = attr.GetType().Assembly; 
                        // Make a security check to ensure that the context attribute
                        // is from a trusted assembly!
			CheckForInfrastructurePermission(asm);
                    
                        bCtxOK = attr.IsContextOK(ctx, ctorMsg);
                        if (bCtxOK == false)
                        {
                            break;
                        }
                    }                        
                    else
                    {
                        throw new RemotingException(
                                Environment.GetResourceString(
                                    "Remoting_Activation_BadAttribute"));
                    }
                }
            }
            return bCtxOK;
        }

        internal static void GetPropertiesFromAttributes(
            IConstructionCallMessage ctorMsg, Object[] attributes)
        {
            if (attributes != null)
            {
                for (int i=0; i<attributes.Length; i++)
                {
                    // All the attributes, including callsite attributes should 
                    // expose IContextAttribute. If not, then we throw an 
                    // exception.
                    IContextAttribute attr = attributes[i] as IContextAttribute;
                    if(null != attr) 
                    {         
                        Assembly asm = attr.GetType().Assembly; 
                        // Make a security check to ensure that the context attribute
                        // is from a trusted assembly!
			CheckForInfrastructurePermission(asm);
                    
                        // We ask each attribute to contribute its properties.
                        // The attributes can examine the current list of properties
                        // from the ctorCallMsg and decide which properties to add.

                        // They can also tweak the Activator chain during this process
                        attr.GetPropertiesForNewContext(ctorMsg);
                    }
                    else
                    {
                        throw new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_Activation_BadAttribute"));
                    }
                }
            }
        }

        // Return the default implementation of the proxy attribute
        internal static ProxyAttribute DefaultProxyAttribute
        {
            get { return _proxyAttribute; }
        }

        // Iterate over the custom attributes of the class and see if it 
        // defines a ProxyAttribute. If one is not defined then return
        // the default proxy attribute defined by us.
        internal static ProxyAttribute GetProxyAttribute(Type serverType)
        {
            
            BCLDebug.Assert(    
                serverType.IsMarshalByRef,
                "type should be at least marshal-by-ref");

            if (!serverType.HasProxyAttribute)
                return DefaultProxyAttribute;
                    
            // Type has a non-default proxy attribute ... see if we have
            // it cached?
            ProxyAttribute pa = _proxyTable[serverType] as ProxyAttribute;

            if (pa == null)
            {   
                Object[] ca = Attribute.GetCustomAttributes(
                                            serverType, 
                                            proxyAttributeType, 
                                            true);
                if((null != ca) && (0 != ca.Length))
                {                    
                    if (!serverType.IsContextful)
                    {
                        throw new RemotingException(
                                    Environment.GetResourceString(
                                    "Remoting_Activation_MBR_ProxyAttribute"));
                    }
                    pa = ca[0] as ProxyAttribute;
                }
                BCLDebug.Assert(pa!=null, "expect proxy attribute");

                if(!_proxyTable.Contains(serverType))
                {
                    lock(_proxyTable)
                    {
                        if(!_proxyTable.Contains(serverType))
                        {
                            // Create a new entry
                            _proxyTable.Add(serverType, pa);
                        }
                    }                    
                }
            }

            return pa;
        }

        // Creates either an uninitialized object or a proxy depending on 
        // whether the current context is OK or not.
        internal static MarshalByRefObject CreateInstance(Type serverType)
        {
            // Use our activation implementation               
            MarshalByRefObject retObj = null;
            ConstructorCallMessage ctorCallMsg = null;
            bool bCtxOK = IsCurrentContextOK(serverType, null, ref ctorCallMsg);
            if (bCtxOK && !serverType.IsContextful)
            {
                // Marshal by ref case
                Message.DebugOut("Allocating blank marshal-by-ref object"); 
                // We create an uninitialized instance of the actual type and
                // let the regular jit-activation execute the constructor on it.
                retObj = RemotingServices.AllocateUninitializedObject(serverType);
            }
            else 
            {
                //  (context not OK) || (serverType is Contextful)
                // We always return a proxy and the jit-activation's attempt
                // to execute the constructor gets intercepted by the TP stub
                // and routed to RemotingServices.Activate() below.

                // if context *is* OK 
                // this is a contextBound type being activated for which 
                // we always create a proxy (proxies-everywhere!)

                // if context *is not* OK 
                // this could be a MarshalByRef or ContextBound type 
                // being activated outside this appDomain 
                // OR
                // this could be a ContextBound type being activated cross-ctx 

                RemotingProxy rp;
                // See if the object-type is configured for Connect (with a URL)
                retObj = (MarshalByRefObject)ConnectIfNecessary(ctorCallMsg);
                if (retObj == null)
                {
                    // not configured for connect, take the usual route of 
                    // creating a proxy
                    Message.DebugOut("Creating remoting proxy for " + 
                                 serverType.FullName + " in context " + 
                                 Thread.CurrentContext);  

                    rp = new RemotingProxy(serverType);

                    Message.DebugOut("Created remoting proxy\n");
                    retObj = (MarshalByRefObject)rp.GetTransparentProxy();
                }
                else
                {
                    Message.DebugOut("NewObj did a Connect!");
                    rp = (RemotingProxy)RemotingServices.GetRealProxy(retObj);                    
                }                    

                // Store the constructor call message in the proxy
                rp.ConstructorMessage = ctorCallMsg;

                if (!bCtxOK)
                {

                    // Chain in the context level activator now
                    ContextLevelActivator activator = new ContextLevelActivator();
                    activator.NextActivator = ctorCallMsg.Activator; 
                    ctorCallMsg.Activator = activator;
                }
                else
                {
                    // Set the flag to indicate that the activation
                    // will not be leaving the client context .. in this case
                    // the default construction level activator is enough
                    // to complete the activation process.
                    ctorCallMsg.ActivateInContext = true;
                }
            }

            return retObj;
        }

        //
        // NOTE: This is an internal method used by RemotingProxy to do Activation
        // 
        // Starts off the activation chain by sending the constructor call message to 
        // the activator or the client context sink chain. On return, a constructor
        // return message is made and the out parameters are propagated.
        //
        internal static IConstructionReturnMessage Activate(
            RemotingProxy remProxy, IConstructionCallMessage ctorMsg)
        {
            IConstructionReturnMessage ctorRetMsg = null;

            if (((ConstructorCallMessage)ctorMsg).ActivateInContext)
            {
                // The current context was approved for activation
                BCLDebug.Assert(
                    ctorMsg.Activator.Level == ActivatorLevel.Construction,
                    "activator level must ActivatorLevel.Construction!");

                // This has to be a ContextBound type (proxies-everywhere)
                BCLDebug.Assert(ctorMsg.ActivationType.IsContextful, 
                                "Inconsistent state during activation");

                // Ask the activator in the message to take over
                ctorRetMsg = ctorMsg.Activator.Activate(ctorMsg);

                if (ctorRetMsg.Exception != null)
                {
                    throw ctorRetMsg.Exception;
                }
            }
            else
            {
                // Client context was not approved for activation ...
                BCLDebug.Assert(
                    ctorMsg.Activator.Level >= ActivatorLevel.Context,
                    "activator level must be at least x-context!");

                // Check with ActivationServices if we did a "Connect" with
                // a remote server during IsContextOK
                BCLDebug.Assert(
                    ActivationServices.CheckIfConnected(remProxy, ctorMsg) == null,
                    "We shouldn't come through this path on a Connect.");                    

                // Client context was not approved for activation ...
                // This is the more elaborate (real) activation case i.e.
                // we have to go at least out of the client context to 
                // finish the work.

                // Prepare for the handoff to Activation Service

                // Ask various attributes to contribute properties
                // The attributes may chain in other activators into
                // the activation chain (to hijack/participate in
                // the activation process).
                ActivationServices.GetPropertiesFromAttributes(
                    (IConstructionCallMessage)ctorMsg, 
                    ctorMsg.CallSiteActivationAttributes);

                ActivationServices.GetPropertiesFromAttributes(
                    ctorMsg, 
                    ((ConstructorCallMessage)ctorMsg).GetWOMAttributes());

                ActivationServices.GetPropertiesFromAttributes(
                    (IConstructionCallMessage)ctorMsg, 
                ((ConstructorCallMessage)ctorMsg).GetTypeAttributes());       

                // Fetch the client context chain
                IMessageSink cliCtxChain = 
                Thread.CurrentContext.GetClientContextChain();

                // Ask the client context chain to take over from here.
                IMethodReturnMessage retMsg =  
                    (IMethodReturnMessage)
                        cliCtxChain.SyncProcessMessage(ctorMsg);

                // The return message may not be of type
                // IConstructionReturnMessage if an exception happens
                // in the sink chains.
                ctorRetMsg = retMsg as IConstructionReturnMessage;
                if (null == retMsg)
                {
                    throw new RemotingException(
                        Environment.GetResourceString(
                            "Remoting_Activation_Failed"));
                }
                else if (retMsg.Exception != null)
                {
                    throw retMsg.Exception;
                }

                // install the callContext from the return message on the thread
                CallContext.SetLogicalCallContext(
                    (LogicalCallContext)
                        ctorRetMsg.Properties[Message.CallContextKey]);                    
                
            }
            // Note: PropagateOutParameters is now handled by RealProxy
            BCLDebug.Assert(
                null != ctorRetMsg, 
                "Activate returning null ConstructorReturnMessage");
                
            return ctorRetMsg;
        }

        // This function is called by ActivationServices in case
        // the activation needs to be within the same appdomain. These
        // are only for ContextBound types.
        // It is also called to do satisfy remote incoming requests from
        // the activation services. These could be for both ContextBound
        // and MarshalByRef types.
        internal static IConstructionReturnMessage DoCrossContextActivation(
            IConstructionCallMessage reqMsg)
        {           
            bool bCtxBound = reqMsg.ActivationType.IsContextful;
            ContextTransitionFrame frame = new ContextTransitionFrame();
            if (bCtxBound)
            {
                // If the type is context bound, we need to create 
                // the appropriate context and activate the object inside
                // it.

                // Create a new Context
                Context serverContext = new Context();              

                
                ArrayList list = (ArrayList) reqMsg.ContextProperties;
                Assembly asm = null;
                for (int i=0; i<list.Count; i++)
                {
                    IContextProperty prop = list[i] as IContextProperty;
                    if (null == prop)
                    {
                        throw new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_Activation_BadAttribute"));
                    }
                    asm = prop.GetType().Assembly; 
                    // Make a security check to ensure that the context property
                    // is from a trusted assembly!
		    CheckForInfrastructurePermission(asm);

                    // This ensures that we don't try to add duplicate
                    // attributes (eg. type attributes common on both client
                    // and server end)
                    if (serverContext.GetProperty(prop.Name) == null)
                    {
                        serverContext.SetProperty(prop);
                    }
                }
                // No more property changes to the server context from here.
                serverContext.Freeze();

                // (This seems like an overkill but that is how it is spec-ed)
                // Ask each of the properties in the context we formed from
                // if it is happy with the current context.
                for (int i=0; i<list.Count;i++)
                {
                    if (!((IContextProperty)list[i]).IsNewContextOK(
                        serverContext))
                    {
                        throw new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_Activation_PropertyUnhappy"));
                    }
                }

                // Change to server context
                Thread.CurrentThread.EnterContext(serverContext, ref frame);
            }

            // call the first sink in the server context chain
            IMethodReturnMessage retMsg =  (IMethodReturnMessage) 
                    Thread.CurrentContext.GetServerContextChain().SyncProcessMessage(reqMsg);

            // The return message may not be of type
            // IConstructionReturnMessage if an exception happens
            // in the sink chains.
            Exception e = null;
            IConstructionReturnMessage replyMsg = retMsg as IConstructionReturnMessage;
            if (null == replyMsg)
            {
                if (retMsg != null)
                {
                    e = retMsg.Exception;
                }
                else
                {
                    e = new RemotingException(
                            Environment.GetResourceString(
                                "Remoting_Activation_Failed"));

                }
                replyMsg = new ConstructorReturnMessage(e,null); 
                // We have created our own message ... transfer the callcontext
                // from the request message.
                ((ConstructorReturnMessage)replyMsg).SetLogicalCallContext(
                        (LogicalCallContext)
                        reqMsg.Properties[Message.CallContextKey]);
            }

            if (bCtxBound)
            {
                Thread.CurrentThread.ReturnToContext(ref frame);
            }

            return replyMsg;
        }

        internal static IConstructionReturnMessage DoServerContextActivation(
            IConstructionCallMessage reqMsg)
        {
            BCLDebug.Assert(reqMsg!=null, "NULL ctorReqMsg");
            Exception e=null;
            Type serverType = reqMsg.ActivationType;
            Object serverObj = ActivateWithMessage(
                                    serverType, 
                                    reqMsg, 
                                    null,
                                    out e);

            IConstructionReturnMessage replyMsg = 
                SetupConstructionReply(serverObj, reqMsg, e);

            BCLDebug.Assert(replyMsg!=null, "NULL ctorRetMsg");
            return replyMsg;
        }

        internal static IConstructionReturnMessage SetupConstructionReply(
            Object serverObj,
            IConstructionCallMessage ctorMsg,
            Exception e)
        {
            IConstructionReturnMessage replyMsg = null;
            if (e == null)
            {
                replyMsg =
                    new ConstructorReturnMessage(
                        (MarshalByRefObject)serverObj,
                        null,
                        0,
                        (LogicalCallContext)
                            ctorMsg.Properties[Message.CallContextKey],
                        ctorMsg);
            }
            else
            {
                replyMsg = new ConstructorReturnMessage(e,null);
                // We have created our own message ... transfer the callcontext
                // from the request message.
                ((ConstructorReturnMessage)replyMsg).SetLogicalCallContext(
                        (LogicalCallContext)
                            ctorMsg.Properties[Message.CallContextKey]);

            }
            return replyMsg;
        }

        internal static Object ActivateWithMessage(
            Type serverType, IMessage msg, ServerIdentity srvIdToBind,
            out Exception e)
        {
            Object server = null;
            e = null;

            // Create a blank instance!
            server = RemotingServices.AllocateUninitializedObject(serverType);

            Object proxyForObject = null;
            if (serverType.IsContextful)
            {
                if (msg is ConstructorCallMessage)
                {
                    // If it is a strictly x-context activation then
                    // this pointer for the message is the TP that we
                    // returned to JIT in first phase of activation
                    proxyForObject = ((ConstructorCallMessage)msg).GetThisPtr();
                }
                else
                {
                    // we are out of the app-domain, so wrap this object now
                    proxyForObject = null;
                }

                // This associates the proxy with the real object and sets
                // up the proxy's native context, ID etc.
                proxyForObject = RemotingServices.Wrap(
                                        (ContextBoundObject)server,
                                        proxyForObject, 
                                        false);
                BCLDebug.Assert(
                    RemotingServices.IsTransparentProxy(proxyForObject),
                    "Wrapped object should be a transparent proxy");
            }
            else
            {
                // Marshal-by-ref case we just return the naked object
                proxyForObject = server;
            }

            // Create the dispatcher which will help run the CTOR
            IMessageSink dispatcher = (IMessageSink)new StackBuilderSink(proxyForObject);

            // This call runs the CTOR on the object
            IMethodReturnMessage retMsg = (IMethodReturnMessage) 
                                        dispatcher.SyncProcessMessage(msg);

            if (retMsg.Exception == null)
            {
                if (serverType.IsContextful)
                {
                    // call wrap to finish the operation.
                    return RemotingServices.Wrap((ContextBoundObject)server);
                }
                else
                {
                    return server;
                }
            }
            else
            {
                e = retMsg.Exception;
                return null;
            }
        }

        public static MarshalByRefObject ConstructObject(IConstructionCallMessage ctorMsg)
        {
            Exception e = null;
            MarshalByRefObject o = 
                (MarshalByRefObject)
                    ActivateWithMessage(    
                        ctorMsg.ActivationType, 
                        ctorMsg, 
                        null, 
                        out e);

            if (e != null)
                throw e;
            return o;
        }

        internal static void StartListeningForRemoteRequests()
        {
            // Get the activation services set up.
            Startup();
            DomainSpecificRemotingData remData = Thread.GetDomain().RemotingData;
            if (!remData.ActivatorListening)
            {
                lock (remData.ConfigLock)
                {
                    if (!remData.ActivatorListening)
                    {
                        BCLDebug.Log("Registering remoting activator");

                        // Marshal the object so that it is ready to take remote
                        // calls. We have to create the objref because that is when
                        // the channels start listening for requests.
                        RemotingServices.MarshalInternal(
                            Thread.GetDomain().RemotingData.ActivationListener, 
                            ActivationServiceURI,
                            typeof(System.Runtime.Remoting.Activation.IActivator)); 
                            
                        ServerIdentity srvID = (ServerIdentity)
                            IdentityHolder.ResolveIdentity(ActivationServiceURI);

                        // Set Singleton to prevent lease from being created
                        srvID.SetSingletonObjectMode();
                        //DBG Console.WriteLine("Activator URI: = " + activatorURI);
                        remData.ActivatorListening = true;
                    }
                }
            }
        }

        // This returns the local activator
        internal static IActivator GetActivator()
        {
            DomainSpecificRemotingData remData = Thread.GetDomain().RemotingData;
            if (remData.LocalActivator == null)
            {
                Startup();
            }
            return (IActivator)remData.LocalActivator;
        }

        internal static void Initialize()
        {
            GetActivator();
        }

        // This returns the attribute that takes part in every activation
        // The local activator itself exposes that functionality
        internal static ContextAttribute GetGlobalAttribute()
        {
            DomainSpecificRemotingData remData = Thread.GetDomain().RemotingData;
            if (remData.LocalActivator == null)
            {
                Startup();
            }
            return (ContextAttribute)remData.LocalActivator;
        }
            
        // This function returns the array of custom attributes of
        // type "ContextAttribute" walking the entire class hierarchy
        // of the serverType
        // If duplicates are found the attribute on the more derived
        // type is kept.
        // The return array may be sparse and is terminated by a NULL.    
        internal static IContextAttribute[] GetContextAttributesForType(Type serverType)
        {
            if (!(typeof(ContextBoundObject).IsAssignableFrom(serverType)) ||
                serverType.IsCOMObject)
            {
                return new ContextAttribute[0];
            }

            Type currType = serverType;
            Object[] currAttr = null;
            int retSize = 8;
            IContextAttribute[] retAttr = new IContextAttribute[retSize];
            int numAttr = 0;
    
    
            // Obtain the custom attributes that implement 
            // IContextAttribute for this type
            currAttr = currType.GetCustomAttributes(
                                    typeof(IContextAttribute),
                                    true);  // recursively on the typeHierarchy
            Boolean bDupe;                             
            foreach (IContextAttribute attr in currAttr)
            {
                Type attrType = attr.GetType();
                bDupe = false;
                for (int i=0; i<numAttr; i++)
                {
                    if (attrType.Equals(retAttr[i].GetType()))
                    {
                        bDupe = true;
                        break;
                    }
                }

                if (!bDupe)
                { 
                    // We must add this attribute to our list
                    numAttr++;
                     
                    // Check if we have enough space to store it
                    // Leaving one spot for a NULL value!
                    if (numAttr > retSize-1)
                    {
                        IContextAttribute[] newAttr = new IContextAttribute[2*retSize];
                        Array.Copy(
                            retAttr,     // srcArray
                            0,          // srcIndex
                            newAttr,    // destArray
                            0,          // destIndex
                            retSize);   // lengthToCopy
                        retAttr = newAttr;
                        retSize = retSize*2;
                    }
                    retAttr[numAttr-1] = attr;
                }
            }
           
            IContextAttribute[] ctxAttr = new IContextAttribute[numAttr];
            Array.Copy(retAttr, ctxAttr, numAttr);
            return ctxAttr;
        }  
        
       //   Get or set the callsite activation attributes in the thread local
       //   static
       //   
        internal static Object[] ActivationAttributes
        {
            get
            {
                return _activationAttributes;
            }
            set
            {
                _activationAttributes = value;
            }
        }
                
        // This is called during RS.IsContextOK to check if the new XXX() is configured
        // to do a direct connect, and if it is we Connect to the URL and return
        // the proxy. In the second stage, when the constructor executes on the proxy
        // we will make sure (in Activate) that there are no CTOR args.
        internal static Object ConnectIfNecessary(IConstructionCallMessage ctorMsg)
        {
            // If the type being instantiated is configured for Connect
            // we would have added its URL as the connect key during 
            // LocalActivator::IsContextOK

            // Look for the connect URL
            String objURL = (String) ctorMsg.Properties[ConnectKey];
            Object proxy = null;
            if (objURL != null)
            {   
                // Connect to the URL and return the proxy
                proxy = RemotingServices.Connect(
                                        ctorMsg.ActivationType,
                                        objURL);
            }

            // If the type is not setup for connecting we return null!
            return proxy;                                            
        }                                        

        // This is really used to distinguish between proxies for completely 
        // within AppDomain activations and the ones from "Connect" during
        // the second stage (RS::Activate)
        // For the former, we have to run constructor etc and for the latter
        // we have to check that there is no non-defaul CTOR.
        internal static Object CheckIfConnected(
            RemotingProxy proxy, IConstructionCallMessage ctorMsg)
        {
            // If we performed a connect, we must have put the URL as
            // the connectKey in the message.
            String objURL = (String)
                            ctorMsg.Properties[ConnectKey];
            Object tp = null;

            if (objURL != null)
            {   
                // We did perform a connect during IsContextOK 
                // Just get the TP from RP and return it.
                tp = (Object)proxy.GetTransparentProxy();
            } 
            // We return null if we do not recognize this proxy!
            return tp;                                            
        }
        
    } // class ActivationServices    


    // This is the local activation helper and also the Attribute 
    // that gets queried about every activation
    internal class LocalActivator: ContextAttribute, IActivator
    {
        internal LocalActivator()
            : base(ActivationServices.ActivationServiceURI)
        {
        
        }

        // ---------------------------------------------------------------        
        // ContextAttribute functionality
        // ---------------------------------------------------------------        

        // IContextAttribute::IsContextOK
        // This will check if a type is configured for remote activation ... 
        // We return 'false' for IsContextOK if it is.
        public override bool IsContextOK(
            Context ctx, IConstructionCallMessage ctorMsg)
        {

            // If the app is not using config mechanism, we don't want
            // to intercept activation.
            if (RemotingConfigHandler.Info == null)
            {
                return true;
            }

            // check if this type is configured for connect 
            // (instead of remote activate)
            WellKnownClientTypeEntry wkte = 
                RemotingConfigHandler.IsWellKnownClientType(
                    ctorMsg.ActivationType);
            String typeURL = (wkte == null ? null : wkte.ObjectUrl);

            if (typeURL != null)
            {
                // this type does have a direct uri, we will try to connect 
                // to it during the activate call. Cache it in the message.
                ctorMsg.Properties[ActivationServices.ConnectKey] = typeURL;
                return false;
            }
            else
            {
                ActivatedClientTypeEntry acte = 
                    RemotingConfigHandler.IsRemotelyActivatedClientType(
                        ctorMsg.ActivationType);

                String appURL = null;

                if (acte == null)
                {
                    // This is the case where the config file had no entry for this type.
                    // We should check the callsite attributes for a URL
                    Object[] callsiteAttributes = ctorMsg.CallSiteActivationAttributes;
                    if(null != callsiteAttributes)
                    {
                        for(int i = 0; i < callsiteAttributes.Length; i++)
                        {
                            UrlAttribute attr = callsiteAttributes[i] as UrlAttribute;
                            if(null != attr)
                            {
                                appURL = attr.UrlValue; 
                            }
                        }
                    }
                        
                    if(appURL == null)
                    {
                        // We don't really care about intercepting the activation in this case.
                        return true;
                    }
                }
                else
                {
                    appURL = acte.ApplicationUrl;
                }

                // Generate the URL of the remote activator                    
                String activatorURL = null;
                if (!appURL.EndsWith("/"))
                    activatorURL = appURL + "/" + ActivationServices.ActivationServiceURI;
                else
                    activatorURL = appURL + ActivationServices.ActivationServiceURI;
                            
                // Mark a flag for remote activation 
                // (caching the url of the activation svc of the remote server)
                ctorMsg.Properties[ActivationServices.RemoteActivateKey] = activatorURL;
                return false;
            }
        }
                
        // IContextAttribute::GetPropertiesForNewContext
        public override void GetPropertiesForNewContext(
            IConstructionCallMessage ctorMsg)
        {            
            BCLDebug.Log("ActivationSvc:GlobalAttrib::GetPropForNewCtx");
            // This is called during RS::Activate .. when we are sure that
            // activation is at least x-context and this is a real activation
            // instead of a spoofed connect underneath the "new".
            BCLDebug.Assert(ctorMsg!=null, "ctorMsg null?");
            if (ctorMsg.Properties.Contains(ActivationServices.RemoteActivateKey))
            {
                // Means we did want to intercept activation!
                String remActivatorURL = (String)
                    ctorMsg.Properties[ActivationServices.RemoteActivateKey];
                
                AppDomainLevelActivator activator = 
                    new AppDomainLevelActivator(remActivatorURL);
                // Chain ourselves at the end of the AppDomainLevel activators
                BCLDebug.Assert(
                        ctorMsg.Activator != null, 
                        "Should have at least x-context activator");
                IActivator curr = ctorMsg.Activator;
                
                if (curr.Level < ActivatorLevel.AppDomain)
                {
                    // Common case .. .only x-context activator(s) in chain
                    activator.NextActivator = curr;
                    ctorMsg.Activator = activator;
                }
                else if (curr.NextActivator == null)
                {  
                    // Only one activator but not ContextLevel ...
                    // We go at the end of the chain 
                    curr.NextActivator = activator;
                }
                else
                {
                    // We will have to walk the chain till the end of the last
                    // AD activator and plug ourselves in.
                    while (curr.NextActivator.Level >= ActivatorLevel.AppDomain)
                    {                        
                        curr = curr.NextActivator;
                    }
                    BCLDebug.Assert(
                        curr.NextActivator.Level.Equals(ActivatorLevel.Context),
                        "bad ordering of activators!");
                    activator.NextActivator = curr.NextActivator;
                    curr.NextActivator = activator;
                }                                
            }
        }                     

        // ---------------------------------------------------------------        
        // IActivator functionality
        // ---------------------------------------------------------------        
        //IActivator::NextActivator
        public virtual IActivator NextActivator 
        {
            // We are a singleton internal infrastructure object.
            get { return null; }
            // Don't allow a set either.
            set { throw new InvalidOperationException();}
        }

        //IActivator::ActivatorLevel
        public virtual ActivatorLevel Level
        {
            get {return ActivatorLevel.AppDomain;}
        }
        
        private static MethodBase GetMethodBase(IConstructionCallMessage msg)
        {
            MethodBase mb = msg.MethodBase;
            if(null == mb)      
            {
                BCLDebug.Trace("REMOTE", "Method missing w/name ", msg.MethodName);
                    throw new RemotingException(
                        String.Format(
                            Environment.GetResourceString(
                                "Remoting_Message_MethodMissing"),
                            msg.MethodName,
                            msg.TypeName));
            }
                    
            return mb;
        }

        //IActivator::Activate
        public virtual IConstructionReturnMessage Activate(
            IConstructionCallMessage ctorMsg)
        {
            // This is where the activation service hooks in to activation
            // requests. We get called as the activation message is recognized
            // by the ClientContextTerminatorSink & routed to us. 
            //
            // NOTE: This gets called for both purely within appDomain activation
            // and 'real' remote activation scenarios as the request goes out of
            // the client context.
            // It also gets called as an incoming remote call is routed to the
            // local activator by the remote activator object.
            //
            BCLDebug.Log("Activation Services:: new Activate()");
            if (ctorMsg == null)
            {
                throw new ArgumentNullException("ctorMsg");
            }
            
            // Check if we have marked this activation to go remote 
            if (ctorMsg.Properties.Contains(ActivationServices.RemoteActivateKey))
            {
                //DBG Console.WriteLine("Attempting remote activation!");
                                
                return DoRemoteActivation(ctorMsg);
            }
            else
            {                
                // We must be in either a pure cross context activation or
                // a remote incoming activation request (in which case we
                // already checked the permission to create an instance of
                // this type).
                if (ctorMsg.Properties.Contains(ActivationServices.PermissionKey))
                { 
                    Type activationType = ctorMsg.ActivationType;                    

                    // We are on the server end of a real remote activation
                    // Create a local attribute that contributes the context
                    // properties requested by the remote request
                    Object[] attr = null;
                    if (activationType.IsContextful)
                    {
                    IList cp = ctorMsg.ContextProperties;
                    if (cp != null && cp.Count > 0)
                    {
                        RemotePropertyHolderAttribute rph = new RemotePropertyHolderAttribute(cp);
                        attr = new Object[1];
                        attr[0] = rph;
                    }
                    }

                    MethodBase mb = GetMethodBase(ctorMsg); 
                    RemotingMethodCachedData methodCache = 
                                            InternalRemotingServices.GetReflectionCachedData(mb);
                    Object[] args = Message.CoerceArgs(ctorMsg, methodCache.Parameters);
                    
                    Object server = Activator.CreateInstance(
                        activationType, 
                        args,
                        attr);

                    // check to see if we need to do redirection
                    if (RemotingServices.IsClientProxy(server))
                    {
                        // The wellknown type is remoted so we must wrap the proxy
                        // with a local object.

                        // The redirection proxy masquerades as an object of the appropriate
                        // type, and forwards incoming messages to the actual proxy.
                        RedirectionProxy redirectedProxy = 
                            new RedirectionProxy((MarshalByRefObject)server, activationType);
                        RemotingServices.MarshalInternal(redirectedProxy, null, activationType);

                        server = redirectedProxy;
                    }                        
                     
                    return ActivationServices.SetupConstructionReply(
                        server, ctorMsg, null);
                }
                else
                {
                    BCLDebug.Log("Attempting X-Context activation!");
                    // delegate to the Activator in the message 
                    return ctorMsg.Activator.Activate(ctorMsg);
                }
            }
        }


        // This is called by the local activator during an outgoing activation
        // request.
        internal static IConstructionReturnMessage DoRemoteActivation(
            IConstructionCallMessage ctorMsg)
        {
            BCLDebug.Assert(ctorMsg != null, "Null ctorMsg");


            
            BCLDebug.Log("Attempting Connection to remote activation service");
            IActivator remActivator = null;
            String remActivatorURL = (String)
                ctorMsg.Properties[ActivationServices.RemoteActivateKey];
            try 
            {
                remActivator = (IActivator) 
                    RemotingServices.Connect(
                        typeof(System.Runtime.Remoting.Activation.IActivator),
                        remActivatorURL);
                                    
            }
            catch (Exception e)
            {
                throw new RemotingException(
                    String.Format(
                        Environment.GetResourceString(
                            "Remoting_Activation_ConnectFailed"),
                        e));
            }
            // Remove the remote activate key as its purpose is served.
            ctorMsg.Properties.Remove(ActivationServices.RemoteActivateKey);

            // Delegate the work to the remote activator
            return remActivator.Activate(ctorMsg);                        
        }
        
    }// class LocalActivator

    // This is the object that listens to activation requests
    internal class ActivationListener:MarshalByRefObject, IActivator
    {
        // Override lifetime services to make this object live forever...
        public override Object InitializeLifetimeService()
        {
           return null;
        }

        //IActivator::NextActivator
        public virtual IActivator NextActivator 
        {
            // We are a singleton internal infrastructure object.
            get { return null; }
            // Don't allow a set either.
            set { throw new InvalidOperationException();}
        }

        //IActivator::ActivatorLevel
        public virtual ActivatorLevel Level
        {
            get {return ActivatorLevel.AppDomain;}
        }
    
        //IActivator::Activate
        public virtual IConstructionReturnMessage Activate(
            IConstructionCallMessage ctorMsg)
        {
            BCLDebug.Log("ActivationListener: received new activation request!");
            if (ctorMsg == null || RemotingServices.IsTransparentProxy(ctorMsg))
            {
                throw new ArgumentNullException("ctorMsg");
            }
            
            // Add the permission key which distinguishes pure-cross context activation from
            // a remote request (both of which go through DoCrossContextActivation)
            ctorMsg.Properties[ActivationServices.PermissionKey] = "allowed";

            // Check to make sure that this activation type has been allowed.
            Type activationType = ctorMsg.ActivationType;
            if (activationType == null)
            {
                throw new RemotingException(
                    String.Format(
                        Environment.GetResourceString("Remoting_BadType"),
                        ctorMsg.ActivationTypeName));
            }

            if (!RemotingConfigHandler.IsActivationAllowed(activationType))
            {
                throw new RemotingException(
                    String.Format(
                        Environment.GetResourceString(
                            "Remoting_Activation_PermissionDenied"),
                        ctorMsg.ActivationTypeName));
            }
            
            // Delegate to the local activator for further work
            return ActivationServices.GetActivator().Activate(ctorMsg);
        }        

        
    }   // class ActivationListener


    // This is a lightweight object to help with the activation
    // at the appDomain level ... it delegates its work to 
    // ActivationServices.LocalActivator ... which is a heavy 
    // object we can't afford to carry around in the ActivatorChain
    // (since it may get Serialized/Deserialized)
    internal class AppDomainLevelActivator : IActivator
    {
        IActivator m_NextActivator;        

        // Do we need this?
        String m_RemActivatorURL;
        
        internal AppDomainLevelActivator(String remActivatorURL)
        {
            BCLDebug.Assert(remActivatorURL!=null,"Bad activator URL");
            m_RemActivatorURL = remActivatorURL;
        }

        internal AppDomainLevelActivator(SerializationInfo info, StreamingContext context) {
            if (info==null)
            {
                throw new ArgumentNullException("info");
            }
            m_NextActivator = (IActivator) info.GetValue("m_NextActivator",typeof(IActivator));
        }

        //IActivator::NextActivator
        public virtual IActivator NextActivator 
        {            
            get { return m_NextActivator; }
            set { m_NextActivator = value; }
        }

        //IActivator::ActivatorLevel
        public virtual ActivatorLevel Level
        {
            get { return ActivatorLevel.AppDomain; }
        }

        //IActivator::Activate
        public virtual IConstructionReturnMessage Activate(
            IConstructionCallMessage ctorMsg)
        {
            // This function will get invoked when the ClientContextTerminator sink
            // notices that an activation request message is passed to it ... it
            // will route the message to the Activator present inside the ctorMsg
            // (which happens to be us in this case!)

            // remove ourselves from the Activator chain
            ctorMsg.Activator = m_NextActivator;
            return ActivationServices.GetActivator().Activate(ctorMsg);            
        }
    }

    // This is a lightweight object to help with the activation
    // at the context level ...     
    [Serializable]
    internal class ContextLevelActivator : IActivator
    {                
        IActivator m_NextActivator;
        internal ContextLevelActivator()
        {
            m_NextActivator = null;
        }
        
        internal ContextLevelActivator(SerializationInfo info, StreamingContext context) 
        {
            if (info==null)
            {
                throw new ArgumentNullException("info");
            }
            m_NextActivator = (IActivator) info.GetValue("m_NextActivator",typeof(IActivator));
        }

        //IActivator::NextActivator
        public virtual IActivator NextActivator 
        {            
            get { return m_NextActivator; }
            set { m_NextActivator = value; }
        }

        //IActivator::ActivatorLevel
        public virtual ActivatorLevel Level
        {
            get { return ActivatorLevel.Context; }
        }

        //IActivator::Activate
        public virtual IConstructionReturnMessage Activate(
            IConstructionCallMessage ctorMsg)
        {
            // remove ourselves from the Activator chain
            ctorMsg.Activator = ctorMsg.Activator.NextActivator;
            
            // Delegate to remoting services to do the hard work.
            // This will create a context, enter it, run through
            // the context sink chain & then delegate to the nex
            // activator inside the ctorMsg (quite likely to be 
            // the default ConstructionLevelActivator)
            return ActivationServices.DoCrossContextActivation(ctorMsg);
        }        
    }

    // This is a lightweight object to help with the activation
    // at the appDomain level ... it delegates its work to 
    // ActivationServices.LocalActivator ... which is a heavy 
    // object we can't afford to carry around in the ActivatorChain
    // (since it may get Serialized/Deserialized)
    [Serializable]
    internal class ConstructionLevelActivator : IActivator
    {                
        internal ConstructionLevelActivator()
        {
        
        }


        //IActivator::NextActivator
        public virtual IActivator NextActivator 
        {            
            // The construction level activator is a terminating activator
            get { return null; }
            // Throw if someone attempts a set
            set { throw new InvalidOperationException(); }
        }

        //IActivator::ActivatorLevel
        public virtual ActivatorLevel Level
        {
            get { return ActivatorLevel.Construction; }
        }

        //IActivator::Activate
        public virtual IConstructionReturnMessage Activate(
            IConstructionCallMessage ctorMsg)
        {
            // This function will get invoked when the ClientContextTerminator sink
            // notices that an activation request message is passed to it ... it
            // will route the message to the Activator present inside the ctorMsg
            // (which happens to be us in this case!)

            // remove ourselves from the Activator chain
            ctorMsg.Activator = ctorMsg.Activator.NextActivator;
            return ActivationServices.DoServerContextActivation(ctorMsg);            
        }
    }

    // This acts as a pseudo call site attribute and transfers 
    // the context properties carried in from a remote activation request
    // to the server side activation 
    internal class RemotePropertyHolderAttribute : IContextAttribute
    {
        IList _cp;      // incoming list of context properties
        internal RemotePropertyHolderAttribute(IList cp)
        {
            _cp = cp;
            BCLDebug.Assert(cp != null && cp.Count > 0,"Bad _cp?");
        }
        public virtual bool IsContextOK(Context ctx, IConstructionCallMessage msg)
        {
            // The fact that we got instantiated means some remote activation
            // has contributed non-default context properties to the ctorMsg
            return false;
        }


        // properties are collected in order from callsite, wom & type 
        // attributes ... so we get a first shot at adding properties
        public virtual void GetPropertiesForNewContext(IConstructionCallMessage ctorMsg)
        {
            for (int i=0; i<_cp.Count; i++)
            {
                // Just cycle through the list and add the properties to 
                // the construction message.
                // We will throw at a later stage if any of these do not
                // implement IContextProperty
                ctorMsg.ContextProperties.Add(_cp[i]);
            }
        }
    }
        
}//namespace
