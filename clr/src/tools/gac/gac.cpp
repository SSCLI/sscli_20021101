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

#include <winwrap.h>
#include <stdio.h>
#include <windows.h>
#include <assert.h>
#include <cor.h>
#include <corerror.h>
#include <corver.h>
#include <utilcode.h>
#include <mscoree.h>
#include <__file__.ver>
#include <palclr.h>
#include <palstartupw.h>

#define INITGUID
#include <guiddef.h>
#include "fusion.h"


//
// Function pointers late bound calls to Fusion.  Fusion is delay loaded for side by side
//
typedef HRESULT (__stdcall *CreateAsmCache)(IAssemblyCache **ppAsmCache, DWORD dwReserved);
typedef HRESULT (__stdcall *CreateAsmNameObj)(LPASSEMBLYNAME *ppAssemblyNameObj, LPCWSTR szAssemblyName, DWORD dwFlags, LPVOID pvReserved);
typedef HRESULT (__stdcall *CreateAsmEnum)(IAssemblyEnum **pEnum, IUnknown *pAppCtx, IAssemblyName *pName, DWORD dwFlags, LPVOID pvReserved);

CreateAsmCache g_pfnCreateAssemblyCache = NULL;
CreateAsmNameObj g_pfnCreateAssemblyNameObject = NULL;
CreateAsmEnum g_pfnCreateAssemblyEnum = NULL;

HMODULE          g_FusionDll = NULL;

bool g_bdisplayLogo = true;
bool g_bSilent = false;

#define BSILENT_PRINTF0ARG(str) if(!g_bSilent) printf(str)
#define BSILENT_PRINTF1ARG(str, arg1) if(!g_bSilent) printf(str, arg1)
#define BSILENT_PRINTF2ARG(str, arg1, arg2) if(!g_bSilent) printf(str, arg1, arg2)

#define MAX_COUNT MAX_PATH

bool FusionInit(void)
{
    // 
    // Load the version of Fusion from the directory the EE is in
    //
    HRESULT hr = LoadLibraryShim(MAKEDLLNAME_W(L"fusion"), 0, 0, &g_FusionDll);

    //
    // Save pointers to call through later
    //
    if (SUCCEEDED(hr))
    {
        if ((g_pfnCreateAssemblyCache      = (CreateAsmCache)GetProcAddress(g_FusionDll, "CreateAssemblyCache")) == NULL) return false;
        if ((g_pfnCreateAssemblyNameObject = (CreateAsmNameObj)GetProcAddress(g_FusionDll, "CreateAssemblyNameObject")) == NULL) return false;
        if ((g_pfnCreateAssemblyEnum       = (CreateAsmEnum)GetProcAddress(g_FusionDll, "CreateAssemblyEnum")) == NULL) return false;
    }
    else
    {
        return false;
    }

    return true;
}

void Title()
{
    if (g_bSilent) return;

    printf("\nMicrosoft (R) Shared Source CLI Global Assembly Cache Utility.  Version " VER_FILEVERSION_STR);
    printf("\n%S\n\n", VER_LEGALCOPYRIGHT_DOS_STR);
}

void Usage()
{
    if (g_bSilent) return;

    printf("Usage: gacutil <option> [<parameters>]\n");
    printf(" Options:\n");
    printf("  /i\n");
    printf("    Installs an assembly to the global assembly cache.  Include the\n");
    printf("    name of the file containing the manifest as a parameter.\n");  
    printf("    Example:  /i myDll.dll \n");
    printf("\n");


    printf("  /u\n");
    printf("    Uninstalls an assembly. Include the name of the assembly to\n"); 
    printf("    remove as a parameter. The assembly is removed from the global\n");
    printf("    assembly cache.\n");
    printf("    Example:\n");
    printf("      /u myDll,Version=1.1.0.0,Culture=en,PublicKeyToken=874e23ab874e23ab\n");
    printf("\n");


    printf("  /l\n");
    printf("    Lists the contents of the global assembly cache. Allows optional\n");
    printf("    assembly name parameter to list matching assemblies only\n");
    printf("\n");


    printf("  /nologo\n");
    printf("    Suppresses display of the logo banner\n");
    printf("\n");

    printf("  /silent\n");
    printf("    Suppresses display of all output\n");
    printf("\n");

    printf("  /?\n");
    printf("    Displays this help screen\n");

}

void ReportError(HRESULT hr)
{
    //
    // First, check to see if this is one of the Fusion HRESULTS
    //
    if (hr == FUSION_E_PRIVATE_ASM_DISALLOWED)
    {
        BSILENT_PRINTF0ARG("Attempt to install an assembly without a strong name\n");
    }
    else if (hr == FUSION_E_SIGNATURE_CHECK_FAILED)
    {
        BSILENT_PRINTF0ARG("Strong name signature could not be verified.  Was the assembly built delay-signed?\n");     
    }
    else if (hr == FUSION_E_ASM_MODULE_MISSING)
    {
        BSILENT_PRINTF0ARG("One or more modules specified in the manifest not found.\n");
    }
    else if (hr == FUSION_E_UNEXPECTED_MODULE_FOUND)
    {
        BSILENT_PRINTF0ARG("One or more modules were streamed in which did not match those specified by the manifest.  Invalid file hash in the manifest?\n");
    }
    else if (hr == FUSION_E_DATABASE_ERROR)
    {
        BSILENT_PRINTF0ARG("An unexpected error was encountered in the assembly cache.\n");
    }
	else if (hr == FUSION_E_INVALID_NAME)
    {
        BSILENT_PRINTF0ARG("Invalid file or assembly name.  The name of the file must be the name of the assembly plus .dll or .exe .\n");
    }
    else
    {
        //
        // Try the system messages
        //
        LPVOID lpMsgBuf;
        DWORD bOk = WszFormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL); 

        if (bOk)
        {
            BSILENT_PRINTF1ARG("\t%ws\n", (LPTSTR)((LPTSTR)lpMsgBuf));
            LocalFree( lpMsgBuf );
        }
        else
        {
            BSILENT_PRINTF0ARG("Unknown Error\n");
        }
    }

}


bool InstallAssembly(LPCWSTR pszManifestFile, LPFUSION_INSTALL_REFERENCE pInstallReference, DWORD dwFlag)
{

    IAssemblyCache*     pCache      = NULL;
    HRESULT             hr          = S_OK;
    
    hr = (*g_pfnCreateAssemblyCache)(&pCache, 0);
    if (FAILED(hr))
    {
        BSILENT_PRINTF0ARG("Failure adding assembly to the cache: ");
        ReportError(hr);
        if (pCache) pCache->Release();
        return false;
    }


    hr = pCache->InstallAssembly(dwFlag, pszManifestFile, pInstallReference);
    if (hr==S_FALSE)
    {
        BSILENT_PRINTF0ARG("Assembly already exists in cache. Use /if option to force overwrite\n");
        pCache->Release();
        return true;
    }
    else
    if (SUCCEEDED(hr))
    {
        BSILENT_PRINTF0ARG("Assembly successfully added to the cache\n");
        pCache->Release();
        return true;
    }
    else
    {
        BSILENT_PRINTF0ARG("Failure adding assembly to the cache: ");
        ReportError(hr);
        pCache->Release();
        return false;
    }

}




bool UninstallListOfAssemblies(IAssemblyCache* pCache,
                                         LPWSTR arrpszDispName[],
                                         DWORD dwDispNameCount,
                                         bool bzapCache, 
                                         LPCFUSION_INSTALL_REFERENCE pInstallReference,
                                         bool bRemoveAllRefs,
                                         DWORD *pdwFailures,
                                         DWORD *pdwOmitFromCount)
{
    HRESULT         hr              = S_OK;
    DWORD           dwCount         = 0;
    DWORD           dwFailures      = 0;
    DWORD           dwOmitFromCount = 0;
    bool            bOmit           = false;
    ULONG           ulDisp          = 0;

    while( dwDispNameCount > dwCount)
    {
        LPWSTR szDisplayName = arrpszDispName[dwCount];
        
        BSILENT_PRINTF1ARG("\nAssembly: %ws\n", szDisplayName);
        

        //Call uninstall with full display name (will always uninstall only 1 assembly)
        hr = pCache->UninstallAssembly(0, szDisplayName, pInstallReference, &ulDisp);
        if (SUCCEEDED(hr) && 
            ((ulDisp == IASSEMBLYCACHE_UNINSTALL_DISPOSITION_REFERENCE_NOT_FOUND) ||
            (ulDisp == IASSEMBLYCACHE_UNINSTALL_DISPOSITION_HAS_INSTALL_REFERENCES)))
        {

            bOmit = true;
        }
        else
        if (SUCCEEDED(hr))
        {   
            MAKE_ANSIPTR_FROMWIDE(pszA, szDisplayName);
            BSILENT_PRINTF1ARG("Uninstalled: %s\n", pszA);
        }
        else
        {
            dwFailures++;
            BSILENT_PRINTF1ARG("\nUninstall Failed for: %ws\n", szDisplayName);
            ReportError(hr);
            hr = S_OK;
        }
        dwCount++;
        if (bOmit)
        {
            dwOmitFromCount++;
            bOmit = false;
        }

    }

    *pdwFailures = dwFailures;
    *pdwOmitFromCount = dwOmitFromCount;

    return true;
}

bool UninstallAssembly(LPCWSTR pszAssemblyName, bool bzapCache, LPCFUSION_INSTALL_REFERENCE pInstallReference, bool bRemoveAllRefs)
{
    IAssemblyCache* pCache          = NULL;
    IAssemblyName*  pEnumName       = NULL;
    IAssemblyName*  pAsmName        = NULL;
    IAssemblyEnum*  pEnum           = NULL;
    HRESULT         hr              = S_OK;
    DWORD           dwCount         = 0;
    DWORD           dwFailures      = 0;
    DWORD           dwOmitFromCount = 0;
    LPWSTR          szDisplayName = NULL;
    DWORD           dwLen       = 0;
    DWORD           dwDisplayFlags = ASM_DISPLAYF_VERSION 
                        | ASM_DISPLAYF_CULTURE
                        | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                        | ASM_DISPLAYF_CUSTOM;

    LPWSTR arrpszDispName[MAX_COUNT+1];
    DWORD dwDispNameCount=0;

    memset( (LPBYTE) arrpszDispName, 0, sizeof(arrpszDispName));

    hr = (*g_pfnCreateAssemblyCache)(&pCache, 0);
    if (FAILED(hr))
    {
        BSILENT_PRINTF0ARG("Failure removing assembly from cache: ");
        ReportError(hr);
        return false;
    }
    
    //Name passed in may be partial, therefore enumerate matching assemblies
    //and uninstall each one. Uninstall API should be called with full name ref.

    //Create AssemblyName for enum
    if ((hr = (*g_pfnCreateAssemblyNameObject)(&pEnumName, pszAssemblyName, CANOF_PARSE_DISPLAY_NAME, NULL)))
    {
        BSILENT_PRINTF0ARG("Failure removing assembly from cache: ");
        ReportError(hr);
        if (pCache) pCache->Release();
        return false;
    }
    

    //
    // For zaps, null out the custom string
    if (bzapCache)
    {
            DWORD dwSize = 0;
            //Check if Custom string has been set
    hr = pEnumName->GetProperty(ASM_NAME_CUSTOM, NULL, &dwSize);

            if (!dwSize)
            {
            //Custom String not set - set to NULL to unset property so lookup is partial
                    pEnumName->SetProperty(ASM_NAME_CUSTOM, NULL, 0);
            }
    }

    hr = (*g_pfnCreateAssemblyEnum)(&pEnum, 
                            NULL, 
                            pEnumName,
                            bzapCache ? ASM_CACHE_ZAP : ASM_CACHE_GAC, 
                            NULL);
    if (hr != S_OK)
    {
        BSILENT_PRINTF1ARG("No assemblies found that match: %ws\n", pszAssemblyName);
    }

    if (pEnumName) pEnumName->Release();

    //Loop through assemblies and uninstall each one
    while (hr == S_OK)
    {
        hr = pEnum->GetNextAssembly(NULL, &pAsmName, 0);
        if (hr == S_OK)
        {
            dwLen = 0;
            hr = pAsmName->GetDisplayName(NULL, &dwLen, dwDisplayFlags);
            if (dwLen)
            {
                szDisplayName = new WCHAR[dwLen+1];
                hr = pAsmName->GetDisplayName(szDisplayName, &dwLen, dwDisplayFlags);
                if (SUCCEEDED(hr))
                {
                    arrpszDispName[dwDispNameCount++] = szDisplayName;
                    szDisplayName = NULL;
                }
                else
                {
                    BSILENT_PRINTF1ARG("Error in IAssemblyName::GetDisplayName (HRESULT=%X)\n",hr);
                }
            }

            if (pAsmName)
            {
                pAsmName->Release();
                pAsmName = NULL;
            }
        }
    }

    if(dwDispNameCount)
    {
        UninstallListOfAssemblies(pCache, 
                                         arrpszDispName,
                                         dwDispNameCount, bzapCache,
                                         pInstallReference,
                                         bRemoveAllRefs,
                                         &dwFailures,
                                         &dwOmitFromCount);

    }

    dwCount = 0;
    while(dwDispNameCount > dwCount)
    {
        delete [] arrpszDispName[dwCount];
        arrpszDispName[dwCount] = NULL;
        dwCount++;
    }

    dwCount = dwDispNameCount - dwFailures - dwOmitFromCount;

    if (pEnum) pEnum->Release();

    BSILENT_PRINTF1ARG("\nNumber of items uninstalled = %d\n",dwCount);

    BSILENT_PRINTF1ARG("Number of failures = %d\n\n",dwFailures);

    
    // if everything failed return a 1 return code....
    if ((dwFailures != 0) && (dwCount == 0))
        return false;
    else
        return true;

}

int EnumerateAssemblies(DWORD dwWhichCache, LPCWSTR pszAssemblyName, bool bPrintInstallRefs)
{
    HRESULT                 hr              = S_OK;
    IAssemblyEnum*          pEnum           = NULL;
    IAssemblyName*          pAsmName        = NULL;
    IAssemblyName*           pEnumName       = NULL;
    DWORD                   dwCount         = 0;
    WCHAR*                  szDisplayName   = NULL;
    DWORD                   dwLen           = 0;
    DWORD                   dwDisplayFlags  = ASM_DISPLAYF_VERSION 
                                    | ASM_DISPLAYF_CULTURE 
                                    | ASM_DISPLAYF_PUBLIC_KEY_TOKEN
                                    | ASM_DISPLAYF_CUSTOM; 

    if (pszAssemblyName)
    {
            if ((hr = (*g_pfnCreateAssemblyNameObject)(&pEnumName, pszAssemblyName, CANOF_PARSE_DISPLAY_NAME, NULL)))
            {
                BSILENT_PRINTF0ARG("Failure enumerating assemblies: ");
                ReportError(hr);            
                return false;
            }
    }
    
    hr = (*g_pfnCreateAssemblyEnum)(&pEnum,
                                    NULL, 
                                    pEnumName,
                                    dwWhichCache,
                                    NULL);
    while (hr == S_OK)
    {
        hr = pEnum->GetNextAssembly(NULL, &pAsmName, 0);
        if (hr == S_OK)
        {
            dwCount++;
            dwLen = 0;
            hr = pAsmName->GetDisplayName(NULL, &dwLen, dwDisplayFlags);
            if (dwLen)
            {
                szDisplayName = new WCHAR[dwLen+1];
                hr = pAsmName->GetDisplayName(szDisplayName, &dwLen, dwDisplayFlags);
                if (SUCCEEDED(hr))
                {
                                        MAKE_ANSIPTR_FROMWIDE(pszA, szDisplayName);
                    BSILENT_PRINTF1ARG("\t%s\n", pszA);
                }
                else
                {
                    BSILENT_PRINTF1ARG("Error displaying assembly name. HRESULT= %x : ", hr);
                }
                delete [] szDisplayName;
                szDisplayName = NULL;
            }

            if (pAsmName)
            {

                pAsmName->Release();
                pAsmName = NULL;
            }
        }
    }
    
    if (pEnum)
    {
        pEnum->Release();
        pEnum = NULL;
    }

    if (pEnumName)
    {
        pEnumName->Release();
        pEnumName = NULL;
    }

    return dwCount;
}



//
// Command line parsing code...
//

#define CURRENT_ARG_STRING &(argv[currentArg][1])

bool ValidSwitchSyntax(int currentArg, LPWSTR argv[])
{
    if ((argv[currentArg][0] == L'-') || (argv[currentArg][0] == L'/')) 
    {
        return true;
    }
    else
    {
        return false;
    }
}

void SetDisplayOptions(int argc, LPWSTR argv[])
{
        for(int currentArg = 1; currentArg < argc; currentArg++)
        {
                // only check switches
                if ((argv[currentArg][0] == L'-') || (argv[currentArg][0] == L'/'))  
                {
                        if (_wcsicmp(CURRENT_ARG_STRING, L"nologo") == 0) g_bdisplayLogo = false;
                        if (_wcsicmp(CURRENT_ARG_STRING, L"silent") == 0) g_bSilent = true;
                }
        }

}

bool CheckArgs(LPCWSTR pszOption, int argsRequired, int argCount)
{

    int argsAllowed = argsRequired;

    assert(argCount >= 2); // if we got this far, we've got at least 2 args.

    //
    // Remove 2 from argCount.  1 for the name of the program and 1 for the name of the option itself
    //
    argCount -= 2;

    //
    // If nologo was specified, allow an additonal arg -- i.e nologo
    //
    if (!g_bdisplayLogo) argsAllowed++;
        
        //
    // If silent was specified, allow an additonal arg -- i.e silent
    //
    if (g_bSilent) argsAllowed++;


    if (argsAllowed != argCount)
    {
        //
        // just customizing the error string
        //
        if (argsRequired == 0)
        {
            BSILENT_PRINTF1ARG("Option -%ws takes no arguments\n", pszOption);
        }
        else
        {
            BSILENT_PRINTF2ARG("Option -%ws takes %i argument\n", pszOption, argsRequired);
        }
        return false;
    }

    return true;
}

extern "C" int __cdecl wmain(int argc, WCHAR **argv)
{
    if (!PAL_RegisterLibraryW(L"rotor_palrt") ||
        !PAL_RegisterLibraryW(L"sscoree"))
    {
        printf("Failed to register libraries\n");
        return 1;
    }

    // Initialize Wsz wrappers.
    OnUnicodeSystem();

    // Initialize Fusion
    if (!FusionInit())
    {
        printf("Failure initializing gacutil\n");
        return 1;
    }

    bool bResult = true;

    if ((argc < 2) || (!ValidSwitchSyntax(1, argv))) 
    {
        Title();
        Usage();
        return 1;
    }

    SetDisplayOptions(argc, argv); // sets g_bdisplayLogo and g_bSilent
    if (g_bdisplayLogo)
    {
            Title();
    }

        int currentArg = 1; // skip the name of the program
        bool argsValid = true;

        // 
        // As long as args are valid...
        //
        while (((currentArg < argc) && (argsValid))) 
        {
                    //
                        // be sure we're dealing with a switch
                        //
                        if (!ValidSwitchSyntax(currentArg, argv))
                        {
                                BSILENT_PRINTF1ARG("Unknown option: %ws\n", &(argv[currentArg][0]));
                                return 1;
                        }

                        if (_wcsicmp(CURRENT_ARG_STRING, L"i") == 0)
                        {
                                //
                                // Determine whether the correct number of args are given
                                //
                                if (!CheckArgs(CURRENT_ARG_STRING, 1, argc))
                                        return 1;
                
                                bResult = InstallAssembly(argv[currentArg+1], NULL, IASSEMBLYCACHE_INSTALL_FLAG_REFRESH);
                                currentArg++; // skip past the assembly file name

                        }
                        else if (_wcsicmp(CURRENT_ARG_STRING, L"u") == 0)
                        {
                                //
                                // Determine whether the correct number of args are given
                                //
                                if (!CheckArgs(CURRENT_ARG_STRING, 1, argc))
                                        return 1;

                                
                                bResult = UninstallAssembly(argv[currentArg+1], false, NULL, false);
                                
                                currentArg++; // skip past the assembly name
                        }
                        else if (_wcsicmp(CURRENT_ARG_STRING, L"l") == 0)
                        {
                                bool bPrintInstallRefs = false;
                                //
                                // Determine whether the correct number of args are given
                                //
                                if (argc > 3)
                                {
                                    BSILENT_PRINTF0ARG("Option -l takes 0 or 1 arguments\n");                               
                                    return 1;
                                }

                                BSILENT_PRINTF0ARG("The Global Assembly Cache contains the following assemblies:\n");
                                int gacCount = 0;
                                gacCount = EnumerateAssemblies(ASM_CACHE_GAC, argv[currentArg+1], bPrintInstallRefs);
                                if (argc == 3)
                                    currentArg++;
                        }
                        else if ((_wcsicmp(CURRENT_ARG_STRING, L"nologo") == 0) || (_wcsicmp(CURRENT_ARG_STRING, L"silent") == 0))
                        {
                                // just skip it.
                        }
                        else if ((_wcsicmp(CURRENT_ARG_STRING, L"?") == 0) || (_wcsicmp(CURRENT_ARG_STRING, L"h") ==0))
                        {
                                Usage();
                                bResult = true;
                        }
                        else
                        {
                                BSILENT_PRINTF1ARG("Unknown option: %ws\n", &(argv[currentArg][0]));
                                bResult = false;
                                argsValid = false; // break out of the loop
                        }

      
        currentArg++;
    }

    return bResult ? 0 : 1;
}
