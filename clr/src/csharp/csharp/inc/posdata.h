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
// ===========================================================================
// File: posdata.h
//
// ===========================================================================

#ifndef _POSDATA_H_
#define _POSDATA_H_

#define MAX_POS_LINE_LEN    2046    // One less to distinguish from -1
#define MAX_POS_LINE        2097151 //  "   "    "    "         "

////////////////////////////////////////////////////////////////////////////////
// POSDATA
//
// This miniscule class holds line and character index data IN 32 BITS.
// POSDATA's can be compared in stream-oriented fashion, since they always
// represent a single character in the stream.

struct POSDATA
{
    unsigned long   iChar:11;       // 2047 characters per line max
    unsigned long   iLine:21;       // 2097152 lines per file max

    POSDATA () {}
    POSDATA (long i) : iChar(0xFFFFFFFF), iLine(0xFFFFFFFF) { ASSERT (i == -1); }
    POSDATA (const POSDATA &p) { iLine = p.iLine; iChar = p.iChar; }
    POSDATA (long i, long c) { iLine = i; iChar = c; }
    BOOL    operator < (const POSDATA &p) const {
        return (iLine < p.iLine || (iLine == p.iLine && iChar < p.iChar));
    }
    BOOL    operator > (const POSDATA &p) const {
        return (iLine > p.iLine || (iLine == p.iLine && iChar > p.iChar));
    }
    BOOL    operator <= (const POSDATA &p) const {
        return (*this < p || *this == p);
    }
    BOOL    operator >= (const POSDATA &p) const {
        return (*this > p || *this == p);
    }
    BOOL    operator == (const POSDATA &p) const {
        return (iLine == p.iLine && iChar == p.iChar);
    }
    BOOL    operator != (const POSDATA &p) const {
        return (iLine != p.iLine || iChar != p.iChar);
    }

    void    SetEmpty () { iLine = 0xFFFFFFFF; iChar = 0xFFFFFFFF; }
    BOOL    IsEmpty () const {
        return (iLine == 0x1FFFFF && iChar == 0x7FF);
    }

    static  long FindNearestPosition (POSDATA *ppos, long iSize, const POSDATA &pos);
};

#endif  // _POSDATA_H_
