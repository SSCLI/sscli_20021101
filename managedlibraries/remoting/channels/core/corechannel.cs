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
using System;
using System.Text;
using System.Threading;
using System.Reflection;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters;
using System.Runtime.Serialization.Formatters.Soap;
using System.Runtime.Serialization.Formatters.Binary;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Messaging;       
using System.Runtime.Remoting.Metadata;
using System.IO;
using System.Net;
using System.Collections;
using System.Net.Sockets;
using System.Resources;
using System.Diagnostics;
using System.Globalization;

namespace System.Runtime.Remoting.Channels
{

    // Use this internal indicator (as opposed to the nested enum found
    //   on some of the server channel sinks)
    internal enum SinkChannelProtocol
    {
        Http, // special processing needed for http
        Other
    } // ChannelProtocol

    

    internal class CoreChannel    
    {
        private static IByteBufferPool _bufferPool = new ByteBufferPool(10, 4096);
        private static RequestQueue _requestQueue = new RequestQueue(8,4,250);        

        internal static IByteBufferPool BufferPool { get { return _bufferPool; } }
        internal static RequestQueue RequestQueue { get { return _requestQueue; } }
        
    
        internal const int MaxStringLen = 512;

        internal const String SOAPMimeType = "text/xml";
        internal const String BinaryMimeType = "application/octet-stream";

        internal const String SOAPContentType = "text/xml; charset=\"utf-8\"";

        private static String s_hostName = null; 
        private static String s_MachineName = null;
        private static String s_MachineIp = null;

        // Copy of consts defined in RemotingServices.cs
        internal const int CLIENT_MSG_GEN          = 1;
        internal const int CLIENT_MSG_SINK_CHAIN   = 2;
        internal const int CLIENT_MSG_SER          = 3;
        internal const int CLIENT_MSG_SEND         = 4;
        internal const int SERVER_MSG_RECEIVE      = 5;
        internal const int SERVER_MSG_DESER        = 6;
        internal const int SERVER_MSG_SINK_CHAIN   = 7;
        internal const int SERVER_MSG_STACK_BUILD  = 8;
        internal const int SERVER_DISPATCH         = 9;
        internal const int SERVER_RET_STACK_BUILD  = 10;
        internal const int SERVER_RET_SINK_CHAIN   = 11;
        internal const int SERVER_RET_SER          = 12;
        internal const int SERVER_RET_SEND         = 13;
        internal const int SERVER_RET_END          = 14;
        internal const int CLIENT_RET_RECEIVE      = 15;
        internal const int CLIENT_RET_DESER        = 16;
        internal const int CLIENT_RET_SINK_CHAIN   = 17;
        internal const int CLIENT_RET_PROPAGATION  = 18;
        internal const int CLIENT_END_CALL         = 19;
        internal const int TIMING_DATA_EOF         = 99;

        internal static void DebugInitialize(String s)
        {
        }


        internal static String GetHostName()
        {
            if (s_hostName == null)
            {
                s_hostName = Dns.GetHostName();

                if (s_hostName == null)
                {
                    throw new ArgumentNullException("hostName");
                }
            }

            return s_hostName;
        } // GetHostName

        internal static String GetMachineName()
        {
            if (s_MachineName == null)
            {     
                String machineName = GetHostName();
                if (machineName != null)
                {
                    IPHostEntry host = Dns.GetHostByName(machineName);
                    if (host != null)
                        s_MachineName = host.HostName;
                } 

                if (s_MachineName == null)
                {
                    throw new ArgumentNullException("machine");
                }
            }
            
            return s_MachineName;      
        } // GetMachineName


        // process specified host name to see if it is a meta-hostname that
        //   should be replaced with something else.
        internal static String DecodeMachineName(String machineName)
        {
            if (machineName.Equals("$hostName"))
                return GetHostName();

            return machineName;
        } // DecodeMachineName
        

        internal static String GetMachineIp()
        {
            if (s_MachineIp == null)
            {            
                String hostName = GetMachineName();

                // NOTE: We intentionally allow exceptions from these api's
                //  propagate out to the caller.
                IPHostEntry ipEntries = Dns.GetHostByName(hostName);
                if ((ipEntries != null) && (ipEntries.AddressList.Length > 0))
                {
                    s_MachineIp = ipEntries.AddressList[0].ToString();
                }               
    
                if (s_MachineIp == null)
                {
                    throw new ArgumentNullException("ip");
                }
            }
            
            return s_MachineIp;      
        } // GetMachineIp

        //
        // Core Serialization and Deserialization support
        //        
        internal static Header[] GetMessagePropertiesAsSoapHeader(IMessage reqMsg)
        {
            IDictionary d = reqMsg.Properties;
            if (d == null)
                return null;
                
            int count = d.Count;
            if (count == 0)
                return null;

            IDictionaryEnumerator e = (IDictionaryEnumerator) d.GetEnumerator();

            // cycle through the headers to get a length
            bool[] map = new bool[count];
            int len = 0, i=0;
            IMethodMessage msg = (IMethodMessage)reqMsg;
            while (e.MoveNext())
            {                   
                String key = (String)e.Key;
                if ((key.Length >= 2) &&
                    (String.CompareOrdinal(key, 0, "__", 0, 2)  == 0) 
                     &&
                     (
                        key.Equals("__Args") 
                                ||
                        key.Equals("__OutArgs") 
                                ||
                        key.Equals("__Return") 
                                ||
                        key.Equals("__Uri") 
                                ||
                        key.Equals("__MethodName") 
                                ||
                        (key.Equals("__MethodSignature") 
                                && (!RemotingServices.IsMethodOverloaded(msg))
                                && (!msg.HasVarArgs))
                                ||
                        key.Equals("__TypeName") 
                                ||
                        key.Equals("__Fault") 
                                ||
                        (key.Equals("__CallContext") 
                                && ((e.Value != null) ? (((LogicalCallContext)e.Value).HasInfo==false) : true))
                      )                       
                  )
                {
                        i++;
                    continue;
                }
                map[i] = true;
                i++;                
                len++;
            }
            if (len == 0)
                return null;

            
            Header[] ret = new Header[len];
            e.Reset();
            int k=0; 
            i = 0;
            while (e.MoveNext())
            {
                Object key = e.Key;
                if (!map[k])
                {
                    k++;
                    continue;
                }
                
                Header h = e.Value as Header;

                // If the property is not a header, then make a header out of it.
                if (h == null)
                {
                    h = 
                        new Header(
                            (String)key, e.Value, false,
                            "http://schemas.microsoft.com/clr/soap/messageProperties");
                }

                if (i == ret.Length)
                {
                    InternalRemotingServices.RemotingTrace("HTTPChannel::GetHeaders creating a new array of length " + (i+1) + "\n");
                    Header[] newret= new Header[i+1];
                    Array.Copy(ret, newret, i);
                    ret = newret;
                }
                ret[i] = h;
                i++;
                k++;
            }
            
            return ret;
        } // GetMessagePropertiesAsSoapHeader
        

        internal static Header[] GetSoapHeaders(IMessage reqMsg)
        {    
            // If there are message properties, we'll need to resize the header array.
            Header[] msgProperties = GetMessagePropertiesAsSoapHeader(reqMsg);

            return msgProperties;
        } // GetSoapHeaders



        internal static SoapFormatter CreateSoapFormatter(bool serialize, bool includeVersions)
        {            
            SoapFormatter remotingFormatter = new SoapFormatter();

            if (serialize)
            {
                RemotingSurrogateSelector rss = new RemotingSurrogateSelector();
                remotingFormatter.SurrogateSelector = rss;
                rss.UseSoapFormat();
            }
            else
                remotingFormatter.SurrogateSelector = null;

            remotingFormatter.Context = new StreamingContext(StreamingContextStates.Other);

            remotingFormatter.AssemblyFormat = 
                includeVersions ? 
                    FormatterAssemblyStyle.Full :
                    FormatterAssemblyStyle.Simple;

            return remotingFormatter;
        } // CreateSoapFormatter


        internal static BinaryFormatter CreateBinaryFormatter(bool serialize, 
                                                              bool includeVersionsOrStrictBinding)
        {
            BinaryFormatter remotingFormatter = new BinaryFormatter();

            if (serialize)
            {
                RemotingSurrogateSelector rss = new RemotingSurrogateSelector();
                remotingFormatter.SurrogateSelector = rss;
            }
            else
            {
                remotingFormatter.SurrogateSelector = null;
            }

            remotingFormatter.Context = new StreamingContext(StreamingContextStates.Other);

            remotingFormatter.AssemblyFormat = 
                includeVersionsOrStrictBinding ? 
                    FormatterAssemblyStyle.Full :
                    FormatterAssemblyStyle.Simple;  
            
            return remotingFormatter;
        } // CreateBinaryFormatter




        internal static void SerializeSoapMessage(IMessage msg, Stream outputStream, bool includeVersions)
        {
            // create soap formatter
            SoapFormatter fmt = CreateSoapFormatter(true, includeVersions);

            //check for special options if this is the SoapFormatter
            IMethodMessage methodMsg = msg as IMethodMessage;
            if (methodMsg != null)
            {
                MethodBase mb = methodMsg.MethodBase;
                if (mb != null)
                {
                    Type type = methodMsg.MethodBase.DeclaringType;
                    SoapTypeAttribute cache = 
                        (SoapTypeAttribute)InternalRemotingServices.GetCachedSoapAttribute(type);
                    if ((cache.SoapOptions & SoapOption.AlwaysIncludeTypes) == SoapOption.AlwaysIncludeTypes)
                        fmt.TypeFormat |= FormatterTypeStyle.TypesAlways;
                    if ((cache.SoapOptions & SoapOption.XsdString) == SoapOption.XsdString)
                        fmt.TypeFormat |= FormatterTypeStyle.XsdString;                
                }
            }
            // end of set special options for SoapFormatter
            
            Header[] h = GetSoapHeaders(msg);

            // this is to make messages within a  message serialize correctly 
            // and not use the fake type
            ((RemotingSurrogateSelector)fmt.SurrogateSelector).SetRootObject(msg);
            fmt.Serialize(outputStream, msg, h);
        } // SerializeSoapMessage

        internal static Stream SerializeSoapMessage(IMessage msg, bool includeVersions)
        {
            MemoryStream memStream = new MemoryStream();
            SerializeSoapMessage(msg, memStream, includeVersions);
            memStream.Position = 0;
            return memStream;
        } // SerializeSoapMessage
        


        internal static void SerializeBinaryMessage(IMessage msg, Stream outputStream, bool includeVersions)
        {
            // create binary formatter
            BinaryFormatter fmt = CreateBinaryFormatter(true, includeVersions);

            // WE SHOULD NOT CALL GetHeaders() here. The BinaryFormatter does special
            //   serialization for any headers that might be present.

            fmt.Serialize(outputStream, msg, null);
        } // SerializeBinaryMessage
        
        internal static Stream SerializeBinaryMessage(IMessage msg, bool includeVersions)
        {
            MemoryStream memStream = new MemoryStream();
            SerializeBinaryMessage(msg, memStream, includeVersions);
            memStream.Position = 0;
            return memStream;
        } // SerializeBinaryMessage



        // class used to pass uri into binary serializer, so that the message
        //   gets the object uri.
        private class UriHeaderHandler
        {
            String _uri = null;

            internal UriHeaderHandler(String uri)
            {
                _uri = uri;
            }
        
            public Object HeaderHandler(Header[] Headers)
            {
                return _uri;
            }
            
        } // classUriHeaderHandler


        internal static IMessage DeserializeSoapRequestMessage(
            Stream inputStream, Header[] h, bool bStrictBinding)
        {
            SoapFormatter fmt = CreateSoapFormatter(false, bStrictBinding);

            MethodCall mc = new MethodCall(h);
            fmt.Deserialize(inputStream, new HeaderHandler(mc.HeaderHandler));

            IMessage resMessage = (IMessage)mc;

            return resMessage;
        } // DeserializeSoapRequestMessage


        internal static IMessage DeserializeSoapResponseMessage(
            Stream inputStream, IMessage requestMsg, Header[] h, bool bStrictBinding)
        {
            SoapFormatter fmt = CreateSoapFormatter(false, bStrictBinding);

            IMethodCallMessage mcm = (IMethodCallMessage)requestMsg;
            MethodResponse mr = new MethodResponse(h, mcm);
            fmt.Deserialize(inputStream, new HeaderHandler(mr.HeaderHandler));

            IMessage resMessage = (IMessage)mr;

            return resMessage;
        } // DeserializeSoapResponseMessage


        internal static IMessage DeserializeBinaryRequestMessage(
            String objectUri, 
            Stream inputStream,
            bool bStrictBinding)
        {
            BinaryFormatter fmt = CreateBinaryFormatter(false, bStrictBinding);

            UriHeaderHandler uriHH = new UriHeaderHandler(objectUri);

            IMessage reqMsg = 
                (IMessage)fmt.Deserialize(inputStream, new HeaderHandler(uriHH.HeaderHandler));

            return reqMsg;
        } // DeserializeBinaryRequestMessage


        internal static IMessage DeserializeBinaryResponseMessage(
            Stream inputStream,
            IMethodCallMessage reqMsg,
            bool bStrictBinding)
        {
            BinaryFormatter fmt = CreateBinaryFormatter(false, bStrictBinding);

            IMessage replyMsg = (IMessage)fmt.DeserializeMethodResponse(inputStream, null, reqMsg);
            return replyMsg;
        } // DeserializeBinaryResponseMessage

        

        internal static Stream SerializeMessage(String mimeType, IMessage msg)
        {
            return SerializeMessage(mimeType, msg, false);
        } // SerializeMessage

        internal static Stream SerializeMessage(String mimeType, IMessage msg, bool includeVersions)
        {
            Stream returnStream = new MemoryStream();
            SerializeMessage(mimeType, msg, returnStream, includeVersions);
            returnStream.Position = 0;
            return returnStream;
        } // SerializeMessage


        internal static void SerializeMessage(String mimeType, IMessage msg, Stream outputStream,
                                              bool includeVersions)
        {
            InternalRemotingServices.RemotingTrace("SerializeMessage");
            InternalRemotingServices.RemotingTrace("MimeType: " + mimeType);
            CoreChannel.DebugMessage(msg);
            
            if (string.Compare(mimeType, SOAPMimeType, false, CultureInfo.InvariantCulture) == 0)
            {
                SerializeSoapMessage(msg, outputStream, includeVersions);
            }
            else
            if (string.Compare(mimeType, BinaryMimeType, false, CultureInfo.InvariantCulture) == 0)
            {
                SerializeBinaryMessage(msg, outputStream, includeVersions);
            }                   

            InternalRemotingServices.RemotingTrace("SerializeMessage: OUT");
        } // SerializeMessage



        
        internal static IMessage DeserializeMessage(String mimeType, Stream xstm, bool methodRequest, IMessage msg)
        {
            return DeserializeMessage(mimeType, xstm, methodRequest, msg, null);
        }

        internal static IMessage DeserializeMessage(String mimeType, Stream xstm, bool methodRequest, IMessage msg, Header[] h)
        {
            InternalRemotingServices.RemotingTrace("DeserializeMessage");
            InternalRemotingServices.RemotingTrace("MimeType: " + mimeType);

            CoreChannel.DebugOutXMLStream(xstm, "Deserializing");

            Stream fmtStm = null;

            bool bin64encode = false;
            bool doHeaderBodyAsOne = true;

            if (string.Compare(mimeType, BinaryMimeType, false, CultureInfo.InvariantCulture) == 0)
            {
                doHeaderBodyAsOne = true;
            }

            if (string.Compare(mimeType, SOAPMimeType, false, CultureInfo.InvariantCulture) == 0)
            {
                doHeaderBodyAsOne = false;
            }

            if (bin64encode == false)
            {
                fmtStm  = xstm;
            }
            else
            {
                InternalRemotingServices.RemotingTrace("***************** Before base64 decode *****");

                long Position = xstm.Position;
                MemoryStream inStm = (MemoryStream)xstm;
                byte[] byteArray = inStm.ToArray();
                xstm.Position = Position;

                String base64String = Encoding.ASCII.GetString(byteArray,0, byteArray.Length);

                byte[] byteArrayContent = Convert.FromBase64String(base64String);

                MemoryStream memStm = new MemoryStream(byteArrayContent);

                fmtStm = memStm;
                InternalRemotingServices.RemotingTrace("***************** after base64 decode *****");
            }

            Object ret;
            IRemotingFormatter fmt = MimeTypeToFormatter(mimeType, false);

            if (doHeaderBodyAsOne == true)
            {
                ret = ((BinaryFormatter)fmt).DeserializeMethodResponse(fmtStm, null, (IMethodCallMessage)msg);
            }
            else
            {
                InternalRemotingServices.RemotingTrace("Content");
                InternalRemotingServices.RemotingTrace("***************** Before Deserialize Headers *****");

                InternalRemotingServices.RemotingTrace("***************** After Deserialize Headers *****");

                InternalRemotingServices.RemotingTrace("***************** Before Deserialize Message *****");

                if (methodRequest == true)
                {
                    MethodCall mc = new MethodCall(h);
                    InternalRemotingServices.RemotingTrace("***************** Before Deserialize Message - as MethodCall *****");
                    fmt.Deserialize(fmtStm, new HeaderHandler(mc.HeaderHandler));
                    ret = mc;
                }
                else
                {
                    IMethodCallMessage mcm = (IMethodCallMessage)msg;
                    MethodResponse mr = new MethodResponse(h, mcm);
                    InternalRemotingServices.RemotingTrace("***************** Before Deserialize Message - as MethodResponse *****");
                    fmt.Deserialize(fmtStm, new HeaderHandler(mr.HeaderHandler));
                    ret = mr;
                }

                InternalRemotingServices.RemotingTrace("***************** After Deserialize Message *****");
            }

            // Workaround to make this method verifiable
            IMessage resMessage = (IMessage) ret;

            InternalRemotingServices.RemotingTrace("CoreChannel::DeserializeMessage OUT");
            CoreChannel.DebugMessage(resMessage);

            return resMessage;
        }
       
    

        internal static IRemotingFormatter MimeTypeToFormatter(String mimeType, bool serialize)
        {
            InternalRemotingServices.RemotingTrace("MimeTypeToFormatter: mimeType: " + mimeType);

            if (string.Compare(mimeType, SOAPMimeType, false, CultureInfo.InvariantCulture) == 0)
            {
                return CreateSoapFormatter(serialize, true);
            }
            else
            if (string.Compare(mimeType, BinaryMimeType, false, CultureInfo.InvariantCulture) == 0)
            {
                return CreateBinaryFormatter(serialize, true);
            }     

            return null;
        } // MimeTypeToFormatter


        //
        // Other helper methods
        //

        internal static String GetObjectUriFromRequestUri(String uri, IChannel receiver)
        {
            String objectUri;
            String channelUri = receiver.Parse(uri, out objectUri);

            // if there was no channel scheme, then objectUri will be null.
            if (objectUri == null)
                objectUri = uri;

            return RemoveApplicationNameFromUri(objectUri);            
        } // GetObjectUriFromRequestUri


        // combine channel uri with application name to get full channel uri
        internal static String AppendApplicationNameToChannelUri(String channelUri)
        {
            if (channelUri == null)
                return null;

            if (!channelUri.EndsWith("/"))
                channelUri += "/";

            String appName = RemotingConfiguration.ApplicationName;
            if ((appName == null) || (appName.Length == 0))
                return channelUri;
            else
                return channelUri + appName + "/";
            
        } // AppendApplicationNameToChannelUri


        // combine application name with objectURI to get full object uri
        internal static String PrependApplicationNameToObjectUri(String objectUri)
        {
            if (objectUri == null)
                return null;
            
            if (objectUri.StartsWith("/"))
                objectUri = objectUri.Substring(1);

            String appName = RemotingConfiguration.ApplicationName;
            if ((appName == null) || (appName.Length == 0))
                return objectUri;            
            else
                return appName + "/" + objectUri;
        } // PrependApplicationNameToObjectUri
        

        // Removes application name from front of uri if present.
        internal static String RemoveApplicationNameFromUri(String uri)
        {
            if (uri == null)
                return null;
        
            String appName = RemotingConfiguration.ApplicationName;
            if ((appName == null) || (appName.Length == 0))
                return uri;

            // uri must be longer than the appname plus a slash (hence the "+2")
            if (uri.Length < (appName.Length + 2))
                return uri;
            
            // case-insensitively determine if uri starts with app name
            if (String.Compare(appName, 0, uri, 0, appName.Length, true, CultureInfo.InvariantCulture) == 0)
            {
                // make sure a slash follows the app name (we already made sure
                //   uri was long enough above)
                if (uri[appName.Length] == '/')
                {
                    uri = uri.Substring(appName.Length + 1);
                }
            }            

            return uri;            
        } // RemoveApplicationNameFromUri


        internal static void AppendProviderToClientProviderChain(
            IClientChannelSinkProvider providerChain,
            IClientChannelSinkProvider provider)
        {
            if (providerChain == null)
                throw new ArgumentNullException("providerChain");

            // walk to last provider in chain
            while (providerChain.Next != null)
            {
                providerChain = providerChain.Next;
            }

            providerChain.Next = provider;
        } // AppendProviderToClientProviderChain

        internal static void CollectChannelDataFromServerSinkProviders(
            ChannelDataStore channelData,
            IServerChannelSinkProvider provider)
        {
            // walk chain and ask each provider for channel data
            while (provider != null)
            {
                provider.GetChannelData(channelData);
            
                provider = provider.Next;
            }
        } // CollectChannelDataFromServerSinkProviders


        // called by providers that aren't expecting custom provider data
        internal static void VerifyNoProviderData(String providerTypeName, ICollection providerData)
        {
            if ((providerData != null) && (providerData.Count > 0))
            {
                throw new RemotingException(
                    String.Format(
                        CoreChannel.GetResourceString(
                            "Remoting_Providers_Config_NotExpectingProviderData"),
                        providerTypeName));
            }                    
        } // VerifyNoProviderData

        internal static void ReportUnknownProviderConfigProperty(String providerTypeName,
                                                                 String propertyName)
        {
            throw new RemotingException(
                String.Format(
                    CoreChannel.GetResourceString(
                        "Remoting_Providers_Config_UnknownProperty"),
                    providerTypeName, propertyName));
        } // ReportUnknownProviderConfigProperty



        internal static SinkChannelProtocol DetermineChannelProtocol(IChannel channel)
        {
            String objectUri;
            String channelUri = channel.Parse("http://foo.com/foo", out objectUri);
            if (channelUri != null)
                return SinkChannelProtocol.Http;

            return SinkChannelProtocol.Other;
        } // DetermineChannelProtocol


        internal static bool SetupUrlBashingForIisSslIfNecessary()
        {
            // Rotor doesn't support SSL.
            return false;
        } // SetupUrlBashingForIisSslIfNecessary

        internal static void CleanupUrlBashingForIisSslIfNecessary(bool bBashedUrl)
        {
            if (bBashedUrl)
                CallContext.FreeNamedDataSlot("__bashChannelUrl");
        } // CleanupUrlBashingForIisSslIfNecessary

        

        //** Resource helpers ***************************************************
        internal static ResourceManager SystemResMgr;

        private static ResourceManager InitResourceManager()
        {
			if (SystemResMgr == null)
                SystemResMgr = new ResourceManager("System.Runtime.Remoting", typeof(CoreChannel).Module.Assembly);
			return SystemResMgr;
        }

        // Looks up the resource string value for key.
        // 
        internal static String GetResourceString(String key)
        {
            if (SystemResMgr == null)
                InitResourceManager();
            String s = SystemResMgr.GetString(key, null);
            Debug.Assert(s!=null, "Resource string lookup failed.  Resource name was: \""+key+"\"");
            return s;
        }
    
        //** Debug items ***************************************************
        [Conditional("_DEBUG")]
        internal static void DebugOut(String s)
        {
                InternalRemotingServices.DebugOutChnl(s);
        }

        [Conditional("_DEBUG")]
        internal static void DebugOutXMLStream(Stream stm, String tag)
        {
            /*
            This can't be done when using networked streams.
            long oldpos = stm.Position;
            stm.Position=0;
            StreamReader sr = new StreamReader(stm, Encoding.UTF8);
            String line;
            InternalRemotingServices.DebugOutChnl("\n   -----------" + tag + " OPEN-------------\n") ;
            while ((line = sr.ReadLine()) != null)
            {
                InternalRemotingServices.DebugOutChnl(line);
            }
            InternalRemotingServices.DebugOutChnl("\n   -----------" + tag + " CLOSE------------\n") ;

            stm.Position = oldpos;
            */
        }

        [Conditional("_DEBUG")]
        internal static void DebugMessage(IMessage msg)
        {
            /*
              if (msg is IMethodCallMessage)
                InternalRemotingServices.RemotingTrace("IMethodCallMessage");
                
              if (msg is IMethodReturnMessage)
                InternalRemotingServices.RemotingTrace("IMethodReturnMessage");
        
              if (msg == null)
                InternalRemotingServices.RemotingTrace("***** IMessage is null");
        
              InternalRemotingServices.RemotingTrace("DebugMessage Here");
              IDictionary d = msg.Properties;
              if (d == null)
                InternalRemotingServices.RemotingTrace("***** Properties is null");
        
              InternalRemotingServices.RemotingTrace("DebugMessage Here0");      
              if (d.Count == 0)
              {
                  InternalRemotingServices.RemotingTrace("Zero Properties");
                  return;
              }
        
              InternalRemotingServices.RemotingTrace("DebugMessage Here1");
              IDictionaryEnumerator e = (IDictionaryEnumerator) d.GetEnumerator();
              InternalRemotingServices.RemotingTrace("DebugMessage Here1");
        
              while (e.MoveNext())
              {
                InternalRemotingServices.RemotingTrace("DebugMessage Here2");
        
                Object key = e.Key;
                
                InternalRemotingServices.RemotingTrace("DebugMessage Here3");
        
                String keyName = key.ToString();
        
                InternalRemotingServices.RemotingTrace("DebugMessage Here4");
        
                Object value = e.Value;
        
                InternalRemotingServices.RemotingTrace("DebugMessage Here5");
        
                InternalRemotingServices.RemotingTrace(keyName + ":" + e.Value);
        
                InternalRemotingServices.RemotingTrace("DebugMessage Here6");
        
                if (String.Compare(keyName, "__CallContext", true) == 0)
                {
                }
                
                InternalRemotingServices.RemotingTrace("DebugMessage Here7");
              }
              */
        }

        [Conditional("_DEBUG")]
        internal static void DebugException(String name, Exception e)
        {
            InternalRemotingServices.RemotingTrace("****************************************************\r\n");
            InternalRemotingServices.RemotingTrace("EXCEPTION THROWN!!!!!! - " + name);
            InternalRemotingServices.RemotingTrace("\r\n");

            InternalRemotingServices.RemotingTrace(e.Message);
            InternalRemotingServices.RemotingTrace("\r\n");

            InternalRemotingServices.RemotingTrace(e.GetType().FullName);
            InternalRemotingServices.RemotingTrace("\r\n");

            InternalRemotingServices.RemotingTrace(e.StackTrace);
            InternalRemotingServices.RemotingTrace("\r\n");
            InternalRemotingServices.RemotingTrace("****************************************************\r\n");
        }

        [Conditional("_DEBUG")]
        internal static void DebugStream(Stream stm)
        {
            /*
              try
              {
                long Position = stm.Position;
        
                MemoryStream memStm = (MemoryStream)stm;
                byte[] byteArray = memStm.ToArray();
                int byteArrayLength = byteArray.Length;
                String streamString = Encoding.ASCII.GetString(byteArray,0, byteArrayLength);
                InternalRemotingServices.RemotingTrace(streamString);
                stm.Position = Position;
              }
              catch(Exception e)
              {
                DebugException("DebugStream", e);
              }
              */
        }

    } // class CoreChannel


    
}

