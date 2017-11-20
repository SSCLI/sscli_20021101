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
//============================================================
//
// File:    MetaDataServices
// Purpose: Defines APIs for Metadata
//
// Date:    April 01, 2000
//
//============================================================
namespace System.Runtime.Remoting.MetadataServices
{
    using System;
    using System.Threading;
    using System.Collections;
    using System.Reflection;
    using System.Xml;
    using System.Diagnostics;
    using System.IO;
    using System.Net;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting.Channels;
    using System.Runtime.Remoting.Metadata;

    using Microsoft.CSharp;

    /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData"]/*' />
    public class MetaData
    {

        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.ConvertTypesToSchemaToFile"]/*' />
        public static void ConvertTypesToSchemaToFile(Type[] types, SdlType sdlType, String path)
        {
			Util.Log("MetaData.ConvertTypesToSchemaToFile 1 "+path);						
            ConvertTypesToSchemaToStream(types, sdlType, File.Create(path));
        } // ConvertTypesToSchemaToFile

        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.ConvertTypesToSchemaToStream"]/*' />
        public static void ConvertTypesToSchemaToStream(Type[] types, SdlType sdlType, Stream outputStream)
        {
			Util.Log("MetaData.ConvertTypesToSchemaToFile 2 ");						

            ServiceType[] serviceTypes = new ServiceType[types.Length];
            for (int i=0; i<types.Length; i++)
                serviceTypes[i] = new ServiceType(types[i]);

            ConvertTypesToSchemaToStream(serviceTypes, sdlType, outputStream);

        } // ConvertTypesToSchemaToStream


        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.ConvertTypesToSchemaToFile1"]/*' />
        public static void ConvertTypesToSchemaToFile(ServiceType[] types, SdlType sdlType, String path)
        {
            Util.Log("MetaData.ConvertTypesToSchemaToFile 3 "+path);
            ConvertTypesToSchemaToStream(types, sdlType, File.Create(path));
        } // ConvertTypesToSchemaToFile


        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.ConvertTypesToSchemaToFile2"]/*' />
        public static void ConvertTypesToSchemaToStream(ServiceType[] serviceTypes, SdlType sdlType, Stream outputStream)
        {
			Util.Log("MetaData.ConvertTypesToSchemaToStream 4");			
			TextWriter tw = new StreamWriter(outputStream, new UTF8Encoding(false, true));
            SUDSGenerator sgen = new SUDSGenerator(serviceTypes, sdlType, tw);
            sgen.Generate();
            tw.Flush();

        } // ConvertTypesToSchemaToStream

        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.RetrieveSchemaFromUrlToStream"]/*' />
        public static void RetrieveSchemaFromUrlToStream(String url, Stream outputStream)
        {
            WebRequest              Request;
            WebResponse             Response;
            Stream                  RespStream;
			Util.Log("MetaData.RetrieveSchemaFromUrlToStream "+url);						
            TextWriter tw = new StreamWriter(outputStream, new UTF8Encoding(false, true));

            Request = WebRequest.Create(url);
            Response = Request.GetResponse();
            RespStream = Response.GetResponseStream();
			StreamReader sr = new StreamReader(RespStream);
			tw.Write(sr.ReadToEnd());
            tw.Flush();
        }

        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.RetrieveSchemaFromUrlToFile"]/*' />
        public static void RetrieveSchemaFromUrlToFile(String url, String path)
        {
			Util.Log("MetaData.RetrieveSchemaFromUrlToFile "+url+" file "+path);									
            RetrieveSchemaFromUrlToStream(url, File.Create(path));
        }




        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.ConvertSchemaStreamToCodeSourceStream"]/*' />
        public static void ConvertSchemaStreamToCodeSourceStream(bool clientProxy, String outputDirectory, Stream inputStream, ArrayList outCodeStreamList, String proxyUrl, String proxyNamespace)
        {
			Util.Log("MetaData.ConvertSchemaStreamToCodeSourceStream 1 "+outputDirectory+" proxyUrl "+proxyNamespace);									
            TextReader input;

            input = (TextReader) new StreamReader(inputStream);

            SUDSParser parser = new SUDSParser(input, outputDirectory, outCodeStreamList, proxyUrl, clientProxy, proxyNamespace);
            parser.Parse();
        }

        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.ConvertSchemaStreamToCodeSourceStream2"]/*' />
        public static void ConvertSchemaStreamToCodeSourceStream(bool clientProxy, String outputDirectory, Stream inputStream, ArrayList outCodeStreamList, String proxyUrl)
        {
			Util.Log("MetaData.ConvertSchemaStreamToCodeSourceStream 3 "+outputDirectory);									
          ConvertSchemaStreamToCodeSourceStream(clientProxy, outputDirectory, inputStream, outCodeStreamList, proxyUrl, "");
        }

        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.ConvertSchemaStreamToCodeSourceStream1"]/*' />
        public static void ConvertSchemaStreamToCodeSourceStream(bool clientProxy, String outputDirectory, Stream inputStream, ArrayList outCodeStreamList)
        {
			Util.Log("MetaData.ConvertSchemaStreamToCodeSourceStream 2 "+outputDirectory);									
          ConvertSchemaStreamToCodeSourceStream(clientProxy, outputDirectory, inputStream, outCodeStreamList, "", "");
        }
        

        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.ConvertCodeSourceStreamToAssemblyFile"]/*' />
        public static void ConvertCodeSourceStreamToAssemblyFile(ArrayList outCodeStreamList, String assemblyPath, String strongNameFilename)
        {
            throw new NotImplementedException("Not Implemented in Rotor");
        }


/*
/target:module -> Name="target", Value="module"
/target:library -> Name="target", Value="library"
/main:MyClass -> Name="m", Value="MyClass"
/debug+ -> Name="debug", Value=true

// The dictionary of options takes almost ALL of the normal command-line options, but only using their 'short form' and without the preceding slash or dash.
*/

        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.ConvertCodeSourceFileToAssemblyFile"]/*' />
        public static void ConvertCodeSourceFileToAssemblyFile(String codePath, String assemblyPath, String strongNameFilename)
        {
			Util.Log("MetaData.ConvertCodeSourceFileToAssemblyFile codePath "+codePath+" assemblyPath "+assemblyPath);												
            ArrayList arrayList = new ArrayList();
            arrayList.Add(codePath);
            ConvertCodeSourceStreamToAssemblyFile(arrayList, assemblyPath, strongNameFilename);
        }

        // Helpers

        /// <include file='doc\MetaData.uex' path='docs/doc[@for="MetaData.SaveStreamToFile"]/*' />
        public static void SaveStreamToFile(Stream inputStream, String path)
        {
			Util.Log("MetaData.SaveStreamToFile "+path);			
            Stream outputStream = File.Create(path);
            TextWriter tw = new StreamWriter(outputStream, new UTF8Encoding(false, true));
			StreamReader sr = new StreamReader(inputStream);
			tw.Write(sr.ReadToEnd());
            tw.Flush();
            tw.Close();
            outputStream.Close();
        }


    } // class MetaData
        
        
    /// <include file='doc\MetaData.uex' path='docs/doc[@for="ServiceType"]/*' />
    public class ServiceType
        {
        private Type    _type;  // Type of object being exported.
        private String  _url;   // This may be null if no address is available.
        
        /// <include file='doc\MetaData.uex' path='docs/doc[@for="ServiceType.ServiceType"]/*' />
        public ServiceType(Type type)
        {
            _type = type;
            _url = null;
        } // ServiceType

        /// <include file='doc\MetaData.uex' path='docs/doc[@for="ServiceType.ServiceType1"]/*' />
        public ServiceType(Type type, String url)
            {
            _type = type;
            _url = url;
        } // ServiceType

        /// <include file='doc\MetaData.uex' path='docs/doc[@for="ServiceType.ObjectType"]/*' />
        public Type   ObjectType { get { return _type; } }
        /// <include file='doc\MetaData.uex' path='docs/doc[@for="ServiceType.Url"]/*' />
        public String Url { get { return _url; } }        
       
    } // ServiceType
        
        

} // namespace System.Runtime.Remoting.MetadataServices

