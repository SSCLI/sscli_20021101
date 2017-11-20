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
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace System.Reflection.Emit {
    using System;
    using IList = System.Collections.IList;
    using ArrayList = System.Collections.ArrayList;
    using System.Reflection;
    using System.Security;
    using System.Diagnostics;
    using CultureInfo = System.Globalization.CultureInfo;
    using ResourceWriter = System.Resources.ResourceWriter;
    using MemoryStream = System.IO.MemoryStream;
    
    // This is a package private class. This class hold all of the managed
    // data member for AssemblyBuilder. Note that what ever data members added to
    // this class cannot be accessed from the EE.
    internal class AssemblyBuilderData
    {
        internal AssemblyBuilderData(
            Assembly            assembly, 
            String              strAssemblyName, 
            AssemblyBuilderAccess access,
            String              dir)
        {
            m_assembly = assembly;
            m_strAssemblyName = strAssemblyName;
            m_access = access;
            m_moduleBuilderList = new ArrayList();
            m_resWriterList = new ArrayList();
            m_publicComTypeList = null;
            m_CABuilders = null;
            m_CABytes = null;
            m_CACons = null;
            m_iPublicComTypeCount = 0;    
            m_iCABuilder = 0;
            m_iCAs = 0;
            m_entryPointModule = null;
            m_isSaved = false;
            if (dir == null && access != AssemblyBuilderAccess.Run)
                m_strDir = Environment.CurrentDirectory;
            else
                m_strDir = dir;
            m_RequiredPset = null;
            m_OptionalPset = null;
            m_RefusedPset = null;
            m_isSynchronized = false;

            m_InMemoryAssemblyModule = null;
            m_OnDiskAssemblyModule = null;
            m_peFileKind = PEFileKinds.Dll;

            m_entryPointMethod = null;
            m_ISymWrapperAssembly = null;
         }
    
        // Helper to add a dynamic module into the tracking list
        internal void AddModule(ModuleBuilder dynModule)
        {
            m_moduleBuilderList.Add(dynModule);

            if(m_assembly != null) 
                // also add the Module into the file list in the Assembly.
                m_assembly.nAddFileToInMemoryFileList(dynModule.m_moduleData.m_strFileName, dynModule);
        }
    
        // Helper to add a resource information into the tracking list
        internal void AddResWriter(ResWriterData resData)
        {
            m_resWriterList.Add(resData);
        }


        // Helper to track CAs to persist onto disk
        internal void AddCustomAttribute(CustomAttributeBuilder customBuilder)
        {

            // make sure we have room for this CA
            if (m_CABuilders == null)
            {
                m_CABuilders = new CustomAttributeBuilder[m_iInitialSize];
            }
            if (m_iCABuilder == m_CABuilders.Length)
            {
                CustomAttributeBuilder[]  tempCABuilders = new CustomAttributeBuilder[m_iCABuilder * 2];
                Array.Copy(m_CABuilders, tempCABuilders, m_iCABuilder);
                m_CABuilders = tempCABuilders;            
            }
            m_CABuilders[m_iCABuilder] = customBuilder;
            
            m_iCABuilder++;
        }

        // Helper to track CAs to persist onto disk
        internal void AddCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
        {

            // make sure we have room for this CA
            if (m_CABytes == null)
            {
                m_CABytes = new byte[m_iInitialSize][];
                m_CACons = new ConstructorInfo[m_iInitialSize];                
            }
            if (m_iCAs == m_CABytes.Length)
            {
                // enlarge the arrays
                byte[][]  temp = new byte[m_iCAs * 2][];
                ConstructorInfo[] tempCon = new ConstructorInfo[m_iCAs * 2];
                for (int i=0; i < m_iCAs; i++)
                {
                    temp[i] = m_CABytes[i];
                    tempCon[i] = m_CACons[i];
                }
                m_CABytes = temp;
                m_CACons = tempCon;
            }

            byte[] attrs = new byte[binaryAttribute.Length];
            Array.Copy(binaryAttribute, attrs, binaryAttribute.Length);
            m_CABytes[m_iCAs] = attrs;
            m_CACons[m_iCAs] = con;
            m_iCAs++;
        }

        
        // Helper to ensure the resource name is unique underneath assemblyBuilder
        internal void CheckResNameConflict(String strNewResName)
        {
            int         size = m_resWriterList.Count;
            int         i;
            for (i = 0; i < size; i++) 
            {
                ResWriterData resWriter = (ResWriterData) m_resWriterList[i];
                if (resWriter.m_strName.Equals(strNewResName))
                {
                    // Cannot have two resources with the same name
                    throw new ArgumentException(Environment.GetResourceString("Argument_DuplicateResourceName"));
                }
            }        
        }

    
        // Helper to ensure the module name is unique underneath assemblyBuilder
        internal void CheckNameConflict(String strNewModuleName)
        {
            int         size = m_moduleBuilderList.Count;
            int         i;
            for (i = 0; i < size; i++) 
            {
                ModuleBuilder moduleBuilder = (ModuleBuilder) m_moduleBuilderList[i];
                if (moduleBuilder.m_moduleData.m_strModuleName.Equals(strNewModuleName))
                {
                    // Cannot have two dynamic modules with the same name
                    throw new ArgumentException(Environment.GetResourceString("Argument_DuplicateModuleName"));
                }
            }
            // Also check to see if there is any loaded module with the same name.
            // This is to take care of the case where a Dynamic module might be
            // added to a static assembly.
            if (!(m_assembly is AssemblyBuilder))
            {
                if (m_assembly.GetModule(strNewModuleName) != null)
                {
                    // Cannot have two dynamic modules with the same name
                    throw new ArgumentException(Environment.GetResourceString("Argument_DuplicateModuleName"));
                }
            }
        }
    
        // Helper to ensure the type name is unique underneath assemblyBuilder
        internal void CheckTypeNameConflict(String strTypeName, TypeBuilder enclosingType)
        {
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: AssemblyBuilderData.CheckTypeNameConflict( " + strTypeName + " )"); 
            for (int i = 0; i < m_moduleBuilderList.Count; i++) 
            {
                ModuleBuilder curModule = (ModuleBuilder)m_moduleBuilderList[i];
                for (int j = 0; j < curModule.m_TypeBuilderList.Count; j++)
                {
                    Type curType = (Type)curModule.m_TypeBuilderList[j];
                    if (curType.FullName.Equals(strTypeName) && curType.DeclaringType == enclosingType)
                    {
                        // Cannot have two types with the same name
                        throw new ArgumentException(Environment.GetResourceString("Argument_DuplicateTypeName"));
                    }
                }
            }

            // Also check to see if there is any loaded type with the same name.
            // You only need to make this test for non-nested types since any
            // duplicates in nested types will be caught at the top level.
            if ((enclosingType == null) && !(m_assembly is AssemblyBuilder))
            {
                if (m_assembly.GetTypeInternal(strTypeName, false, false, false) != null)
                {
                    // Cannot have two types with the same name
                    throw new ArgumentException(Environment.GetResourceString("Argument_DuplicateTypeName"));
                }
            }
        }
        

        // Helper to ensure the file name is unique underneath assemblyBuilder. This includes 
        // modules and resources.
        internal void CheckFileNameConflict(String strFileName)
        {
            int         size = m_moduleBuilderList.Count;
            int         i;
            for (i = 0; i < size; i++) 
            {
                ModuleBuilder moduleBuilder = (ModuleBuilder) m_moduleBuilderList[i];
                if (moduleBuilder.m_moduleData.m_strFileName != null)
                {
                    if (String.Compare(moduleBuilder.m_moduleData.m_strFileName, strFileName, true, CultureInfo.InvariantCulture) == 0)
                    {
                        // Cannot have two dynamic module with the same name
                        throw new ArgumentException(Environment.GetResourceString("Argument_DuplicatedFileName"));
                    }
                }
            }    
            size = m_resWriterList.Count;
            for (i = 0; i < size; i++) 
            {
                ResWriterData resWriter = (ResWriterData) m_resWriterList[i];
                if (resWriter.m_strFileName != null)
                {
                    if (String.Compare(resWriter.m_strFileName, strFileName, true, CultureInfo.InvariantCulture) == 0)
                    {
                        // Cannot have two dynamic module with the same name
                        throw new ArgumentException(Environment.GetResourceString("Argument_DuplicatedFileName"));
                    }
                }
            }    

        }
    
        // Helper to look up which module that assembly is supposed to be stored at
        internal ModuleBuilder FindModuleWithFileName(String strFileName)
        {
            int         size = m_moduleBuilderList.Count;
            int         i;
            for (i = 0; i < size; i++) 
            {
                ModuleBuilder moduleBuilder = (ModuleBuilder) m_moduleBuilderList[i];
                if (moduleBuilder.m_moduleData.m_strFileName != null)
                {
                    if (String.Compare(moduleBuilder.m_moduleData.m_strFileName, strFileName, true, CultureInfo.InvariantCulture) == 0)
                    {
                        return moduleBuilder;
                    }
                }
            } 
            return null;       
        }

        // Helper to look up module by name.
        internal ModuleBuilder FindModuleWithName(String strName)
        {
            int         size = m_moduleBuilderList.Count;
            int         i;
            for (i = 0; i < size; i++) 
            {
                ModuleBuilder moduleBuilder = (ModuleBuilder) m_moduleBuilderList[i];
                if (moduleBuilder.m_moduleData.m_strModuleName != null)
                {
                    if (String.Compare(moduleBuilder.m_moduleData.m_strModuleName, strName, true, CultureInfo.InvariantCulture) == 0)
                        return moduleBuilder;
                }
            } 
            return null;       
        }

        
        // add type to public COMType tracking list
        internal void AddPublicComType(Type type)
        {
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: AssemblyBuilderData.AddPublicComType( " + type.FullName + " )"); 
            if (m_isSaved == true)
            {
                // assembly has been saved before!
                throw new InvalidOperationException(Environment.GetResourceString(
                    "InvalidOperation_CannotAlterAssembly"));
            }        
            EnsurePublicComTypeCapacity();
            m_publicComTypeList[m_iPublicComTypeCount] = type;
            m_iPublicComTypeCount++;
        }
        
        // add security permission requests
        internal void AddPermissionRequests(
            PermissionSet       required,
            PermissionSet       optional,
            PermissionSet       refused)
        {
            BCLDebug.Log("DYNIL","## DYNIL LOGGING: AssemblyBuilderData.AddPermissionRequests");
            if (m_isSaved == true)
            {
                // assembly has been saved before!
                throw new InvalidOperationException(Environment.GetResourceString(
                    "InvalidOperation_CannotAlterAssembly"));
            }        
            m_RequiredPset = required;
            m_OptionalPset = optional;
            m_RefusedPset = refused;
        }
        
        internal void EnsurePublicComTypeCapacity()
        {
            if (m_publicComTypeList == null)
            {
                m_publicComTypeList = new Type[m_iInitialSize];
            }
            if (m_iPublicComTypeCount == m_publicComTypeList.Length)
            {
                Type[]  tempTypeList = new Type[m_iPublicComTypeCount * 2];
                Array.Copy(m_publicComTypeList, tempTypeList, m_iPublicComTypeCount);
                m_publicComTypeList = tempTypeList;            
            }
        }
         
        internal void Enter()
        {
            if (m_isSynchronized)
                System.Threading.Monitor.Enter(this);
        }
        
        internal void Exit()
        {
            if (m_isSynchronized)
                System.Threading.Monitor.Exit(this);
        }

        internal ModuleBuilder GetInMemoryAssemblyModule()
        {
            if (m_InMemoryAssemblyModule == null)
            {
                ModuleBuilder modBuilder = m_assembly.nGetInMemoryAssemblyModule();
                modBuilder.Init("RefEmit_InMemoryManifestModule", null, null);
                m_InMemoryAssemblyModule = modBuilder;
            }
            return m_InMemoryAssemblyModule;
        }

        internal ModuleBuilder GetOnDiskAssemblyModule()
        {
            if (m_OnDiskAssemblyModule == null)
            {
                ModuleBuilder modBuilder = m_assembly.nGetOnDiskAssemblyModule();
                modBuilder.Init("RefEmit_OnDiskManifestModule", null, null);
                m_OnDiskAssemblyModule = modBuilder;
            }
            return m_OnDiskAssemblyModule;
        }

        internal void SetOnDiskAssemblyModule(ModuleBuilder modBuilder)
        {
            m_OnDiskAssemblyModule = modBuilder;
        }

        internal ArrayList              m_moduleBuilderList;
        internal ArrayList              m_resWriterList;
        internal String                 m_strAssemblyName;
        internal AssemblyBuilderAccess  m_access;
        internal Assembly               m_assembly;
        
        internal Type[]                 m_publicComTypeList;
        internal int                    m_iPublicComTypeCount;
        
        internal ModuleBuilder          m_entryPointModule;
        internal bool                   m_isSaved;    
        internal static int             m_iInitialSize = 16;
        internal String                 m_strDir;

        // hard coding the assembly def token
        internal static int             m_tkAssembly = 0x20000001;
    
        // Security permission requests
        internal PermissionSet          m_RequiredPset;
        internal PermissionSet          m_OptionalPset;
        internal PermissionSet          m_RefusedPset;
        
        // Assembly builder may be optionally synchronized.
        internal bool                   m_isSynchronized;

        // tracking AssemblyDef's CAs for persistence to disk
        internal CustomAttributeBuilder[] m_CABuilders;
        internal int                    m_iCABuilder;
        internal byte[][]               m_CABytes;
        internal ConstructorInfo[]      m_CACons;
        internal int                    m_iCAs;
        internal PEFileKinds            m_peFileKind;           // assembly file kind
        private  ModuleBuilder          m_InMemoryAssemblyModule;
        private  ModuleBuilder          m_OnDiskAssemblyModule;
        internal MethodInfo             m_entryPointMethod;
        internal Assembly               m_ISymWrapperAssembly;
                                  

    }

    
    /**********************************************
    *
    * Internal structure to track the list of ResourceWriter for
    * AssemblyBuilder & ModuleBuilder.
    *
    **********************************************/
    internal class ResWriterData 
    {
        internal ResWriterData(
            ResourceWriter  resWriter,
            MemoryStream    memoryStream,
            String          strName,
            String          strFileName,
            String          strFullFileName,
            ResourceAttributes attribute)
        {
            m_resWriter = resWriter;
            m_memoryStream = memoryStream;
            m_strName = strName;
            m_strFileName = strFileName;
            m_strFullFileName = strFullFileName;
            m_nextResWriter = null;
            m_attribute = attribute;
        }

        internal ResourceWriter         m_resWriter;
        internal String                 m_strName;
        internal String                 m_strFileName;
        internal String                 m_strFullFileName;
        internal MemoryStream           m_memoryStream;
        internal ResWriterData          m_nextResWriter;
        internal ResourceAttributes     m_attribute;
    }


}
