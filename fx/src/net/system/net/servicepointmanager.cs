//------------------------------------------------------------------------------
// <copyright file="ServicePointManager.cs" company="Microsoft">
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
    using System.Security.Permissions;
    using System.Threading;

    //
    // The ServicePointManager class hands out ServicePoints (may exist or be created
    // as needed) and makes sure they are garbage collected when they expire.
    // The ServicePointManager runs in its own thread so that it never
    //

    /// <include file='doc\ServicePointManager.uex' path='docs/doc[@for="ServicePointManager"]/*' />
    /// <devdoc>
    /// <para>Manages the collection of <see cref='System.Net.ServicePoint'/> instances.</para>
    /// </devdoc>
    public class ServicePointManager {

        /// <include file='doc\ServicePointManager.uex' path='docs/doc[@for="ServicePointManager.DefaultNonPersistentConnectionLimit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The number of non-persistant connections allowed on a <see cref='System.Net.ServicePoint'/>.
        ///    </para>
        /// </devdoc>
        public const int DefaultNonPersistentConnectionLimit = 4;
        /// <include file='doc\ServicePointManager.uex' path='docs/doc[@for="ServicePointManager.DefaultPersistentConnectionLimit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The default number of persistent connections allowed on a <see cref='System.Net.ServicePoint'/>.
        ///    </para>
        /// </devdoc>
        public const int DefaultPersistentConnectionLimit = 2;

        internal static readonly string SpecialConnectGroupName = "/.NET/NetClasses/HttpWebRequest/CONNECT__Group$$/";

        //
        // data  - only statics used
        //

        //
        // s_ServicePointTable - Uri of ServicePoint is the hash key
        // We provide our own comparer function that knows about Uris
        //
        private static Hashtable s_ServicePointTable = new Hashtable(10);
        private static int s_MaxServicePoints = 0;
        private static int s_MaxServicePointIdleTime = 15*60*1000; // TTL: Default value is 15 minutes
        private static Hashtable s_ConfigTable = null;
        private static int s_ConnectionLimit = DefaultPersistentConnectionLimit;

        //
        // s_StrongReferenceServicePointList - holds a cache of links
        //  to service points that are idle, but not expired.  On each
        //  Find through the ServicePoint list, we peak in this list
        //  to see if any are expired.
        //

        private static SortedList s_StrongReferenceServicePointList = new SortedList(10);

        //
        // InternalConnectionLimit -
        //  set/get Connection Limit on demand, checking config beforehand
        //

        private static int InternalConnectionLimit {
            get {
                if (s_ConfigTable == null) {
                    // init config
                    s_ConfigTable = ConfigTable;
                }
                return s_ConnectionLimit;
            }
            set {
                if (s_ConfigTable == null) {
                    // init config
                    s_ConfigTable = ConfigTable;
                }
                s_ConnectionLimit = value;
            }
        }

        [System.Diagnostics.Conditional("DEBUG")]
        internal static void Debug(int requestHash) {
            try {
                foreach(WeakReference servicePointReference in  s_ServicePointTable) {

                    ServicePoint servicePoint;

                    if ( servicePointReference != null && servicePointReference.IsAlive) {
                        servicePoint = (ServicePoint)servicePointReference.Target;
                    }
                    else {
                        servicePoint = null;
                    }

                    if (servicePoint!=null) {
                        servicePoint.Debug(requestHash);
                    }
                }
            }
            catch {
                Console.WriteLine("something threw");
            }
        }

        //
        // ConfigTable -
        // read ConfigTable from Config, or create
        //  a default on failure
        //

        private static Hashtable ConfigTable {
            get {
                if (s_ConfigTable == null) {
                    lock(typeof(ServicePointManager)) {
                        if (s_ConfigTable == null) {
                            Hashtable configTable;
                            configTable = (Hashtable)
                                System.Configuration.ConfigurationSettings.GetConfig("system.net/connectionManagement");

                            if (configTable == null) {
                                configTable = new Hashtable();
                            }

                            // we piggy back loading the ConnectionLimit here
                            if (configTable.ContainsKey("*") ) {
                                int connectionLimit  = (int) configTable["*"];
                                if ( connectionLimit < 1 ) {
                                    connectionLimit = DefaultPersistentConnectionLimit;
                                }
                                s_ConnectionLimit = connectionLimit;
                            }
                            s_ConfigTable = configTable;
                        }
                    }
                }
                return s_ConfigTable;
            }
        }

        //
        // AddIdleServicePoint -
        //
        //  Adds a ServicePoint to our idle list, done, when a servicePoint
        //    detects that its connection count goes to 0.  And its ready to idle.
        //
        internal static void AddIdleServicePoint(ServicePoint servicePoint) {
            GlobalLog.Enter("ServicePointManager::AddIdleServicePoint() servicePoint#" + ValidationHelper.HashString(servicePoint) + " ExpiresAt:" + (servicePoint.ExpiresAt.ToFileTime()).ToString());
            lock (s_StrongReferenceServicePointList) {

                // DateTime.Now wasn't accurate enough to guarantee uniqueness
                while (s_StrongReferenceServicePointList.IndexOfKey(servicePoint.ExpiresAt) != -1) {
                    servicePoint.IncrementExpiresAt();
                }

                GlobalLog.Assert(
                    ! s_StrongReferenceServicePointList.ContainsValue(servicePoint),
                    "s_StrongReferenceServicePointList.ContainsValue(servicePoint)",
                    "s_StrongReferenceServicePointList should not have a value added twice" );

                s_StrongReferenceServicePointList.Add(servicePoint.ExpiresAt, servicePoint);
            }
            GlobalLog.Leave("ServicePointManager::AddIdleServicePoint");
        }

        //
        // RemoveIdleServicePoint -
        //
        //  Removes an Idled ServicePoint when it is resurected to
        //  begin doing connections again, assumes, the service point
        //  has not changed its idle time, since it went idle
        //

        internal static void RemoveIdleServicePoint(ServicePoint servicePoint) {
            GlobalLog.Enter("ServicePointManager::RemoveIdleServicePoint() servicePoint#" + ValidationHelper.HashString(servicePoint) + " ExpiresAt:" + (servicePoint.ExpiresAt.ToFileTime()).ToString());
            try {
                lock (s_StrongReferenceServicePointList) {
                    s_StrongReferenceServicePointList.Remove(servicePoint.ExpiresAt);
                }
            }
            catch (Exception exception) {
                GlobalLog.Print("ServicePointManager::RemoveIdleServicePoint() caught exception accessing s_StrongReferenceServicePointList:" + exception.ToString());
                GlobalLog.Assert(
                    false,
                    "Exception thrown in ServicePointManager::RemoveIdleServicePoint",
                    exception.Message );
            }
            GlobalLog.Leave("ServicePointManager::RemoveIdleServicePoint");
        }

        //
        // UpdateIdleServicePoint -
        //
        // Perform an update if the servicePoint changed its timeout
        //  value, most likly by an external call.  But we need to
        //  do this, otherwise the servicePoint may sit a while,
        //  before we recognize the change
        //

        internal static void UpdateIdleServicePoint(DateTime oldExpiresAt, ServicePoint servicePoint) {
            GlobalLog.Enter("ServicePointManager::UpdateIdleServicePoint() servicePoint#" + ValidationHelper.HashString(servicePoint) + " ExpiresAt:" + (servicePoint.ExpiresAt.ToFileTime()).ToString() + " oldExpiresAt:" + (oldExpiresAt.ToFileTime()).ToString());
            lock (s_StrongReferenceServicePointList) {

                try {
                    // may have already been deleted by scavenge

                    // if we for some reason changed ourselves,
                    // so that we don't expire early as planed,
                    // readd ourselves, so that we expire later
                    //

                    s_StrongReferenceServicePointList.Remove(oldExpiresAt);
                    s_StrongReferenceServicePointList.Add(servicePoint.ExpiresAt, servicePoint);
                }
                catch (Exception exception) {
                    GlobalLog.Print("ServicePointManager::UpdateIdleServicePoint() caught exception accessing s_StrongReferenceServicePointList:" + exception.ToString());
                }
            }
            GlobalLog.Leave("ServicePointManager::UpdateIdleServicePoint");
        }

        //
        // ScavengeIdleServicePoints -
        //
        //  Removes an Idled ServicePoint as it is detected to be
        //  expired and ready to be removed.  This is called fairly
        //  often, so we should be O(1) in the case when no service points
        //  have idled.
        //

        private static void ScavengeIdleServicePoints() {
            DateTime now = DateTime.Now;
            GlobalLog.Enter("ServicePointManager::ScavengeIdleServicePoints() now:" + (now.ToFileTime()).ToString());
            lock (s_StrongReferenceServicePointList) {
                while (s_StrongReferenceServicePointList.Count != 0 )  {
                    DateTime expiresAt = (DateTime) s_StrongReferenceServicePointList.GetKey(0);
                    if ( ServicePoint.InternalTimedOut(now, expiresAt) )  {
                        ServicePoint servicePoint = (ServicePoint)
                            s_StrongReferenceServicePointList.GetByIndex(0);
                        GlobalLog.Print("ServicePointManager::ScavengeIdleServicePoints() servicePoint#" + ValidationHelper.HashString(servicePoint) + " ExpiresAt:" + (servicePoint.ExpiresAt.ToFileTime()).ToString() + " is being removed");
                        s_StrongReferenceServicePointList.RemoveAt(0);
                    }
                    else {
                        // if the first in the list isn't expired, than no one later is either
                        break;
                    }
                }
            }
            GlobalLog.Leave("ServicePointManager::ScavengeIdleServicePoints");
        }


        //
        // constructors
        //

        private ServicePointManager() {
        }

        //
        // accessors
        //

        /// <include file='doc\ServicePointManager.uex' path='docs/doc[@for="ServicePointManager.MaxServicePoints"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the maximum number of <see cref='System.Net.ServicePoint'/> instances that should be maintained at any
        ///    time.</para>
        /// </devdoc>
        public static int MaxServicePoints {
            get {
                return s_MaxServicePoints;
            }
            set {
                if (!ValidationHelper.ValidateRange(value, 0, Int32.MaxValue)) {
                    throw new ArgumentOutOfRangeException("MaxServicePoints");
                }
                s_MaxServicePoints = value;
            }
        }

        /// <include file='doc\ServicePointManager.uex' path='docs/doc[@for="ServicePointManager.DefaultConnectionLimit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int DefaultConnectionLimit {
            get {
                return InternalConnectionLimit;
            }
            set {

                if (value > 0) {
                    InternalConnectionLimit = value;

                }
                else {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.net_toosmall));
                }
            }
        }



        /// <include file='doc\ServicePointManager.uex' path='docs/doc[@for="ServicePointManager.MaxServicePointIdleTime"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the maximum idle time in seconds of a <see cref='System.Net.ServicePoint'/>.</para>
        /// </devdoc>
        public static int MaxServicePointIdleTime {
            get {
                return s_MaxServicePointIdleTime;
            }
            set {
                if ( !ValidationHelper.ValidateRange(value, Timeout.Infinite, Int32.MaxValue)) {
                    throw new ArgumentOutOfRangeException("MaxServicePointIdleTime");
                }
                s_MaxServicePointIdleTime = value;
            }
        }


        //
        // class methods
        //

        //
        // MakeQueryString - Just a short macro to handle creating the query
        //  string that we search for host ports in the host list
        //
        internal static string MakeQueryString(Uri address) {
            return address.Scheme + "://" + address.Host + ":" + address.Port.ToString();
        }

        //
        // FindServicePoint - Query using an Uri string for a given ServerPoint Object
        //

        /// <include file='doc\ServicePointManager.uex' path='docs/doc[@for="ServicePointManager.FindServicePoint"]/*' />
        /// <devdoc>
        /// <para>Finds an existing <see cref='System.Net.ServicePoint'/> or creates a new <see cref='System.Net.ServicePoint'/> to manage communications to the
        ///    specified Uniform Resource Identifier.</para>
        /// </devdoc>
        public static ServicePoint FindServicePoint(Uri address) {
            return FindServicePoint(address, GlobalProxySelection.GetEmptyWebProxy());
        }


        /// <include file='doc\ServicePointManager.uex' path='docs/doc[@for="ServicePointManager.FindServicePoint1"]/*' />
        /// <devdoc>
        /// <para>Finds an existing <see cref='System.Net.ServicePoint'/> or creates a new <see cref='System.Net.ServicePoint'/> to manage communications to the
        ///    specified Uniform Resource Identifier.</para>
        /// </devdoc>
        public static ServicePoint FindServicePoint(string uriString, IWebProxy proxy) {

            Uri uri = new Uri(uriString);

            return FindServicePoint(uri, proxy);
        }

        //
        // FindServicePoint - Query using an Uri for a given server point
        //

        /// <include file='doc\ServicePointManager.uex' path='docs/doc[@for="ServicePointManager.FindServicePoint2"]/*' />
        /// <devdoc>
        /// <para>Findes an existing <see cref='System.Net.ServicePoint'/> or creates a new <see cref='System.Net.ServicePoint'/> to manage communications to the specified <see cref='System.Uri'/>
        /// instance.</para>
        /// </devdoc>
        public static ServicePoint FindServicePoint(Uri address, IWebProxy proxy) {
            if (address==null) {
                throw new ArgumentNullException("address");
            }
            GlobalLog.Enter("ServicePointManager::FindServicePoint() address:" + address.ToString());

            string tempEntry;
            bool isProxyServicePoint = false;

            ScavengeIdleServicePoints();

            //
            // find proxy info, and then switch on proxy
            //
            if (proxy!=null && !proxy.IsBypassed(address)) {
                // use proxy support
                // rework address
                Uri proxyAddress = proxy.GetProxy(address);
                if (proxyAddress.Scheme != Uri.UriSchemeHttps && proxyAddress.Scheme != Uri.UriSchemeHttp) {
                    Exception exception = new NotSupportedException(SR.GetString(SR.net_proxyschemenotsupported, proxyAddress.Scheme));
                    GlobalLog.LeaveException("ServicePointManager::FindServicePoint() proxy has unsupported scheme:" + proxyAddress.Scheme.ToString(), exception);
                    throw exception;
                }
                address = proxyAddress;

                isProxyServicePoint = true;

                //
                // Search for the correct proxy host,
                //  then match its acutal host by using ConnectionGroups
                //  which are located on the actual ServicePoint.
                //

                tempEntry = MakeQueryString(proxyAddress);
            }
            else {
                //
                // Typical Host lookup
                //
                tempEntry = MakeQueryString(address);
            }

            //
            // lookup service point in the table
            //
            ServicePoint servicePoint = null;

            lock (s_ServicePointTable) {
                //
                // once we grab the lock, check if it wasn't already added
                //
                WeakReference servicePointReference =  (WeakReference) s_ServicePointTable[tempEntry];

                if ( servicePointReference != null ) {
                    servicePoint = (ServicePoint)servicePointReference.Target;
                }

                if (servicePoint == null) {
                    //
                    // lookup failure or timeout, we need to create a new ServicePoint
                    //
                    if (s_MaxServicePoints<=0 || s_ServicePointTable.Count<s_MaxServicePoints) {
                        //
                        // Determine Connection Limit
                        //
                        int connectionLimit = InternalConnectionLimit;
                        string schemeHostPort = MakeQueryString(address);

                        if (ConfigTable.ContainsKey(schemeHostPort) ) {
                            connectionLimit = (int) ConfigTable[schemeHostPort];
                        }

                        // Note: we don't check permissions to access proxy.
                        //      Rather, we should protect proxy property from being changed

                        servicePoint = new ServicePoint(address, s_MaxServicePointIdleTime, connectionLimit);
                        servicePointReference = new WeakReference(servicePoint);

                        // only set this when created, donates a proxy, statt Server
                        servicePoint.InternalProxyServicePoint = isProxyServicePoint;

                        s_ServicePointTable[tempEntry] = servicePointReference;
                    }
                    else {
                        Exception exception = new InvalidOperationException(SR.GetString(SR.net_maxsrvpoints));
                        GlobalLog.LeaveException("ServicePointManager::FindServicePoint() reached the limit count:" + s_ServicePointTable.Count.ToString() + " limit:" + s_MaxServicePoints.ToString(), exception);
                        throw exception;
                    }
                }

            } // lock

            GlobalLog.Leave("ServicePointManager::FindServicePoint() servicePoint#" + ValidationHelper.HashString(servicePoint));

            return servicePoint;
        }

    } // class ServicePointManager


} // namespace System.Net
