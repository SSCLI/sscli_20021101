//------------------------------------------------------------------------------
// <copyright file="TraceInternal.cs" company="Microsoft">
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

namespace System.Diagnostics {
    internal class TraceInternal {
        static TraceListenerCollection listeners;
        static bool autoFlush;
        static int indentLevel;
        static int indentSize;
        static bool settingsInitialized;
        static object critSec = new object();

        public static TraceListenerCollection Listeners { 
            get {
                if (!settingsInitialized) InitializeSettings();
                if (listeners == null) {
                    lock (typeof(Trace)) {
                        if (listeners == null) {
                            listeners = new TraceListenerCollection();
                            TraceListener defaultListener = new DefaultTraceListener();
                            defaultListener.IndentLevel = indentLevel;
                            defaultListener.IndentSize = indentSize;
                            listeners.Add(defaultListener);
                        }
                    }
                }
                return listeners;
            }
        }          

        public static bool AutoFlush { 
            get { 
                if (!settingsInitialized) InitializeSettings();
                return autoFlush; 
            }

            set {
                if (!settingsInitialized) InitializeSettings();    
                autoFlush = value;
            }
        }

        public static int IndentLevel {
            get { return indentLevel; }

            set {
                lock (critSec) {
                    // We don't want to throw here -- it is very bad form to have debug or trace
                    // code throw exceptions!
                    if (value < 0) {
                        value = 0;
                    }
                    indentLevel = value;
                    
                    if (listeners != null) {
                        foreach (TraceListener listener in Listeners) {
                            listener.IndentLevel = indentLevel;
                        }
                    }
                }
            }
        }

        public static int IndentSize {
            get { 
                if (!settingsInitialized) InitializeSettings();
                return indentSize; 
            }
            
            set {
                if (!settingsInitialized) InitializeSettings();    

                SetIndentSize(value);
            }
        }        

        static void SetIndentSize(int value) {
            lock (critSec) {                
                // We don't want to throw here -- it is very bad form to have debug or trace
                // code throw exceptions!            
                if (value < 0) {
                    value = 0;
                }

                indentSize = value;
                
                if (listeners != null) {
                    foreach (TraceListener listener in Listeners) {
                        listener.IndentSize = indentSize;
                    }
                } 
            }
        }

        public static void Indent() {
            lock (critSec) {
                if (!settingsInitialized) InitializeSettings();
                indentLevel++;
                foreach (TraceListener listener in Listeners) {
                    listener.IndentLevel = indentLevel;
                }
            }
        }

        public static void Unindent() {
            lock (critSec) {
                if (!settingsInitialized) InitializeSettings();
                if (indentLevel > 0) {
                    indentLevel--;
                }
                foreach (TraceListener listener in Listeners) {
                    listener.IndentLevel = indentLevel;
                }
            }
        }

        public static void Flush() {
            if (listeners != null) {
                lock (critSec) {
                    foreach (TraceListener listener in Listeners) {
                        listener.Flush();
                    }
                }
            }
        }

        public static void Close() {
            if (listeners != null) {
                lock (critSec) {
                    foreach (TraceListener listener in Listeners) {
                        listener.Close();
                    }
                }
            }
        }

        public static void Assert(bool condition) {
            if (condition) return;
            Fail(string.Empty);
        }

        public static void Assert(bool condition, string message) {
            if (condition) return;
            Fail(message);
        }

        public static void Assert(bool condition, string message, string detailMessage) {
            if (condition) return;
            Fail(message, detailMessage);
        }

        public static void Fail(string message) {
            Fail(message, null);        
        }        

        public static void Fail(string message, string detailMessage) {
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.Fail(message, detailMessage);
                    if (AutoFlush) listener.Flush();
                }            
            }
        }        

        private static void InitializeSettings() {
            // Getting IndentSize and AutoFlush will load config on demand.
            // If we load config and there are trace listeners added, we'll
            // end up recursing, but that recursion will be stopped in
            // DiagnosticsConfiguration.Initialize()           
            SetIndentSize(DiagnosticsConfiguration.IndentSize);
            autoFlush = DiagnosticsConfiguration.AutoFlush;
            settingsInitialized = true;
        }

        public static void Write(string message) {
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.Write(message);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void Write(object value) {
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.Write(value);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void Write(string message, string category) {
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.Write(message, category);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void Write(object value, string category) {
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.Write(value, category);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void WriteLine(string message) {
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.WriteLine(message);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void WriteLine(object value) {
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.WriteLine(value);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void WriteLine(string message, string category) {
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.WriteLine(message, category);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void WriteLine(object value, string category) {
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.WriteLine(value, category);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void WriteIf(bool condition, string message) {
            if (!condition) return;
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.Write(message);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void WriteIf(bool condition, object value) {
            if (!condition) return;
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.Write(value);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void WriteIf(bool condition, string message, string category) {
            if (!condition) return;
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.Write(message, category);
                    if (AutoFlush) listener.Flush();
                }
            }                        
        }

        public static void WriteIf(bool condition, object value, string category) {
            if (!condition) return;
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.Write(value, category);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void WriteLineIf(bool condition, string message) {
            if (!condition) return;
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.WriteLine(message);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void WriteLineIf(bool condition, object value) {
            if (!condition) return;
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.WriteLine(value);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void WriteLineIf(bool condition, string message, string category) {
            if (!condition) return;
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.WriteLine(message, category);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }

        public static void WriteLineIf(bool condition, object value, string category) {
            if (!condition) return;
            lock (critSec) {
                foreach (TraceListener listener in Listeners) {
                    listener.WriteLine(value, category);
                    if (AutoFlush) listener.Flush();
                }                        
            }
        }
    }
}
