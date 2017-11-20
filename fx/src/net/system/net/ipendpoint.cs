//------------------------------------------------------------------------------
// <copyright file="IPEndPoint.cs" company="Microsoft">
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

    /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides an IP address.
    ///    </para>
    /// </devdoc>
    [Serializable]
    public class IPEndPoint : EndPoint {
        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.MinPort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the minimum acceptable value for the <see cref='System.Net.IPEndPoint.Port'/>
        ///       property.
        ///    </para>
        /// </devdoc>
        public const int MinPort = 0x00000000;
        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.MaxPort"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies the maximum acceptable value for the <see cref='System.Net.IPEndPoint.Port'/>
        ///       property.
        ///    </para>
        /// </devdoc>
        public const int MaxPort = 0x0000FFFF;

        private IPAddress m_Address;
        private int m_Port;
        private const int AddressSize = 16;

        internal const int AnyPort = MinPort;
        internal static IPEndPoint Any = new IPEndPoint(IPAddress.Any, AnyPort);


        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.AddressFamily"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override AddressFamily AddressFamily {
            get {
                return Sockets.AddressFamily.InterNetwork;
            }
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.IPEndPoint"]/*' />
        /// <devdoc>
        ///    <para>Creates a new instance of the IPEndPoint class with the specified address and
        ///       port.</para>
        /// </devdoc>
        public IPEndPoint(long address, int port) {
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }
            m_Port = port;
            m_Address = new IPAddress(address);
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.IPEndPoint1"]/*' />
        /// <devdoc>
        ///    <para>Creates a new instance of the IPEndPoint class with the specified address and port.</para>
        /// </devdoc>
        public IPEndPoint(IPAddress address, int port) {
            if (address==null) {
                throw new ArgumentNullException("address");
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }
            m_Port = port;
            m_Address = address;
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.Address"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the IP address.
        ///    </para>
        /// </devdoc>
        public IPAddress Address {
            get {
                return m_Address;
            }
            set {
                m_Address = value;
            }
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.Port"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the port.
        ///    </para>
        /// </devdoc>
        public int Port {
            get {
                return m_Port;
            }
            set {
                if (!ValidationHelper.ValidateTcpPort(value)) {
                    throw new ArgumentOutOfRangeException("value");
                }
                m_Port = value;
            }
        }


        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ToString() {
            return Address.ToString() + ":" + Port.ToString();
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.Serialize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override SocketAddress Serialize() {
            //
            // create a new SocketAddress
            //
            SocketAddress socketAddress = new SocketAddress(this.AddressFamily, AddressSize);
            //
            // populate it
            //
            int port = this.Port;
            socketAddress[2] = unchecked((byte)(port>>8));
            socketAddress[3] = unchecked((byte)(port   ));

            int address = unchecked((int)this.Address.Address);
            socketAddress[4] = unchecked((byte)(address    ));
            socketAddress[5] = unchecked((byte)(address>> 8));
            socketAddress[6] = unchecked((byte)(address>>16));
            socketAddress[7] = unchecked((byte)(address>>24));

            GlobalLog.Print("IPEndPoint::Serialize: " + this.ToString() );

            //
            // return it
            //
            return socketAddress;
        }

        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.Create"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override EndPoint Create(SocketAddress socketAddress) {
            //
            // validate SocketAddress
            //
            if (socketAddress.Family != this.AddressFamily) {
                throw new ArgumentException(SR.GetString(SR.net_InvalidAddressFamily, socketAddress.Family.ToString(), this.GetType().FullName, this.AddressFamily.ToString()));
            }
            if (socketAddress.Size<8) {
                throw new ArgumentException(SR.GetString(SR.net_InvalidSocketAddressSize, socketAddress.GetType().FullName, this.GetType().FullName));
            }
            //
            // strip out of SocketAddress information on the EndPoint
            //
            int port = 
                    ((int)socketAddress[2]<<8) |
                    ((int)socketAddress[3]);

            long address = (long)(
                    ((int)socketAddress[4]    ) |
                    ((int)socketAddress[5]<<8 ) |
                    ((int)socketAddress[6]<<16) |
                    ((int)socketAddress[7]<<24)
                    ) & 0x00000000FFFFFFFF;

            IPEndPoint created = new IPEndPoint(address, port);

            GlobalLog.Print("IPEndPoint::Create: " + this.ToString() + " -> " + created.ToString() );

            //
            // return it
            //
            return created;
        }


        //UEUE
        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.Equals"]/*' />
        public override bool Equals(object comparand) {
            if (!(comparand is IPEndPoint)) {
                return false;
            }
            return ((IPEndPoint)comparand).m_Address.Equals(m_Address) && ((IPEndPoint)comparand).m_Port==m_Port;
        }

        //UEUE
        /// <include file='doc\IPEndPoint.uex' path='docs/doc[@for="IPEndPoint.GetHashCode"]/*' />
        public override int GetHashCode() {
            return m_Address.GetHashCode() ^ m_Port;
        }


    }; // class IPEndPoint


} // namespace System.Net





/*

#define IPPROTO_IPV6 41

//
// Desired design of maximum size and alignment.
// These are implementation specific.
//
#define _SS_MAXSIZE 128                  // Maximum size.
#define _SS_ALIGNSIZE (sizeof(__int64))  // Desired alignment.

//
// Definitions used for sockaddr_storage structure paddings design.
//
#define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof (short))
#define _SS_PAD2SIZE (_SS_MAXSIZE - (sizeof (short) + _SS_PAD1SIZE \
                                                    + _SS_ALIGNSIZE))

struct sockaddr_storage {
    short ss_family;               // Address family.
    char __ss_pad1[_SS_PAD1SIZE];  // 6 byte pad, this is to make
                                   // implementation specific pad up to
                                   // alignment field that follows explicit
                                   // in the data structure.
    __int64 __ss_align;            // Field to force desired structure.
    char __ss_pad2[_SS_PAD2SIZE];  // 112 byte pad to achieve desired size;
                                   // _SS_MAXSIZE value minus size of
                                   // ss_family, __ss_pad1, and
                                   // __ss_align fields is 112.
};

typedef struct sockaddr_storage SOCKADDR_STORAGE;
typedef struct sockaddr_storage *PSOCKADDR_STORAGE;
typedef struct sockaddr_storage FAR *LPSOCKADDR_STORAGE;

#endif // !IPPROTO_IPV6
#endif // _WINSOCK2API_

#ifdef _WS2TCPIP_H_
// This section gets included if ws2tcpip.h is included

#ifndef IPV6_JOIN_GROUP

#define in6_addr in_addr6

// Macro that works for both IPv4 and IPv6
#define SS_PORT(ssp) (((struct sockaddr_in*)(ssp))->sin_port)

#define IN6ADDR_ANY_INIT        { 0 }
#define IN6ADDR_LOOPBACK_INIT   { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }

TPIPV6_EXTERN const struct in6_addr in6addr_any;
TPIPV6_EXTERN const struct in6_addr in6addr_loopback;

TPIPV6_INLINE int
IN6_ADDR_EQUAL(const struct in6_addr *a, const struct in6_addr *b)
{
    return (memcmp(a, b, sizeof(struct in6_addr)) == 0);
}

TPIPV6_INLINE int
IN6_IS_ADDR_UNSPECIFIED(const struct in6_addr *a)
{
    return IN6_ADDR_EQUAL(a, &in6addr_any);
}

TPIPV6_INLINE int
IN6_IS_ADDR_LOOPBACK(const struct in6_addr *a)
{
    return IN6_ADDR_EQUAL(a, &in6addr_loopback);
}

TPIPV6_INLINE int
IN6_IS_ADDR_MULTICAST(const struct in6_addr *a)
{
    return (a->s6_addr[0] == 0xff);
}

TPIPV6_INLINE int
IN6_IS_ADDR_LINKLOCAL(const struct in6_addr *a)
{
    return ((a->s6_addr[0] == 0xfe) &&
            ((a->s6_addr[1] & 0xc0) == 0x80));
}

TPIPV6_INLINE int
IN6_IS_ADDR_SITELOCAL(const struct in6_addr *a)
{
    return ((a->s6_addr[0] == 0xfe) &&
            ((a->s6_addr[1] & 0xc0) == 0xc0));
}

TPIPV6_INLINE int
IN6_IS_ADDR_V4MAPPED(const struct in6_addr *a)
{
    return ((a->s6_addr[0] == 0) &&
            (a->s6_addr[1] == 0) &&
            (a->s6_addr[2] == 0) &&
            (a->s6_addr[3] == 0) &&
            (a->s6_addr[4] == 0) &&
            (a->s6_addr[5] == 0) &&
            (a->s6_addr[6] == 0) &&
            (a->s6_addr[7] == 0) &&
            (a->s6_addr[8] == 0) &&
            (a->s6_addr[9] == 0) &&
            (a->s6_addr[10] == 0xff) &&
            (a->s6_addr[11] == 0xff));
}

TPIPV6_INLINE int
IN6_IS_ADDR_V4COMPAT(const struct in6_addr *a)
{
    return ((a->s6_addr[0] == 0) &&
            (a->s6_addr[1] == 0) &&
            (a->s6_addr[2] == 0) &&
            (a->s6_addr[3] == 0) &&
            (a->s6_addr[4] == 0) &&
            (a->s6_addr[5] == 0) &&
            (a->s6_addr[6] == 0) &&
            (a->s6_addr[7] == 0) &&
            (a->s6_addr[8] == 0) &&
            (a->s6_addr[9] == 0) &&
            (a->s6_addr[10] == 0) &&
            (a->s6_addr[11] == 0) &&
            !((a->s6_addr[12] == 0) &&
             (a->s6_addr[13] == 0) &&
             (a->s6_addr[14] == 0) &&
             ((a->s6_addr[15] == 0) ||
             (a->s6_addr[15] == 1))));
}

TPIPV6_INLINE int
IN6_IS_ADDR_MC_NODELOCAL(const struct in6_addr *a)
{
    return IN6_IS_ADDR_MULTICAST(a) && ((a->s6_addr[1] & 0xf) == 1);
}

TPIPV6_INLINE int
IN6_IS_ADDR_MC_LINKLOCAL(const struct in6_addr *a)
{
    return IN6_IS_ADDR_MULTICAST(a) && ((a->s6_addr[1] & 0xf) == 2);
}

TPIPV6_INLINE int
IN6_IS_ADDR_MC_SITELOCAL(const struct in6_addr *a)
{
    return IN6_IS_ADDR_MULTICAST(a) && ((a->s6_addr[1] & 0xf) == 5);
}

TPIPV6_INLINE int
IN6_IS_ADDR_MC_ORGLOCAL(const struct in6_addr *a)
{
    return IN6_IS_ADDR_MULTICAST(a) && ((a->s6_addr[1] & 0xf) == 8);
}

TPIPV6_INLINE int
IN6_IS_ADDR_MC_GLOBAL(const struct in6_addr *a)
{
    return IN6_IS_ADDR_MULTICAST(a) && ((a->s6_addr[1] & 0xf) == 0xe);
}

// Argument structure for IPV6_JOIN_GROUP and IPV6_LEAVE_GROUP

typedef struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr;  // IPv6 multicast address.
    unsigned int    ipv6mr_interface;  // Interface index.
} IPV6_MREQ;

//
// Socket options at the IPPROTO_IPV6 level.
//
#define IPV6_UNICAST_HOPS       4  // Set/get IP unicast hop limit.
#define IPV6_MULTICAST_IF       9  // Set/get IP multicast interface.
#define IPV6_MULTICAST_HOPS     10 // Set/get IP multicast ttl.
#define IPV6_MULTICAST_LOOP     11 // Set/get IP multicast loopback.
#define IPV6_ADD_MEMBERSHIP     12 // Add an IP group membership.
#define IPV6_DROP_MEMBERSHIP    13 // Drop an IP group membership.
#define IPV6_JOIN_GROUP         IPV6_ADD_MEMBERSHIP
#define IPV6_LEAVE_GROUP        IPV6_DROP_MEMBERSHIP

//
// Socket options at the IPPROTO_UDP level.
//
#define UDP_CHECKSUM_COVERAGE   20  // Set/get UDP-Lite checksum coverage.

//
// Structure used in getaddrinfo() call.
//
typedef struct AddressInfo {
    int ai_flags;              // AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST.
    int ai_family;             // PF_xxx.
    int ai_socktype;           // SOCK_xxx.
    int ai_protocol;           // 0 or IPPROTO_xxx for IPv4 and IPv6.
    size_t ai_addrlen;         // Length of ai_addr.
    char *ai_canonname;        // Canonical name for nodename.
    struct sockaddr *ai_addr;  // Binary address.
    struct AddressInfo *ai_next;  // Next structure in linked list.
} ADDRINFO, FAR * LPADDRINFO;

*/
