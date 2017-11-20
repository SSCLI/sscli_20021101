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
namespace System {
    
    using System;
    
    /// <include file='doc\UnhandledExceptionEventArgs.uex' path='docs/doc[@for="UnhandledExceptionEventArgs"]/*' />
    [Serializable()]
    public class UnhandledExceptionEventArgs : EventArgs {
        private Object _Exception;
        private bool _IsTerminating;

        /// <include file='doc\UnhandledExceptionEventArgs.uex' path='docs/doc[@for="UnhandledExceptionEventArgs.UnhandledExceptionEventArgs"]/*' />
        public UnhandledExceptionEventArgs(Object exception, bool isTerminating) {
            _Exception = exception;
            _IsTerminating = isTerminating;
        }
        /// <include file='doc\UnhandledExceptionEventArgs.uex' path='docs/doc[@for="UnhandledExceptionEventArgs.ExceptionObject"]/*' />
        public Object ExceptionObject { get { return _Exception; } }
        /// <include file='doc\UnhandledExceptionEventArgs.uex' path='docs/doc[@for="UnhandledExceptionEventArgs.IsTerminating"]/*' />
        public bool IsTerminating { get { return _IsTerminating; } }
    }
}
