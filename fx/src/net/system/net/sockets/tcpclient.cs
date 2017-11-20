//------------------------------------------------------------------------------
// <copyright file="TCPClient.cs" company="Microsoft">
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

    /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient"]/*' />
    /// <devdoc>
    /// <para>The <see cref='System.Net.Sockets.TcpClient'/> class provide TCP services at a higher level
    ///    of abstraction than the <see cref='System.Net.Sockets.Socket'/> class. <see cref='System.Net.Sockets.TcpClient'/>
    ///    is used to create a Client connection to a remote host.</para>
    /// </devdoc>
    public class TcpClient : IDisposable {

        Socket m_ClientSocket;
        bool m_Active;
        NetworkStream m_DataStream;
        bool m_DataStreamCreated;

        // specify local IP and port
        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.TcpClient"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.Sockets.TcpClient'/>
        ///       class with the specified end point.
        ///    </para>
        /// </devdoc>
        public TcpClient(IPEndPoint localEP) {
            if (localEP==null) {
                throw new ArgumentNullException("localEP");
            }
            initialize();
            Client.Bind(localEP);
        }

        // TcpClient(IPaddress localaddr); // port is arbitrary
        // TcpClient(int outgoingPort); // local IP is arbitrary

        // address+port is arbitrary
        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.TcpClient1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Net.Sockets.TcpClient'/> class.
        ///    </para>
        /// </devdoc>
        public TcpClient() {
            initialize();
        }

        // bind and connect
        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.TcpClient2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Net.Sockets.TcpClient'/> class and connects to the
        ///    specified port on the specified host.</para>
        /// </devdoc>
        public TcpClient(string hostname, int port) {
            if (hostname==null) {
                throw new ArgumentNullException("hostname");
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }
            initialize();
            Connect(hostname, port);
        }

        //
        // used by TcpListener.Accept()
        //
        internal TcpClient(Socket acceptedSocket) {
            Client = acceptedSocket;
            m_Active = true;
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.Client"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used by the class to provide
        ///       the underlying network socket.
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

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.Active"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used by the class to indicate that a connection has been made.
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

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.Connect"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Connects the Client to the specified port on the specified host.
        ///    </para>
        /// </devdoc>
        public void Connect(string hostname, int port) {
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (hostname==null) {
                throw new ArgumentNullException("hostname");
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }
            IPAddress address = Dns.Resolve(hostname).AddressList[0];
            Connect(address, port);
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.Connect1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Connects the Client to the specified port on the specified host.
        ///    </para>
        /// </devdoc>
        public void Connect(IPAddress address, int port) {
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (address==null) {
                throw new ArgumentNullException("address");
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }
            IPEndPoint remoteEP = new IPEndPoint(address, port);
            Connect(remoteEP);
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.Connect2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Connect the Client to the specified end point.
        ///    </para>
        /// </devdoc>
        public void Connect(IPEndPoint remoteEP) {
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (remoteEP==null) {
                throw new ArgumentNullException("remoteEP");
            }
            Client.Connect(remoteEP);
            m_Active = true;
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.GetStream"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the stream used to read and write data to the
        ///       remote host.
        ///    </para>
        /// </devdoc>
        public NetworkStream GetStream() {
            if (m_CleanedUp){
                throw new ObjectDisposedException(this.GetType().FullName);
            }
            if (!Client.Connected) {
                throw new InvalidOperationException(SR.GetString(SR.net_notconnected));
            }
            if (m_DataStream==null) {
                m_DataStream = new NetworkStream(Client, true);
                m_DataStreamCreated = true;
            }
            return m_DataStream;
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disposes the Tcp connection.
        ///    </para>
        /// </devdoc>
        //UEUE
        public void Close() {
            GlobalLog.Print("TcpClient::Close()");
            ((IDisposable)this).Dispose();
        }

        private bool m_CleanedUp = false;

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.Dispose"]/*' />
        protected virtual void Dispose(bool disposing) {
            if (m_CleanedUp) {
                return;
            }

            if (disposing) {
                //no managed objects to cleanup
            }

            if (!m_DataStreamCreated) {
                //
                // if the NetworkStream wasn't created, the Socket might
                // still be there and needs to be closed. In the case in which
                // we are bound to a local IPEndPoint this will remove the
                // binding and free up the IPEndPoint for later uses.
                //
                Socket chkClientSocket = Client;
                if (chkClientSocket!= null) {
                    chkClientSocket.InternalShutdown(SocketShutdown.Both);
                    chkClientSocket.Close();
                    Client = null;
                }
            }
            //
            // wether the NetworkStream was created or not, we need to
            // lose its internal reference so that this TcpClient can be
            // collected and the NetworkStream can still be used.
            // note that the NetworkStream was created as owning the Socket,
            // so we lose the internal reference to the Socket above as well.
            //
            m_DataStream = null;
            m_CleanedUp = true;
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.Finalize"]/*' />
        ~TcpClient() {
            Dispose(false);
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.ReceiveBufferSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the size of the receive buffer in bytes.
        ///    </para>
        /// </devdoc>
        public int ReceiveBufferSize {
            get {
                return numericOption(SocketOptionLevel.Socket,
                                     SocketOptionName.ReceiveBuffer);
            }
            set {
                Client.SetSocketOption(SocketOptionLevel.Socket,
                                  SocketOptionName.ReceiveBuffer, value);
            }
        }


        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.SendBufferSize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the size of the send buffer in bytes.
        ///    </para>
        /// </devdoc>
        public int SendBufferSize {
            get {
                return numericOption(SocketOptionLevel.Socket,
                                     SocketOptionName.SendBuffer);
            }

            set {
                Client.SetSocketOption(SocketOptionLevel.Socket,
                                  SocketOptionName.SendBuffer, value);
            }
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.ReceiveTimeout"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the receive time out value of the connection in seconds.
        ///    </para>
        /// </devdoc>
        public int ReceiveTimeout {
            get {
                return numericOption(SocketOptionLevel.Socket,
                                     SocketOptionName.ReceiveTimeout);
            }
            set {
                Client.SetSocketOption(SocketOptionLevel.Socket,
                                  SocketOptionName.ReceiveTimeout, value);
            }
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.SendTimeout"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the send time out value of the connection in seconds.
        ///    </para>
        /// </devdoc>
        public int SendTimeout {
            get {
                return numericOption(SocketOptionLevel.Socket, SocketOptionName.SendTimeout);
            }

            set {
                Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.SendTimeout, value);
            }
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.LingerState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of the connection's linger option.
        ///    </para>
        /// </devdoc>
        public LingerOption LingerState {
            get {
                return (LingerOption)Client.GetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Linger);
            }
            set {
                Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Linger, value);
            }
        }

        /// <include file='doc\TCPClient.uex' path='docs/doc[@for="TcpClient.NoDelay"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Enables or disables delay when send or receive buffers are full.
        ///    </para>
        /// </devdoc>
        public bool NoDelay {
            get {
                return numericOption(SocketOptionLevel.Tcp, SocketOptionName.NoDelay) != 0 ? true : false;
            }
            set {
                Client.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.NoDelay, value ? 1 : 0);
            }
        }

        private void initialize() {
            Client = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            m_Active = false;
        }

        private int numericOption(SocketOptionLevel optionLevel, SocketOptionName optionName) {
            return (int)Client.GetSocketOption(optionLevel, optionName);
        }

    }; // class TCPClient


} // namespace System.Net.Sockets
