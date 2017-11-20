//------------------------------------------------------------------------------
// <copyright file="IPAddress.cs" company="Microsoft">
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
    using System.Net.Sockets;
    using System.Globalization;
    
    /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress"]/*' />
    /// <devdoc>
    ///    <para>Provides an internet protocol (IP) address.</para>
    /// </devdoc>
    [Serializable]
    public class IPAddress {

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Any"]/*' />
        public static readonly IPAddress Any = new IPAddress(0x0000000000000000);
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Loopback"]/*' />
        public static readonly  IPAddress Loopback = new IPAddress(0x000000000100007F);
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Broadcast"]/*' />
        public static readonly  IPAddress Broadcast = new IPAddress(0x00000000FFFFFFFF);
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.None"]/*' />
        public static readonly  IPAddress None = Broadcast;

        internal const long LoopbackMask = 0x000000000000007F;
        internal const string InaddrNoneString = "255.255.255.255";
        internal const string InaddrNoneStringHex = "0xff.0xff.0xff.0xff";
        internal const string InaddrNoneStringOct = "0377.0377.0377.0377";

        private long m_Address;


        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.IPAddress"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.IPAddress'/>
        ///       class with the specified
        ///       address.
        ///    </para>
        /// </devdoc>
        public IPAddress(long newAddress) {
            if (newAddress<0 || newAddress>0x00000000FFFFFFFF) {
                throw new ArgumentOutOfRangeException("newAddress");
            }
            m_Address = newAddress;
        }

        //
        // we need this internally since we need to interface with winsock
        // and winsock only understands Int32
        //
        internal IPAddress(int newAddress) {
            m_Address = (long)newAddress & 0x00000000FFFFFFFF;
        }


        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Parse"]/*' />
        /// <devdoc>
        /// <para>Converts an IP address string to an <see cref='System.Net.IPAddress'/>
        /// instance.</para>
        /// </devdoc>
        public static IPAddress Parse(string ipString) {
            if (ipString == null) {
                throw new ArgumentNullException("ipString");
            }

            int address = UnsafeNclNativeMethods.OSSOCK.inet_addr(ipString);
            
#if BIGENDIAN
            // OSSOCK.inet_addr always returns IP address as a byte array converted to int.
            // thus we need to convert the address into a uniform integer value.
            address = (int)( ((uint)address << 24) | (((uint)address & 0x0000FF00) << 8) |
                (((uint)address >> 8) & 0x0000FF00) | ((uint)address >> 24) );
#endif

            GlobalLog.Print("IPAddress::Parse() ipString:" + ValidationHelper.ToString(ipString) + " inet_addr() returned address:" + address.ToString());

            if (address==-1
                && string.Compare(ipString, InaddrNoneString, false, CultureInfo.InvariantCulture)!=0
                && string.Compare(ipString, InaddrNoneStringHex, true, CultureInfo.InvariantCulture)!=0
                && string.Compare(ipString, InaddrNoneStringOct, false, CultureInfo.InvariantCulture)!=0) {
                throw new FormatException(SR.GetString(SR.dns_bad_ip_address));
            }

            IPAddress returnValue = new IPAddress(address);

            return returnValue;

        } // Parse


        /*
        public static IPAddress Parse(string ipString) {
            if (ipString == null) {
                throw new ArgumentNullException("ipString");
            }
            if (Dns.s_Initialized) {
                GlobalLog.Print("IPAddress::Parse() initialized Sockets");
            }

            SocketAddress socketAddress = IPEndPoint.Any.Serialize();

            int errorCode =
                UnsafeNclNativeMethods.OSSOCK.WSAStringToAddress(
                    ipString,
                    AddressFamily.InterNetwork,
                    IntPtr.Zero,
                    socketAddress.m_Buffer,
                    ref socketAddress.m_Size );

            GlobalLog.Print("IPAddress::Parse() ipString:[" + ValidationHelper.ToString(ipString) + "] WSAStringToAddress() returned errorCode:" + errorCode.ToString());

            if (errorCode!=SocketErrors.Success) {
                throw new FormatException(SR.GetString(SR.dns_bad_ip_address), new SocketException());
            }

            IPAddress ipAddress = ((IPEndPoint)IPEndPoint.Any.Create(socketAddress)).Address;
            return ipAddress;

        } // Parse
        */


        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Address"]/*' />
        public long Address {
            get {
                return m_Address;
            }
            set {
                m_Address = value;
            }
        }

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.AddressFamily"]/*' />
        public AddressFamily AddressFamily {
            get {
                return AddressFamily.InterNetwork;
            }
        }

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts the Internet address to standard dotted quad format.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return
                (m_Address&0xFF).ToString() + "." +
                ((m_Address>>8)&0xFF).ToString() + "." +
                ((m_Address>>16)&0xFF).ToString() + "." +
                ((m_Address>>24)&0xFF).ToString();
        }

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.HostToNetworkOrder"]/*' />
        public static long HostToNetworkOrder(long host) {
#if BIGENDIAN
            return host;
#else        
            return (((long)HostToNetworkOrder((int)host) & 0xFFFFFFFF) << 32)
                    | ((long)HostToNetworkOrder((int)(host >> 32)) & 0xFFFFFFFF);
#endif                    
        }
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.HostToNetworkOrder1"]/*' />
        public static int HostToNetworkOrder(int host) {
#if BIGENDIAN
            return host;
#else                    
            return (((int)HostToNetworkOrder((short)host) & 0xFFFF) << 16)
                    | ((int)HostToNetworkOrder((short)(host >> 16)) & 0xFFFF);
#endif                    
        }
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.HostToNetworkOrder2"]/*' />
        public static short HostToNetworkOrder(short host) {
#if BIGENDIAN
            return host;
#else
            return (short)((((int)host & 0xFF) << 8) | (int)((host >> 8) & 0xFF));
#endif
        }
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.NetworkToHostOrder"]/*' />
        public static long NetworkToHostOrder(long network) {
            return HostToNetworkOrder(network);
        }
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.NetworkToHostOrder1"]/*' />
        public static int NetworkToHostOrder(int network) {
            return HostToNetworkOrder(network);
        }
        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.NetworkToHostOrder2"]/*' />
        public static short NetworkToHostOrder(short network) {
            return HostToNetworkOrder(network);
        }

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.IsLoopback"]/*' />
        public static bool IsLoopback(IPAddress address) {
            return ((address.m_Address & IPAddress.LoopbackMask) == (IPAddress.Loopback.Address & IPAddress.LoopbackMask));
        }

        internal bool IsBroadcast {
            get {
                return m_Address==Broadcast.m_Address;
            }
        }


        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.Equals"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares to IP addresses.
        ///    </para>
        /// </devdoc>
        public override bool Equals(object comparand) {
            if (!(comparand is IPAddress)) {
                return false;
            }
            return ((IPAddress)comparand).m_Address==this.m_Address;
        }

        /// <include file='doc\IPAddress.uex' path='docs/doc[@for="IPAddress.GetHashCode"]/*' />
        public override int GetHashCode() {
            return unchecked((int)m_Address);
        }

    } // class IPAddress


} // namespace System.Net
