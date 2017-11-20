/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    string.c

Abstract:

    Implementation of the string functions in the C runtime library that Windows specific.

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include "pal/cruntime.h"
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>

SET_DEFAULT_DEBUG_CHANNEL(CRT);

/*
 * This is the key in thread local storage of the per-thread strtok_r context
 *  pointer, for use in PAL_strtok. 
 * It is initialized and freed in PAL_Initialize() and
 * PAL_Terminate, respectively
 */
static pthread_key_t strtokKey = PTHREAD_KEYS_MAX;



/*++

Function:
  StrtokInitialize (internal)

Initialize the thread local storage used for the context pointer
in strtok

Returns:
   TRUE    success
   FALSE   failure

--*/
BOOL StrtokInitialize(void)
{
    int ret;

    if((PTHREAD_KEYS_MAX != strtokKey) || (PTHREAD_KEYS_MAX != wcstokKey))
    {
        ASSERT("strtok/wcstok initialization has already been done!");
        return FALSE;
    }

    if ((ret = pthread_key_create(&strtokKey, NULL)) != 0)
    {
       ERROR("pthread_key_create() failed error:%d (%s)\n",
              ret, strerror(ret));
       return FALSE;
    }

    if ((ret = pthread_key_create(&wcstokKey, NULL)) != 0)
    {
        ERROR("pthread_key_create() failed error:%d (%s)\n",
               ret, strerror(ret));
        
        if ((ret = pthread_key_delete(strtokKey)) != 0)
        {
            ERROR("pthread_key_delete() failed error:%d (%s)\n",
                   ret, strerror(ret));
        }
        return FALSE;
    }
    
    return TRUE;
}


/*++

Function:
  StrtokCleanup (internal)

Free the thread local storage used for the context pointer
in strtok

--*/
void StrtokCleanup(void)
{
    int ret;

    if (PTHREAD_KEYS_MAX == strtokKey || PTHREAD_KEYS_MAX == wcstokKey)
    {
        ASSERT("strtok/wcstok TLS storage isn't initialized\n");
        return;
    }
    
    if ((ret = pthread_key_delete(strtokKey)) != 0)
    {
        ERROR("pthread_key_delete() failed error:%d (%s)\n",
               ret, strerror(ret));
    }
    if ((ret = pthread_key_delete(wcstokKey)) != 0)
    {
        ERROR("pthread_key_delete() failed error:%d (%s)\n",
               ret, strerror(ret));
    }
}


/*++
Function:
  _strnicmp

compare at most count characters from two strings, ignoring case

The strnicmp() function compares, with case insensitivity, at most count
characters from s1 to s2. All uppercase characters from s1 and s2 are 
mapped to lowercase for the purposes of doing the comparison.

Returns:

Value Meaning

< 0   s1 is less than s2 
0     s1 is equal to s2 
> 0   s1 is greater than s2

--*/
int 
__cdecl
_strnicmp( const char *s1, const char *s2, size_t count )
{
    int ret;

    ENTRY("_strnicmp (s1=%p (%s), s2=%p (%s), count=%d)\n", s1?s1:"NULL", s1?s1:"NULL", s2?s2:"NULL", s2?s2:"NULL", count);

    ret = strncasecmp(s1, s2, count );

    LOGEXIT("_strnicmp returning int %d\n", ret);
    return ret;
}

/*++
Function:
  _stricmp

compare two strings, ignoring case

The stricmp() function compares, with case insensitivity, the string
pointed to by s1 to the string pointed to by s2. All uppercase
characters from s1 and s2 are mapped to lowercase for the purposes of
doing the comparison.

Returns:

Value Meaning

< 0   s1 is less than s2 
0     s1 is equal to s2 
> 0   s1 is greater than s2

--*/
int 
__cdecl
_stricmp(
         const char *s1, 
         const char *s2)
{
    int ret;

    ENTRY("_stricmp (s1=%p (%s), s2=%p (%s))\n", s1?s1:"NULL", s1?s1:"NULL", s2?s2:"NULL", s2?s2:"NULL");

    ret = strcasecmp(s1, s2);

    LOGEXIT("_stricmp returning int %d\n", ret);
    return ret;
}


/*++
Function:
  _strlwr

Convert a string to lowercase.


This function returns a pointer to the converted string. Because the
modification is done in place, the pointer returned is the same as the
pointer passed as the input argument. No return value is reserved to
indicate an error.

Parameter

string  Null-terminated string to convert to lowercase

Remarks

The _strlwr function converts any uppercase letters in string to
lowercase as determined by the LC_CTYPE category setting of the
current locale. Other characters are not affected. For more
information on LC_CTYPE, see setlocale.

--*/
char *  
__cdecl
_strlwr(
        char *str)
{
    char *orig = str;

    ENTRY("_strlwr (str=%p (%s))\n", str?str:"NULL", str?str:"NULL");

    while (*str)
    {
        *str = tolower(*str);
        str++;
    } 
   
    LOGEXIT("_strlwr returning char* %p (%s)\n", orig?orig:"NULL", orig?orig:"NULL");
    return orig;
}


/*++
Function:
  _swab

Swaps bytes.

Return Value

None

Parameters

src        Data to be copied and swapped
dest       Storage location for swapped data
n          Number of bytes to be copied and swapped

Remarks

The _swab function copies n bytes from src, swaps each pair of
adjacent bytes, and stores the result at dest. The integer n should be
an even number to allow for swapping. _swab is typically used to
prepare binary data for transfer to a machine that uses a different
byte order.

Example

char from[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char   to[] = "..........................";

printf("Before:\n%s\n%s\n\n", from, to);
_swab(from, to, strlen(from));
printf("After:\n%s\n%s\n\n", from, to);

Before:
ABCDEFGHIJKLMNOPQRSTUVWXYZ
..........................

After:
ABCDEFGHIJKLMNOPQRSTUVWXYZ
BADCFEHGJILKNMPORQTSVUXWZY

--*/
void
__cdecl
_swab(char *src, char *dest, int n)
{
    ENTRY("_swab (src=%p (%s), dest=%p (%s), n=%d)\n", src?src:"NULL", src?src:"NULL", dest?dest:"NULL", dest?dest:"NULL", n);
    swab(src, dest, n);
    LOGEXIT("_swab returning\n");
}


/*++
Function:
   PAL_strtok

Finds the next token in a string.

Return value:

A pointer to the next token found in strToken.  Returns NULL when no more 
tokens are found.  Each call modifies strToken by substituting a NULL 
character for each delimiter that is encountered.

Parameters:
strToken        String cotaining token(s)
strDelimit      Set of delimiter characters

Remarks:
In FreeBSD, strtok is not re-entrant, strtok_r is.  It manages re-entrancy 
by using a passed-in context pointer (which will be stored in thread local
storage)  According to the strtok MSDN documentation, "Calling these functions
simultaneously from multiple threads does not have undesirable effects", so
we need to use strtok_r.
--*/
char *
__cdecl
PAL_strtok(char *strToken, const char *strDelimit)
{
    char *retval=NULL;
    char *context;
    int ret;

    ENTRY("strtok (strToken=%p (%s), strDelimit=%p (%s))\n", 
          strToken?strToken:"NULL", 
          strToken?strToken:"NULL", strDelimit?strDelimit:"NULL", strDelimit?strDelimit:"NULL");
    
    context  = pthread_getspecific(strtokKey);

    retval = strtok_r(strToken, strDelimit, &context);
 
    if ((ret = pthread_setspecific(strtokKey, (LPVOID) context)) != 0)
    {
        ERROR("pthread_setspecific() failed error:%d (%s)\n",
               ret, strerror(ret));
        retval = NULL;
    }

    LOGEXIT("strtok returns %p (%s)\n", retval?retval:"NULL", retval?retval:"NULL");
    return(retval);
}
