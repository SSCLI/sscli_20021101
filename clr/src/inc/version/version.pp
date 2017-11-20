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

[assembly: AssemblyKeyFileAttribute(KEY_LOCATION)]
[assembly: AssemblyDelaySign(true)]
[assembly: AssemblyVersionAttribute(VER_ASSEMBLYVERSION_STR)]

// Tell the ResourceManager our fallback language is US English.
[assembly: NeutralResourcesLanguageAttribute("en-US")]

// Tell the ResourceManager to look for this version of the satellites.
[assembly: SatelliteContractVersionAttribute(VER_ASSEMBLYVERSION_STR)]

namespace Util {

    // This is a preprocessed file that will generate a C# class with
    // the runtime version string in here.  This way managed tools can be
    // compiled with their appropriate version number built into them, instead
    // of using the current installed version of the runtime and without
    // having to preprocess your managed tool's source itself.
    internal class Version
    {
        public const String VersionString = VER_FILEVERSION_STR;
        public const String SBSVersionString = VER_SBSFILEVERSION_STR;
    }

}
