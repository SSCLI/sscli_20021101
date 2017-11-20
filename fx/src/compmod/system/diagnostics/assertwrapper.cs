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
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32;
using System.Security.Permissions;
using System.Security;

namespace System.Diagnostics {
    internal class AssertWrapper {

        
        public static void ShowAssert(string stackTrace, StackFrame frame, string message, string detailMessage) {
            ShowMessageBoxAssert(stackTrace, message, detailMessage);                                  
        }


        private static void DebuggerLaunchFailedMessageBox() {
            //Nothing needs to be done here because the debugger already 
            //brings up a dialog.
            /*
            int flags = 0x00000000 | 0x00000010; // OK | IconHand
            NativeMethods.MessageBox(0, SR.GetString(SR.DebugLaunchFailedSR.), GetString("DebugLaunchFailedTitle"), flags);        
            */
            
            return;
        }

        private static void ShowMessageBoxAssert(string stackTrace, string message, string detailMessage) {
            string fullMessage = message + "\r\n" + detailMessage + "\r\n" + stackTrace;


            int flags = 0x00000002 /*AbortRetryIgnore*/ | 0x00000200 /*DefaultButton3*/ | 0x00000010 /*IconHand*/;
            int rval = SafeNativeMethods.MessageBox(IntPtr.Zero, fullMessage, SR.GetString(SR.DebugAssertTitle), flags);
            switch (rval) {
                case 3: // abort
                    new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Assert();
                    try {
                        Environment.Exit(1);
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();    
                    }
                    break;
                case 4: // retry
                    if (!System.Diagnostics.Debugger.IsAttached) {
                        bool succeeded = System.Diagnostics.Debugger.Launch();
                        if (!succeeded)
                            DebuggerLaunchFailedMessageBox();
                    }
                    System.Diagnostics.Debugger.Break();
                    break;
            }
        }


    }
}
