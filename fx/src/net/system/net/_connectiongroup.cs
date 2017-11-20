//------------------------------------------------------------------------------
// <copyright file="_ConnectionGroup.cs" company="Microsoft">
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

    using System.Collections;
    using System.Diagnostics;

    //
    // ConnectionGroup groups a list of connections within the ServerPoint context,
    //   this used to keep context for things such as proxies or seperate clients.
    //

    internal class ConnectionGroup {

        //
        // Members
        //

        private ServicePoint    m_ServicePoint;
        private string          m_Name;
        private int             m_ConnectionLimit;
        private bool            m_UserDefinedLimit;
        private ArrayList       m_ConnectionList;
        private IPAddress       m_IPAddress;
        private Version         m_Version;
        const   int             defConnectionListSize  = 3;

        //
        // Constructors
        //

        internal ConnectionGroup(
            ServicePoint   servicePoint,
            IPAddress      ipAddress,
            int            connectionLimit,
            String         connName) {

            m_ServicePoint      = servicePoint;
            m_ConnectionLimit   = connectionLimit;
            m_UserDefinedLimit  = servicePoint.UserDefinedLimit;
            m_ConnectionList    = new ArrayList(defConnectionListSize); //it may grow beyond
            m_IPAddress         = ipAddress;
            m_Name              = MakeQueryStr(connName);

            GlobalLog.Print("ConnectionGroup::.ctor m_ConnectionLimit:" + m_ConnectionLimit.ToString());
        }

        //
        // Accessors
        //
        public string Name {
            get {
                return m_Name;
            }
        }

        public int CurrentConnections {
            get {
                return m_ConnectionList.Count;
            }
        }

        public int ConnectionLimit {
            get {
                return m_ConnectionLimit;
            }
            set {
                m_ConnectionLimit = value;
                m_UserDefinedLimit = true;
                GlobalLog.Print("ConnectionGroup::ConnectionLimit.set m_ConnectionLimit:" + m_ConnectionLimit.ToString());
            }
        }

        internal int InternalConnectionLimit {
            get {
                return m_ConnectionLimit;
            }
            set {
                if (!m_UserDefinedLimit) {
                    m_ConnectionLimit = value;
                    GlobalLog.Print("ConnectionGroup::InternalConnectionLimit.set m_ConnectionLimit:" + m_ConnectionLimit.ToString());
                }
            }
        }

        internal Version ProtocolVersion {
            get {
                return m_Version;
            }
            set {
                m_Version = value;
            }
        }

        internal IPAddress RemoteIPAddress {
            get {
                return m_IPAddress;
            }
            set {
                m_IPAddress = value;
            }
        }

        //
        // Methods
        //

        internal static string MakeQueryStr(string connName) {
            return ((connName == null) ? "" : connName);
        }


        /// <devdoc>
        ///    <para>         
        ///       These methods are made available to the underlying Connection
        ///       object so that we don't leak them because we're keeping a local
        ///       reference in our m_ConnectionList.
        ///       Called by the Connection's constructor
        ///    </para>
        /// </devdoc>
        public void Associate(WeakReference connection) {
            lock (m_ConnectionList) {
                m_ConnectionList.Add(connection);                
            }
            GlobalLog.Print("ConnectionGroup::Associate() Connection:" + connection.Target.GetHashCode());
        }
        
        
        
        /// <devdoc>
        ///    <para>         
        ///       Used by the Connection's explicit finalizer (note this is
        ///       not a destructor, since that's never calld unless we
        ///       remove reference to the object from our internal list)        
        ///    </para>
        /// </devdoc>
        public void Disassociate(WeakReference connection) {
            lock (m_ConnectionList) {
                m_ConnectionList.Remove(connection);
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Forces all connections on the ConnectionGroup to not be KeepAlive.
        ///    </para>
        /// </devdoc>
        internal void DisableKeepAliveOnConnections() {
            lock (m_ConnectionList) {
                GlobalLog.Print("ConnectionGroup#" + ValidationHelper.HashString(this) + "::DisableKeepAliveOnConnections() Count:" + m_ConnectionList.Count);
                foreach (WeakReference currentConnectionReference in m_ConnectionList) {
                    Connection currentConnection = null;
                    if (currentConnectionReference!=null) {
                        currentConnection = currentConnectionReference.Target as Connection;
                    }
                    //
                    // If the weak reference is alive, set KeepAlive to false
                    //
                    if (currentConnection != null) {
                        GlobalLog.Print("ConnectionGroup#" + ValidationHelper.HashString(this) + "::DisableKeepAliveOnConnections() setting KeepAlive to false Connection#" + ValidationHelper.HashString(currentConnection));
                        currentConnection.CloseOnIdle();
                    }
                }
                m_ConnectionList.Clear();
            }
        }


        /// <devdoc>
        ///    <para>         
        ///       Used by the ServicePoint to find a free or new Connection 
        ///       for use in making Requests.
        ///    </para>
        /// </devdoc>
        public Connection FindConnection(string connName) {
            Connection leastbusyConnection = null;
            Connection newConnection = null;
            bool freeConnectionsAvail = false;
            
            GlobalLog.Print("ConnectionGroup::FindConnection [" + connName + "] m_ConnectionList.Count:" + m_ConnectionList.Count.ToString());

            lock (m_ConnectionList) {
                //
                // go through the list of open connections to this service point and pick
                // the first empty one or, if none is empty, pick the least busy one.
                //
                bool completed; 
                do {
                    int minBusyCount = Int32.MaxValue;
                    completed = true; // by default, only once through the outer loop
                    foreach (WeakReference currentConnectionReference in m_ConnectionList) {
                        Connection currentConnection = null;
                        if (currentConnectionReference!=null) {
                            currentConnection = currentConnectionReference.Target as Connection;
                        }
                        //
                        // If the weak reference is alive, see if its not busy, 
                        //  otherwise make sure to remove it from the list
                        //
                        if (currentConnection != null) {
                            GlobalLog.Print("ConnectionGroup::FindConnection currentConnection.BusyCount:" + currentConnection.BusyCount.ToString());
                            if (currentConnection.BusyCount < minBusyCount) {
                                leastbusyConnection = currentConnection;
                                minBusyCount = currentConnection.BusyCount;
                                if (minBusyCount == 0) {
                                    freeConnectionsAvail = true;
                                    break;
                                }
                            }
                        }
                        else {
                            m_ConnectionList.Remove(currentConnectionReference);
                            completed = false;
                            // now start iterating again because we changed the ArrayList
                            break;
                        }
                    }
                } while (!completed);
                

                //
                // If there is NOT a Connection free, then we allocate a new Connection
                //
                if (!freeConnectionsAvail && CurrentConnections < InternalConnectionLimit) {
                    //
                    // If we can create a new connection, then do it,
                    // this may have complications in pipeling because
                    // we may wish to optimize this case by actually
                    // using existing connections, rather than creating new ones
                    //
                    // Note: this implicately results in a this.Associate being called.
                    //
                    
                    GlobalLog.Print("ConnectionGroup::FindConnection [returning new Connection] freeConnectionsAvail:" + freeConnectionsAvail.ToString() + " CurrentConnections:" + CurrentConnections.ToString() + " InternalConnectionLimit:" + InternalConnectionLimit.ToString());

                    newConnection =
                        new Connection(
                            this,
                            m_ServicePoint,
                            m_IPAddress,
                            m_ServicePoint.ProtocolVersion,
                            m_ServicePoint.SupportsPipelining );
                    
                }
                else {
                    //
                    // All connections are busy, use the least busy one
                    //

                    GlobalLog.Print("ConnectionGroup::FindConnection [returning leastbusyConnection] freeConnectionsAvail:" + freeConnectionsAvail.ToString() + " CurrentConnections:" + CurrentConnections.ToString() + " InternalConnectionLimit:" + InternalConnectionLimit.ToString());
                    GlobalLog.Assert(leastbusyConnection != null, "Connect.leastbusyConnection != null", "All connections have BusyCount equal to Int32.MaxValue");                        

                    newConnection = leastbusyConnection;
                }
            }

            return newConnection;
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal void Debug(int requestHash) {
            foreach(WeakReference connectionReference in  m_ConnectionList) {
                Connection connection;

                if ( connectionReference != null && connectionReference.IsAlive) {
                    connection = (Connection)connectionReference.Target;
                } else {
                    connection = null;
                }

                if (connection!=null) {
                    connection.Debug(requestHash);
                }
            }
        }

    } // class ConnectionGroup


} // namespace System.Net
