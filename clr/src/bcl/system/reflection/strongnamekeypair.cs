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
** File:    StrongNameKeyPair.cs
**
**                                            
**
** Purpose: Encapsulate access to a public/private key pair
**          used to sign strong name assemblies.
**
** Date:    Feb 23, 2000
**
===========================================================*/
namespace System.Reflection
{

	using System;
	using System.IO;
	using System.Security.Permissions;
	using System.Runtime.CompilerServices;

    /// <include file='doc\StrongNameKeyPair.uex' path='docs/doc[@for="StrongNameKeyPair"]/*' />
	[Serializable()]
    public class StrongNameKeyPair
    {
        private bool    _keyPairExported;
        private byte[]  _keyPairArray;
        private String  _keyPairContainer;
        private byte[]  _publicKey;

        // Build key pair from file.
        /// <include file='doc\StrongNameKeyPair.uex' path='docs/doc[@for="StrongNameKeyPair.StrongNameKeyPair"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public StrongNameKeyPair(FileStream keyPairFile)
        {
            if (keyPairFile == null)
                throw new ArgumentNullException("keyPairFile");

            int length = (int)keyPairFile.Length;
            _keyPairArray = new byte[length];
            keyPairFile.Read(_keyPairArray, 0, length);

            _keyPairExported = true;
        }

        // Build key pair from byte array in memory.
        /// <include file='doc\StrongNameKeyPair.uex' path='docs/doc[@for="StrongNameKeyPair.StrongNameKeyPair1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public StrongNameKeyPair(byte[] keyPairArray)
        {
            if (keyPairArray == null)
                throw new ArgumentNullException("keyPairArray");

            _keyPairArray = new byte[keyPairArray.Length];
            Array.Copy(keyPairArray, _keyPairArray, keyPairArray.Length);

            _keyPairExported = true;
        }

        // Reference key pair in named key container.
        /// <include file='doc\StrongNameKeyPair.uex' path='docs/doc[@for="StrongNameKeyPair.StrongNameKeyPair2"]/*' />
        [SecurityPermissionAttribute(SecurityAction.Demand, Flags=SecurityPermissionFlag.UnmanagedCode)]
        public StrongNameKeyPair(String keyPairContainer)
        {
            if (keyPairContainer == null)
                throw new ArgumentNullException("keyPairContainer");

            _keyPairContainer = keyPairContainer;

            _keyPairExported = false;
        }

        // Get the public portion of the key pair.
        /// <include file='doc\StrongNameKeyPair.uex' path='docs/doc[@for="StrongNameKeyPair.PublicKey"]/*' />
        public byte[] PublicKey
        {
            get
            {
                if (_publicKey == null)
                    _publicKey = nGetPublicKey(_keyPairExported, _keyPairArray, _keyPairContainer);

                return _publicKey;
            }
        }

        // Internal routine used to retrieve key pair info from unmanaged code.
        private bool GetKeyPair(out Object arrayOrContainer)
        {
            arrayOrContainer = _keyPairExported ? (Object)_keyPairArray : (Object)_keyPairContainer;
            return _keyPairExported;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern byte[] nGetPublicKey(bool exported, byte[] array, String container);
    }
}
