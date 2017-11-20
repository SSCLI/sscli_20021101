//------------------------------------------------------------------------------
// <copyright file="ProcessWaitHandle.cs" company="Microsoft">
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

using System;
using System.Threading;

namespace System.Diagnostics {
    internal class ProcessWaitHandle : WaitHandle {

        protected override void Dispose(bool explicitDisposing) {
            // WaitHandle.Dispose(bool) closes our handle - we
            // don't want to do that because Process.Dispose will
            // take care of it
        }
    }
}
