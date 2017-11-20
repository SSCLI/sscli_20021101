//------------------------------------------------------------------------------
// <copyright file="_LoggingObject.cs" company="Microsoft">
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

//
//  We have function based stack and thread based logging of basic behavior.  We
//  also now have the ability to run a "watch thread" which does basic hang detection
//  and error-event based logging.   The logging code buffers the callstack/picture
//  of all COMNET threads, and upon error from an assert or a hang, it will open a file
//  and dump the snapsnot.  Future work will allow this to be configed by registry and
//  to use Runtime based logging.  We'd also like to have different levels of logging.
//

namespace System.Net {

    using System.Collections;
    using System.IO;
    using System.Threading;
    using System.Diagnostics;
    using System.Security.Permissions;
    using Microsoft.Win32;

    //
    // BaseLoggingObject - used to disable logging,
    //  this is just a base class that does nothing.
    //

    internal class BaseLoggingObject {

        internal BaseLoggingObject() {
        }

        internal virtual void EnterFunc(string funcname) {
        }

        internal virtual void LeaveFunc(string funcname) {
        }

        internal virtual void DumpArrayToConsole() {
        }

        internal virtual void PrintLine(string msg) {
        }

        internal virtual void DumpArray(bool shouldClose) {
        }

        internal virtual void DumpArrayToFile(bool shouldClose) {
        }

        internal virtual void Flush() {
        }

        internal virtual void Flush(bool close) {
        }

        internal virtual void LoggingMonitorTick() {
        }

        internal virtual void Dump(byte[] buffer) {
        }

        internal virtual void Dump(byte[] buffer, int length) {
        }

        internal virtual void Dump(byte[] buffer, int offset, int length) {
        }
    } // class BaseLoggingObject


    /// <include file='doc\_LoggingObject.uex' path='docs/doc[@for="GlobalLog"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>

    internal class GlobalLog {

        //
        // Logging Initalization - I need to disable Logging code, and limit
        //  the effect it has when it is dissabled, so I use a bool here.
        //
        //  This can only be set when the logging code is built and enabled.
        //  By specifing the "CSC_COMPILE_FLAGS=/D:TRAVE" in the build environment,
        //  this code will be built and then checks against an enviroment variable
        //  and a BooleanSwitch to see if any of the two have enabled logging.
        //

        private static BaseLoggingObject Logobject = GlobalLog.LoggingInitialize();
        private static BaseLoggingObject LoggingInitialize() {

#if DEBUG
            InitConnectionMonitor();
#endif
            // otherwise disable
            return new BaseLoggingObject();
        }

#if DEBUG

        // Enables auto-hang detection, which will "snap" a log on hang
        public const bool EnableMonitorThread = false;

        // Default value for hang timer
        public const int DefaultTickValue = 1000*60; // 60 secs
#endif // DEBUG

        [System.Diagnostics.Conditional("TRAVE")]
        public static void AddToArray(string msg) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Ignore(object msg) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Print(string msg) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void PrintHex(string msg, object value) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Enter(string func) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Enter(string func, string parms) {
        }

        [System.Diagnostics.Conditional("DEBUG")]
        public static void Assert(
                bool ShouldNotFireAssert,
                string ErrorMsg,
                string Msg2
                )
        {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void LeaveException(string func, Exception exception) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Leave(string func) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Leave(string func, string result) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Leave(string func, int returnval) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Leave(string func, bool returnval) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void DumpArray() {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Dump(byte[] buffer) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Dump(byte[] buffer, int length) {
        }

        [System.Diagnostics.Conditional("TRAVE")]
        public static void Dump(byte[] buffer, int offset, int length) {
        }

#if DEBUG
        private class HttpWebRequestComparer : IComparer {
            public int Compare(
                   object x1,
                   object y1
                   ) {

                HttpWebRequest x = (HttpWebRequest) x1;
                HttpWebRequest y = (HttpWebRequest) y1;

                if (x.GetHashCode() == y.GetHashCode()) {
                    return 0;
                } else if (x.GetHashCode() < y.GetHashCode()) {
                    return -1;
                } else if (x.GetHashCode() > y.GetHashCode()) {
                    return 1;
                }

                return 0;
            }
        }

        private class ConnectionMonitorEntry {
            public HttpWebRequest m_Request;
            public int m_Flags;
            public DateTime m_TimeAdded;
            public Connection m_Connection;

            public ConnectionMonitorEntry(HttpWebRequest request, Connection connection, int flags) {
                m_Request = request;
                m_Connection = connection;
                m_Flags = flags;
                m_TimeAdded = DateTime.Now;
            }
        }

        private static ManualResetEvent s_ShutdownEvent;
        private static SortedList s_RequestList;

        internal const int WaitingForReadDoneFlag = 0x1;

        [System.Diagnostics.Conditional("DEBUG")]
        private static void ConnectionMonitor() {
            while(! s_ShutdownEvent.WaitOne(DefaultTickValue, false)) {
                if (GlobalLog.EnableMonitorThread) {
                }

                int hungCount = 0;
                lock (s_RequestList) {
                    DateTime dateNow = DateTime.Now;
                    DateTime dateExpired = dateNow.AddSeconds(-120);
                    foreach (ConnectionMonitorEntry monitorEntry in s_RequestList.GetValueList() ) {
                        if (monitorEntry != null &&
                            (dateExpired > monitorEntry.m_TimeAdded))
                        {
                            hungCount++;
                            monitorEntry.m_Connection.Debug(monitorEntry.m_Request.GetHashCode());
                        }
                    }
                }
                GlobalLog.Assert(
                        hungCount == 0,
                        "Warning: Hang Detected on Connection(s) of greater than: " + DefaultTickValue + " ms and " + hungCount + " request(s) did hang",
                        "Please Dump System.Net.GlobalLog.s_RequestList for pending requests, make sure your streams are calling .Close(), and that your destination server is up" );

            }

        }
#endif // DEBUG

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void AppDomainUnloadEvent(object sender, EventArgs e) {
#if DEBUG
            s_ShutdownEvent.Set();
#endif
        }

#if DEBUG
        [System.Diagnostics.Conditional("DEBUG")]
        private static void InitConnectionMonitor() {
            s_RequestList = new SortedList(new HttpWebRequestComparer(), 10);
            s_ShutdownEvent = new ManualResetEvent(false);
            AppDomain.CurrentDomain.DomainUnload += new EventHandler(AppDomainUnloadEvent);
            AppDomain.CurrentDomain.ProcessExit += new EventHandler(AppDomainUnloadEvent);
            Thread threadMonitor = new Thread(new ThreadStart(ConnectionMonitor));
            threadMonitor.IsBackground = true;
            threadMonitor.Start();
        }
#endif

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void DebugAddRequest(HttpWebRequest request, Connection connection, int flags) {
#if DEBUG
            lock(s_RequestList) {
                GlobalLog.Assert(
                    ! s_RequestList.ContainsKey(request),
                    "s_RequestList.ContainsKey(request)",
                    "a HttpWebRequest should not be submitted twice" );

                ConnectionMonitorEntry requestEntry =
                    new ConnectionMonitorEntry(request, connection, flags);

                try {
                    s_RequestList.Add(request, requestEntry);
                } catch {
                }
            }
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void DebugRemoveRequest(HttpWebRequest request) {
#if DEBUG
            lock(s_RequestList) {
                GlobalLog.Assert(
                    s_RequestList.ContainsKey(request),
                    "!s_RequestList.ContainsKey(request)",
                    "a HttpWebRequest should not be removed twice" );

                try {
                    s_RequestList.Remove(request);
                } catch {
                }
            }
#endif
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void DebugUpdateRequest(HttpWebRequest request, Connection connection, int flags) {
#if DEBUG
            lock(s_RequestList) {

                if(!s_RequestList.ContainsKey(request)) {
                    return;
                }

                ConnectionMonitorEntry requestEntry =
                    new ConnectionMonitorEntry(request, connection, flags);

                try {
                    s_RequestList.Remove(request);
                    s_RequestList.Add(request, requestEntry);
                } catch {
                }
            }
#endif
        }

    } // class GlobalLog


} // namespace System.Net
