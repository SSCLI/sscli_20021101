/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    directory.c

Abstract:

    Implementation of the file WIN API for the PAL

Revision History:

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include "pal/file.h"

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif  // HAVE_ALLOCA_H
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

SET_DEFAULT_DEBUG_CHANNEL(FILE);



/*++
Function:
  CreateDirectoryW

Note:
  lpSecurityAttributes always NULL.

See MSDN doc.
--*/
BOOL
PALAPI
CreateDirectoryW(
         IN LPCWSTR lpPathName,
         IN LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    BOOL  bRet = FALSE;
    DWORD dwLastError = 0;
    int   mb_size;
    char  *mb_dir = NULL;

    ENTRY("CreateDirectoryW(lpPathName=%p (%S), lpSecurityAttr=%p)\n",
          lpPathName?lpPathName:W16_NULLSTRING,
          lpPathName?lpPathName:W16_NULLSTRING, lpSecurityAttributes);

    if ( lpSecurityAttributes )
    {
        ASSERT("lpSecurityAttributes is not NULL as it should be\n");
        dwLastError = ERROR_INVALID_PARAMETER;
        goto done;
    }

    /* translate the wide char lpPathName string to multibyte string */
    if(0 == (mb_size = WideCharToMultiByte( CP_ACP, 0, lpPathName, -1, NULL, 0,
                                            NULL, NULL )))
    {
        ASSERT("WideCharToMultiByte failure! error is %d\n", GetLastError());
        dwLastError = ERROR_INTERNAL_ERROR;
        goto done;
    }

    if (((mb_dir = malloc(mb_size)) == NULL) ||
        (WideCharToMultiByte( CP_ACP, 0, lpPathName, -1, mb_dir, mb_size, NULL,
                              NULL) != mb_size))
    {
        ASSERT("WideCharToMultiByte or malloc failure! LastError:%d errno:%d\n",
              GetLastError(), errno);
        dwLastError = ERROR_INTERNAL_ERROR;
        goto done;
    }

    bRet = CreateDirectoryA(mb_dir,NULL);
done:
    if( dwLastError )
    {
        SetLastError( dwLastError );
    }
    if (mb_dir != NULL)
    {
        free(mb_dir);
    }
    LOGEXIT("CreateDirectoryW returns BOOL %d\n", bRet);
    return bRet;
}


/*++
Function:
  RemoveDirectoryW

See MSDN doc.
--*/
BOOL
PALAPI
RemoveDirectoryW(
         IN LPCWSTR lpPathName)
{
    char  mb_dir[MAX_PATH];
    int   mb_size;
    DWORD dwLastError = 0;
    BOOL  bRet = FALSE;

    ENTRY("RemoveDirectoryW(lpPathName=%p (%S))\n",
          lpPathName?lpPathName:W16_NULLSTRING,
          lpPathName?lpPathName:W16_NULLSTRING);

    if (lpPathName == NULL) 
    {
        dwLastError = ERROR_PATH_NOT_FOUND;
        goto done;
    }
    
    mb_size = WideCharToMultiByte( CP_ACP, 0, lpPathName, -1, mb_dir, MAX_PATH,
                                   NULL, NULL );
    if( mb_size == 0 )
    {
        dwLastError = GetLastError();
        if( dwLastError == ERROR_INSUFFICIENT_BUFFER )
        {
            ERROR("lpPathName is larger than MAX_PATH (%d)!\n", MAX_PATH);
        }
        else
        {
            ASSERT("WideCharToMultiByte failure! error is %d\n", dwLastError);
        }
        dwLastError = ERROR_INVALID_PARAMETER;
        goto done;
    }

    FILEDosToUnixPathA( mb_dir );
    if ( !FILEGetFileNameFromSymLink(mb_dir))
    {
        FILEGetProperNotFoundError( mb_dir, &dwLastError );
        goto done;
    }

    if ( rmdir(mb_dir) != 0 )
    {
        TRACE("Removal of directory [%s] was unsuccessful, errno = %d.\n",
              mb_dir, errno);

        switch( errno )
        {
        case ENOTDIR:
            /* FALL THROUGH */
        case ENOENT:
        {
            struct stat stat_data;
            
            if ( stat( mb_dir, &stat_data) == 0 && 
                 (stat_data.st_mode & S_IFMT) == S_IFREG )
            {
                /* Not a directory, it is a file. */
                dwLastError = ERROR_DIRECTORY;
            }
            else
            {
                FILEGetProperNotFoundError( mb_dir, &dwLastError );
            }
            break;
        }
        case ENOTEMPTY:
            dwLastError = ERROR_DIR_IS_NOT_EMPTY; 
            break;
        default:
            dwLastError = ERROR_ACCESS_DENIED;
        }
    }
    else
    {
        TRACE("Removal of directory [%s] was successful.\n", mb_dir);
        bRet = TRUE;
    }

done:
    if( dwLastError )
    {
        SetLastError( dwLastError );
    }

    LOGEXIT("RemoveDirectoryW returns BOOL %d\n", bRet);
    return bRet;
}


/*++
Function:
  GetCurrentDirectoryA

See MSDN doc.
--*/
DWORD
PALAPI
GetCurrentDirectoryA(
             IN DWORD nBufferLength,
             OUT LPSTR lpBuffer)
{
    DWORD dwDirLen = 0;
    DWORD dwLastError = 0;

    char  *current_dir;

    ENTRY("GetCurrentDirectoryA(nBufferLength=%u, lpBuffer=%p)\n", nBufferLength, lpBuffer);

    /* NULL first arg means getcwd will allocate the string */
    current_dir = getcwd( NULL, MAX_PATH + 1 );

    if ( !current_dir )
    {
        ASSERT( "getcwd returned NULL\n" );
        dwLastError = ERROR_INTERNAL_ERROR;
        goto done;
    }

    dwDirLen = strlen( current_dir );

    /* if the supplied buffer isn't long enough, return the required
       length, including room for the NULL terminator */
    if ( nBufferLength <= dwDirLen )
    {
        ++dwDirLen; /* include space for the NULL */
        goto done;
    }
    else
    {
        strcpy( lpBuffer, current_dir );
    }

done:
    free( current_dir );

    if ( dwLastError )
    {
        SetLastError(dwLastError);
    }

    LOGEXIT("GetCurrentDirectoryA returns DWORD %u\n", dwDirLen);
    return dwDirLen;
}


/*++
Function:
  GetCurrentDirectoryW

See MSDN doc.
--*/
DWORD
PALAPI
GetCurrentDirectoryW(
             IN DWORD nBufferLength,
             OUT LPWSTR lpBuffer)
{
    DWORD dwWideLen = 0;
    DWORD dwLastError = 0;

    char  *current_dir;
    int   dir_len;

    ENTRY("GetCurrentDirectoryW(nBufferLength=%u, lpBuffer=%p)\n",
          nBufferLength, lpBuffer);

    current_dir = getcwd( NULL, MAX_PATH + 1 );

    if ( !current_dir )
    {
        ASSERT( "getcwd returned NULL\n" );
        dwLastError = ERROR_INTERNAL_ERROR;
        goto done;
    }

    dir_len = strlen( current_dir );
    dwWideLen = MultiByteToWideChar( CP_ACP, 0,
                 current_dir, dir_len,
                 NULL, 0 );

    /* if the supplied buffer isn't long enough, return the required
       length, including room for the NULL terminator */
    if ( nBufferLength > dwWideLen )
    {
        if(!MultiByteToWideChar( CP_ACP, 0, current_dir, dir_len + 1,
                     lpBuffer, nBufferLength ))
        {
            ASSERT("MultiByteToWideChar failure!\n");
            dwWideLen = 0;
            dwLastError = ERROR_INTERNAL_ERROR;
        }
    }
    else
    {
        ++dwWideLen; /* include space for the NULL */
    }

done:
    free( current_dir );

    if ( dwLastError )
    {
        SetLastError(dwLastError);
    }

    LOGEXIT("GetCurrentDirectoryW returns DWORD %u\n", dwWideLen);
    return dwWideLen;
}


/*++
Function:
  SetCurrentDirectoryW

See MSDN doc.
--*/
BOOL
PALAPI
SetCurrentDirectoryW(
            IN LPCWSTR lpPathName)
{
    BOOL bRet;
    DWORD dwLastError = 0;
    char dir[MAX_PATH];
    int  size;

    ENTRY("SetCurrentDirectoryW(lpPathName=%p (%S))\n",
          lpPathName?lpPathName:W16_NULLSTRING,
          lpPathName?lpPathName:W16_NULLSTRING);

   /*check if the given path is null. If so
     return FALSE*/
    if (lpPathName == NULL )
    {
        ERROR("Invalid path/directory name\n");
        dwLastError = ERROR_INVALID_NAME;
        bRet = FALSE;
        goto done;
    }

    size = WideCharToMultiByte( CP_ACP, 0, lpPathName, -1, dir, MAX_PATH,
                                NULL, NULL );
    if( size == 0 )
    {
        dwLastError = GetLastError();
        if( dwLastError == ERROR_INSUFFICIENT_BUFFER )
        {
            ERROR("lpPathName is larger than MAX_PATH (%d)!\n", MAX_PATH);
        }
        else
        {
            ASSERT("WideCharToMultiByte failure! error is %d\n", dwLastError);
        }
        dwLastError = ERROR_INVALID_PARAMETER;
        bRet = FALSE;
        goto done;
    }

    bRet = SetCurrentDirectoryA(dir);
done:
    if( dwLastError )
    {
        SetLastError(dwLastError);
    }

    LOGEXIT("SetCurrentDirectoryW returns BOOL %d\n", bRet);
    return bRet;
}

/*++
Function:
  CreateDirectoryA

Note:
  lpSecurityAttributes always NULL.

See MSDN doc.
--*/
BOOL
PALAPI
CreateDirectoryA(
         IN LPCSTR lpPathName,
         IN LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    BOOL  bRet = FALSE;
    DWORD dwLastError = 0;
    char *realPath;
    LPSTR UnixPathName = NULL;
    int pathLength;
    int i;
    const int mode = S_IRWXU | S_IRWXG | S_IRWXO;

    ENTRY("CreateDirectoryA(lpPathName=%p (%s), lpSecurityAttr=%p)\n",
          lpPathName?lpPathName:"NULL",
          lpPathName?lpPathName:"NULL", lpSecurityAttributes);

    if ( lpSecurityAttributes )
    {
        ASSERT("lpSecurityAttributes is not NULL as it should be\n");
        dwLastError = ERROR_INVALID_PARAMETER;
        goto done;
    }


    UnixPathName = strdup(lpPathName);
    if (UnixPathName == NULL )
    {
        ERROR("malloc() failed\n");
        dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }
    FILEDosToUnixPathA( UnixPathName );
    // Remove any trailing slashes at the end because mkdir might not
    // handle them appropriately on all platforms.
    pathLength = strlen(UnixPathName);
    i = pathLength;
    while(i > 1)
    {
        if(UnixPathName[i - 1] =='/')
        {
            UnixPathName[i - 1]='\0';
            i--;
        }
        else
        {
            break;
        }
    }

    // Check the constraint for the real path length (should be < MAX_PATH).

    // Get an absolute path.
    if (UnixPathName[0] == '/')
    {
        realPath = UnixPathName;
    }
    else
    {
        const char *cwd = getcwd(NULL, MAX_PATH);
        // Copy cwd, '/', path
        realPath = alloca(strlen(cwd) + 1 + pathLength + 1);
        sprintf(realPath, "%s/%s", cwd, UnixPathName);
    }
    
    // Canonicalize the path so we can determine its length.
    FILECanonicalizePath(realPath);

    if (strlen(realPath) >= MAX_PATH)
    {
        ERROR("UnixPathName is larger than MAX_PATH (%d)!\n", MAX_PATH);
        dwLastError = ERROR_FILENAME_EXCED_RANGE;
        goto done;
    }

    if ( mkdir(realPath, mode) != 0 )
    {
        TRACE("Creation of directory [%s] was unsuccessful, errno = %d.\n",
              UnixPathName, errno);

        switch( errno )
        {
        case ENOTDIR:
            /* FALL THROUGH */
        case ENOENT:
            FILEGetProperNotFoundError( realPath, &dwLastError );
            goto done;
        case EEXIST:
            dwLastError = ERROR_ALREADY_EXISTS;
            break;
        default:
            dwLastError = ERROR_ACCESS_DENIED;
        }
    }
    else
    {
        TRACE("Creation of directory [%s] was successful.\n", UnixPathName);
        bRet = TRUE;
    }

done:
    if( dwLastError )
    {
        SetLastError( dwLastError );
    }
    free( UnixPathName );
    LOGEXIT("CreateDirectoryA returns BOOL %d\n", bRet);
    return bRet;
}

/*++
Function:
  SetCurrentDirectoryA

See MSDN doc.
--*/
BOOL
PALAPI
SetCurrentDirectoryA(
            IN LPCSTR lpPathName)
{
    BOOL bRet = FALSE;
    DWORD dwLastError = 0;
    int result;
    LPSTR UnixPathName = NULL;

    ENTRY("SetCurrentDirectoryA(lpPathName=%p (%s))\n",
          lpPathName?lpPathName:"NULL",
          lpPathName?lpPathName:"NULL");

   /*check if the given path is null. If so
     return FALSE*/
    if (lpPathName == NULL )
    {
        ERROR("Invalid path/directory name\n");
        dwLastError = ERROR_INVALID_NAME;
        goto done;
    }


    UnixPathName = strdup(lpPathName);
    if (UnixPathName == NULL )
    {
        ERROR("malloc() failed\n");
        dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }
    FILEDosToUnixPathA( UnixPathName );

    TRACE("Attempting to open Unix dir [%s]\n", UnixPathName);
    result = chdir(UnixPathName);

    if ( result == 0 )
    {
        bRet = TRUE;
    }
    else
    {
        if ( errno == ENOTDIR || errno == ENOENT )
        {
            struct stat stat_data;

            if ( stat( UnixPathName, &stat_data) == 0 &&
                 (stat_data.st_mode & S_IFMT) == S_IFREG )
            {
                /* Not a directory, it is a file. */
                dwLastError = ERROR_DIRECTORY;
            }
            else
            {
                FILEGetProperNotFoundError( UnixPathName, &dwLastError );
            }
            TRACE("chdir() failed, path was invalid.\n");
        }
        else
        {
            dwLastError = ERROR_ACCESS_DENIED;
            ERROR("chdir() failed; errno is %d (%s)\n", errno, strerror(errno));
        }
    }


done:
    if( dwLastError )
    {
        SetLastError(dwLastError);
    }

    if(UnixPathName != NULL)
    {
        free( UnixPathName );
    }

    LOGEXIT("SetCurrentDirectoryA returns BOOL %d\n", bRet);
    return bRet;
}
