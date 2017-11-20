/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    include/pal/cruntime.h

Abstract:

    Header file for C runtime utility functions.

--*/

#ifndef _PAL_CRUNTIME_H_
#define _PAL_CRUNTIME_H_

#include <string.h>
#include <stdarg.h>
#include <pthread.h>

typedef enum
{
    PFF_MINUS =  1,
    PFF_POUND =  2,
    PFF_ZERO  =  4,
    PFF_SPACE =  8,
    PFF_PLUS  = 16
}PRINTF_FORMAT_FLAGS;

typedef enum
{
    PRECISION_STAR    = -2, /* e.g. "%10.*s"  */
    PRECISION_DOT     = -3, /* e.g. "%10.s"   */
    PRECISION_INVALID = -4  /* e.g. "%10.*3s" */
}PRECISION_FLAGS;

typedef enum
{
    PFF_PREFIX_SHORT    = 1,
    PFF_PREFIX_LONG     = 2,
    PFF_PREFIX_LONGLONG = 3,
    PFF_PREFIX_LONG_W   = 4
}PRINTF_PREFIXES;

typedef enum
{
    PFF_TYPE_CHAR    = 1,
    PFF_TYPE_STRING  = 2,
    PFF_TYPE_WSTRING = 3,
    PFF_TYPE_INT     = 4,
    PFF_TYPE_P       = 5,
    PFF_TYPE_N       = 6,
    PFF_TYPE_FLOAT   = 7
}PRINTF_TYPES;

typedef enum
{
    SCANF_PREFIX_SHORT = 1,
    SCANF_PREFIX_LONG = 2,
    SCANF_PREFIX_LONGLONG = 3
}SCANF_PREFIXES;

typedef enum
{
    SCANF_TYPE_CHAR     = 1,
    SCANF_TYPE_STRING   = 2,
    SCANF_TYPE_INT      = 3,
    SCANF_TYPE_N        = 4,
    SCANF_TYPE_FLOAT    = 5,
    SCANF_TYPE_BRACKETS = 6,
    SCANF_TYPE_SPACE    = 7
}SCANF_TYPES;

/* TLS index for wcstok context pointer; declare it here because we use it in 
   wchar.h but initialize it in string.c */                             
extern pthread_key_t wcstokKey;

/*
Initialize the thread local storage used for the context pointer
in strtok and wcstok
*/
BOOL StrtokInitialize(void);

/*
Free the thread local storage used for the context pointer
in strtok and wcstok
*/
void StrtokCleanup(void);

/*******************************************************************************
Function:
  Internal_AddPaddingA

Parameters:
  Out
    - buffer to place padding and given string (In)
  Count
    - maximum chars to be copied so as not to overrun given buffer
  In
    - string to place into (Out) accompanied with padding
  Padding
    - number of padding chars to add
  Flags
    - padding style flags (PRINTF_FORMAT_FLAGS)
*******************************************************************************/
BOOL Internal_AddPaddingA(LPSTR *Out, INT Count, LPSTR In, INT Padding, INT Flags);

/*******************************************************************************
Function:
  Internal_AddPaddingA

Parameters:
  Out
    - buffer to place padding and given string (In)
  Count
    - maximum chars to be copied so as not to overrun given buffer
  In
    - string to place into (Out) accompanied with padding
  Padding
    - number of padding chars to add
  Flags
    - padding style flags (PRINTF_FORMAT_FLAGS)
*******************************************************************************/
void PAL_printf_arg_remover(va_list *ap, INT Precision, INT Type, INT Prefix);

/*++
Function:
  Silent_PAL_vsnprintf

See MSDN doc.
--*/
INT Silent_PAL_vsnprintf(LPSTR Buffer, INT Count, LPCSTR Format, va_list ap);

/*++
Function:
  Silent_PAL_vfprintf

See MSDN doc.
--*/
int Silent_PAL_vfprintf(PAL_FILE *stream, const char *format, va_list ap);



/*++
Function:
  PAL_iswlower

See MSDN

--*/
int __cdecl PAL_iswlower( wchar_16 c );


/*++
Function:
  PAL_iswalpha

See MSDN

--*/
int __cdecl PAL_iswalpha( wchar_16 c );

#if HAVE_CFSTRING
/*--
Function:
  PAL_iswblank

Returns TRUE if c is a Win32 "blank" character.
--*/
int __cdecl PAL_iswblank(wchar_16 c);

/*--
Function:
  PAL_iswcntrl

Returns TRUE if c is a control character.
--*/
int __cdecl PAL_iswcntrl(wchar_16 c);
#endif  // HAVE_CFSTRING

/*++

struct PAL_FILE.
Used to mimic the behavior of windows.
fwrite under windows can set the ferror flag,
under BSD fwrite doesn't.
--*/
struct _FILE
{
   FILE *   bsdFilePtr;     /* The BSD file to be passed to the
                            functions needing it. */

   INT      PALferrorCode;  /* The ferror code that fwrite sets,
                            incase of error */

   BOOL     bTextMode;     /* Boolean variable to denote that the
                              fle is opened in text/binary mode*/ 
};

enum CRT_ERROR_CODES
{
    PAL_FILE_NOERROR = 0,
    PAL_FILE_ERROR
};

/* Global variables storing the std streams. Defined in cruntime/file.c. */
extern PAL_FILE PAL_Stdout;
extern PAL_FILE PAL_Stdin; 
extern PAL_FILE PAL_Stderr;

/*++

Functio:

    CRTInitStdStreams.
    
    Initilizes the standard streams.
    Returns TRUE on success, FALSE otherwise.
--*/
BOOL CRTInitStdStreams( void );

#endif /* _PAL_CRUNTIME_H_ */
