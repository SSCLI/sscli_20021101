/*=============================================================
**
** Source: UnMapViewOfFile.c
**
** Purpose: Positive test the MapViewOfFile API.
**          Call MapViewOfFile with access FILE_MAP_ALL_ACCESS.
**
** Depends: CreateFile,
**          GetFileSize,
**          memset,
**          memcpy,
**          memcmp,
**          ReadFile,
**          UnMapViewOfFile,
**          CreateFileMapping,
**          CloseHandle.
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
**============================================================*/
#include <palsuite.h>

int __cdecl main(int argc, char *argv[])
{
    const   int MappingSize = 2048;
    HANDLE  hFile;
    HANDLE  hFileMapping;
    LPVOID lpMapViewAddress;
    char    buf[] = "this is a test string";
    char    ch[2048];
    char    readString[2048];
    char    lpFileName[] = "test.tmp";
    DWORD   dwBytesRead;
    BOOL    bRetVal;

    /* Initialize the PAL environment.
     */
    if(0 != PAL_Initialize(argc, argv))
    {
        return FAIL;
    }

    /* Create a file handle with CreateFile.
     */
    hFile = CreateFile( lpFileName,
                        GENERIC_WRITE|GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
                        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        Fail("ERROR: %u :unable to create file \"%s\".\n",
            GetLastError(),
            lpFileName);
    }

    /* Initialize the buffers.
     */
    memset(ch,  0, MappingSize);
    memset(readString,  0, MappingSize);

    /* Create a unnamed file-mapping object with file handle FileHandle
     * and with PAGE_READWRITE protection.
     */
    hFileMapping = CreateFileMapping(
                            hFile,
        NULL,               /*not inherited*/
        PAGE_READWRITE,     /*read and wite*/
        0,                  /*high-order of object size*/
                            MappingSize,        /*low-orger of object size*/
        NULL);              /*unnamed object*/

    if(NULL == hFileMapping)
        {
        Trace("ERROR:%u: Failed to create File Mapping.\n", 
             GetLastError());
        CloseHandle(hFile);
        Fail("");
        }

    /* maps a view of a file into the address space of the calling process.
     */
    lpMapViewAddress = MapViewOfFile(
                            hFileMapping,
                            FILE_MAP_ALL_ACCESS, /* access code */
                            0,                   /* high order offset */
                            0,                   /* low order offset */
                            MappingSize);        /* number of bytes for map */

    if(NULL == lpMapViewAddress)
        {
        Trace("ERROR:%u: Failed to call MapViewOfFile API to map a view"
             " of file!\n",
             GetLastError());
        CloseHandle(hFile);
        CloseHandle(hFileMapping);
        Fail("");
    }

    /* Write to the MapView and copy the MapViewOfFile
     * to buffer, so we can compare with value read from 
     * file directly.
     */

    memcpy(lpMapViewAddress, buf, strlen(buf));
    memcpy(ch, (LPCSTR)lpMapViewAddress, MappingSize);
    
    /* Read from the File handle.
     */
    bRetVal = ReadFile(hFile,
                       readString,
                       strlen(buf),
                       &dwBytesRead,
                       NULL);

    if (bRetVal == FALSE)
        {
        Trace("ERROR: %u :unable to read from file handle "
                "hFile=0x%lx\n",
                GetLastError(),
                hFile);
        CloseHandle(hFile);
        CloseHandle(hFileMapping);
        Fail("");
    }

    if (memcmp(ch, readString, strlen(readString)) != 0)
        {
        Trace("ERROR: Read string from file \"%s\", is "
              "not equal to string written through MapView "
              "\"%s\".\n",
              readString,
              ch);
        CloseHandle(hFile);
        CloseHandle(hFileMapping);
        Fail("");
        }

    /* Unmap the view of file.
     */
    if(UnmapViewOfFile(lpMapViewAddress) == FALSE)
        {
        Trace("ERROR: Failed to call UnmapViewOfFile API to"
             " unmap the view of a file, error code=%u\n",
             GetLastError());
        CloseHandle(hFile);
        CloseHandle(hFileMapping);
        Fail("");
        }

    /* Re-initialize the buffer.
     */
    memset(ch,  0, MappingSize);

    /* Attempt to read from the MapView that has
     * been Un-mapped, should throw an exception.
     */
    PAL_TRY
        {
        memcpy(ch, (LPCSTR)lpMapViewAddress, MappingSize);
        }
    PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Just continue if exception is thrown.
        */        
    }
    PAL_ENDTRY

    /* Close handle to created file mapping.
     */
    if(CloseHandle(hFileMapping) == FALSE)
    {
        Trace("ERROR:%u:Failed to call CloseHandle API "
             "to close a file mapping handle.",
             GetLastError());
        CloseHandle(hFile);
        Fail("");
    }

    /* Close handle to create file.
     */
    if(CloseHandle(hFile) == FALSE)
    {
        Fail("ERROR:%u:Failed to call CloseHandle API "
             "to close a file handle.",
             GetLastError());
    }

    PAL_Terminate();
    return PASS;
}

  
