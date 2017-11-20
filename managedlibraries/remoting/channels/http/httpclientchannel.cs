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
//  File:       HttpClientChannel.cs
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
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Messaging;
using System.Threading;
using System.Globalization;


namespace System.Runtime.Remoting.Channels.Http
{



    /// <include file='doc\HttpClientChannel.uex' path='docs/doc[@for="HttpClientChannel"]/*' />
    public class HttpClientChannel : BaseChannelWithProperties, IChannelSender
    {
        // Property Keys (purposely all lower-case)
        private const String ProxyNameKey = "proxyname";
        private const String ProxyPortKey = "proxyport";

        // If above keys get modified be sure to modify, the KeySet property on this
        // class.
        private static ICollection s_keySet = null;
        
    
        // Settings
        private int    _channelPriority = 1;  // channel priority
        private String _channelName = "http"; // channel name

        // Proxy settings (_proxyObject gets recreated when _proxyName and _proxyPort are updated)
        private IWebProxy _proxyObject = null; // proxy object for request, can be overridden in transport sink
        private String    _proxyName = null;
        private int       _proxyPort = -1;

        private int _clientConnectionLimit = 0; // bump connection limit to at least this number (only meaningful if > 0)
        private bool _bUseDefaultCredentials = false; // should default credentials be used?
        
        private IClientChannelSinkProvider _sinkProvider = null; // sink chain provider                       


        /// <include file='doc\HttpClientChannel.uex' path='docs/doc[@for="HttpClientChannel.HttpClientChannel"]/*' />
        public HttpClientChannel()
        {
            SetupChannel();
        } // HttpClientChannel()


        /// <include file='doc\HttpClientChannel.uex' path='docs/doc[@for="HttpClientChannel.HttpClientChannel1"]/*' />
        public HttpClientChannel(String name, IClientChannelSinkProvider sinkProvider)
        {
            _channelName = name;
            _sinkProvider = sinkProvider;

            SetupChannel();
        } // HttpClientChannel(IClientChannelSinkProvider sinkProvider)
       

        // constructor used by config file
        /// <include file='doc\HttpClientChannel.uex' path='docs/doc[@for="HttpClientChannel.HttpClientChannel2"]/*' />
        public HttpClientChannel(IDictionary properties, IClientChannelSinkProvider sinkProvider)
        {
            if (properties != null)
            {
                foreach (DictionaryEntry entry in properties)
                {
                    switch ((String)entry.Key)
                    {
                    case "name": _channelName = (String)entry.Value; break;
                    case "priority": _channelPriority = Convert.ToInt32(entry.Value); break;

                    case "proxyName": this["proxyName"] = entry.Value; break;
                    case "proxyPort": this["proxyPort"] = entry.Value; break;

                    case "clientConnectionLimit": 
                    {
                        _clientConnectionLimit = Convert.ToInt32(entry.Value); 
                        break;
                    }

                    case "useDefaultCredentials":
                    {
                        _bUseDefaultCredentials = Convert.ToBoolean(entry.Value);
                        break;
                    }

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

            SetupChannel();
        } // HttpClientChannel
        

        private void SetupChannel()
        {
            if (_sinkProvider != null)
            {
                CoreChannel.AppendProviderToClientProviderChain(
                    _sinkProvider, new HttpClientTransportSinkProvider());                                                
            }
            else
                _sinkProvider = CreateDefaultClientProviderChain();
                
        
            // proxy might have been created by setting proxyname/port in constructor with dictionary
            if (_proxyObject == null) 
            {
                // In this case, try to use the default proxy settings.
                WebProxy defaultProxy = WebProxy.GetDefaultProxy();
                if (defaultProxy != null)
                {
                    Uri address = defaultProxy.Address;
                    if (address != null)
                    {
                        _proxyName = address.Host;
                        _proxyPort = address.Port;
                    }
                }
                UpdateProxy();
            }
        } // SetupChannel()



        //
        // IChannel implementation
        //

        /// <include file='doc\HttpClientChannel.uex' path='docs/doc[@for="HttpClientChannel.ChannelPriority"]/*' />
        public int ChannelPriority
        {
            get { return _channelPriority; }    
        }

        /// <include file='doc\HttpClientChannel.uex' path='docs/doc[@for="HttpClientChannel.ChannelName"]/*' />
        public String ChannelName
        {
            get { return _channelName; }
        }

        // returns channelURI and places object uri into out parameter
        /// <include file='doc\HttpClientChannel.uex' path='docs/doc[@for="HttpClientChannel.Parse"]/*' />
        public String Parse(String url, out String objectURI)
        {            
            return HttpChannelHelper.ParseURL(url, out objectURI);
        } // Parse

        //
        // end of IChannel implementation
        // 



        //
        // IChannelSender implementation
        //

        /// <include file='doc\HttpClientChannel.uex' path='docs/doc[@for="HttpClientChannel.CreateMessageSink"]/*' />
        public virtual IMessageSink CreateMessageSink(String url, Object remoteChannelData, out String objectURI)
        {
            // Set the out parameters
            objectURI = null;
            String channelURI = null;

            
            if (url != null) // Is this a well known object?
            {
                // Parse returns null if this is not one of our url's
                channelURI = Parse(url, out objectURI);
            }
            else // determine if we want to connect based on the channel data
            {
                if (remoteChannelData != null)
                {
                    if (remoteChannelData is IChannelDataStore)
                    {
                        IChannelDataStore cds = (IChannelDataStore)remoteChannelData;

                        // see if this is an http uri
                        String simpleChannelUri = Parse(cds.ChannelUris[0], out objectURI);
                        if (simpleChannelUri != null)
                            channelURI = cds.ChannelUris[0];
                    }
                }
            }

            if (channelURI != null)
            {
                if (url == null)
                    url = channelURI;

                if (_clientConnectionLimit > 0)
                {
                    ServicePoint sp = ServicePointManager.FindServicePoint(new Uri(channelURI));
                    if (sp.ConnectionLimit < _clientConnectionLimit)
                        sp.ConnectionLimit = _clientConnectionLimit;
                }

                // This will return null if one of the sink providers decides it doesn't
                // want to allow (or can't provide) a connection through this channel.
                IClientChannelSink sink = _sinkProvider.CreateSink(this, url, remoteChannelData);
                
                // return sink after making sure that it implements IMessageSink
                IMessageSink msgSink = sink as IMessageSink;
                if ((sink != null) && (msgSink == null))
                {
                    throw new RemotingException(
                        CoreChannel.GetResourceString("Remoting_Channels_ChannelSinkNotMsgSink"));
                }
                    
                return msgSink;
            }

            return null;
        } // CreateMessageSink


        //
        // end of IChannelSender implementation
        //


        private IClientChannelSinkProvider CreateDefaultClientProviderChain()
        {
            IClientChannelSinkProvider chain = new SoapClientFormatterSinkProvider();            
            IClientChannelSinkProvider sink = chain;
            
            sink.Next = new HttpClientTransportSinkProvider();
            
            return chain;
        } // CreateDefaultClientProviderChain
        


        //
        // Support for properties (through BaseChannelSinkWithProperties)
        //
        
        /// <include file='doc\HttpClientChannel.uex' path='docs/doc[@for="HttpClientChannel.this"]/*' />
        public override Object this[Object key]
        {
            get
            {
                String keyStr = key as String;
                if (keyStr == null)
                    return null;
            
                switch (keyStr.ToLower(CultureInfo.InvariantCulture))
                {
                    case ProxyNameKey: return _proxyName;
                    case ProxyPortKey: return _proxyPort;
                } // switch (keyStr.ToLower(CultureInfo.InvariantCulture))

                return null;
            }

            set
            {
                String keyStr = key as String;
                if (keyStr == null)
                    return;
    
                switch (keyStr.ToLower(CultureInfo.InvariantCulture))
                {
                    case ProxyNameKey: _proxyName = (String)value; UpdateProxy(); break;
                    case ProxyPortKey: _proxyPort = Convert.ToInt32(value); UpdateProxy(); break;                        
                } // switch (keyStr.ToLower(CultureInfo.InvariantCulture))
            }
        } // this[]   


        /// <include file='doc\HttpClientChannel.uex' path='docs/doc[@for="HttpClientChannel.Keys"]/*' />
        public override ICollection Keys
        {
            get
            {
                if (s_keySet == null)
                {
                    // Don't need to synchronize. Doesn't matter if the list gets
                    // generated twice.
                    ArrayList keys = new ArrayList(2);
                    keys.Add(ProxyNameKey);
                    keys.Add(ProxyPortKey);
                    
                    s_keySet = keys;
                }

                return s_keySet;
            }
        } // Keys


        //
        // end of Support for properties
        //


        //
        // Helper functions for processing settings and properties
        //

        // Called to recreate proxy object whenever the proxy name or port is changed.
        private void UpdateProxy()
        {
            if ((_proxyName != null) && (_proxyName.Length > 0) &&                
                (_proxyPort > 0))
            {
                WebProxy proxy = new WebProxy(_proxyName, _proxyPort);
                
                // disable proxy use when the host is local. i.e. without periods
                proxy.BypassProxyOnLocal = true;

                // setup bypasslist to include local ip address
                String[] bypassList = new String[]{ CoreChannel.GetMachineIp() };
                proxy.BypassList = bypassList;

                _proxyObject = proxy;
            }
            else
            {
                _proxyObject = new WebProxy();
            }
        } // UpdateProxy

        //
        // end of Helper functions for processing settings and properties
        //

        //
        // Methods to access properties (internals are for use by the transport sink)     
        //

        internal IWebProxy ProxyObject { get { return _proxyObject; } }
        internal bool UseDefaultCredentials { get { return _bUseDefaultCredentials; } }

        //
        // end of Methods to access properties
        //

    } // class HttpClientChannel




    internal class HttpClientTransportSinkProvider : IClientChannelSinkProvider
    {
        internal HttpClientTransportSinkProvider()
        {
        }    
   
        public IClientChannelSink CreateSink(IChannelSender channel, String url, 
                                             Object remoteChannelData)
        {
            // url is set to the channel uri in CreateMessageSink        
            return new HttpClientTransportSink((HttpClientChannel)channel, url);
        }

        public IClientChannelSinkProvider Next
        {
            get { return null; }
            set { throw new NotSupportedException(); }
        }
    } // class HttpClientTransportSinkProvider




    // transport sender sink used by HttpClientChannel
    internal class HttpClientTransportSink : BaseChannelSinkWithProperties, IClientChannelSink
    {
        private const String s_defaultVerb = "POST";

        private static String s_userAgent =
            "Mozilla/4.0+(compatible; MSIE 6.0; Windows " + 
            "; MS .NET Remoting; MS .NET CLR " + System.Environment.Version.ToString() + " )";
        
        // Property keys (purposely all lower-case)
        private const String UserNameKey = "username";
        private const String PasswordKey = "password";
        private const String DomainKey = "domain";
        private const String PreAuthenticateKey = "preauthenticate";
        private const String CredentialsKey = "credentials";
        private const String ClientCertificatesKey = "clientcertificates";
        private const String ProxyNameKey = "proxyname";
        private const String ProxyPortKey = "proxyport";
        private const String TimeoutKey = "timeout";
        private const String AllowAutoRedirectKey = "allowautoredirect";

        // If above keys get modified be sure to modify, the KeySet property on this
        // class.
        private static ICollection s_keySet = null;

        // Property values
        private String _securityUserName = null;
        private String _securityPassword = null;
        private String _securityDomain = null;
        private bool   _bSecurityPreAuthenticate = false;
        private ICredentials _credentials = null; // this overrides all of the other security settings

        private int  _timeout = System.Threading.Timeout.Infinite; // timeout value in milliseconds (only used if greater than 0)
        private bool _bAllowAutoRedirect = false;

        // Proxy settings (_proxyObject gets recreated when _proxyName and _proxyPort are updated)
        private IWebProxy _proxyObject = null; // overrides channel proxy object if non-null
        private String    _proxyName = null;
        private int       _proxyPort = -1;

        // Other members
        private HttpClientChannel _channel; // channel that created this sink
        private String            _channelURI; // complete url to remote object        

        // settings
        private bool _useChunked = false; // FUTURE: Consider enabling chunked after implementing a method to avoid the perf hit on small requests.
        private bool _useKeepAlive = true;


        internal HttpClientTransportSink(HttpClientChannel channel, String channelURI) : base()
        {
            _channel = channel;
        
            _channelURI = channelURI;
            
            // make sure channel uri doesn't end with a slash.
            if (_channelURI.EndsWith("/"))
                _channelURI = _channelURI.Substring(0, _channelURI.Length - 1);
                
        } // HttpClientTransportSink
        

        public void ProcessMessage(IMessage msg,
                                   ITransportHeaders requestHeaders, Stream requestStream,
                                   out ITransportHeaders responseHeaders, out Stream responseStream)
        {

            InternalRemotingServices.RemotingTrace("HttpTransportSenderSink::ProcessMessage");

            HttpWebRequest httpWebRequest = ProcessAndSend(msg, requestHeaders, requestStream);

            // receive server response
            HttpWebResponse response = null;
            try
            {
                response = (HttpWebResponse)httpWebRequest.GetResponse();
            }
            catch (WebException webException)
            {
                ProcessResponseException(webException, out response);
            }
    
            ReceiveAndProcess(response, out responseHeaders, out responseStream);
        } // ProcessMessage


        public void AsyncProcessRequest(IClientChannelSinkStack sinkStack, IMessage msg,
                                        ITransportHeaders headers, Stream stream)
        {
            HttpWebRequest httpWebRequest = ProcessAndSend(msg, headers, stream);

            sinkStack.Push(this, httpWebRequest);
            
            httpWebRequest.BeginGetResponse(
                new AsyncCallback(this.OnHttpMessageReturn), 
                sinkStack);
        } // AsyncProcessRequest


        private void OnHttpMessageReturn(IAsyncResult ar)
        {                    
            IClientChannelSinkStack sinkStack = (IClientChannelSinkStack)ar.AsyncState;

            HttpWebResponse response = null;

            HttpWebRequest httpWebRequest = (HttpWebRequest)sinkStack.Pop(this);
            try
            {
                response = (HttpWebResponse) httpWebRequest.EndGetResponse(ar);
            }
            catch (WebException webException)
            {
                ProcessResponseException(webException, out response);
            }

            // process incoming response
            ITransportHeaders responseHeaders;
            Stream responseStream;
            ReceiveAndProcess(response, out responseHeaders, out responseStream);

            // call down the sink chain
            sinkStack.AsyncProcessResponse(responseHeaders, responseStream);
        } // OnHttpMessageReturn


        private void ProcessResponseException(WebException webException, out HttpWebResponse response)
        {
            // if a timeout occurred throw a RemotingTimeoutException
            if (webException.Status == WebExceptionStatus.Timeout)
                throw new RemotingTimeoutException(
                    CoreChannel.GetResourceString(
                        "Remoting_Channels_RequestTimedOut"),
                    webException);
        
            response = webException.Response as HttpWebResponse;
            if ((response == null))
                throw webException;                
                
            // if server error (500-599 continue with processing the soap fault);
            //   otherwise, rethrow the exception.

            int statusCode = (int)(response.StatusCode);
            if ((statusCode < 500) || 
                (statusCode > 599))
            {
                throw webException;
            }   
        } // ProcessResponseException


        public void AsyncProcessResponse(IClientResponseChannelSinkStack sinkStack, Object state,
                                         ITransportHeaders headers, Stream stream)
        {
            // We don't have to implement this since we are always last in the chain.
        } // AsyncProcessRequest
        

        
        public Stream GetRequestStream(IMessage msg, ITransportHeaders headers)
        {
            return null; 
        } // GetRequestStream


        public IClientChannelSink NextChannelSink
        {
            get { return null; }
        }
    


        private HttpWebRequest SetupWebRequest(IMessage msg, ITransportHeaders headers)
        {
            IMethodCallMessage mcMsg = msg as IMethodCallMessage;        

            String msgUri = (String)headers[CommonTransportKeys.RequestUri];
            InternalRemotingServices.RemotingTrace("HttpClientChannel::SetupWebRequest Message uri is " + msgUri);

            if (msgUri == null)
            {
                if (mcMsg != null)
                    msgUri = mcMsg.Uri;
                else
                    msgUri = (String)msg.Properties["__Uri"];
            }
            
            String fullPath;
            if (HttpChannelHelper.StartsWithHttp(msgUri) != -1)
            {
                // this is the full path
                fullPath = msgUri;
            }
            else
            {
                // this is not the full path (_channelURI never has trailing slash)
                if (!msgUri.StartsWith("/"))
                    msgUri = "/" + msgUri;
             
                fullPath = _channelURI + msgUri;                
            }
            InternalRemotingServices.RemotingTrace("HttpClientChannel::SetupWebRequest FullPath " + fullPath);

            // based on headers, initialize the network stream

            String verb = (String)headers["__RequestVerb"];
            if (verb == null)
                verb = s_defaultVerb;            

            HttpWebRequest httpWebRequest = (HttpWebRequest)WebRequest.Create(fullPath);
            httpWebRequest.AllowAutoRedirect = _bAllowAutoRedirect;
            httpWebRequest.Method = verb;
            httpWebRequest.SendChunked = _useChunked; 
            httpWebRequest.KeepAlive = _useKeepAlive;
            httpWebRequest.Pipelined = false;
            httpWebRequest.UserAgent = s_userAgent;
            httpWebRequest.Timeout = _timeout;

            // see if we should use a proxy object
            IWebProxy proxy = _proxyObject;
            if (proxy == null) // use channel proxy if one hasn't been explicity set for this sink
                proxy = _channel.ProxyObject;
            if (proxy != null)
                httpWebRequest.Proxy = proxy; 
                                            
            // see if security should be used
            //   order of applying credentials is:
            //   1. check for explicitly set credentials
            //   2. else check for explicitly set username, password, domain
            //   3. else use default credentials if channel is configured to do so.
            if (_credentials != null)
            {
                httpWebRequest.Credentials = _credentials;
                httpWebRequest.PreAuthenticate = _bSecurityPreAuthenticate;
            }
            else
            if (_securityUserName != null)
            {
                if (_securityDomain == null)
                    httpWebRequest.Credentials = new NetworkCredential(_securityUserName, _securityPassword);
                else
                    httpWebRequest.Credentials = new NetworkCredential(_securityUserName, _securityPassword, _securityDomain);
                httpWebRequest.PreAuthenticate = _bSecurityPreAuthenticate;
            }
            else
            if (_channel.UseDefaultCredentials)
            {
                httpWebRequest.Credentials = CredentialCache.DefaultCredentials;
                httpWebRequest.PreAuthenticate = _bSecurityPreAuthenticate;
            }


            InternalRemotingServices.RemotingTrace("HttpClientTransportSink::SetupWebRequest - Get Http Request Headers");

            // add headers
            foreach (DictionaryEntry header in headers)
            {
                String key = header.Key as String;
                
                // if header name starts with "__", it is a special value that shouldn't be
                //   actually sent out.
                if ((key != null) && !key.StartsWith("__")) 
                {
                    if (key.Equals("Content-Type"))
                        httpWebRequest.ContentType = header.Value.ToString();
                    else
                        httpWebRequest.Headers[key] = header.Value.ToString();
                }
            }

            return httpWebRequest;
        } // SetupWebRequest


        private HttpWebRequest ProcessAndSend(IMessage msg, ITransportHeaders headers, 
                                              Stream inputStream)
        {      
            // If the stream is seekable, we can retry once on a failure to write.
            long initialPosition = 0;
            bool bCanSeek = false;
            if (inputStream != null)
            {
                bCanSeek = inputStream.CanSeek;
                if (bCanSeek)
                    initialPosition = inputStream.Position;
            }
        
            HttpWebRequest httpWebRequest = null;
            Stream writeStream = null;
            try
            {
                httpWebRequest = SetupWebRequest(msg, headers);

                if (inputStream != null)
                {
                    if (!_useChunked)
                        httpWebRequest.ContentLength = (int)inputStream.Length;
          
                    writeStream = httpWebRequest.GetRequestStream();
                    StreamHelper.CopyStream(inputStream, writeStream);                  
                }
            }    
            catch (Exception)
            {
                // try to send one more time if possible
                if (bCanSeek)
                {
                    httpWebRequest = SetupWebRequest(msg, headers);

                    if (inputStream != null)
                    {
                        inputStream.Position = initialPosition;
                    
                        if (!_useChunked)
                            httpWebRequest.ContentLength = (int)inputStream.Length;
          
                        writeStream = httpWebRequest.GetRequestStream();
                        StreamHelper.CopyStream(inputStream, writeStream);                  
                    }                
                } // end of "try to send one more time"
            }

            if (inputStream != null)
                inputStream.Close();                

            if (writeStream != null)
                writeStream.Close(); 

            return httpWebRequest;
        } // ProcessAndSend


        private void ReceiveAndProcess(HttpWebResponse response, 
                                       out ITransportHeaders returnHeaders,
                                       out Stream returnStream)
        {
            HttpStatusCode statusCode = response.StatusCode;

            //
            // Read Response Message

            // Just hand back the network stream
            //   (NOTE: The channel sinks are responsible for calling Close() on a stream
            //    once they are done with it).
            returnStream = new BufferedStream(response.GetResponseStream(), 1024);  

            // collect headers
            returnHeaders = new TransportHeaders();
            foreach (Object key in response.Headers)
            {
                String keyString = key.ToString();
                returnHeaders[keyString] = response.Headers[keyString];
            }
        } // ReceiveAndProcess



        //
        // Support for properties (through BaseChannelSinkWithProperties)
        //

        public override Object this[Object key]
        {
            get
            {
                String keyStr = key as String;
                if (keyStr == null)
                    return null;
            
                switch (keyStr.ToLower(CultureInfo.InvariantCulture))
                {
                case UserNameKey: return _securityUserName; 
                case PasswordKey: return null; // Intentionally refuse to return password.
                case DomainKey: return _securityDomain;
                case PreAuthenticateKey: return _bSecurityPreAuthenticate; 
                case CredentialsKey: return _credentials;
                case ClientCertificatesKey: return null; // Intentionally refuse to return certificates
                case ProxyNameKey: return _proxyName; 
                case ProxyPortKey: return _proxyPort; 
                case TimeoutKey: return _timeout;
                case AllowAutoRedirectKey: return _bAllowAutoRedirect;
                } // switch (keyStr.ToLower(CultureInfo.InvariantCulture))

                return null; 
            }
        
            set
            {
                String keyStr = key as String;
                if (keyStr == null)
                    return;
    
                switch (keyStr.ToLower(CultureInfo.InvariantCulture))
                {
                case UserNameKey: _securityUserName = (String)value; break;
                case PasswordKey: _securityPassword = (String)value; break;    
                case DomainKey: _securityDomain = (String)value; break;                
                case PreAuthenticateKey: _bSecurityPreAuthenticate = Convert.ToBoolean(value); break;
                case CredentialsKey: _credentials = (ICredentials)value; break;
                case ProxyNameKey: _proxyName = (String)value; UpdateProxy(); break;
                case ProxyPortKey: _proxyPort = Convert.ToInt32(value); UpdateProxy(); break;

                case TimeoutKey: 
                {
                    if (value is TimeSpan)
                        _timeout = (int)((TimeSpan)value).TotalMilliseconds;
                    else
                        _timeout = Convert.ToInt32(value); 
                    break;
                } // case TimeoutKey

                case AllowAutoRedirectKey: _bAllowAutoRedirect = Convert.ToBoolean(value); break;
                
                } // switch (keyStr.ToLower(CultureInfo.InvariantCulturey))
            }
        } // this[]   
        

        public override ICollection Keys
        {
            get
            {
                if (s_keySet == null)
                {
                    // Don't need to synchronize. Doesn't matter if the list gets
                    // generated twice.
                    ArrayList keys = new ArrayList(6);
                    keys.Add(UserNameKey);
                    keys.Add(PasswordKey);
                    keys.Add(DomainKey);
                    keys.Add(PreAuthenticateKey);
                    keys.Add(CredentialsKey);
                    keys.Add(ClientCertificatesKey);
                    keys.Add(ProxyNameKey);
                    keys.Add(ProxyPortKey);
                    keys.Add(TimeoutKey);
                    keys.Add(AllowAutoRedirectKey);                    
                    
                    s_keySet = keys;
                }

                return s_keySet;
            }
        } // Keys


        //
        // end of Support for properties
        //


        //
        // Helper functions for processing settings and properties
        //

        // Called to recreate proxy object whenever the proxy name or port is changed.
        private void UpdateProxy()
        {
            if ((_proxyName != null) && (_proxyPort > 0))
            {
                WebProxy proxy = new WebProxy(_proxyName, _proxyPort);
                
                // disable proxy use when the host is local. i.e. without periods
                proxy.BypassProxyOnLocal = true;

                _proxyObject = proxy;
            }
        } // UpdateProxy

        //
        // end of Helper functions for processing settings and properties
        //


        internal static String UserAgent
        {
            get { return s_userAgent; }
        }       
        
                
    } // class HttpClientTransportSink




} // namespace System.Runtime.Remoting.Channels.Http
