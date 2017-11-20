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
using System;
using System.Reflection;
using System.Runtime;
using System.Resources;
using System.Security;

[assembly: AssemblyKeyFileAttribute(KEY_LOCATION)]
[assembly: AssemblyDelaySign(true)]
[assembly: AssemblyVersionAttribute(VER_ASSEMBLYVERSION_STR)]

[assembly: SatelliteContractVersionAttribute(VER_ASSEMBLYVERSION_STR)]
[assembly: NeutralResourcesLanguageAttribute("en-US")]

[assembly:AllowPartiallyTrustedCallers()]

namespace Microsoft.JScript {

    // This is a preprocessed file that will generate a C# class with
    // the runtime version string in here.  This way managed tools can be
    // compiled with their appropriate version number built into them, instead
    // of using the current installed version of the runtime and without
    // having to preprocess your managed tool's source itself.
    internal class BuildVersionInfo
    {
	public const int    MajorVersion = COR_BUILD_YEAR;
	public const int    MinorVersion = COR_BUILD_MONTH; 
	public const int    Build = CLR_OFFICIAL_ASSEMBLY_NUMBER;
	public const int    RevisionNumber = CLR_PRIVATE_ASSEMBLY_NUMBER;
	public const String AssemblyVersionString = VER_ASSEMBLYVERSION_STR;
        public const String FileVersionString = VER_FILEVERSION_STR;
        public const String SBSVersionString = VER_SBSFILEVERSION_STR;
    }

}
