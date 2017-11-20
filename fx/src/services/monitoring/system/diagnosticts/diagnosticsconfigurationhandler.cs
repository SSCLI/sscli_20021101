//------------------------------------------------------------------------------
// <copyright file="DiagnosticsConfigurationHandler.cs" company="Microsoft">
//     
//      Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//     
//      The use and distribution terms for this software are contained in the file
//      named license.txt, which can be found in the root of this distribution.
//      By using this software in any fashion, you are agreeing to be bound by the
//      terms of this license.
//     
//      You must not remove this notice, or any other, from this software.
//     
// </copyright>                                                                
//------------------------------------------------------------------------------

#if !LIB
#define TRACE
#define DEBUG
namespace System.Diagnostics {

    using System;
    using System.Collections;
    using System.Diagnostics;    
    using System.Xml;
    using System.Configuration;
    using System.Reflection;

    /// <include file='doc\DiagnosticsConfigurationHandler.uex' path='docs/doc[@for="DiagnosticsConfigurationHandler"]/*' />
    /// <devdoc>
    ///    The configuration section handler for the diagnostics section of the configuration
    ///    file. The section handler participates in the resolution of configuration settings 
    ///    between the &lt;diagnostics&gt; and &lt;/diagnostics&gt;portion of the .config file.
    /// </devdoc>
    /// <internalonly/>
    public class DiagnosticsConfigurationHandler : IConfigurationSectionHandler {

        /// <include file='doc\DiagnosticsConfigurationHandler.uex' path='docs/doc[@for="DiagnosticsConfigurationHandler.Create"]/*' />
        /// <devdoc>
        ///    <para>Parses the configuration settings between the 
        ///       &lt;diagnostics&gt; and &lt;/diagnostics&gt; portion of the .config file to populate
        ///       the values of 'WebServicesConfiguration' object and returning it.
        ///    </para>
        /// </devdoc>
        /// <internalonly/>
        public virtual object Create(object parent, object configContext, XmlNode section) {
            // Since the tracing and switch code lives in System.Dll and config is in System.Configuration.dll
            // the settings just go into a hashtable to communicate to the values to the diagnostics code in System.dll
            Hashtable parentConfig = (Hashtable)parent;
            Hashtable config;
            if (parentConfig == null)
                config = new Hashtable();
            else
                config = (Hashtable)parentConfig.Clone();

            foreach (XmlNode child in section.ChildNodes) {
                if (HandlerBase.IsIgnorableAlsoCheckForNonElement(child))
                    continue;

                switch (child.Name) {
                    case "switches":                        
                        HandleSwitches(config, child, configContext);
                        break;
                    case "assert":
                        HandleAssert(config, child, configContext);
                        break;
                    case "trace":
                        HandleTrace(config, child, configContext);
                        break;
                    default:
                        HandlerBase.ThrowUnrecognizedElement(child);
                        break;
                } // switch(child.Name)
                
                HandlerBase.CheckForUnrecognizedAttributes(child);
            }
            return config;
        }

        private static void HandleSwitches(Hashtable config, XmlNode switchesNode, object context) {
            config["switches"] = new SwitchesDictionarySectionHandler().Create(config["switches"], context, switchesNode);
        }

        private static void HandleAssert(Hashtable config, XmlNode assertNode, object context) {
            bool assertuienabled = false;
            if (HandlerBase.GetAndRemoveBooleanAttribute(assertNode, "assertuienabled", ref assertuienabled) != null)
                config["assertuienabled"] = assertuienabled;

            string logfilename = null;
            if (HandlerBase.GetAndRemoveStringAttribute(assertNode, "logfilename", ref logfilename) != null)
                config["logfilename"] = logfilename;

            HandlerBase.CheckForChildNodes(assertNode);
        }

        
        private static void HandleTrace(Hashtable config, XmlNode traceNode, object context) {
            bool autoflush = false;
            if (HandlerBase.GetAndRemoveBooleanAttribute(traceNode, "autoflush", ref autoflush) != null)
                config["autoflush"] = autoflush;
                                       
            int indentsize = 0;
            if (HandlerBase.GetAndRemoveIntegerAttribute(traceNode, "indentsize", ref indentsize) != null)
                config["indentsize"] = indentsize;

            foreach (XmlNode traceChild in traceNode.ChildNodes) {
                if (HandlerBase.IsIgnorableAlsoCheckForNonElement(traceChild))
                    continue;
                
                if (traceChild.Name == "listeners") {
                    HandleListeners(config, traceChild, context);
                }
                else {
                    HandlerBase.ThrowUnrecognizedElement(traceChild);
                }
            }
        }

        private static void HandleListeners(Hashtable config, XmlNode listenersNode, object context) {
            foreach (XmlNode listenersChild in listenersNode.ChildNodes) {
                if (HandlerBase.IsIgnorableAlsoCheckForNonElement(listenersChild))
                    continue;
                
                string name = null, className = null, initializeData = null;
                string op = listenersChild.Name;
                
                switch (op) {
                    case "add":
                    case "remove":
                    case "clear":
                        break;
                    default:
                        HandlerBase.ThrowUnrecognizedElement(listenersChild);
                        break;
                }

                HandlerBase.GetAndRemoveStringAttribute(listenersChild, "name", ref name);
                HandlerBase.GetAndRemoveStringAttribute(listenersChild, "type", ref className);
                HandlerBase.GetAndRemoveStringAttribute(listenersChild, "initializeData", ref initializeData);
                HandlerBase.CheckForUnrecognizedAttributes(listenersChild);

                TraceListener newListener = null;
                if (className != null) {  
                    Type t = Type.GetType(className);

                    if (t == null)
                        throw new ConfigurationException(SR.GetString(SR.Could_not_find_type, className));

                    // create a listener with parameterless constructor 
                    if (initializeData == null) {
                        ConstructorInfo ctorInfo = t.GetConstructor(new Type[] {});
                        if (ctorInfo == null)
                            throw new ConfigurationException(SR.GetString(SR.Could_not_get_constructor, className));
                        newListener = (TraceListener)(ctorInfo.Invoke(new object[] {}));
                    }
                    // create a listener with a one-string constructor
                    else {
                        ConstructorInfo ctorInfo = t.GetConstructor(new Type[] { typeof(string) });
                        if (ctorInfo == null)
                            throw new ConfigurationException(SR.GetString(SR.Could_not_get_constructor, className));
                        newListener = (TraceListener)(ctorInfo.Invoke(new object[] { initializeData }));
                    }
                    if (name != null) {
                        newListener.Name = name;
                    }
                }

                switch (op) {
                    case "add":
                        if (newListener == null)
                            throw new ConfigurationException(SR.GetString(SR.Could_not_create_listener));  
    
                        Trace.Listeners.Add(newListener);
    
                        break;
                    case "remove":
                        if (newListener == null) {
                            // no type specified, we'll have to delete by name
    
                            // if no name is specified we can't do anything
                            if (name == null)
                                throw new ConfigurationException(SR.GetString(SR.Cannot_remove_with_null));
    
                            Trace.Listeners.Remove(name);
                        }
                        else {
                            // remove by listener
                            Trace.Listeners.Remove(newListener);
                        }
                        break;
                    case "clear":
                        Trace.Listeners.Clear();
                        break;
                    default:
                        HandlerBase.ThrowUnrecognizedElement(listenersChild);
                        break;
                }
            }
        }
    }

    /// <include file='doc\DiagnosticsConfigurationHandler.uex' path='docs/doc[@for="SwitchesDictionarySectionHandler"]/*' />
    internal class SwitchesDictionarySectionHandler : DictionarySectionHandler {
        protected override string KeyAttributeName {
            get { return "name";}
        }
    }
}

#endif

