/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    mbstring.c

Abstract:

    Implementation of the multi-byte string functions in the C runtime library that 
    are Windows specific.

Implementation Notes:

    Assuming it is not possible to change multi-byte code page using
    the PAL (_setmbcp does not seem to be required), these functions
    should have a trivial implementation (treat as single-byte). If it
    is possible, then support for multi-byte code pages will have to
    be implemented before these functions can behave correctly for
    multi-byte strings.

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"


SET_DEFAULT_DEBUG_CHANNEL(CRT);


/*++
Function:
  _mbslen

Determines the number of characters (code points) in a multibyte
character string.

Parameters

string  Points to a multibyte character string.

Return Values

The mbslen subroutine returns the number of multibyte characters in a
multibyte character string. It returns 0 if the string parameter
points to a null character or if a character cannot be formed from the
string pointed to by this parameter.

--*/
size_t 
__cdecl
_mbslen(
        const unsigned char *string)
{
    size_t ret = 0;
    CPINFO cpinfo;
    ENTRY("_mbslen (string=%p (%s))\n", string, string);

    if (string)
    {
        if (GetCPInfo(CP_ACP, &cpinfo) && cpinfo.MaxCharSize == 1)
        {
            ret = strlen(string);
        }
        else
        {
            while (*string)
            {
                if (IsDBCSLeadByteEx(CP_ACP, *string))
                {
                    ++string;
                }
                ++string;
                ++ret;
            }
        }
    }

    LOGEXIT("_mbslen returning size_t %u\n", ret);
    return ret;
}

/*++
Function:
  _mbsinc

Return Value

Returns a pointer to the character that immediately follows string.

Parameter

string  Character pointer

Remarks

The _mbsinc function returns a pointer to the first byte of the
multibyte character that immediately follows string.

--*/
unsigned char *
__cdecl
_mbsinc(
        const unsigned char *string)
{
    unsigned char *ret;

    ENTRY("_mbsinc (string=%p)\n", string);

    if (string == NULL)
    {
        ret = NULL;
    }
    else
    {
        ret = (unsigned char *) string;
        if (IsDBCSLeadByteEx(CP_ACP, *ret))
        {
            ++ret;
        }
        ++ret;
    }

    LOGEXIT("_mbsinc returning unsigned char* %p (%s)\n", ret, ret);
    return ret;
}


/*++
Function:
  _mbsninc

Return Value

Returns a pointer to string after string has been incremented by count
characters, or NULL if the supplied pointer is NULL. If count is
greater than or equal to the number of characters in string, the
result is undefined.

Parameters

string  Source string
count   Number of characters to increment string pointer

Remarks

The _mbsninc function increments string by count multibyte
characters. _mbsninc recognizes multibyte-character sequences
according to the multibyte code page currently in use.

--*/
unsigned char *
__cdecl
_mbsninc(
         const unsigned char *string, size_t count)
{
    unsigned char *ret;
    CPINFO cpinfo;

    ENTRY("_mbsninc (string=%p, count=%d)\n", string, count);
    if (string == NULL || count < 0 )
    {
        ret = NULL;
    }
    else
    {
        ret = (unsigned char *) string;
        if (GetCPInfo(CP_ACP, &cpinfo) && cpinfo.MaxCharSize == 1)
        {
            ret += min(count, strlen(string));
        }
        else
        {
            while (count-- && (*ret != 0))
            {
                if (IsDBCSLeadByteEx(CP_ACP, *ret))
                {
                    ++ret;
                }
                ++ret;
            }
        }
    }
    LOGEXIT("_mbsninc returning unsigned char* %p (%s)\n", ret, ret);
    return ret;
}

/*++
Function:
  _mbsdec

Return Value

_mbsdec returns a pointer to the character that immediately precedes
current; _mbsdec returns NULL if the value of start is greater than or
equal to that of current.

Parameters

start    Pointer to first byte of any multibyte character in the source
         string; start must precede current in the source string

current  Pointer to first byte of any multibyte character in the source
         string; current must follow start in the source string

--*/
unsigned char * 
__cdecl
_mbsdec(
        const unsigned char *start, 
        const unsigned char *current)
{
    unsigned char *ret;
    unsigned char *strPtr;
    CPINFO cpinfo;

    ENTRY("_mbsdec (start=%p, current=%p)\n", start, current);

    if (current <= start)
    {
        ret = NULL;
    }
    else if (GetCPInfo(CP_ACP, &cpinfo) && cpinfo.MaxCharSize == 1)
    {
        ret = (unsigned char *) current - 1;
    }
    else
    {
        ret = strPtr = (unsigned char *) start;
        while (strPtr < current)
        {
            ret = strPtr;
            if (IsDBCSLeadByteEx(CP_ACP, *strPtr))
            {
                ++strPtr;
            }
            ++strPtr;
        }
    }
    LOGEXIT("_mbsdec returning unsigned int %p (%s)\n", ret, ret);
    return ret;
}
