//------------------------------------------------------------------------------
// <copyright file="XmlChildEnumerator.cs" company="Microsoft">
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

namespace System.Xml 
{
    using System.Collections;

    /// <include file='doc\XmlChildEnumerator.uex' path='docs/doc[@for="XmlChildEnumerator"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal sealed class XmlChildEnumerator: IEnumerator {
        internal XmlNode container;
        internal XmlNode child;
        internal bool isFirst;

        internal XmlChildEnumerator( XmlNode container ) {
            this.container = container;
            this.child = container.FirstChild;
            this.isFirst = true;
        }

        bool IEnumerator.MoveNext() {
            return this.MoveNext();
        }

        /// <include file='doc\XmlChildEnumerator.uex' path='docs/doc[@for="XmlChildEnumerator.MoveNext"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal bool MoveNext() {
            if (isFirst) {
                child = container.FirstChild;
                isFirst = false;
            }
            else if (child != null) {
                child = child.NextSibling;
            }

            return child != null;
        }

        void IEnumerator.Reset() {
            isFirst = true;
            child = container.FirstChild;
        }

        object IEnumerator.Current {
            get {
                if (isFirst || child == null)
                    throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));

                return child;
            }
        }

        /// <include file='doc\XmlChildEnumerator.uex' path='docs/doc[@for="XmlChildEnumerator.Current"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal XmlNode Current {
            get {
                if (isFirst || child == null)
                    throw new InvalidOperationException(Res.GetString(Res.Xml_InvalidOperation));

                return child;
            }
        }
    }
}
