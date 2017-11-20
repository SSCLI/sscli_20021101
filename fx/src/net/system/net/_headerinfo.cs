//------------------------------------------------------------------------------
// <copyright file="_HeaderInfo.cs" company="Microsoft">
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

namespace System.Net {

    internal delegate string[] HeaderParser(string value);

    internal class HeaderInfo {

        internal bool IsRestricted;
        internal HeaderParser Parser;

        //
        // Note that the HeaderName field is not always valid, and should not
        // be used after initialization. In particular, the HeaderInfo returned
        // for an unknown header will not have the correct header name.
        //

        internal string HeaderName;
        internal bool AllowMultiValues;

        internal HeaderInfo(string name, bool restricted, bool multi, HeaderParser p) {
            HeaderName = name;
            IsRestricted = restricted;
            Parser = p;
            AllowMultiValues = multi;
        }
    }
} // namespace System.Net
