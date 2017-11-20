# ==++==
# 
#   
#    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
#   
#    The use and distribution terms for this software are contained in the file
#    named license.txt, which can be found in the root of this distribution.
#    By using this software in any fashion, you are agreeing to be bound by the
#    terms of this license.
#   
#    You must not remove this notice, or any other, from this software.
#   
# 
# ==--==
#
# Build instructions for this directory
#

NO_CRT                                  =1
!include $(NTMAKEENV)\sources.cor

!if "$(PLATFORM_UNIX)" != "1"
# WarningControl.h disables too many warnings.  VM folder will not use it.
COMPILER_WARNINGS	=/WX
!else
# This warns about behavior in fcall.h that's intended.
COMPILER_WARNINGS   =$(COMPILER_WARNINGS) -Wno-ctor-dtor-privacy
!endif

INCLUDES                        = $(INCLUDES);..;..\$(_TGTCPU);$(O);..\..\inc;..\..\inc\$(O);..\..\classlibnative\inc;..\..\md\inc;..\..\xmlparser


MINORCOMP                       =cor

TARGETNAME                      =$(CORBUILDENV)cee_$(_CORFLAVOR)
TARGETPATH                      =$(TARGETCORBIN)
TARGETTYPE                      =LIBRARY

COR_C_FLAGS                     = -DUNICODE -D_UNICODE -DCOMPLUS_EE

!IF "$(DDKBUILDENV)"=="checked"
COR_C_FLAGS = $(COR_C_FLAGS) -DBUILDENV_CHECKED=1
!ENDIF

NTTARGETFILE0 = $(O)/asmconstants.inc

# On NT or Alpha or IA64
COR_C_FLAGS                     = $(COR_C_FLAGS)
# helpful undocumented switch that shows the include hierarchy:  -d1Bshow_incl

!IF "$(PROFILING_SUPPORTED_BUILD)"=="1"
COR_C_FLAGS                     = $(COR_C_FLAGS) -DENABLE_PERF_COUNTERS
!ENDIF

!IF "$(PLATFORM_UNIX)" != "1" && "$(_TGTCPU)"=="i386"
COR_C_FLAGS			= $(COR_C_FLAGS) -W4
!ENDIF

!IF "$(NO_EJIT)"=="1"
COR_C_FLAGS                     = $(COR_C_FLAGS) -DNO_EJIT
!ENDIF


!if "$(PLATFORM_UNIX)" != "1"
# Profile guided optimization
!IF "$(_TGTCPU)"=="i386"
# COR_C_FLAGS=$(COR_C_FLAGS) -d2pv$(CORBASE)\src\inc\mscorwks.pgd
!ENDIF
!endif

CDEFINES                        = $(CDEFINES) $(USER_SPECIFIC_C_DEFINES) $(COR_C_FLAGS)

PRECOMPILED_INCLUDE             = ..\common.h
PRECOMPILED_CXX                 = 1
PRECOMPILED_SOURCEFILE          = ..\common.cpp
CORTHUNKLIBS                    =

#This supresses the annoying warnings from build.exe
CONDITIONAL_INCLUDES=$(CONDITIONAL_INCLUDES) vgmem.h unistd.h alphaops.h stddef.h macwin32.h time.h urlmon.h iostream.h opcode.def assert.h sys\mman.h \usr\include\malloc.h ole2int.h sys\param.h

TARGETLIBS              = $(TARGETLIBS)\
        $(SDK_LIB_PATH)\ole32.lib \
        $(SDK_LIB_PATH)\comdlg32.lib \
        $(SDK_LIB_PATH)\crypt32.lib \
        $(SDK_LIB_PATH)\userenv.lib

#=============================================================
# Lets keep the list of sources in ALPHABETICAL order please!
# These all seem to be relative to the previous directory
# because this file is included by wks\SOURCES and svr\SOURCES
#=============================================================
SOURCES = \
        ..\AppDomain.cpp \
        ..\AppDomainHelper.cpp \
        ..\AppDomainNative.cpp \
        ..\array.cpp \
        ..\Assembly.cpp \
        ..\AssemblyName.cpp \
        ..\AssemblyNative.cpp \
        ..\AssemblySpec.cpp \
        ..\AssemblySink.cpp \
        ..\binder.cpp \
        ..\cachelinealloc.cpp \
        ..\ceeload.cpp \
        ..\ceemain.cpp \
        ..\class.cpp       \
        ..\clsload.cpp \
        ..\codeman.cpp \
        ..\ConfigHelper.cpp \
        ..\COMArrayHelpers.cpp \
        ..\COMArrayInfo.cpp \
        ..\ComCallWrapper.cpp \
        ..\COMCodeAccessSecurityEngine.cpp \
        ..\COMClass.cpp \
        ..\COMCurrency.cpp \
        ..\COMDateTime.cpp \
        ..\COMDecimal.cpp \
        ..\COMDelegate.cpp \
        ..\COMDynamic.cpp \
        ..\COMEvent.cpp \
        ..\COMIsolatedStorage.cpp \
        ..\COMMethodRental.cpp \
        ..\COMMember.cpp \
        ..\COMMutex.cpp \
        ..\COMNumber.cpp \
        ..\COMModule.cpp \
        ..\COMNDirect.cpp \
        ..\COMObject.cpp \
        ..\COMReflectionCommon.cpp \
        ..\COMSecurityConfig.cpp \
        ..\COMSecurityRuntime.cpp \
        ..\COMStreams.cpp \
        ..\COMString.cpp \
        ..\COMStringBuffer.cpp \
        ..\COMSynchronizable.cpp \
        ..\COMSystem.cpp \
        ..\COMThreadPool.cpp \
        ..\COMUtilNative.cpp \
        ..\COMVariant.cpp \
        ..\COMVarArgs.cpp \
        ..\COMWaitHandle.cpp \
        ..\CorHost.cpp \
        ..\CorMap.cpp      \
        ..\CustomerDebugHelper.cpp \
        ..\CustomMarshalerInfo.cpp \
        ..\crst.cpp        \
        ..\contexts.cpp \
        ..\CustomAttribute.cpp \
        ..\DebugDebugger.cpp \
        ..\debugHelp.cpp \
        ..\dump-tables.cpp \
        ..\ecall.cpp       \
        ..\eeconfig.cpp \
        ..\eeconfigfactory.cpp \
        ..\eehash.cpp\
        ..\EJitMgr.cpp\
        ..\EventStore.cpp  \
        ..\excep.cpp       \
        ..\fcall.cpp       \
        ..\Field.cpp \
        ..\Fjit_EETwain.cpp \
        ..\frames.cpp      \
        ..\fusionwrap.cpp \
        ..\gcEE.cpp \
        ..\gchost.cpp \
        ..\gcscan.cpp \
        ..\gmheap.cpp \
        ..\handletable.cpp \
        ..\handletableCache.cpp \
        ..\handletableCore.cpp \
        ..\handletableScan.cpp \
        ..\hash.cpp \
        ..\InteropConverter.cpp \
        ..\interoputil.cpp \
        ..\InvokeUtil.cpp \
        ..\JITInterface.cpp \
        ..\list.cpp \
        ..\ListLock.cpp \
        ..\memorypool.cpp \
        ..\message.cpp \
        ..\Method.cpp \
        ..\MethodImpl.cpp \
        ..\ml.cpp \
        ..\mlcache.cpp \
        ..\mlgen.cpp \
        ..\mlinfo.cpp \
        ..\NativeOverlapped.cpp \
        ..\ndirect.cpp \
        ..\nexport.cpp \
        ..\nstruct.cpp \
        ..\object.cpp \
        ..\objecthandle.cpp \
        ..\OleVariant.cpp \
        ..\PEFile.cpp      \
        ..\permset.cpp \
        ..\PEVerifier.cpp \
        ..\Prestub.cpp \
        ..\ProfToEEInterfaceImpl.cpp \
        ..\rangetree.cpp \
        ..\ReflectClassWriter.cpp \
        ..\ReflectUtil.cpp \
        ..\ReflectWrap.cpp \
        ..\remoting.cpp \
        ..\rwlock.cpp \
        ..\SecurityDB.cpp \
        ..\security.cpp \
        ..\siginfo.cpp \
        ..\SigFormat.cpp \
        ..\SimpleRWLock.cpp \
        ..\spinlock.cpp \
        ..\StackingAllocator.cpp \
        ..\stackwalk.cpp \
        ..\StackBuilderSink.cpp \
        ..\stublink.cpp \
        ..\StringLiteralMap.cpp \
        ..\stubmgr.cpp \
        ..\syncblk.cpp \
        ..\SyncClean.cpp \
        ..\expandSig.cpp \
        ..\threads.cpp \
        ..\tls.cpp \
        ..\tpoolwrap.cpp \
        ..\typehash.cpp \
        ..\util.cpp \
        ..\validator.cpp \
        ..\vars.cpp \
        ..\verifier.cpp \
        ..\versig.cpp \
        ..\Win32Threadpool.cpp


#============================================================
# Was the file you added to the list inserted in ALPHABETICAL
# order - if not please rerun the ALPHABETIZE IMA!
#============================================================

