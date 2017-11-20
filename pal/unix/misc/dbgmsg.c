/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    misc/dbgmsg.c

Abstract:
    Implementation of Debug Message utilies. Relay channel information,
    output functions, etc.

--*/

/* PAL headers */

#include "config.h"

#include "pal/thread.h"
#include "pal/dbgmsg.h"
#include "pal/cruntime.h"
#include "pal/critsect.h"
#include "pal/misc.h"

/* standard headers */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> /* for pthread_self */
#include <errno.h>

/* <stdarg.h> needs to be included after "palinternal.h" to avoid name 
   collision for va_start and va_end */
#include <stdarg.h>

/* append mode file I/O is safer */
#define _PAL_APPEND_DBG_OUTPUT_

#if defined(_PAL_APPEND_DBG_OUTPUT_)
static const char FOPEN_FLAGS[] = "at";
#else
static const char FOPEN_FLAGS[] = "wt";
#endif

/* number of ENTRY nesting levels to indicate with a '.' */
#define MAX_NESTING 50

/* size of output buffer (arbitrary) */
#define DBG_BUFFER_SIZE 20000

/* global and static variables */

DWORD dbg_channel_flags[DCI_LAST];
BOOL dbg_asserts_enabled;

/* we must use stdio functions directly rather that rely on PAL functions for
  output, because those functions do tracing and we need to avoid recursion */
FILE *output_file = NULL;

/* master switch for debug channel enablement, to be modified by debugger */
volatile BOOL dbg_master_switch = TRUE;


static const char *dbg_channel_names[]=
{
    "PAL",
    "LOADER",
    "HANDLE",
    "SHMEM",
    "THREAD",
    "EXCEPT",
    "CRT",
    "UNICODE",
    "ARCH",
    "SYNC",
    "FILE",
    "VIRTUAL",
    "MEM",
    "SOCKET",
    "DEBUG",
    "LOCALE",
    "MISC",
    "MUTEX",
    "CRITSEC"
};

static const char *dbg_level_names[]=
{
    "ENTRY",
    "TRACE",
    "WARN",
    "ERROR",
    "ASSERT",
    "EXIT"
};

static const char ENV_FILE[]="PAL_API_TRACING";
static const char ENV_CHANNELS[]="PAL_DBG_CHANNELS";
static const char ENV_ASSERTS[]="PAL_DISABLE_ASSERTS";
static const char ENV_ENTRY_LEVELS[]="PAL_API_LEVELS";

/* per-thread storage for ENTRY tracing level */
static pthread_key_t entry_level_key;

/* entry level limitation */
static int max_entry_level;

/* character to use for ENTRY indentation */
static const char INDENT_CHAR = '.';

static BOOL DBG_get_indent(DBG_LEVEL_ID level, const char *format, 
                           char *indent_string);

static CRITICAL_SECTION fprintf_crit_section;

/* Function definitions */

/*++
Function :
    DBG_init_channels

    Parse environment variables PAL_DBG_CHANNELS and PAL_API_TRACING for debug
    channel settings; initialize static variables.

    (no parameters, no return value)
--*/
BOOL DBG_init_channels(void)
{
    INT i;
    LPCSTR env_string;
    LPSTR env_workstring;
    LPSTR env_pcache;
    LPSTR entry_ptr;
    LPSTR level_ptr;
    CHAR plus_or_minus;
    DWORD flag_mask = 0;
    int ret;

    if (0 != SYNCInitializeCriticalSection(&fprintf_crit_section))
    {
        return FALSE;
    }

    /* output only asserts by default [only affects no-vararg-support case; if 
       we have varargs, these flags aren't even checked for ASSERTs] */
    for(i=0;i<DCI_LAST;i++)
        dbg_channel_flags[i]=1<<DLI_ASSERT;

    /* parse PAL_DBG_CHANNELS environment variable */

    if (!(env_string = MiscGetenv(ENV_CHANNELS))) 
    {
        env_pcache = env_workstring = NULL;
    }
    else
    {
        env_pcache = env_workstring = strdup(env_string);

        if (env_workstring == NULL)
        {
            /* Not enough memory */
            DeleteCriticalSection(&fprintf_crit_section);;
            return FALSE;
        }
    }
    while(env_workstring)
    {
        entry_ptr=env_workstring;

        /* find beginning of next entry */
        while((*entry_ptr != '\0') &&(*entry_ptr != '+') && (*entry_ptr != '-'))
        {
            entry_ptr++;
        }
        
        /* break if end of string is reached */
        if(*entry_ptr == '\0')
        {
           break;
        }
        
        plus_or_minus=*entry_ptr++;

        /* find end of entry; if strchr returns NULL, we have reached the end
           of the string and we will leave the loop at the end of this pass. */
        env_workstring=strchr(entry_ptr,':');

        /* NULL-terminate entry, make env_string point to rest of string */
        if(env_workstring)
        {
            *env_workstring++='\0';
        }
        
        /* find period that separates channel name from level name */
        level_ptr=strchr(entry_ptr,'.');

        /* an entry with no period is illegal : ignore it */
        if(!level_ptr)
        {
            continue;
        }
        /* NULL-terminate channel name, make level_ptr point to the level name */
        *level_ptr++='\0';

        /* build the flag mask based on requested level */

        /* if "all" level is specified, we want to open/close all levels at
           once, so mask is either all ones or all zeroes */
        if(!strcmp(level_ptr,"all"))
        {
            if(plus_or_minus=='+')
            {
                flag_mask=0xFFFF;  /* OR this to open all levels */
            }
            else
            {
                flag_mask=0;       /* AND this to close all levels*/
            }
        }
        else
        {
            for(i=0;i<DLI_LAST;i++)
            {
                if(!strcmp(level_ptr,dbg_level_names[i]))
                {
                    if(plus_or_minus=='+')
                    {
                        flag_mask=1<<i;     /* OR this to open the level */
                    }
                    else
                    {
                        flag_mask=~(1<<i);  /* AND this to close the level */
                    }
                    break;
                }
            }
            /* didn't find a matching level : skip it. */
            if(i==DLI_LAST)
            {
                continue;
            }
        }

        /* Set EXIT and ENTRY channels to be identical */
        if(!(flag_mask & (1<<DLI_ENTRY)))
        {
            flag_mask = flag_mask & (~(1<<DLI_EXIT));
        }
        else
        {
            flag_mask = flag_mask | (1<<DLI_EXIT);
        }

        /* apply the flag mask to the specified channel */

        /* if "all" channel is specified, apply mask to all channels */
        if(!strcmp(entry_ptr,"all"))
        {
            if(plus_or_minus=='+')
            {
                for(i=0;i<DCI_LAST;i++)
                {
                    dbg_channel_flags[i] |= flag_mask; /* OR to open levels*/
                }
            } 
            else
            {
                for(i=0;i<DCI_LAST;i++)
                {
                    dbg_channel_flags[i] &= flag_mask; /* AND to close levels */
                }
            }
        }
        else
        {
            for(i=0;i<DCI_LAST;i++)
            {
                if(!strcmp(entry_ptr,dbg_channel_names[i]))
                {
                    if(plus_or_minus=='+')
                    {
                        dbg_channel_flags[i] |= flag_mask;
                    }
                    else
                    {
                        dbg_channel_flags[i] &= flag_mask;
                    }

                    break;
                }
            }
            /* ignore the entry if the channel name is unknown */
        }
        /* done processing this entry; on to the next. */
    }
    free(env_pcache);

    /* select output file */
    env_string=MiscGetenv(ENV_FILE);
    if(env_string && *env_string!='\0')
    {
        if(!strcmp(env_string, "stderr"))
        {
            output_file=stderr;
        }
        else if(!strcmp(env_string, "stdout"))
        {
            output_file=stdout;
        }
        else
        {
            output_file=fopen(env_string,FOPEN_FLAGS);

            /* if file can't be opened, default to stderr */
            if(!output_file)
            {
                output_file=stderr;
                fprintf(stderr, "Can't open %s for writing : debug messages "
                        "will go to stderr. Check your PAL_API_TRACING "
                        "variable!\n", env_string);
            }
        }
    } else output_file = stderr; /* output to stderr by default */

    /* see if we need to disable assertions */
    env_string = MiscGetenv(ENV_ASSERTS);
    if(env_string && 0 == strcmp(env_string,"1"))
    {
        dbg_asserts_enabled = FALSE;
    }
    else
    {
        dbg_asserts_enabled = TRUE;
    }

    /* select ENTRY level limitation */
    env_string = MiscGetenv(ENV_ENTRY_LEVELS);    
    if(env_string)
    {
        max_entry_level = atoi(env_string);
    }
    else
    {
        max_entry_level = 1;
    }

    /* if necessary, allocate TLS key for entry nesting level */
    if(0 != max_entry_level)
    {
        if ((ret = pthread_key_create(&entry_level_key,NULL)) != 0)
        {
            fprintf(stderr, "ERROR : pthread_key_create() failed error:%d (%s)\n", 
                   ret, strerror(ret));
            DeleteCriticalSection(&fprintf_crit_section);;
            return FALSE;
        }
    }

    return TRUE;
}

/*++
Function :
    DBG_close_channels

    Stop outputting debug messages by closing the associated file.

    (no parameters, no return value)
--*/
void DBG_close_channels(void)
{
    if(output_file && output_file!=stderr && output_file!=stdout)
    {
        if (fclose(output_file) != 0)
        {
            fprintf(stderr, "ERROR : fclose() failed errno:%d (%s)\n", 
                   errno, strerror(errno));
        }
    }

    output_file=NULL;

    DeleteCriticalSection(&fprintf_crit_section);

    /* if necessary, release TLS key for entry nesting level */
    if(0 != max_entry_level)
    {
        int retval;

        retval = pthread_key_delete(entry_level_key);
        if(0 != retval)
        {
            fprintf(stderr, "ERROR : pthread_key_delete() returned %d! (%s)\n",
                    retval, strerror(retval));
        }
    }
}


/*++
Function :
    DBG_printf_gcc

    Internal function for debug channels; don't use.
    This function outputs a complete debug message, including the function name.

Parameters :
    DBG_CHANNEL_ID channel : debug channel to use
    DBG_LEVEL_ID level : debug message level
    BOOL bHeader : whether or not to output message header (thread id, etc)
    LPSTR function : current function
    LPSTR file : current file
    INT line : line number
    LPSTR format, ... : standard printf parameter list.

Return Value :
    always 1.

Notes :
    This version is for gnu compilers that support variable-argument macros
    and the __FUNCTION__ pseudo-macro.

--*/
int DBG_printf_gcc(DBG_CHANNEL_ID channel, DBG_LEVEL_ID level, BOOL bHeader,
                   LPSTR function, LPSTR file, INT line, LPSTR format, ...)
{
    CHAR buffer[DBG_BUFFER_SIZE];
    CHAR indent[MAX_NESTING+1];
    LPSTR buffer_ptr;
    INT output_size;
    va_list args;
    INT thread_id;
    int old_errno = 0;

    old_errno = errno;

    if(!DBG_get_indent(level, format, indent))
    {
        return 1;
    }

    thread_id = (INT)THREADSilentGetCurrentThreadId();

    if(bHeader)
    {
        /* Print file instead of function name for ENTRY messages, because those
           already include the function name */
        /* also print file name for ASSERTs, to match Win32 behavior */
        if( DLI_ENTRY == level || DLI_ASSERT == level || DLI_EXIT == level)
        {
            output_size=snprintf(buffer, sizeof(buffer), 
                                 "{%08X} %-5s [%-7s] at %s.%d: ",
                                 thread_id, dbg_level_names[level],
                                 dbg_channel_names[channel], file, line);
        }
        else
        {
            output_size=snprintf(buffer, sizeof(buffer),
                                 "{%08X} %-5s [%-7s] at %s.%d: ",
                                 thread_id, dbg_level_names[level],
                                 dbg_channel_names[channel], function, line);
        }
        
        if(output_size + 1 > sizeof(buffer))
        {
            fprintf(stderr, "ERROR : buffer overflow in DBG_printf_gcc");
            return 1;
        }
        
        buffer_ptr=buffer+output_size;
    }
    else
    {
        buffer_ptr = buffer;
        output_size = 0;
    }

    va_start(args, format);

    output_size+=Silent_PAL_vsnprintf(buffer_ptr, DBG_BUFFER_SIZE-output_size,
                                      format, args);
    va_end(args);

    if( output_size > DBG_BUFFER_SIZE )
    {
        fprintf(stderr, "ERROR : buffer overflow in DBG_printf_gcc");
    }

    /* Use a Critical section before calling printf code to
       avoid holding a libc lock while another thread is calling
       SuspendThread on this one. */
    SYNCEnterCriticalSection(&fprintf_crit_section, TRUE);
    fprintf( output_file,"%s%s",indent, buffer );
    SYNCLeaveCriticalSection(&fprintf_crit_section, TRUE);

    /* flush the output to file */
    if (fflush(output_file) != 0)
    {
        fprintf(stderr, "ERROR : fflush() failed errno:%d (%s)\n", 
                errno, strerror(errno));
    }

    if ( old_errno != errno )
    {
        fprintf( stderr,"ERROR: errno changed by DBG_printf_gcc\n" );
        errno = old_errno;
    }

    return 1;
}

/*++
Function :
    DBG_printf_c99

    Internal function for debug channels; don't use.
    This function outputs a complete debug message, without function name.

Parameters :
    DBG_CHANNEL_ID channel : debug channel to use
    DBG_LEVEL_ID level : debug message level
    BOOL bHeader : whether or not to output message header (thread id, etc)
    LPSTR file : current file
    INT line : line number
    LPSTR format, ... : standard printf parameter list.

Return Value :
    always 1.

Notes :
    This version is for compilers that support the C99 flavor of
    variable-argument macros but not the gnu flavor, and do not support the
    __FUNCTION__ pseudo-macro.

--*/
int DBG_printf_c99(DBG_CHANNEL_ID channel, DBG_LEVEL_ID level, BOOL bHeader,
                   LPSTR file, INT line, LPSTR format, ...)
{
    CHAR buffer[DBG_BUFFER_SIZE];
    CHAR indent[MAX_NESTING+1];
    LPSTR buffer_ptr;
    INT output_size;
    va_list args;
    static INT call_count=0;
    UINT thread_id;
    int old_errno = 0;

    old_errno = errno;

    if(!DBG_get_indent(level, format, indent))
    {
        return 1;
    }

    thread_id=(INT) THREADSilentGetCurrentThreadId();

    if(bHeader)
    {
        output_size=snprintf(buffer, sizeof(buffer), 
                             "{%08X} %-5s [%-7s] at %s.%d: ", thread_id,
                             dbg_level_names[level], dbg_channel_names[channel], 
                             file, line);

        if(output_size + 1 > sizeof(buffer))
        {
            fprintf(stderr, "ERROR : buffer overflow in DBG_printf_gcc");
            return 1;
        }
        
        buffer_ptr=buffer+output_size;
    }
    else
    {
        output_size = 0;
        buffer_ptr = buffer;
    }

    va_start(args, format);
    output_size+=Silent_PAL_vsnprintf(buffer_ptr, DBG_BUFFER_SIZE-output_size, 
                                      format, args);
    va_end(args);

    if(output_size>DBG_BUFFER_SIZE)
        fprintf(stderr, "ERROR : buffer overflow in DBG_printf_c99");

    /* Use a Critical section before calling printf code to
       avoid holding a libc lock while another thread is calling
       SuspendThread on this one. */
    SYNCEnterCriticalSection(&fprintf_crit_section, TRUE);
    fprintf( output_file,"%s", buffer );
    SYNCLeaveCriticalSection(&fprintf_crit_section, TRUE);

    /* flush the output to file every once in a while */
    call_count++;
    if(call_count>5)
    {
        call_count=0;
        if (fflush(output_file) != 0)
        {
            fprintf(stderr, "ERROR : fflush() failed errno:%d (%s)\n", 
                   errno, strerror(errno));
        }
    }
    
    if ( old_errno != errno )
    {
        fprintf( stderr, "ERROR: DBG_printf_c99 changed the errno.\n" );
        errno = old_errno;
    }

    return 1;
}

/*++
Function :
    DBG_get_indent

    generate an indentation string to be used for message output

Parameters :                                  
    DBG_LEVEL_ID level  : level of message (DLI_ENTRY, etc)
    const char *format  : printf format string of message
    char *indent_string : destination for indentation string

Return value :
    TRUE if output can proceed, FALSE otherwise
    
Notes:
As a side-effect, this function updates the ENTRY nesting level for the current 
thread : it decrements it if 'format' contains the string 'return', increments 
it otherwise (but only if 'level' is DLI_ENTRY). The function will return 
FALSE if the current nesting level is beyond our treshold (max_nesting_level);
it always returns TRUE for other message levels
--*/
static BOOL DBG_get_indent(DBG_LEVEL_ID level, const char *format, 
                           char *indent_string)
{
    int ret;

    /* determine whether to output an ENTRY line */
    if(DLI_ENTRY == level||DLI_EXIT == level)
    {
        if(0 != max_entry_level)
        {
            int nesting;

            /* Determine if this is an entry or an
               exit */
            if(DLI_EXIT == level)
            {
                nesting = (int)pthread_getspecific(entry_level_key);
                /* avoid going negative */
                if(nesting != 0)
                {
                    nesting--;
                    if ((ret = pthread_setspecific(entry_level_key,
                                                     (LPVOID)nesting)) != 0)
                    {
                        fprintf(stderr, "ERROR : pthread_setspecific() failed "
                                "error:%d (%s)\n", ret, strerror(ret));
                    }
                }
            }
            else
            {
                nesting = (int)pthread_getspecific(entry_level_key);

                if ((ret = pthread_setspecific(entry_level_key,
                                                 (LPVOID)(nesting+1))) != 0)
                {
                    fprintf(stderr, "ERROR : pthread_setspecific() failed "
                            "error:%d (%s)\n", ret, strerror(ret));
                }
            }

            /* see if we're past the level treshold */
            if(nesting >= max_entry_level)
            {
                return FALSE;
            }

            /* generate indentation string */
            if(MAX_NESTING < nesting)
            {
                nesting = MAX_NESTING;
            }
            memset(indent_string,INDENT_CHAR ,nesting);
            indent_string[nesting] = '\0';
        }
        else
        {
            indent_string[0] = '\0';
        }
    }
    else
    {
        indent_string[0] = '\0';
    }
    return TRUE;
}

/*++
Function :
    DBG_change_entrylevel

    retrieve current ENTRY nesting level and [optionnally] modify it

Parameters :
    int new_level : value to which the nesting level must be set, or -1

Return value :
    nesting level at the time the function was called
    
Notes:
if new_level is -1, the nesting level will not be modified
--*/
int DBG_change_entrylevel(int new_level)
{
    int old_level;
    int ret;

    if(0 == max_entry_level)
    {
        return 0;
    }
    old_level = (int)pthread_getspecific(entry_level_key);
    if(-1 != new_level)
    {
        if ((ret = pthread_setspecific(entry_level_key,(LPVOID)new_level)) != 0)
        {
            fprintf(stderr, "ERROR : pthread_setspecific() failed "
                    "error:%d (%s)\n", ret, strerror(ret));
        }
    }
    return old_level;
}

