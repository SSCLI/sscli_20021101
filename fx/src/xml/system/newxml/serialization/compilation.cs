//------------------------------------------------------------------------------
// <copyright file="Compilation.cs" company="Microsoft">
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
    using System.Collections;
    using System.IO;
    using System;
    using System.Xml;
    using System.Threading;
    using System.Security;
    using System.Security.Permissions;

    internal class TempAssembly {
        const string GeneratedAssemblyNamespace = "Microsoft.Xml.Serialization.GeneratedAssembly";
        Assembly assembly;
        Type writerType;
        Type readerType;
        TempMethod[] methods;
        static object[] emptyObjectArray = new object[0];
        Hashtable assemblies;
        int assemblyResolveThread;
        bool allAssembliesAllowPartialTrust;

        internal class TempMethod {
            public MethodInfo writeMethod;
            public MethodInfo readMethod;
            public string name;
            public string ns;
        }

        public TempAssembly(XmlMapping[] xmlMappings) {
            Compiler compiler = new Compiler();
            allAssembliesAllowPartialTrust = false;
            try {
                IndentedWriter writer = new IndentedWriter(compiler.Source, false);
                writer.WriteLine("[assembly:System.Security.AllowPartiallyTrustedCallers()]");
                writer.WriteLine("namespace " + GeneratedAssemblyNamespace + " {");
                writer.Indent++;
                writer.WriteLine();

                Hashtable scopeTable = new Hashtable();
                foreach (XmlMapping mapping in xmlMappings)
                    scopeTable[mapping.Scope] = mapping;
                TypeScope[] scopes = new TypeScope[scopeTable.Keys.Count];
                scopeTable.Keys.CopyTo(scopes, 0);
                
                XmlSerializationWriterCodeGen writerCodeGen = new XmlSerializationWriterCodeGen(writer, scopes);
                writerCodeGen.GenerateBegin();
                string[] writeMethodNames = new string[xmlMappings.Length];
                for (int i = 0; i < xmlMappings.Length; i++)
                    writeMethodNames[i] = writerCodeGen.GenerateElement(xmlMappings[i]);
                writerCodeGen.GenerateEnd();
                    
                writer.WriteLine();
                
                XmlSerializationReaderCodeGen readerCodeGen = new XmlSerializationReaderCodeGen(writer, scopes);
                readerCodeGen.GenerateBegin();
                string[] readMethodNames = new string[xmlMappings.Length];
                for (int i = 0; i < xmlMappings.Length; i++)
                    readMethodNames[i] = readerCodeGen.GenerateElement(xmlMappings[i]);
                readerCodeGen.GenerateEnd();
                    
                writer.Indent--;
                writer.WriteLine("}");
                allAssembliesAllowPartialTrust = true;
                assemblies = new Hashtable();
                foreach (TypeScope scope in scopes) {
                    foreach (Type t in scope.Types) {
                        compiler.AddImport(t);
                        Assembly a = t.Assembly;
                        if (allAssembliesAllowPartialTrust && !AssemblyAllowsPartialTrust(a))
                            allAssembliesAllowPartialTrust = false;
                        if (!a.GlobalAssemblyCache)
                            assemblies[a.FullName] = a;
                    }
                }
                compiler.AddImport(typeof(XmlWriter));
                compiler.AddImport(typeof(XmlSerializationWriter));
                compiler.AddImport(typeof(XmlReader));
                compiler.AddImport(typeof(XmlSerializationReader));

                assembly = compiler.Compile();

                methods = new TempMethod[xmlMappings.Length];
                
                readerType = GetTypeFromAssembly("XmlSerializationReader1");
                writerType = GetTypeFromAssembly("XmlSerializationWriter1");

                for (int i = 0; i < methods.Length; i++) {
                    TempMethod method = new TempMethod();
                    XmlTypeMapping xmlTypeMapping = xmlMappings[i] as XmlTypeMapping;
                    if (xmlTypeMapping != null) {
                        method.name = xmlTypeMapping.ElementName;
                        method.ns = xmlTypeMapping.Namespace;
                    }
                    method.readMethod = GetMethodFromType(readerType, readMethodNames[i]);
                    method.writeMethod = GetMethodFromType(writerType, writeMethodNames[i]);
                    methods[i] = method;
                }
            }
            finally {
                compiler.Close();
            }
        }

        // assert ControlEvidence so we can search the assembly's evidence
		[SecurityPermission(SecurityAction.Assert, Flags=SecurityPermissionFlag.ControlEvidence)]
		bool AssemblyAllowsPartialTrust(Assembly a) 
		{
			// assembly allows partial trust if it has APTC attr or is not strong-named signed
			if ( a.GetCustomAttributes(typeof(AllowPartiallyTrustedCallersAttribute), false).Length > 0) 
			{
				// has APTC attr
				return true;
			}
			else 
			{
				// no attr: check for strong-name signing
				bool isStrongNameSigned = false;
				foreach (object o in a.Evidence) 
				{
					if (o is System.Security.Policy.StrongName) 
					{
						isStrongNameSigned = true;
						break;
					}
				}
				return !isStrongNameSigned;
			}
		}

        MethodInfo GetMethodFromType(Type type, string methodName) {
            MethodInfo method = type.GetMethod(methodName);
            if (method != null) return method;
            throw new InvalidOperationException(Res.GetString(Res.XmlMissingMethod, methodName, type.FullName));
        }
        
        Type GetTypeFromAssembly(string typeName) {
            typeName = GeneratedAssemblyNamespace + "." + typeName;
            Type type = assembly.GetType(typeName);
            if (type == null) throw new InvalidOperationException(Res.GetString(Res.XmlMissingType, typeName));
            return type;
        }

        public bool CanRead(int methodIndex, XmlReader xmlReader) {
            TempMethod method = methods[methodIndex];
            return xmlReader.IsStartElement(method.name, method.ns);
        }
        
        public object InvokeReader(int methodIndex, XmlReader xmlReader, XmlDeserializationEvents events) {
            if (!allAssembliesAllowPartialTrust)
                new PermissionSet(PermissionState.Unrestricted).Demand();
            XmlSerializationReader reader = (XmlSerializationReader)Activator.CreateInstance(readerType);
            reader.Init(xmlReader, events);
            ResolveEventHandler resolver = new ResolveEventHandler(this.OnAssemblyResolve);
            assemblyResolveThread = Thread.CurrentThread.GetHashCode();
            AppDomain.CurrentDomain.AssemblyResolve += resolver;
            object ret = methods[methodIndex].readMethod.Invoke(reader, emptyObjectArray);
            AppDomain.CurrentDomain.AssemblyResolve -= resolver;
            assemblyResolveThread = 0;
            return ret;
        }
        
        public void InvokeWriter(int methodIndex, XmlWriter xmlWriter, object o, ArrayList namespaces) {
            if (!allAssembliesAllowPartialTrust)
                new PermissionSet(PermissionState.Unrestricted).Demand();
            XmlSerializationWriter writer = (XmlSerializationWriter)Activator.CreateInstance(writerType);
            writer.Init(xmlWriter, namespaces);
            ResolveEventHandler resolver = new ResolveEventHandler(this.OnAssemblyResolve);
            assemblyResolveThread = Thread.CurrentThread.GetHashCode();
            AppDomain.CurrentDomain.AssemblyResolve += resolver;
            methods[methodIndex].writeMethod.Invoke(writer, new object[] { o });
            AppDomain.CurrentDomain.AssemblyResolve -= resolver;
            assemblyResolveThread = 0;
        }

        public Assembly OnAssemblyResolve(object sender, ResolveEventArgs args) {
            return assemblyResolveThread == Thread.CurrentThread.GetHashCode() && assemblies != null ? (Assembly)assemblies[args.Name] : null;
        }
    }

    internal class TempAssemblyCache {
        Hashtable cache = new Hashtable();

        public TempAssembly this[object key] {
            get { return (TempAssembly)cache[key]; }
        }

        public void Add(object key, TempAssembly assembly) {
            lock(this) {
                if (cache[key] == assembly) return;
                Hashtable clone = new Hashtable();
                foreach (object k in cache.Keys) {
                    clone.Add(k, cache[k]);
                }
                cache = clone;
                cache[key] = assembly;
            }
        }
    }
}

