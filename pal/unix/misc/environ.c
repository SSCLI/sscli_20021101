/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    environ.c

Abstract:

    Implementation of functions manipulating environment variables.

Revision History:

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include "pal/misc.h"
#include <stdlib.h>

SET_DEFAULT_DEBUG_CHANNEL(MISC);



/*++
Function:
  GetEnvironmentVariableA

The GetEnvironmentVariable function retrieves the value of the
specified variable from the environment block of the calling
process. The value is in the form of a null-terminated string of
characters.

Parameters

lpName 
       [in] Pointer to a null-terminated string that specifies the environment variable. 
lpBuffer 
       [out] Pointer to a buffer to receive the value of the specified environment variable. 
nSize 
       [in] Specifies the size, in TCHARs, of the buffer pointed to by the lpBuffer parameter. 

Return Values

If the function succeeds, the return value is the number of TCHARs
stored into the buffer pointed to by lpBuffer, not including the
terminating null character.

If the specified environment variable name was not found in the
environment block for the current process, the return value is zero.

If the buffer pointed to by lpBuffer is not large enough, the return
value is the buffer size, in TCHARs, required to hold the value string
and its terminating null character.

--*/
DWORD
PALAPI
GetEnvironmentVariableA(
            IN LPCSTR lpName,
            OUT LPSTR lpBuffer,
            IN DWORD nSize)
{
    char  *value;
    DWORD dwRet = 0;

    ENTRY("GetEnvironmentVariableA(lpName=%p (%s), lpBuffer=%p, nSize=%u)\n",
        lpName?lpName:"NULL",
        lpName?lpName:"NULL", lpBuffer, nSize);

    if ((lpName == NULL) || (lpName[0] == 0))
    {
        ERROR("lpName is NULL\n");
        goto done;
    }

    if (strchr(lpName, '=') != NULL)
    {
        // GetEnvironmentVariable doesn't permit '=' in variable names.
        value = NULL;
    }
    else
    {
        value = MiscGetenv(lpName);
    }
    if (value == NULL)
    {
        TRACE("%s is not found\n", lpName);
        goto done;
    }

    if (strlen(value) < nSize)
    {
        strcpy(lpBuffer, value);
        dwRet = strlen(value);
    } 
    else 
    {
        dwRet = strlen(value)+1;
    }

done:
    LOGEXIT("GetEnvironmentVariableA returns DWORD 0x%x\n", dwRet);
    return dwRet;
}


/*++
Function:
  GetEnvironmentVariableW

See MSDN doc.
--*/
DWORD
PALAPI
GetEnvironmentVariableW(
            IN LPCWSTR lpName,
            OUT LPWSTR lpBuffer,
            IN DWORD nSize)
{
    CHAR *inBuff = NULL;
    CHAR *outBuff = NULL;
    INT inBuffSize;
    DWORD size = 0;

    ENTRY("GetEnvironmentVariableW(lpName=%p (%S), lpBuffer=%p, nSize=%u)\n",
          lpName?lpName:W16_NULLSTRING,
          lpName?lpName:W16_NULLSTRING, lpBuffer, nSize);
    
    inBuffSize = WideCharToMultiByte( CP_ACP, 0, lpName, -1, 
                                      inBuff, 0, NULL, NULL);
    if ( 0 == inBuffSize )
    {
        ERROR( "lpName has to be a valid parameter\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto done;
    }

    inBuff = malloc(inBuffSize);
    if (inBuff == NULL)
    {
        ERROR("malloc failed\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto done;
    }
    
    outBuff  = malloc(nSize*2);
    if (outBuff == NULL)
    {
        ERROR("malloc failed\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto done;
    }

    if ( 0 == WideCharToMultiByte( CP_ACP, 0, lpName, -1, inBuff, 
                                   inBuffSize, NULL, NULL ) )
    {
        ASSERT( "WideCharToMultiByte failed!\n" );
        SetLastError( ERROR_INTERNAL_ERROR );
        goto done;
    }
    size = GetEnvironmentVariableA(inBuff, outBuff, nSize);
    if (size > nSize)
    {
        TRACE("Insufficient buffer\n");
    }
    else if ( size == 0 )
    {
        /* error handle in GetEnvironmentVariableA */
    }
    else
    {
        size = MultiByteToWideChar(CP_ACP, 0, outBuff, -1, lpBuffer, nSize);
        if ( 0 != size )
        {
            /* Not including the NULL. */
            size--;
        }
        else
        {
            ASSERT( "MultiByteToWideChar failed!\n" );
            SetLastError( ERROR_INTERNAL_ERROR );
            size = 0;
            *lpBuffer = '\0';
        }
    }

done:
    free(outBuff);
    free(inBuff);

    LOGEXIT("GetEnvironmentVariableW returns DWORD 0x%x\n", size);
    return size;
}


/*++
Function:
  SetEnvironmentVariableW

The SetEnvironmentVariable function sets the value of an environment
variable for the current process.

Parameters

lpName 
       [in] Pointer to a null-terminated string that specifies the
       environment variable whose value is being set. The operating
       system creates the environment variable if it does not exist
       and lpValue is not NULL.
lpValue
       [in] Pointer to a null-terminated string containing the new
       value of the specified environment variable. If this parameter
       is NULL, the variable is deleted from the current process's
       environment.

Return Values

If the function succeeds, the return value is nonzero.

If the function fails, the return value is zero. To get extended error
information, call GetLastError.

Remarks

This function has no effect on the system environment variables or the
environment variables of other processes.

--*/
BOOL
PALAPI
SetEnvironmentVariableW(
            IN LPCWSTR lpName,
            IN LPCWSTR lpValue)
{
    PCHAR name = NULL;
    PCHAR value = NULL;
    INT nameSize = 0;
    INT valueSize = 0;
    BOOL bRet = FALSE;

    ENTRY("SetEnvironmentVariableW(lpName=%p (%S), lpValue=%p (%S))\n",
        lpName?lpName:W16_NULLSTRING,
        lpName?lpName:W16_NULLSTRING, lpValue?lpValue:W16_NULLSTRING, lpValue?lpValue:W16_NULLSTRING);

    if ((nameSize = WideCharToMultiByte(CP_ACP, 0, lpName, -1, name, 0, 
                                        NULL, NULL)) == 0)
    {
        ERROR("WideCharToMultiByte failed\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    name = malloc(sizeof(CHAR)* nameSize);
    if (name == NULL)
    {
        ERROR("malloc failed\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto done;
    }
    
    if ( 0 == WideCharToMultiByte(CP_ACP, 0, lpName,  -1, 
                                  name,  nameSize, NULL, NULL ) )
    {
        ASSERT( "WideCharToMultiByte returned 0\n" );
        SetLastError( ERROR_INTERNAL_ERROR );
        goto done;
    }

    if ( NULL != lpValue )
    {
        if ((valueSize = WideCharToMultiByte(CP_ACP, 0, lpValue, -1, value, 
                                             0, NULL, NULL)) == 0)
        {
            ERROR("WideCharToMultiByte failed\n");
            SetLastError(ERROR_INVALID_PARAMETER);
            goto done;
        }

        value = malloc(sizeof(CHAR)*valueSize);
        
        if ( NULL == value )
        {
            ERROR("malloc failed\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }
        
        if ( 0 == WideCharToMultiByte( CP_ACP, 0, lpValue, -1, 
                                       value, valueSize, NULL, NULL ) )
        {
            ASSERT("WideCharToMultiByte failed\n");
            SetLastError( ERROR_INTERNAL_ERROR );
            goto done;
        }
    }
    

    bRet = SetEnvironmentVariableA(name, value);
done:
    free(value);
    free(name);
    
    LOGEXIT("SetEnvironmentVariableW returning BOOL %d\n", bRet);
    return bRet;
}


/*++
Function:
  GetEnvironmentStringsW

The GetEnvironmentStrings function retrieves the environment block for
the current process.

Parameters

This function has no parameters.

Return Values

The return value is a pointer to an environment block for the current process.

Remarks

The GetEnvironmentStrings function returns a pointer to the
environment block of the calling process. This should be treated as a
read-only block; do not modify it directly.  Instead, use the
GetEnvironmentVariable and SetEnvironmentVariable functions to
retrieve or change the environment variables within this block. When
the block is no longer needed, it should be freed by calling
FreeEnvironmentStrings.

--*/
LPWSTR
PALAPI
GetEnvironmentStringsW(
               VOID)
{
    WCHAR *wenviron = NULL, *tempEnviron;
    int i, len, envNum;

    envNum = 0;
    len    = 0;

    ENTRY("GetEnvironmentStringsW()\n");

    /* get total length of the bytes that we need to allocate */
    for (i = 0; palEnvironment[i] != 0; i++)
    {
        len = MultiByteToWideChar(CP_ACP, 0, palEnvironment[i], -1, wenviron, 0);
        envNum += len;
    }

    wenviron = malloc(sizeof(WCHAR)* (envNum + 1));
    if (wenviron == NULL) 
    {
        ERROR("malloc failed\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto EXIT;
    }

    len = 0;
    tempEnviron = wenviron;
    for (i = 0; palEnvironment[i] != 0; i++)
    {
        len = MultiByteToWideChar(CP_ACP, 0, palEnvironment[i], -1, tempEnviron, envNum);
        tempEnviron += len;
        envNum      -= len;
    }

    *tempEnviron = 0; /* Put an extra NULL at the end */
 EXIT:
    LOGEXIT("GetEnvironmentStringsW returning %p\n", wenviron);
    return wenviron;
}


/*++
Function:
  FreeEnvironmentStringsW

The FreeEnvironmentStrings function frees a block of environment strings.

Parameters

lpszEnvironmentBlock   [in] Pointer to a block of environment strings. The pointer to
                            the block must be obtained by calling the
                            GetEnvironmentStrings function. 

Return Values

If the function succeeds, the return value is nonzero.  If the
function fails, the return value is zero. To get extended error
information, call GetLastError.

Remarks

When GetEnvironmentStrings is called, it allocates memory for a block
of environment strings. When the block is no longer needed, it should
be freed by calling FreeEnvironmentStrings.

--*/
BOOL
PALAPI
FreeEnvironmentStringsW(
            IN LPWSTR lpValue)
{
    ENTRY("FreeEnvironmentStringsW(lpValue=%p (%S))\n", lpValue?lpValue:W16_NULLSTRING, lpValue?lpValue:W16_NULLSTRING);

    if (lpValue != NULL)
    {
        free(lpValue);
    }

    LOGEXIT("FreeEnvironmentStringW returning BOOL TRUE\n");
    return TRUE ;
}

/*++
Function:
  SetEnvironmentVariableA

The SetEnvironmentVariable function sets the value of an environment
variable for the current process.

Parameters

lpName
       [in] Pointer to a null-terminated string that specifies the
       environment variable whose value is being set. The operating
       system creates the environment variable if it does not exist
       and lpValue is not NULL.
lpValue
       [in] Pointer to a null-terminated string containing the new
       value of the specified environment variable. If this parameter
       is NULL, the variable is deleted from the current process's
       environment.

Return Values

If the function succeeds, the return value is nonzero.

If the function fails, the return value is zero. To get extended error
information, call GetLastError.

Remarks

This function has no effect on the system environment variables or the
environment variables of other processes.

--*/
BOOL
PALAPI
SetEnvironmentVariableA(
			IN LPCSTR lpName,
			IN LPCSTR lpValue)
{

    BOOL bRet = FALSE;
    int nResult =0;
    ENTRY("SetEnvironmentVariableA(lpName=%p (%s), lpValue=%p (%s))\n",
        lpName?lpName:"NULL",
        lpName?lpName:"NULL", lpValue?lpValue:"NULL", lpValue?lpValue:"NULL");

    /*check if the input variable name is null
     * and if so exit*/
    if ((lpName == NULL) || (lpName[0] == 0))
    {
        ERROR("lpName is NULL\n");
        goto done;
    }
    /*check if the input value is null and if so
     * check if the input name is valid and delete
     * the variable name from process environment*/
    if (lpValue == NULL)
    {
        if ((lpValue = MiscGetenv(lpName)) == NULL)
        {
            ERROR("Couldn't find environment variable (%s)\n", lpName);
            SetLastError(ERROR_ENVVAR_NOT_FOUND);
            goto done;
        }
        MiscUnsetenv(lpName);
    }
    /*All the conditions are met. Set the variable*/
    else
    {
        LPSTR string = (LPSTR) malloc(strlen(lpName) + strlen(lpValue) + 2);
        if (string == NULL)
        {
            bRet = FALSE;
            ERROR("Unable to allocate memory\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }
        sprintf(string, "%s=%s", lpName, lpValue);
        nResult = MiscPutenv(string, FALSE) ? 0 : -1;
        // If MiscPutenv returns FALSE, it almost certainly failed to
        // allocate memory.
        if(nResult == -1)
        {
            bRet = FALSE;
            ERROR("Unable to allocate memory\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }
    }
    bRet = TRUE;
done:
    LOGEXIT("SetEnvironmentVariableA returning BOOL %d\n", bRet);    
    return bRet;
}


