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
// File:    SudsWriter.cs
// Purpose: Defines SUDSParser that parses a given SUDS document
//          and generates types defined in it.
//
// Date:    April 01, 2000
// Revised: November 15, 2000 (Wsdl)                       
//
//============================================================
namespace System.Runtime.Remoting.MetadataServices
{
	using System;
    using System.IO;
    using System.Reflection;

	// Represents exceptions thrown by the SUDSGenerator
    /// <include file='doc\SudsWriter.uex' path='docs/doc[@for="SUDSGeneratorException"]/*' />
	public class SUDSGeneratorException : Exception
	{
		internal SUDSGeneratorException(String msg)
				: base(msg)
		{
		}
	}

	// This class generates SUDS documents
	internal class SUDSGenerator
	{
        WsdlGenerator wsdlGenerator = null;
        SdlGenerator sdlGenerator = null;
        SdlType sdlType;
		// Constructor
		internal SUDSGenerator(Type[] types, TextWriter output)
		{
			Util.Log("SUDSGenerator.SUDSGenerator 1");
            wsdlGenerator = new WsdlGenerator(types, output);
            sdlType = SdlType.Wsdl;
		}

		// Constructor
		internal SUDSGenerator(Type[] types, SdlType sdlType, TextWriter output)
		{
			Util.Log("SUDSGenerator.SUDSGenerator 2");
            if (sdlType == SdlType.Sdl)
                sdlGenerator = new SdlGenerator(types, sdlType, output);
            else
                wsdlGenerator = new WsdlGenerator(types, sdlType, output);
            this.sdlType = sdlType;
		}

		// Constructor
		internal SUDSGenerator(Type[] types, TextWriter output, Assembly assembly, String url)
		{
			Util.Log("SUDSGenerator.SUDSGenerator 3 "+url);
            wsdlGenerator = new WsdlGenerator(types, output, assembly, url);
            sdlType = SdlType.Wsdl;
		}

		// Constructor
		internal SUDSGenerator(Type[] types, SdlType sdlType, TextWriter output, Assembly assembly, String url)
		{
			Util.Log("SUDSGenerator.SUDSGenerator 4 "+url);			
            if (sdlType == SdlType.Sdl)
                sdlGenerator = new SdlGenerator(types, sdlType, output, assembly, url);
            else
                wsdlGenerator = new WsdlGenerator(types, sdlType, output, assembly, url);
            this.sdlType =sdlType;
		}

		internal SUDSGenerator(ServiceType[] serviceTypes, SdlType sdlType, TextWriter output)
		{
			Util.Log("SUDSGenerator.SUDSGenerator 5 ");
            if (sdlType == SdlType.Sdl)
                sdlGenerator = new SdlGenerator(serviceTypes, sdlType, output);
            else
                wsdlGenerator = new WsdlGenerator(serviceTypes, sdlType, output);
            this.sdlType = sdlType;
		}


		// Generates SUDS
		internal void Generate()
		{
			Util.Log("SUDSGenerator.Generate");			
            if (sdlType == SdlType.Sdl)
                sdlGenerator.Generate();
            else
                wsdlGenerator.Generate();
        }
    }
}

