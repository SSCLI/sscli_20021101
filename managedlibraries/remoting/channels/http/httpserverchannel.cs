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
//==========================================================================
//  File:       HttpServerChannel.cs
//
//  Summary:    Implements a client channel that transmits method calls over HTTP.
//
//  Classes:    public HttpClientChannel
//              internal HttpClientTransportSink
//
//==========================================================================

using System;
using System.Collections;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Messaging;
using System.Runtime.Remoting.Metadata;
using System.Runtime.Remoting.MetadataServices;
using System.Text;
using System.Threading;

using System.Runtime.InteropServices;


namespace System.Runtime.Remoting.Channels.Http
{

    /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel"]/*' />
    public class HttpServerChannel : BaseChannelWithProperties,
                                     IChannelReceiver, IChannelReceiverHook
    {
        private int               _channelPriority = 1;  // priority of channel (default=1)
        private String            _channelName = "http"; // channel name
        private String            _machineName = null;   // machine name
        private int               _port = -1;            // port to listen on
        private ChannelDataStore  _channelData = null;   // channel data

        private String _forcedMachineName = null; // an explicitly configured machine name
        private bool _bUseIpAddress = true; // by default, we'll use the ip address.
        private IPAddress _bindToAddr = IPAddress.Any; // address to bind to.
        private bool _bSuppressChannelData = false; // should we hand out null for our channel data
        
        private IServerChannelSinkProvider _sinkProvider = null;
        private HttpServerTransportSink    _transportSink = null;
        private IServerChannelSink         _sinkChain = null;

        private bool _wantsToListen = true;
        private bool _bHooked = false; // has anyone hooked into the channel?       
        
        
        private TcpListener _tcpListener;
        private Thread      _listenerThread;
        private bool        _bListening = false; // are we listening at the moment?
        private Exception   _startListeningException = null; // if an exception happens on the listener thread when attempting
                                                         //   to start listening, that will get set here.
        private AutoResetEvent  _waitForStartListening = new AutoResetEvent(false);


        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.HttpServerChannel"]/*' />
        public HttpServerChannel() : base()
        {
            SetupMachineName();
            SetupChannel();
        }

        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.HttpServerChannel1"]/*' />
        public HttpServerChannel(int port) : base()
        {
            _port = port;
            SetupMachineName();
            SetupChannel();
        } // HttpServerChannel()
    
        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.HttpServerChannel2"]/*' />
        public HttpServerChannel(String name, int port) : base()
        {
            _channelName = name;
            _port = port;
            SetupMachineName();
            SetupChannel();
        } // HttpServerChannel()

        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.HttpServerChannel3"]/*' />
        public HttpServerChannel(String name, int port, IServerChannelSinkProvider sinkProvider) : base()
        {
            _channelName = name;
            _port = port;
            _sinkProvider = sinkProvider;
            SetupMachineName();
            SetupChannel();
        } // HttpServerChannel()


        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.HttpServerChannel4"]/*' />
        public HttpServerChannel(IDictionary properties, IServerChannelSinkProvider sinkProvider) : base()
        {               
            if (properties != null)
            {
                foreach (DictionaryEntry entry in properties)
                {
                    switch ((String)entry.Key)
                    {
                    case "name": _channelName = (String)entry.Value; break; 
                    case "bindTo": _bindToAddr = IPAddress.Parse((String)entry.Value); break;
                    case "listen": _wantsToListen = Convert.ToBoolean(entry.Value); break;   
                    case "machineName": _forcedMachineName = (String)entry.Value; break;
                    case "port": _port = Convert.ToInt32(entry.Value); break;
                    case "priority": _channelPriority = Convert.ToInt32(entry.Value); break;
                    case "suppressChannelData": _bSuppressChannelData = Convert.ToBoolean(entry.Value); break;
                    case "useIpAddress": _bUseIpAddress = Convert.ToBoolean(entry.Value); break;
                
                    default: 
                         throw new ArgumentException(
                            String.Format(
                                CoreChannel.GetResourceString(
                                    "Remoting_Channels_BadCtorArgs"),
                                entry.Key));
                    }
                }
            }

            _sinkProvider = sinkProvider;
            SetupMachineName();
            SetupChannel();
        } // HttpServerChannel


        private void SetupMachineName()
        {
            if (_forcedMachineName != null)
            {
                // an explicitly configured machine name was used
                _machineName = CoreChannel.DecodeMachineName(_forcedMachineName);
            }
            else
            {
                if (!_bUseIpAddress)
                    _machineName = CoreChannel.GetMachineName();
                else
                {
                    if (_bindToAddr == IPAddress.Any)
                        _machineName = CoreChannel.GetMachineIp();
                    else
                        _machineName = _bindToAddr.ToString();
                }
            }
        } // SetupMachineName


        private void SetupChannel()
        {   
            // set channel data
            // (These get changed inside of StartListening(), in the case where the listen
            //   port is 0, because we can't determine the port number until after the
            //   TcpListener starts.)

            _channelData = new ChannelDataStore(null);
            if (_port > 0)
            {
                String channelUri = GetChannelUri();
                _channelData.ChannelUris = new String[1];
                _channelData.ChannelUris[0] = channelUri;

                _wantsToListen = false;
            }

            // set default provider (soap formatter) if no provider has been set
            if (_sinkProvider == null)
                _sinkProvider = CreateDefaultServerProviderChain();

            CoreChannel.CollectChannelDataFromServerSinkProviders(_channelData, _sinkProvider);

            // construct sink chain
            _sinkChain = ChannelServices.CreateServerChannelSinkChain(_sinkProvider, this);
            _transportSink = new HttpServerTransportSink(_sinkChain);

            // set sink properties on base class, so that properties will be chained.
            SinksWithProperties = _sinkChain;
            
            if (_port >= 0)
            {
                // Open a TCP port and create a thread to start listening
                _tcpListener = new TcpListener(_bindToAddr, _port);
                ThreadStart t = new ThreadStart(this.Listen);
                _listenerThread = new Thread(t);
                _listenerThread.IsBackground = true;

                // Wait for thread to spin up
                StartListening(null);
            }
        } // SetupChannel


        private IServerChannelSinkProvider CreateDefaultServerProviderChain()
        {
            IServerChannelSinkProvider chain = new SdlChannelSinkProvider();            
            IServerChannelSinkProvider sink = chain;
            
            sink.Next = new SoapServerFormatterSinkProvider();
            sink = sink.Next;
            sink.Next = new BinaryServerFormatterSinkProvider();
            
            return chain;
        } // CreateDefaultServerProviderChain


        //
        // IChannel implementation
        //

        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.ChannelPriority"]/*' />
        public int ChannelPriority
        {
            get { return _channelPriority; }
        }

        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.ChannelName"]/*' />
        public String ChannelName
        {
            get { return _channelName; }
        }

        // returns channelURI and places object uri into out parameter
        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.Parse"]/*' />
        public String Parse(String url, out String objectURI)
        {            
            return HttpChannelHelper.ParseURL(url, out objectURI);
        } // Parse

        //
        // end of IChannel implementation
        //


        //
        // IChannelReceiver implementation
        //

        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.ChannelData"]/*' />
        public Object ChannelData
        {
            get
            {
                if (!_bSuppressChannelData && 
                        (_bListening || _bHooked))
                {
                    return _channelData;
                }
                else
                {
                    return null;
                }
            }
        } // ChannelData


        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.GetChannelUri"]/*' />
        public String GetChannelUri()
        {
            if ((_channelData != null) && (_channelData.ChannelUris != null))
            {
                return _channelData.ChannelUris[0];
            }
            else
            {
                return "http://" + _machineName + ":" + _port;
            }
        } // GetChannelURI


        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.GetUrlsForUri"]/*' />
        public virtual String[] GetUrlsForUri(String objectUri)
        {
            String[] retVal = new String[1];

            if (!objectUri.StartsWith("/"))
                objectUri = "/" + objectUri;
            retVal[0] = GetChannelUri() + objectUri;

            return retVal;
        } // GetURLsforURI


        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.StartListening"]/*' />
        public void StartListening(Object data)
        {
            InternalRemotingServices.RemotingTrace("HttpChannel.StartListening");

            if (_port >= 0)
            {
                if (_listenerThread.IsAlive == false)
                {
                    _listenerThread.Start();
                    _waitForStartListening.WaitOne(); // listener thread will signal this after starting TcpListener

                    if (_startListeningException != null)
                    {
                        // An exception happened when we tried to start listening (such as "socket already in use)
                        Exception e = _startListeningException;
                        _startListeningException = null;
                        throw e;
                    }

                    _bListening = true;

                    // get new port assignment if a port of 0 was used to auto-select a port
                    if (_port == 0)
                    {
                        _port = ((IPEndPoint)_tcpListener.LocalEndpoint).Port;
                        
                        if (_channelData != null)
                        {
                            String channelUri = GetChannelUri();
                            _channelData.ChannelUris = new String[1];
                            _channelData.ChannelUris[0] = channelUri;
                        }
                    }
                }
            }
        } // StartListening


        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.StopListening"]/*' />
        public void StopListening(Object data)
        {
            InternalRemotingServices.RemotingTrace("HTTPChannel.StopListening");

            if (_port > 0)
            {
                _bListening = false;
            
                // Ask the TCP listener to stop listening on the port
                if(null != _tcpListener)
                {
                    _tcpListener.Stop();
                }
            }
        } // StopListening

        //
        // end of IChannelReceiver implementation
        //

        //
        // IChannelReceiverHook implementation
        //

        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.ChannelScheme"]/*' />
        public String ChannelScheme { get { return "http"; } }

        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.WantsToListen"]/*' />
        public bool WantsToListen 
        { 
            get { return _wantsToListen; } 
            set { _wantsToListen = value; }
        } // WantsToListen

        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.ChannelSinkChain"]/*' />
        public IServerChannelSink ChannelSinkChain { get { return _sinkChain; } }

        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.AddHookChannelUri"]/*' />
        public void AddHookChannelUri(String channelUri)
        {
            if (_channelData.ChannelUris != null)
            {
                throw new RemotingException(
                    CoreChannel.GetResourceString("Remoting_Http_LimitListenerOfOne"));
            }
            else
            {
                // replace machine name with explicitly configured
                //   machine name or ip address if necessary
                if (_forcedMachineName != null)
                {
                    channelUri = 
                        HttpChannelHelper.ReplaceMachineNameWithThisString(channelUri, _forcedMachineName);
                }
                else
                if (_bUseIpAddress)
                {
                    channelUri = 
                        HttpChannelHelper.ReplaceMachineNameWithThisString(channelUri, CoreChannel.GetMachineIp());
                }
            
                _channelData.ChannelUris = new String[] { channelUri };
                _wantsToListen = false;
                _bHooked = true;
            }
        } // AddHookChannelUri
        
        
        //
        // end of IChannelReceiverHook implementation
        //


        //
        // Server helpers
        //

        // Thread for listening
        void Listen()
        {
            bool bOkToListen = false;
        
            try
            {
                _tcpListener.Start();
                bOkToListen = true;
            }
            catch (Exception e)
            {
                _startListeningException = e;
            }                

            _waitForStartListening.Set(); // allow main thread to continue now that we have tried to start the socket                

            InternalRemotingServices.RemotingTrace( "Waiting to Accept the Socket on Port: " + _port);

            //
            // Wait for an incoming socket
            //
            Socket socket;
            
            while (bOkToListen)
            {
                InternalRemotingServices.RemotingTrace("TCPChannel::Listen - tcpListen.Pending() == true");                

                try
                {
                    socket = _tcpListener.AcceptSocket();

                    if (socket == null)
                    {
                        throw new RemotingException(
                            String.Format(
                                CoreChannel.GetResourceString("Remoting_Socket_Accept"),
                                Marshal.GetLastWin32Error().ToString()));
                    }
                    else
                    {
                        // disable nagle delay
                        socket.SetSocketOption(SocketOptionLevel.Tcp, SocketOptionName.NoDelay, 1);

                        // set linger option
                        LingerOption lingerOption = new LingerOption(true, 3);
                        socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.Linger, lingerOption);
                    
                        HttpServerSocketHandler streamManager = new HttpServerSocketHandler(socket, CoreChannel.RequestQueue);
                        streamManager.DataArrivedCallback = new WaitCallback(_transportSink.ServiceRequest);
                        streamManager.BeginReadMessage();               
                    }
                } 
                catch (Exception e)
                {
                    if (!_bListening)
                    {
                        // We called Stop() on the tcp listener, so gracefully exit.
                        bOkToListen = false;                        
                    }
                    else
                    {
                        // we want the exception to show up as unhandled since this
                        //   is an unexpected failure.
                        if (!(e is SocketException))
                        {
                            //throw;                   
                        }
                    }
                }
            } // while (bOkToListen)
        }


        //
        // Support for properties (through BaseChannelWithProperties)
        //

        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.this"]/*' />
        public override Object this[Object key]
        {
            get { return null; }
        
            set
            {
            }
        } // this[]
    
        /// <include file='doc\HttpServerChannel.uex' path='docs/doc[@for="HttpServerChannel.Keys"]/*' />
        public override ICollection Keys 
        {
            get
            {
                return new ArrayList(); 
            }
        }

    } // HttpServerChannel

    


    internal class HttpServerTransportSink : IServerChannelSink
    {
        private static String s_serverHeader =
            "MS .NET Remoting, MS .NET CLR " + System.Environment.Version.ToString();
    
        // sink state
        private IServerChannelSink _nextSink;
        

        public HttpServerTransportSink(IServerChannelSink nextSink)
        {
            _nextSink = nextSink;
        } // IServerChannelSink
        
    
        internal void ServiceRequest(Object state)
        {        
            HttpServerSocketHandler streamManager = (HttpServerSocketHandler)state;

            ITransportHeaders headers = streamManager.ReadHeaders();
            Stream requestStream = streamManager.GetRequestStream();

            // process request
            ServerChannelSinkStack sinkStack = new ServerChannelSinkStack();
            sinkStack.Push(this, streamManager);

            IMessage responseMessage;
            ITransportHeaders responseHeaders;
            Stream responseStream;

            ServerProcessing processing = 
                _nextSink.ProcessMessage(sinkStack, null, headers, requestStream, 
                                         out responseMessage,
                                         out responseHeaders, out responseStream);

            // handle response
            switch (processing)
            {                    

            case ServerProcessing.Complete:
            {
                // Send the response. Call completed synchronously.
                sinkStack.Pop(this);
                streamManager.SendResponse(responseStream, "200", "OK", responseHeaders);
                break;
            } // case ServerProcessing.Complete
            
            case ServerProcessing.OneWay:
            {
                // Just send back a 200 OK
                streamManager.SendResponse(null, "202", "Accepted", responseHeaders);
                break;
            } // case ServerProcessing.OneWay

            case ServerProcessing.Async:
            {
                sinkStack.StoreAndDispatch(this, streamManager);
                break;
            }// case ServerProcessing.Async

            } // switch (processing)


            // async processing will take care if handling this later
            if (processing != ServerProcessing.Async)
            {
                if (streamManager.CanServiceAnotherRequest())
                    streamManager.BeginReadMessage();
                else
                    streamManager.Close();
            }
            
        } // ServiceRequest
      



        //
        // IServerChannelSink implementation
        //

        public ServerProcessing ProcessMessage(IServerChannelSinkStack sinkStack,
            IMessage requestMsg,
            ITransportHeaders requestHeaders, Stream requestStream,
            out IMessage responseMsg, out ITransportHeaders responseHeaders,
            out Stream responseStream)
        {
            // NOTE: This doesn't have to be implemented because the server transport
            //   sink is always first.
            throw new NotSupportedException();
        } // ProcessMessage
           

        public void AsyncProcessResponse(IServerResponseChannelSinkStack sinkStack, Object state,
                                         IMessage msg, ITransportHeaders headers, Stream stream)                 
        {
            HttpServerSocketHandler streamManager = null;

            streamManager = (HttpServerSocketHandler)state;

            // send the response
            streamManager.SendResponse(stream, "200", "OK", headers);

            if (streamManager.CanServiceAnotherRequest())
                streamManager.BeginReadMessage();
            else
                streamManager.Close();            
        } // AsyncProcessResponse


        public Stream GetResponseStream(IServerResponseChannelSinkStack sinkStack, Object state,
                                        IMessage msg, ITransportHeaders headers)
        {
            HttpServerSocketHandler streamManager = (HttpServerSocketHandler)state;

            if (streamManager.AllowChunkedResponse)
                return streamManager.GetResponseStream("200", "OK", headers);
            else
                return null;
        } // GetResponseStream


        public IServerChannelSink NextChannelSink
        {
            get { return _nextSink; }
        }


        public IDictionary Properties
        {
            get { return null; }
        } // Properties
        
        //
        // end of IServerChannelSink implementation
        //


        internal static String ServerHeader
        {
            get { return s_serverHeader; }
        }

        
        
    } // HttpServerTransportSink



    internal class ErrorMessage: IMethodCallMessage
    {

        // IMessage
        public IDictionary Properties     { get{ return null;} }

        // IMethodMessage
        public String Uri                      { get{ return m_URI; } }
        public String MethodName               { get{ return m_MethodName; }}
        public String TypeName                 { get{ return m_TypeName; } }
        public Object MethodSignature          { get { return m_MethodSignature;} }
        public MethodBase MethodBase           { get { return null; }}
        public int ArgCount                    { get { return m_ArgCount;} }
        public String GetArgName(int index)    { return m_ArgName; }
        public Object GetArg(int argNum)       { return null;}
        public Object[] Args                   { get { return null;} }

        public bool HasVarArgs                 { get { return false;} }
        public LogicalCallContext LogicalCallContext { get { return null; }}


        // IMethodCallMessage
        public int InArgCount                  { get { return m_ArgCount;} }
        public String GetInArgName(int index)   { return null; }
        public Object GetInArg(int argNum)      { return null;}
        public Object[] InArgs                { get { return null; }}

        String m_URI = "Exception";
        String m_MethodName = "Unknown";
        String m_TypeName = "Unknown";
        Object m_MethodSignature = null;
        int m_ArgCount = 0;
        String m_ArgName = "Unknown";
    } // ErrorMessage




} // namespace System.Runtime.Remoting.Channels.Http
