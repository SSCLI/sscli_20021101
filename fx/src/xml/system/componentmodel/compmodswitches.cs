
//------------------------------------------------------------------------------
// <copyright file="Component.cs" company="Microsoft">
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
    using System.Diagnostics;

    /// <internalonly/>
    internal sealed class CompModSwitches {
        private static TraceSwitch xmlSchema;
        private static BooleanSwitch keepTempFiles;        
        private static TraceSwitch xmlSerialization;
        
        public static TraceSwitch XmlSchema {
            get {
                if (xmlSchema == null) {
                    xmlSchema = new TraceSwitch("XmlSchema", "Enable tracing for the XmlSchema class.");
                }
                return xmlSchema;
            }
        }        
        
        public static BooleanSwitch KeepTempFiles {
            get {
                if (keepTempFiles == null) {
                    keepTempFiles = new BooleanSwitch("XmlSerialization.Compilation", "Keep XmlSerialization generated (temp) files.");
                }
                return keepTempFiles;
            }
        }
        
        public static TraceSwitch XmlSerialization {
            get {
                if (xmlSerialization == null) {
                    xmlSerialization = new TraceSwitch("XmlSerialization", "Enable tracing for the System.Xml.Serialization component.");
                }
                return xmlSerialization;
            }
        }
        
    }
}
