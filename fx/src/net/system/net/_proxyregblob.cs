//------------------------------------------------------------------------------
// <copyright file="_ProxyRegBlob.cs" company="Microsoft">
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
    using System;
    using System.Globalization;
    using Microsoft.Win32;
    using System.Security.Permissions;
    using System.Text;
    using System.Collections;

    internal class ProxyRegBlob {

        private static string ReadConfigString(String ConfigName)
	{
            const int parameterValueLength = 255;
            StringBuilder parameterValue = new StringBuilder(parameterValueLength);
            bool rc = UnsafeNclNativeMethods.FetchConfigurationString(true, ConfigName, parameterValue, parameterValueLength);
            if (rc) {
	        return parameterValue.ToString();
	    }
	    return "";
	}

        //
        // Parses out a string from IE and turns it into a URI 
        //
        private static Uri ParseProxyUri(string proxyString, bool validate) {
            if (validate) { 
                if (proxyString.Length == 0) {
                    return null;
                }

                if (proxyString.IndexOf('=') != -1) {
                    return null;
                }
            }

            if (proxyString.IndexOf("://") == -1) {
                proxyString = "http://" + proxyString;
            }

            return new Uri(proxyString);
        }

        //
        // Builds a hashtable containing the protocol and proxy URI to use for it.
        //
        private static Hashtable ParseProtocolProxies(string proxyListString) {           
            if (proxyListString.Length == 0) {
                return null;
            }

            // parse something like "http=http://http-proxy;https=http://https-proxy;ftp=http://ftp-proxy"
            char [] splitChars = new char[] { ';', '=' };
            String [] proxyListStrings = proxyListString.Split(splitChars);
            bool protocolPass = true;
            string protocolString = null;

            Hashtable proxyListHashTable = new Hashtable(CaseInsensitiveString.StaticInstance, CaseInsensitiveString.StaticInstance);   

            foreach (string elementString in proxyListStrings) {
                string elementString2 = elementString.Trim().ToLower(CultureInfo.InvariantCulture); 
                if (protocolPass) {
                    protocolString = elementString2;
                } else { 
                    proxyListHashTable[protocolString] = ParseProxyUri(elementString2,false);                    
                }

                protocolPass = ! protocolPass;
            }

            if (proxyListHashTable.Count == 0) {
                return null;
            } 
            
            return proxyListHashTable;
        }    

        //
        // Converts a simple IE regular expresion string into one
        //  that is compatible with Regex escape sequences.
        //
        private static string BypassStringEscape(string bypassString) {
            StringBuilder escapedBypass = new StringBuilder();
            // (\, *, +, ?, |, {, [, (,), ^, $, ., #, and whitespace) are reserved
            foreach (char c in bypassString){
                if (c == '\\' || c == '.' || c=='?') {
                    escapedBypass.Append('\\');
                } else if (c == '*') {
                    escapedBypass.Append('.');
                }
                escapedBypass.Append(c);
            }
            escapedBypass.Append('$');
            return escapedBypass.ToString();
        }


        //
        // Parses out a string of bypass list entries and coverts it to Regex's that can be used 
        //   to match against.
        //
        private static string [] ParseBypassList(string bypassListString, out bool bypassOnLocal) {

            char [] splitChars = new char[] { ';' };
            String [] bypassListStrings = bypassListString.Split(splitChars);

            bypassOnLocal = false;

            if (bypassListStrings.Length == 0) {
                return null;
            }

            ArrayList stringArrayList = new ArrayList();
                      
            foreach (string bypassString in bypassListStrings) {
                string bypassString2 = bypassString;
                if ( bypassString2 != null) {
                    bypassString2 = bypassString2.Trim(); 
                    if (bypassString2.ToLower(CultureInfo.InvariantCulture) == "<local>") {
                        bypassOnLocal = true; 
                    } else if (bypassString2.Length > 0) {                                            
                        bypassString2 = BypassStringEscape(bypassString2);
                        stringArrayList.Add(bypassString2);
                    }
                }
            }

            if (stringArrayList.Count == 0) {
                return null;
            }

            return (string []) stringArrayList.ToArray(typeof(string));
        }


        //
        // Uses this object, by instancing, and retrieves
        //  the proxy settings from IE = converts to URI
        //
        static internal WebProxy GetIEProxy() 
        {
            bool isLocalOverride = false;
            Hashtable proxyHashTable = null;
            string [] bypassList = null;
            Uri proxyUri = null;
            string proxyUriString = ReadConfigString("ProxyUri");
            string proxyBypassString = ReadConfigString("ProxyBypass");
            try {   
                proxyUri = ParseProxyUri(proxyUriString, true);
                if ( proxyUri == null ) {
                    proxyHashTable = ParseProtocolProxies(proxyUriString);
                }
                        
                if ((proxyUri != null || proxyHashTable != null) && proxyBypassString != null ) {
                    bypassList = ParseBypassList(proxyBypassString, out isLocalOverride);                        
                }   

                // success if we reach here
            } catch (Exception) {
            }
            WebProxy webProxy = null;

            if (proxyHashTable != null) {
                webProxy = new WebProxy((Uri) proxyHashTable["http"], isLocalOverride, bypassList);
            } else {
                webProxy = new WebProxy(proxyUri, isLocalOverride, bypassList);
            }
                      
            return webProxy;
        }

    }; // ProxyRegBlob

}

