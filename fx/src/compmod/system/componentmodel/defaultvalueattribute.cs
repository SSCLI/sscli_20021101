//------------------------------------------------------------------------------
// <copyright file="DefaultValueAttribute.cs" company="Microsoft">
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

/*
 */
namespace System.ComponentModel {

    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization.Formatters;

    /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute"]/*' />
    /// <devdoc>
    ///    <para>Specifies the default value for a property.</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.All)]
    public sealed class DefaultValueAttribute : Attribute {
        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.value"]/*' />
        /// <devdoc>
        ///     This is the default value.
        /// </devdoc>
        private readonly object value;


        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.DefaultValueAttribute1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.DefaultValueAttribute'/> class using a Unicode
        ///    character.</para>
        /// </devdoc>
        public DefaultValueAttribute(char value) {
            this.value = value;
        }
        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.DefaultValueAttribute2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.DefaultValueAttribute'/> class using an 8-bit unsigned
        ///    integer.</para>
        /// </devdoc>
        public DefaultValueAttribute(byte value) {
            this.value = value;
        }
        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.DefaultValueAttribute3"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.DefaultValueAttribute'/> class using a 16-bit signed
        ///    integer.</para>
        /// </devdoc>
        public DefaultValueAttribute(short value) {
            this.value = value;
        }
        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.DefaultValueAttribute4"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.DefaultValueAttribute'/> class using a 32-bit signed
        ///    integer.</para>
        /// </devdoc>
        public DefaultValueAttribute(int value) {
            this.value = value;
        }
        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.DefaultValueAttribute5"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.DefaultValueAttribute'/> class using a 64-bit signed
        ///    integer.</para>
        /// </devdoc>
        public DefaultValueAttribute(long value) {
            this.value = value;
        }
        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.DefaultValueAttribute6"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.DefaultValueAttribute'/> class using a
        ///    single-precision floating point
        ///    number.</para>
        /// </devdoc>
        public DefaultValueAttribute(float value) {
            this.value = value;
        }
        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.DefaultValueAttribute7"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.DefaultValueAttribute'/> class using a
        ///    double-precision floating point
        ///    number.</para>
        /// </devdoc>
        public DefaultValueAttribute(double value) {
            this.value = value;
        }
        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.DefaultValueAttribute8"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.DefaultValueAttribute'/> class using a <see cref='System.Boolean'/>
        /// value.</para>
        /// </devdoc>
        public DefaultValueAttribute(bool value) {
            this.value = value;
        }
        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.DefaultValueAttribute9"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.DefaultValueAttribute'/> class using a <see cref='System.String'/>.</para>
        /// </devdoc>
        public DefaultValueAttribute(string value) {
            this.value = value;
        }

        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.DefaultValueAttribute10"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.ComponentModel.DefaultValueAttribute'/>
        /// class.</para>
        /// </devdoc>
        public DefaultValueAttribute(object value) {
            this.value = value;
        }

        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the default value of the property this
        ///       attribute is
        ///       bound to.
        ///    </para>
        /// </devdoc>
        public object Value {
            get {
                return value;
            }
        }

        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.Equals"]/*' />
        public override bool Equals(object obj) {
            if (obj == this) {
                return true;
            }

            DefaultValueAttribute other = obj as DefaultValueAttribute;

            if (other != null) {
                if (value != null) {
                    return value.Equals(other.Value);
                }
                else {
                    return (other.Value == null);           
                }
            }
            return false;
        }

        /// <include file='doc\DefaultValueAttribute.uex' path='docs/doc[@for="DefaultValueAttribute.GetHashCode"]/*' />
        public override int GetHashCode() {
            return base.GetHashCode();
        }
    }
}
