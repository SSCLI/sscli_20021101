//------------------------------------------------------------------------------
// <copyright file="TCPListener.cs" company="Microsoft">
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
    using System;
    using System.Net;

    /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener"]/*' />
    /// <devdoc>
    /// <para>The <see cref='System.Net.Sockets.TcpListener'/> class provide TCP services at a higher level of abstraction than the <see cref='System.Net.Sockets.Socket'/>
    /// class. <see cref='System.Net.Sockets.TcpListener'/> is used to create a host process that
    /// listens for connections from TCP clients.</para>
    /// </devdoc>
    public class TcpListener {

        IPEndPoint m_ServerSocketEP;
        Socket m_ServerSocket;


        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.TcpListener"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the TcpListener class with the specified local
        ///       end point.
        ///    </para>
        /// </devdoc>
        public TcpListener(IPEndPoint localEP) {
            if (localEP==null) {
                throw new ArgumentNullException("localEP");
            }
            m_ServerSocketEP = localEP;
        }

        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.TcpListener1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the TcpListener class that listens to the
        ///       specified IP address and port.
        ///    </para>
        /// </devdoc>
        public TcpListener(IPAddress localaddr, int port) {
            if (localaddr==null) {
                throw new ArgumentNullException("localaddr");
            }
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }
            m_ServerSocketEP = new IPEndPoint(localaddr, port);
        }

        // implementation picks an address for client
        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.TcpListener2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initiailizes a new instance of the TcpListener class
        ///       that listens on the specified
        ///       port.
        ///    </para>
        /// </devdoc>
        public TcpListener(int port) {
            if (!ValidationHelper.ValidateTcpPort(port)) {
                throw new ArgumentOutOfRangeException("port");
            }
            m_ServerSocketEP = new IPEndPoint(IPAddress.Any, port);
        }

        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.Server"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used by the class to provide the underlying network socket.
        ///    </para>
        /// </devdoc>
        protected Socket Server {
            get {
                return m_ServerSocket;
            }
        }

        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.Active"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used
        ///       by the class to indicate that the listener's socket has been bound to a port
        ///       and started listening.
        ///    </para>
        /// </devdoc>
        protected bool Active {
            get {
                return m_ServerSocket!=null;
            }
        }
                
        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.LocalEndpoint"]/*' />
        /// <devdoc>
        ///    <para>
        ///        Gets the m_Active EndPoint for the local listener socket.
        ///    </para>
        /// </devdoc>
        public EndPoint LocalEndpoint {
            get {
                return (m_ServerSocket!=null) ? m_ServerSocket.LocalEndPoint : m_ServerSocketEP;
            }
        }

        // Start/stop the listener
        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.Start"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts listening to network requests.
        ///    </para>
        /// </devdoc>
        public void Start() {
            GlobalLog.Print("TCPListener::Start()");
            if (m_ServerSocket!=null) {
                //
                // TcpListener was already started,
                // ignore the failure and keep listening.
                //
                return;
            }
            //
            // create a new accepting socket (it will be in blocking mode)
            // then Bind() it to the supplied IPEndPoint and Listen()
            //
            m_ServerSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            m_ServerSocket.Bind(m_ServerSocketEP);
            m_ServerSocket.Listen((int)SocketOptionName.MaxConnections);
        }

        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.Stop"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes the network connection.
        ///    </para>
        /// </devdoc>
        public void Stop() {
            GlobalLog.Print("TCPListener::Stop()");
            if (m_ServerSocket==null) {
                //
                // TcpListener was already stopped or never started,
                // ignore the failure and don't listen.
                //
                return;
            }
            //
            // Close() the Socket and null it out so that we can reuse
            // the bound end point as soon as possible
            //
            m_ServerSocket.Close();
            m_ServerSocket = null;
        }

        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.Finalize"]/*' />
        ~TcpListener() {
            Stop();
        }


        // Determine if there are pending connections
        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.Pending"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determine if there are pending connection requests.
        ///    </para>
        /// </devdoc>
        public bool Pending() {
            if (m_ServerSocket==null) {
                throw new InvalidOperationException(SR.GetString(SR.net_stopped));

            }
            return m_ServerSocket.Poll(0, SelectMode.SelectRead);
        }

        // Accept the first pending connection
        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.AcceptSocket"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Accepts a pending connection request.
        ///    </para>
        /// </devdoc>
        public Socket AcceptSocket() {
            if (m_ServerSocket==null) {
                throw new InvalidOperationException(SR.GetString(SR.net_stopped));
            }
            return m_ServerSocket.Accept();
        }

        // UEUE
        /// <include file='doc\TCPListener.uex' path='docs/doc[@for="TcpListener.AcceptTcpClient"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public TcpClient AcceptTcpClient() {
            if (m_ServerSocket==null) {
                throw new InvalidOperationException(SR.GetString(SR.net_stopped));
            }
            Socket acceptedSocket = m_ServerSocket.Accept();
            TcpClient returnValue = new TcpClient(acceptedSocket);
            return returnValue;
        }

    }; // class TcpListener


} // namespace System.Net.Sockets
