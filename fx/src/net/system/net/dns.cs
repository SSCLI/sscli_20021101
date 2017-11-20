//------------------------------------------------------------------------------
// <copyright file="DNS.cs" company="Microsoft">
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


namespace System.Net {
    using System.Text;
    using System.Collections;
    using System.Net.Sockets;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Threading;
    using System.Security;

    /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns"]/*' />
    /// <devdoc>
    ///    <para>Provides simple
    ///       domain name resolution functionality.</para>
    /// </devdoc>

    public sealed class Dns {
        //
        // used by GetHostName() to preallocate a buffer for the call to gethostname.
        //
        private const int HostNameBufferLength = 256;
        private const int DefaultLocalHostTimeOut = 1 * (60 * 1000); // 1 min?
        private static DnsPermission s_DnsPermission = new DnsPermission(PermissionState.Unrestricted);
        //
        //
        private const int MaxHostName = 126;

        // Static bool variable activates static socket initializing method
        // we need this because socket is in a different namespace
        private static bool         s_Initialized = Socket.InitializeSockets();
        private static int          s_LastLocalHostCount ;
        private static IPHostEntry  s_LocalHost ;


        //
        // Constructor is private to prevent instantiation
        //
        private Dns() {
        }


        /*++

        Routine Description:

            Takes a native pointer (expressed as an int) to a hostent structure,
            and converts the information in their to an IPHostEntry class. This
            involves walking through an array of native pointers, and a temporary
            ArrayList object is used in doing this.

        Arguments:

            nativePointer   - Native pointer to hostent structure.



        Return Value:

            An IPHostEntry structure.

        --*/

        private static IPHostEntry NativeToHostEntry(IntPtr nativePointer) {
            //
            // marshal pointer to struct
            //

            hostent Host = (hostent)Marshal.PtrToStructure(nativePointer, typeof(hostent));
            IPHostEntry HostEntry = new IPHostEntry();

            if (Host.h_name != IntPtr.Zero) {
                HostEntry.HostName = Marshal.PtrToStringAnsi(Host.h_name);
                GlobalLog.Print("HostEntry.HostName: " + HostEntry.HostName);
            }

            // decode h_addr_list to ArrayList of IP addresses.
            // The h_addr_list field is really a pointer to an array of pointers
            // to IP addresses. Loop through the array, and while the pointer
            // isn't NULL read the IP address, convert it to an IPAddress class,
            // and add it to the list.

            ArrayList TempList = new ArrayList();
            int IPAddressToAdd;
            string AliasToAdd;
            IntPtr currentArrayElement;

            //
            // get the first pointer in the array
            //
            currentArrayElement = Host.h_addr_list;
            nativePointer = Marshal.ReadIntPtr(currentArrayElement);

            while (nativePointer != IntPtr.Zero) {
                //
                // if it's not null it points to an IPAddress,
                // read it...
                //
                IPAddressToAdd = Marshal.ReadInt32(nativePointer);
#if BIGENDIAN
                // IP addresses from native code are always a byte array
                // converted to int.  We need to convert the address into
                // a uniform integer value.
                IPAddressToAdd = (int)( ((uint)IPAddressToAdd << 24) |
                                        (((uint)IPAddressToAdd & 0x0000FF00) << 8) |
                                        (((uint)IPAddressToAdd >> 8) & 0x0000FF00) |
                                        ((uint)IPAddressToAdd >> 24) );
#endif

                GlobalLog.Print("currentArrayElement: " + currentArrayElement.ToString() + " nativePointer: " + nativePointer.ToString() + " IPAddressToAdd:" + IPAddressToAdd.ToString());

                //
                // ...and add it to the list
                //
                TempList.Add(new IPAddress(IPAddressToAdd));

                //
                // now get the next pointer in the array and start over
                //
                currentArrayElement = IntPtrHelper.Add(currentArrayElement, IntPtr.Size);
                nativePointer = Marshal.ReadIntPtr(currentArrayElement);
            }

            HostEntry.AddressList = new IPAddress[TempList.Count];
            TempList.CopyTo(HostEntry.AddressList, 0);

            //
            // Now do the same thing for the aliases.
            //

            TempList.Clear();

            currentArrayElement = Host.h_aliases;
            nativePointer = Marshal.ReadIntPtr(currentArrayElement);

            while (nativePointer != IntPtr.Zero) {

                GlobalLog.Print("currentArrayElement: " + ((long)currentArrayElement).ToString() + "nativePointer: " + ((long)nativePointer).ToString());

                //
                // if it's not null it points to an Alias,
                // read it...
                //
                AliasToAdd = Marshal.PtrToStringAnsi(nativePointer);

                //
                // ...and add it to the list
                //
                TempList.Add(AliasToAdd);

                //
                // now get the next pointer in the array and start over
                //
                currentArrayElement = IntPtrHelper.Add(currentArrayElement, IntPtr.Size);
                nativePointer = Marshal.ReadIntPtr(currentArrayElement);

            }

            HostEntry.Aliases = new string[TempList.Count];
            TempList.CopyTo(HostEntry.Aliases, 0);

            return HostEntry;

        } // NativeToHostEntry

        /*****************************************************************************
         Function :    gethostbyname

         Abstract:     Queries DNS for hostname address

         Input Parameters: str (String to query)

         Returns: Void
        ******************************************************************************/

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.GetHostByName"]/*' />
        /// <devdoc>
        /// <para>Retrieves the <see cref='System.Net.IPHostEntry'/>
        /// information
        /// corresponding to the DNS name provided in the host
        /// parameter.</para>
        /// </devdoc>
        public static IPHostEntry GetHostByName(string hostName) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            if (hostName == null) {
                throw new ArgumentNullException("hostName");
            }
            if (hostName.Length>MaxHostName) {
                throw new ArgumentOutOfRangeException(SR.GetString(SR.net_toolong, "hostName", MaxHostName.ToString()));
            }

            GlobalLog.Print("Dns.GetHostByName: " + hostName);

            IntPtr nativePointer =
                UnsafeNclNativeMethods.OSSOCK.gethostbyname(
                    hostName);

            if (nativePointer == IntPtr.Zero) {
                //This is for compatiblity with NT4
                SocketException e = new SocketException();
                //This block supresses "unknown error" on NT4 when input is
                //arbitrary IP address. It simulates same result as on Win2K.
                try {
                    IPAddress address = IPAddress.Parse(hostName);
                    IPHostEntry ipHostEntry = new IPHostEntry();
                    ipHostEntry.HostName = string.Copy(hostName);
                    ipHostEntry.Aliases = new string[0];
                    ipHostEntry.AddressList = new IPAddress[] {address};
                    return ipHostEntry;
                }
                catch {
                    //Report original DNS error (not from IPAddress.Parse())
                    throw e;
                }
            }

            return NativeToHostEntry(nativePointer);

        } // GetHostByName



        //
        // Find out if we need to rebuild our LocalHost. If the process
        //  time has been running for a while the machine's IP addresses
        //  could have changed, this causing us to want to requery the info.
        //
        internal static bool IsLocalHostExpired() {
            int counter = Environment.TickCount;
            bool timerExpired = false;
            if (s_LastLocalHostCount > counter) {
                timerExpired = true;
            } else {
                counter -= s_LastLocalHostCount;
                if (counter > DefaultLocalHostTimeOut) {
                    timerExpired = true;
                }
            }
            return timerExpired;
        }
 
        //
        // Returns a list of our local addresses, caches the result of GetLocalHost
        //
        internal static IPHostEntry LocalHost {
            get {
                bool timerExpired = IsLocalHostExpired();
                if (s_LocalHost == null || timerExpired ) {
                    lock (typeof(Dns)) {
                        if (s_LocalHost == null || timerExpired ) {
                            s_LastLocalHostCount = Environment.TickCount;
                            s_LocalHost = GetLocalHost();
                        }
                    }
                }
                return s_LocalHost;
            }
        }
 
        //
        // Returns a list of our local addresses by calling gethostbyname with null.
        //
        private static IPHostEntry GetLocalHost() {
            GlobalLog.Print("Dns.GetLocalHost");

            IntPtr nativePointer =
                UnsafeNclNativeMethods.OSSOCK.gethostbyname(
                    null);

            if (nativePointer == IntPtr.Zero) {
                throw new SocketException();
            }

            return NativeToHostEntry(nativePointer);

        } // GetLocalHost


        /*****************************************************************************
         Function :    gethostbyaddr

         Abstract:     Queries IP address string and tries to match with a host name

         Input Parameters: str (String to query)

         Returns: IPHostEntry
        ******************************************************************************/

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.GetHostByAddress"]/*' />
        /// <devdoc>
        /// <para>Creates an <see cref='System.Net.IPHostEntry'/>
        /// instance from an IP dotted address.</para>
        /// </devdoc>
        public static IPHostEntry GetHostByAddress(string address) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            if (address == null) {
                throw new ArgumentNullException("address");
            }

            GlobalLog.Print("Dns.GetHostByAddress: " + address);

            // convert String based address to bit encoded address stored in an int

            IPAddress ip_addr = IPAddress.Parse(address);

            return GetHostByAddress(ip_addr);

        } // GetHostByAddress

        /*****************************************************************************
         Function :    gethostbyaddr

         Abstract:     Queries IP address and tries to match with a host name

         Input Parameters: address (address to query)

         Returns: IPHostEntry
        ******************************************************************************/

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.GetHostByAddress1"]/*' />
        /// <devdoc>
        /// <para>Creates an <see cref='System.Net.IPHostEntry'/> instance from an <see cref='System.Net.IPAddress'/>
        /// instance.</para>
        /// </devdoc>
        public static IPHostEntry GetHostByAddress(IPAddress address) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            if (address == null) {
                throw new ArgumentNullException("address");
            }

            GlobalLog.Print("Dns.GetHostByAddress: " + address.ToString());

            int addressAsInt = unchecked((int)address.Address);

#if BIGENDIAN
            addressAsInt = (int)( ((uint)addressAsInt << 24) | (((uint)addressAsInt & 0x0000FF00) << 8) |
                (((uint)addressAsInt >> 8) & 0x0000FF00) | ((uint)addressAsInt >> 24) );
#endif

            IntPtr nativePointer =
                UnsafeNclNativeMethods.OSSOCK.gethostbyaddr(
                    ref addressAsInt,
                    Marshal.SizeOf(typeof(int)),
                    ProtocolFamily.InterNetwork);


            if (nativePointer == IntPtr.Zero) {
                throw new SocketException();
            }

            return NativeToHostEntry(nativePointer);

        } // GetHostByAddress

        /*****************************************************************************
         Function :    inet_ntoa

         Abstract:     Numerical IP value to IP address string

         Input Parameters: address

         Returns: String
        ******************************************************************************/

        /*
        /// <summary>
        ///    <para>Creates a string containing the
        ///       DNS name of the host specified in address.</para>
        /// </summary>
        /// <param name='address'>An <see cref='System.Net.IPAddress'/> instance representing the IP (or dotted IP) address of the host.</param>
        /// <returns>
        ///    <para>A string containing the DNS name of the host
        ///       specified in the address.</para>
        /// </returns>
        /// <exception cref='System.ArgumentNullException'><paramref name="address "/>is null.</exception>
        // obsolete, use ToString()
        public static string InetNtoa(IPAddress address) {
            if (address == null) {
                throw new ArgumentNullException("address");
            }

            return address.ToString();
        }
        */

        /*****************************************************************************
         Function :    gethostname

         Abstract:     Queries the hostname from DNS

         Input Parameters:

         Returns: String
        ******************************************************************************/

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.GetHostName"]/*' />
        /// <devdoc>
        ///    <para>Gets the host name of the local machine.</para>
        /// </devdoc>
        // UEUE: note that this method is not threadsafe!!
        public static string GetHostName() {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            GlobalLog.Print("Dns.GetHostName");

            //
            // note that we could cache the result ourselves since you
            // wouldn't expect the hostname of the machine to change during
            // execution, but this might still happen and we would want to
            // react to that change.
            //
            StringBuilder sb = new StringBuilder(HostNameBufferLength);
            int errorCode = UnsafeNclNativeMethods.OSSOCK.gethostname(sb, HostNameBufferLength);

            //
            // if the call failed throw a SocketException()
            //
            if (errorCode != 0) {
                throw new SocketException();
            }
            return sb.ToString();
        } // GetHostName

        /*****************************************************************************
         Function :    resolve

         Abstract:     Converts IP/hostnames to IP numerical address using DNS
                       Additional methods provided for convenience
                       (These methods will resolve strings and hostnames. In case of
                       multiple IP addresses, the address returned is chosen arbitrarily.)

         Input Parameters: host/IP

         Returns: IPAddress
        ******************************************************************************/

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.Resolve"]/*' />
        /// <devdoc>
        /// <para>Creates an <see cref='System.Net.IPAddress'/>
        /// instance from a DNS hostname.</para>
        /// </devdoc>
        // UEUE
        public static IPHostEntry Resolve(string hostName) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();
            //This is a perf optimization, this method call others that will demand and we already did that.
            s_DnsPermission.Assert();
            
            IPHostEntry ipHostEntry = null;
            try {
                if (hostName == null) {
                    throw new ArgumentNullException("hostName");
                }

                GlobalLog.Print("Dns.Resolve: " + hostName);

                //
                // as a minor perf improvement, we'll look at the first character
                // in the hostName and if that's a digit, call GetHostByAddress() first.
                // note that GetHostByAddress() will succeed ONLY if the first character is a digit.
                // hence if this is not the case, below, we will only call GetHostByName()
                // specifically, on Win2k, only the following are valid:
                // "11.22.33.44" - success
                // "0x0B16212C" - success
                // while these will fail or return bogus data (note the prepended spaces):
                // " 11.22.33.44" - bogus data
                // " 0x0B16212C" - bogus data
                // " 0B16212C" - bogus data
                // "0B16212C" - failure
                //
                if (hostName.Length>0 && hostName[0]>='0' && hostName[0]<='9') {
                    try {
                        ipHostEntry = GetHostByAddress(hostName);
                    }
                    catch {
                        //
                        // this can fail for weird hostnames like 3com.com
                        //
                    }
                }
                if (ipHostEntry==null) {
                    ipHostEntry = GetHostByName(hostName);
                }
            }
            finally {
                DnsPermission.RevertAssert();
            }

            return ipHostEntry;
        }


        internal static IPHostEntry InternalResolve(string hostName) {
            GlobalLog.Assert(hostName!=null, "hostName == null", "");
            GlobalLog.Print("Dns.InternalResolve: " + hostName);

            //
            // the differences between this method and the previous public
            // Resolve() are:
            //
            // 1) we bypass DNS permission check
            // 2) we avoid exceptions by duplicating GetHostByAddress and GetHotByName code here
            //

            IntPtr nativePointer = IntPtr.Zero;
            int address=0;

            //To not miss resolution of numeric hostname into FQDN, try FIRST get by address
            if (hostName[0]>='0' && hostName[0]<='9') {
                address = UnsafeNclNativeMethods.OSSOCK.inet_addr(hostName);
                GlobalLog.Print("Dns::InternalResolve() inet_addr() returned address:" + address.ToString());

                if (address != -1 || hostName.Equals(IPAddress.InaddrNoneString)) {
                    nativePointer =
                        UnsafeNclNativeMethods.OSSOCK.gethostbyaddr(
                            ref address,
                            Marshal.SizeOf(typeof(int)),
                            ProtocolFamily.InterNetwork);

                    GlobalLog.Print("Dns::InternalResolve() gethostbyaddr() returned nativePointer:" + nativePointer.ToString());

                    if (nativePointer != IntPtr.Zero) {
                        return NativeToHostEntry(nativePointer);
                    }
                }
                // here we'll get the weird hostnames like 3com.com
            }

            if (hostName.Length<=MaxHostName) {
                //
                // we duplicate the code in GetHostByName() to avoid
                // having to catch the thrown exception
                //
                nativePointer = UnsafeNclNativeMethods.OSSOCK.gethostbyname(hostName);
                GlobalLog.Print("Dns::InternalResolve() gethostbyname() returned nativePointer:" + nativePointer.ToString());
                if (nativePointer == IntPtr.Zero) {
                    GlobalLog.Print("Dns::InternalResolve() gethostbyname() failed:" + (new SocketException()).Message);
                }
            }

            //
            // if gethostbyname fails, do the last attempt
            //
            if (nativePointer == IntPtr.Zero) {
                
                //This is for compatiblity with NT4
                //This block supresses "unknown error" on NT4 when input is
                //arbitrary IP address. It simulates same result as on Win2K.
                if (address == 0) {
                    //we still did not call OSSOCK.inet_addr(), do it now
                    address = UnsafeNclNativeMethods.OSSOCK.inet_addr(hostName);
                    GlobalLog.Print("Dns::InternalResolve() inet_addr() returned address:" + address.ToString());
                }

                if (address != -1 || hostName == IPAddress.InaddrNoneString) {
#if BIGENDIAN
                    // OSSOCK.inet_addr always returns IP address as a byte array converted to int,
                    // thus we need to convert the address into a uniform integer value.
                    address = (int)( ((uint)address << 24) | (((uint)address & 0x0000FF00) << 8) |
                        (((uint)address >> 8) & 0x0000FF00) | ((uint)address >> 24) );
#endif
                    //
                    // the call to inet_addr() succeeded
                    //
                    IPHostEntry ipHostEntry = new IPHostEntry();
                    ipHostEntry.HostName    = hostName;
                    ipHostEntry.Aliases     = new string[0];
                    ipHostEntry.AddressList = new IPAddress[] {new IPAddress(address)};

                    GlobalLog.Print("Dns::InternalResolve() returning address:" + address.ToString());
                    return ipHostEntry;
                }

                GlobalLog.Print("Dns::InternalResolve() returning null");
                // Not a numeric IP string
                return null;
            }

            return NativeToHostEntry(nativePointer);
        }


        //
        // A note on the use of delegates for the following Async APIs.
        // The concern is that a worker thread would be blocked if an
        // exception is thrown by the method that we delegate execution to.
        // We're actually guaranteed that the exception is caught and returned to
        // the user by the subsequent EndInvoke call.
        // This delegate async execution is something that the C# compiler and the EE
        // will manage.                                                                        
        //
        private delegate IPHostEntry GetHostByNameDelegate(string hostName);
        private static GetHostByNameDelegate getHostByName = new GetHostByNameDelegate(Dns.GetHostByName);

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.BeginGetHostByName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static IAsyncResult BeginGetHostByName(string hostName, AsyncCallback requestCallback, object stateObject) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            if (hostName == null) {
                throw new ArgumentNullException("hostName");
            }

            GlobalLog.Print("Dns.BeginGetHostByName: " + hostName);

            return getHostByName.BeginInvoke(hostName, requestCallback, stateObject);

        } // BeginGetHostByName

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.EndGetHostByName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static IPHostEntry EndGetHostByName(IAsyncResult asyncResult) {
            //
            // parameter validation
            //
            if (asyncResult == null) {
                throw new ArgumentNullException("asyncResult");
            }

            GlobalLog.Print("Dns.EndGetHostByName");

            if (!asyncResult.IsCompleted) {
                asyncResult.AsyncWaitHandle.WaitOne();
            }
            return getHostByName.EndInvoke(asyncResult);

        } // EndGetHostByName()


        private delegate IPHostEntry ResolveDelegate(string hostName);
        private static ResolveDelegate resolve = new ResolveDelegate(Dns.Resolve);

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.BeginResolve"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static IAsyncResult BeginResolve(string hostName, AsyncCallback requestCallback, object stateObject) {
            //
            // demand Unrestricted DnsPermission for this call
            //
            s_DnsPermission.Demand();

            if (hostName == null) {
                throw new ArgumentNullException("hostName");
            }

            GlobalLog.Print("Dns.BeginResolve: " + hostName);

            return resolve.BeginInvoke(hostName, requestCallback, stateObject);

        } // BeginResolve

        /// <include file='doc\DNS.uex' path='docs/doc[@for="Dns.EndResolve"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static IPHostEntry EndResolve(IAsyncResult asyncResult) {
            //
            // parameter validation
            //
            if (asyncResult == null) {
                throw new ArgumentNullException("asyncResult");
            }

            GlobalLog.Print("Dns.EndResolve");

            if (!asyncResult.IsCompleted) {
                asyncResult.AsyncWaitHandle.WaitOne();
            }
            return resolve.EndInvoke(asyncResult);

        } // EndResolve()




    }; // class Dns


} // namespace System.Net
