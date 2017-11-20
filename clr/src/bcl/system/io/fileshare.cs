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
** Enum:   FileShare
**
**                                                    
**
** Purpose: Enum describing how to share files with other 
** processes - ie, whether two processes can simultaneously
** read from the same file.
**
** Date:  February 18, 2000
**
===========================================================*/

using System;

namespace System.IO {
    // Contains constants for controlling file sharing options while
    // opening files.  You can specify what access other processes trying
    // to open the same file concurrently can have.
    // 
    /// <include file='doc\FileShare.uex' path='docs/doc[@for="FileShare"]/*' />
    [Serializable, Flags]
    public enum FileShare
    {
        // No sharing. Any request to open the file (by this process or another
        // process) will fail until the file is closed.
        /// <include file='doc\FileShare.uex' path='docs/doc[@for="FileShare.None"]/*' />
        None = 0,
    
        // Allows subsequent opening of the file for reading. If this flag is not
        // specified, any request to open the file for reading (by this process or
        // another process) will fail until the file is closed.
        /// <include file='doc\FileShare.uex' path='docs/doc[@for="FileShare.Read"]/*' />
        Read = 1,
    
        // Allows subsequent opening of the file for writing. If this flag is not
        // specified, any request to open the file for writing (by this process or
        // another process) will fail until the file is closed.
        /// <include file='doc\FileShare.uex' path='docs/doc[@for="FileShare.Write"]/*' />
        Write = 2,
    
        // Allows subsequent opening of the file for writing or reading. If this flag
        // is not specified, any request to open the file for writing or reading (by
        // this process or another process) will fail until the file is closed.
        /// <include file='doc\FileShare.uex' path='docs/doc[@for="FileShare.ReadWrite"]/*' />
        ReadWrite = 3,


        // Whether the file handle should be inheritable by child processes.
        // Note this is not directly supported like this by Win32.
        /// <include file='doc\FileShare.uex' path='docs/doc[@for="FileShare.Inheritable"]/*' />
        Inheritable = 0x10,
    }
}
