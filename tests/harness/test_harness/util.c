/*============================================================
**
** Source: util.c
**
** Purpose: Contains miscellaneous helper functions.
**
** 
**  Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
** 
**  The use and distribution terms for this software are contained in the file
**  named license.txt, which can be found in the root of this distribution.
**  By using this software in any fashion, you are agreeing to be bound by the
**  terms of this license.
** 
**  You must not remove this notice, or any other, from this software.
** 
**
**=========================================================*/

#include <stdarg.h>

#include "testharness.h"
#include "util.h"

/*
 * Takes a string representing a type and returns the equivalent test type 
 * enum.  The string must match exactly (case sensitive, no whitespace 
 * padding).
 */
TEST_TYPE StringToType(char *TypeString)
{

    if (strcmp(TypeString, TSTTYPE_DEFAULT) == 0)
    {
        return DEFAULT_TYPE;
    }
    else if (strcmp(TypeString, TSTTYPE_CLIENT) == 0)
    {
        return CLIENT_TYPE;
    }
    else if (strcmp(TypeString, TSTTYPE_SERVER) == 0)
    {
        return SERVER_TYPE;
    }
    else if (strcmp(TypeString, TSTTYPE_CLNTSRV) == 0)
    {
        return CLNTSRV_TYPE;
    }

    return UNKNOWN_TYPE;
}


/*
 * Returns a pointer to the type string for a given test type enum.
 */
char *TypeToString(TEST_TYPE Type)
{
    if (Type == DEFAULT_TYPE)
    {
        return TSTTYPE_DEFAULT;
    }
    else if (Type == CLIENT_TYPE)
    {
        return TSTTYPE_CLIENT;
    }
    else if (Type == SERVER_TYPE)
    {
        return TSTTYPE_SERVER;
    }
    else if (Type == CLNTSRV_TYPE)
    {
        return TSTTYPE_CLNTSRV;
    }
    else
    {
        return TSTTYPE_UNKNOWN;
    }

}

/*
 * Takes a variable number of strings and copies them to a buffer with a 
 * specific maximum size.  If the buffer is too small, 1 is returned and the 
 * contents of the buffer should be considered undefined.  If the function 
 * succeeds 0 is returned.
 *
 * NOTE: the variable argument list must be terminated with a NULL string!
 */
int AppendStringsVA(char *Buffer, int MaxLength, ...)
{
    char *ptr;
    char *component;
    va_list arglist;
    int len;

    va_start(arglist, MaxLength);

    ptr = Buffer;
    component = va_arg(arglist, char*);
    while (component != NULL)
    {
        len = strlen(component);
        if (len >= MaxLength)
        {
            return 1;
        }

        strcpy(ptr, component);
        ptr += len;        


        MaxLength -= len;

        component = va_arg(arglist, char*);
    }

    return 0;
}

/*
 * Takes a variable number of strings and copies them to a buffer with a 
 * specific maximum size, delimited by the DelimChar.  For each string to be 
 * copied to the buffer, each character of it that matches the DelimChar or 
 * the EscapeChar is prefixed with EscapeChar when copied.  If the buffer is 
 * too small, 1 is returned and the contents of the buffer should be 
 * considered undefined.  If the function succeeds 0 is returned.
 *
 * NOTE: the variable argument list must be terminated with a NULL string!
 */
int AppendDelimitedStringsVA(char *Buffer, int MaxLength, char DelimChar,
                             char EscapeChar, ...)
{
    char *component;
    va_list arglist;
    char *ptr;
    int i;

    va_start(arglist, EscapeChar);

    ptr = Buffer;
    component = va_arg(arglist, char*);
    while (component != NULL)
    {
        i = 0;

        while (component[i] != '\0')
        {
            if (component[i] == EscapeChar || 
                component[i] == DelimChar)
            {
                if (MaxLength-- < 1)
                {
                    return 1;
                }
                *ptr++ = EscapeChar;
            }

            if (MaxLength-- < 1)
            {
                return 1;
            }
            *ptr++ = component[i++];
        }

        if (MaxLength-- < 1)
        {
            return 1;
        }
        *ptr++ = DelimChar;

        component = va_arg(arglist, char*);
    }

    if (MaxLength <= 0)
    {
        return 1;
    }

    *ptr = '\0';
    return 0;
}


/*
 * Reads in a line of text from the given input file, using ReadBuf as an 
 * intermediate store between calls.  The line of text is stored in Line.
 * On the first call, ReadBuf should be a zero length string 
 * (ReadBuf[0] = '\0').  Returns 0 on success.
 *
 * ReadBuf is assumed to be READ_BUF_SIZE bytes
 * Line is assumed to be LINE_BUF_SIZE bytes
 */
/*
int ReadLine(FILE *InputFile, char *ReadBuf, char *Line)
{
    int l, iFullLen;
    int iLen, rmaindr;
    char *NewlineChar;
    int ErrFlag = 0;
  
    Line[0] = 0;
    iFullLen = strlen(ReadBuf);
  
    if (iFullLen < LINE_BUF_SIZE)
    {
        l = fread(ReadBuf + iFullLen, 1, READ_BUF_SIZE - iFullLen, InputFile);
        iFullLen += l;
    }
  
    if (iFullLen == 0)
    {
        return ErrFlag;
    }
  
    NewlineChar = strchr(ReadBuf, '\n');
    if (NewlineChar == 0)
    {
        iLen = strlen(ReadBuf);
    }
    else
    {
        iLen = NewlineChar - ReadBuf;
    }

    if (NewlineChar == ReadBuf)
    {
        ErrFlag = 1;
    }

    if (iLen < LINE_BUF_SIZE)
    {
        // Only copy the line if it will fit in the line buffer. /
        strncpy(Line, ReadBuf, iLen);
        Line[iLen] = 0;
    }
    else
    {
        // Set the error flag and continue /
        ErrFlag = 1;
    }
  
    rmaindr =  iFullLen - (iLen + 1);
    if (rmaindr > 0)
    {
        memmove(ReadBuf, ReadBuf+iLen+1, rmaindr);
        memset(ReadBuf + rmaindr, 0, READ_BUF_SIZE - rmaindr);
    }
    else
    {
        memset(ReadBuf, 0, READ_BUF_SIZE);
    }

    if (ErrFlag == 0)
    {
        if (Line[iLen-1] == '\r')
        {
            Line[iLen-1] = 0;
        }
    }
  
    return ErrFlag;
}
*/

/*
 * Reads in a line of text from the given input file. 
 * The line of text is stored in pLine. 
 *
 * Returns: 0   on success.
 *          EOF on end of file
 *          -1  on error
 *
 * Line is assumed to be LINE_BUF_SIZE bytes
 */
int ReadLineEx(FILE *pInputFile, char *pLine)
{
    char *pFeed;
    char szTempLine[LINE_BUF_SIZE] = "";
    int nSize;

    if( NULL == fgets(szTempLine, 
                      LINE_BUF_SIZE, 
                      pInputFile))
    { /* ERROR */
        if( feof(pInputFile) )
        {   
            return EOF;
        }
        else
        {
            return -1;
        }
    }
    /* Remove \n from line */   
    nSize = strlen( szTempLine );
    pFeed = strchr( szTempLine, '\n' );

    if( pFeed != NULL )
    {
        nSize = (int)(pFeed - szTempLine);
    }

    /* Reset szLine with the new line value */

    memset( pLine, 0, LINE_BUF_SIZE );
    
    if( NULL == strncpy( pLine, szTempLine, nSize ) )
    { /*ERROR*/
        return -1;
    }

    return 0;
}

/**
 * TrimWhiteSpace
 *
 * Removes all heading and trailing whitespaces, \n, \r, \t 
 * from buffer
 *
 */
void TrimWhiteSpace(char *szBuf)
{
    int idx = 0;
    int len = 0;

    len = strlen(szBuf);

    while (szBuf[idx] == ' ' ||  szBuf[idx] == '\n' ||
           szBuf[idx] == '\r' || szBuf[idx] == '\t')
    {
        idx++;
    }

    memmove(szBuf, szBuf+idx, len-idx+1);
    len -= idx;

    idx = len - 1;
    while (szBuf[idx] == ' ' ||  szBuf[idx] == '\n' ||
           szBuf[idx] == '\r' || szBuf[idx] == '\t')
    {
        szBuf[idx] = 0;
        idx--;
    }
}
