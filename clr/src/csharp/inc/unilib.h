// ==++==
//
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
//
// ==--==
// ===========================================================================
// File: unilib.h
//
// ===========================================================================

#ifndef __UNILIB_H__
#define __UNILIB_H__

// Just delegate to uniprop.h after making sure rotor_palrt.h is loaded
#include "rotor_palrt.h"
#include "uniprop.h"
#include "utf.h"


#define W_CreateFile CreateFileW
#define W_CopyFile CopyFileW
#define W_GetTempPath GetTempPathW
#define W_GetTempFileName GetTempFileNameW
#define W_DeleteFile DeleteFileW
#define W_GetFullPathName GetFullPathNameW
#define W_GetActualFileCase GetActualFileCaseW
#define W_GetCurrentDirectory GetCurrentDirectoryW
#define W_GetFileAttributes GetFileAttributesW
#define W_FindFirstFile FindFirstFileW
#define W_FindNextFile FindNextFileW

#ifdef __cplusplus
extern "C" {
#endif

// Rotor assumes a Unicode system
inline BOOL W_IsUnicodeSystem()	{ return TRUE; }

// Note this doesn't set the errno if there's an error but no one seemed to be using it.
int WINAPI W_Access(PCWSTR pPathName, int mode);

void WINAPI W_GetActualFileCase(PCWSTR pszName, PWSTR psz);
PWSTR WINAPI ToLowerCase (PCWSTR pSrc, PWSTR pDst, size_t cch);

#ifdef __cplusplus
}
#endif

#define CP_UNICODE          1200 // Unicode
#define CP_UNICODESWAP      1201 // Unicode Big-Endian

#endif // __UNILIB_H__
