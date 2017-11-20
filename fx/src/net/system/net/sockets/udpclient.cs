//------------------------------------------------------------------------------
// <copyright file="UDPClient.cs" company="Microsoft">
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

namespace System.Net.Sockets {

    /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The <see cref='System.Net.Sockets.UdpClient'/> class provides access to UDP services at a
    ///       higher abstraction level than the <see cref='System.Net.Sockets.Socket'/> class. <see cref='System.Net.Sockets.UdpClient'/>
    ///       is used to connect to a remote host and to receive connections from a remote
    ///       Client.
    ///    </para>
    /// </devdoc>
    public class UdpClient : IDisposable {

        private Socket m_ClientSocket;
        private bool m_Active;

        // bind to arbitrary IP+Port
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.UdpClient"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.Sockets.UdpClient'/>class.
        ///    </para>
        /// </devdoc>
        public UdpClient() {
            createClientSocket();
        }

        // bind specific port, arbitrary IP
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.UdpClient1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the UdpClient class that communicates on the
        ///       specified port number.
        ///    </para>
        /// </devdoc>
        public UdpClient(int port) {
            //
            // parameter validation
            //
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }

            IPEndPoint localEP = new IPEndPoint(IPAddress.Any, port);

            createClientSocket();

            Client.Bind(localEP);
        }

        // bind to given local endpoint
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.UdpClient2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the UdpClient class that communicates on the
        ///       specified end point.
        ///    </para>
        /// </devdoc>
        public UdpClient(IPEndPoint localEP) {
            //
            // parameter validation
            //
            if (localEP == null) {
                throw new ArgumentNullException("localEP");
            }

            createClientSocket();

            Client.Bind(localEP);
        }

        // bind and connect
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.UdpClient3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new instance of the <see cref='System.Net.Sockets.UdpClient'/> class and connects to the
        ///       specified remote host on the specified port.
        ///    </para>
        /// </devdoc>
        public UdpClient(string hostname, int port) {
            //
            // parameter validation
            //
            if (hostname == null) {
                throw new ArgumentNullException("hostname");
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }

            createClientSocket();
            Connect(hostname, port);
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Client"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used by the class to provide the underlying network socket.
        ///    </para>
        /// </devdoc>
        protected Socket Client {
            get {
                return m_ClientSocket;
            }
            set {
                m_ClientSocket = value;
            }
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Active"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used by the class to indicate that a connection to a remote host has been
        ///       made.
        ///    </para>
        /// </devdoc>
        protected bool Active {
            get {
                return m_Active;
            }
            set {
                m_Active = value;
            }
        }

        //UEUE
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Close"]/*' />
        public void Close() {
            GlobalLog.Print("UdpClient::Close()");
            this.FreeResources();
            GC.SuppressFinalize(this);
        }
        private bool m_CleanedUp = false;
        private void FreeResources() {
            //
            // only resource we need to free is the network stream, since this
            // is based on the client socket, closing the stream will cause us
            // to flush the data to the network, close the stream and (in the
            // NetoworkStream code) close the socket as well.
            //
            if (m_CleanedUp) {
                return;
            }

            Socket chkClientSocket = Client;
            if (chkClientSocket!=null) {
                //
                // if the NetworkStream wasn't retrieved, the Socket might
                // still be there and needs to be closed to release the effect
                // of the Bind() call and free the bound IPEndPoint.
                //
                chkClientSocket.InternalShutdown(SocketShutdown.Both);
                chkClientSocket.Close();
                Client = null;
            }
            m_CleanedUp = true;
        }
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            this.Close();
        }


        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Connect"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Establishes a connection to the specified port on the
        ///       specified host.
        ///    </para>
        /// </devdoc>
        public void Connect(string hostname, int port) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }

            IPAddress address = Dns.Resolve(hostname).AddressList[0];
            CheckForBroadcast(address);
            IPEndPoint endPoint = new IPEndPoint(address, port);
            Connect(endPoint);
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Connect1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Establishes a connection with the host at the specified address on the
        ///       specified port.
        ///    </para>
        /// </devdoc>
        public void Connect(IPAddress addr, int port) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (addr==null){
                throw new ArgumentNullException("addr");
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }

            CheckForBroadcast(addr);
            IPEndPoint endPoint = new IPEndPoint(addr, port);
            Connect(endPoint);
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Connect2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Establishes a connection to a remote end point.
        ///    </para>
        /// </devdoc>
        public void Connect(IPEndPoint endPoint) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (endPoint==null){
                throw new ArgumentNullException("endPoint");
            }

            CheckForBroadcast(endPoint.Address);
            Client.Connect(endPoint);
            m_Active = true;
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Send"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends a UDP datagram to the host at the remote end point.
        ///    </para>
        /// </devdoc>
        public int Send(byte[] dgram, int bytes, IPEndPoint endPoint) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (dgram==null){
                throw new ArgumentNullException("dgram");
            }
            if (m_Active && endPoint!=null) {
                //
                // Do not allow sending packets to arbitrary host when connected
                //
                throw new InvalidOperationException(SR.GetString(SR.net_udpconnected));
            }

            if (endPoint==null) {
                return Client.Send(dgram, 0, bytes, SocketFlags.None);
            }

            CheckForBroadcast(endPoint.Address);

            return Client.SendTo(dgram, 0, bytes, SocketFlags.None, endPoint);
        }

        private bool m_IsBroadcast;
        private void CheckForBroadcast(IPAddress ipAddress) {
            //
            //                         05/01/2001 here we check to see if the user is trying to use a Broadcast IP address
            // we only detect IPAddress.Broadcast (which is not the only Broadcast address)
            // and in that case we set SocketOptionName.Broadcast on the socket to allow its use.
            // if the user really wants complete control over Broadcast addresses he needs to
            // inherit from UdpClient and gain control over the Socket and do whatever is appropriate.
            //
            if (Client!=null && !m_IsBroadcast && ipAddress.IsBroadcast) {
                //
                // we need to set the Broadcast socket option.
                // note that, once we set the option on the Socket, we never reset it.
                //
                m_IsBroadcast = true;
                Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Broadcast, 1);
            }
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Send1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends a UDP datagram to the specified port on the specified remote host.
        ///    </para>
        /// </devdoc>
        public int Send(byte[] dgram, int bytes, string hostname, int port) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (dgram==null){
                throw new ArgumentNullException("dgram");
            }
            if (m_Active && ((hostname != null) || (port != 0))) {
                //
                // Do not allow sending packets to arbitrary host when connected
                //
                throw new InvalidOperationException(SR.GetString(SR.net_udpconnected));
            }

            if (hostname==null || port==0) {
                return Client.Send(dgram, 0, bytes, SocketFlags.None);
            }

            IPAddress address = Dns.Resolve(hostname).AddressList[0];

            CheckForBroadcast(address);

            IPEndPoint ipEndPoint = new IPEndPoint(address, port);

            return Client.SendTo(dgram, 0, bytes, SocketFlags.None, ipEndPoint);
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Send2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sends a UDP datagram to a
        ///       remote host.
        ///    </para>
        /// </devdoc>
        public int Send(byte[] dgram, int bytes) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (dgram==null){
                throw new ArgumentNullException("dgram");
            }
            if (!m_Active) {
                //
                // only allowed on connected socket
                //
                throw new InvalidOperationException(SR.GetString(SR.net_notconnected));
            }

            return Client.Send(dgram, 0, bytes, SocketFlags.None);

        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.Receive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a datagram sent by a server.
        ///    </para>
        /// </devdoc>
        public byte[] Receive(ref IPEndPoint remoteEP) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            //
            // negative timeout implies blocking operation
            // basically block until data is available and then read it.
            //
            Client.Poll(-1, SelectMode.SelectRead);

            //
            // receive as much data as is available to read from the client socket
            //
            int available = Client.Available;
            byte[] buffer = new byte[available];

            //
            // this is a fix due to the nature of the ReceiveFrom() call and the 
            // ref parameter convention, we need to cast an IPEndPoint to it's base
            // class EndPoint and cast it back down to IPEndPoint. ugly but it works.
            //
            EndPoint tempRemoteEP = (EndPoint)IPEndPoint.Any;
            Client.ReceiveFrom(buffer, available, 0 , ref tempRemoteEP);
            remoteEP = (IPEndPoint)tempRemoteEP;

            return buffer;
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.JoinMulticastGroup"]/*' />
        ///     <devdoc>
        ///         <para>
        ///             Joins a multicast address group.
        ///         </para>
        ///     </devdoc>
        public void JoinMulticastGroup(IPAddress multicastAddr) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }

            MulticastOption mcOpt = new MulticastOption(multicastAddr);

            Client.SetSocketOption(
                SocketOptionLevel.IP,
                SocketOptionName.AddMembership,
                mcOpt );
        }
        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.JoinMulticastGroup1"]/*' />
        /// <devdoc>
        ///     <para>
        ///         Joins a multicast address group with the specified time to live (TTL).
        ///     </para>
        /// </devdoc>
        public void JoinMulticastGroup(IPAddress multicastAddr, int timeToLive) {
            //
            // parameter validation;
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (multicastAddr==null){
                throw new ArgumentNullException("multicastAddr");
            }
            if (!ValidationHelper.ValidateRange(timeToLive, 0, 255)) {
                throw new ArgumentOutOfRangeException("timeToLive");
            }

            //
            // join the Multicast Group
            //
            JoinMulticastGroup(multicastAddr);

            //
            // set Time To Live (TLL)
            //
            Client.SetSocketOption(
                SocketOptionLevel.IP,
                SocketOptionName.MulticastTimeToLive,
                timeToLive );
        }

        /// <include file='doc\UDPClient.uex' path='docs/doc[@for="UdpClient.DropMulticastGroup"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Leaves a multicast address group.
        ///    </para>
        /// </devdoc>
        public void DropMulticastGroup(IPAddress multicastAddr) {
            //
            // parameter validation
            //
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (multicastAddr==null){
                throw new ArgumentNullException("multicastAddr");
            }

            MulticastOption mcOpt = new MulticastOption(multicastAddr);

            Client.SetSocketOption(
                SocketOptionLevel.IP,
                SocketOptionName.DropMembership,
                mcOpt );
        }

        private void createClientSocket() {
            //
            // common initialization code
            //
            Client = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
        }

    } // class UdpClient



} // namespace System.Net.Sockets
