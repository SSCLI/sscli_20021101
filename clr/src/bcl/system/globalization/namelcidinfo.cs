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
////////////////////////////////////////////////////////////////////////////
//
//  Class:    NameLCIDInfo
//
//
//  Purpose:  Package private class used to map string to an appropriate LCID.
//
//  Date:     March 31, 1999
//
////////////////////////////////////////////////////////////////////////////

namespace System.Globalization {
    
	using System;
    [Serializable()]
    internal struct NameLCIDInfo
    {
        internal String name;
        internal int    LCID;
        internal NameLCIDInfo(String name, int LCID)
        {
            this.name = name;
            this.LCID = LCID;
        }
        
        public override int GetHashCode() {
            return name.GetHashCode() ^ LCID;
        }
    
    
    }

}
