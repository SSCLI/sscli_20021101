/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    file.c

Abstract:

    Implementation of the file functions in the C runtime library that
    are Windows specific.

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include "pal/file.h"
#include "pal/handle.h"
#include "pal/cruntime.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

SET_DEFAULT_DEBUG_CHANNEL(CRT);

/* Global variables storing the std streams.*/
PAL_FILE PAL_Stdout;
PAL_FILE PAL_Stdin; 
PAL_FILE PAL_Stderr;

/*++

Function:

    CRTInitStdStreams.

    Initilizes the standard streams.
    Returns TRUE on success, FALSE otherwise.
--*/
BOOL CRTInitStdStreams( void )
{
    /* stdout */
    PAL_Stdout.bsdFilePtr = stdout;
    PAL_Stdout.PALferrorCode = PAL_FILE_NOERROR;
    PAL_Stdout.bTextMode = TRUE;

    /* stdin */
    PAL_Stdin.bsdFilePtr = stdin;
    PAL_Stdin.PALferrorCode = PAL_FILE_NOERROR;
    PAL_Stdin.bTextMode = TRUE;

    /* stderr */
    PAL_Stderr.bsdFilePtr = stderr;
    PAL_Stderr.PALferrorCode = PAL_FILE_NOERROR;
    PAL_Stderr.bTextMode = TRUE;

    return TRUE;
}

/*++
Function :

    StripUnsupportedModes

    Returns a string with the unsupported modes removed.

--*/
static LPSTR StripUnsupportedModes( LPSTR str , BOOL * bTextMode)
{
    LPSTR retval = NULL;
    LPSTR temp = NULL;

    *bTextMode = TRUE;

    if ( !str )
    {
        ASSERT( "StripUnsupportedModes called with a NULL parameter\n" );
        return NULL;
    }
    retval = (LPSTR)malloc( ( strlen( str ) + 1 ) * sizeof( CHAR ) );
    if ( !retval )
    {
        ERROR( "Unable to allocate memory.\n" );
        return NULL;
    }

    /*check if the mode specifies opening in binary
      if so set the bTextMode to False*/
    if(NULL != strchr(str,'b'))
    {
        *bTextMode = FALSE;
    }

    temp = retval;
    while ( *str )
    {
        if ( *str == 'r' || *str == 'w' || *str == 'a' )
        {
            *temp = *str;
            temp++;
            if ( ( ++str != NULL ) && *str == '+' )
            {
                *temp = *str;
                temp++;
                str++;
            }
        }
        else
        {
            str++;
        }
    }
    *temp = '\0';
    return retval;
}

/*++
Function:
  _open_osfhandle

See MSDN doc.
--*/
int
__cdecl
_open_osfhandle( INT_PTR osfhandle, int flags )
{
    INT nRetVal = -1;
    HOBJSTRUCT * hObj = NULL;
    INT openFlags = 0;

    ENTRY( "_open_osfhandle (osfhandle=%#x, flags=%#x)\n", osfhandle, flags );

    if ((flags != _O_RDONLY) && ((flags & _O_BINARY) != _O_BINARY))
    {
        ASSERT("flag(%#x) not supported\n", flags);
        goto EXIT;
    }

    if ( flags & _O_BINARY )
    {
        ASSERT("_O_BINARY flag not implemented yet\n");
        goto EXIT;
    }

    openFlags |= O_RDONLY;

    hObj = HMGRLockHandle2( (HANDLE)osfhandle, HOBJ_FILE);

    if ( hObj )
    {
        file * fileObject = (file *)hObj;
        
        if (fileObject->unix_filename != NULL)
        {
            nRetVal = open( fileObject->unix_filename, openFlags );
        }
        else /* the only file object with NULL unix_filename is a pipe */
        {
            /* check if the file pipe descrptor is for read or write */
            if (fileObject->open_flags == O_WRONLY)
            {
                ERROR( "Couldn't open a write pipe on read mode\n");
                goto EXIT;
            }

            nRetVal = dup(fileObject->unix_fd);
        }
        
        if ( nRetVal == -1 )
        {
            ERROR( "Error: %s.\n", strerror( errno ) );
        }
    }
    else
    {
        ERROR( "Not a valid file handle.\n" );
    }

EXIT:    
    if(NULL!=hObj)
    {
        HMGRUnlockHandle((HANDLE)osfhandle, hObj);
    }
    LOGEXIT( "_open_osfhandle return nRetVal:%d\n", nRetVal);
    return nRetVal;
}


/*++
Function:
  _getw

Gets an integer from a stream.

Return Value

_getw returns the integer value read. A return value of EOF indicates
either an error or end of file. However, because the EOF value is also
a legitimate integer value, use feof or ferror to verify an
end-of-file or error condition.

Parameter

file  Pointer to FILE structure

--*/
int
__cdecl
_getw( PAL_FILE *file )
{
    INT ret = 0;
    
    ENTRY("_getw (file=%p)\n", file);
    ret = getw( file->bsdFilePtr );
    LOGEXIT( "returning %d\n", ret );
    
    return ret;
}


/*++
Function:
  _fdopen

see MSDN

--*/
PAL_FILE *
__cdecl
_fdopen(
    int handle,
    const char *mode)
{
    PAL_FILE *f = NULL;
    LPSTR supported = NULL;
    BOOL bTextMode = TRUE;

    ENTRY("_fdopen (handle=%d mode=%p (%s))\n", handle, mode, mode);

    f = (PAL_FILE*)malloc( sizeof( PAL_FILE ) );
    if ( f )
    {
        supported = StripUnsupportedModes( (char*)mode , &bTextMode);
        if ( !supported )
        {
            free(f);
            f = NULL;
            goto EXIT;
        }

        f->bsdFilePtr = (FILE *)fdopen( handle, supported );
        f->PALferrorCode = PAL_FILE_NOERROR;
        /* Make sure fdopen did not fail. */
        if ( !f->bsdFilePtr )
        {
            free( f );
            f = NULL;
        }

        free( supported );
        supported = NULL;
    }
    else
    {
        ERROR( "Unable to allocate memory for the PAL_FILE wrapper!\n" );
    }

EXIT:
    LOGEXIT( "_fdopen returns FILE* %p\n", f );
    return f;
}


/*++

Function :
    fopen

see MSDN doc.

--*/
PAL_FILE *
__cdecl
PAL_fopen( const char * fileName, const char * mode)
{
    PAL_FILE *f = NULL;
    LPSTR supported = NULL;
    LPSTR UnixFileName = NULL;
    struct stat stat_data;
    BOOL bTextMode = TRUE;

    ENTRY("fopen ( fileName=%p (%s) mode=%p (%s))\n", fileName, fileName, mode , mode );

    if ( *mode == 'r' || *mode == 'w' || *mode == 'a' )
    {
        supported = StripUnsupportedModes( (char*)mode,&bTextMode);

        if ( !supported )
        {
            goto done;
        }

        UnixFileName = strdup(fileName);
        if (UnixFileName == NULL )
        {
            ERROR("malloc() failed\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto done;
        }
        FILEDosToUnixPathA( UnixFileName );

        /*I am not checking for the case where stat fails
         *as fopen will handle the error more gracefully in case
         *UnixFileName is invalid*/
        if ((stat(UnixFileName, &stat_data) == 0 ) &&
            ((stat_data.st_mode & S_IFMT) == S_IFDIR))
        {
            goto done;
        }

        f = (PAL_FILE*)malloc( sizeof( PAL_FILE ) );
        if ( f )
        {
            f->bsdFilePtr =  (FILE*)fopen( UnixFileName, supported );
            f->PALferrorCode = PAL_FILE_NOERROR;
            f->bTextMode = bTextMode;
            if ( !f->bsdFilePtr )
            {
                /* Failed */
                free( f );
                f = NULL;
            }
        }
        else
        {
            ERROR( "Unable to allocate memory to the PAL_FILE wrapper\n" );
        }
    }
    else
    {
        ERROR( "The mode flags must start with either an a, w, or r.\n" );
    }

done:
    free( supported );
    supported = NULL;
    free( UnixFileName );

    LOGEXIT( "fopen returns FILE* %p\n", f );
    return f;
}

/*++
Function:
  _wfopen

see MSDN doc.

--*/
PAL_FILE *
__cdecl
_wfopen(
    const wchar_16 *fileName,
    const wchar_16 *mode)
{
    CHAR mbFileName[ _MAX_PATH ];
    CHAR mbMode[ 10 ];
    PAL_FILE * filePtr = NULL;

    ENTRY("_wfopen(fileName:%p (%S), mode:%p (%S))\n", fileName, fileName, mode, mode);

    /* Convert the parameters to ASCII and defer to PAL_fopen */
    if ( WideCharToMultiByte( CP_ACP, 0, fileName, -1, mbFileName,
                              sizeof mbFileName, NULL, NULL ) != 0 )
    {
        if ( WideCharToMultiByte( CP_ACP, 0, mode, -1, mbMode,
                                  sizeof mbMode, NULL, NULL ) != 0 )
        {
            filePtr = PAL_fopen(mbFileName, mbMode);
        }
        else
        {
            ERROR( "An error occured while converting mode to ANSI.\n" );
        }
    }
    else
    {
        ERROR( "An error occured while converting"
               " fileName to ANSI string.\n" );
    }
    LOGEXIT("_wfopen returning FILE* %p\n", filePtr);
    return filePtr;
}


/*++
Function:
  _putw

Writes an integer to a stream.

Return Value

_putw returns the value written. A return value of EOF may indicate an
error. Because EOF is also a legitimate integer value, use ferror to
verify an error.

Parameters

c     Binary integer to be output
file  Pointer to FILE structure

--*/
int
__cdecl
_putw(int c, PAL_FILE *file)
{
    INT ret = 0;

    ENTRY("_putw (c=0x%x, file=%p)\n", c, file);
    ret = putw(c,  file->bsdFilePtr );
    LOGEXIT( "returning %d\n", ret );

    return ret;
}


/*++
Function
    PAL_get_stdout.

    Returns the stdout stream.
--*/
PAL_FILE * __cdecl PAL_get_stdout()
{
    ENTRY("PAL_get_stdout\n");
    LOGEXIT("PAL_get_stdout returns PAL_FILE * %p\n", &PAL_Stdout );
    return &PAL_Stdout;
}

/*++
Function
    PAL_get_stdin.

    Returns the stdin stream.
--*/
PAL_FILE * __cdecl PAL_get_stdin()
{
    ENTRY("PAL_get_stdin\n");
    LOGEXIT("PAL_get_stdin returns PAL_FILE * %p\n", &PAL_Stdin );
    return &PAL_Stdin;
}

/*++
Function
    PAL_get_stderr.

    Returns the stderr stream.
--*/
PAL_FILE * __cdecl PAL_get_stderr()
{
    ENTRY("PAL_get_stderr\n");
    LOGEXIT("PAL_get_stderr returns PAL_FILE * %p\n", &PAL_Stderr );
    return &PAL_Stderr;
}


/*++

Function:

    _close

See msdn for more details.
--*/
int __cdecl PAL__close( int handle )
{
    INT nRetVal = 0;

    ENTRY( "_close( handle=%d )\n", handle );

    nRetVal = close( handle );

    LOGEXIT( "_close returning %d.\n", nRetVal );
    return nRetVal;
}

/*++

Function:

    fgets

See msdn for more details.

Notes:
    in Unix systems, fgets() can return an error if it gets interrupted by a
    signal before reading anything, and errno is set to EINTR. When this
    happens, it is SOP to call fgets again
--*/
char * __cdecl PAL_fgets(char *s, int n, PAL_FILE *f)
{
    char *retval = NULL;
    ENTRY( "fgets(s=%p (%s) n=%d f=%p)\n", s, s,n,f);

    if ( NULL != f )
    {
        do
        {
            retval =  fgets( s, n, f->bsdFilePtr );
            if(NULL==retval)
            {
                if( feof( f->bsdFilePtr ) )
                {
                    TRACE("Reached EOF\n");
                    break;
                }
                /* The man page suggests using ferror and feof to distinguish
                   between error and EOF, but feof and errno is sufficient.
                   Not all cases that set errno also flag ferror, so just
                   checking errno is the best solution. */
                if( EINTR != errno )
                {
                    WARN("got error; errno is %d (%s)\n",errno, strerror(errno));
                    break;
                }
                /* we ignored a EINTR error, reset the stream's error state */
                clearerr( f->bsdFilePtr );
                TRACE("call got interrupted (EINTR), trying again\n");
            }
            if(f->bTextMode == TRUE)
            {
                int len = strlen(s);
                if((len>=2) && (s[len-1]=='\n') && (s[len-2]=='\r'))
                {
                    s[len-2]='\n';
                    s[len-1]='\0';
                }
            }
        } while(NULL == retval);
    }
    else
    {
        ERROR( "stream was NULL\n" );
    }

    LOGEXIT("fgets() returns %p\n", retval);
    return retval;
}

/*++
Function :

    fwrite

    See MSDN for more details.
--*/
size_t
__cdecl
PAL_fwrite( const void * buffer, size_t size, size_t count, PAL_FILE * stream )
{
    size_t nWrittenBytes = 0;

    ENTRY( "fwrite( buffer=%p, size=%d, count=%d, stream=%p )\n",
           buffer, size, count, stream );

    nWrittenBytes = fwrite( buffer, size, count, stream->bsdFilePtr );

    /* Make sure no error ocurred. */
    if ( nWrittenBytes < count )
    {
        /* Set the FILE* error code */
        stream->PALferrorCode = PAL_FILE_ERROR;
    }

    LOGEXIT( "fwrite returning size_t %d\n", nWrittenBytes );
    return nWrittenBytes;
}


/*++
Function :

    fread

    See MSDN for more details.
--*/
size_t
__cdecl
PAL_fread( void * buffer, size_t size, size_t count, PAL_FILE * stream )
{
    size_t nReadBytes = 0;
    LPSTR temp;
    int nChar = 0;
    int nCount =0;

    ENTRY( "fread( buffer=%p, size=%d, count=%d, stream=%p )\n",
           buffer, size, count, stream );

    if ( stream  )
   {
        if(stream->bTextMode != TRUE)
        {
            nReadBytes = fread( buffer, size, count, stream->bsdFilePtr );
        }

        else
        {
            int i=0;
            int j=0;
            temp = buffer;
            nCount =0;
            if(size == 0)
            {
                goto done;
            }
            for(i=0; i< count; i++)
            {
                for(j=0; j< size; j++)
                {
                    if((nChar = PAL_getc(stream)) == EOF)
                    {
                        goto done;
                    }
                    else
                    {
                        temp[nCount++]=nChar;
                    }
                }
            }
        done:
            nReadBytes = i;
        }
    }
    LOGEXIT( "fread returning size_t %d\n", nReadBytes );
    return nReadBytes;
}


/*++
Function :

    ferror

    See MSDN for more details.
--*/
int
_cdecl
PAL_ferror( PAL_FILE * stream )
{
    INT nErrorCode = PAL_FILE_NOERROR;

    ENTRY( "ferror( stream=%p )\n", stream );

    nErrorCode = ferror( stream->bsdFilePtr );
    if ( 0 == nErrorCode )
    {
        /* See if the PAL file error code is set. */
        nErrorCode = stream->PALferrorCode;
    }

    LOGEXIT( "ferror returns %d\n", nErrorCode );
    return nErrorCode;
}


/*++
Function :

    fclose

    See MSDN for more details.
--*/
int
_cdecl
PAL_fclose( PAL_FILE * stream )
{
    INT nRetVal = 0;

    ENTRY( "fclose( %p )\n", stream );

    nRetVal = fclose( stream->bsdFilePtr );
    free( stream );
    stream = NULL;

    LOGEXIT( "fclose returning %d\n", nRetVal );
    return nRetVal;
}

/*++
Function :

    fflush

    See MSDN for more details.
--*/

int
_cdecl
PAL_fflush( PAL_FILE * stream )
{
    INT nRetVal = 0;

    ENTRY( "fflush( %p )\n", stream );

    if ( stream )
    {
        nRetVal = fflush( stream->bsdFilePtr );
    }

    LOGEXIT( "fflush returning %d\n", nRetVal );
    return nRetVal;
}


/*++
Function :

    fputs

    See MSDN for more details.
--*/
int
_cdecl
PAL_fputs( const char * str,  PAL_FILE * stream )
{
    INT nRetVal = 0;

    ENTRY( "fputs( %p (%s), %p )\n", str, str, stream);

    nRetVal = fputs( str, stream->bsdFilePtr );

    LOGEXIT( "fputs returning %d\n", nRetVal );
    return nRetVal;
}


/*++
Function :

    fseek

    See MSDN for more details.
--*/
int
_cdecl
PAL_fseek( PAL_FILE * stream, long offset, int whence )
{
    INT nRetVal = 0;

    ENTRY( "fseek( %p, %ld, %d )\n", stream, offset, whence );

    nRetVal = fseek( stream->bsdFilePtr, offset, whence  );

    LOGEXIT( "fseek returning %d\n", nRetVal );
    return nRetVal;
}

/*++
Function :

    ftell

    See MSDN for more details.
--*/
long
_cdecl
PAL_ftell( PAL_FILE * stream )
{
    long nRetVal = 0;

    ENTRY( "ftell( %p )\n", stream );

    nRetVal = ftell( stream->bsdFilePtr );

    LOGEXIT( "ftell returning %ld\n", nRetVal );
    return nRetVal;
}

/*++
Function :

    feof

    See MSDN for more details.
--*/
int
_cdecl
PAL_feof( PAL_FILE * stream )
{
    INT nRetVal = 0;

    ENTRY( "feof( %p )\n", stream );

    nRetVal = feof( stream->bsdFilePtr );

    LOGEXIT( "feof returning %d\n", nRetVal );
    return nRetVal;
}

/*++
Function :

    getc

    See MSDN for more details.
--*/
int
_cdecl
PAL_getc( PAL_FILE * stream )
{
    INT nRetVal = 0;
    INT temp =0;

    ENTRY( "getc( %p )\n", stream );

    nRetVal = getc( stream->bsdFilePtr );
    if(stream->bTextMode)
    {
        if(nRetVal == '\r')
        {
            if((temp = getc( stream->bsdFilePtr ))== '\n')
            {
                nRetVal ='\n';
            }
            else
            {
                if(EOF == ungetc(temp,stream->bsdFilePtr))
                {
                    ERROR("ungetc operation failed\n");
                }
            }
        }
    }

    LOGEXIT( "getc returning %d\n", nRetVal );
    return nRetVal;
}

/*++
Function :

    ungetc

    See MSDN for more details.
--*/
int
_cdecl
PAL_ungetc( int c, PAL_FILE * stream )
{
    INT nRetVal = 0;

    ENTRY( "ungetc( %c, %p )\n", c, stream );

    nRetVal = ungetc( c, stream->bsdFilePtr );

    LOGEXIT( "ungetc returning %d\n", nRetVal );
    return nRetVal;
}



