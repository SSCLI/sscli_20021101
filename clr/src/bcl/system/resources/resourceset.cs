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
** Class:  ResourceSet
**
**                                                    
**
** Purpose: Culture-specific collection of resources.
**
** Date:  March 26, 1999
** 
===========================================================*/
namespace System.Resources {
    using System;
    using System.Collections;
    using System.IO;
    using System.Runtime.Remoting.Activation;
    using System.Globalization;
    using System.Security.Permissions;

    // A ResourceSet stores all the resources defined in one particular CultureInfo.
    // 
    // The method used to load resources is straightforward - this class
    // enumerates over an IResourceReader, loading every name and value, and 
    // stores them in a hash table.  Custom IResourceReaders can be used.
    // 
    /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet"]/*' />
    [Serializable()]
    public class ResourceSet : IDisposable
    {
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.Reader"]/*' />
        protected IResourceReader Reader;
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.Table"]/*' />
        protected Hashtable Table;

        private Hashtable _caseInsensitiveTable;  // For case-insensitive lookups.
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.ResourceSet"]/*' />
        protected ResourceSet()
        {
            // To not inconvenience people subclassing us, we should allocate a new
            // hashtable here just so that Table is set to something.
            Table = new Hashtable();
        }
        
        // Creates a ResourceSet using the system default ResourceReader
        // implementation.  Use this constructor to open & read from a file 
        // on disk.
        // 
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.ResourceSet1"]/*' />
        public ResourceSet(String fileName)
        {
            Reader = new ResourceReader(fileName);
            Table = new Hashtable();
            ReadResources();
        }
    
        // Creates a ResourceSet using the system default ResourceReader
        // implementation.  Use this constructor to read from an open stream 
        // of data.
        // 
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.ResourceSet2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]
        public ResourceSet(Stream stream)
        {
            Reader = new ResourceReader(stream);
            Table = new Hashtable();
            ReadResources();
        }
    

        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.ResourceSet3"]/*' />
        public ResourceSet(IResourceReader reader)
        {
            if (reader == null)
                throw new ArgumentNullException("reader");
            Reader = reader;
            Table = new Hashtable();
            ReadResources();
        }
    
        // Closes and releases any resources used by this ResourceSet, if any.
        // All calls to methods on the ResourceSet after a call to close may 
        // fail.  Close is guaranteed to be safely callable multiple times on a 
        // particular ResourceSet, and all subclasses must support these semantics.
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.Close"]/*' />
        public virtual void Close()
        {
            Dispose(true);
        }
        
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.Dispose"]/*' />
        protected virtual void Dispose(bool disposing)
        {
            if (disposing) {
                // Close the Reader in a thread-safe way.  Note that calling
                // Close multiple times needs to be thread-safe.
                IResourceReader copyOfReader = Reader;
                if (copyOfReader != null)
                    copyOfReader.Close();
            }
            Reader = null;
            _caseInsensitiveTable = null;
            Table = null;
        }

        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.Dispose1"]/*' />
        public void Dispose()
        {
            Dispose(true);
        }

        // Returns the preferred IResourceReader class for this kind of ResourceSet.
        // Subclasses of ResourceSet using their own Readers &; should override
        // GetDefaultReader and GetDefaultWriter.
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.GetDefaultReader"]/*' />
        public virtual Type GetDefaultReader()
        {
            return typeof(ResourceReader);
        }
    
        // Returns the preferred IResourceWriter class for this kind of ResourceSet.
        // Subclasses of ResourceSet using their own Readers &; should override
        // GetDefaultReader and GetDefaultWriter.
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.GetDefaultWriter"]/*' />
        public virtual Type GetDefaultWriter()
        {
            return typeof(ResourceWriter);
        }
        
        // Look up a string value for a resource given its name.
        // 
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.GetString"]/*' />
        public virtual String GetString(String name)
        {
            if (Table == null)
                throw new InvalidOperationException(Environment.GetResourceString("Arg_ClosedResourceTable"));
            if (name==null)
                throw new ArgumentNullException("name");
    
            try {
                return (String) Table[name];
            }
            catch (InvalidCastException) {
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ResourceNotString_Name", name));
            }
        }

        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.GetString2"]/*' />
        public virtual String GetString(String name, bool ignoreCase)
        {
            if (Table == null)
                throw new InvalidOperationException(Environment.GetResourceString("Arg_ClosedResourceTable"));
            if (name==null)
                throw new ArgumentNullException("name");

            String s;
            try {
                s = (String) Table[name];
            }
            catch (InvalidCastException) {
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ResourceNotString_Name", name));
            }
            if (s != null || !ignoreCase)
                return s;

            // Try doing a case-insensitive lookup.
            if (_caseInsensitiveTable == null) {
                _caseInsensitiveTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
#if _DEBUG
                //Console.WriteLine("ResourceSet::GetString loading up data.  ignoreCase: "+ignoreCase);
                BCLDebug.Perf(!ignoreCase, "Using case-insensitive lookups is bad perf-wise.  Consider capitalizing "+name+" correctly in your source");
#endif
                IDictionaryEnumerator en = Reader.GetEnumerator();
                while (en.MoveNext()) {
                    _caseInsensitiveTable.Add(en.Key, en.Value);
                }
            }
            try {
                return (String) _caseInsensitiveTable[name];
            }
            catch (InvalidCastException) {
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_ResourceNotString_Name", name));
            }
        }
        
        // Look up an object value for a resource given its name.
        // 
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.GetObject"]/*' />
        public virtual Object GetObject(String name)
        {
            if (Table == null)
                throw new InvalidOperationException(Environment.GetResourceString("Arg_ClosedResourceTable"));
            if (name==null)
                throw new ArgumentNullException("name");
            
            return Table[name];
        }

        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.GetObject2"]/*' />
        public virtual Object GetObject(String name, bool ignoreCase)
        {
            if (Table == null)
                throw new InvalidOperationException(Environment.GetResourceString("Arg_ClosedResourceTable"));
            if (name==null)
                throw new ArgumentNullException("name");
            
            Object obj = Table[name];
            if (obj != null || !ignoreCase)
                return obj;

            // Try doing a case-insensitive lookup.
            if (_caseInsensitiveTable == null) {
                _caseInsensitiveTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
#if _DEBUG
                //Console.WriteLine("ResourceSet::GetObject loading up case-insensitive data");
                BCLDebug.Perf(!ignoreCase, "Using case-insensitive lookups is bad perf-wise.  Consider capitalizing "+name+" correctly in your source");
#endif
                IDictionaryEnumerator en = Reader.GetEnumerator();
                while (en.MoveNext()) {
                    _caseInsensitiveTable.Add(en.Key, en.Value);
                }
            }

            return _caseInsensitiveTable[name];
        }
    
        /// <include file='doc\ResourceSet.uex' path='docs/doc[@for="ResourceSet.ReadResources"]/*' />
        protected virtual void ReadResources()
        {
            IDictionaryEnumerator en = Reader.GetEnumerator();
            while (en.MoveNext()) {
                Table.Add(en.Key, en.Value);
            }
            // While technically possible to close the Reader here, don't close it
            // to help with some WinRes lifetime issues.
        }
    }
}
