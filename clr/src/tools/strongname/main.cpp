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
//

#include <utilcode.h>
#include <winwrap.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <strongname.h>
#include <cor.h>
#include <corimage.h>
#include <corver.h>
#include <__file__.ver>
#include <resources.h>
#include <palstartupw.h>

#if PLATFORM_UNIX
#define EOL     L"\n"
#else   // PLATFORM_UNIX
#define EOL     L"\r\n"
#endif  // PLATFORM_UNIX

bool g_bQuiet = false;


#define MESSAGE_DLL L"sn.satellite"

static HSATELLITE LoadSatelliteResource()
{
    WCHAR path[MAX_PATH];
    WCHAR *pEnd, *pSlashEnd;

    if (!GetModuleFileNameW(NULL, path, MAX_PATH))
        return 0;

    pEnd = wcsrchr(path, L'\\');
    pSlashEnd = wcsrchr(path, L'/');
    if (pSlashEnd > pEnd)
        pEnd = pSlashEnd;
        
    if (!pEnd)
        return 0;
    ++pEnd;  // point just beyond.

    // Append message DLL name if it fits.
    if ((int) sizeof(MESSAGE_DLL) + (pEnd - path) * sizeof(WCHAR) > (int) sizeof(path) - 1)
        return 0;
    wcscpy(pEnd, MESSAGE_DLL);

    return PAL_LoadSatelliteResourceW(path);
}

static HSATELLITE g_hSatellite = NULL;

static UINT LoadResourceString(UINT uID, LPWSTR lpBuffer, UINT nBufferMax)
{
    if (g_hSatellite == NULL)
    {
        g_hSatellite = LoadSatelliteResource();
        if (!g_hSatellite) return NULL;
    }

    return PAL_LoadSatelliteStringW(g_hSatellite, uID, lpBuffer, nBufferMax);
}



// Various routines for formatting and writing messages to the console.
void OutputList(LPCWSTR szFormat, va_list pArgs)
{
    DWORD   dwLength;
    LPSTR   szMessage;
    DWORD   dwWritten;

        WCHAR  szBuffer[8192];
        if (_vsnwprintf(szBuffer, sizeof(szBuffer) / sizeof(WCHAR), szFormat, pArgs) == -1) {
            WCHAR   szWarning[256];
            if (LoadResourceString(SN_TRUNCATED_OUTPUT, szWarning, sizeof(szWarning) / sizeof(WCHAR)))
                wcscpy(&szBuffer[(sizeof(szBuffer) / sizeof(WCHAR)) - wcslen(szWarning) - 1], szWarning);
        }
        szBuffer[(sizeof(szBuffer) / sizeof(WCHAR)) - 1] = L'\0';

        dwLength = (wcslen(szBuffer) + 1) * 3;
        szMessage = (LPSTR)_alloca(dwLength);
        WszWideCharToMultiByte(GetConsoleOutputCP(), 0, szBuffer, -1, szMessage, dwLength - 1, NULL, NULL);

    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), szMessage, strlen(szMessage), &dwWritten, NULL);
}

void OutputString(LPCWSTR szFormat, ...)
{
    va_list pArgs;

    va_start(pArgs, szFormat);
    OutputList(szFormat, pArgs);
    va_end(pArgs);
}

void Output(LPCWSTR szFormat, ...)
{
    va_list pArgs;

    va_start(pArgs, szFormat);
    OutputList(szFormat, pArgs);
    OutputList(EOL, pArgs);
    va_end(pArgs);
}

void Output(DWORD dwResId, ...)
{
    va_list pArgs;
    WCHAR   szFormat[1024];

    if (LoadResourceString(dwResId, szFormat, sizeof(szFormat)/sizeof(WCHAR))) {
        va_start(pArgs, dwResId);
        OutputList(szFormat, pArgs);
        OutputList(EOL, pArgs);
        va_end(pArgs);
    }
}


// Get the text for a given error code (inserts not supported). Note that the
// returned string is static (not need to deallocate, but calling this routine a
// second time will obliterate the result of the first call).
LPWSTR GetErrorString(ULONG ulError)
{
    static WCHAR szOutput[1024];

    if (!WszFormatMessage(
                          FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          ulError,
                          0,
                          szOutput,
                          sizeof(szOutput) / sizeof(WCHAR),
                          NULL)) {
        if (!LoadResourceString(SN_NO_ERROR_MESSAGE, szOutput, sizeof(szOutput) / sizeof(WCHAR)))
            wcscpy(szOutput, L"Unable to format error message ");

        // Tack on the error number in hex. Build this in 8-bit (because
        // wsprintf doesn't work on Win9X) and explode it out into 16-bit as a
        // post processing step.
        WCHAR szErrorNum[9];
        sprintf((char*)szErrorNum, "%08X", ulError);
        for (int i = 7; i >= 0; i--)
            szErrorNum[i] = (WCHAR)((char*)szErrorNum)[i];
        szErrorNum[8] = L'\0';

        wcscat(szOutput, szErrorNum);
    }

    return szOutput;
}


void Title()
{
    OutputString(EOL);
    Output(SN_TITLE, VER_FILEVERSION_WSTR);
    Output(VER_LEGALCOPYRIGHT_DOS_STR);
    OutputString(EOL);
}


void Usage()
{
    Output(SN_USAGE);
    Output(SN_OPTIONS);
    Output(SN_OPT_UD_1);
    Output(SN_OPT_UD_2);
    Output(SN_OPT_E_1);
    Output(SN_OPT_E_2);
    Output(SN_OPT_K_1);
    Output(SN_OPT_K_2);
    Output(SN_OPT_O_1);
    Output(SN_OPT_O_2);
    Output(SN_OPT_O_3);
    Output(SN_OPT_O_4);
    Output(SN_OPT_P_1);
    Output(SN_OPT_P_2);
    Output(SN_OPT_Q_1);
    Output(SN_OPT_Q_2);
    Output(SN_OPT_Q_3);
    Output(SN_OPT_UR_1);
    Output(SN_OPT_UR_2);
    Output(SN_OPT_TP_1);
    Output(SN_OPT_TP_2);
    Output(SN_OPT_TP_3);
    Output(SN_OPT_UTP_1);
    Output(SN_OPT_UTP_2);
    Output(SN_OPT_UTP_3);
    Output(SN_OPT_VF_1);
    Output(SN_OPT_VF_2);
    Output(SN_OPT_VF_3);
    Output(SN_OPT_VL_1);
    Output(SN_OPT_VL_2);
    Output(SN_OPT_VR_1);
    Output(SN_OPT_VR_2);
    Output(SN_OPT_VR_3);
    Output(SN_OPT_VR_4);
    Output(SN_OPT_VR_5);
    Output(SN_OPT_VR_6);
    Output(SN_OPT_VU_1);
    Output(SN_OPT_VU_2);
    Output(SN_OPT_VU_3);
    Output(SN_OPT_VX_1);
    Output(SN_OPT_VX_2);
    Output(SN_OPT_H_1);
    Output(SN_OPT_H_2);
    Output(SN_OPT_H_3);
}


// Read the entire contents of a file into a buffer. This routine allocates the
// buffer (which should be freed with delete []).
DWORD ReadFileIntoBuffer(LPWSTR szFile, BYTE **ppbBuffer, DWORD *pcbBuffer)
{
    // Open the file.
    HANDLE hFile = WszCreateFile(szFile,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    // Determine file size and allocate an appropriate buffer.
    DWORD dwHigh;
    *pcbBuffer = GetFileSize(hFile, &dwHigh);
    if (dwHigh != 0) {
        CloseHandle(hFile);
        return E_FAIL;
    }
    *ppbBuffer = new BYTE[*pcbBuffer];
    if (!*ppbBuffer) {
        CloseHandle(hFile);
        return ERROR_OUTOFMEMORY;
    }

    // Read the file into the buffer.
    DWORD dwBytesRead;
    if (!ReadFile(hFile, *ppbBuffer, *pcbBuffer, &dwBytesRead, NULL)) {
        CloseHandle(hFile);
        return GetLastError();
    }

    CloseHandle(hFile);

    return ERROR_SUCCESS;
}


// Write the contents of a buffer into a file.
DWORD WriteFileFromBuffer(LPCWSTR szFile, BYTE *pbBuffer, DWORD cbBuffer)
{
    // Create the file (overwriting if necessary).
    HANDLE hFile = WszCreateFile(szFile,
                               GENERIC_WRITE,
                               0,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    // Write the buffer contents.
    DWORD dwBytesWritten;
    if (!WriteFile(hFile, pbBuffer, cbBuffer, &dwBytesWritten, NULL)) {
        CloseHandle(hFile);
        return GetLastError();
    }

    CloseHandle(hFile);

    return ERROR_SUCCESS;
}


// Generate a temporary key container name based on process ID.
LPWSTR GetKeyContainerName()
{
    return NULL;
}

// Helper to format an 8-bit integer as a two character hex string.
void PutHex(BYTE bValue, WCHAR *szString)
{
    static const WCHAR *szHexDigits = L"0123456789abcdef";
    szString[0] = szHexDigits[bValue >> 4];
    szString[1] = szHexDigits[bValue & 0xf];
}


// Generate a hex string for a public key token.
LPWSTR GetTokenString(BYTE *pbToken, DWORD cbToken)
{
    LPWSTR  szString;
    DWORD   i;

    szString = new WCHAR[(cbToken * 2) + 1];
    if (szString == NULL)
        return L"<out of memory>";

    for (i = 0; i < cbToken; i++)
        PutHex(pbToken[i], &szString[i * 2]);
    szString[cbToken * 2] = L'\0';

    return szString;
}


// Generate a hex string for a public key.
LPWSTR GetPublicKeyString(BYTE *pbKey, DWORD cbKey)
{
    LPWSTR  szString;
    DWORD   i, j, src, dst;

    szString = new WCHAR[(cbKey * 2) + (((cbKey + 38) / 39) * 2) + 1];
    if (szString == NULL)
        return L"<out of memory>";

    dst = 0;
    for (i = 0; i < (cbKey + 38) / 39; i++) {
        for (j = 0; j < 39; j++) {
            src = i * 39 + j;
            if (src == cbKey)
                break;
            PutHex(pbKey[src], &szString[dst]);
            dst += 2;
        }
        szString[dst++] = L'\r';
        szString[dst++] = L'\n';
    }
    szString[dst] = L'\0';

    return szString;
}


// Check that the given file represents a strongly named assembly.
bool IsStronglyNamedAssembly(LPWSTR szAssembly)
{
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    HANDLE                      hMap = NULL;
    BYTE                       *pbBase  = NULL;
    IMAGE_NT_HEADERS           *pNtHeaders;
    IMAGE_COR20_HEADER         *pCorHeader;
    bool                        bIsStrongNamedAssembly = false;

    // Open the file.
    hFile = WszCreateFile(szAssembly,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        0);
    if (hFile == INVALID_HANDLE_VALUE)
        goto Cleanup;

    // Create a file mapping.
    hMap = WszCreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMap == NULL)
        goto Cleanup;

    // Map the file into memory.
    pbBase = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (pbBase == NULL)
        goto Cleanup;

    // Locate the standard file headers.
    pNtHeaders = Cor_RtlImageNtHeader(pbBase);
    if (pNtHeaders == NULL)
        goto Cleanup;

    // See if we can find a COM+ header extension.
    pCorHeader = (IMAGE_COR20_HEADER*)Cor_RtlImageRvaToVa(pNtHeaders,
                                                   pbBase, 
                                                   VAL32(pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress),
                                                   NULL);
    if (pCorHeader == NULL)
        goto Cleanup;

    // Check that space has been allocated in the PE for a signature. For now
    // assume that the signature is generated from a 1024-bit RSA signing key.
    if ((VAL32(pCorHeader->StrongNameSignature.VirtualAddress) == 0) ||
        (VAL32(pCorHeader->StrongNameSignature.Size) != 128))
        goto Cleanup;

    bIsStrongNamedAssembly = true;

 Cleanup:

    // Cleanup all resources we used.
    if (pbBase)
        UnmapViewOfFile(pbBase);
    if (hMap)
        CloseHandle(hMap);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    if (!bIsStrongNamedAssembly)
        Output(SN_NOT_STRONG_NAMED, szAssembly);

    return bIsStrongNamedAssembly;
}


// Verify a strongly named assembly for self consistency.
bool VerifyAssembly(LPWSTR szAssembly, bool bForceVerify)
{
    if (!IsStronglyNamedAssembly(szAssembly))
        return false;

    if (!StrongNameSignatureVerificationEx(szAssembly, bForceVerify, NULL)) {
        Output(SN_FAILED_VERIFY, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    if (!g_bQuiet) Output(SN_ASSEMBLY_VALID, szAssembly);
    
    return true;
}


// Generate a random public/private key pair and write it to a file.
bool GenerateKeyPair(LPWSTR szKeyFile)
{
    BYTE   *pbKey;
    DWORD   cbKey;
    DWORD   dwError;


    // Write a new public/private key pair to memory.
    if (!StrongNameKeyGen(GetKeyContainerName(), 0, &pbKey, &cbKey)) {
        Output(SN_FAILED_KEYPAIR_GEN, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Persist the key pair to disk.
    if ((dwError = WriteFileFromBuffer(szKeyFile, pbKey, cbKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_CREATE, szKeyFile, GetErrorString(dwError));
        return false;
    }

    if (!g_bQuiet) Output(SN_KEYPAIR_WRITTEN, szKeyFile);
        
    return true;
}


// Extract the public portion of a public/private key pair.
bool ExtractPublicKey(LPWSTR szInFile, LPWSTR szOutFile)
{
    BYTE   *pbKey;
    DWORD   cbKey;
    BYTE   *pbPublicKey;
    DWORD   cbPublicKey;
    DWORD   dwError;


    // Read the public/private key pair into memory.
    if ((dwError = ReadFileIntoBuffer(szInFile, &pbKey, &cbKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_READ, szInFile, GetErrorString(dwError));
        return false;
    }

    // Extract the public portion into a buffer.
    if (!StrongNameGetPublicKey(GetKeyContainerName(), pbKey, cbKey, &pbPublicKey, &cbPublicKey)) {
        Output(SN_FAILED_EXTRACT, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Write the public portion to disk.
    if ((dwError = WriteFileFromBuffer(szOutFile, pbPublicKey, cbPublicKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_CREATE, szOutFile, GetErrorString(dwError));
        return false;
    }

    if (!g_bQuiet) Output(SN_PUBLICKEY_WRITTEN, szOutFile);
        
    return true;
}



// Display the token form of a public key read from a file.
bool DisplayTokenFromKey(LPWSTR szFile, BOOL bShowPublic)
{
    BYTE   *pbKey;
    DWORD   cbKey;
    BYTE   *pbToken;
    DWORD   cbToken;
    DWORD   dwError;

    // Read the public key from a file.
    if ((dwError = ReadFileIntoBuffer(szFile, &pbKey, &cbKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_READ, szFile, GetErrorString(dwError));
        return false;
    }

    // Convert the key into a token.
    if (!StrongNameTokenFromPublicKey(pbKey, cbKey, &pbToken, &cbToken)) {
        Output(SN_FAILED_CONVERT, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Display public key if requested.
    if (bShowPublic)
        Output(SN_PUBLICKEY, EOL, GetPublicKeyString(pbKey, cbKey));

    // And display it.
    Output(SN_PUBLICKEYTOKEN, GetTokenString(pbToken, cbToken));

    return true;
}


// Display the token form of a public key used to sign an assembly.
bool DisplayTokenFromAssembly(LPWSTR szAssembly, BOOL bShowPublic)
{
    BYTE   *pbToken;
    DWORD   cbToken;
    BYTE   *pbKey;
    DWORD   cbKey;

    if (!IsStronglyNamedAssembly(szAssembly))
        return false;

    // Read the token direct from the assembly.
    if (!StrongNameTokenFromAssemblyEx(szAssembly, &pbToken, &cbToken, &pbKey, &cbKey)) {
        Output(SN_FAILED_READ_TOKEN, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Display public key if requested.
    if (bShowPublic)
        Output(SN_PUBLICKEY, EOL, GetPublicKeyString(pbKey, cbKey));

    // And display it.
    Output(SN_PUBLICKEYTOKEN, GetTokenString(pbToken, cbToken));

    return true;
}


// Write a public key to a file as a comma separated value list.
bool WriteCSV(LPWSTR szInFile, LPWSTR szOutFile)
{
    BYTE   *pbKey;
    DWORD   cbKey;
    DWORD   dwError;
    BYTE   *pbBuffer;
    DWORD   cbBuffer;
    DWORD   i;

    // Read the public key from a file.
    if ((dwError = ReadFileIntoBuffer(szInFile, &pbKey, &cbKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_READ, szInFile, GetErrorString(dwError));
        return false;
    }

    // Check for non-empty file.
    if (cbKey == 0) {
        Output(SN_EMPTY, szInFile);
        return false;
    }

    // Calculate the size of the text output buffer:
    //    Each byte -> 3 chars (space prefixed decimal) + 2 (", ")
    //  + 2 chars ("\r\n")
    //  - 2 chars (final ", " not needed)
    cbBuffer = (cbKey * (3 + 2)) + 2 - 2;

    // Allocate buffer (plus an extra couple of characters for the temporary
    // slop-over from writing a trailing ", " we don't need).
    pbBuffer = new BYTE[cbBuffer + 2];
    if (pbBuffer == NULL) {
        Output(SN_FAILED_ALLOCATE);
        return false;
    }

    // Convert the byte stream into a CSV (Comma Seperated Value) list.
    for (i = 0; i < cbKey; i++)
        sprintf((char*)&pbBuffer[i * 5], "% 3u, ", pbKey[i]);
    pbBuffer[cbBuffer - 2] = '\r';
    pbBuffer[cbBuffer - 1] = '\n';

    // If an output filename was provided write the CSV list to it.
    if (szOutFile) {
        if ((dwError = WriteFileFromBuffer(szOutFile, pbBuffer, cbBuffer)) != ERROR_SUCCESS) {
            Output(SN_FAILED_CREATE, szOutFile, GetErrorString(dwError));
            return false;
        }
        if (!g_bQuiet) Output(SN_PUBLICKEY_WRITTEN_CSV, szOutFile);
    } else {

        DWORD dwBytesWritten;
        if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), pbBuffer, cbBuffer, &dwBytesWritten, NULL) != ERROR_SUCCESS) {
            return false;
        }

        if (!g_bQuiet) Output(SN_PUBLICKEY_WRITTEN_CSV, L"console");

    }

    return true;
}


// Extract the public key from an assembly and place it in a file.
bool ExtractPublicKeyFromAssembly(LPWSTR szAssembly, LPWSTR szFile)
{
    BYTE   *pbToken;
    DWORD   cbToken;
    BYTE   *pbKey;
    DWORD   cbKey;
    DWORD   dwError;

    if (!IsStronglyNamedAssembly(szAssembly))
        return false;

    // Read the public key from the assembly.
    if (!StrongNameTokenFromAssemblyEx(szAssembly, &pbToken, &cbToken, &pbKey, &cbKey)) {
        Output(SN_FAILED_READ_TOKEN, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // And write it to disk.
    if ((dwError = WriteFileFromBuffer(szFile, pbKey, cbKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_CREATE, szFile, GetErrorString(dwError));
        return false;
    }

    if (!g_bQuiet) Output(SN_PUBLICKEY_EXTRACTED, szFile);

    return true;
}


// Check that two assemblies differ only by their strong name signature.
bool DiffAssemblies(LPWSTR szAssembly1, LPWSTR szAssembly2)
{
    DWORD   dwResult;

    if (!IsStronglyNamedAssembly(szAssembly1))
        return false;

    if (!IsStronglyNamedAssembly(szAssembly2))
        return false;

    // Compare the assembly images.
    if (!StrongNameCompareAssemblies(szAssembly1, szAssembly2, &dwResult)) {
        Output(SN_FAILED_COMPARE, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Print a message describing how similar they are.
    if (!g_bQuiet)
        switch (dwResult) {
        case SN_CMP_DIFFERENT:
            Output(SN_DIFFER_MORE);
            break;
        case SN_CMP_IDENTICAL:
            Output(SN_DIFFER_NOT);
            break;
        case SN_CMP_SIGONLY:
            Output(SN_DIFFER_ONLY);
            break;
        default:
            Output(SN_INTERNAL_1, dwResult);
            return false;
        }

    // Return a failure code on differences.
    return dwResult != SN_CMP_DIFFERENT;
}


// Re-sign a previously signed assembly with a key pair from a file or a key
// container.
bool ResignAssembly(LPWSTR szAssembly, LPWSTR szFileOrContainer, bool bContainer)
{
    LPWSTR  szContainer = NULL;
    BYTE   *pbKey = NULL;
    DWORD   cbKey = NULL;
    DWORD   dwError;

    if (!IsStronglyNamedAssembly(szAssembly))
        return false;

    if (bContainer) {
        // Key is provided in container.
        szContainer = szFileOrContainer;
    } else {
        // Key is provided in file.

        // Read the public/private key pair into memory.
        if ((dwError = ReadFileIntoBuffer(szFileOrContainer, &pbKey, &cbKey)) != ERROR_SUCCESS) {
            Output(SN_FAILED_READ, szFileOrContainer, GetErrorString(dwError));
            return false;
        }
    }

    // Recompute the signature in the assembly file.
    if (!StrongNameSignatureGeneration(szAssembly, szContainer,
                                       pbKey, cbKey, NULL, NULL)) {
        Output(SN_FAILED_RESIGN, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    if (!g_bQuiet) Output(SN_ASSEMBLY_RESIGNED, szAssembly);

    return true;
}

// Build a name for an assembly that includes the internal name and a hex
// representation of the strong name (public key).
LPWSTR GetAssemblyName(LPWSTR szAssembly)
{
    HRESULT                     hr;
    IMetaDataDispenser         *pDisp;
    IMetaDataAssemblyImport    *pAsmImport;
    mdAssembly                  tkAssembly;
    BYTE                       *pbKey;
    DWORD                       cbKey;
    static WCHAR                szAssemblyName[1024];
    WCHAR                       szStrongName[1024];
    BYTE                       *pbToken;
    DWORD                       cbToken;
    DWORD                       i;

    if (FAILED(hr = PAL_CoCreateInstance(CLSID_CorMetaDataDispenser,
                                     IID_IMetaDataDispenser,
                                     (void**)&pDisp))) {
        Output(SN_FAILED_MD_ACCESS, GetErrorString(hr));
        return NULL;
    }

    // Open a scope on the file.
    if (FAILED(hr = pDisp->OpenScope(szAssembly,
                                     0,
                                     IID_IMetaDataAssemblyImport,
                                     (IUnknown**)&pAsmImport))) {
        Output(SN_FAILED_MD_OPEN, szAssembly, GetErrorString(hr));
        return NULL;
    }

    // Determine the assemblydef token.
    if (FAILED(hr = pAsmImport->GetAssemblyFromScope(&tkAssembly))) {
        Output(SN_FAILED_MD_ASSEMBLY, szAssembly, GetErrorString(hr));
        return NULL;
    }

    // Read the assemblydef properties to get the public key and name.
    if (FAILED(hr = pAsmImport->GetAssemblyProps(tkAssembly,
                                                 (const void **)&pbKey,
                                                 &cbKey,
                                                 NULL,
                                                 szAssemblyName,
                                                 sizeof(szAssemblyName) / sizeof(WCHAR),
                                                 NULL,
                                                 NULL,
                                                 NULL))) {
        Output(SN_FAILED_STRONGNAME, szAssembly, GetErrorString(hr));
        return NULL;
    }

    // Check for strong name.
    if ((pbKey == NULL) || (cbKey == 0)) {
        Output(SN_NOT_STRONG_NAMED, szAssembly);
        return NULL;
    }

    // Compress the strong name down to a token.
    if (!StrongNameTokenFromPublicKey(pbKey, cbKey, &pbToken, &cbToken)) {
        Output(SN_FAILED_CONVERT, GetErrorString(StrongNameErrorInfo()));
        return NULL;
    }

    // Turn the token into hex.
    for (i = 0; i < cbToken; i++)
        swprintf(&szStrongName[i * 2], L"%02X", pbToken[i]);

    // Build the name (in a static buffer).
    wcscat(szAssemblyName, L",");
    wcscat(szAssemblyName, szStrongName);

    StrongNameFreeBuffer(pbToken);
    pAsmImport->Release();
    pDisp->Release();


    return szAssemblyName;
}


BOOL CheckStrongName(WCHAR* pwz, DWORD length)
{
    while (length--)
    {
        WCHAR ch = *pwz++;
        if (((ch < '0') || (ch > '9')) &&
            ((ch < 'A') || (ch > 'F')) &&
            ((ch < 'a') || (ch > 'f')))
            return FALSE;
    }

    return TRUE;
}

// Parse an assembly an assembly name handed to a register/unregister
// verification skipping function. The input name can be "*" for all assemblies,
// "*,<hex digits>" for all assemblies with a given strong name or the filename
// of a specific assembly. The output is a string of the form:
// "<simple name>,<hex strong name>" where the first or both fields can be
// wildcarded with "*", and the hex strong name is a hexidecimal representation
// of the public key token (as we expect in the "*,<hex digits>" input form).
LPWSTR ParseAssemblyName(LPWSTR szAssembly)
{
    if ((wcscmp(L"*", szAssembly) == 0) ||
        (wcscmp(L"*,*", szAssembly) == 0))
        return L"*,*";
    else if (wcsncmp(L"*,", szAssembly, 2) == 0) {
        DWORD i = wcslen(szAssembly) - 2;
        if ((i == 0) || (i & 1) || !CheckStrongName(&szAssembly[2], i)) {
            Output(SN_INVALID_SVR_FORMAT);
            return NULL;
        }
        return szAssembly;
    } else
        return GetAssemblyName(szAssembly);
}


#define CONFIG_FILE_NAME   L"strongname.ini"
#define DIRECTORYSEPERATOR L"\\"
#define DIRECTORYSEPERATORLENGTH 1

static BOOL GetStrongNameConfigPath(WCHAR* lpFileName, DWORD cbFileName)
{
    if (!PAL_GetMachineConfigurationDirectoryW(lpFileName, cbFileName))
        return FALSE;

        if (wcslen(lpFileName) + sizeof(CONFIG_FILE_NAME)/sizeof(WCHAR) + DIRECTORYSEPERATORLENGTH >= 
        cbFileName) {
        return FALSE;
    }

    wcscat(lpFileName, DIRECTORYSEPERATOR);
    wcscat(lpFileName, CONFIG_FILE_NAME);

    return TRUE;
}

static DWORD PAL_IniReadStringEx(HINI hIni, 
             LPCWSTR pszSection, 
             LPCWSTR pszKey, 
             LPWSTR* ppszValue)
{
    DWORD       dwRet;
    LPWSTR      pwzBuf = NULL;
    DWORD       dwSize = 128;

    for (;;) {
        pwzBuf = new WCHAR[dwSize];
        if (!pwzBuf) {
            *ppszValue = NULL;
            return 0;
        }
    
        dwRet = PAL_IniReadString(hIni, pszSection, pszKey, 
                                        pwzBuf, dwSize);
        if ( (pszSection && pszKey && dwRet == dwSize - 1) ||
            ((!pszSection || !pszKey) && dwRet == dwSize - 2) )
        {
            delete [] pwzBuf;
            dwSize *= 2;
            continue;
        }

        if (dwRet == 0)
        {
            delete [] pwzBuf;
            *ppszValue = NULL;
            return 0;
        }

        break;
    }

    *ppszValue = pwzBuf;
    return dwRet;
}

// Register an assembly for verification skipping.
bool VerifyRegister(LPWSTR szAssembly, LPWSTR szUserlist)
{
    LPWSTR      szAssemblyName;
    WCHAR       szSubKey[MAX_PATH + 1];
    WCHAR       wzConfigPath[MAX_PATH+1];
    HINI        hIni = NULL;
    WCHAR      *pEntries = NULL;
    WCHAR      *p;
    int         c = 1;
    WCHAR       szKey[20];

    // Get the internal name for the assembly (possibly containing wildcards).
    szAssemblyName = ParseAssemblyName(szAssembly);
    if (szAssemblyName == NULL)
        return false;

    if (!GetStrongNameConfigPath(wzConfigPath, MAX_PATH+1))
        goto LError;

    hIni = PAL_IniCreate();
    if (!hIni)
        goto LError;

    PAL_IniLoad(hIni, wzConfigPath);

    // Blow away any old user list.

    if (PAL_IniReadStringEx(hIni, SN_CONFIG_VERIFICATION_W, NULL, &pEntries))
    {
        p = pEntries;

        while (*p != '\0')
        {
            if (PAL_IniReadString(hIni, SN_CONFIG_VERIFICATION_W, p, szSubKey, MAX_PATH + 1))
            {
                if (wcscmp(szSubKey, szAssemblyName) == 0) {
                    PAL_IniWriteString(hIni, SN_CONFIG_VERIFICATION_W, p, NULL);
                    PAL_IniWriteString(hIni, SN_CONFIG_USERLIST_W, p, NULL);
                }
            }

            p += wcslen(p) + 1;
        }
    }

    // find a unused key

    for (;;)
    {
        swprintf(szKey, L"_%d", c);
        if (PAL_IniReadString(hIni, SN_CONFIG_VERIFICATION_W, szKey, szSubKey, MAX_PATH + 1) == 0)
            break;
        c++;
    }

    PAL_IniWriteString(hIni, SN_CONFIG_VERIFICATION_W, szKey, szAssemblyName);
    if (szUserlist && (wcscmp(L"*", szUserlist) != 0)) {
        PAL_IniWriteString(hIni, SN_CONFIG_USERLIST_W, szKey, szUserlist);
    }
    else {
        PAL_IniWriteString(hIni, SN_CONFIG_USERLIST_W, szKey, NULL);
    }

    if (!PAL_IniSave(hIni, wzConfigPath, TRUE)) {
        Output(SN_FAILED_REG_WRITE, GetErrorString(GetLastError()));
        goto LError;
    }

    PAL_IniClose(hIni);
    delete [] pEntries;

    if (!g_bQuiet) Output(SN_SVR_ADDED, szAssemblyName);

    return true;

LError:
    PAL_IniClose(hIni);
    delete [] pEntries;
    return false;
}


// Unregister an assembly for verification skipping.
bool VerifyUnregister(LPWSTR szAssembly)
{
    LPWSTR      szAssemblyName;
    WCHAR       szSubKey[MAX_PATH + 1];
    WCHAR       wzConfigPath[MAX_PATH+1];
    HINI        hIni = NULL;
    WCHAR      *pEntries = NULL;
    WCHAR      *p;

    // Get the internal name for the assembly (possibly containing wildcards).
    szAssemblyName = ParseAssemblyName(szAssembly);
    if (szAssemblyName == NULL)
        return false;

    if (!GetStrongNameConfigPath(wzConfigPath, MAX_PATH+1))
        goto LError;

    hIni = PAL_IniCreate();
    if (!hIni)
        goto LError;

    PAL_IniLoad(hIni, wzConfigPath);

    if (PAL_IniReadStringEx(hIni, SN_CONFIG_VERIFICATION_W, NULL, &pEntries))
    {
        p = pEntries;

        while (*p != '\0')
        {
            if (PAL_IniReadString(hIni, SN_CONFIG_VERIFICATION_W, p, szSubKey, MAX_PATH + 1))
            {
                if (wcscmp(szSubKey, szAssemblyName) == 0) {
                    PAL_IniWriteString(hIni, SN_CONFIG_VERIFICATION_W, p, NULL);
                    PAL_IniWriteString(hIni, SN_CONFIG_USERLIST_W, p, NULL);
                }
            }

            p += wcslen(p) + 1;
        }
    }

    if (!PAL_IniSave(hIni, wzConfigPath, TRUE)) {
        Output(SN_FAILED_REG_WRITE, GetErrorString(GetLastError()));
        goto LError;
    }

    PAL_IniClose(hIni);
    delete [] pEntries;

    if (!g_bQuiet) Output(SN_SVR_REMOVED, szAssemblyName);

    return true;

LError:
    if (hIni != NULL)
        PAL_IniClose(hIni);
    delete [] pEntries;
    return false;
}

// Unregister all verification skipping entries.
bool VerifyUnregisterAll()
{
    WCHAR   wzConfigPath[MAX_PATH+1];
    HINI    hIni = NULL;

    if (!GetStrongNameConfigPath(wzConfigPath, MAX_PATH+1))
        goto LError;

    hIni = PAL_IniCreate();
    if (!hIni)
        goto LError;

    PAL_IniLoad(hIni, wzConfigPath);

    if (!PAL_IniWriteString(hIni, SN_CONFIG_VERIFICATION_W, NULL, NULL))
        goto LError;

    if (!PAL_IniWriteString(hIni, SN_CONFIG_USERLIST_W, NULL, NULL))
        goto LError;

    if (!PAL_IniSave(hIni, wzConfigPath, TRUE)) {
        Output(SN_FAILED_REG_WRITE, GetErrorString(GetLastError()));
        goto LError;
    }

    PAL_IniClose(hIni);

    if (!g_bQuiet) Output(SN_SVR_ALL_REMOVED);

    return true;

LError:
    if (hIni != NULL)
        PAL_IniClose(hIni);
    return false;
}

// List state of verification on this machine.
bool VerifyList()
{
    DWORD           j;
    DWORD           dwEntries;
    WCHAR           szSubKey[MAX_PATH + 1];
    WCHAR          *mszUserList;
    WCHAR          *szUser;
    WCHAR           wzConfigPath[MAX_PATH+1];
    HINI            hIni = NULL;
    WCHAR          *pEntries = NULL;
    WCHAR          *p;
    DWORD           cchPad;
    LPWSTR          szPad;

    // Count entries we find.
    dwEntries = 0;

    if (!GetStrongNameConfigPath(wzConfigPath, MAX_PATH+1))
        goto LError;

    hIni = PAL_IniCreate();
    if (!hIni)
        goto LError;

    PAL_IniLoad(hIni, wzConfigPath);

    if (!PAL_IniReadStringEx(hIni, SN_CONFIG_VERIFICATION_W, NULL, &pEntries))
        goto Finished;

    p = pEntries;

    while (*p != '\0')
    {
        if (PAL_IniReadString(hIni, SN_CONFIG_VERIFICATION_W, p, szSubKey, MAX_PATH + 1))
        {
            dwEntries++;

            if (dwEntries == 1) {
                Output(SN_SVR_TITLE_1);
                Output(SN_SVR_TITLE_2);
            }

            if (wcslen(szSubKey) < 38) {
                cchPad = 38 - wcslen(szSubKey);
                szPad = (LPWSTR)_alloca((cchPad + 1) * sizeof(WCHAR));
                memset(szPad, 0, (cchPad + 1) * sizeof(WCHAR));
                for (j = 0; j < cchPad; j++)
                    szPad[j] = L' ';
                OutputString(L"%s%s", szSubKey, szPad);
            } else
                OutputString(L"%s ", szSubKey);

            // Read a list of valid users, if supplied.
            mszUserList = NULL;
            if (PAL_IniReadStringEx(hIni, SN_CONFIG_USERLIST_W, p, &mszUserList)) {

                szUser = mszUserList;
                while (szUser) {
                    WCHAR* szEnd = wcschr(szUser, ',');
                    if (szEnd) 
                        *szEnd = '\0';
                    OutputString(L"%s ", szUser);
                    szUser = szEnd ? (szEnd + 1) : NULL;
                }
                OutputString(EOL);

                delete [] mszUserList;
            } else
                Output(SN_ALL_USERS);
        }

        p += wcslen(p) + 1;
    }

Finished:
    PAL_IniClose(hIni);
    delete [] pEntries;

    if (!g_bQuiet && (dwEntries == 0))
        Output(SN_NO_SVR);

    return true;

LError:
    if (hIni != NULL)
        PAL_IniClose(hIni);
    delete [] pEntries;
    return false;
}


// Check that a given command line option has been given the right number of arguments.
#define OPT_CHECK(_opt, _min, _max) do {                                                                \
    if (wcscmp(L##_opt, &argv[1][1])) {                                                                 \
        Output(SN_INVALID_OPTION, argv[1]);                                                             \
        Usage();                                                                                        \
        return 1;                                                                                       \
    } else if ((argc > ((_max) + 2)) && (argv[2 + (_max)][0] == '-' || argv[2 + (_max)][0] == '/')) {   \
        Output(SN_OPT_ONLY_ONE);                                                                        \
        return 1;                                                                                       \
    } else if ((argc < ((_min) + 2)) || (argc > ((_max) + 2))) {                                        \
        if ((_min) == (_max)) {                                                                         \
            if ((_min) == 0)                                                                            \
                Output(SN_OPT_NO_ARGS, (L##_opt));                                                      \
            else if ((_min) == 1)                                                                       \
                Output(SN_OPT_ONE_ARG, (L##_opt));                                                      \
            else                                                                                        \
                Output(SN_OPT_N_ARGS, (L##_opt), (_min));                                               \
        } else                                                                                          \
            Output(SN_OPT_ARG_RANGE, (L##_opt), (_min), (_max));                                        \
        Usage();                                                                                        \
        return 1;                                                                                       \
    }                                                                                                   \
} while (0)


extern "C" int _cdecl wmain(int argc, WCHAR **argv)
{
    bool bResult;

    // Initialize Wsz wrappers.
    OnUnicodeSystem();

    if (!PAL_RegisterLibrary(L"rotor_palrt") ||
        !PAL_RegisterLibrary(L"sscoree"))
    {
        fprintf(stderr, "Unable to register libraries\n");
        return 1;
    }
    
    // Check for quiet mode.
    if ((argc > 1) &&
        ((argv[1][0] == L'-') || (argv[1][0] == L'/')) &&
        (argv[1][1] == L'q')) {
        g_bQuiet = true;
        argc--;
        argv = &argv[1];
    }

    if (!g_bQuiet)
        Title();

    if ((argc < 2) || ((argv[1][0] != L'-') && (argv[1][0] != L'/'))) {
        Usage();
        return 0;
    }

    switch (argv[1][1]) {
    case 'v':
        if (argv[1][2] == L'f') {
            OPT_CHECK("vf", 1, 1);
            bResult = VerifyAssembly(argv[2], true);
        } else {
            OPT_CHECK("v", 1, 1);
            bResult = VerifyAssembly(argv[2], false);
        }
        break;
    case 'k':
        OPT_CHECK("k", 1, 1);
        bResult = GenerateKeyPair(argv[2]);
        break;
    case 'p':
        {
            OPT_CHECK("p", 2, 2);
            bResult = ExtractPublicKey(argv[2], argv[3]);
        }
        break;
    case 'V':
        switch (argv[1][2]) {
        case 'l':
            OPT_CHECK("Vl", 0, 0);
            bResult = VerifyList();
            break;
        case 'r':
            OPT_CHECK("Vr", 1, 2);
            bResult = VerifyRegister(argv[2], argc > 3 ? argv[3] : NULL);
            break;
        case 'u':
            OPT_CHECK("Vu", 1, 1);
            bResult = VerifyUnregister(argv[2]);
            break;
        case 'x':
            OPT_CHECK("Vx", 0, 0);
            bResult = VerifyUnregisterAll();
            break;
        default:
            Output(SN_INVALID_V_OPT, argv[1]);
            Usage();
            return 1;
        }
        break;
    case 't':
        if (argv[1][2] == L'p') {
            OPT_CHECK("tp", 1, 1);
            bResult = DisplayTokenFromKey(argv[2], true);
        } else {
            OPT_CHECK("t", 1, 1);
            bResult = DisplayTokenFromKey(argv[2], false);
        }
        break;
    case 'T':
        if (argv[1][2] == L'p') {
            OPT_CHECK("Tp", 1, 1);
            bResult = DisplayTokenFromAssembly(argv[2], true);
        } else {
            OPT_CHECK("T", 1, 1);
            bResult = DisplayTokenFromAssembly(argv[2], false);
        }
        break;
    case 'e':
        OPT_CHECK("e", 2, 2);
        bResult = ExtractPublicKeyFromAssembly(argv[2], argv[3]);
        break;
    case 'o':
        OPT_CHECK("o", 1, 2);
        bResult = WriteCSV(argv[2], argc > 3 ? argv[3] : NULL);
        break;
    case 'D':
        OPT_CHECK("D", 2, 2);
        bResult = DiffAssemblies(argv[2], argv[3]);
        break;
    case 'R':
        {
            OPT_CHECK("R", 2, 2);
            bResult = ResignAssembly(argv[2], argv[3], false);
        }
        break;
    case '?':
    case 'h':
    case 'H':
        Usage();
        bResult = true;
        break;
    default:
        Output(SN_INVALID_OPTION, &argv[1][1]);
        bResult = false;
    }

    return bResult ? 0 : 1;
}
