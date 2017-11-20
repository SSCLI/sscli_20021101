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
/*============================================================
**
** Header:  COMStreams.h
**       
**
** Purpose: Native implementation for System.IO
**
** Date:  June 29, 1998
** 
===========================================================*/

#ifndef _COMSTREAMS_H
#define _COMSTREAMS_H

#include "fcall.h"

class COMStreams {

  public:
	// Used by CodePageEncoding
	static FCDECL7(UINT32, BytesToUnicode, UINT codePage, U1Array* bytes0, UINT byteIndex, \
				   UINT byteCount, CHARArray* chars0, UINT charIndex, UINT charCount);
	static FCDECL7(UINT32, UnicodeToBytes, UINT codePage, CHARArray* chars0, UINT charIndex, \
				   UINT charCount, U1Array* bytes0, UINT byteIndex, UINT byteCount/*, LPBOOL lpUsedDefaultChar*/);
    static FCDECL1(INT32, GetCPMaxCharSize, UINT codePage);
    static FCDECL7(LPVOID, GetFullPathHelper, 
                        StringObject*   path, 
                        CHARArray*      invalidChars, 
                        CHARArray*      whitespaceChars, 
                        DWORD           directorySeparator, 
                        DWORD           altDirectorySeparator, 
                        BYTE            fullCheck, 
                        STRINGREF*      newPath);

    static FCDECL0(BOOL, RunningOnWinNT);

    // Used by Console.
    static FCDECL1(INT, ConsoleHandleIsValid, HANDLE handle);
	static FCDECL0(INT, ConsoleInputCP);
	static FCDECL0(INT, ConsoleOutputCP);
};

#endif
