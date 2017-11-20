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
** Class: SharedStatics
**
**                                                
**
** Purpose: Container for statics that are shared across AppDomains.
**
** Date: May 9, 2000
**
=============================================================================*/

namespace System {

    using System.Threading;
    using System.Runtime.Remoting;
    using System.Security;
    using System.Security.Util;

    internal sealed class SharedStatics
    {
        // this is declared static but is actually forced to be the same object 
        // for each AppDomain at AppDomain create time.
        internal static SharedStatics _sharedStatics;
        
        // when we create the single object we can construct anything we will need
        // here. If not too many, then just create them all in the constructor, otherwise
        // can have the property check & create. Need to be aware of threading issues 
        // when do so though.
        SharedStatics() {
            _Remoting_Identity_IDGuid = null;
            _Remoting_Identity_IDSeqNum = 0x40; // Reserve initial numbers for well known objects.
        }

        private String _Remoting_Identity_IDGuid;
        public static String Remoting_Identity_IDGuid 
        { 
            get 
            {
                if (_sharedStatics._Remoting_Identity_IDGuid == null)
                {
                    lock (_sharedStatics)
                    {
                        if (_sharedStatics._Remoting_Identity_IDGuid == null)
                        {
                            _sharedStatics._Remoting_Identity_IDGuid = Guid.NewGuid().ToString().Replace('-', '_');
                        }
                    }
                }

                BCLDebug.Assert(_sharedStatics._Remoting_Identity_IDGuid != null,
                                "_sharedStatics._Remoting_Identity_IDGuid != null");
                return _sharedStatics._Remoting_Identity_IDGuid;
            } 
        }

        private int _Remoting_Identity_IDSeqNum;
        public static int Remoting_Identity_GetNextSeqNum()
        {
            return Interlocked.Increment(ref _sharedStatics._Remoting_Identity_IDSeqNum);
        }

        private PermissionTokenFactory _Security_PermissionTokenFactory;
        public static PermissionTokenFactory Security_PermissionTokenFactory
        {
            get
            {
                if (_sharedStatics._Security_PermissionTokenFactory == null)
                {
                    lock (_sharedStatics)
                    {
                        if (_sharedStatics._Security_PermissionTokenFactory == null)
                        {
                            _sharedStatics._Security_PermissionTokenFactory =
                                new PermissionTokenFactory( 16 );
                        }
                    }
                }
                return _sharedStatics._Security_PermissionTokenFactory;
            }
        }


        private static ConfigId m_currentConfigId;
        public static ConfigId GetNextConfigId()
        {
            ConfigId id;

            lock (_sharedStatics)
            {
                if (m_currentConfigId == 0)
                    m_currentConfigId = ConfigId.Reserved;
                id = m_currentConfigId++;
            }
            
            return id;
        }

        //
        // This is just designed to prevent compiler warnings.
        // This field is used from native, but we need to prevent the compiler warnings.
        //
#if _DEBUG
        private void DontTouchThis() {
            _sharedStatics = null;
        }
#endif
    }

}
