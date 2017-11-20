//------------------------------------------------------------------------------
// <copyright file="Process.cs" company="Microsoft">
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
    using System.Text;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.IO;
    using Microsoft.Win32;        
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\Process.uex' path='docs/doc[@for="Process"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides access to local and remote
    ///       processes. Enables you to start and stop system processes.
    ///    </para>
    /// </devdoc>
    [
    // Disabling partial trust scenarios
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust"),
    PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")
    ]
    public class Process : Component {

        //
        // FIELDS
        //

        bool disposed;
        bool haveProcessId;
        int processId;
        bool haveProcessHandle;
        IntPtr processHandle;
        ProcessStartInfo startInfo;
        bool watchForExit;
        bool watchingForExit;
        EventHandler onExited;
        bool exited;
        int exitCode;
        DateTime exitTime;
        bool haveExitTime;
        bool raisedOnExited;
        RegisteredWaitHandle registeredWaitHandle;
        WaitHandle waitHandle;
        ISynchronizeInvoke synchronizingObject;
        StreamReader standardOutput;
        StreamWriter standardInput;
        StreamReader standardError;

#if DEBUG
        internal static TraceSwitch processTracing = new TraceSwitch("processTracing", "Controls debug output from Process component");
#else
        internal static TraceSwitch processTracing = null;
#endif

        //
        // CONSTRUCTORS
        //

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Process"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Diagnostics.Process'/> class.
        ///    </para>
        /// </devdoc>
        public Process() {
        }        
        
        Process(int processId, IntPtr processHandle) : base() {
            this.processId = processId;
            this.haveProcessId = true;
            this.processHandle = processHandle;
            this.haveProcessHandle = true;           
        }

        //
        // PROPERTIES
        //

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Associated"]/*' />
        /// <devdoc>
        ///     Returns whether this process component is associated with a real process.
        /// </devdoc>
        /// <internalonly/>
        bool Associated {
            get {
                return haveProcessId || haveProcessHandle;
            }
        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.ExitCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the
        ///       value that was specified by the associated process when it was terminated.
        ///    </para>
        /// </devdoc>
        public int ExitCode {
            get {
                EnsureState(State.Exited);
                return exitCode;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.HasExited"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a
        ///       value indicating whether the associated process has been terminated.
        ///    </para>
        /// </devdoc>
        public bool HasExited {
            get {
                if (!exited) {
                    EnsureState(State.Associated);
                    IntPtr processHandle = (IntPtr)0;
                    try {
                        processHandle = OpenProcessHandle();
                        if (processHandle == NativeMethods.INVALID_HANDLE_VALUE) {
                            exited = true;
                        }
                        else {
                            int exitCode;
                            if (!NativeMethods.GetExitCodeProcess(processHandle, out exitCode))
                                throw new Win32Exception();
                            if (exitCode != NativeMethods.STILL_ACTIVE) {
                                this.exited = true;
                                this.exitCode = exitCode;
                            }
                        }
                    }
                    finally {
                        ReleaseProcessHandle(processHandle);
                    }
                    if (exited)
                        RaiseOnExited();
                }
                return exited;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.ExitTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the time that the associated process exited.
        ///    </para>
        /// </devdoc>
        public DateTime ExitTime {
            get {
                if (!haveExitTime) {
                    EnsureState(State.Exited);
                    IntPtr processHandle = (IntPtr)0;
                    try {
                        processHandle = OpenProcessHandle();
                        long[] create = new long[1];
                        long[] exit = new long[1];
                        long[] kernel = new long[1];
                        long[] user = new long[1];
                        if (!NativeMethods.GetProcessTimes(processHandle, create, exit, kernel, user))
                            throw new Win32Exception();
                        exitTime = DateTime.FromFileTime(exit[0]);
                        haveExitTime = true;
                    }
                    finally {
                        ReleaseProcessHandle(processHandle);
                    }
                }
                return exitTime;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Handle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the native handle for the associated process. The handle is only available
        ///       if this component started the process.
        ///    </para>
        /// </devdoc>
        public IntPtr Handle {
            get {
                EnsureState(State.Associated);
                return OpenProcessHandle();
            }
        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Id"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the unique identifier for the associated process.
        ///    </para>
        /// </devdoc>
        public int Id {
            get {
                EnsureState(State.HaveId);
                return processId;
            }
        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.StartInfo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the properties to pass into the <see cref='System.Diagnostics.Process.Start'/> method for the <see cref='System.Diagnostics.Process'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public ProcessStartInfo StartInfo {
            get {
                if (startInfo == null)
                    startInfo = new ProcessStartInfo(this);
                return startInfo;
            }
            set {
                if (value == null) throw new ArgumentNullException("value");
                startInfo = value;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.SynchronizingObject"]/*' />
        /// <devdoc>
        ///   Represents the object used to marshal the event handler
        ///   calls issued as a result of a Process exit. Normally 
        ///   this property will  be set when the component is placed 
        ///   inside a control or  a from, since those components are 
        ///   bound to a specific thread.
        /// </devdoc>
        public ISynchronizeInvoke SynchronizingObject {
            get {

                return this.synchronizingObject;
            }
            
            set {
                this.synchronizingObject = value;
            }
        }        
        

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.EnableRaisingEvents"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether the <see cref='System.Diagnostics.Process.Exited'/>
        ///       event is fired
        ///       when the process terminates.
        ///    </para>
        /// </devdoc>
        public bool EnableRaisingEvents {
            get {
                return watchForExit;
            }
            set {
                if (value != watchForExit) {
                    if (Associated) {
                        if (value) {
                            OpenProcessHandle();
                            EnsureWatchingForExit();
                        }
                        else
                            StopWatchingForExit();
                    }
                    watchForExit = value;
                }
            }
        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.StandardInput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public StreamWriter StandardInput {
            get { 
                if (standardInput == null)
                    throw new InvalidOperationException(SR.GetString(SR.CantGetStandardIn));

                return standardInput;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.StandardOutput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public StreamReader StandardOutput {
            get {
                if (standardOutput == null)
                    throw new InvalidOperationException(SR.GetString(SR.CantGetStandardOut));

                return standardOutput;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.StandardError"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public StreamReader StandardError {
            get { 
                if (standardError == null)
                    throw new InvalidOperationException(SR.GetString(SR.CantGetStandardError));

                return standardError;
            }
        }


        //
        // METHODS
        //

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Exited"]/*' />
        public event EventHandler Exited {
            add {
                onExited += value;
            }
            remove {
                onExited -= value;
            }
        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.ReleaseProcessHandle"]/*' />
        /// <devdoc>
        ///     Close the process handle if it has been removed.
        /// </devdoc>
        /// <internalonly/>
        void ReleaseProcessHandle(IntPtr handle) {
            if (haveProcessHandle && handle == processHandle) return;
            if (handle == (IntPtr)0) return;
            Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(process)");
            SafeNativeMethods.CloseHandle(handle);
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.CompletionCallback"]/*' />
        /// <devdoc>
        ///     This is called from the threadpool when a proces exits.
        /// </devdoc>
        /// <internalonly/>
        private void CompletionCallback(object context, bool wasSignaled) {
            StopWatchingForExit();
            RaiseOnExited();      
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Dispose1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Free any resources associated with this component.
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                //Dispose managed and unmanaged resources
                Close();
            }
            else {
                //Dispose unmanaged resources
                if (haveProcessHandle && processHandle != (IntPtr)0) {                    
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(process) in Dispose(false)");
                    SafeNativeMethods.CloseHandle(processHandle);
                    processHandle = (IntPtr)0;
                }                    
            }
            
            this.disposed = true;
            base.Dispose(disposing);
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Frees any resources associated with this component.
        ///    </para>
        /// </devdoc>
        public void Close() {
            if (Associated) {
                if (haveProcessHandle) {
                    StopWatchingForExit();
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(process) in Close()");
                    SafeNativeMethods.CloseHandle(processHandle);
                    processHandle = (IntPtr)0;
                    haveProcessHandle = false;
                }
                haveProcessId = false;
                raisedOnExited = false;
                
                //                             Don't call close on the Readers and writers
                //since they might be referenced by somebody else while the 
                //process is still alive but this method called.
                standardOutput = null;
                standardInput = null;
                standardError = null;

                Refresh();
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.EnsureState"]/*' />
        /// <devdoc>
        ///     Helper method for checking preconditions when accessing properties.
        /// </devdoc>
        /// <internalonly/>
        void EnsureState(State state) {

            if ((state & State.Associated) != (State)0)
                if (!Associated)
                    throw new InvalidOperationException(SR.GetString(SR.NoAssociatedProcess));

            if ((state & State.HaveId) != (State)0) {
                if (!haveProcessId) {
                    throw new InvalidOperationException(SR.GetString(SR.ProcessIdRequired));
                }
            }


            if ((state & State.Exited) != (State)0) {
                if (!HasExited)
                    throw new InvalidOperationException(SR.GetString(SR.WaitTillExit));
                if (!haveProcessHandle)
                    throw new InvalidOperationException(SR.GetString(SR.NoProcessHandle));
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.EnsureWatchingForExit"]/*' />
        /// <devdoc>
        ///     Make sure we are watching for a process exit.
        /// </devdoc>
        /// <internalonly/>
        void EnsureWatchingForExit() {
            if (!watchingForExit) {
                Debug.Assert(haveProcessHandle, "Process.EnsureWatchingForExit called with no process handle");
                Debug.Assert(Associated, "Process.EnsureWatchingForExit called with no associated process");
                watchingForExit = true;
                try {
                    this.waitHandle = new ProcessWaitHandle();
                    this.waitHandle.Handle = processHandle;
                    this.registeredWaitHandle = ThreadPool.RegisterWaitForSingleObject(this.waitHandle,
                        new WaitOrTimerCallback(this.CompletionCallback), null, -1, true);                    
                }
                catch {
                    watchingForExit = false;
                    throw;
                }
            }
        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.GetCurrentProcess"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a new <see cref='System.Diagnostics.Process'/>
        ///       component and associates it with the current active process.
        ///    </para>
        /// </devdoc>
        public static Process GetCurrentProcess() {
            return new Process(NativeMethods.GetCurrentProcessId(), NativeMethods.GetCurrentProcess());
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.OnExited"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Diagnostics.Process.Exited'/> event.
        ///    </para>
        /// </devdoc>
        protected void OnExited() {
            if (onExited != null) {
                if (this.SynchronizingObject != null && this.SynchronizingObject.InvokeRequired)
                    this.SynchronizingObject.BeginInvoke(this.onExited, new object[]{this, EventArgs.Empty});
                else                        
                   onExited(this, EventArgs.Empty);                
            }               
        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.OpenProcessHandle"]/*' />
        /// <devdoc>
        ///     Opens a long-term handle to the process, with all access.  If a handle exists,
        ///     then it is reused.  If the process has exited, it throws an exception.
        /// </devdoc>
        /// <internalonly/>
        IntPtr OpenProcessHandle() {
            if (!haveProcessHandle) {
                //Cannot open a new process handle if the object has been disposed, since finalization has been suppressed.            
                if (this.disposed)
                    throw new ObjectDisposedException(GetType().Name);
                throw new InvalidOperationException(SR.GetString(SR.ProcessIdRequired));
            }                
            return processHandle;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.RaiseOnExited"]/*' />
        /// <devdoc>
        ///     Raise the Exited event, but make sure we don't do it more than once.
        /// </devdoc>
        /// <internalonly/>
        void RaiseOnExited() {
            if (!raisedOnExited) {
                lock (this) {
                    if (!raisedOnExited) {
                        raisedOnExited = true;
                        OnExited();
                    }
                }
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Refresh"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Discards any information about the associated process
        ///       that has been cached inside the process component. After <see cref='System.Diagnostics.Process.Refresh'/> is called, the
        ///       first request for information for each property causes the process component
        ///       to obtain a new value from the associated process.
        ///    </para>
        /// </devdoc>
        public void Refresh() {
            exited = false;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.SetProcessHandle"]/*' />
        /// <devdoc>
        ///     Helper to associate a process handle with this component.
        /// </devdoc>
        /// <internalonly/>
        void SetProcessHandle(IntPtr processHandle) {
            this.processHandle = processHandle;
            this.haveProcessHandle = true;
            if (watchForExit)
                EnsureWatchingForExit();
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.SetProcessId"]/*' />
        /// <devdoc>
        ///     Helper to associate a process id with this component.
        /// </devdoc>
        /// <internalonly/>
        void SetProcessId(int processId) {
            this.processId = processId;
            this.haveProcessId = true;
        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Start"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts a process specified by the <see cref='System.Diagnostics.Process.StartInfo'/> property of this <see cref='System.Diagnostics.Process'/>
        ///       component and associates it with the
        ///    <see cref='System.Diagnostics.Process'/> . If a process resource is reused 
        ///       rather than started, the reused process is associated with this <see cref='System.Diagnostics.Process'/>
        ///       component.
        ///    </para>
        /// </devdoc>
        public bool Start() {
            Close();
            ProcessStartInfo startInfo = StartInfo;
            if (startInfo.FileName.Length == 0) 
                throw new InvalidOperationException(SR.GetString(SR.FileNameMissing));

            if (startInfo.UseShellExecute) {
                throw new InvalidOperationException(SR.GetString(SR.net_perm_invalid_val, "StartInfo.UseShellExecute", true));
            }

            return StartWithCreateProcess(startInfo);
        }


        private void CreatePipe(out IntPtr parentHandle, out IntPtr childHandle, bool parentInputs) {
            NativeMethods.SecurityAttributes securityAttributesParent = new NativeMethods.SecurityAttributes();
            securityAttributesParent.bInheritHandle = true;
            if (parentInputs) {
                NativeMethods.IntCreatePipe(out childHandle, 
                                                        out parentHandle, 
                                                        securityAttributesParent, 
                                                        0);
                                                        
            } 
            else {
                NativeMethods.IntCreatePipe(out parentHandle, 
                                                        out childHandle, 
                                                        securityAttributesParent, 
                                                        0);                                                                              
            }
        }

        private bool StartWithCreateProcess(ProcessStartInfo startInfo) {
            
            // See knowledge base article Q190351 for an explanation of the following code.  Noteworthy tricky points:
            //    * the handles are duplicated as non-inheritable before they are passed to CreateProcess so
            //      that the child process can close them
            //    * CreateProcess allows you to redirrect all or none of the standard io handles, so we use
            //      GetStdHandle for the handles X for which RedirectStandardX == false
            //    * It is important to close our copy of the "child" ends of the pipes after we hand them of or 
            //      the read/writes will fail

            //Cannot start a new process and store its handle if the object has been disposed, since finalization has been suppressed.            
            if (this.disposed)
                throw new ObjectDisposedException(GetType().Name);

            NativeMethods.CreateProcessStartupInfo startupInfo = new NativeMethods.CreateProcessStartupInfo();
            NativeMethods.CreateProcessProcessInformation processInfo = new NativeMethods.CreateProcessProcessInformation();
            
            // Construct a StringBuilder with the appropriate command line
            // to pass to CreateProcess.  If the filename isn't already 
            // in quotes, we quote it here.  This prevents some security
            // problems (it specifies exactly which part of the string
            // is the file to execute).
            StringBuilder commandLine = new StringBuilder();
            string fileName = startInfo.FileName.Trim();
            string arguments = startInfo.Arguments;
            bool fileNameIsQuoted = (fileName.StartsWith("\"") && fileName.EndsWith("\""));
            if (!fileNameIsQuoted) commandLine.Append("\"");
            commandLine.Append(fileName);
            if (!fileNameIsQuoted) commandLine.Append("\"");
            if (arguments != null && arguments.Length > 0) {
                commandLine.Append(" ");
                commandLine.Append(arguments);
            }

            IntPtr hStdInReadPipe  = NativeMethods.INVALID_HANDLE_VALUE, hStdInWritePipe  = NativeMethods.INVALID_HANDLE_VALUE;
            IntPtr hStdOutReadPipe = NativeMethods.INVALID_HANDLE_VALUE, hStdOutWritePipe = NativeMethods.INVALID_HANDLE_VALUE;
            IntPtr hStdErrReadPipe = NativeMethods.INVALID_HANDLE_VALUE, hStdErrWritePipe = NativeMethods.INVALID_HANDLE_VALUE;
            bool needToCloseIn = false, needToCloseOut = false, needToCloseErr = false;
            GCHandle environmentHandle = new GCHandle();

            try {

                // set up the streams
                if (startInfo.RedirectStandardInput || startInfo.RedirectStandardOutput || startInfo.RedirectStandardError) {                        
                    if (startInfo.RedirectStandardInput) {
                        CreatePipe(out hStdInWritePipe, out hStdInReadPipe, true);                                                                                                                                    
                        needToCloseIn = true;
                    } else {
                        hStdInReadPipe = NativeMethods.GetStdHandle(NativeMethods.STD_INPUT_HANDLE);
                    }
    
                    if (startInfo.RedirectStandardOutput) {
                        CreatePipe(out hStdOutReadPipe, out hStdOutWritePipe, false);                                                                                                                                                                                    
                        needToCloseOut = true;
                    } else {
                        hStdOutWritePipe = NativeMethods.GetStdHandle(NativeMethods.STD_OUTPUT_HANDLE);
                    }
    
                    if (startInfo.RedirectStandardError) {
                        CreatePipe(out hStdErrReadPipe, out hStdErrWritePipe, false);                                                                                                                                                                                                                                    
                        needToCloseErr = true;
                    } else {
                        hStdErrWritePipe = NativeMethods.GetStdHandle(NativeMethods.STD_ERROR_HANDLE);
                    }
    
                    startupInfo.dwFlags = NativeMethods.STARTF_USESTDHANDLES;
                    startupInfo.hStdInput  = hStdInReadPipe;
                    startupInfo.hStdOutput = hStdOutWritePipe;
                    startupInfo.hStdError  = hStdErrWritePipe;        
                }
    
                // set up the creation flags paramater
                int creationFlags = 0;

                // set up the environment block parameter
                IntPtr environmentPtr = (IntPtr)0;
                if (startInfo.environmentVariables != null) {
                    string environmentString = EnvironmentBlock.ToString(startInfo.environmentVariables);
                    byte[] environmentBytes = null;
                    environmentBytes = Encoding.Default.GetBytes(environmentString);
                    environmentHandle = GCHandle.Alloc(environmentBytes, GCHandleType.Pinned);
                    environmentPtr = environmentHandle.AddrOfPinnedObject();
                }

                string workingDirectory = startInfo.WorkingDirectory;
                if (workingDirectory == string.Empty)
                    workingDirectory = null;

                if (!NativeMethods.CreateProcess (
                        null,               // we don't need this since all the info is in commandLine
                        commandLine,        // pointer to the command line string
                        null,               // pointer to process security attributes, we don't need to inheriat the handle
                        null,               // pointer to thread security attributes
                        true,               // handle inheritance flag
                        creationFlags,      // creation flags
                        environmentPtr,     // pointer to new environment block
                        workingDirectory,   // pointer to current directory name
                        startupInfo,        // pointer to STARTUPINFO
                        processInfo         // pointer to PROCESS_INFORMATION
                    )) {
                    throw new Win32Exception();
                }
                 
            }
            finally {
                // free environment block
                if (environmentHandle.IsAllocated)
                    environmentHandle.Free();   
            
                // clean up handles
                if (processInfo.hThread != NativeMethods.INVALID_HANDLE_VALUE) {
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(hThread)");
                    SafeNativeMethods.CloseHandle(processInfo.hThread);
                }
                if (needToCloseIn) {
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(stdIn)");
                    SafeNativeMethods.CloseHandle(hStdInReadPipe);
                }
                if (needToCloseOut) {
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(stdOut)");
                    SafeNativeMethods.CloseHandle(hStdOutWritePipe);
                }
                if (needToCloseErr) {
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(stdErr)");
                    SafeNativeMethods.CloseHandle(hStdErrWritePipe);
                }
            }
            
            Encoding enc = null;
            if (startInfo.RedirectStandardInput) {
                enc = Encoding.GetEncoding(NativeMethods.GetConsoleCP()); 
                standardInput = new StreamWriter(new FileStream(hStdInWritePipe, FileAccess.Write, true, 4096, true), enc);
                standardInput.AutoFlush = true;
            }
            if (startInfo.RedirectStandardOutput) {
                enc = Encoding.GetEncoding(NativeMethods.GetConsoleOutputCP()); 
                standardOutput = new StreamReader(new FileStream(hStdOutReadPipe, FileAccess.Read, true, 4096, true), enc);
            }
            if (startInfo.RedirectStandardError) {
                enc = Encoding.GetEncoding(NativeMethods.GetConsoleOutputCP()); 
                standardError = new StreamReader(new FileStream(hStdErrReadPipe, FileAccess.Read, true, 4096, true), enc);
            }
            
            if (processInfo.hProcess != (IntPtr)0) {
                SetProcessHandle(processInfo.hProcess);
                SetProcessId(processInfo.dwProcessId);
                return true;
            }
            else
                return false;

        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Start1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts a process resource by specifying the name of a
        ///       document or application file. Associates the process resource with a new <see cref='System.Diagnostics.Process'/>
        ///       component.
        ///    </para>
        /// </devdoc>
        public static Process Start(string fileName) {
            return Start(new ProcessStartInfo(fileName));
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Start2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts a process resource by specifying the name of an
        ///       application and a set of command line arguments. Associates the process resource
        ///       with a new <see cref='System.Diagnostics.Process'/>
        ///       component.
        ///    </para>
        /// </devdoc>
        public static Process Start(string fileName, string arguments) {
            return Start(new ProcessStartInfo(fileName, arguments));
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Start3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts a process resource specified by the process start
        ///       information passed in, for example the file name of the process to start.
        ///       Associates the process resource with a new <see cref='System.Diagnostics.Process'/>
        ///       component.
        ///    </para>
        ///    <note type="rnotes">
        ///    </note>
        /// </devdoc>
        public static Process Start(ProcessStartInfo startInfo) {
            Process process = new Process();
            if (startInfo == null) throw new ArgumentNullException("startInfo");
            process.StartInfo = startInfo;
            if (process.Start())
                return process;
            return null;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Kill"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Stops the
        ///       associated process immediately.
        ///    </para>
        /// </devdoc>
        public void Kill() {
            IntPtr processHandle = (IntPtr)0;
            try {
                processHandle = OpenProcessHandle();
                if (!NativeMethods.TerminateProcess(processHandle, -1))
                    throw new Win32Exception();
            }
            finally {
                ReleaseProcessHandle(processHandle);
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.StopWatchingForExit"]/*' />
        /// <devdoc>
        ///     Make sure we are not watching for process exit.
        /// </devdoc>
        /// <internalonly/>
        void StopWatchingForExit() {
            if (watchingForExit) {
                lock (this) {
                    if (watchingForExit) {
                        watchingForExit = false;
                        registeredWaitHandle.Unregister(waitHandle);                
                        waitHandle = null;
                        registeredWaitHandle = null;
                    }
                }
            }
        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.WaitForExit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Instructs the <see cref='System.Diagnostics.Process'/> component to wait the specified number of milliseconds for the associated process to exit.
        ///    </para>
        /// </devdoc>
        public bool WaitForExit(int milliseconds) {
            IntPtr processHandle = (IntPtr)0;
            bool exited;
            try {
                processHandle = OpenProcessHandle();
                if (processHandle == NativeMethods.INVALID_HANDLE_VALUE)
                    exited = true;
                else {
                    int result = NativeMethods.WaitForSingleObject(processHandle, milliseconds);
                    if (result == NativeMethods.WAIT_OBJECT_0)
                        exited = true;
                    else if (result == NativeMethods.WAIT_TIMEOUT)
                        exited = false;
                    else
                        throw new Win32Exception();
                }
            }
            finally {
                ReleaseProcessHandle(processHandle);
            }
            if (exited && watchForExit)
                RaiseOnExited();
            return exited;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.WaitForExit1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Instructs the <see cref='System.Diagnostics.Process'/> component to wait
        ///       indefinitely for the associated process to exit.
        ///    </para>
        /// </devdoc>
        public void WaitForExit() {
            WaitForExit(Int32.MaxValue);
        }


        /// <summary>
        ///     A desired internal state.
        /// </summary>
        /// <internalonly/>
        enum State {
            HaveId = 0x1,
            Exited = 0x10,
            Associated = 0x20,
        }
    }


    internal class EnvironmentBlock {
        public static void toStringDictionary(IntPtr bufferPtr, StringDictionary sd) {
            string[] subs;
            char[] splitter = new char[] {'='};
            string entry = Marshal.PtrToStringAnsi(bufferPtr);
            while (entry.Length > 0) {
                subs = entry.Split(splitter);
                if (subs.Length != 2) 
                    throw new InvalidOperationException(SR.GetString(SR.EnvironmentBlock));
                    
                sd.Add(subs[0], subs[1]);
                bufferPtr = (IntPtr)((long)bufferPtr + entry.Length + 1);
                entry = Marshal.PtrToStringAnsi(bufferPtr);
            }
        }

        public static string ToString(StringDictionary sd) {
            // get the keys
            string[] keys = new string[sd.Count];
            sd.Keys.CopyTo(keys, 0);
            
            // get the values
            string[] values = new string[sd.Count];
            sd.Values.CopyTo(values, 0);
            
            // sort both by the keys
            Array.Sort(keys, values, Comparer.Default);

            // create a list of null terminated "key=val" strings
            StringBuilder stringBuff = new StringBuilder();
            for (int i = 0; i < sd.Count; ++ i) {
                stringBuff.Append(keys[i]);
                stringBuff.Append('=');
                stringBuff.Append(values[i]);
                stringBuff.Append('\0');
            }
            // an extra null at the end indicates end of list.
            stringBuff.Append('\0');
            
            return stringBuff.ToString();
        }
    }
}
