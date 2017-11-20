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
//-------------------------------------------------------------
// FusionInterfaces.cs
//
// This implements wrappers to Fusion interfaces
//-------------------------------------------------------------
namespace Microsoft.Win32
{
    using System;
    using System.IO;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;
    using System.Globalization;     
    using StringBuilder = System.Text.StringBuilder;

    internal struct AssemblyInformation
    {
        public String FullName;
        public String Name;
        public String Version;
        public String Locale;
        public String PublicKeyToken;
    }
    
    
    internal class ASM_CACHE
    {
         public const uint ZAP          = 0x1;
         public const uint GAC          = 0x2;
         public const uint DOWNLOAD     = 0x4;
    }

    internal class CANOF
    {
        public const uint PARSE_DISPLAY_NAME = 0x1;
        public const uint SET_DEFAULT_VALUES = 0x2;
    }
    
    internal class ASM_NAME
    {   
         public const uint PUBLIC_KEY            = 0;
         public const uint PUBLIC_KEY_TOKEN      = PUBLIC_KEY + 1;
         public const uint HASH_VALUE            = PUBLIC_KEY_TOKEN + 1;
         public const uint NAME                  = HASH_VALUE + 1;
         public const uint MAJOR_VERSION         = NAME + 1;
         public const uint MINOR_VERSION         = MAJOR_VERSION + 1;
         public const uint BUILD_NUMBER          = MINOR_VERSION + 1;
         public const uint REVISION_NUMBER       = BUILD_NUMBER + 1;
         public const uint CULTURE               = REVISION_NUMBER + 1;
         public const uint PROCESSOR_ID_ARRAY    = CULTURE + 1;
         public const uint OSINFO_ARRAY          = PROCESSOR_ID_ARRAY + 1;
         public const uint HASH_ALGID            = OSINFO_ARRAY + 1;
         public const uint ALIAS                 = HASH_ALGID + 1;
         public const uint CODEBASE_URL          = ALIAS + 1;
         public const uint CODEBASE_LASTMOD      = CODEBASE_URL + 1;
         public const uint NULL_PUBLIC_KEY       = CODEBASE_LASTMOD + 1;
         public const uint NULL_PUBLIC_KEY_TOKEN  = NULL_PUBLIC_KEY + 1;
         public const uint CUSTOM                = NULL_PUBLIC_KEY_TOKEN + 1;
         public const uint NULL_CUSTOM           = CUSTOM + 1;
         public const uint MVID                  = NULL_CUSTOM + 1;
         public const uint _32_BIT_ONLY          = MVID + 1;
         public const uint MAX_PARAMS            = _32_BIT_ONLY + 1;
    }
    
    internal class Fusion
    {
        public static void ReadCache(ArrayList alAssems, String name, uint nFlag)
        {
            IntPtr          aEnum       = IntPtr.Zero;
            IntPtr          aName       = IntPtr.Zero;
            IntPtr          aNameEnum   = IntPtr.Zero;
            IntPtr          AppCtx      = IntPtr.Zero;
            int hr;
            
            try 
            {
                if (name != null) {
                    hr = CreateAssemblyNameObject(out aNameEnum, name, CANOF.PARSE_DISPLAY_NAME, IntPtr.Zero);
                    if (hr != 0)
                        return;
                }
                
                hr = CreateAssemblyEnum(out aEnum, AppCtx, aNameEnum, nFlag, IntPtr.Zero);
                while (hr == 0)
                {
                    hr = GetNextAssembly(aEnum, ref AppCtx, ref aName, 0);
                    if (hr != 0)
                        break;

                    String sDisplayName = GetDisplayName(aName, 0);

                    if (sDisplayName == null)
                        continue;

                    // Our info is in a comma seperated list. Let's pull it out
                    String[] sFields = sDisplayName.Split(new char[] {','});

                    AssemblyInformation newguy = new AssemblyInformation();
                    newguy.FullName = sDisplayName;
                    newguy.Name = sFields[0];
                    // The version string is represented as Version=######
                    // Let's take out the 'Version='
                    newguy.Version = sFields[1].Substring(sFields[1].IndexOf('=')+1);
                    // Same goes for the locale
                    newguy.Locale = sFields[2].Substring(sFields[2].IndexOf('=')+1);
                    // And the  key token
                    sFields[3]=sFields[3].Substring(sFields[3].IndexOf('=')+1);
                    if (sFields[3].Equals("null"))
                        sFields[3] = "null"; 
                    newguy.PublicKeyToken = sFields[3];
                    
                    alAssems.Add(newguy);
                }               
            }
            finally
            {
                ReleaseFusionHandle(ref aEnum);
                ReleaseFusionHandle(ref aName);
                ReleaseFusionHandle(ref aNameEnum);
                ReleaseFusionHandle(ref AppCtx);
            }
        }


        [DllImport(Win32Native.FUSION, CharSet=CharSet.Auto)]
        static extern int CreateAssemblyNameObject(out IntPtr ppEnum, [MarshalAs(UnmanagedType.LPWStr)]String szAssemblyName, uint dwFlags, IntPtr pvReserved);

        [DllImport(Win32Native.FUSION, CharSet=CharSet.Auto)]
        static extern int CreateAssemblyEnum(out IntPtr ppEnum, IntPtr pAppCtx, IntPtr pName, uint dwFlags, IntPtr pvReserved);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern int GetNextAssembly(IntPtr pEnum, ref IntPtr ppAppCtx, ref IntPtr ppName, uint dwFlags);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern String GetDisplayName(IntPtr pName, uint dwDisplayFlags);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        static extern void ReleaseFusionHandle(ref IntPtr pp);
    }
}
