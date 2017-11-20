//------------------------------------------------------------------------------
// <copyright file="_BasicHostName.cs" company="Microsoft">
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

using System.Diagnostics;
using System.Globalization;

namespace System {
    internal class BasicHostName : HostNameType {

        internal BasicHostName(string name) : base(name) {
        }

        public override bool Equals(object comparand) {
            Debug.Assert(comparand != null);
            return String.Compare(Name, comparand.ToString(), true, CultureInfo.InvariantCulture) == 0;
        }

        public override int GetHashCode() {
            // 11/15/00 - avoid compiler warning
            return base.GetHashCode();
        }
    }
}
