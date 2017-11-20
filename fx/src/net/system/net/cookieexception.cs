//------------------------------------------------------------------------------
// <copyright file="cookieexception.cs" company="Microsoft">
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
    using System.Runtime.Serialization;
    /// <include file='doc\cookieexception.uex' path='docs/doc[@for="CookieException"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [Serializable]
    public class CookieException : FormatException, ISerializable {
        /// <include file='doc\cookieexception.uex' path='docs/doc[@for="CookieException.CookieException"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CookieException() : base() {
        }

        internal CookieException(string message) : base(message) {
        }

        internal CookieException(string message, Exception inner) : base(message, inner) {
        }

        /// <include file='doc\cookieexception.uex' path='docs/doc[@for="CookieException.CookieException1"]/*' />
        protected CookieException(SerializationInfo serializationInfo, StreamingContext streamingContext)
            : base(serializationInfo, streamingContext) {
        }

        /// <include file='doc\cookieexception.uex' path='docs/doc[@for="CookieException.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            base.GetObjectData(serializationInfo, streamingContext);
        }
    }
}
