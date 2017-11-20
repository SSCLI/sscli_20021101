//------------------------------------------------------------------------------
// <copyright file="EventHandlerList.cs" company="Microsoft">
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

namespace System.ComponentModel {
    using System;

    /// <include file='doc\EventHandlerList.uex' path='docs/doc[@for="EventHandlerList"]/*' />
    /// <devdoc>
    ///    <para>Provides a simple list of delegates. This class cannot be inherited.</para>
    /// </devdoc>
    public sealed class EventHandlerList : IDisposable {
        ListEntry head;

        /// <include file='doc\EventHandlerList.uex' path='docs/doc[@for="EventHandlerList.this"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the delegate for the specified key.</para>
        /// </devdoc>
        public Delegate this[object key] {
            get {
                ListEntry e = Find(key);
                if (e != null) {
                    return e.handler;
                }
                else {
                    return null;
                }
            }
            set {
                ListEntry e = Find(key);
                if (e != null) {
                    e.handler = value;
                }
                else {
                    head = new ListEntry(key, value, head);
                }
            }
        }

        /// <include file='doc\EventHandlerList.uex' path='docs/doc[@for="EventHandlerList.AddHandler"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddHandler(object key, Delegate value) {
            ListEntry e = Find(key);
            if (e != null) {
                e.handler = Delegate.Combine(e.handler, value);
            }
            else {
                head = new ListEntry(key, value, head);
            }
        }

        /// <include file='doc\EventHandlerList.uex' path='docs/doc[@for="EventHandlerList.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Dispose() {
            head = null;
        }

        private ListEntry Find(object key) {
            ListEntry found = head;
            while (found != null) {
                if (found.key == key) {
                    break;
                }
                found = found.next;
            }
            return found;
        }

        /// <include file='doc\EventHandlerList.uex' path='docs/doc[@for="EventHandlerList.RemoveHandler"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void RemoveHandler(object key, Delegate value) {
            ListEntry e = Find(key);
            if (e != null) {
                e.handler = Delegate.Remove(e.handler, value);
            }
            // else... no error for removal of non-existant delegate
            //
        }

        private sealed class ListEntry {
            internal ListEntry next;
            internal object key;
            internal Delegate handler;

            public ListEntry(object key, Delegate handler, ListEntry next) {
                this.next = next;
                this.key = key;
                this.handler = handler;
            }
        }
    }


}


