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
** Class:  FileSystemInfo	
**
**                                                        
**
** Purpose: 
**
** Date:  April 7, 2000
**
===========================================================*/

using System;
using System.Collections;
using System.Security;
using System.Security.Permissions;
using Microsoft.Win32;
using System.Text;
using System.Runtime.InteropServices;

namespace System.IO {
    /// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo"]/*' />
	[Serializable]
    [FileIOPermissionAttribute(SecurityAction.InheritanceDemand,Unrestricted=true)]
    public abstract class FileSystemInfo : MarshalByRefObject {
		internal Win32Native.WIN32_FILE_ATTRIBUTE_DATA _data; // Cache the file information
		internal int _dataInitialised = -1; // We use this field in conjunction with the Refresh methods, if we succeed
										   // we store a zero, on failure we store the HResult in it so that we can
										   // give back a generic error back.

		private const int ERROR_INVALID_PARAMETER = 87;
		internal const int ERROR_ACCESS_DENIED = 0x5;
	    		
        /// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.FullPath"]/*' />
        protected String FullPath;  // fully qualified path of the directory
		/// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.OriginalPath"]/*' />
		protected String OriginalPath; // path passed in by the user
		   
		// Full path of the direcory/file
        /// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.FullName"]/*' />
        public virtual String FullName {
            get 
			{
				String demandDir;
				if (this is DirectoryInfo) {
					if (FullPath.EndsWith( @"\" ))
						demandDir = FullPath + @".";
					else
						demandDir = FullPath + @"\.";
				}
				else
					demandDir = FullPath;
				new FileIOPermission(FileIOPermissionAccess.PathDiscovery, demandDir).Demand();
				return FullPath;
			}
        }

		/// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.Extension"]/*' />
		public String Extension 
		{
			get
			{
				// GetFullPathInternal would have already stripped out the terminating "." if present.
			   int length = FullPath.Length;
				for (int i = length; --i >= 0;) {
					char ch = FullPath[i];
					if (ch == '.')
						return FullPath.Substring(i, length - i);
					if (ch == Path.DirectorySeparatorChar || ch == Path.AltDirectorySeparatorChar || ch == Path.VolumeSeparatorChar)
						break;
				}
				return String.Empty;
			}
		}

		// For files name of the file is returned, for directories the last directory in hierarchy is returned if possible,
		// otherwise the fully qualified name s returned
        /// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.Name"]/*' />
        public abstract String Name {
            get;
        }
        
		// Whether a file/directory exists
	    /// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.Exists"]/*' />
	    public abstract bool Exists
		{
			get;
		}

		// Delete a file/directory
	   	/// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.Delete"]/*' />
	   	public abstract void Delete();

	   /// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.CreationTime"]/*' />
       public DateTime CreationTime {
            get {
				if (_dataInitialised == -1) {
					_data = new Win32Native.WIN32_FILE_ATTRIBUTE_DATA();
					Refresh();
				}

				if (_dataInitialised != 0) // Refresh was unable to initialise the data
					__Error.WinIOError(_dataInitialised, OriginalPath);
				
				long fileTime = ((long)_data.ftCreationTimeHigh << 32) | _data.ftCreationTimeLow;
    			return DateTime.FromFileTime(fileTime);
				
            }
		
			set {
				if (this is DirectoryInfo)
					Directory.SetCreationTime(FullPath,value);
				else
					File.SetCreationTime(FullPath,value);
				_dataInitialised = -1;
			}
		}

        /// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.LastAccessTime"]/*' />
        public DateTime LastAccessTime {
            get {
				if (_dataInitialised == -1) {
					_data = new Win32Native.WIN32_FILE_ATTRIBUTE_DATA();
					Refresh();
				}

				if (_dataInitialised != 0) // Refresh was unable to initialise the data
					__Error.WinIOError(_dataInitialised, OriginalPath);
					
				long fileTime = ((long)_data.ftLastAccessTimeHigh << 32) | _data.ftLastAccessTimeLow;
    			return DateTime.FromFileTime(fileTime);
	
            }
			set {
				if (this is DirectoryInfo)
					Directory.SetLastAccessTime(FullPath,value);
				else
					File.SetLastAccessTime(FullPath,value);
				_dataInitialised = -1;
			}
        }

        /// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.LastWriteTime"]/*' />
        public DateTime LastWriteTime {
            get {
				if (_dataInitialised == -1) {
					_data = new Win32Native.WIN32_FILE_ATTRIBUTE_DATA();
					Refresh();
				}

				if (_dataInitialised != 0) // Refresh was unable to initialise the data
					__Error.WinIOError(_dataInitialised, OriginalPath);
		
			
			    long fileTime = ((long)_data.ftLastWriteTimeHigh << 32) | _data.ftLastWriteTimeLow;
				return DateTime.FromFileTime(fileTime);
			}
			set {
				if (this is DirectoryInfo)
					Directory.SetLastWriteTime(FullPath,value);
				else
					File.SetLastWriteTime(FullPath,value);
				_dataInitialised = -1;
			}
        }

		/// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.Refresh"]/*' />
		public void Refresh()
		{
			_dataInitialised = File.FillAttributeInfo(FullPath,ref _data,false);
		}

		/// <include file='doc\FileSystemInfo.uex' path='docs/doc[@for="FileSystemInfo.Attributes"]/*' />
		public FileAttributes Attributes {
            get {
                if (_dataInitialised == -1) {
					_data = new Win32Native.WIN32_FILE_ATTRIBUTE_DATA();
					Refresh(); // Call refresh to intialise the data
				}

				if (_dataInitialised != 0) // Refresh was unable to initialise the data
					__Error.WinIOError(_dataInitialised, OriginalPath);

				return (FileAttributes) _data.fileAttributes;
            }
			set {
                new FileIOPermission(FileIOPermissionAccess.Write, FullPath).Demand();
                bool r = Win32Native.SetFileAttributes(FullPath, (int) value);
                if (!r) {
                    int hr = Marshal.GetLastWin32Error();
                    if (hr==ERROR_INVALID_PARAMETER || hr == ERROR_ACCESS_DENIED)
                        throw new ArgumentException(Environment.GetResourceString("Arg_InvalidFileAttrs"));
				    __Error.WinIOError(hr, OriginalPath);
                }
				_dataInitialised = -1;
            }
        }


	}
       
}
