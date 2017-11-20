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
using System;
using System.Reflection;
using System.Configuration;
using System.Collections.Specialized;
using System.Globalization;

namespace System.Configuration {

    /// <include file='doc\AppSettingsReader.uex' path='docs/doc[@for="AppSettingsReader"]/*' />
    /// <devdoc>
    ///     The AppSettingsReader class provides a wrapper for System.Configuration.ConfigurationSettings.AppSettings
    ///     which provides a single method for reading values from the config file of a particular type.
    /// </devdoc>
    public class AppSettingsReader {
        private NameValueCollection map;
        static Type stringType = typeof(string);
        static Type[] paramsArray = new Type[] { stringType };
        static string NullString = "None";

        /// <include file='doc\AppSettingsReader.uex' path='docs/doc[@for="AppSettingsReader.AppSettingsReader"]/*' />
        /// <devdoc>
        ///     Constructor
        /// </devdoc>
        public AppSettingsReader() {
            map = System.Configuration.ConfigurationSettings.AppSettings;
        }

        /// <include file='doc\AppSettingsReader.uex' path='docs/doc[@for="AppSettingsReader.GetValue"]/*' />
        /// <devdoc>
        ///     Gets the value for specified key from ConfigurationSettings.AppSettings, and returns
        ///     an object of the specified type containing the value from the config file.  If the key
        ///     isn't in the config file, or if it is not a valid value for the given type, it will 
        ///     throw an exception with a descriptive message so the user can make the appropriate
        ///     change
        /// </devdoc>
        public object GetValue(string key, Type type) {
            if (key == null) throw new ArgumentNullException("key");
            if (type == null) throw new ArgumentNullException("type");

            string val = map[key];

            if (val == null) throw new InvalidOperationException(SR.GetString(SR.AppSettingsReaderNoKey, key));

            if (type == stringType) {
                // It's a string, so we can ALMOST just return the value.  The only
                // tricky point is that if it's the string "(None)", then we want to
                // return null.  And of course we need a way to represent the string
                // (None), so we use ((None)), and so on... so it's a little complicated.
                int NoneNesting = GetNoneNesting(val);
                if (NoneNesting == 0) {
                    // val is not of the form ((..((None))..))
                    return val;
                }
                else if (NoneNesting == 1) {
                    // val is (None)
                    return null;
                }
                else {
                    // val is of the form ((..((None))..))
                    return val.Substring(1, val.Length - 2);
                }
            }
            else {
                MethodInfo method = null;
                try {
                    method = type.GetMethod("Parse", paramsArray);
                }
                catch(Exception) {
                    // don't do anything - method will be null below and 
                    // we'll throw there.
                }
                if (method == null) {
                    throw new InvalidOperationException(SR.GetString(SR.AppSettingsReaderNoParser, type.ToString()));
                }
                else {
                    try {
                        return method.Invoke(null, new object[] { val });
                    } catch (Exception) {
                        string displayString = (val.Length == 0) ? SR.AppSettingsReaderEmptyString : val;
                        throw new InvalidOperationException(SR.GetString(SR.AppSettingsReaderCantParse, displayString, key, type.ToString()));
                    }
                }
            }  
        }

        private int GetNoneNesting(string val) {
            int count = 0;
            int len = val.Length;
            if (len > 1) {
                while (val[count] == '(' && val[len - count - 1] == ')') {
                    count++;
                }
                if (count > 0 && string.Compare(NullString, 0, val, count, len - 2 * count, false, CultureInfo.InvariantCulture) != 0) {
                    // the stuff between the parens is not "None"
                    count = 0;
                }
            }
            return count;
        }
    }
}
