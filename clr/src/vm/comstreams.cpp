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
** Class:  COMStreams
**
**                                                    
**
** Purpose: Streams native implementation
**
** Date:  June 29, 1998
** 
===========================================================*/
#include "common.h"
#include "excep.h"
#include "object.h"
#include "comstreams.h"
#include "field.h"
#include "eeconfig.h"
#include "comstring.h"

union FILEPOS {
    struct {
        UINT32 posLo;
        INT32 posHi;
    } u;
    UINT64 pos;
};


FCIMPL0(BOOL, COMStreams::RunningOnWinNT)
    FC_GC_POLL_RET();
    return ::RunningOnWinNT();
FCIMPLEND


FCIMPL7(UINT32, COMStreams::BytesToUnicode, UINT codePage, U1Array* byteArray, UINT byteIndex, \
        UINT byteCount, CHARArray* charArray, UINT charIndex, UINT charCount)
    _ASSERTE(byteArray);
    _ASSERTE(byteIndex >=0);
    _ASSERTE(byteCount >=0);
    _ASSERTE(charIndex == 0 || (charIndex > 0 && charArray != NULL));
    _ASSERTE(charCount == 0 || (charCount > 0 && charArray != NULL));

    const char * bytes = (const char*) byteArray->GetDirectConstPointerToNonObjectElements();
    INT32 ret;

    if (charArray != NULL)
    {
        WCHAR* chars = (WCHAR*) charArray->GetDirectPointerToNonObjectElements();
        return WszMultiByteToWideChar(codePage, 0, bytes + byteIndex, 
            byteCount, chars + charIndex, charCount);
    }
    else 
        ret = WszMultiByteToWideChar(codePage, 0, bytes + byteIndex, byteCount, NULL, 0);

    FC_GC_POLL_RET();
    return ret;
FCIMPLEND


FCIMPL7(UINT32, COMStreams::UnicodeToBytes, UINT codePage, CHARArray* charArray, UINT charIndex, \
        UINT charCount, U1Array* byteArray, UINT byteIndex, UINT byteCount /*, LPBOOL lpUsedDefaultChar*/)
    _ASSERTE(charArray);
    _ASSERTE(charIndex >=0);
    _ASSERTE(charCount >=0);
    _ASSERTE(byteIndex == 0 || (byteIndex > 0 && byteArray != NULL));
    _ASSERTE(byteCount == 0 || (byteCount > 0 && byteArray != NULL));

    // WARNING: There's a bug in the OS's WideCharToMultiByte such that if you pass in a 
	// non-null lpUsedDefaultChar and a code page for a "DLL based encoding" (vs. a table 
    // based one?), WCtoMB will fail and GetLastError will give you E_INVALIDARG.               
    //                           This is not good, so I've removed the parameter here to avoid the 
    // problem.                                         
    //_ASSERTE(!(codePage == CP_UTF8 && lpUsedDefaultChar != NULL));

    const WCHAR * chars = (const WCHAR*) charArray->GetDirectConstPointerToNonObjectElements();
    INT32 ret;
    if (byteArray != NULL)
    {
        char* bytes = (char*) byteArray->GetDirectPointerToNonObjectElements();
        ret = WszWideCharToMultiByte(codePage, 0, chars + charIndex, charCount, bytes + byteIndex, byteCount, 0, NULL/*lpUsedDefaultChar*/);
    } 
    else 
        ret = WszWideCharToMultiByte(codePage, 0, chars + charIndex, charCount, NULL, 0, 0, 0);

    FC_GC_POLL_RET();
    return ret;
FCIMPLEND


FCIMPL0(INT, COMStreams::ConsoleInputCP)
{
    return GetConsoleCP();
}
FCIMPLEND

FCIMPL0(INT, COMStreams::ConsoleOutputCP)
{
    return GetConsoleOutputCP();
}
FCIMPLEND


FCIMPL1(INT32, COMStreams::GetCPMaxCharSize, UINT codePage)
{
    THROWSCOMPLUSEXCEPTION();
    CPINFO cpInfo;

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    if (!GetCPInfo(codePage, &cpInfo)) 
    {
        // CodePage is either invalid or not installed.
        // However, on NT4, UTF-7 & UTF-8 aren't defined.
        if (codePage == CP_UTF8)
        {
            cpInfo.MaxCharSize = 4;
        }
        else if (codePage == CP_UTF7)
        {
            cpInfo.MaxCharSize = 5;
        }
        else
        {
        COMPlusThrow(kArgumentException, L"Argument_InvalidCP");
    }
    }

    HELPER_METHOD_FRAME_END();

    return cpInfo.MaxCharSize;
}
FCIMPLEND

FCIMPL1(INT, COMStreams::ConsoleHandleIsValid, HANDLE handle)
{
    // Do NOT call this method on stdin!

    // Windows apps may have non-null valid looking handle values for stdin, stdout
    // and stderr, but they may not be readable or writable.  Verify this by 
    // calling ReadFile & WriteFile in the appropriate modes.
    // This must handle VB console-less scenarios & WinCE.
    if (handle==INVALID_HANDLE_VALUE)
        return FALSE;  // WinCE should return here.
    DWORD bytesWritten;
    BYTE junkByte = 0x41;
    BOOL bResult;
    bResult = WriteFile(handle, (LPCVOID) &junkByte, 0, &bytesWritten, NULL);
    // In Win32 apps w/ no console, bResult should be 0 for failure.
    return bResult != 0;
}
FCIMPLEND


FCIMPL7(LPVOID, COMStreams::GetFullPathHelper, 
                    StringObject*   pathUNSAFE, 
                    CHARArray*      invalidCharsUNSAFE, 
                    CHARArray*      whitespaceCharsUNSAFE, 
                    DWORD           directorySeparator, 
                    DWORD           altDirectorySeparator, 
                    BYTE            fullCheck, 
                    STRINGREF*      newPath)
{
    DWORD retval;

    THROWSCOMPLUSEXCEPTION();

    HELPER_METHOD_FRAME_BEGIN_RET_0();

    struct _gc
{
        STRINGREF       path;
        CHARARRAYREF    invalidChars;
        CHARARRAYREF    whitespaceChars;
    } gc;

    gc.path             = (STRINGREF)    pathUNSAFE;
    gc.invalidChars     = (CHARARRAYREF) invalidCharsUNSAFE;
    gc.whitespaceChars  = (CHARARRAYREF) whitespaceCharsUNSAFE;

    GCPROTECT_BEGIN(gc);

    size_t pathLength = gc.path->GetStringLength();

    if (pathLength >= MAX_PATH) // CreateFile freaks out for a path of 260. Only upto 259 works fine.
        COMPlusThrow( kPathTooLongException, IDS_EE_PATH_TOO_LONG );

    size_t numInvalidChars      = gc.invalidChars->GetNumComponents();
    WCHAR* invalidCharsBuffer   = gc.invalidChars->GetDirectPointerToNonObjectElements();
    size_t numWhiteChars        = gc.whitespaceChars->GetNumComponents();
    WCHAR* whiteSpaceBuffer     = gc.whitespaceChars->GetDirectPointerToNonObjectElements();
    WCHAR* pathBuffer           = gc.path->GetBuffer();
    WCHAR newBuffer[MAX_PATH+1];
    WCHAR finalBuffer[MAX_PATH+1];

    size_t numSpaces = 0;
    bool fixupDirectorySeparator = false;
	bool fixupDotSeparator = true;
    size_t numDots = 0;
    size_t newBufferIndex = 0;
    size_t index = 1;

    // We need to trim whitespace off the end of the string.
    // To do this, we'll just start walking at the back of the
    // path looking for whitespace and stop when we're done.

    if (fullCheck)
    {
        for (; pathLength > 0; --pathLength)
        {
            bool foundMatch = false;

            for (size_t whiteIndex = 0; whiteIndex < numWhiteChars; ++whiteIndex)
            {
                if (pathBuffer[pathLength-1] == whiteSpaceBuffer[whiteIndex])
                {
                    foundMatch = true;
                    break;
                }
            }

            if (!foundMatch)
                break;
        }
    }

    if (pathLength == 0)
        COMPlusThrow( kArgumentException, IDS_EE_PATH_ILLEGAL );

   

    // Can't think of a no good way to do this in 1 loop
	// Do the argument validation in the first loop.
	for (; index < pathLength; ++index)
    {
        WCHAR currentChar = pathBuffer[index];

        // Check for invalid characters by iterating through the
        // provided array and looking for matches.

        if (fullCheck)
        {
            for (size_t invalidIndex = 0; invalidIndex < numInvalidChars; ++invalidIndex)
            {
                if (currentChar == invalidCharsBuffer[invalidIndex])
                    COMPlusThrow( kArgumentException, IDS_EE_PATH_HAS_IMPROPER_CHAR );
            }
        }
	}

	index = 0;
#if !PLATFORM_UNIX
	// Fixup for Win9x since //server/share becomes c://server/share
	if (pathBuffer[0] == L'/' || pathBuffer[0] == L'\\')
	{
		newBuffer[newBufferIndex++] = L'\\';
		index++;
	}
#endif
   
	while (index < pathLength)
    {
        WCHAR currentChar = pathBuffer[index];

        // We handle both directory separators and dots specially.  For directory
        // separators, we consume consecutive appearances.  For dots, we consume
        // all dots beyond the second in succession.  All other characters are
        // added as is.  If addition we consume all spaces after the last other
        // character in a directory name up until the directory separator.

        if (currentChar == (WCHAR)directorySeparator || currentChar == (WCHAR)altDirectorySeparator)
        {
			// We may be looking at abc.../foo and we should simply copy the characterss or ..../foo in which case we should reduce
			if (fixupDotSeparator)
			{
				if (numDots > 2)
					numDots = 2; // reduce mutliple dots to 2 dots
			}
			for (size_t count = 0; count < numDots; ++count)
			{
				newBuffer[newBufferIndex++] = '.';
			}

            numSpaces = 0;
            numDots = 0;
			fixupDotSeparator = true; 

            if (!fixupDirectorySeparator)
            {
                fixupDirectorySeparator = true;
                newBuffer[newBufferIndex++] = directorySeparator;
            }
        }
        else if (currentChar == L'.' && fixupDotSeparator) // Reduce only multiple .'s only after slash to 2 dots. For instance a...b is a valid file name.
        {
				numDots++;
			fixupDirectorySeparator = false;
            numSpaces = 0;
        }
        else 
		{
	  	    fixupDirectorySeparator = false;

			if (currentChar == ' ')
			{
				numSpaces ++;
			}
			else 
			{ 
				// Flush pending dots since we've looked at first non-dot character
   				for (size_t count = 0; count < numDots; ++count)
           		{
					newBuffer[newBufferIndex++] = '.';
   				}
				numDots = 0;
  				fixupDotSeparator = false;

				// Flush non-terminal spaces in a section of the path.
               	for (size_t count = 0; count < numSpaces; ++count)
                {
      	            newBuffer[newBufferIndex++] = ' ';
               	}
                numSpaces = 0;
            newBuffer[newBufferIndex++] = currentChar;
        }
	    }

		index++;
    }

    // We may be looking at abc... and we should simply copy the characterss or .... in which case we should reduce
	if (fixupDotSeparator)
	{
		if (numDots > 2)
			numDots = 2; // reduce mutliple dots to 2 dots
	}
	// If we missed some left-over dots like in .... or abc...
    for (size_t count = 0; count < numDots; ++count)
    {
	    newBuffer[newBufferIndex++] = '.';
	}

    // If we ended up eating all the characters, bail out.
    if (newBufferIndex == 0)
        COMPlusThrow( kArgumentException, IDS_EE_PATH_ILLEGAL );
    
    newBuffer[newBufferIndex] = L'\0';

	/* Throw an ArgumentException for paths like \\, \\server, \\server\ */
	if (newBuffer[0] == L'\\' && newBuffer[1] == L'\\') {
		size_t startIndex = 2;
		while (startIndex < newBufferIndex) {
			if (newBuffer[startIndex] == L'\\') {
				startIndex++;
				break;
			}
			else {
				startIndex++;
			}
		}
		if (startIndex == newBufferIndex) {
			 COMPlusThrow( kArgumentException, IDS_EE_PATH_INVALID_UNCPATH );
		}
	}

    _ASSERTE( newBufferIndex <= MAX_PATH && "Overflowed temporary path buffer" );

    // Call the Win32 API to do the final canonicalization step.

    WCHAR* name;
    DWORD result;

    if (fullCheck)
    {
        result = WszGetFullPathName( newBuffer, MAX_PATH + 1, finalBuffer, &name );
    }
    else
    {
        // This just needs to be some non-zero number that is less than MAX_PATH.
        result = 1;
    }

    // Check our result and form the managed string as necessary.

	if (result >= MAX_PATH)
        COMPlusThrow( kPathTooLongException, IDS_EE_PATH_TOO_LONG );

    if (result == 0)
    {
        retval = GetLastError();
        *newPath = NULL;
    }
    else
    {
        retval = 0;

        if (fullCheck)
            *newPath = COMString::NewString( finalBuffer );
        else
            *newPath = COMString::NewString( newBuffer );
    }

    GCPROTECT_END();

    HELPER_METHOD_FRAME_END();


    RETURN( retval, DWORD );
}
FCIMPLEND
    




