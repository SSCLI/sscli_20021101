//------------------------------------------------------------------------------
// <copyright file="HResults.cs" company="Microsoft">
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

namespace System {
// Note: FACILITY_URT is defined as 0x13 (0x8013xxxx).  Within that
// range, 0x1yyy is for Runtime errors (used for Security, Metadata, etc).
// In that subrange, 0x15zz and 0x16zz have been allocated for classlib-type 
// HResults. Also note that some of our HResults have to map to certain 
// COM HR's, etc.
// Another arbitrary decision...  Feel free to change this, as long as you
// renumber the HResults yourself (and update rexcep.h).
// Reflection will use 0x1600 -> 0x1620.  IO will use 0x1620 -> 0x1640.
// Security will use 0x1640 -> 0x1660
// There are HResults files in the IO, Remoting, Reflection & 
// Security/Util directories as well, so choose your HResults carefully.
    
    using System;
    internal sealed class HResults{
        public const int Configuration = unchecked((int)0x80131902);
        public const int Xml = unchecked((int)0x80131940);
        public const int XmlSchema = unchecked((int)0x80131941);
        public const int XmlXslt = unchecked((int)0x80131942);
        public const int XmlXPath = unchecked((int)0x80131943);
    }
}
