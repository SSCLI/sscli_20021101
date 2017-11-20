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
/*============================================================
**
** Class: Environment
**
**                                
**
** Purpose: Provides some basic access to some environment 
** functionality.
**
** Date: March 3, 2000
**
============================================================*/
namespace System {
    using System.IO;
    using System.Security;
    using System.Resources;
    using System.Globalization;
    using System.Collections;
    using System.Security.Permissions;
    using System.Text;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.Reflection;
    using System.Diagnostics;
    using Microsoft.Win32;
    using System.Runtime.CompilerServices;

    /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment"]/*' />
    public sealed class Environment {

        internal static ResourceManager SystemResMgr;
        // To avoid infinite loops when calling GetResourceString.  See comments
        // in GetResourceString for these two fields.
        internal static Object m_resMgrLockObject;
        internal static bool m_loadingResource;

        private static String m_machineName;
        private const  int    MaxMachineNameLength = 256;


        private Environment() {}              // Prevent from begin created

        /*==================================TickCount===================================
        **Action: Gets the number of ticks since the system was started.
        **Returns: The number of ticks since the system was started.
        **Arguments: None
        **Exceptions: None
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.TickCount"]/*' />
        public static int TickCount {
            get {
                return nativeGetTickCount();
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int nativeGetTickCount();
        
        // Terminates this process with the given exit code.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void ExitNative(int exitCode);
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.Exit"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Exit(int exitCode) {
            ExitNative(exitCode);
        }


        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.ExitCode"]/*' />
        public static int ExitCode {
            get {
                return nativeGetExitCode();
            }

            set {
                nativeSetExitCode(value);
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void nativeSetExitCode(int exitCode);
    
        // Gets the exit code of the process.
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int nativeGetExitCode();


        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.CommandLine"]/*' />
        public static String CommandLine {
            get {
        new EnvironmentPermission(EnvironmentPermissionAccess.Read, "Path").Demand();
                return GetCommandLineNative();
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String GetCommandLineNative();
        
        /*===============================CurrentDirectory===============================
        **Action:  Provides a getter and setter for the current directory.  The original
        **         current directory is the one from which the process was started.  
        **Returns: The current directory (from the getter).  Void from the setter.
        **Arguments: The current directory to which to switch to the setter.
        **Exceptions: 
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.CurrentDirectory"]/*' />
        public static String CurrentDirectory {
            get{
                return Directory.GetCurrentDirectory();
            }

            set { 
                Directory.SetCurrentDirectory(value);
            }
        }

        // Returns the system directory (ie, C:\WinNT\System32).
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.SystemDirectory"]/*' />
        public static String SystemDirectory {
            get {
                StringBuilder sb = new StringBuilder(Path.MAX_PATH);
                int r = Win32Native.GetSystemDirectory(sb, Path.MAX_PATH);
                BCLDebug.Assert(r < Path.MAX_PATH, "r < Path.MAX_PATH");
                if (r==0) __Error.WinIOError();
                String path = sb.ToString();
                
                // Do security check
                new FileIOPermission(FileIOPermissionAccess.Read, path).Demand();

                return path;
            }
        }


        /*==============================GetCommandLineArgs==============================
        **Action: Gets the command line and splits it appropriately to deal with whitespace,
        **        quotes, and escape characters.
        **Returns: A string array containing your command line arguments.
        **Arguments: None
        **Exceptions: None.
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.GetCommandLineArgs"]/*' />
        public static String[] GetCommandLineArgs() {
            new EnvironmentPermission(EnvironmentPermissionAccess.Read, "Path").Demand();
            return GetCommandLineArgsNative();
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String[] GetCommandLineArgsNative();
        
        /*============================GetEnvironmentVariable============================
        **Action:
        **Returns:
        **Arguments:
        **Exceptions:
        ==============================================================================*/
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String nativeGetEnvironmentVariable(String variable);
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.GetEnvironmentVariable"]/*' />
        public static String GetEnvironmentVariable(String variable)
        {
            if (variable == null)
                throw new ArgumentNullException("variable");
            (new EnvironmentPermission(EnvironmentPermissionAccess.Read, variable)).Demand();
            return nativeGetEnvironmentVariable(variable);
        }
        
        /*===========================GetEnvironmentVariables============================
        **Action: Returns an IDictionary containing all enviroment variables and their values.
        **Returns: An IDictionary containing all environment variables and their values.
        **Arguments: None.
        **Exceptions: None.
        ==============================================================================*/
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern char[] nativeGetEnvironmentCharArray();
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.GetEnvironmentVariables"]/*' />
        public static IDictionary GetEnvironmentVariables()
        {
            new EnvironmentPermission(PermissionState.Unrestricted).Demand();
            
            char[] block = nativeGetEnvironmentCharArray();
            
            Hashtable table = new Hashtable(20);
            
            // Copy strings out, parsing into pairs and inserting into the table.
            // The first few environment variable entries start with an '='!
            // The current working directory of every drive (except for those drives
            // you haven't cd'ed into in your DOS window) are stored in the 
            // environment block (as =C:=pwd) and the program's exit code is 
            // as well (=ExitCode=00000000)  Skip all that start with =.
            // Read docs about Environment Blocks on MSDN's CreateProcess page.
            
            // Format for GetEnvironmentStrings is:
            // (=HiddenVar=value\0 | Variable=value\0)* \0
            // See the description of Environment Blocks in MSDN's
            // CreateProcess page (null-terminated array of null-terminated strings).
            // Note the =HiddenVar's aren't always at the beginning.
            
            // GetEnvironmentCharArray will not return the trailing 0 to terminate
            // the array - we have the array length instead.
            for(int i=0; i<block.Length; i++) {
                int startKey = i;
                // Skip to key
                while(block[i]!='=')
                    i++;
                // Skip over environment variables starting with '='
                if (i-startKey==0) {
                    while(block[i]!=0) 
                        i++;
                    continue;
                }
                String key = new String(block, startKey, i-startKey);
                i++;  // skip over '='
                int startValue = i;
                while(block[i]!=0)  // Read to end of this entry
                    i++;
                String value = new String(block, startValue, i-startValue);
                // skip over 0 handled by for loop's i++
                table[key]=value;
            }
                
            return table;
        }
        
        /*===============================GetLogicalDrives===============================
        **Action: Retrieves the names of the logical drives on this machine in the  form "C:\". 
        **Arguments:   None.
        **Exceptions:  IOException.
        **Permissions: SystemInfo Permission.
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.GetLogicalDrives"]/*' />
        public static String[] GetLogicalDrives() {
#if !PLATFORM_UNIX
            new EnvironmentPermission(PermissionState.Unrestricted).Demand();
                                 
            int drives = Win32Native.GetLogicalDrives();
            if (drives==0)
                __Error.WinIOError();
            uint d = (uint)drives;
            int count = 0;
            while (d != 0) {
                if (((int)d & 1) != 0) count++;
                d >>= 1;
            }
            String[] result = new String[count];
            char[] root = new char[] {'A', ':', '\\'};
            d = (uint)drives;
            count = 0;
            while (d != 0) {
                if (((int)d & 1) != 0) {
                    result[count++] = new String(root);
                }
                d >>= 1;
                root[0]++;
            }
            return result;
#else
            return new String[0];
#endif // !PLATFORM_UNIX
        }
        
        /*===================================NewLine====================================
        **Action: A property which returns the appropriate newline string for the given
        **        platform.
        **Returns: \r\n on Win32.
        **Arguments: None.
        **Exceptions: None.
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.NewLine"]/*' />
        public static String NewLine {
            get {
#if !PLATFORM_UNIX
                return "\r\n";
#else
                return "\n";
#endif // !PLATFORM_UNIX
            }
        }

        
        
        /*===================================Version====================================
        **Action: Returns the COM+ version struct, describing the build number.
        **Returns:
        **Arguments:
        **Exceptions:
        ==============================================================================*/
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String nativeGetVersion();
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.Version"]/*' />
        public static Version Version {
            get {
                String [] versioninfo = nativeGetVersion().Split(new Char[] {'.',' '});
                                
                BCLDebug.Assert(versioninfo.Length>=4,"versioninfo.Length>=4");
                return new Version(Int32.Parse(versioninfo[0], CultureInfo.InvariantCulture),
                                    Int32.Parse(versioninfo[1], CultureInfo.InvariantCulture),
                                    Int32.Parse(versioninfo[2], CultureInfo.InvariantCulture),
                                    Int32.Parse(versioninfo[3], CultureInfo.InvariantCulture));
            }
        }

        
        /*==================================WorkingSet==================================
        **Action:
        **Returns:
        **Arguments:
        **Exceptions:
        ==============================================================================*/
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern long nativeGetWorkingSet();
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.WorkingSet"]/*' />
        public static long WorkingSet {
            get {
                new EnvironmentPermission(PermissionState.Unrestricted).Demand();
                return (long)nativeGetWorkingSet();
            }
        }

        
        /*==================================StackTrace==================================
        **Action:
        **Returns:
        **Arguments:
        **Exceptions:
        ==============================================================================*/
        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.StackTrace"]/*' />
        public static String StackTrace {
            get {
                new EnvironmentPermission(PermissionState.Unrestricted).Demand();
                return GetStackTrace(null);
            }
        }

        [ReflectionPermissionAttribute(SecurityAction.Assert, TypeInformation=true)]
        internal static String GetStackTrace(Exception e)
        {
            StackTrace st;
            if (e == null)
                st = new StackTrace(true);
            else
                st = new StackTrace(e,true);
            String newLine = Environment.NewLine;
            StringBuilder sb = new StringBuilder(255);
    
            for (int i = 0; i < st.FrameCount; i++)
            {
                StackFrame sf = st.GetFrame(i);
 
                sb.Append("   at ");
            
                MethodBase method = sf.GetMethod();
                Type t = method.DeclaringType;
                if (t != null)
                {
                    String nameSpace = t.Namespace;
                    if (nameSpace != null)
                    {
                        sb.Append(nameSpace);
                        if (sb != null)
                            sb.Append(".");
                    }

                    sb.Append(t.Name);
                    sb.Append(".");
                }
                sb.Append(method.Name);
                sb.Append("(");

                ParameterInfo[] arrParams = method.GetParameters();

                for (int j = 0; j < arrParams.Length; j++) 
                {
                    String typeName = "<UnknownType>";
                    if (arrParams[j].ParameterType != null)
                        typeName = arrParams[j].ParameterType.Name;

                    sb.Append((j != 0 ? ", " : "") + typeName + " " + arrParams[j].Name);
                }

                sb.Append(")");
         
                if (sf.GetILOffset() != -1)
                {
                    // It's possible we have a debug version of an executable but no PDB.  In
                    // this case, the file name will be null.
                    String fileName = sf.GetFileName();

                    if (fileName != null)
                        sb.Append(" in " + fileName + ":line " + sf.GetFileLineNumber());
                }
            
                if (i != st.FrameCount - 1)                 
                    sb.Append(newLine);
            }
            return sb.ToString();
        }

        private static ResourceManager InitResourceManager()
        {
            if (SystemResMgr == null) {
                lock(typeof(Environment)) {
                    if (SystemResMgr == null) {
                        // Do not reorder these two field assignments.
                        m_resMgrLockObject = new Object();
                        SystemResMgr = new ResourceManager("mscorlib", typeof(String).Assembly);
                    }
                }
            }
            return SystemResMgr;
        }
        
        // This should ideally become visible only within mscorlib's Assembly.
        // 
        // Looks up the resource string value for key.
        // 
        internal static String GetResourceString(String key)
        {
            if (SystemResMgr == null)
                InitResourceManager();
            String s;
            // We unfortunately have a somewhat common potential for infinite 
            // loops with mscorlib's ResourceManager.  If "potentially dangerous"
            // code throws an exception, we will get into an infinite loop
            // inside the ResourceManager and this "potentially dangerous" code.
            // Potentially dangerous code includes the IO package, CultureInfo,
            // parts of the loader, some parts of Reflection, Security (including 
            // custom user-written permissions that may parse an XML file at
            // class load time), assembly load event handlers, etc.  Essentially,
            // this is not a bounded set of code, and we need to fix the problem.
            // Fortunately, this is limited to mscorlib's error lookups and is NOT
            // a general problem for all user code using the ResourceManager.
            
            // The solution is to make sure only one thread at a time can call 
            // GetResourceString.  If the same thread comes into GetResourceString
            // twice before returning, we're going into an infinite loop and we
            // should return a bogus string.                                       
            // Note: typeof(Environment) is used elsewhere - don't lock on it.
            lock(m_resMgrLockObject) {
                if (m_loadingResource) {
                    // This may be bad, depending on how it happens.
                    BCLDebug.Correctness(false, "Infinite recursion during resource lookup.  Resource name: "+key);
                    return "[Resource lookup failed - infinite recursion detected.  Resource name: "+key+']';
                }
                m_loadingResource = true;
                s = SystemResMgr.GetString(key, null);
                m_loadingResource = false;
            }
            BCLDebug.Assert(s!=null, "Managed resource string lookup failed.  Was your resource name misspelled?  Did you rebuild mscorlib after adding a resource to resources.txt?  Debug this w/ cordbg and bug whoever owns the code that called Environment.GetResourceString.  Resource name was: \""+key+"\"");
            return s;
        }

        internal static String GetResourceString(String key, params Object[]values)
        {
            if (SystemResMgr == null)
                InitResourceManager();
            String s;
            // We unfortunately have a somewhat common potential for infinite 
            // loops with mscorlib's ResourceManager.  If "potentially dangerous"
            // code throws an exception, we will get into an infinite loop
            // inside the ResourceManager and this "potentially dangerous" code.
            // Potentially dangerous code includes the IO package, CultureInfo,
            // parts of the loader, some parts of Reflection, Security (including 
            // custom user-written permissions that may parse an XML file at
            // class load time), assembly load event handlers, etc.  Essentially,
            // this is not a bounded set of code, and we need to fix the problem.
            // Fortunately, this is limited to mscorlib's error lookups and is NOT
            // a general problem for all user code using the ResourceManager.
            
            // The solution is to make sure only one thread at a time can call 
            // GetResourceString.  If the same thread comes into GetResourceString
            // twice before returning, we're going into an infinite loop and we
            // should return a bogus string.                                       
            // Note: typeof(Environment) is used elsewhere - don't lock on it.
            lock(m_resMgrLockObject) {
                if (m_loadingResource)
                    return "[Resource lookup failed - infinite recursion detected.  Resource name: "+key+']';
                m_loadingResource = true;
                s = SystemResMgr.GetString(key, null);
                m_loadingResource = false;
            }
            BCLDebug.Assert(s!=null, "Managed resource string lookup failed.  Was your resource name misspelled?  Did you rebuild mscorlib after adding a resource to resources.txt?  Debug this w/ cordbg and bug whoever owns the code that called Environment.GetResourceString.  Resource name was: \""+key+"\"");
            return String.Format(s, values);
        }

        /// <include file='doc\Environment.uex' path='docs/doc[@for="Environment.HasShutdownStarted"]/*' />
        public bool HasShutdownStarted {
            get { return nativeHasShutdownStarted(); }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool nativeHasShutdownStarted();

    }
}
