//------------------------------------------------------------------------------
// <copyright file="DiagnosticsConfiguration.cs" company="Microsoft">
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

namespace System.Diagnostics {
    using System;
    using System.Reflection;
    using System.Collections;
    using System.Configuration;

    internal enum InitState {
        NotInitialized,
        Initializing,
        Initialized
    }

    internal class DiagnosticsConfiguration {
        private static Hashtable configTable;
        private static InitState initState = InitState.NotInitialized;        


        // setting for Switch.switchSetting
        internal static IDictionary SwitchSettings {
            get { 
                Initialize();
                if (configTable != null && configTable.ContainsKey("switches"))
                    return (IDictionary)configTable["switches"];
                else
                    return null;
            }
        }

        // setting for DefaultTraceListener.AssertUIEnabled
        internal static bool AssertUIEnabled {
            get { 
                Initialize();
                if (configTable != null && configTable.ContainsKey("assertuienabled"))
                    return (bool)configTable["assertuienabled"];
                else
                    return true; // the default
            }
        }

        // setting for DefaultTraceListener.LogFileName
        internal static string LogFileName {
            get { 
                Initialize();
                if (configTable != null && configTable.ContainsKey("logfilename"))
                    return (string)configTable["logfilename"];
                else
                    return string.Empty; // the default
            }
        }

        // setting for TraceInternal.AutoFlush
        internal static bool AutoFlush {
            get { 
                Initialize();
                if (configTable != null && configTable.ContainsKey("autoflush"))
                    return (bool)configTable["autoflush"];
                else
                    return false; // the default
            }
        }

        // setting for TraceInternal.IndentSize
        internal static int IndentSize {
            get { 
                Initialize();
                if (configTable != null && configTable.ContainsKey("indentsize"))
                    return (int)configTable["indentsize"];
                else
                    return 4; // the default
            }
        }

        
        private static Hashtable GetConfigTable() {
            Hashtable configTable = (Hashtable) ConfigurationSettings.GetConfig("system.diagnostics");
            return configTable;
        }

        internal static bool IsInitializing() {
            return initState == InitState.Initializing;
        }

        internal static bool CanInitialize() {
            return (initState != InitState.Initializing) && !(ConfigurationSettings.SetConfigurationSystemInProgress);
        }
        
        internal static void Initialize() {
            // because some of the code used to load config also uses diagnostics
            // we can't block them while we initialize from config. Therefore we just
            // return immediately and they just use the default values.
            lock (typeof(DiagnosticsConfiguration)) {

                if (initState != InitState.NotInitialized || ConfigurationSettings.SetConfigurationSystemInProgress)
                    return;

                initState = InitState.Initializing; // used for preventing recursion
                try {
                    configTable = GetConfigTable();
                }
                finally {
                    initState = InitState.Initialized;
                }
            }
        }
    }
}

