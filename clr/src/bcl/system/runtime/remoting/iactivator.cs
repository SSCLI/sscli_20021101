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
** File:    IActivator.cs
**
**                                                    
**
** Purpose: Defines the interface provided by activation services
**          
**
** Date:    Jun 26, 1999
**
===========================================================*/
namespace System.Runtime.Remoting.Activation {

    using System;
    using System.Runtime.Remoting.Messaging;
    using System.Collections;
    using System.Security.Permissions;
    
    /// <include file='doc\IActivator.uex' path='docs/doc[@for="IActivator"]/*' />
    public interface IActivator
    {
        /// <include file='doc\IActivator.uex' path='docs/doc[@for="IActivator.NextActivator"]/*' />
        // return the next activator in the chain
        IActivator NextActivator 
        {
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
	    get; 
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
	    set;
	}
        
           /// <include file='doc\IActivator.uex' path='docs/doc[@for="IActivator.Activate"]/*' />
        // New method for activators.
	[SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
        IConstructionReturnMessage Activate(IConstructionCallMessage msg);     
           /// <include file='doc\IActivator.uex' path='docs/doc[@for="IActivator.Level"]/*' />

           // Returns the level at which this activator is active ..
           // Should return one of the ActivatorLevels below
        ActivatorLevel Level 
	    { 
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
	    get;
	    }

           
    }

    /// <include file='doc\IActivator.uex' path='docs/doc[@for="ActivatorLevel"]/*' />
	[Serializable]
    public enum ActivatorLevel
    {
        /// <include file='doc\IActivator.uex' path='docs/doc[@for="ActivatorLevel.Construction"]/*' />
        Construction = 4,
        /// <include file='doc\IActivator.uex' path='docs/doc[@for="ActivatorLevel.Context"]/*' />
        Context = 8,
        /// <include file='doc\IActivator.uex' path='docs/doc[@for="ActivatorLevel.AppDomain"]/*' />
        AppDomain = 12,
        /// <include file='doc\IActivator.uex' path='docs/doc[@for="ActivatorLevel.Process"]/*' />
        Process = 16,
        /// <include file='doc\IActivator.uex' path='docs/doc[@for="ActivatorLevel.Machine"]/*' />
        Machine = 20
    }

    /// <include file='doc\IActivator.uex' path='docs/doc[@for="IConstructionCallMessage"]/*' />
    public interface IConstructionCallMessage : IMethodCallMessage
    {
        /// <include file='doc\IActivator.uex' path='docs/doc[@for="IConstructionCallMessage.Activator"]/*' />
        IActivator Activator                   
        { 
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
	    get;
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
	    set;
	}
        /// <include file='doc\IActivator.uex' path='docs/doc[@for="IConstructionCallMessage.CallSiteActivationAttributes"]/*' />
        Object[] CallSiteActivationAttributes  
        {
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
	     get;
	}
        /// <include file='doc\IActivator.uex' path='docs/doc[@for="IConstructionCallMessage.ActivationTypeName"]/*' />
        String ActivationTypeName               
        {
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
	    get;
        }
        /// <include file='doc\IActivator.uex' path='docs/doc[@for="IConstructionCallMessage.ActivationType"]/*' />
        Type ActivationType                     
        { 
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
	    get;
        }
        /// <include file='doc\IActivator.uex' path='docs/doc[@for="IConstructionCallMessage.ContextProperties"]/*' />
        IList ContextProperties                
        {
	    [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.Infrastructure)]	
	     get;
        }
    }
    
    /// <include file='doc\IActivator.uex' path='docs/doc[@for="IConstructionReturnMessage"]/*' />
    public interface IConstructionReturnMessage : IMethodReturnMessage
    {
    }
    
}
