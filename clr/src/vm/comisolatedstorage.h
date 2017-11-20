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
/*============================================================
 *
 * Class: COMIsolatedStorage
 *       
 *
 * Purpose: Implementation of IsolatedStorage
 *
 * Date:  Feb 14, 2000
 *
 ===========================================================*/

#ifndef __COMISOLATEDSTORAGE_h__
#define __COMISOLATEDSTORAGE_h__

// Dependency in managed : System.IO.IsolatedStorage.IsolatedStorage.cs
#define ISS_ROAMING_STORE   0x08
#define ISS_MACHINE_STORE   0x10

class COMIsolatedStorage
{
public:
    static FCDECL0(Object*, GetCaller);
    static void ThrowISS(HRESULT hr);

private:

    static StackWalkAction StackWalkCallBack(CrawlFrame* pCf, PVOID ppv);
};

class COMIsolatedStorageFile
{
public:

	//typedef struct {
	//    DECLARE_ECALL_I4_ARG(DWORD, dwFlags);
	//} _GetRootDir;
    static FCDECL1(Object*, GetRootDir, DWORD dwFlags);

	//typedef struct {
	//    DECLARE_ECALL_PTR_ARG(LPVOID, handle);
	//} _GetUsage;
    static FCDECL1(UINT64, GetUsage, LPVOID handle);

	//typedef struct {
	//    DECLARE_ECALL_I1_ARG(bool,      fFree);
	//    DECLARE_ECALL_PTR_ARG(UINT64*, pqwReserve);
	//    DECLARE_ECALL_PTR_ARG(UINT64*, pqwQuota);
	//    DECLARE_ECALL_PTR_ARG(LPVOID,   handle);
	//} _Reserve;
    static FCDECL4(void, Reserve, LPVOID handle, UINT64* pqwQuota, UINT64* pqwReserve, bool fFree);

	//typedef struct {
	//    DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, fileName);
	//} _Open;
    static FCDECL1(LPVOID, Open, StringObject* fileNameUNSAFE);

	//typedef struct {
	//    DECLARE_ECALL_PTR_ARG(LPVOID, handle);
	//} _Close;
    static FCDECL1(void, Close, LPVOID handle);

private:

    static void GetRootDirInternal(DWORD dwFlags,WCHAR *path, DWORD cPath);
    static void CreateDirectoryIfNotPresent(WCHAR *path);
};

// --- [ Structure of data that gets persisted on disk ] -------------(Begin)

// non-standard extension: 0-length arrays in struct
#ifdef _MSC_VER
#pragma warning(disable:4200)
#endif
#include <pshpack1.h>

typedef unsigned __int64 QWORD;

typedef QWORD ISS_USAGE;

// Accounting Information
typedef struct
{
    ISS_USAGE   cUsage;           // The amount of resource used

    QWORD       qwReserved[7];    // For future use, set to 0

} ISS_RECORD;

#include <poppack.h>
#ifdef _MSC_VER
#pragma warning(default:4200)
#endif

// --- [ Structure of data that gets persisted on disk ] ---------------(End)

class AccountingInfo
{
public:

    // The file name is used to open / create the file.
    // A synchronization object will also be created using this name
    // with '\' replaced by '-'

    AccountingInfo(WCHAR *wszFileName);

    // Init should be called before Reserve / GetUsage is called.

    HRESULT Init();             // Creates the file if necessary

    // Reserves space (Increments qwQuota)
    // This method is synchrinized. If quota + request > limit, method fails

    HRESULT Reserve(
        ISS_USAGE   cLimit,     // The max allowed
        ISS_USAGE   cRequest,   // amount of space (request / free)
        BOOL        fFree);     // TRUE will free, FALSE will reserve

    // Method is not synchronized. So the information may not be current.
    // This implies "Pass if (Request + GetUsage() < Limit)" is an Error!
    // Use Reserve() method instead.

    HRESULT GetUsage(
        ISS_USAGE   *pcUsage);  // [out] The amount of space / resource used

    // Frees cached pointers, Closes handles

    ~AccountingInfo();          

private:

    HRESULT Map();      // Maps the store file into memory
    void    Unmap();    // Unmaps the store file from memory
    void    Close();    // Close the store file, and file mapping
    HRESULT Lock();     // Machine wide Lock
    void    Unlock();   // Unlock the store

private:

    WCHAR          *m_wszFileName;  // The file name
    HANDLE          m_hFile;        // File handle for the file
    HANDLE          m_hMapping;     // File mapping for the memory mapped file

    // members used for synchronization 
    WCHAR          *m_wszName;      // The name of the mutex object
    HANDLE          m_hLock;        // Handle to the Mutex object
#ifdef _DEBUG
    ULONG           m_dwNumLocks;   // The number of locks owned by this object
#endif

    union {
        PBYTE       m_pData;        // The start of file stream
        ISS_RECORD *m_pISSRecord;
    };
};

#endif  // __COMISOLATEDSTORAGE_h__
