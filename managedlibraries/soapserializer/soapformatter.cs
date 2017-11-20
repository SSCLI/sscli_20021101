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
// Class: SoapFormatter
// Purpose: Soap XML Formatter
//
// Date:  June 10, 1999
//
//============================================================
 
[assembly:System.CLSCompliant(true)]

namespace System.Runtime.Serialization.Formatters.Soap
{

	using System;
	using System.Runtime.Serialization.Formatters;
	using System.IO;
	using System.Reflection;
	using System.Globalization;
	using System.Collections;
	using System.Runtime.Serialization;
	using System.Runtime.Remoting;	
	using System.Runtime.Remoting.Messaging;	
	using System.Text;


	/// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter"]/*' />

	sealed public class SoapFormatter : IRemotingFormatter
	{
		private SoapParser soapParser = null;
		private ISurrogateSelector m_surrogates;
		private StreamingContext m_context;
		private FormatterTypeStyle m_typeFormat = FormatterTypeStyle.TypesWhenNeeded;
		private ISoapMessage m_topObject = null;
		//private FormatterAssemblyStyle m_assemblyFormat = FormatterAssemblyStyle.Simple;		
        private FormatterAssemblyStyle m_assemblyFormat = FormatterAssemblyStyle.Full;		
        private SerializationBinder m_binder;
		private Stream currentStream = null;


    	// Property which specifies an object of type ISoapMessage into which
		// the SoapTop object is serialized. Should only be used if the Soap 
		// top record is a methodCall or methodResponse element. 
		/// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.TopObject"]/*' />
		public ISoapMessage TopObject
		{
			get {return m_topObject;}
			set {m_topObject = value;}
		}

    	// Property which specifies how types are serialized,
    	// FormatterTypeStyle Enum specifies options
		/// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.TypeFormat"]/*' />
		public FormatterTypeStyle TypeFormat
		{
			get {return m_typeFormat;}
			set
			{
				// Reset the value if TypesWhenNeeded
				// Or the value for TypesAlways and XsdString
				if (value == FormatterTypeStyle.TypesWhenNeeded)
					m_typeFormat = FormatterTypeStyle.TypesWhenNeeded;
				else
					m_typeFormat |= value;
			}
		}		

    	// Property which specifies how types are serialized,
    	// FormatterAssemblyStyle Enum specifies options
		/// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.AssemblyFormat"]/*' />
		public FormatterAssemblyStyle AssemblyFormat
		{
			get {return m_assemblyFormat;}
			set {m_assemblyFormat = value;}
		}	
		
    	// Constructor
		/// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.SoapFormatter"]/*' />
		public SoapFormatter()
		{
			m_surrogates=null;
			m_context = new StreamingContext(StreamingContextStates.All);
		}

    	// Constructor
		/// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.SoapFormatter1"]/*' />
		public SoapFormatter(ISurrogateSelector selector, StreamingContext context)
		{
			m_surrogates = selector;
			m_context = context;
		}

    	// Deserialize the stream into an object graph.
		/// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.Deserialize"]/*' />
		public Object Deserialize(Stream serializationStream)
		{
			return Deserialize(serializationStream, null);
		}

    	// Deserialize the stream into an object graph.
		/// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.Deserialize1"]/*' />
		public Object Deserialize(Stream serializationStream, HeaderHandler handler) {
			InternalST.InfoSoap("Enter SoapFormatter.Deserialize ");			
            if (serializationStream==null) {
                throw new ArgumentNullException("serializationStream");
            }

			if (serializationStream.CanSeek && (serializationStream.Length == 0))
				throw new SerializationException(SoapUtil.GetResourceString("Serialization_Stream"));								

			InternalST.Soap( this, "Deserialize Entry");
			InternalFE formatterEnums = new InternalFE();
			formatterEnums.FEtypeFormat = m_typeFormat;
			formatterEnums.FEtopObject = m_topObject;
			formatterEnums.FEserializerTypeEnum = InternalSerializerTypeE.Soap;
			formatterEnums.FEassemblyFormat = m_assemblyFormat;
			ObjectReader sor = new ObjectReader(serializationStream, m_surrogates, m_context, formatterEnums, m_binder);

			// If this is the first call, or a new stream is being used a new Soap parser is created.
			// If this is a continuing call, then the existing SoapParser is used.
			// One stream can contains multiple Soap XML documents. The XMLParser buffers the XML so
			// that the same XMLParser has to be used to continue a stream.
			if ((soapParser == null) || (serializationStream != currentStream))
			{
				soapParser = new SoapParser(serializationStream);
				currentStream = serializationStream;
			}
			soapParser.Init(sor);
			Object obj = sor.Deserialize(handler, soapParser);
			InternalST.InfoSoap("Leave SoapFormatter.Deserialize ");
			return obj;
		}

		/// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.Serialize"]/*' />
		public void Serialize(Stream serializationStream, Object graph)
		{
			Serialize(serializationStream, graph, null);
		}

    	// Commences the process of serializing the entire graph.  All of the data (in the appropriate format)
    	// is emitted onto the stream.

		/// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.Serialize1"]/*' />
		public void Serialize(Stream serializationStream, Object graph, Header[] headers)
		{
			InternalST.InfoSoap("Enter SoapFormatter.Serialize ");
            if (serializationStream==null) {
                throw new ArgumentNullException("serializationStream");
            }

			InternalST.Soap( this, "Serialize Entry");
			InternalFE formatterEnums = new InternalFE();
			formatterEnums.FEtypeFormat = m_typeFormat;
			formatterEnums.FEtopObject = m_topObject;
			formatterEnums.FEserializerTypeEnum = InternalSerializerTypeE.Soap;
			formatterEnums.FEassemblyFormat = m_assemblyFormat;
			ObjectWriter sow = new ObjectWriter(serializationStream, m_surrogates, m_context, formatterEnums);
			sow.Serialize(graph, headers, new SoapWriter(serializationStream));
			InternalST.InfoSoap("Leave SoapFormatter.Serialize ");			
		}

        /// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.SurrogateSelector"]/*' />
        public ISurrogateSelector SurrogateSelector {
            get {
                return m_surrogates;
            }

            set {
                m_surrogates = value;
            }
        }

        /// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.Binder"]/*' />
        public SerializationBinder Binder {
            get {
                return m_binder;
            }

            set {
                m_binder=value;
            }
        }

        /// <include file='doc\SoapFormatter.uex' path='docs/doc[@for="SoapFormatter.Context"]/*' />
        public StreamingContext Context {
            get {
                return m_context;
            }

            set {
                m_context = value;
            }
        }
	}
}


