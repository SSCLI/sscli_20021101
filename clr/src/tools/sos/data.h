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
#ifndef __data_h__
#define __data_h__

#include <palclr.h>
#include "../../inc/cor.h"

BOOL FileExist (const char *filename);
BOOL FileExist (const WCHAR *filename);

enum JitType {UNKNOWN=0, JIT, EJIT, PJIT};

// We use global variables
// because move returns void if it fails
//typedef DWORD DWORD_PTR;
//typedef ULONG ULONG_PTR;

// Max length in WCHAR for a buffer to store metadata name
const int mdNameLen = 2048;
extern WCHAR g_mdName[mdNameLen];

const int nMDIMPORT = 128;
struct MDIMPORT
{
    WCHAR *name;
    IMetaDataImport *pImport;
    MDIMPORT *left;
    MDIMPORT *right;
};

class MDImportSet
{
    MDIMPORT *root;
public:
    MDImportSet()
        :root(NULL)
    {
    }
    ~MDImportSet()
    {
    }
    void Destroy();
    IMetaDataImport *GetImport(WCHAR *moduleName);
private:
    void DestroyInternal(MDIMPORT *node);
};

#ifdef _X86_

struct CodeInfo
{
    JitType jitType;
    DWORD_PTR IPBegin;
    unsigned methodSize;
    DWORD_PTR gcinfoAddr;
    unsigned char prologSize;
    unsigned char epilogStart;
    unsigned char epilogCount:3;
    unsigned char epilogAtEnd:1;
    unsigned char ediSaved   :1;
    unsigned char esiSaved   :1;
    unsigned char ebxSaved   :1;
    unsigned char ebpSaved   :1;
    unsigned char ebpFrame;
    unsigned short argCount;
};

#elif defined(_PPC_)

struct CodeInfo {
    JitType jitType;
    DWORD_PTR IPBegin;
    unsigned methodSize;
    DWORD_PTR gcinfoAddr;
    unsigned char prologSize;
    unsigned char epilogStart;
    unsigned char epilogCount:3;
    unsigned char epilogAtEnd:1;
    unsigned short argCount;
};

#else

PORTABILITY_WARNING("CodeInfo is not defined on this platform.")

struct CodeInfo {
    JitType jitType;
    DWORD_PTR IPBegin;
    unsigned methodSize;
    DWORD_PTR gcinfoAddr;
    unsigned char prologSize;
    unsigned char epilogStart;
    unsigned char epilogCount:3;
    unsigned char epilogAtEnd:1;
    // Unknown other fields
    unsigned short argCount;
};

#endif

extern MDImportSet mdImportSet;

extern DWORD_PTR EEManager;
extern BOOL ControlC;
extern IMetaDataDispenserEx *pDisp;

const int NumEEDllPath=8;
class EEDllPath
{
    WCHAR path[NumEEDllPath][MAX_PATH];
    EEDllPath *next;
public:
    EEDllPath ()
    {
        next = NULL;
        for (int i = 0; i < NumEEDllPath; i ++) {
            path[i][0] = L'\0';
        }
    }
    ~EEDllPath ()
    {
        if (next) {
            delete next;
        }
    }
    void AddPath (const WCHAR* str)
    {
        EEDllPath *ptr = this;
        while (ptr->next != NULL) {
            ptr = ptr->next;
        }
        for (int i = 0; i < NumEEDllPath; i ++) {
            if (ptr->path[i][0] == L'\0') {
                wcscpy (ptr->path[i], str);
                return;
            }
            else if (_wcsicmp (ptr->path[i],str) == 0) {
                return;
            }
        }
        if (ptr->next == NULL) {
            ptr->next = new EEDllPath;
        }
        ptr = ptr->next;
        wcscpy (ptr->path[0], str);
    }

    void AddPath (const char* str)
    {
		WCHAR path[MAX_PATH+1];
		MultiByteToWideChar (CP_ACP,0,str,-1,path,MAX_PATH);
		AddPath (path);
    }

    const WCHAR* PathToDll (const WCHAR* str)
    {
        EEDllPath *ptr = this;
        WCHAR filename[MAX_PATH+1];
        while (ptr) {
            for (int i = 0; i < NumEEDllPath; i ++) {
                if (ptr->path[i][0] == L'\0') {
                    return NULL;
                }
                wcscpy (filename, ptr->path[i]);
                wcscat (filename, L"\\");
                size_t n = wcslen (filename);
                wcsncat (filename, str, MAX_PATH-n);
                filename[MAX_PATH] = L'\0';
                if (FileExist (filename)) {
                    return ptr->path[i];
                }
            }
            ptr = ptr->next;
        }
        return NULL;
    }

    void DisplayPath ();

    void Reset ()
    {
        EEDllPath *ptr = this;
        while (ptr) {
            for (int i = 0; i < NumEEDllPath; i ++) {
                ptr->path[i][0] = L'\0';
            }
            ptr = ptr->next;
        }
    }
};

extern EEDllPath *DllPath;
#endif // __data_h__
