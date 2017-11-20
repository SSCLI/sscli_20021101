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

MAJORCOMP=fusion
MINORCOMP=asmcache

TARGETNAME=urtasmc

TARGETTYPE=LIBRARY
TARGETPATHLIB=$(FUSION_LIB_DEST)


SOURCES= \
    asmcache.cpp \
    asmint.cpp   \
    asmitem.cpp  \
    asmstrm.cpp  \
    asmenum.cpp  \
    transprt.cpp \
    cache.cpp    \
    scavenger.cpp \
    enum.cpp      \
    cacheUtils.cpp

