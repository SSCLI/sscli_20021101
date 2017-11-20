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
/*=============================================================================
**
** Class: Marshal
**
**                                                  
**
** Purpose: This class contains methods that are mainly used to marshal 
**          between unmanaged and managed types.
**
** Date: January 31, 2000
**
=============================================================================*/

namespace System.Runtime.InteropServices {
    
    using System;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using System.Threading;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Activation;
    using System.Runtime.CompilerServices;
    using System.Runtime.Remoting.Proxies;
    using System.Globalization;
    using Win32Native = Microsoft.Win32.Win32Native;

    //========================================================================
    // All public methods, including PInvoke, are protected with linkchecks.  
    // Remove the default demands for all PInvoke methods with this global 
    // declaration on the class.
    //========================================================================

    /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal"]/*' />
    [SuppressUnmanagedCodeSecurityAttribute()]
    public sealed class Marshal
    { 
        //====================================================================
        // Defines used inside the Marshal class.
        //====================================================================
        private const int LMEM_FIXED = 0;

        // Win32 has the concept of Atoms, where a pointer can either be a pointer
        // or an int.  If it's less than 64K, this is guaranteed to NOT be a 
        // pointer since the bottom 64K bytes are reserved in a process' page table.
        // We should be careful about deallocating this stuff.  Extracted to
        // a function to avoid C# problems with lack of support for IntPtr.
        // We have 2 of these methods for slightly different semantics for NULL.
        private static bool IsWin32Atom(IntPtr ptr)
        {
	    return false;
        }

        private static bool IsNotWin32Atom(IntPtr ptr)
        {
	return false;
        }

        //====================================================================
        // The default character size for the system.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.SystemDefaultCharSize"]/*' />
        public static readonly int SystemDefaultCharSize;

        //====================================================================
        // The max DBCS character size for the system.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.SystemMaxDBCSCharSize"]/*' />
        public static readonly int SystemMaxDBCSCharSize;


        //====================================================================
        // The name, title and description of the assembly that will contain 
        // the dynamically generated interop types. 
        //====================================================================
        private const String s_strConvertedTypeInfoAssemblyName   = "InteropDynamicTypes";
        private const String s_strConvertedTypeInfoAssemblyTitle  = "Interop Dynamic Types";
        private const String s_strConvertedTypeInfoAssemblyDesc   = "Type dynamically generated from ITypeInfo's";
        private const String s_strConvertedTypeInfoNameSpace      = "InteropDynamicTypes";


        //====================================================================
        // Class constructor.
        //====================================================================
        static Marshal() 
        {
            SystemDefaultCharSize = 3 - Win32Native.lstrlen(new sbyte [] {0x41, 0x41, 0, 0});
            SystemMaxDBCSCharSize = GetSystemMaxDBCSCharSize();
        }


        //====================================================================
        // Prevent instantiation
        //====================================================================
        private Marshal() {}  
        

        //====================================================================
        // Helper method to retrieve the system's maximum DBCS character size.
        //====================================================================
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern int GetSystemMaxDBCSCharSize();

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringAnsi"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String PtrToStringAnsi(IntPtr ptr)
        {
            if (Win32Native.NULL == ptr) {
                return null;
            }
            else if (IsWin32Atom(ptr)) {
                return null;
            }
            else {
                int nb = Win32Native.lstrlenA(ptr);
                StringBuilder sb = new StringBuilder(nb);
                Win32Native.CopyMemoryAnsi(sb, ptr, new IntPtr(1+nb));
                return sb.ToString();
            }
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringAnsi1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern String PtrToStringAnsi(IntPtr ptr, int len);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringUni"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern String PtrToStringUni(IntPtr ptr, int len);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringAuto"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String PtrToStringAuto(IntPtr ptr, int len)
        {
            return (SystemDefaultCharSize == 1) ? PtrToStringAnsi(ptr, len) : PtrToStringUni(ptr, len);
        }    
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringUni1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String PtrToStringUni(IntPtr ptr)
        {
            if (Win32Native.NULL == ptr) {
                return null;
            }
            else if (IsWin32Atom(ptr)) {
                return null;
            } 
            else {
                int nc = Win32Native.lstrlenW(ptr);
                StringBuilder sb = new StringBuilder(nc);
                Win32Native.CopyMemoryUni(sb, ptr, new IntPtr(2*(1+nc)));
                return sb.ToString();
            }
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStringAuto1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static String PtrToStringAuto(IntPtr ptr)
        {
            if (Win32Native.NULL == ptr) {
                return null;
            } 
            else if (IsWin32Atom(ptr)) {
                return null;
            } 
            else {
                int nc = Win32Native.lstrlen(ptr);
                StringBuilder sb = new StringBuilder(nc);
                Win32Native.lstrcpy(sb, ptr);
                return sb.ToString();
            }
        }            

        //====================================================================
        // SizeOf()
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.SizeOf"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int SizeOf(Object structure);
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.SizeOf1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int SizeOf(Type t);    
    

        //====================================================================
        // OffsetOf()
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.OffsetOf"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr OffsetOf(Type t, String fieldName)
        {
            if (t == null)
                throw new ArgumentNullException("t");
            
            FieldInfo f = t.GetField(fieldName);
            if (f == null)
                throw new ArgumentException(Environment.GetResourceString("Argument_OffsetOfFieldNotFound", t.FullName), "fieldName");
                
            return OffsetOfHelper(f);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern IntPtr OffsetOfHelper(FieldInfo f);
        

        //====================================================================
        // UnsafeAddrOfPinnedArrayElement()
        //
        // IMPORTANT NOTICE: This method does not do any verification on the
        // array. It must be used with EXTREME CAUTION since passing in 
        // an array that is not pinned or in the fixed heap can cause 
        // unexpected results !
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.UnsafeAddrOfPinnedArrayElement"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern IntPtr UnsafeAddrOfPinnedArrayElement(Array arr, int index);
        

        //====================================================================
        // Copy blocks from COM+ arrays to native memory.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(int[]     source, int startIndex, IntPtr destination, int length);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy3"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(char[]    source, int startIndex, IntPtr destination, int length);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy5"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(short[]   source, int startIndex, IntPtr destination, int length);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy7"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(long[]    source, int startIndex, IntPtr destination, int length);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy9"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(float[]   source, int startIndex, IntPtr destination, int length);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy11"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(double[]  source, int startIndex, IntPtr destination, int length);
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy12"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(byte[] source, int startIndex, IntPtr destination, int length)
        {
            CopyBytesToNative(source, startIndex, destination, length);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void CopyBytesToNative(byte[] source, int startIndex, IntPtr destination, int length);
    

        //====================================================================
        // Copy blocks from native memory to COM+ arrays
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy14"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(IntPtr source, int[]     destination, int startIndex, int length);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy16"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(IntPtr source, char[]    destination, int startIndex, int length);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy18"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(IntPtr source, short[]   destination, int startIndex, int length);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy20"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(IntPtr source, long[]    destination, int startIndex, int length);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy21"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(IntPtr source, float[]   destination, int startIndex, int length);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy23"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall),
         SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Copy(IntPtr source, double[]  destination, int startIndex, int length);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Copy24"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void Copy(IntPtr source, byte[] destination, int startIndex, int length)
        {
            CopyBytesToManaged((int)source, destination, startIndex, length);
        }
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void CopyBytesToManaged(int source, byte[]   destination, int startIndex, int length);
        

        //====================================================================
        // Read from memory
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadByte"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_RU1")]
        public static extern byte ReadByte([MarshalAs(UnmanagedType.AsAny), In] Object ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadByte1"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_RU1")]
        public static extern byte ReadByte(IntPtr ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadByte2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static byte ReadByte(IntPtr ptr)
        {
            return ReadByte(ptr,0);
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt16"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_RI2")]
        public static extern short ReadInt16([MarshalAs(UnmanagedType.AsAny),In] Object ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt161"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_RI2")]
        public static extern short ReadInt16(IntPtr ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt162"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static short ReadInt16(IntPtr ptr)
        {
            return ReadInt16(ptr, 0);
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt32"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_RI4")]
        public static extern int ReadInt32([MarshalAs(UnmanagedType.AsAny),In] Object ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt321"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_RI4")]
        public static extern int ReadInt32(IntPtr ptr, int ofs);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt322"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static int ReadInt32(IntPtr ptr)
        {
            return ReadInt32(ptr,0);
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadIntPtr"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr ReadIntPtr([MarshalAs(UnmanagedType.AsAny),In] Object ptr, int ofs)
        {
            #if WIN32
                return (IntPtr) ReadInt32(ptr, ofs);
            #else
                return (IntPtr) ReadInt64(ptr, ofs);
            #endif
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadIntPtr1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr ReadIntPtr(IntPtr ptr, int ofs)
        {
            #if WIN32
                return (IntPtr) ReadInt32(ptr, ofs);
            #else
                return (IntPtr) ReadInt64(ptr, ofs);
            #endif
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadIntPtr2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr ReadIntPtr(IntPtr ptr)
        {
            #if WIN32
                return (IntPtr) ReadInt32(ptr, 0);
            #else
                return (IntPtr) ReadInt64(ptr, 0);
            #endif
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt64"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_RI8")]
        public static extern long ReadInt64([MarshalAs(UnmanagedType.AsAny),In] Object ptr, int ofs);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt641"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_RI8")]
        public static extern long ReadInt64(IntPtr ptr, int ofs);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ReadInt642"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static long ReadInt64(IntPtr ptr)
        {
            return ReadInt64(ptr,0);
        }
    
    
        //====================================================================
        // Write to memory
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteByte"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_WU1")]
        public static extern void WriteByte(IntPtr ptr, int ofs, byte val);

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteByte1"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_WU1")]
        public static extern void WriteByte([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, byte val);    

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteByte2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteByte(IntPtr ptr, byte val)
        {
            WriteByte(ptr, 0, val);
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt16"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_WI2")]
        public static extern void WriteInt16(IntPtr ptr, int ofs, short val);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt161"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_WI2")]
        public static extern void WriteInt16([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, short val);
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt162"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteInt16(IntPtr ptr, short val)
        {
            WriteInt16(ptr, 0, val);
        }    
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt163"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_WI2")]
        public static extern void WriteInt16(IntPtr ptr, int ofs, char val);
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt164"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_WI2")]
        public static extern void WriteInt16([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, char val);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt165"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteInt16(IntPtr ptr, char val)
        {
            WriteInt16(ptr, 0, val);
        }
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt32"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_WI4")]
        public static extern void WriteInt32(IntPtr ptr, int ofs, int val);
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt321"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_WI4")]
        public static extern void WriteInt32([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, int val);
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt322"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteInt32(IntPtr ptr, int val)
        {
            WriteInt32(ptr,0,val);
        }    
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteIntPtr"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteIntPtr(IntPtr ptr, int ofs, IntPtr val)
        {
            #if WIN32
                WriteInt32(ptr, ofs, (int)val);
            #else
                WriteInt64(ptr, ofs, (long)val);
            #endif
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteIntPtr1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteIntPtr([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, IntPtr val)
        {
            #if WIN32
                WriteInt32(ptr, ofs, (int)val);
            #else
                WriteInt64(ptr, ofs, (long)val);
            #endif
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteIntPtr2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteIntPtr(IntPtr ptr, IntPtr val)
        {
            #if WIN32
                WriteInt32(ptr, 0, (int)val);
            #else
                WriteInt64(ptr, 0, (long)val);
            #endif
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt64"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_WI8")]
        public static extern void WriteInt64(IntPtr ptr, int ofs, long val);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt641"]/*' />
        [DllImport(Win32Native.SHIM, EntryPoint="ND_WI8")]        
        public static extern void WriteInt64([MarshalAs(UnmanagedType.AsAny),In,Out] Object ptr, int ofs, long val);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.WriteInt642"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void WriteInt64(IntPtr ptr, long val)
        {
            WriteInt64(ptr, 0, val);
        }
    
    
        //====================================================================
        // GetLastWin32Error
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetLastWin32Error"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int GetLastWin32Error();
    

        //====================================================================
        // GetHRForLastWin32Error
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetHRForLastWin32Error"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static int GetHRForLastWin32Error()
        {
            return GetLastWin32Error() | unchecked((int)0x80070000);
        }


        //====================================================================
        // Prelink
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.Prelink"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void Prelink(MethodInfo m);
    
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PrelinkAll"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void PrelinkAll(Type c)
        {
            if (c == null)
                throw new ArgumentNullException("c");

            MethodInfo[] mi = c.GetMethods();
            if (mi != null) 
            {
                for (int i = 0; i < mi.Length; i++) 
                {
                    Prelink(mi[i]);
                }
            }
        }
    
        //====================================================================
        // NumParamBytes
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.NumParamBytes"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int NumParamBytes(MethodInfo m);
    
    
        //====================================================================
        // Win32 Exception stuff
        // These are mostly interesting for Structured exception handling,
        // but need to be exposed for all exceptions (not just SEHException).
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetExceptionPointers"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern /* struct _EXCEPTION_POINTERS* */ IntPtr GetExceptionPointers();

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetExceptionCode"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int GetExceptionCode();


        //====================================================================
        // Marshals data from a structure class to a native memory block.
        // If the structure contains pointers to allocated blocks and
        // "fDeleteOld" is true, this routine will call DestroyStructure() first. 
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.StructureToPtr"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void StructureToPtr(Object structure, IntPtr ptr, bool fDeleteOld);
    

        //====================================================================
        // Marshals data from a native memory block to a preallocated structure class.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStructure"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void PtrToStructure(IntPtr ptr, Object structure)
        {
            PtrToStructureHelper(ptr, structure, false);
        }

        
        //====================================================================
        // Creates a new instance of "structuretype" and marshals data from a
        // native memory block to it.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.PtrToStructure1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static Object PtrToStructure(IntPtr ptr, Type structureType)
        {
            if (ptr == Win32Native.NULL) return null;
            Object structure = Activator.CreateInstance(structureType, true);
            PtrToStructureHelper(ptr, structure, true);
            return structure;
        }
    

        //====================================================================
        // Helper function to copy a pointer into a preallocated structure.
        //====================================================================
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void PtrToStructureHelper(IntPtr ptr, Object structure, bool allowValueClasses);


        //====================================================================
        // Freeds all substructures pointed to by the native memory block.
        // "structureclass" is used to provide layout information.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.DestroyStructure"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void DestroyStructure(IntPtr ptr, Type structuretype);
    

        //====================================================================
        // Returns the HInstance for this module.  Returns -1 if the module 
        // doesn't have an HInstance.  In Memory (Dynamic) Modules won't have 
        // an HInstance.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetHINSTANCE"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr GetHINSTANCE(Module m)
        {
            if (m == null)
                throw new ArgumentNullException("m");
            return m.GetHINSTANCE();
        }    


        //====================================================================
        // Converts the HRESULT to a COM+ exception.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ThrowExceptionForHR"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void ThrowExceptionForHR(int errorCode)
        {
            ThrowExceptionForHR(errorCode, Win32Native.NULL);
        }
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.ThrowExceptionForHR1"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern void ThrowExceptionForHR(int errorCode, IntPtr errorInfo);


        //====================================================================
        // Converts the COM+ exception to an HRESULT. This function also sets
        // up an IErrorInfo for the exception.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.GetHRForException"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static extern int GetHRForException(Exception e);


        //====================================================================
        // Memory allocation and dealocation.
        //====================================================================
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.AllocHGlobal"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr AllocHGlobal(IntPtr cb)
        {
            IntPtr pNewMem = Win32Native.LocalAlloc(LMEM_FIXED, cb);
            if (pNewMem == Win32Native.NULL) {
                throw new OutOfMemoryException();
            }
            return pNewMem;
        }

        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.AllocHGlobal1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static IntPtr AllocHGlobal(int cb)
        {
            return AllocHGlobal((IntPtr)cb);
        }
        
        /// <include file='doc\Marshal.uex' path='docs/doc[@for="Marshal.FreeHGlobal"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public static void FreeHGlobal(IntPtr hglobal)
        {
            if (IsNotWin32Atom(hglobal)) {
                if (Win32Native.NULL != Win32Native.LocalFree(hglobal)) {
                    ThrowExceptionForHR(GetHRForLastWin32Error());
                }
            }
        }


    }

}







