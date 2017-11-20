#
# 
#  Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
# 
#  The use and distribution terms for this software are contained in the file
#  named license.txt, which can be found in the root of this distribution.
#  By using this software in any fashion, you are agreeing to be bound by the
#  terms of this license.
# 
#  You must not remove this notice, or any other, from this software.
# 
#

!CMDSWITCHES -I

NEWCALL_TOO             = 1             #                                                                   

!include $(NTMAKEENV)\sources.csc

!ifdef PASS0ONLY
!else
!ifdef LINKONLY
NTTARGETFILE1   = csc_target_sign
!else
NTTARGETFILE0   = 
!endif
!endif

!include $(NTMAKEENV)\sources.cor

TARGETNAME      = System.Runtime.Remoting
TARGETLIBNAME   =$(TARGETNAME)


TARGETPATH      = $(TARGETCORLIB)\$(TARGET_DIRECTORY)\$(DDKBUILDENV)
TARGETTYPE      = DYNLINK
NO_BROWSER_FILE = 1
SOURCES         = 

CSC_SOURCE_DIR  = $(CORBASE)\src\Remoting
CSC_CLASS_DIR   = $(CSC_SOURCE_DIR)\$(_OBJ_DIR)\$(TARGET_DIRECTORY)
CSC_TARGET_TYPE = DYNLINK

# Set version information in assembly manifest
CSC_SET_VERSION = 1 

ASSEMBLY_KEY_TYPE=ECMA
SIGN_ASSEMBLY=1

# Base address to load mscorlib.dll
CSC_BASE         = $(TARGETNAME)

#Make sure everything else is built
SYNCHRONIZE_DRAIN=1

# Version resource

WIN32_RESOURCE_FILE =remoting.rc

IMPORTS = /R:System.Runtime.Serialization.Formatters.Soap.dll \
        /R:$(TARGETCOMPLUS)\System.dll                           \
        /R:$(TARGETCOMPLUS)\System.Xml.dll                       \
        
        
#        


CSC_COMPILE_FLAGS = $(CSC_EXTRA_PREPROCESS_FLAGS) $(CSC_COMPILE_FLAGS)


CSC_PERMISSION_REBUILD = 1
CSC_SECURITY_DB = SecurityDB

CSC_COMPILE_FLAGS = $(CSC_COMPILE_FLAGS) \
    /res:$(TARGETPATH)\System.Runtime.Remoting.resources,System.Runtime.Remoting.resources

# The first set of ~17 files are in order of the classes 
# loaded at startup. Try not to change this order. Working set
# perf degrades if done so.

CHANNELS_HTTP_SOURCES = \
        ..\channels\http\combinedhttpchannel.cs \
        ..\channels\http\httpchannelhelper.cs \
        ..\channels\http\httpclientchannel.cs \
        ..\channels\http\httpserverchannel.cs \
        ..\channels\http\httpsocketmanager.cs \
        ..\channels\http\httpstreams.cs


CHANNELS_TCP_SOURCES = \
        ..\channels\tcp\combinedtcpchannel.cs \
        ..\channels\tcp\tcpchannelhelper.cs \
        ..\channels\tcp\tcpclientchannel.cs \
        ..\channels\tcp\tcpclientsocketmanager.cs \
        ..\channels\tcp\tcpserverchannel.cs \
        ..\channels\tcp\tcpsocketmanager.cs \
        ..\channels\tcp\tcpstreams.cs \
        ..\channels\tcp\tcpwireprotocol.cs \

CHANNELS_CORE_SOURCES = \
        ..\channels\core\basetransportheaders.cs \
        ..\channels\core\bytebufferpool.cs \
        ..\channels\core\chunkedmemorystream.cs \
        ..\channels\core\commonchannelkeys.cs \
        ..\channels\core\corechannel.cs  \
        ..\channels\core\requestqueue.cs \
        ..\channels\core\socketcache.cs \
        ..\channels\core\socketmanager.cs \
        ..\channels\core\socketstream.cs \
        ..\channels\core\streamhelper.cs  \
        ..\channels\core\stringhelper.cs  \


CHANNELS_SINKS_SOURCES = \
        ..\channels\sinks\binaryformattersinks.cs \
        ..\channels\sinks\soapformattersinks.cs \

METADATA_SOURCES = \
        ..\metadata\metadata.cs \
        ..\metadata\sdlchannelsink.cs \
        ..\metadata\sudsparser.cs \
        ..\metadata\sudswriter.cs \
        ..\metadata\sudscommon.cs \
        ..\metadata\sdlparser.cs \
        ..\metadata\sdlwriter.cs \
        ..\metadata\wsdlparser.cs \
        ..\metadata\wsdlwriter.cs 

        
SERVICES_SOURCES = \
        ..\services\remotingclientproxy.cs



!IF "$(FEATURE_PAL)" == "1" || "$(_REM_DLL_NAME)"=="MainDll"
CSC_SOURCES  =    \
#       $(ACTIVATION_SOURCES)   \
        $(CHANNELS_CORE_SOURCES)        \
        $(CHANNELS_HTTP_SOURCES)        \
        $(CHANNELS_TCP_SOURCES) \
        $(CHANNELS_SINKS_SOURCES) \
        $(METADATA_SOURCES)     \
        $(SERVICES_SOURCES)     \
!ELSE
CSC_SOURCES  =    \
        $(CHANNELS_CORE_SOURCES_FOR_SMTP)        \
        $(CHANNELS_SMTP_SOURCES)        \
!ENDIF


