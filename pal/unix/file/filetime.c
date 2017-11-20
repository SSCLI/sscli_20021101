/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    filetime.c

Abstract:

    Implementation of the file WIN API related to file time.

Notes:

One very important thing to note is that on BSD systems, the stat structure
stores nanoseconds for the time-related fields. This is implemented by
replacing the time_t fields st_atime, st_mtime, and st_ctime by timespec
structures, instead named st_atimespec, st_mtimespec, and st_ctimespec.

However, if _POSIX_SOURCE is defined, the fields are time_t values and use
their POSIX names. For compatibility purposes, when _POSIX_SOURCE is NOT
defined, the time-related fields are defined in sys/stat.h as:

#ifndef _POSIX_SOURCE
#define st_atime st_atimespec.tv_sec
#define st_mtime st_mtimespec.tv_sec
#define st_ctime st_ctimespec.tv_sec
#endif

Furthermore, if _POSIX_SOURCE is defined, the structure still has
additional fields for nanoseconds, named st_atimensec, st_mtimensec, and
st_ctimensec.

In the PAL, there is a configure check to see if the system supports
nanoseconds for the time-related fields. This source file also sets macros
so that STAT_ATIME_NSEC etc. will always refer to the appropriate field
if it exists, and are defined as 0 otherwise.

--

Also note that there is no analog to "creation time" on Unix systems.
Instead, we use the inode change time, which is set to the current time
whenever mtime changes or when chmod, chown, etc. syscalls modify the
file status.

--*/

#include "pal/palinternal.h"
#include "pal/dbgmsg.h"
#include "pal/file.h"
#include "pal/filetime.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <time.h>
#include <fcntl.h>
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif  // HAVE_SYS_TIME_H

SET_DEFAULT_DEBUG_CHANNEL(FILE);

/* Magic number explanation:
   Both epochs are Gregorian. 1970 - 1601 = 369. Assuming a leap
   year every four years, 369 / 4 = 92. However, 1700, 1800, and 1900
   were NOT leap years, so 89 leap years, 280 non-leap years.
   89 * 366 + 280 * 365 = 134744 days between epochs. Of course
   60 * 60 * 24 = 86400 seconds per day, so 134744 * 86400 =
   11644473600 = SECS_BETWEEN_EPOCHS.

   This result is also confirmed in the MSDN documentation on how
   to convert a time_t value to a win32 FILETIME.
*/
static const __int64 SECS_BETWEEN_EPOCHS = 11644473600;
static const __int64 SECS_TO_100NS = 10000000; /* 10^7 */

/* to be used as args to FILEAdjustFileTimeForTimezone */
#define ADJUST_FROM_UTC -1
#define ADJUST_TO_UTC    1

static FILETIME FILEAdjustFileTimeForTimezone( const FILETIME *Orig,
                                               int Direction );


/*++
Function:
  CompareFileTime

See MSDN doc.
--*/
LONG
PALAPI
CompareFileTime(
        IN CONST FILETIME *lpFileTime1,
        IN CONST FILETIME *lpFileTime2)
{
    __int64 First;
    __int64 Second;

    long Ret;

    ENTRY("CompareFileTime(lpFileTime1=%p lpFileTime2=%p)\n", 
          lpFileTime1, lpFileTime2);

    First = ((__int64)lpFileTime1->dwHighDateTime << 32) +
        lpFileTime1->dwLowDateTime;
    Second = ((__int64)lpFileTime2->dwHighDateTime << 32) +
        lpFileTime2->dwLowDateTime;

    if ( First < Second )
    {
        Ret = -1;
    }
    else if ( First > Second )
    {
        Ret = 1;
    }
    else
    {
        Ret = 0;
    }
    
    LOGEXIT("CompareFileTime returns LONG %ld\n", Ret);
    return Ret;
}



/*++
Function:
  SetFileTime

Notes: This function will drop one digit (radix 10) of precision from
the supplied times, since Unix can set to the microsecond (at most, i.e.
if the futimes() function is available).

As noted in the file header, there is no analog to "creation time" on Unix
systems, so the lpCreationTime argument to this function will always be
ignored, and the inode change time will be set to the current time.
--*/
BOOL
PALAPI
SetFileTime(
        IN HANDLE hFile,
        IN CONST FILETIME *lpCreationTime,
        IN CONST FILETIME *lpLastAccessTime,
        IN CONST FILETIME *lpLastWriteTime)
{
    struct timeval Times[2];
    long nsec;
    file *file_data = NULL;
    BOOL  bRet = FALSE;
    DWORD dwLastError = 0;
    const UINT64 MAX_FILETIMEVALUE = 0x8000000000000000;

    ENTRY("SetFileTime(hFile=%p, lpCreationTime=%p, lpLastAccessTime=%p, "
          "lpLastWriteTime=%p)\n", hFile, lpCreationTime, lpLastAccessTime, 
          lpLastWriteTime);
    /* validate filetime values */
    if ( (lpCreationTime && (((UINT64)lpCreationTime->dwHighDateTime   << 32) + 
          lpCreationTime->dwLowDateTime   >= MAX_FILETIMEVALUE)) ||        
         (lpLastAccessTime && (((UINT64)lpLastAccessTime->dwHighDateTime << 32) + 
          lpLastAccessTime->dwLowDateTime >= MAX_FILETIMEVALUE)) ||
         (lpLastWriteTime && (((UINT64)lpLastWriteTime->dwHighDateTime  << 32) + 
          lpLastWriteTime->dwLowDateTime  >= MAX_FILETIMEVALUE)))
    {
        dwLastError = ERROR_INVALID_HANDLE;
        goto done;
    }

    file_data = FILEAcquireFileStruct(hFile);

    if ( !file_data )
    {
        dwLastError = ERROR_INVALID_HANDLE;
        goto done;
    }
    
    if (lpLastAccessTime)
    {
        Times[0].tv_sec = FILEFileTimeToUnixTime( *lpLastAccessTime, &nsec );
        Times[0].tv_usec = nsec / 1000; /* convert to microseconds */
    }

    if (lpLastWriteTime)
    {
        Times[1].tv_sec = FILEFileTimeToUnixTime( *lpLastWriteTime, &nsec );
        Times[1].tv_usec = nsec / 1000; /* convert to microseconds */
    }

    if (lpCreationTime)
    {
        dwLastError = ERROR_NOT_SUPPORTED;
        goto done;
    }

    TRACE("Setting atime = [%ld.%ld], mtime = [%ld.%ld]\n",
          Times[0].tv_sec, Times[0].tv_usec,
          Times[1].tv_sec, Times[1].tv_usec);

#if HAVE_FUTIMES
    if ( futimes(file_data->unix_fd, Times) != 0 )
#elif HAVE_UTIMES
    if ( utimes(file_data->unix_filename, Times) != 0 )
#else
  #error Operating system not supported
#endif
    {
        dwLastError = FILEGetLastErrorFromErrno();
    }
    else
    {
        bRet = TRUE;
    }

done:
    if (file_data) 
    {
        FILEReleaseFileStruct(hFile, file_data);
    }
    if (dwLastError) 
    {
        SetLastError(dwLastError);
    }
    LOGEXIT("SetFileTime returns BOOL %d\n", bRet);
    return bRet;
}


/*++
Function:
  GetFileTime

Notes: As noted at the top of this file, there is no analog to "creation
time" on Unix systems, so the inode change time is used instead. Also, Win32
LastAccessTime is updated after a write operation, but it is not on Unix.
To be consistent with Win32, this function returns the greater of mtime and
atime for LastAccessTime.
--*/
BOOL
PALAPI
GetFileTime(
        IN HANDLE hFile,
        OUT LPFILETIME lpCreationTime,
        OUT LPFILETIME lpLastAccessTime,
        OUT LPFILETIME lpLastWriteTime)
{
    file  *FileData = NULL;
    int   Fd = -1;

    struct stat StatData;

    DWORD dwLastError = 0;
    BOOL  bRet = FALSE;

    ENTRY("GetFileTime(hFile=%p, lpCreationTime=%p, lpLastAccessTime=%p, "
          "lpLastWriteTime=%p)\n",
          hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);

    FileData = FILEAcquireFileStruct(hFile);
    Fd = FileData?FileData->unix_fd:-1;

    if ( !FileData || Fd == -1 )
    {
        TRACE("FileData = [%p], Fd = %d\n", FileData, Fd);
        dwLastError = ERROR_INVALID_HANDLE;
        goto done;
    }

    if ( fstat(Fd, &StatData) != 0 )
    {
        TRACE("fstat failed on file descriptor %d\n", Fd);
        dwLastError = FILEGetLastErrorFromErrno();
        goto done;
    }

    if ( lpCreationTime )
    {
        *lpCreationTime = FILEUnixTimeToFileTime(StatData.st_ctime,
                                                 ST_CTIME_NSEC(&StatData));
    }
    if ( lpLastWriteTime )
    {
        *lpLastWriteTime = FILEUnixTimeToFileTime(StatData.st_mtime,
                                                  ST_MTIME_NSEC(&StatData));
    }
    if ( lpLastAccessTime )
    {
        *lpLastAccessTime = FILEUnixTimeToFileTime(StatData.st_atime,
                                                   ST_ATIME_NSEC(&StatData));
        /* if Unix mtime is greater than atime, return mtime as the last
           access time */
        if ( lpLastWriteTime &&
             CompareFileTime( (const FILETIME*)lpLastAccessTime,
                              (const FILETIME*)lpLastWriteTime ) < 0 )
        {
            *lpLastAccessTime = *lpLastWriteTime;
        }
    }
    bRet = TRUE;

done:
    if (FileData)
    {
        FILEReleaseFileStruct(hFile, FileData);
    }
    if (dwLastError) 
    {
        SetLastError(dwLastError);
    }

    LOGEXIT("GetFileTime returns BOOL %d\n", bRet);
    return bRet;
}



/*++
Function:
  FileTimeToLocalTime

See MSDN doc.
--*/
BOOL
PALAPI
FileTimeToLocalFileTime(
            IN CONST FILETIME *lpFileTime,
            OUT LPFILETIME lpLocalFileTime)
{
    ENTRY("FileTimeToLocalFileTime(lpFileTime=%p, lpLocalFileTime=%p)\n", 
          lpFileTime, lpLocalFileTime);

    *lpLocalFileTime = FILEAdjustFileTimeForTimezone(lpFileTime, 
                                                     ADJUST_FROM_UTC);
    LOGEXIT("FileTimeToLocalFileTime returns BOOL TRUE\n");
    return TRUE;
}


/*++
Function:
  LocalTimeToFileTime

--*/
BOOL
PALAPI
LocalFileTimeToFileTime(
            IN CONST FILETIME *lpLocalFileTime,
            OUT LPFILETIME lpFileTime)
{
    ENTRY("LocalFileTimeToFileTime(lpLocalFileTime=%p, lpFileTime=%p)\n", 
          lpLocalFileTime, lpFileTime);

    *lpFileTime = FILEAdjustFileTimeForTimezone(lpLocalFileTime, 
                                                ADJUST_TO_UTC);
    LOGEXIT("LocalFileTimeToFileTime returns BOOL TRUE\n");
    return TRUE;
}



/*++
Function:
  GetSystemTimeAsFileTime

See MSDN doc.
--*/
VOID
PALAPI
GetSystemTimeAsFileTime(
            OUT LPFILETIME lpSystemTimeAsFileTime)
{
    struct timeval Time;

    ENTRY("GetSystemTimeAsFileTime(lpSystemTimeAsFileTime=%p)\n", 
          lpSystemTimeAsFileTime);

    if ( gettimeofday( &Time, NULL ) != 0 )
    {
        ASSERT("gettimeofday() failed");
        /* no way to indicate failure, so set time to zero */
        *lpSystemTimeAsFileTime = FILEUnixTimeToFileTime( 0, 0 );
    }
    else
    {
        /* use (tv_usec * 1000) because 2nd arg is in nanoseconds */
        *lpSystemTimeAsFileTime = FILEUnixTimeToFileTime( Time.tv_sec,
                                                          Time.tv_usec * 1000 );
    }

    LOGEXIT("GetSystemTimeAsFileTime returns.\n");
}


/*++
Function:
  FILEUnixTimeToFileTime

Convert a time_t value to a win32 FILETIME structure, as described in
MSDN documentation. time_t is the number of seconds elapsed since 
00:00 01 January 1970 UTC (Unix epoch), while FILETIME represents a 
64-bit number of 100-nanosecond intervals that have passed since 00:00 
01 January 1601 UTC (win32 epoch).
--*/
FILETIME FILEUnixTimeToFileTime( time_t sec, long nsec )
{
    __int64 Result;
    FILETIME Ret;

    Result = ((__int64)sec + SECS_BETWEEN_EPOCHS) * SECS_TO_100NS +
        (nsec / 100);

    Ret.dwLowDateTime = (DWORD)Result;
    Ret.dwHighDateTime = (DWORD)(Result >> 32);

    TRACE("Unix time = [%ld.%09ld] converts to Win32 FILETIME = [%#x:%#x]\n", 
          sec, nsec, Ret.dwHighDateTime, Ret.dwLowDateTime);

    return Ret;
}


/*++
Function:
  FILEFileTimeToUnixTime

See FILEUnixTimeToFileTime above.

This function takes a win32 FILETIME structures, returns the equivalent
time_t value, and, if the nsec parameter is non-null, also returns the
nanoseconds.

NOTE: a 32-bit time_t is only capable of representing dates between
13 December 1901 and 19 January 2038. This function will calculate the
number of seconds (positive or negative) since the Unix epoch, however if
this value is outside of the range of 32-bit numbers, the result will be
truncated on systems with a 32-bit time_t.
--*/
time_t FILEFileTimeToUnixTime( FILETIME FileTime, long *nsec )
{
    __int64 UnixTime;

    /* get the full win32 value, in 100ns */
    UnixTime = ((__int64)FileTime.dwHighDateTime << 32) + 
        FileTime.dwLowDateTime;

    /* convert to the Unix epoch */
    UnixTime -= (SECS_BETWEEN_EPOCHS * SECS_TO_100NS);

    TRACE("nsec=%p\n", nsec);

    if ( nsec )
    {
        /* get the number of 100ns, convert to ns */
        *nsec = (UnixTime % SECS_TO_100NS) * 100;
    }

    UnixTime /= SECS_TO_100NS; /* now convert to seconds */

    if ( (time_t)UnixTime != UnixTime )
    {
        WARN("Resulting value is too big for a time_t value\n");
    }

    TRACE("Win32 FILETIME = [%#x:%#x] converts to Unix time = [%ld.%09ld]\n", 
          FileTime.dwHighDateTime, FileTime.dwLowDateTime ,(long) UnixTime,
          nsec?*nsec:0L);

    return (time_t)UnixTime;
}


/*++
Function:
  FILEAdjustFileTimeForTimezone

Given a FILETIME, adjust it for the local time zone.
Direction is ADJUST_FROM_UTC to go from UTC to localtime, and
ADJUST_TO_UTC to go from localtime to UTC.

The offset from UTC can be determined in one of two ways. On *BSD,
struct tm contains a tm_gmtoff field which is a signed integer indicating
the difference, in seconds, between local time and UTC at the supplied
time. eg. in EST, tm_gmtoff is -18000. This value _will_ account for 
Daylight Savings Time, however that is of no concern since the time and
date in question is 1 January.

On SysV systems, there exists an external variable named timezone, which
is similar to the tm_gmtoff field, but stores the seconds west of UTC.
Thus for EST, timezone == 18000. This variable does not account for DST.
--*/
static FILETIME FILEAdjustFileTimeForTimezone( const FILETIME *Orig,
                                               int Direction )
{
    __int64 FullTime;
    __int64 Offset;

    FILETIME Ret;

    struct tm *tmTimePtr;
#if HAVE_LOCALTIME_R
    struct tm tmTime;
#endif  /* HAVE_LOCALTIME_R */
    time_t timeNow;

    /* Use the current time and date to calculate UTC offset */
    timeNow = time(NULL);
#if HAVE_LOCALTIME_R
    tmTimePtr = &tmTime;
    localtime_r( &timeNow, tmTimePtr );
#else   /* HAVE_LOCALTIME_R */
    tmTimePtr = localtime( &timeNow );
#endif  /* HAVE_LOCALTIME_R */

#if HAVE_TM_GMTOFF
    Offset = -tmTimePtr->tm_gmtoff;
#elif HAVE_TIMEZONE_VAR
    // Adjust timezone for DST to get the appropriate offset.
    Offset = timezone - (tmTimePtr->tm_isdst ? 3600 : 0);
#else   // !(HAVE_TM_GMTOFF || HAVE_TIMEZONE_VAR)

#error Unable to determine timezone information
    Offset = 0; // Quiet the warning about Offset being used unitialized
#endif

    TRACE("Calculated UTC offset to be %I64d seconds\n", Offset);

    FullTime = (((__int64)Orig->dwHighDateTime) << 32) + 
        Orig->dwLowDateTime;

    FullTime += Offset * SECS_TO_100NS * Direction;

    Ret.dwLowDateTime = (DWORD)(FullTime);
    Ret.dwHighDateTime = (DWORD)(FullTime >> 32);

    return Ret;
}


/*++
Function:
  DosDateTimeToFileTime

See MSDN doc.
--*/
BOOL PALAPI DosDateTimeToFileTime(
                                 IN WORD wFatDate,
                                 IN WORD wFatTime,
                                 OUT LPFILETIME lpFileTime
                                  )
{

    BOOL bRet = FALSE;
    struct tm  tmTime = { 0 };
    time_t    tmUnix;

    ENTRY("DosDateTimeToFileTime(wFatdate=%#hx,wFatTime=%#hx,lpFileTime=%p)\n",
          wFatDate,wFatTime,lpFileTime);
    /*Breakdown wFatDate & wfatTime to fill the tm structure */
    /*wFatDate contains the Date data & wFatTime time data*/
    /*wFatDate 0-4 bits-Day of month(0-31)*/
    /*5-8 Month(1=Jan,2=Feb)*/
    /*9-15 Year offset from 1980*/
    /*wFtime 0-4 Second divided by 2*/
    /*5-10 Minute(0-59)*/
    /*11-15 Hour 0-23 on a 24 hour clock*/

    tmTime.tm_mday = (wFatDate & 0x1F);
    if ( tmTime.tm_mday < 1 || tmTime.tm_mday > 31 )
    {
        ERROR( "Incorrect day format was passed in.\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto done;
    }
    
    /*tm_mon is the no. of months from january*/
    tmTime.tm_mon = ((wFatDate >> 5) & 0x0F)-1;
    if ( tmTime.tm_mon < 0 || tmTime.tm_mon > 11 )
    {
        ERROR( "Incorrect month format was passed in.\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto done;
    }

    /*tm_year is the no. of years from 1900*/
    tmTime.tm_year = ((wFatDate >> 9) & 0x7F) + 80;
    if ( tmTime.tm_year < 0 || tmTime.tm_year > 207 )
    {
        ERROR( "Incorrect year format was passed in. %d\n", tmTime.tm_year );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto done;
    }

    tmTime.tm_sec = ( (wFatTime & 0x1F)*2 );
    if ( tmTime.tm_sec < 0 || tmTime.tm_sec > 59 )
    {
        ERROR( "Incorrect sec format was passed in.\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto done;
    }
    
    tmTime.tm_min = ((wFatTime >> 5) & 0x3F);
    if ( tmTime.tm_min < 0 || tmTime.tm_min > 59 )
    {
        ERROR( "Incorrect minute format was passed in.\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto done;
    }

    tmTime.tm_hour = ((wFatTime >> 11) & 0x1F);
    if ( tmTime.tm_hour < 0 || tmTime.tm_hour > 23 )
    {
        ERROR( "Incorrect hour format was passed in.\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto done;
    }
    
    // Get the time in seconds
#if HAVE_TIMEGM
    tmUnix = timegm(&tmTime);
#elif HAVE_TIMEZONE_VAR
    // Have the system try to determine if DST is being observed.
    tmTime.tm_isdst = -1;
    
    tmUnix = mktime(&tmTime);
    // mktime() doesn't take the time zone into account. Adjust it
    // by the time zone, but also consider DST to get the appropriate
    // offset.
    if (tmUnix > timezone - (tmTime.tm_isdst ? 3600 : 0))
    {
        tmUnix -= timezone - (tmTime.tm_isdst ? 3600 : 0);
    }
    else
    {
        ERROR( "Received a time before the epoch.\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto done;
    }
#else
#error Unable to calculate time offsets on this platform
    tmUnix = 0; // Avoid a compiler warning about tmUnix being uninitialized
#endif
    
    /*check if the output buffer is valid*/
    if( lpFileTime )
    {
        *lpFileTime = FILEUnixTimeToFileTime( tmUnix, 0 );
        bRet = TRUE;
    }

done:
    LOGEXIT("DosDateTimeToFileTime returns BOOl  %d \n",bRet);
    return bRet;

}

/**
Function 

    FileTimeToSystemTime()
    
    Helper function for FileTimeToDosTime.
    Converts the necessary file time attibutes to system time, for
    easier manipulation in FileTimeToDosTime.
        
--*/
static BOOL FileTimeToSystemTime( CONST FILETIME * lpFileTime, 
                                  LPSYSTEMTIME lpSystemTime )
{
    long long int FileTime = 0;
    time_t UnixFileTime = 0;
    struct tm * UnixSystemTime = 0;

    /* Combine the file time. */
    FileTime = lpFileTime->dwHighDateTime;
    FileTime <<= 32;
    FileTime |= (UINT)lpFileTime->dwLowDateTime;
    FileTime -= SECS_BETWEEN_EPOCHS * SECS_TO_100NS;
                      
    if ( FileTime < 0x8000000000000000LL )
    {
#if HAVE_GMTIME_R
        struct tm timeBuf;
#endif  /* HAVE_GMTIME_R */
        /* Convert file time to unix time. */
        if ( FileTime < 0 )
        {
            UnixFileTime =  -1 - ( ( -FileTime - 1 ) / 10000000 );            
        }
        else
        {
            UnixFileTime = FileTime / 10000000;
        }

        /* Convert unix file time to Unix System time. */
#if HAVE_GMTIME_R
        UnixSystemTime = gmtime_r( &UnixFileTime, &timeBuf );
#else   /* HAVE_GMTIME_R */
        UnixSystemTime = gmtime( &UnixFileTime );
#endif  /* HAVE_GMTIME_R */

        /* Convert unix system time to Windows system time. */
        lpSystemTime->wDay      = UnixSystemTime->tm_mday;
    
        /* Unix time counts January as a 0, under Windows it is 1*/
        lpSystemTime->wMonth    = UnixSystemTime->tm_mon + 1;
        /* Unix time returns the year - 1900, Windows returns the current year*/
        lpSystemTime->wYear     = UnixSystemTime->tm_year + 1900;
        
        lpSystemTime->wSecond   = UnixSystemTime->tm_sec;
        lpSystemTime->wMinute   = UnixSystemTime->tm_min;
        lpSystemTime->wHour     = UnixSystemTime->tm_hour;
        return TRUE;
    }
    else
    {
        ERROR( "The file time is to large.\n" );
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}
    
/**
Function:
    FileTimeToDosDateTime
    
    Notes due to the difference between how BSD and Windows
    calculates time, this function can only repersent dates between
    1980 and 2037. 2037 is the upperlimit for the BSD time functions( 1900 - 
    2037 range ).
    
See msdn for more details.
--*/
BOOL
PALAPI
FileTimeToDosDateTime(
            IN CONST FILETIME *lpFileTime,
            OUT LPWORD lpFatDate,
            OUT LPWORD lpFatTime )
{
    BOOL bRetVal = FALSE;

    ENTRY( "FileTimeToDosDateTime( lpFileTime=%p, lpFatDate=%p, lpFatTime=%p )\n",
           lpFileTime, lpFatDate, lpFatTime );

    /* Sanity checks. */
    if ( !lpFileTime || !lpFatDate || !lpFatTime )
    {
        ERROR( "Incorrect parameters.\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    else
    {
        /* Do conversion. */
        SYSTEMTIME SysTime;
        if ( FileTimeToSystemTime( lpFileTime, &SysTime ) )
        {
            if ( SysTime.wYear >= 1980 && SysTime.wYear <= 2037 )
            {
                *lpFatDate = 0;
                *lpFatTime = 0;

                *lpFatDate |= ( SysTime.wDay & 0x1F );
                *lpFatDate |= ( ( SysTime.wMonth & 0xF ) << 5 );
                *lpFatDate |= ( ( ( SysTime.wYear - 1980 ) & 0x7F ) << 9 );

                if ( SysTime.wSecond % 2 == 0 )
                {
                    *lpFatTime |= ( ( SysTime.wSecond / 2 )  & 0x1F );
                }
                else
                {
                    *lpFatTime |= ( ( SysTime.wSecond / 2 + 1 )  & 0x1F );
                }

                *lpFatTime |= ( ( SysTime.wMinute & 0x3F ) << 5 );
                *lpFatTime |= ( ( SysTime.wHour & 0x1F ) << 11 );
                
                bRetVal = TRUE;
            }
            else
            {
                ERROR( "The function can only repersent dates between 1/1/1980"
                       " and 12/31/2037\n" );
                SetLastError( ERROR_INVALID_PARAMETER );
            }
        }
        else
        {
            ERROR( "Unable to convert file time to system time.\n" );
            SetLastError( ERROR_INVALID_PARAMETER );
            bRetVal = FALSE;
        }
    }

    LOGEXIT( "returning BOOL %d\n", bRetVal );
    return bRetVal;
}

