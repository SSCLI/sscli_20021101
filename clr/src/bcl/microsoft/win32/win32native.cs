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
** Class:  Microsoft.Win32.Native
**
**                                         
**
** Purpose: The CLR wrapper for all Win32 (Win2000, NT4, Win9x, Wince, etc.)
**          native operations.
**
** Date:  September 28, 1999
**
===========================================================*/
/**
 * Notes to PInvoke users:  Getting the syntax exactly correct is crucial, and
 * more than a little confusing.  Here's some guidelines.
 *
 * Use IntPtr for all OS handles and pointers.  IntPtr will do the right thing
 * when porting to 64 bit platforms, and MUST be used in all of our public
 * APIs (and therefore in our private APIs too, once we get 64 bit builds working).
 *
 * If you have a method that takes a native struct, you have two options for
 * declaring that struct.  You can make it a value class ('struct' in CSharp), or
 * a normal class.  This choice doesn't seem very interesting, but your function
 * prototype must use different syntax depending on your choice.  For example,
 * if your native method is prototyped as such:
 *
 *    bool GetVersionEx(OSVERSIONINFO & lposvi);
 *
 *
 * you must EITHER THIS OR THE NEXT syntax:
 *
 *    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
 *    internal struct OSVERSIONINFO {  ...  }
 *
 *    [DllImport(KERNEL32, CharSet=CharSet.Auto)]
 *    internal static extern bool GetVersionEx(ref OSVERSIONINFO lposvi);
 *
 * OR:
 *
 *    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
 *    internal class OSVERSIONINFO {  ...  }
 *
 *    [DllImport(KERNEL32, CharSet=CharSet.Auto)]
 *    internal static extern bool GetVersionEx([In, Out] OSVERSIONINFO lposvi);
 *
 * Note that classes require being marked as [In, Out] while value classes must
 * be passed as ref parameters.
 *
 * Also note the CharSet.Auto on GetVersionEx - while it does not take a String
 * as a parameter, the OSVERSIONINFO contains an embedded array of TCHARs, so
 * the size of the struct varies on different platforms, and there's a
 * GetVersionExA & a GetVersionExW.  Also, the OSVERSIONINFO struct has a sizeof
 * field so the OS can ensure you've passed in the correctly-sized copy of an
 * OSVERSIONINFO.  You must explicitly set this using Marshal.SizeOf(Object);
 */

namespace Microsoft.Win32 {

    using System;
    using System.Security;
    using System.Text;
    using System.Configuration.Assemblies;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;

    /**
     * Win32 encapsulation for MSCORLIB.
     */
    // Remove the default demands for all N/Direct methods with this
    // global declaration on the class.
    //

    [SuppressUnmanagedCodeSecurityAttribute()]
    internal sealed class Win32Native {

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        internal class OSVERSIONINFO {
            public OSVERSIONINFO() {
                OSVersionInfoSize = (int)Marshal.SizeOf(this);
            }

            // The OSVersionInfoSize field must be set to Marshal.SizeOf(this)
            internal int OSVersionInfoSize = 0;
            internal int MajorVersion = 0;
            internal int MinorVersion = 0;
            internal int BuildNumber = 0;
            internal int PlatformId = 0;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=128)]
            internal String CSDVersion = null;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal class SECURITY_ATTRIBUTES {
            internal int nLength = 0;
            internal int lpSecurityDescriptor = 0;
            internal int bInheritHandle = 0;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        internal struct WIN32_FILE_ATTRIBUTE_DATA {
            internal int fileAttributes;
            internal uint ftCreationTimeLow;
            internal uint ftCreationTimeHigh;
            internal uint ftLastAccessTimeLow;
            internal uint ftLastAccessTimeHigh;
            internal uint ftLastWriteTimeLow;
            internal uint ftLastWriteTimeHigh;
            internal int fileSizeHigh;
            internal int fileSizeLow;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct FILE_TIME {
            public FILE_TIME(long fileTime) {
                lowDateTime = (int) fileTime;
                highDateTime = (int) (fileTime >> 32);
            }

            internal int lowDateTime;
            internal int highDateTime;
        }


 #if !PLATFORM_UNIX
        internal const String DLLPREFIX = "";
        internal const String DLLSUFFIX = ".dll";
 #else // !PLATFORM_UNIX
  #if __APPLE__
        internal const String DLLPREFIX = "lib";
        internal const String DLLSUFFIX = ".dylib";
  #else
        internal const String DLLPREFIX = "lib";
        internal const String DLLSUFFIX = ".so";
  #endif
 #endif // !PLATFORM_UNIX

        internal const String KERNEL32 = DLLPREFIX + "rotor_pal" + DLLSUFFIX;
        internal const String USER32   = DLLPREFIX + "rotor_pal" + DLLSUFFIX;
        internal const String ADVAPI32 = DLLPREFIX + "rotor_pal" + DLLSUFFIX;
        internal const String OLE32    = DLLPREFIX + "rotor_pal" + DLLSUFFIX;
        internal const String OLEAUT32 = DLLPREFIX + "rotor_palrt" + DLLSUFFIX;
        internal const String SHIM     = DLLPREFIX + "sscoree" + DLLSUFFIX;
        internal const String FUSION   = DLLPREFIX + "fusion" + DLLSUFFIX;


        internal const String LSTRCPY  = "lstrcpy";
        internal const String LSTRCPYN = "lstrcpyn";
        internal const String LSTRLEN  = "lstrlen";
        internal const String LSTRLENA = "lstrlenA";
        internal const String LSTRLENW = "lstrlenW";
        internal const String MOVEMEMORY = "RtlMoveMemory";


        // From WinBase.h
        internal const int SEM_FAILCRITICALERRORS = 1;

        [DllImport(KERNEL32, CharSet=CharSet.Auto, SetLastError=true)]
        internal static extern bool GetVersionEx([In, Out] OSVERSIONINFO ver);

        [DllImport(KERNEL32, CharSet=CharSet.Auto)]
        internal static extern int FormatMessage(int dwFlags, IntPtr lpSource,
                    int dwMessageId, int dwLanguageId, StringBuilder lpBuffer,
                    int nSize, IntPtr va_list_arguments);

        [DllImport(KERNEL32)]
        internal static extern IntPtr LocalAlloc(int uFlags, IntPtr sizetdwBytes);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern IntPtr LocalFree(IntPtr handle);

        [DllImport(KERNEL32, CharSet=CharSet.Auto)]
        internal static extern uint GetTempPath(int bufferLen, StringBuilder buffer);

        [DllImport(KERNEL32, CharSet=CharSet.Auto, EntryPoint=LSTRCPY)]
        internal static extern IntPtr lstrcpy(IntPtr dst, String src);

        [DllImport(KERNEL32, CharSet=CharSet.Auto, EntryPoint=LSTRCPY)]
        internal static extern IntPtr lstrcpy(StringBuilder dst, IntPtr src);

        [DllImport(KERNEL32, CharSet=CharSet.Auto, EntryPoint=LSTRCPYN)]
        internal static extern IntPtr lstrcpyn(Delegate d1, Delegate d2, int cb);

        [DllImport(KERNEL32, CharSet=CharSet.Auto, EntryPoint=LSTRLEN)]
        internal static extern int lstrlen(sbyte [] ptr);

        [DllImport(KERNEL32, CharSet=CharSet.Auto, EntryPoint=LSTRLEN)]
        internal static extern int lstrlen(IntPtr ptr);

        [DllImport(KERNEL32, CharSet=CharSet.Ansi, EntryPoint=LSTRLENA)]
        internal static extern int lstrlenA(IntPtr ptr);

        [DllImport(KERNEL32, CharSet=CharSet.Unicode, EntryPoint=LSTRLENW)]
        internal static extern int lstrlenW(IntPtr ptr);

        [DllImport(Win32Native.OLEAUT32, CharSet=CharSet.Unicode)]
        internal static extern IntPtr SysAllocStringLen(String src, int len);  // BSTR

        [DllImport(Win32Native.OLEAUT32)]
        internal static extern int SysStringLen(IntPtr bstr);

        [DllImport(Win32Native.OLEAUT32)]
        internal static extern void SysFreeString(IntPtr bstr);

        [DllImport(KERNEL32, CharSet=CharSet.Unicode, EntryPoint=MOVEMEMORY)]
        internal static extern void CopyMemoryUni(IntPtr pdst, String psrc, IntPtr sizetcb);

        [DllImport(KERNEL32, CharSet=CharSet.Unicode, EntryPoint=MOVEMEMORY)]
        internal static extern void CopyMemoryUni(StringBuilder pdst,
                    IntPtr psrc, IntPtr sizetcb);

        [DllImport(KERNEL32, CharSet=CharSet.Ansi, EntryPoint=MOVEMEMORY)]
        internal static extern void CopyMemoryAnsi(IntPtr pdst, String psrc, IntPtr sizetcb);

        [DllImport(KERNEL32, CharSet=CharSet.Ansi, EntryPoint=MOVEMEMORY)]
        internal static extern void CopyMemoryAnsi(StringBuilder pdst,
                    IntPtr psrc, IntPtr sizetcb);


        [DllImport(KERNEL32)]
        internal static extern int GetACP();

        // For GetFullPathName, the last param is a useless TCHAR**, set by native.
        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern int GetFullPathName(String path, int numBufferChars, StringBuilder buffer, IntPtr mustBeZero);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern IntPtr CreateFile(String lpFileName,
                    int dwDesiredAccess, int dwShareMode,
                    IntPtr lpSecurityAttributes, int dwCreationDisposition,
                    int dwFlagsAndAttributes, IntPtr hTemplateFile);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern IntPtr CreateFile(String lpFileName,
                    int dwDesiredAccess, System.IO.FileShare dwShareMode,
                    SECURITY_ATTRIBUTES securityAttrs, System.IO.FileMode dwCreationDisposition,
                    int dwFlagsAndAttributes, IntPtr hTemplateFile);

        [DllImport(KERNEL32)]
        internal static extern bool CloseHandle(IntPtr handle);

        [DllImport(KERNEL32)]
        internal static extern int GetFileType(IntPtr handle);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool SetEndOfFile(IntPtr hFile);

        [DllImport(KERNEL32, SetLastError=true, EntryPoint="SetFilePointer")]
        private unsafe static extern int SetFilePointerWin32(IntPtr handle, int lo, int * hi, int origin);

        internal unsafe static long SetFilePointer(IntPtr handle, long offset, System.IO.SeekOrigin origin, out int hr) {
            hr = 0;
            int lo = (int) offset;
            int hi = (int) (offset >> 32);
            lo = SetFilePointerWin32(handle, lo, &hi, (int) origin);

            if (lo == -1 && ((hr = Marshal.GetLastWin32Error()) != 0))
                return -1;
            return (long) (((ulong) ((uint) hi)) << 32) | ((uint) lo);
        }

        [DllImport(KERNEL32, CharSet=CharSet.Unicode, SetLastError=true, EntryPoint="PAL_GetPALDirectoryW")]
        internal static extern int GetSystemDirectory(StringBuilder sb, int length);

        [DllImport(OLEAUT32, CharSet=CharSet.Unicode, SetLastError=true, EntryPoint="PAL_FetchConfigurationStringW")]
        internal static extern bool FetchConfigurationString(bool perMachine, String parameterName, StringBuilder parameterValue, int parameterValueLength);

        [DllImport(KERNEL32, SetLastError=true)]
        internal unsafe static extern bool SetFileTime(IntPtr hFile, FILE_TIME* creationTime,
                    FILE_TIME* lastAccessTime, FILE_TIME* lastWriteTime);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern int GetFileSize(IntPtr hFile, out int highSize);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool LockFile(IntPtr handle, long offset, long count);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool UnlockFile(IntPtr handle,long offset,long count);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern IntPtr GetStdHandle(int nStdHandle);  // param is NOT a handle, but it returns one!

        internal static readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);  // WinBase.h
        internal static readonly IntPtr NULL = IntPtr.Zero;

        // Note, these are #defines used to extract handles, and are NOT handles.
        internal const int STD_INPUT_HANDLE = -10;
        internal const int STD_OUTPUT_HANDLE = -11;
        internal const int STD_ERROR_HANDLE = -12;

        // From wincon.h
        internal const int ENABLE_LINE_INPUT  = 0x0002;
        internal const int ENABLE_ECHO_INPUT  = 0x0004;

        // From WinBase.h
        internal const int FILE_TYPE_DISK = 0x0001;
        internal const int FILE_TYPE_CHAR = 0x0002;
        internal const int FILE_TYPE_PIPE = 0x0003;

        // Constants from WinNT.h
        internal const int FILE_ATTRIBUTE_DIRECTORY = 0x10;

        // Error codes from WinError.h
        internal const int ERROR_FILE_NOT_FOUND = 0x2;
        internal const int ERROR_PATH_NOT_FOUND = 0x3;
        internal const int ERROR_ACCESS_DENIED  = 0x5;
        internal const int ERROR_INVALID_HANDLE = 0x6;
        internal const int ERROR_NO_MORE_FILES = 0x12;
        internal const int ERROR_NOT_READY = 0x15;
        internal const int ERROR_SHARING_VIOLATION = 0x20;
        internal const int ERROR_FILE_EXISTS = 0x50;
        internal const int ERROR_INVALID_PARAMETER = 0x57;
        internal const int ERROR_CALL_NOT_IMPLEMENTED = 0x78;
        internal const int ERROR_FILENAME_EXCED_RANGE = 0xCE;  // filename too long.
        internal const int ERROR_DLL_INIT_FAILED = 0x45A;

        // For the registry class
        internal const int ERROR_MORE_DATA = 234;

        // Use this to translate error codes like the above into HRESULTs like
        // 0x80070006 for ERROR_INVALID_HANDLE
        internal static int MakeHRFromErrorCode(int errorCode)
        {
            BCLDebug.Assert((0xFFFF0000 & errorCode) == 0, "This is an HRESULT, not an error code!");
            return unchecked(((int)0x80070000) | errorCode);
        }

        // Win32 Structs in N/Direct style
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto), Serializable]
        internal class WIN32_FIND_DATA {
            internal int  dwFileAttributes = 0;
            // ftCreationTime was a by-value FILETIME structure
            internal int  ftCreationTime_dwLowDateTime = 0 ;
            internal int  ftCreationTime_dwHighDateTime = 0;
            // ftLastAccessTime was a by-value FILETIME structure
            internal int  ftLastAccessTime_dwLowDateTime = 0;
            internal int  ftLastAccessTime_dwHighDateTime = 0;
            // ftLastWriteTime was a by-value FILETIME structure
            internal int  ftLastWriteTime_dwLowDateTime = 0;
            internal int  ftLastWriteTime_dwHighDateTime = 0;
            internal int  nFileSizeHigh = 0;
            internal int  nFileSizeLow = 0;
            internal int  dwReserved0 = 0;
            internal int  dwReserved1 = 0;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
            internal String   cFileName = null;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst=14)]
            internal String   cAlternateFileName = null;
        }

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern bool CopyFile(
                    String src, String dst, bool failIfExists);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern bool CreateDirectory(
                    String path, int lpSecurityAttributes);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern bool DeleteFile(String path);

        [DllImport(KERNEL32, SetLastError=true)]
        internal static extern bool FindClose(IntPtr hndFindFile);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern IntPtr FindFirstFile(
                    String pFileName,
                    [In, Out]
                    WIN32_FIND_DATA pFindFileData);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern bool FindNextFile(
                    IntPtr hndFindFile,
                    [In, Out, MarshalAs(UnmanagedType.LPStruct)]
                    WIN32_FIND_DATA lpFindFileData);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern int GetCurrentDirectory(
                  int nBufferLength,
                  StringBuilder lpBuffer);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern int GetFileAttributes(String name);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern bool GetFileAttributesEx(String name, int fileInfoLevel, ref WIN32_FILE_ATTRIBUTE_DATA lpFileInformation);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern bool SetFileAttributes(String name, int attr);

#if !PLATFORM_UNIX
        [DllImport("kernel32.dll", SetLastError=true)]
        internal static extern int GetLogicalDrives();
#endif // !PLATFORM_UNIX

        [DllImport(KERNEL32, CharSet=CharSet.Auto, SetLastError=true)]
        internal static extern uint GetTempFileName(String tmpPath, String prefix, uint uniqueIdOrZero, StringBuilder tmpFileName);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern bool MoveFile(String src, String dst);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern bool RemoveDirectory(String path);

        [DllImport(KERNEL32, SetLastError=true, CharSet=CharSet.Auto)]
        internal static extern bool SetCurrentDirectory(String path);

        [DllImport(KERNEL32, SetLastError=false)]
        internal static extern int SetErrorMode(int newMode);

        internal const int LCID_SUPPORTED = 0x00000002;  // supported locale ids

        [DllImport(KERNEL32)]
        internal static extern int /*LCID*/ GetUserDefaultLCID();

        [DllImport(KERNEL32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        internal static extern int GetModuleFileName(IntPtr hModule, StringBuilder buffer, int length);



        // managed cryptography wrapper around the PALRT cryptography api
        internal const int PAL_HCRYPTPROV = 123;

        internal const int CALG_MD2         = ((4 << 13) | 1);
        internal const int CALG_MD4         = ((4 << 13) | 2);
        internal const int CALG_MD5         = ((4 << 13) | 3);
        internal const int CALG_SHA         = ((4 << 13) | 4);
        internal const int CALG_SHA1        = ((4 << 13) | 4);
        internal const int CALG_MAC         = ((4 << 13) | 5);
        internal const int CALG_SSL3_SHAMD5 = ((4 << 13) | 8);
        internal const int CALG_HMAC        = ((4 << 13) | 9);

        internal const int HP_ALGID         = 0x0001;
        internal const int HP_HASHVAL       = 0x0002;
        internal const int HP_HASHSIZE      = 0x0004;

        [DllImport(OLEAUT32, CharSet=CharSet.Unicode, EntryPoint="CryptAcquireContextW")]
        internal extern static bool CryptAcquireContext(out IntPtr hProv,
                           [MarshalAs(UnmanagedType.LPWStr)] string container,
                           [MarshalAs(UnmanagedType.LPWStr)] string provider,
                           int provType,
                           int flags);

        [DllImport(OLEAUT32, SetLastError=true)]
        internal extern static bool CryptReleaseContext( IntPtr hProv, int flags);

        [DllImport(OLEAUT32, SetLastError=true)]
        internal extern static bool CryptCreateHash(IntPtr hProv, int Algid, IntPtr hKey, int flags, out IntPtr hHash);

        [DllImport(OLEAUT32, SetLastError=true)]
        internal extern static bool CryptDestroyHash(IntPtr hHash);

        [DllImport(OLEAUT32, SetLastError=true)]
        internal extern static bool CryptHashData(IntPtr hHash,
                           [In, MarshalAs(UnmanagedType.LPArray)] byte[] data,
                           int length,
                           int flags);

        [DllImport(OLEAUT32, SetLastError=true)]
        internal extern static bool CryptGetHashParam(IntPtr hHash,
                           int param,
                           [Out, MarshalAs(UnmanagedType.LPArray)] byte[] digest,
                           ref int length,
                           int flags);

        [DllImport(OLEAUT32, SetLastError=true)]
        internal extern static bool CryptGetHashParam(IntPtr hHash,
                           int param,
                           out int data,
                           ref int length,
                           int flags);

        [DllImport(KERNEL32, EntryPoint="PAL_Random")]
        internal extern static bool Random(bool bStrong,
                           [Out, MarshalAs(UnmanagedType.LPArray)] byte[] buffer, int length);
    }
}
