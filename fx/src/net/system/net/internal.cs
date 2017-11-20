//------------------------------------------------------------------------------
// <copyright file="Internal.cs" company="Microsoft">
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

    using System.Reflection;
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Net.Sockets;
    using System.Runtime.InteropServices;
    using System.Text.RegularExpressions;
    using System.Security.Permissions;


    internal class IntPtrHelper {
        internal static IntPtr Add(IntPtr a, int b) {
            return (IntPtr) ((long) a + b);
        }

        internal static IntPtr Add(IntPtr a, IntPtr b) {
            return (IntPtr) ((long) a + (long)b);
        }

        internal static IntPtr Add(int a, IntPtr b) {
            return (IntPtr) ((long) a + (long)b);
        }
    }


    //
    // support class for Validation related stuff.
    //
    internal class ValidationHelper {

        public static string [] EmptyArray = new string[0];

        internal static readonly char[]  InvalidMethodChars =
                new char[]{
                ' ',
                '\r',
                '\n',
                '\t'
                };

        // invalid characters that cannot be found in a valid method-verb or http header
        internal static readonly char[]  InvalidParamChars =
                new char[]{
                '(',
                ')',
                '<',
                '>',
                '@',
                ',',
                ';',
                ':',
                '\\',
                '"',
                '\'',
                '/',
                '[',
                ']',
                '?',
                '=',
                '{',
                '}',
                ' ',
                '\t',
                '\r',
                '\n'};

        public static string [] MakeEmptyArrayNull(string [] stringArray) {
            if ( stringArray == null || stringArray.Length == 0 ) {
                return null;
            } else {
                return stringArray;
            }
        }

        public static string MakeStringNull(string stringValue) {
            if ( stringValue == null || stringValue.Length == 0) {
                return null;
            } else {
                return stringValue;
            }
        }

        public static string MakeStringEmpty(string stringValue) {
            if ( stringValue == null || stringValue.Length == 0) {
                return String.Empty;
            } else {
                return stringValue;
            }
        }

        public static int HashCode(object objectValue) {
            if (objectValue == null) {
                return -1;
            } else {
                return objectValue.GetHashCode();
            }
        }
        public static string ToString(object objectValue) {
            if (objectValue == null) {
                return "(null)";
            } else if (objectValue is string && ((string)objectValue).Length==0) {
                return "(string.empty)";
            } else {
                return objectValue.ToString();
            }
        }
        public static string HashString(object objectValue) {
            if (objectValue == null) {
                return "(null)";
            } else if (objectValue is string && ((string)objectValue).Length==0) {
                return "(string.empty)";
            } else {
                return objectValue.GetHashCode().ToString();
            }
        }

        public static bool IsInvalidHttpString(string stringValue) {
            if (stringValue.IndexOfAny(InvalidParamChars) != -1) {
                return true; //  valid
            }

            return false;
        }

        public static bool IsBlankString(string stringValue) {
            if (stringValue == null || stringValue.Length == 0) {
                return true;
            } else {
                return false;
            }
        }

        public static bool ValidateUInt32(long address) {
            // on false, API should throw new ArgumentOutOfRangeException("address");
            return address>=0x00000000 && address<=0xFFFFFFFF;
        }

        public static bool ValidateTcpPort(int port) {
            // on false, API should throw new ArgumentOutOfRangeException("port");
            return port>=IPEndPoint.MinPort && port<=IPEndPoint.MaxPort;
        }

        public static bool ValidateRange(int actual, int fromAllowed, int toAllowed) {
            // on false, API should throw new ArgumentOutOfRangeException("argument");
            return actual>=fromAllowed && actual<=toAllowed;
        }

        public static bool ValidateRange(long actual, long fromAllowed, long toAllowed) {
            // on false, API should throw new ArgumentOutOfRangeException("argument");
            return actual>=fromAllowed && actual<=toAllowed;
        }

    }

    internal class ExceptionHelper {
        private static NotImplementedException methodNotImplementedException;
        public static NotImplementedException MethodNotImplementedException {
            get {
                if (methodNotImplementedException==null) {
                    methodNotImplementedException = new NotImplementedException(SR.GetString(SR.net_MethodNotImplementedException));
                }
                return methodNotImplementedException;
            }
        }
        private static NotImplementedException propertyNotImplementedException;
        public static NotImplementedException PropertyNotImplementedException {
            get {
                if (propertyNotImplementedException==null) {
                    propertyNotImplementedException = new NotImplementedException(SR.GetString(SR.net_PropertyNotImplementedException));
                }
                return propertyNotImplementedException;
            }
        }
        private static NotSupportedException methodNotSupportedException;
        public static NotSupportedException MethodNotSupportedException {
            get {
                if (methodNotSupportedException==null) {
                    methodNotSupportedException = new NotSupportedException(SR.GetString(SR.net_MethodNotSupportedException));
                }
                return methodNotSupportedException;
            }
        }
        private static NotSupportedException propertyNotSupportedException;
        public static NotSupportedException PropertyNotSupportedException {
            get {
                if (propertyNotSupportedException==null) {
                    propertyNotSupportedException = new NotSupportedException(SR.GetString(SR.net_PropertyNotSupportedException));
                }
                return propertyNotSupportedException;
            }
        }
    }



    //
    // WebRequestPrefixElement
    //
    // This is an element of the prefix list. It contains the prefix and the
    // interface to be called to create a request for that prefix.
    //

    /// <include file='doc\Internal.uex' path='docs/doc[@for="WebRequestPrefixElement"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    // internal class WebRequestPrefixElement {
    internal class WebRequestPrefixElement  {

        /// <include file='doc\Internal.uex' path='docs/doc[@for="WebRequestPrefixElement.Prefix"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    string              Prefix;
        /// <include file='doc\Internal.uex' path='docs/doc[@for="WebRequestPrefixElement.Creator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public    IWebRequestCreate   Creator;

        /// <include file='doc\Internal.uex' path='docs/doc[@for="WebRequestPrefixElement.WebRequestPrefixElement"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public WebRequestPrefixElement(string P, IWebRequestCreate C) {
            Prefix = P;
            Creator = C;
        }

    } // class PrefixListElement


    //
    // HttpRequestCreator.
    //
    // This is the class that we use to create HTTP and HTTPS requests.
    //

    internal class HttpRequestCreator : IWebRequestCreate {

        internal HttpRequestCreator() {
        }

        /*++

         Create - Create an HttpWebRequest.

            This is our method to create an HttpWebRequest. We register
            for HTTP and HTTPS Uris, and this method is called when a request
            needs to be created for one of those.


            Input:
                    Uri             - Uri for request being created.

            Returns:
                    The newly created HttpWebRequest.

         --*/

        public WebRequest Create( Uri Uri ) {

            //
            //Note, DNS permissions check will not happen on WebRequest
            //
            (new WebPermission(NetworkAccess.Connect, Uri.AbsoluteUri)).Demand();
            //new WebPermission(NetworkAccess.Connect, new Regex(requestUri.AbsoluteUri, RegexOptions.Compiled)).Demand();
            //new WebPermission(NetworkAccess.Connect, new Regex(requestUri.AbsoluteUri, RegexOptions.None)).Demand();

            return new HttpWebRequest(Uri);
        }

    } // class HttpRequestCreator

    //
    //  CoreResponseData - Used to store result of HTTP header parsing and
    //      response parsing.  Also Contains new stream to use, and
    //      is used as core of new Response
    //
    internal class CoreResponseData {

        // Status Line Response Values
        public HttpStatusCode m_StatusCode;
        public string m_StatusDescription;
        public Version m_Version;

        // Content Length needed for semantics, -1 if chunked
        public long m_ContentLength;

        // Response Headers
        public WebHeaderCollection m_ResponseHeaders;

        // ConnectStream - for reading actual data
        public ConnectStream m_ConnectStream;
    }


    /*++

    StreamChunkBytes - A class to read a chunk stream from a ConnectStream.

    A simple little value class that implements the IReadChunkBytes
    interface.

    --*/
    internal class StreamChunkBytes : IReadChunkBytes {

        public  ConnectStream   ChunkStream;
        public  int             BytesRead = 0;
        public  int             TotalBytesRead = 0;
        private byte            PushByte;
        private bool            HavePush;

        public StreamChunkBytes(ConnectStream connectStream) {
            ChunkStream = connectStream;
            return;
        }

        public int NextByte {
            get {
                if (HavePush) {
                    HavePush = false;
                    return PushByte;
                }

                return ChunkStream.ReadSingleByte();
            }
            set {
                PushByte = (byte)value;
                HavePush = true;
            }
        }

    } // class StreamChunkBytes

    // Delegate type for a SubmitRequestDelegate.

    internal delegate void HttpSubmitDelegate(ConnectStream SubmitStream, WebExceptionStatus Status);

    // internal delegate bool HttpHeadersDelegate();

    internal delegate void HttpAbortDelegate();

    //
    // this class contains known header names
    //

    internal class HttpKnownHeaderNames {

        public const string CacheControl = "Cache-Control";
        public const string Connection = "Connection";
        public const string Date = "Date";
        public const string KeepAlive = "Keep-Alive";
        public const string Pragma = "Pragma";
        public const string ProxyConnection = "Proxy-Connection";
        public const string Trailer = "Trailer";
        public const string TransferEncoding = "Transfer-Encoding";
        public const string Upgrade = "Upgrade";
        public const string Via = "Via";
        public const string Warning = "Warning";
        public const string ContentLength = "Content-Length";
        public const string ContentType = "Content-Type";
        public const string ContentEncoding = "Content-Encoding";
        public const string ContentLanguage = "Content-Language";
        public const string ContentLocation = "Content-Location";
        public const string ContentRange = "Content-Range";
        public const string Expires = "Expires";
        public const string LastModified = "Last-Modified";
        public const string Age = "Age";
        public const string Location = "Location";
        public const string ProxyAuthenticate = "Proxy-Authenticate";
        public const string RetryAfter = "Retry-After";
        public const string Server = "Server";
        public const string SetCookie = "Set-Cookie";
        public const string SetCookie2 = "Set-Cookie2";
        public const string Vary = "Vary";
        public const string WWWAuthenticate = "WWW-Authenticate";
        public const string Accept = "Accept";
        public const string AcceptCharset = "Accept-Charset";
        public const string AcceptEncoding = "Accept-Encoding";
        public const string AcceptLanguage = "Accept-Language";
        public const string Authorization = "Authorization";
        public const string Cookie = "Cookie";
        public const string Cookie2 = "Cookie2";
        public const string Expect = "Expect";
        public const string From = "From";
        public const string Host = "Host";
        public const string IfMatch = "If-Match";
        public const string IfModifiedSince = "If-Modified-Since";
        public const string IfNoneMatch = "If-None-Match";
        public const string IfRange = "If-Range";
        public const string IfUnmodifiedSince = "If-Unmodified-Since";
        public const string MaxForwards = "Max-Forwards";
        public const string ProxyAuthorization = "Proxy-Authorization";
        public const string Referer = "Referer";
        public const string Range = "Range";
        public const string UserAgent = "User-Agent";
        public const string ContentMD5 = "Content-MD5";
        public const string ETag = "ETag";
        public const string TE = "TE";
        public const string Allow = "Allow";
        public const string AcceptRanges = "Accept-Ranges";
    }

    /// <include file='doc\Internal.uex' path='docs/doc[@for="HttpContinueDelegate"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the method that will notify callers when a continue has been
    ///       received by the client.
    ///    </para>
    /// </devdoc>
    // Delegate type for us to notify callers when we receive a continue
    public delegate void HttpContinueDelegate(int StatusCode, WebHeaderCollection httpHeaders);

    //
    // HttpWriteMode - used to control the way in which an entity Body is posted.
    //
    internal enum HttpWriteMode {
        None    = 0,
        Chunked = 1,
        Write   = 2,
    }

    internal enum HttpProcessingResult {
        Continue  = 0,
        ReadWait  = 1,
        WriteWait = 2,
    }

    //
    // HttpVerb - used to define various per Verb Properties
    //

    //
    // Note - this is a place holder for Verb properties,
    //  the following two bools can most likely be combined into
    //  a single Enum type.  And the Verb can be incorporated.
    //

    internal class HttpVerb {

        internal bool m_RequireContentBody; // require content body to be sent
        internal bool m_ContentBodyNotAllowed; // not allowed to send content body
        internal bool m_ConnectRequest; // special semantics for a connect request
        internal bool m_ExpectNoContentResponse; // response will not have content body

        internal HttpVerb(bool RequireContentBody, bool ContentBodyNotAllowed, bool ConnectRequest, bool ExpectNoContentResponse) {

            m_RequireContentBody = RequireContentBody;
            m_ContentBodyNotAllowed = ContentBodyNotAllowed;
            m_ConnectRequest = ConnectRequest;
            m_ExpectNoContentResponse = ExpectNoContentResponse;
        }
    }


    //
    // KnownVerbs - Known Verbs are verbs that require special handling
    //

    internal class KnownVerbs {

        // Force an an init, before we use them
        private static ListDictionary namedHeaders = InitializeKnownVerbs();

        // default verb, contains default properties for an unidentifable verb.
        private static HttpVerb DefaultVerb;

        //
        // InitializeKnownVerbs - Does basic init for this object,
        //  such as creating defaultings and filling them
        //
        private static ListDictionary InitializeKnownVerbs() {

            namedHeaders = new ListDictionary(CaseInsensitiveString.StaticInstance);

            namedHeaders["GET"] = new HttpVerb(false, true, false, false);
            namedHeaders["CONNECT"] = new HttpVerb(false, true, true, false);
            namedHeaders["HEAD"] = new HttpVerb(false, true, false, true);
            namedHeaders["POST"] = new HttpVerb(true, false, false, false);
            namedHeaders["PUT"] = new HttpVerb(true, false, false, false);

            // default Verb
            KnownVerbs.DefaultVerb = new HttpVerb(false, false, false, false);

            return namedHeaders;
        }

        internal static HttpVerb GetHttpVerbType(String name) {

            HttpVerb obj = (HttpVerb)KnownVerbs.namedHeaders[name];

            return obj != null ? obj : KnownVerbs.DefaultVerb;
        }
    }


    //
    // HttpProtocolUtils - A collection of utility functions for HTTP usage.
    //

    internal class HttpProtocolUtils {


        private HttpProtocolUtils() {
        }

        //
        // extra buffers for build/parsing, recv/send HTTP data,
        //  at some point we should consolidate
        //


        // parse String to DateTime format.
        internal static DateTime
        string2date(String S) {
            DateTime dtOut;

            if (HttpDateParse.ParseHttpDate(
                                           S,
                                           out dtOut)) {
                return dtOut;
            }
            else {
                throw new ProtocolViolationException(SR.GetString(SR.net_baddate));
            }

        }

        // convert Date to String using RFC 1123 pattern
        internal static string
        date2string(DateTime D) {
            DateTimeFormatInfo dateFormat = new DateTimeFormatInfo();
            return D.ToUniversalTime().ToString("R", dateFormat);
        }
    }


    internal enum TriState {
        Unknown = -1,
        No = 0,
        Yes = 1
    }

    internal enum DefaultPorts {
        DEFAULT_FTP_PORT = 21,
        DEFAULT_GOPHER_PORT = 70,
        DEFAULT_HTTP_PORT = 80,
        DEFAULT_HTTPS_PORT = 443,
        DEFAULT_NNTP_PORT = 119,
        DEFAULT_SMTP_PORT = 25,
        DEFAULT_TELNET_PORT = 23
    }

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    internal struct hostent {
        public IntPtr   h_name;
        public IntPtr   h_aliases;
        public short    h_addrtype;
        public short    h_length;
        public IntPtr   h_addr_list;
    }


    [StructLayout(LayoutKind.Sequential)]
    internal struct Blob {
        public int cbSize;
        public int pBlobData;
    }



} // namespace System.Net
