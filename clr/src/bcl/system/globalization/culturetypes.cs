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
//
// The enumeration constants used in CultureInfo.GetCultures().
//
namespace System.Globalization {    

    /// <include file='doc\CultureTypes.uex' path='docs/doc[@for="CultureTypes"]/*' />
    [Flags, Serializable]
    public enum CultureTypes {
        /// <include file='doc\CultureTypes.uex' path='docs/doc[@for="CultureTypes.NeutralCultures"]/*' />
        NeutralCultures = 0x0001,   // Get all neutral cultures.  Neutral cultures are cultures like "en", "de", "zh", etc.
        /// <include file='doc\CultureTypes.uex' path='docs/doc[@for="CultureTypes.SpecificCultures"]/*' />
        SpecificCultures = 0x0002,              // Get all non-netural cultuers.  Examples are "en-us", "zh-tw", etc.
        /// <include file='doc\CultureTypes.uex' path='docs/doc[@for="CultureTypes.InstalledWin32Cultures"]/*' />
        InstalledWin32Cultures = 0x0004,        // Get all Win32 installed cultures in the system.
        /// <include file='doc\CultureTypes.uex' path='docs/doc[@for="CultureTypes.AllCultures"]/*' />
        AllCultures = NeutralCultures | SpecificCultures | InstalledWin32Cultures,
    }
}    
