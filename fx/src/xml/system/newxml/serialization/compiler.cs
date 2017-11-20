//------------------------------------------------------------------------------
// <copyright file="Compiler.cs" company="Microsoft">
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

namespace System.Xml.Serialization {
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Collections;
    using System.IO;
    using System;
    using System.Text;
    using System.ComponentModel;
    using System.CodeDom.Compiler;
    using System.Security;
    using System.Security.Permissions;

    internal class Compiler {
        private FileIOPermission fileIOPermission;

        Hashtable imports = new Hashtable();
        StringWriter writer = new StringWriter();

        protected string[] Imports {
            get { 
                string[] array = new string[imports.Values.Count];
                imports.Values.CopyTo(array, 0);
                return array;
            }
        }
        
        internal void AddImport(Type type) {
            Type baseType = type.BaseType;
            if (baseType != null)
                AddImport(baseType);
            foreach (Type intf in type.GetInterfaces())
                AddImport(intf);
            FileIOPermission.Assert();
            Module module = type.Module;
            Assembly assembly = module.Assembly;
            if (module is ModuleBuilder || assembly.Location.Length == 0) {
                throw new InvalidOperationException(Res.GetString(Res.XmlCompilerDynModule, type.FullName, module.Name));
            }
            
            if (imports[assembly] == null) 
                imports.Add(assembly, assembly.Location);
        }

        private FileIOPermission FileIOPermission {
            get {
                if (fileIOPermission == null)
                    fileIOPermission = new FileIOPermission(PermissionState.Unrestricted);
                return fileIOPermission;
            }
        }

        internal TextWriter Source {
            get { return writer; }
        }

        internal void Close() { }

        internal Assembly Compile() {
            CodeDomProvider codeProvider = new Microsoft.CSharp.CSharpCodeProvider();
            ICodeCompiler compiler = codeProvider.CreateCompiler();
            CompilerParameters options = new CompilerParameters(Imports);

            options.WarningLevel = 1;
            options.TreatWarningsAsErrors = true;
            options.CompilerOptions = "/nowarn:183,184,602,612,626,672,679,1030,1200,1201,1202,1203,1522,1570,1574,1580,1581,1584,1589,1590,1592,1596,1598,1607,2002,2014,2023,3012,5000";
            
            if (CompModSwitches.KeepTempFiles.Enabled) {
                options.GenerateInMemory = false;
                options.IncludeDebugInformation = true;
                options.TempFiles = new TempFileCollection();
                options.TempFiles.KeepFiles = true;
            }
            else {
                options.GenerateInMemory = true;
            }

            PermissionSet perms = new PermissionSet(PermissionState.Unrestricted);
            perms.AddPermission(FileIOPermission);
            perms.AddPermission(new EnvironmentPermission(PermissionState.Unrestricted));
            perms.Assert();

            CompilerResults results = null;
            Assembly assembly = null;
            try {
                results = compiler.CompileAssemblyFromSource(options, writer.ToString());
                assembly = results.CompiledAssembly;
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }


            if (results.Errors.HasErrors) {
                StringWriter stringWriter = new StringWriter();
                stringWriter.WriteLine(Res.GetString(Res.XmlCompilerError, results.NativeCompilerReturnValue.ToString()));
                foreach (CompilerError e in results.Errors) {
                    // clear filename. This makes ToString() print just error number and message.
                    e.FileName = "";
                    stringWriter.WriteLine(e.ToString());
                }
                System.Diagnostics.Debug.WriteLine(stringWriter.ToString());
                throw new InvalidOperationException(stringWriter.ToString());
            }

            // somehow we got here without generating an assembly
            if (assembly == null) throw new InvalidOperationException(Res.GetString(Res.XmlInternalError));
            
            return assembly;
        }
    }
}


