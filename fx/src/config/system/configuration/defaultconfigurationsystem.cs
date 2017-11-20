//------------------------------------------------------------------------------
// <copyright file="DefaultConfigurationSystem.cs" company="Microsoft">
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

// config system: finds config files, loads config
// factories, filters out relevant config file sections, and
// feeds them to the factories to create config objects.

#if !LIB

namespace System.Configuration {

    using System.Collections;
    using System.IO;
    using System.Xml;
    using System.Diagnostics;
    using System.Security;
    using System.Security.Permissions;
    using Assembly = System.Reflection.Assembly;
    using StringBuilder = System.Text.StringBuilder;

    internal class DefaultConfigurationSystem : IConfigurationSystem {

        // string constants
        private const string ConfigExtension = "config";
        private const string MachineConfigFilename = "machine.config";
        private const string MachineConfigSubdirectory = "config";
        private const int MaxPathSize = 1024;
        //private ConfigurationRecord _machine;
        private ConfigurationRecord _application;

        internal DefaultConfigurationSystem() {
        }


        object IConfigurationSystem.GetConfig(string configKey) {
            if (_application != null) {
                return _application.GetConfig(configKey);
            }

            throw new InvalidOperationException(SR.GetString(SR.Client_config_init_error));
        }


        void IConfigurationSystem.Init() {
            lock (this) {
                if (_application == null) {
                    ConfigurationRecord machine = null;
                    
                    string machineFilename = MachineConfigurationFilePath;
                    Uri appFilename = AppConfigPath;

                    ConfigurationRecord.TraceVerbose("opening Machine config file     \"" + machineFilename + "\"");
                    _application = machine = new ConfigurationRecord();
                    bool machineFileExists = machine.Load(machineFilename);
                    
                    // only load application config if machine.config exists
                    if (machineFileExists && appFilename != null) {
                        ConfigurationRecord.TraceVerbose("opening Application config file \"" + appFilename + "\"");
                        _application = new ConfigurationRecord(machine);
                        _application.Load(appFilename.ToString());
                    }
                }
            }
        }


        internal static String MsCorLibDirectory {
            get {
                // location of mscorlib.dll
                string filename;
                FileIOPermission perm = new FileIOPermission(PermissionState.None);
                perm.AllFiles = FileIOPermissionAccess.PathDiscovery;
                perm.Assert();
                try {
                    filename = Assembly.GetAssembly(typeof(object)).Location.Replace('/', '\\');
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
                return Path.GetDirectoryName(filename);
            }
        }

        internal static string MachineConfigurationFilePath {
            get {
                return Path.Combine(Path.Combine(MsCorLibDirectory, MachineConfigSubdirectory), MachineConfigFilename);
            }
        }


        internal static Uri AppConfigPath {
            get {                
                // FileIOPermission is required for ApplicationBase in URL-hosted domains
                FileIOPermission perm = new FileIOPermission(PermissionState.Unrestricted);
                perm.Assert();
                try {
                    AppDomainSetup setup = AppDomain.CurrentDomain.SetupInformation;
                    string appBase = setup.ApplicationBase;
                
                    // we need to ensure AppBase ends in an '/'.                                  
                    if (appBase.Length > 0) {
                        char lastChar = appBase[appBase.Length - 1];
                        if (lastChar != '/' && lastChar != '\\') {
                            appBase += '/';
                        }
                    }
                    Uri uri = new Uri(appBase);
                    string config = setup.ConfigurationFile;
                    if (config != null && config.Length > 0) {
                        uri = new Uri(uri, config);
                        return uri;
                    }
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }
                return null;
            }
        }
    }



}

#endif

