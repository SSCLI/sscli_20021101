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
#define INITGUID

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <crtdbg.h>
#include <utilcode.h>
#include <malloc.h>
#include <string.h>
#include "debugmacros.h"
#include "corpriv.h"
#include "ceeload.h"
#include "dynamicarray.h"
#include <metamodelpub.h>
#include "formattype.h"

#define DECLARE_DATA
#include "dasmenum.hpp"
#include "dis.h"

#include "dasmgui.h"
#include "resource.h"
#include "dasm_sz.h"

#include "safegetfilesize.h"

#define MAX_FILENAME_LENGTH         512     //256

#include <corsym.h>
#include "version/__file__.ver"
#include "version/corver.h"
#include "rotor_palrt.h"
#define MESSAGE_DLL L"ildasm.satellite"

struct MIDescriptor
{
    mdToken tkClass;    // defining class token
    mdToken tkDecl;     // implemented method token
    mdToken tkBody;     // implementing method token
    mdToken tkBodyParent;   // parent of the implementing method
};

ISymUnmanagedReader*        g_pSymReader = NULL;

IMDInternalImport*      g_pImport = NULL;
IMetaDataImport*        g_pPubImport;
extern IMetaDataAssemblyImport* g_pAssemblyImport;
PELoader *              g_pPELoader;
void *                  g_pMetaData;
IMAGE_COR20_HEADER *    g_CORHeader;
DynamicArray<__int32>  *g_pPtrTags = NULL;      //to keep track of all "ldptr"
DynamicArray<DWORD>    *g_pPtrSize= NULL;      //to keep track of all "ldptr"
int                     g_iPtrCount = 0;
//DynamicArray<mdToken>   g_cl_list;
//DynamicArray<mdToken>   g_cl_enclosing;
mdToken *				g_cl_list = NULL;
mdToken *				g_cl_enclosing = NULL;
DynamicArray<MIDescriptor>   *g_pmi_list = NULL;
DWORD                   g_NumMI;
DWORD                   g_NumClasses;
DWORD                   g_NumModules;
BOOL                    g_fDumpIL = TRUE;
BOOL                    g_fDumpHeader = FALSE;
BOOL                    g_fDumpAsmCode = TRUE;
extern BOOL             g_fDumpTokens; // declared in formatType.cpp
BOOL                    g_fDumpStats = FALSE;
BOOL                    g_fTDC = FALSE;
BOOL                    g_fShowCA = TRUE;

BOOL                    g_fDumpToPerfWriter = FALSE;
HANDLE                  g_PerfDataFilePtr = NULL;

BOOL                    g_fDumpClassList = FALSE;
BOOL                    g_fDumpSummary = FALSE;
BOOL                    g_fDecompile = FALSE; // still in progress
BOOL                    g_fShowBytes = FALSE;
BOOL                    g_fShowSource = FALSE;
BOOL                    g_fPrettyPrint = FALSE;
BOOL                    g_fInsertSourceLines = FALSE;
BOOL                    g_fThisIsInstanceMethod;
BOOL                    g_fTryInCode = TRUE;

BOOL                    g_fLimitedVisibility = FALSE;
BOOL                    g_fHidePub = TRUE;
BOOL                    g_fHidePriv = TRUE;
BOOL                    g_fHideFam = TRUE;
BOOL                    g_fHideAsm = TRUE;
BOOL                    g_fHideFAA = TRUE;
BOOL                    g_fHideFOA = TRUE;
BOOL                    g_fHidePrivScope = TRUE;

extern BOOL             g_fQuoteAllNames; // declared in formatType.cpp, init to FALSE
BOOL		        g_fOnUnicode;

char                    g_szAsmCodeIndent[MAX_MEMBER_LENGTH];
char                    g_szNamespace[MAX_MEMBER_LENGTH];

DWORD                   g_Mode = MODE_DUMP_ALL;

char                    g_pszClassToDump[MAX_CLASSNAME_LENGTH];
char                    g_pszMethodToDump[MAX_MEMBER_LENGTH];
char                    g_pszSigToDump[MAX_SIGNATURE_LENGTH];

BOOL                    g_fCustomInstructionEncodingSystem = FALSE;


COR_FIELD_OFFSET        *g_rFieldOffset = NULL;
ULONG                   g_cFieldsMax, g_cFieldOffsets;
char					g_szInputFile[512]; // in UTF-8
char					g_szOutputFile[512]; // in UTF-8
char*					g_pszObjFileName;
FILE*                   g_pFile = NULL;

mdToken                 g_tkClassToDump = 0;
mdToken                 g_tkMethodToDump = 0;

BOOL                    g_fCheckOwnership = TRUE;
char*                   g_pszOwner = NULL;

unsigned				g_uCodePage = CP_ACP;

char*					g_rchCA = NULL; // dyn.allocated array of CA dumped/not flags
unsigned				g_uNCA = 0;		// num. of CAs

struct ResourceNode;
extern DynamicArray<ResourceNode*> *g_prResNodePtr;
extern DynamicArray<LocalComTypeDescr*> *g_pLocalComType;
extern ULONG    g_LocalComTypeNum;

// MetaInfo integration:
#include <ctype.h>
#include <crtdbg.h>
#include "../tools/metainfo/mdinfo.h"
#include "ivehandler.h"
BOOL                    g_fDumpMetaInfo = FALSE;
ULONG                   g_ulMetaInfoFilter = MDInfo::dumpDefault;
// Validator module type.
DWORD g_ValModuleType = ValidatorModuleTypeInvalid;
IMetaDataDispenserEx *g_pDisp = NULL;
void DisplayFile(wchar_t* szFile, BOOL isFile, ULONG DumpFilter, wchar_t* szObjFile, strPassBackFn pDisplayString);
extern mdMethodDef      g_tkEntryPoint; // integration with MetaInfo
// Abort disassembly flag:
BOOL    g_fAbortDisassembly = FALSE;

DWORD	DumpResourceToFile(WCHAR*	wzFileName); // see DRES.CPP

struct VTableRef
{
    mdMethodDef tkTok;
    WORD        wEntry;
    WORD        wSlot;
};

DynamicArray<VTableRef> *g_prVTableRef = NULL;
ULONG   g_nVTableRef = 0;

#define DUMP_EAT_ENTRIES
struct EATableRef
{
    mdMethodDef tkTok;
    char*       pszName;
};
DynamicArray<EATableRef> *g_prEATableRef=NULL;
ULONG	g_nEATableRef = 0;
ULONG	g_nEATableBase = 0;

HSATELLITE g_hSatelliteInstance;

_TCHAR* Rstr(unsigned id)
{
    static _TCHAR buff[1024];
    WCHAR path[MAX_PATH];
    WCHAR *pEnd, *pSlash;

    // Init the out parameter
    buff[0] = '\0';
    if (g_hSatelliteInstance == NULL)
    {
        // Get the location of the current component
        if (! GetModuleFileNameW(NULL, path, lengthof(path)))
            return 0;

        // Get the location of the last path seperator
        pEnd = wcsrchr(path, L'\\');
        pSlash = wcsrchr(path, L'/');
        if (pSlash > pEnd)
        {
            pEnd = pSlash;
        }
        if (!pEnd)
        {
            return buff;
        }
        ++pEnd;  // point just beyond.

        // Append message DLL name.
        if (sizeof(MESSAGE_DLL) + (pEnd - path) * sizeof(WCHAR) > (int) sizeof(path) - 1)
            return 0;
        wcscpy(pEnd, MESSAGE_DLL);

        // Load the satellite resource information
        g_hSatelliteInstance = PAL_LoadSatelliteResourceW(path);

        if (g_hSatelliteInstance == NULL)
        {
            return buff;
        }
    }
    // Get the string
    PAL_LoadSatelliteStringA(g_hSatelliteInstance, id, buff, 1024);
    return buff;
}

char* RstrA(unsigned n)
{
	static char buff[2048];
	char* ret = buff;
	_TCHAR* ptch = Rstr(n);
	if(sizeof(_TCHAR) > 1)
	{
		memset(buff,0,2048);
		WszWideCharToMultiByte(CP_ACP,0,(LPCWSTR)ptch,-1,buff,2048,NULL,NULL);
	}
	else
		ret = ptch;
	return ret;
}



BOOL Init()
{
    CoInitializeCor(COINITCOR_DEFAULT);
    CoInitializeEE(COINITEE_DEFAULT);
    return TRUE;
}
extern LPCSTR*  rAsmRefName;  // decl. in formatType.cpp -- for AsmRef aliases
extern ULONG	ulNumAsmRefs; // decl. in formatType.cpp -- for AsmRef aliases

void Cleanup()
{
    int i;
    if(g_pAssemblyImport)
    {
        g_pAssemblyImport->Release();
        g_pAssemblyImport = NULL;
    }
    if (g_pPubImport)
    {
        g_pPubImport->Release();
        g_pPubImport = NULL;
    }
    if (g_pImport)
    {
        g_pImport->Release();
        g_pImport = NULL;
    }
	if(g_pDisp)
	{
	    g_pDisp->Release();
		g_pDisp = NULL;
	}

    if(g_pSymReader)
    {
        g_pSymReader->Release();
        g_pSymReader = NULL;
    }
    if (g_pPELoader != NULL)
    {
        g_pPELoader->close();
        delete(g_pPELoader);
        g_pPELoader = NULL;
    }
    g_iPtrCount = 0;
    g_NumClasses = 0;
    g_NumModules = 0;
    g_tkEntryPoint = 0;
    g_szAsmCodeIndent[0] = 0;
    g_szNamespace[0]=0;
    g_pszClassToDump[0]=0;
    g_pszMethodToDump[0]=0;
    g_pszSigToDump[0] = 0;

    g_fCustomInstructionEncodingSystem = FALSE;


    if(rAsmRefName) 
	{ 
		for(i=0; (unsigned)i < ulNumAsmRefs; i++)
		{
			if(rAsmRefName[i]) delete [] rAsmRefName[i];
		}
		delete [] rAsmRefName; 
		rAsmRefName = NULL; 
		ulNumAsmRefs = 0; 
	}

	if(g_rchCA) { delete [] g_rchCA; g_rchCA = NULL; }
}

void Uninit()
{

    if (g_hSatelliteInstance != NULL)
    {
        PAL_FreeSatelliteResource(g_hSatelliteInstance);
    }
    if (g_pPtrTags)
    {
        delete g_pPtrTags;
    }
    if (g_pPtrSize)
    {
        delete g_pPtrSize;
    }
    if (g_pmi_list)
    {
        delete g_pmi_list;
    }
    if (g_pLocalComType)
    {
        delete g_pLocalComType;
    }
    if (g_prVTableRef)
    {
        delete g_prVTableRef;
    }
    if (g_prEATableRef)
    {
        delete g_prEATableRef;
    }
    if (g_prResNodePtr)
    {
        delete g_prResNodePtr;
    }

    CoUninitializeEE(COUNINITEE_DEFAULT);
    CoUninitializeCor();
}

HRESULT IsClassRefInScope(mdTypeRef classref)
{
    HRESULT     hr = S_OK;
    const char  *pszNameSpace;
    const char  *pszClassName;
    mdTypeDef   classdef;
    mdToken     tkRes;

    g_pImport->GetNameOfTypeRef(classref, &pszNameSpace, &pszClassName);
	MAKE_NAME_IF_NONE(pszClassName,classref);
    tkRes = g_pImport->GetResolutionScopeOfTypeRef(classref);

    hr = g_pImport->FindTypeDef(pszNameSpace, pszClassName,
        (TypeFromToken(tkRes) == mdtTypeRef) ? tkRes : mdTokenNil, &classdef);

    return hr;
}


BOOL EnumClasses()
{
    HRESULT         hr;
    HENUMInternal   hEnum;
    ULONG           i = 0,j;
    char            szString[1024];
    HENUMInternal   hBody;
    HENUMInternal   hDecl;

    hr = g_pImport->EnumTypeDefInit(
        &hEnum);

    if (FAILED(hr))
    {
        printError(g_pFile,RstrA(IDS_E_CLSENUM));
        return FALSE;
    }

    g_NumClasses = g_pImport->EnumTypeDefGetCount(&hEnum);

    g_tkClassToDump = 0;

    g_NumMI = 0;
	if(g_NumClasses == 0) return TRUE;
	
	if(g_cl_list) delete [] g_cl_list;
	g_cl_list = new mdToken[g_NumClasses];
	if(g_cl_list == NULL) return FALSE;

    if(g_cl_enclosing) delete [] g_cl_enclosing;
	g_cl_enclosing = new mdToken[g_NumClasses];
	if(g_cl_enclosing == NULL) 
	{ 
		delete [] g_cl_list;
		g_cl_list = NULL;
		return FALSE;
	}

    // fill the list of typedef tokens
    while(g_pImport->EnumTypeDefNext(&hEnum, &g_cl_list[i]))
    {
        const char *pszClassName;
        const char *pszNamespace;
        mdToken     tkEnclosing;
        HRESULT     hr;

        g_pImport->GetNameOfTypeDef(
            g_cl_list[i],
            &pszClassName,
            &pszNamespace
        );
		MAKE_NAME_IF_NONE(pszClassName,g_cl_list[i]);
        if (g_Mode == MODE_DUMP_CLASS || g_Mode == MODE_DUMP_CLASS_METHOD || g_Mode == MODE_DUMP_CLASS_METHOD_SIG)
        {
            if (strcmp(pszClassName, g_pszClassToDump) == 0) g_tkClassToDump = g_cl_list[i];
        }
        g_cl_enclosing[i] = mdTypeDefNil;

        hr = g_pImport->GetNestedClassProps(g_cl_list[i],&tkEnclosing);
        if(SUCCEEDED(hr) && RidFromToken(tkEnclosing))
            g_cl_enclosing[i] = tkEnclosing;
        if(SUCCEEDED(g_pImport->EnumMethodImplInit(g_cl_list[i],&hBody,&hDecl)))
        {
            if((j = g_pImport->EnumMethodImplGetCount(&hBody,&hDecl)))
            {
                mdToken tkBody,tkDecl,tkBodyParent;
                for(ULONG k = 0; k < j; k++)
                {
                    if(g_pImport->EnumMethodImplNext(&hBody,&hDecl,&tkBody,&tkDecl))
                    {
                        if(SUCCEEDED(g_pImport->GetParentToken(tkBody,&tkBodyParent)))
                        {
                            if (g_pmi_list == NULL)
                            {
                                g_pmi_list = new DynamicArray<MIDescriptor>;
                            }
                            (*g_pmi_list)[g_NumMI].tkClass = g_cl_list[i];
                            (*g_pmi_list)[g_NumMI].tkBody = tkBody;
                            (*g_pmi_list)[g_NumMI].tkDecl = tkDecl;
                            (*g_pmi_list)[g_NumMI].tkBodyParent = tkBodyParent;
                            g_NumMI++;
                        }
                    }
                }
            }
            g_pImport->EnumMethodImplClose(&hBody,&hDecl);
        }
        i++;
    }
    g_pImport->EnumTypeDefClose(&hEnum);
    // check nesting consistency
    for(i = 0; i < g_NumClasses; i++)
    {
        if(g_cl_enclosing[i] != mdTypeDefNil)
        {
            if(g_cl_enclosing[i] == g_cl_list[i])
            {
                sprintf(szString,RstrA(IDS_E_SELFNSTD),g_cl_enclosing[i]);
                printError(g_pFile,szString);
                g_cl_enclosing[i] = mdTypeDefNil;
            }
            else
            {
                for(j = 0; (j < g_NumClasses)&&(g_cl_enclosing[i] != g_cl_list[j]); j++);
                if(j == g_NumClasses)
                {
                    sprintf(szString,RstrA(IDS_E_NOENCLOS),
                        g_cl_list[i],g_cl_enclosing[i]);
                    printError(g_pFile,szString);
                    g_cl_enclosing[i] = mdTypeDefNil;
                }
            }
        }
    }

    return TRUE;
}

BOOL AddContextfulAttrib(const char* name)
{
    BOOL retVal = FALSE;

    return retVal;
}

BOOL AddMarshaledByRefAttrib(const char* name)
{
    BOOL retVal = FALSE;

    return retVal;
}
    

BOOL PrintClassList()
{
    DWORD           i;
    BOOL            fSuccess = FALSE;
    char    szString[1024];
    char*   szptr;

    if(g_NumClasses)
    {
        printLine(g_pFile,"// Classes defined in this module:");
        printLine(g_pFile,"//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    
        for (i = 0; i < g_NumClasses; i++)
        {
            const char *pszClassName;
            const char *pszNamespace;
            DWORD   dwClassAttrs;
            mdTypeRef crExtends;

            g_pImport->GetNameOfTypeDef(
                g_cl_list[i],
                &pszClassName,
                &pszNamespace
            );
			MAKE_NAME_IF_NONE(pszClassName,g_cl_list[i]);
            // if this is the "<Module>" class (there is a misnomer) then skip it!
            g_pImport->GetTypeDefProps(
                g_cl_list[i],
                &dwClassAttrs,
                &crExtends
            );
			szptr = &szString[0];
            szptr+=sprintf(szptr,"// ");
            if (IsTdInterface(dwClassAttrs))        szptr+=sprintf(szptr,"Interface ");
            //else if (IsTdValueType(dwClassAttrs))   szptr+=sprintf(szptr,"Value Class");
            //else if (IsTdUnmanagedValueType(dwClassAttrs)) szptr+=sprintf(szptr,"NotInGCHeap Value Class");
            else   szptr+=sprintf(szptr,"Class ");

            szptr+=sprintf(szptr,"%-30s ", pszClassName);

            if (IsTdPublic(dwClassAttrs))           szptr+=sprintf(szptr,"(public) ");
            if (IsTdAbstract(dwClassAttrs))         szptr+=sprintf(szptr,"(abstract) ");
            if (IsTdAutoLayout(dwClassAttrs))       szptr+=sprintf(szptr,"(auto) ");
            if (IsTdSequentialLayout(dwClassAttrs)) szptr+=sprintf(szptr,"(sequential) ");
            if (IsTdExplicitLayout(dwClassAttrs))   szptr+=sprintf(szptr,"(explicit) ");
            if (IsTdAnsiClass(dwClassAttrs))        szptr+=sprintf(szptr,"(ansi) ");
            if (IsTdUnicodeClass(dwClassAttrs))     szptr+=sprintf(szptr,"(unicode) ");
            if (IsTdAutoClass(dwClassAttrs))        szptr+=sprintf(szptr,"(autochar) ");
            if (IsTdImport(dwClassAttrs))           szptr+=sprintf(szptr,"(import) ");
            //if (IsTdEnum(dwClassAttrs))             szptr+=sprintf(szptr,"(enum) ");
            if (IsTdSealed(dwClassAttrs))           szptr+=sprintf(szptr,"(sealed) ");
            if (IsTdNestedPublic(dwClassAttrs))     szptr+=sprintf(szptr,"(nested public) ");
            if (IsTdNestedPrivate(dwClassAttrs))    szptr+=sprintf(szptr,"(nested private) ");
            if (IsTdNestedFamily(dwClassAttrs))     szptr+=sprintf(szptr,"(nested family) ");
            if (IsTdNestedAssembly(dwClassAttrs))   szptr+=sprintf(szptr,"(nested assembly) ");
            if (IsTdNestedFamANDAssem(dwClassAttrs))   szptr+=sprintf(szptr,"(nested famANDassem) ");
            if (IsTdNestedFamORAssem(dwClassAttrs))    szptr+=sprintf(szptr,"(nested famORassem) ");

            printLine(g_pFile,szString);
        }
        printLine(g_pFile,"//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        printLine(g_pFile,"");
    }
    else
        printLine(g_pFile,"// No classes defined in this module");
    fSuccess = TRUE;

    return fSuccess;
}

BOOL ValidateToken(mdToken tk, ULONG type = (ULONG) ~0)
{
    BOOL        bRtn;
    char szString[1024];
    bRtn = g_pImport->IsValidToken(tk);
    if (!bRtn)
    {
        sprintf(szString,RstrA(IDS_E_INVALIDTK), tk);
        printError(g_pFile,szString);
    }
    else if (type != (ULONG) ~0 && TypeFromToken(tk) != type)
    {
        sprintf(szString,RstrA(IDS_E_UNEXPTYPE),
               TypeFromToken(type), TypeFromToken(tk));
        printError(g_pFile,szString);
        bRtn = FALSE;
    }
    return bRtn;
}


BOOL DumpModule(mdModuleRef mdMod)
{
    const char  *pszModName;
    char szString[1024];
    g_pImport->GetModuleRefProps(mdMod,&pszModName);
	MAKE_NAME_IF_NONE(pszModName,mdMod);
    sprintf(szString,"%s.import \"%s\"",g_szAsmCodeIndent,pszModName); // what about GUID and MVID?
    printLine(g_pFile,szString);
    return TRUE;
}

char* DumpPinvokeMap(DWORD   dwMappingFlags, const char  *szImportName, 
                    mdModuleRef    mrImportDLL, char* szString, void* GUICookie)
{
    const char  *szImportDLLName;
    char*   szptr = &szString[strlen(szString)];

    g_pImport->GetModuleRefProps(mrImportDLL,&szImportDLLName);
    if(strlen(szImportDLLName))					szptr = DumpQString(GUICookie,
																	(char*)szImportDLLName,
																	szString,
																	g_szAsmCodeIndent,
																	80);
    //if(strlen(szImportDLLName))					szptr+=sprintf(szptr,"\"%s\"",szImportDLLName);
    //if(szImportName && strlen(szImportName))    szptr+=sprintf(szptr," as \"%s\"",szImportName);
    if(szImportName && strlen(szImportName))
	{
		szptr+=sprintf(szptr," as ");
		szptr = DumpQString(GUICookie,
							(char*)szImportName,
							szString,
							g_szAsmCodeIndent,
							80);
	}
    if(IsPmNoMangle(dwMappingFlags))            szptr+=sprintf(szptr," nomangle");
    if(IsPmCharSetAnsi(dwMappingFlags))         szptr+=sprintf(szptr," ansi");
    if(IsPmCharSetUnicode(dwMappingFlags))      szptr+=sprintf(szptr," unicode");
    if(IsPmCharSetAuto(dwMappingFlags))         szptr+=sprintf(szptr," autochar");
    if(IsPmSupportsLastError(dwMappingFlags))   szptr+=sprintf(szptr," lasterr");
    if(IsPmCallConvWinapi(dwMappingFlags))      szptr+=sprintf(szptr," winapi");
    if(IsPmCallConvCdecl(dwMappingFlags))       szptr+=sprintf(szptr," cdecl");
    if(IsPmCallConvThiscall(dwMappingFlags))    szptr+=sprintf(szptr," thiscall");
    if(IsPmCallConvFastcall(dwMappingFlags))    szptr+=sprintf(szptr," fastcall");
    return szptr;
}

void DumpByteArray(char* szString, BYTE* pBlob, ULONG ulLen, void* GUICookie)
{
    ULONG32 ulStrOffset,j,k;
    char sz[32];
    bool printsz = FALSE;
    char* szptr;
   
    ulStrOffset = (ULONG32) strlen(szString);
    szptr = &szString[ulStrOffset];
	if(!pBlob) ulLen = 0;
    for(j = 0, k=0; j < ulLen; j++,k++)
    {
        if(k == 16)
        {
            if(printsz) sprintf(szptr,"  // %s",sz);
            printLine(GUICookie,szString);
			strcpy(szString,g_szAsmCodeIndent);
            for(k=(ULONG32) strlen(szString); k < ulStrOffset; k++) szString[k] = ' ';
            szString[k] = 0;
            szptr = &szString[ulStrOffset];
            k = 0;
            printsz = FALSE;
        }
        szptr+=sprintf(szptr,"%2.2X ",pBlob[j]);
        if(isprint(pBlob[j]))
        {
            sz[k] = pBlob[j];
            printsz = TRUE;
        }
        else
        {
            sz[k] = '.';
        }
        sz[k+1] = 0;
    }
    szptr+=sprintf(szptr,") ");
    if(printsz)
    {
        for(j = k; j < 16; j++) szptr+=sprintf(szptr,"   ");
        szptr+=sprintf(szptr,"// %s",sz);
    }
}

void DumpCustomAttribute(mdCustomAttribute tkCA, void *GUICookie, bool bWithOwner)
{
    mdToken         tkType;
    BYTE*           pBlob=NULL;
    ULONG           ulLen=0;
    char            szString[4096];
    char*           szptr = &szString[0];
    BOOL            fCommentItOut = FALSE;
	mdToken			tkOwner;
	static mdToken	tkMod = 0xFFFFFFFF;

	_ASSERTE((TypeFromToken(tkCA)==mdtCustomAttribute)&&(RidFromToken(tkCA)>0));
	_ASSERTE(RidFromToken(tkCA) <= g_uNCA);
	if(tkMod == 0xFFFFFFFF) tkMod = g_pImport->GetModuleFromScope();

	// can't use InternalImport here: need the tkOwner 
    g_pPubImport->GetCustomAttributeProps(              // S_OK or error.
                                            tkCA,       // [IN] CustomValue token.
                                            &tkOwner,   // [OUT, OPTIONAL] Object token.
                                            &tkType,    // [OUT, OPTIONAL] Put TypeDef/TypeRef token here.
                             (const void **)&pBlob,     // [OUT, OPTIONAL] Put pointer to data here.
                                            &ulLen);    // [OUT, OPTIONAL] Put size of date here.

	if(!RidFromToken(tkOwner)) return;
	if(tkType == 0x02000001)
	{
		if(tkOwner == tkMod)
		{
            if(g_fCheckOwnership) return;
            fCommentItOut = TRUE;
		}
	}
	else if((TypeFromToken(tkType) == mdtMemberRef)||(TypeFromToken(tkType) == mdtMethodDef))
	{
		mdToken tkParent;
        const char *    pszClassName = NULL;
        const char *    pszNamespace = NULL;
		if(TypeFromToken(tkType) == mdtMemberRef)
			tkParent =  g_pImport->GetParentOfMemberRef(tkType);
		else
			g_pImport->GetParentToken(tkType,&tkParent);

		if(TypeFromToken(tkParent)==mdtTypeDef)
		{
			g_pImport->GetNameOfTypeDef(tkParent, &pszClassName, &pszNamespace);
		}
		else if(TypeFromToken(tkParent)==mdtTypeRef)
		{
			g_pImport->GetNameOfTypeRef(tkParent, &pszNamespace, &pszClassName);
		}
		if(pszClassName && pszNamespace 
			&& (strcmp(pszNamespace,"System.Diagnostics") == 0)
			&& (strcmp(pszClassName,"DebuggableAttribute") == 0)) fCommentItOut = TRUE;


    }
    if(fCommentItOut)
    {
        sprintf(szString,RstrA(IDS_E_AUTOCA),g_szAsmCodeIndent);
        printLine(GUICookie, szString);
		strcat(g_szAsmCodeIndent,"//  ");
    }
	szptr+=sprintf(szptr,"%s.custom ",g_szAsmCodeIndent);
	if(bWithOwner)
	{
		if(g_fDumpTokens)   szptr+=sprintf(szptr,"/*%08X*/ ",tkCA);
		szptr+=sprintf(szptr,"(");
		switch(TypeFromToken(tkOwner))
		{
			case mdtTypeDef :
			case mdtTypeRef	:
			case mdtTypeSpec:
				{
					CQuickBytes out;
					szptr+=sprintf(szptr, "%s",PrettyPrintClass(&out, tkOwner, g_pImport));
				}
				break;

			case mdtMemberRef:
				{
					PCCOR_SIGNATURE typePtr;
					const char*		pszMemberName;
					ULONG			cComSig;

					pszMemberName = g_pImport->GetNameAndSigOfMemberRef(
						tkOwner,
						&typePtr,
						&cComSig
					);
					unsigned callConv = CorSigUncompressData(typePtr);  

					if (isCallConv(callConv, IMAGE_CEE_CS_CALLCONV_FIELD))
						szptr+=sprintf(szptr,"field ");
					else
						szptr+=sprintf(szptr,"method ");
					PrettyPrintMemberRef(szString,tkOwner,g_pImport,GUICookie);
				}
				break;

			case mdtMethodDef:
				szptr += sprintf(szptr, "method ");
				PrettyPrintMethodDef(szString,tkOwner,g_pImport,GUICookie);
				break;

			default :
				strcat(szString,"UNKNOWN_OWNER");
				break;
		}
	    szptr = &szString[strlen(szString)];
		if(g_fDumpTokens)   szptr+=sprintf(szptr,"/*%08X*/ ",tkOwner);
		szptr+=sprintf(szptr,") ");
	}
	else
	{
		if(g_fDumpTokens)   szptr+=sprintf(szptr,"/*%08X:%08X*/ ",tkCA,tkType);
	}
    switch(TypeFromToken(tkType))
    {
        case mdtTypeDef :
        case mdtTypeRef	:
			{
				CQuickBytes out;
				szptr+=sprintf(szptr, "%s",PrettyPrintClass(&out, tkType, g_pImport));
			}
            break;

        case mdtMemberRef:
            PrettyPrintMemberRef(szString,tkType,g_pImport,GUICookie);
            break;

        case mdtMethodDef:
            PrettyPrintMethodDef(szString,tkType,g_pImport,GUICookie);
            break;

        default :
            strcat(szString,"UNNAMED_CUSTOM_ATTR");
            break;
    }
    szptr = &szString[strlen(szString)];
    if(pBlob && ulLen)
    {
        ULONG32 ulStrOffset,j;

        ulStrOffset = (ULONG32) strlen(szString);
        if(ulStrOffset+ulLen+5 >= sizeof(szString)) goto DumpAsHexbytes;

        for(j = 0; (j < ulLen)&&(isprint(pBlob[j])); j++);
        if(j == ulLen) // all symbols printable, display it as quoted string
        {
            szptr+=sprintf(szptr," = \"");
            ulStrOffset = (ULONG32) strlen(szString);
            memcpy(&szString[strlen(szString)],pBlob,ulLen);
            szptr+=ulLen;
            szptr+=sprintf(szptr,"\"");
        }
        else
        {
DumpAsHexbytes:
            sprintf(szptr," = ( ");
            DumpByteArray(szString,pBlob,ulLen,GUICookie);
        }
    }
    printLine(GUICookie, szString);
    if(fCommentItOut) g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-4] = 0;
	_ASSERTE(g_rchCA);
	_ASSERTE(RidFromToken(tkCA) <= g_uNCA);
	g_rchCA[RidFromToken(tkCA)] = 1;
}

void DumpCustomAttributes(mdToken tkOwner, void *GUICookie)
{
    if(g_fShowCA)
    {
        HENUMInternal    hEnum;
        mdCustomAttribute tkCA;
        
        g_pImport->EnumInit(mdtCustomAttribute, tkOwner,&hEnum);
        while(g_pImport->EnumNext(&hEnum,&tkCA) && RidFromToken(tkCA))
        {
            DumpCustomAttribute(tkCA,GUICookie,false);
        }
        g_pImport->EnumClose( &hEnum);
    }
}

void DumpDefaultValue(mdToken tok, char* szString, void* GUICookie)
{
    MDDefaultValue  MDDV;
    char*           szptr = &szString[strlen(szString)];

    g_pImport->GetDefaultValue(tok,&MDDV);
    switch(MDDV.m_bType)
    {
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
            szptr+=sprintf(szptr," = int8(0x%02X)",MDDV.m_byteValue);
            break;
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
            szptr+=sprintf(szptr," = int16(0x%04X)",MDDV.m_usValue);
            break;
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
            szptr+=sprintf(szptr," = int32(0x%08X)",MDDV.m_ulValue);
            break;
		case ELEMENT_TYPE_CHAR:
			szptr+=sprintf(szptr," = char(0x%04X)",MDDV.m_usValue);
			break;
		case ELEMENT_TYPE_BOOLEAN:
			szptr+=sprintf(szptr," = bool(%s)",MDDV.m_byteValue ? "true" : "false");
			break;
        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
            szptr+=sprintf(szptr," = int64(0x%I64X)",MDDV.m_ullValue);
            break;
        case ELEMENT_TYPE_R4:
            {
                char szf[32];
                _gcvt(MDDV.m_fltValue, 8, szf);
                float df = (float)atof(szf);
                if((df == MDDV.m_fltValue)&&(strchr(szf,'#') == NULL))
                    szptr+=sprintf(szptr," = float32(%s)",szf);             
                else
                    szptr+=sprintf(szptr, " = float32(0x%08X)",MDDV.m_ulValue);
                
            }
            break;
        case ELEMENT_TYPE_R8:
            {
                char szf[32], *pch;
                _gcvt(MDDV.m_dblValue, 17, szf);
                double df = strtod(szf, &pch); //atof(szf);
                szf[31]=0;
                if((df == MDDV.m_dblValue)&&(strchr(szf,'#') == NULL))
                    szptr+=sprintf(szptr," = float64(%s)",szf);             
                else
                    szptr+=sprintf(szptr, " = float64(0x%I64X) // %s",MDDV.m_ullValue,szf);
            }
            break;

        case ELEMENT_TYPE_STRING:
			szptr+=sprintf(szptr," = ");
			szptr = DumpUnicodeString(GUICookie,szString,(WCHAR*)MDDV.m_wzValue,MDDV.m_cbSize/sizeof(WCHAR));
            break;

        case ELEMENT_TYPE_CLASS:
			if(MDDV.m_wzValue==NULL)
			{
				szptr+=sprintf(szptr," = nullref");
				break;
			}
			//else fall thru to default case, to report the error

		default:
			szptr+=sprintf(szptr," /* ILLEGAL CONSTANT type:0x%02X, size:%d bytes, blob: ",MDDV.m_bType,MDDV.m_cbSize);
			if(MDDV.m_wzValue)
			{
				szptr+=sprintf(szptr,"(");
				DumpByteArray(szString,(BYTE*)MDDV.m_wzValue,MDDV.m_cbSize,GUICookie);
			}
			else
			{
				szptr+=sprintf(szptr,"NULL");
			}
			strcat(szString, " */");
			break;
    }
}

void DumpParams(ParamDescriptor* pPD, ULONG ulParams, void* GUICookie)
{
    if(pPD)
    {
        for(ULONG i = ulParams; i<2*ulParams+1; i++) // pPD[ulParams] is return value
        {
            ULONG j = i % (ulParams+1);
            if(RidFromToken(pPD[j].tok))
            {
				HENUMInternal    hEnum;
				mdCustomAttribute tkCA;
                ULONG           ulCAs= 0;

                if(g_fShowCA)
				{
					g_pImport->EnumInit(mdtCustomAttribute, pPD[j].tok,&hEnum);
					ulCAs = g_pImport->EnumGetCount(&hEnum);
				}
                if(ulCAs || IsPdHasDefault(pPD[j].attr))
                {
                    char    szString[4096], *szptr = &szString[0];
                    szptr+=sprintf(szptr,"%s.param [%d]",g_szAsmCodeIndent,i-ulParams);
					if(g_fDumpTokens) szptr+=sprintf(szptr,"/*%08X*/ ",pPD[j].tok);
					if(IsPdHasDefault(pPD[j].attr)) DumpDefaultValue(pPD[j].tok, szString, GUICookie);
                    printLine(GUICookie, szString);
					if(ulCAs)
					{
						while(g_pImport->EnumNext(&hEnum,&tkCA) && RidFromToken(tkCA))
						{
							DumpCustomAttribute(tkCA,GUICookie,false);
						}
					}
                }
		        if(g_fShowCA) g_pImport->EnumClose( &hEnum);
            }
        }
    }
}

void DumpPermissions(mdToken tkOwner, void* GUICookie)
{
    HCORENUM hEnum = NULL;
    mdPermission rPerm[16384];
    ULONG count;
    HRESULT hr;
    char    szString[4096];

	// can't use internal import here: EnumInit not impl. for mdtPrmission
    while (SUCCEEDED(hr = g_pPubImport->EnumPermissionSets( &hEnum,
                     tkOwner, 0, rPerm, 16384, &count)) &&
            count > 0)
    {
        for (ULONG i = 0; i < count; i++)
        {
            DWORD dwAction;
            const BYTE *pvPermission=NULL;
            ULONG cbPermission=0;
            char *szAction;


            if(SUCCEEDED(hr = g_pPubImport->GetPermissionSetProps( rPerm[i], &dwAction,
                                                (const void**)&pvPermission, &cbPermission)))
            {
                switch(dwAction)
                {
                    case dclActionNil:          szAction = ""; break;
                    case dclRequest:            szAction = "request"; break;
                    case dclDemand:             szAction = "demand"; break;
                    case dclAssert:             szAction = "assert"; break;
                    case dclDeny:               szAction = "deny"; break;
                    case dclPermitOnly:         szAction = "permitonly"; break;
                    case dclLinktimeCheck:      szAction = "linkcheck"; break;
                    case dclInheritanceCheck:   szAction = "inheritcheck"; break;
                    case dclRequestMinimum:             szAction = "reqmin"; break;
                    case dclRequestOptional:    szAction = "reqopt"; break;
                    case dclRequestRefuse:      szAction = "reqrefuse"; break;
                    case dclPrejitGrant:                szAction = "prejitgrant"; break;
                    case dclPrejitDenied:               szAction = "prejitdeny"; break;
                    case dclNonCasDemand:               szAction = "noncasdemand"; break;
                    case dclNonCasLinkDemand:           szAction = "noncaslinkdemand"; break;
                    case dclNonCasInheritance:          szAction = "noncasinheritance"; break;
                    default:                    szAction = "<UNKNOWN_ACTION>"; break;
                }
                if(pvPermission && cbPermission)
                {
                    sprintf(szString,"%s.permissionset %s = (",g_szAsmCodeIndent,szAction);

                    DumpByteArray(szString,(BYTE*)pvPermission,cbPermission,GUICookie);
                    printLine(GUICookie,szString);
                }
                else // i.e. if pvPermission == NULL or cbPermission == NULL
                {
                    sprintf(szString,"%s.permissionset %s = ()",g_szAsmCodeIndent,szAction);
                    printLine(GUICookie,szString);
                }
                DumpCustomAttributes(rPerm[i],GUICookie);   
            }// end if(GetPermissionProps)
        } // end for(all permissions)
    }//end while(EnumPermissionSets)
    g_pPubImport->CloseEnum( hEnum);
}

void PrettyPrintMethodSig(char* szString, unsigned* puStringLen, CQuickBytes* pqbMemberSig, PCCOR_SIGNATURE pComSig, ULONG cComSig,
						  char* buff, char* szArgPrefix, void* GUICookie)
{
    if(*buff && (strlen(szString) > 40))
    {
        printLine(GUICookie,szString);
        strcpy(szString,g_szAsmCodeIndent);
        strcat(szString,"        "); // to align with ".method "
    }
    appendStr(pqbMemberSig, szString);
    {
        char* pszTailSig = (char*)PrettyPrintSig(pComSig, cComSig, buff, pqbMemberSig, g_pImport,szArgPrefix);
		if(*buff)
		{
			char* newbuff = new char[strlen(buff)+3];
			sprintf(newbuff," %s(", buff);
			char* pszOffset = strstr(pszTailSig,newbuff);
			if(pszOffset)
			{
				if(pszOffset - pszTailSig > 40)
				{
					char* pszOffset2 = strstr(pszTailSig," marshal(");
					if(pszOffset2 && (pszOffset2 < pszOffset))
					{
						*pszOffset2 = 0;
						printLine(GUICookie,pszTailSig);
						strcpy(pszTailSig,g_szAsmCodeIndent);
						strcat(pszTailSig,"        "); // to align with ".method "
						strcat(pszTailSig,pszOffset2+1);
						pszOffset = strstr(pszTailSig,newbuff);
					}
					*pszOffset = 0 ;
					printLine(GUICookie,pszTailSig);
					strcpy(pszTailSig,g_szAsmCodeIndent);
					strcat(pszTailSig,"        "); // to align with ".method "
					strcat(pszTailSig,pszOffset+1);
					pszOffset = strstr(pszTailSig,newbuff);
				}
				int i, j, k, indent = (int) (pszOffset - pszTailSig + strlen(buff) + 2);
				char chAfterComma;
				char *pComma = pszTailSig+strlen(buff), *pch;
                while((pComma = strchr(pComma,',')))
				{
					for(pch = pszTailSig, i=0, j = 0, k=0; pch < pComma; pch++)
					{
						if(*pch == '\'') j=1-j;
						else if(*pch == '\"') k=1-k;
						else if(j==0)
						{
								if(*pch == '[') i++;
								else if(*pch == ']') i--;
						}
					}
					pComma++; 
					if((i==0)&&(j==0)&&(k==0))// no brackets/quotes or all opened/closed
					{
						chAfterComma = *pComma;
						*pComma = 0;
						printLine(GUICookie,pszTailSig);
						*pComma = chAfterComma;
						for(i=0; i<indent; i++) pszTailSig[i] = ' ';
						strcpy(&pszTailSig[indent],pComma);
						pComma = pszTailSig;
					}
				}
				if(*puStringLen < (unsigned)strlen(pszTailSig)+128)
				{
					free(szString);
					*puStringLen = (unsigned)strlen(pszTailSig)+128; // need additional space for "il managed" etc.
					szString = (char*)malloc(*puStringLen);
				}
			}
			sprintf(szString,"%s",pszTailSig);
			delete newbuff;
		}
		else // it's for GUI, don't split it into several lines
		{
			ULONG L = (ULONG) strlen(szString);
			if(L < 2048)
			{
				L = 2048-L;
				strncpy(szString,pszTailSig,L);
			}
		}
    }
}
// helper to avoid mixing of SEH and stack objects with destructors
BOOL DisassembleWrapper(IMDInternalImport *pImport, BYTE *ILHeader, 
    void *GUICookie, mdToken FuncToken, ParamDescriptor* pszArgname, ULONG ulArgs)
{
    BOOL fRet = FALSE;
    char szString[4096];

	PAL_TRY
	{
		fRet = Disassemble(pImport, ILHeader, GUICookie, FuncToken, pszArgname, ulArgs);           
	} 
	PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER) 
	{
		sprintf(szString,RstrA(IDS_E_DASMERR),g_szAsmCodeIndent);
		printLine(GUICookie, szString);
	}
	PAL_ENDTRY

    return fRet;
}
BOOL DumpMethod(mdToken FuncToken, const char *pszClassName, DWORD dwEntryPointToken,void *GUICookie,BOOL DumpBody)
{
    const char      *pszMemberName;//[MAX_MEMBER_LENGTH];
    const char      *pszMemberSig;
    DWORD           dwAttrs;
    DWORD           dwImplAttrs;
    DWORD           dwOffset;
    DWORD           dwTargetRVA;
    CQuickBytes     qbMemberSig;
    PCCOR_SIGNATURE pComSig;
    ULONG           cComSig;
    char            *buff;//[MAX_MEMBER_LENGTH];
    char            *szString;
    ParamDescriptor* pszArgname = NULL;
    ULONG           ulArgs=0;
    unsigned        retParamIx;
	unsigned		uStringLen = 4096;

    char szArgPrefix[16];
    char*           szptr;
//_ASSERTE(0);
    dwAttrs = g_pImport->GetMethodDefProps(FuncToken);
    if(g_fLimitedVisibility)
    {
            if(g_fHidePub && IsMdPublic(dwAttrs)) return FALSE;
            if(g_fHidePriv && IsMdPrivate(dwAttrs)) return FALSE;
            if(g_fHideFam && IsMdFamily(dwAttrs)) return FALSE;
            if(g_fHideAsm && IsMdAssem(dwAttrs)) return FALSE;
            if(g_fHideFOA && IsMdFamORAssem(dwAttrs)) return FALSE;
            if(g_fHideFAA && IsMdFamANDAssem(dwAttrs)) return FALSE;
            if(g_fHidePrivScope && IsMdPrivateScope(dwAttrs)) return FALSE;
    }
    //if(g_fPubOnly && !(IsMdPublic(dwAttrs) || IsMdFamily(dwAttrs) || IsMdFamORAssem(dwAttrs))) return FALSE;
    g_pImport->GetMethodImplProps(FuncToken, &dwOffset, &dwImplAttrs );
    pszMemberName = g_pImport->GetNameOfMethodDef(FuncToken);
	MAKE_NAME_IF_NONE(pszMemberName,FuncToken);
    pComSig = g_pImport->GetSigOfMethodDef(FuncToken, &cComSig);

	if(cComSig == 0)
	{
		char sz[2048];
		sprintf(sz,"%sERROR: method '%s' has no signature",g_szAsmCodeIndent,pszMemberName);
		printError(GUICookie,sz);
		return FALSE;
	}
    pszMemberSig = PrettyPrintSig(pComSig, cComSig, "", &qbMemberSig, g_pImport,NULL);
    
    if (g_Mode == MODE_DUMP_CLASS_METHOD || g_Mode == MODE_DUMP_CLASS_METHOD_SIG)
    {
        if (strcmp(pszMemberName, g_pszMethodToDump) != 0) return FALSE;

        if (g_Mode == MODE_DUMP_CLASS_METHOD_SIG)
        {
            if (strcmp(pszMemberSig, g_pszSigToDump) != 0) return FALSE;
        }
    }

    if(!DumpBody)
    {
        printLine(GUICookie,(char*)pszMemberSig);
        return TRUE;
    }

	szString = (char*)malloc(uStringLen);
	szptr = &szString[0];
    szString[0] = 0;
    if(DumpBody) szptr+=sprintf(szptr,"%s.method ",g_szAsmCodeIndent);
    else szptr+=sprintf(szptr,".method ");

    if(g_fDumpTokens)               szptr+=sprintf(szptr,"/*%08X*/ ",FuncToken);
    if(IsMdPublic(dwAttrs))         szptr+=sprintf(szptr,"public ");
    if(IsMdPrivate(dwAttrs))        szptr+=sprintf(szptr,"private ");
    if(IsMdFamily(dwAttrs))         szptr+=sprintf(szptr,"family ");
    if(IsMdAssem(dwAttrs))          szptr+=sprintf(szptr,"assembly ");
    if(IsMdFamANDAssem(dwAttrs))    szptr+=sprintf(szptr,"famandassem ");
    if(IsMdFamORAssem(dwAttrs))     szptr+=sprintf(szptr,"famorassem ");
    if(IsMdPrivateScope(dwAttrs))   szptr+=sprintf(szptr,"privatescope ");
    if(IsMdHideBySig(dwAttrs))      szptr+=sprintf(szptr,"hidebysig ");
    if(IsMdNewSlot(dwAttrs))        szptr+=sprintf(szptr,"newslot ");
    if(IsMdSpecialName(dwAttrs))    szptr+=sprintf(szptr,"specialname ");
    if(IsMdRTSpecialName(dwAttrs))  szptr+=sprintf(szptr,"rtspecialname ");
    if (IsMdStatic(dwAttrs))        szptr+=sprintf(szptr,"static ");
    if (IsMdFinal(dwAttrs))         szptr+=sprintf(szptr,"final ");
    if (IsMdVirtual(dwAttrs))       szptr+=sprintf(szptr,"virtual ");
    if (IsMdAbstract(dwAttrs))      szptr+=sprintf(szptr,"abstract ");
    if (IsMdUnmanagedExport(dwAttrs))      szptr+=sprintf(szptr,"unmanagedexp ");
	if(IsMdRequireSecObject(dwAttrs))      szptr+=sprintf(szptr,"reqsecobj ");
    if (IsMdPinvokeImpl(dwAttrs))
    {
        DWORD   dwMappingFlags;
        const char  *szImportName;
        mdModuleRef mrImportDLL;

        szptr+=sprintf(szptr,"pinvokeimpl(");
        if(FAILED(g_pImport->GetPinvokeMap(FuncToken,&dwMappingFlags,
            &szImportName,&mrImportDLL)))  szptr+=sprintf(szptr,"/* No map */");
        else
            szptr=DumpPinvokeMap(dwMappingFlags,  (strcmp(szImportName,pszMemberName)? szImportName : NULL), 
                mrImportDLL,szString,GUICookie);
        szptr+=sprintf(szptr,") ");
    }
    // A little hack to get the formatting we need for Assem.
    g_fThisIsInstanceMethod = !IsMdStatic(dwAttrs);
	{
		char *psz;
		if(IsMdPrivateScope(dwAttrs))
		{
			buff = new char[strlen(pszMemberName)+15];
			sprintf(buff,"%s$PST%08X", pszMemberName,FuncToken );
		}
		else
		{
			buff = new char[strlen(pszMemberName)+3];
			sprintf(buff,"%s", pszMemberName );
		}
		psz = ProperName(buff);
		delete [] buff;
		buff = new char[strlen(psz)+1];
		strcpy(buff,psz);
	}
    qbMemberSig.ReSize(0);
    // Get the argument names, if any
    strcpy(szArgPrefix,(g_fThisIsInstanceMethod ? "A1": "A0"));
    {
        PCCOR_SIGNATURE typePtr = pComSig;
        CorSigUncompressData(typePtr);  // get the calling convention out of the way  
        unsigned  numArgs = CorSigUncompressData(typePtr)+1;
		HENUMInternal    hArgEnum;
        mdParamDef  tkArg;
        g_pImport->EnumInit(mdtParamDef,FuncToken,&hArgEnum);
		ulArgs = g_pImport->EnumGetCount(&hArgEnum);
        retParamIx = numArgs-1;
        if(ulArgs < numArgs) ulArgs = numArgs;
        if(ulArgs)
        {
            pszArgname = new ParamDescriptor[ulArgs+2];
            memset(pszArgname,0,(ulArgs+2)*sizeof(ParamDescriptor));
			LPCSTR szName;
            ULONG ulSequence, ix;
            USHORT wSequence;
            DWORD dwAttr;
            ULONG j;
            for(j=0; g_pImport->EnumNext(&hArgEnum,&tkArg) && RidFromToken(tkArg); j++)
            {
				szName = g_pImport->GetParamDefProps(tkArg,&wSequence,&dwAttr);
                ulSequence = wSequence;

                if(ulSequence > ulArgs+1)
                {
                    char sz[256];
                    sprintf(sz,RstrA(IDS_E_PARAMSEQNO),j,ulSequence,ulSequence);
                    printError(GUICookie,sz);
                }
				else
				{
					ix = retParamIx;
					if(ulSequence)
					{
						ix = ulSequence-1;
						if(*szName)
						{
							pszArgname[ix].name = new char[strlen(szName)+1];
							strcpy(pszArgname[ix].name,szName);
						}
					}
					pszArgname[ix].attr = dwAttr;
					pszArgname[ix].tok = tkArg;
				}
			}// end for( along the params)
			for(j=0; j <numArgs; j++)
            {
                if(pszArgname[j].name == NULL) // we haven't got the name!
                {
                    pszArgname[j].name = new char[16];
                    *pszArgname[j].name = 0;
                }
                if(*pszArgname[j].name == 0) // we haven't got the name!
                {
                    sprintf(pszArgname[j].name,"A_%d",g_fThisIsInstanceMethod ? j+1 : j);
                }
            }// end for( along the argnames)
            sprintf(szArgPrefix,"@%d0",pszArgname);
        } //end if (ulArgs)
        g_pImport->EnumClose(&hArgEnum);
    }
	PrettyPrintMethodSig(szString, &uStringLen, &qbMemberSig, pComSig, cComSig,
						  buff, szArgPrefix, GUICookie);
    szptr = &szString[strlen(szString)];
    delete buff;

    if(IsMiNative(dwImplAttrs))         szptr+=sprintf(szptr," native");
    if(IsMiIL(dwImplAttrs))             szptr+=sprintf(szptr," cil");
    if(IsMiOPTIL(dwImplAttrs))          szptr+=sprintf(szptr," optil");
    if(IsMiRuntime(dwImplAttrs))        szptr+=sprintf(szptr," runtime");
    if(IsMiUnmanaged(dwImplAttrs))      szptr+=sprintf(szptr," unmanaged");
    if(IsMiManaged(dwImplAttrs))        szptr+=sprintf(szptr," managed");
    if(IsMiPreserveSig(dwImplAttrs))    szptr+=sprintf(szptr," preservesig");
    if(IsMiForwardRef(dwImplAttrs))     szptr+=sprintf(szptr," forwardref");
    if(IsMiInternalCall(dwImplAttrs))   szptr+=sprintf(szptr," internalcall");
    if(IsMiSynchronized(dwImplAttrs))   szptr+=sprintf(szptr," synchronized");
    if(IsMiNoInlining(dwImplAttrs))     szptr+=sprintf(szptr," noinlining");

    printLine(GUICookie, szString);

    if(!DumpBody)
	{
		free(szString);
		return TRUE;
	}

	if(g_fShowBytes)
	{
	    pComSig = g_pImport->GetSigOfMethodDef(FuncToken, &cComSig);
		szptr = &szString[0];
		szptr+=sprintf(szptr,"%s// SIG:", g_szAsmCodeIndent);
		for(ULONG i=0; i<cComSig; i++) szptr+=sprintf(szptr," %02X",pComSig[i]);
		printLine(GUICookie, szString); 
	}
	
    szptr = &szString[0];
    szptr+=sprintf(szptr,"%s{", g_szAsmCodeIndent);
    printLine(GUICookie, szString); 
    szptr = &szString[0];
    strcat(g_szAsmCodeIndent,"  ");

    // We have recoreded the entry point token from the CLR Header.  Check to see if this
    // method is the entry point.
    if(FuncToken == static_cast<mdToken>(dwEntryPointToken))
    {
        sprintf(szString,"%s.entrypoint", g_szAsmCodeIndent);
        printLine(GUICookie, szString); 
    }
    DumpCustomAttributes(FuncToken,GUICookie);
    DumpParams(pszArgname, retParamIx, GUICookie);
    DumpPermissions(FuncToken,GUICookie);
    // Check if the method represents entry in VTable fixups and in EATable
    {
        ULONG j;
        for(j=0; j<g_nVTableRef; j++)
        {
            if((*g_prVTableRef)[j].tkTok == FuncToken)
            {
                sprintf(szString,"%s.vtentry %d : %d", 
                    g_szAsmCodeIndent,(*g_prVTableRef)[j].wEntry+1,(*g_prVTableRef)[j].wSlot+1);
                printLine(GUICookie, szString); 
                break;
            }
        }
        for(j=0; j<g_nEATableRef; j++)
        {
            if((*g_prEATableRef)[j].tkTok == FuncToken)
            {
                sprintf(szString,"%s.export [%d] as %s", 
                    g_szAsmCodeIndent,j+g_nEATableBase, ProperName((*g_prEATableRef)[j].pszName));
                printLine(GUICookie, szString); 
                break;
            }
        }
    }
    // Dump method impls of this method:
    for(ULONG i = 0; i < g_NumMI; i++)
    {
        if((*g_pmi_list)[i].tkBody == FuncToken)
        {
            const char *    pszMemberName;
            mdToken         tkDeclParent;
            szptr = &szString[0];
            szptr+=sprintf(szptr,"%s.override ",g_szAsmCodeIndent);
            if(TypeFromToken((*g_pmi_list)[i].tkDecl) == mdtMethodDef)
                pszMemberName = g_pImport->GetNameOfMethodDef((*g_pmi_list)[i].tkDecl);
            else 
            {
                PCCOR_SIGNATURE pComSig;
                ULONG       cComSig;

                pszMemberName = g_pImport->GetNameAndSigOfMemberRef((*g_pmi_list)[i].tkDecl,
                    &pComSig,
                    &cComSig
                );
            }
			MAKE_NAME_IF_NONE(pszMemberName,(*g_pmi_list)[i].tkDecl);

            if(FAILED(g_pImport->GetParentToken((*g_pmi_list)[i].tkDecl,&tkDeclParent))) continue;
            if(TypeFromToken(tkDeclParent) == mdtMethodDef) //get the parent's parent
            {
                mdTypeRef cr1;
                if(FAILED(g_pImport->GetParentToken(tkDeclParent,&cr1))) cr1 = mdTypeRefNil;
                tkDeclParent = cr1;
            }
            if(RidFromToken(tkDeclParent))
            {
				CQuickBytes     out;
                szptr+=sprintf(szptr,"%s::",PrettyPrintClass(&out,tkDeclParent,g_pImport));
            }
            szptr+=sprintf(szptr,"%s",ProperName((char*)pszMemberName));

            if(g_fDumpTokens) szptr+=sprintf(szptr," /*%08X::%08X*/ ",tkDeclParent,(*g_pmi_list)[i].tkDecl);
            printLine(GUICookie,szString);
        }
    }
    dwTargetRVA = dwOffset;
    if (IsMdPinvokeImpl(dwAttrs) &&(!IsMdUnmanagedExport(dwAttrs)))
    {
		if(dwOffset)
		{
			sprintf(szString,"%s// Embedded native code",g_szAsmCodeIndent);
			printLine(GUICookie, szString);
			goto ItsMiNative;
		}
        if(g_szAsmCodeIndent[0]) g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
        sprintf(szString,"%s}",g_szAsmCodeIndent);
        printLine(GUICookie, szString);
        return TRUE;
    }



    if(IsMiManaged(dwImplAttrs))
    {
        if(IsMiIL(dwImplAttrs) || IsMiOPTIL(dwImplAttrs))
        {
            if(g_fShowBytes)
            {
                sprintf(szString,RstrA(IDS_E_METHBEG), g_szAsmCodeIndent,dwTargetRVA);
                printLine(GUICookie, szString);
            }
            szString[0] = 0;
            if (dwTargetRVA != 0)
            {
                void* newTarget;
                g_pPELoader->getVAforRVA(dwTargetRVA,&newTarget);
                DisassembleWrapper(g_pImport, (unsigned char*)newTarget, GUICookie, FuncToken,pszArgname, ulArgs);
            }
        }
        else if(IsMiNative(dwImplAttrs))
        {
ItsMiNative:
            sprintf(szString,RstrA(IDS_E_DASMNATIVE), g_szAsmCodeIndent);
            printError(GUICookie, szString); 

            sprintf(szString,"%s//  Managed TargetRVA = 0x%x", g_szAsmCodeIndent, dwTargetRVA);
            printLine(GUICookie, szString); 
        }
    }
    else if(IsMiUnmanaged(dwImplAttrs)&&IsMiNative(dwImplAttrs))
    {
        _ASSERTE(IsMiNative(dwImplAttrs));
        sprintf(szString,"%s//  Unmanaged TargetRVA = 0x%x", g_szAsmCodeIndent, dwTargetRVA);
        printLine(GUICookie, szString); 
    }
    else if(IsMiRuntime(dwImplAttrs))
    {
        sprintf(szString,RstrA(IDS_E_METHODRT), g_szAsmCodeIndent); 
        printLine(GUICookie, szString); 
    }
#ifdef _DEBUG
    else  _ASSERTE(!"Bad dwImplAttrs");
#endif

    if(g_szAsmCodeIndent[0]) g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
	{
		unsigned L = (unsigned)strlen(ProperName((char*)pszMemberName)) +
					 (unsigned)strlen(g_szAsmCodeIndent) + 32 +
					 (pszClassName ? (unsigned)strlen(ProperName((char*)pszClassName)) : 0);
		if(L > uStringLen)
		{
			free(szString);
			szString = (char*)malloc(L);
		}
		if(pszClassName)
		{
			sprintf(szString,"%s} // end of method %s::",g_szAsmCodeIndent, ProperName((char*)pszClassName));
			sprintf(&szString[strlen(szString)],"%s", ProperName((char*)pszMemberName));
		}
		else
			sprintf(szString,"%s} // end of global method %s",g_szAsmCodeIndent, ProperName((char*)pszMemberName));
	}
    printLine(GUICookie, szString);
    szString[0] = 0;
    printLine(GUICookie, szString); 

    if(pszArgname)
    {
        for(ULONG i=0; i < ulArgs; i++)
        {
            if(pszArgname[i].name) delete pszArgname[i].name;
        }
        delete pszArgname;
    }
	free(szString);
    return TRUE;
}

BOOL DumpField(mdToken FuncToken, const char *pszClassName,void *GUICookie, BOOL DumpBody)
{
    char            *pszMemberName;//[MAX_MEMBER_LENGTH];
    DWORD           dwAttrs;
    CQuickBytes     qbMemberSig;
    PCCOR_SIGNATURE pComSig;
    ULONG           cComSig, L;
    const char     *szStr;//[1024];
    char *szString = NULL;
    char*           szptr;

    const char *psz = g_pImport->GetNameOfFieldDef(FuncToken);
	MAKE_NAME_IF_NONE(psz,FuncToken);

    dwAttrs = g_pImport->GetFieldDefProps(FuncToken);
    if(g_fLimitedVisibility)
    {
        if(g_fHidePub && IsFdPublic(dwAttrs)) return FALSE;
        if(g_fHidePriv && IsFdPrivate(dwAttrs)) return FALSE;
        if(g_fHideFam && IsFdFamily(dwAttrs)) return FALSE;
        if(g_fHideAsm && IsFdAssembly(dwAttrs)) return FALSE;
        if(g_fHideFOA && IsFdFamORAssem(dwAttrs)) return FALSE;
        if(g_fHideFAA && IsFdFamANDAssem(dwAttrs)) return FALSE;
        if(g_fHidePrivScope && IsFdPrivateScope(dwAttrs)) return FALSE;
    }

	{
		char* psz1;
		if(IsFdPrivateScope(dwAttrs))
		{
			pszMemberName = new char[strlen(psz)+15];
			sprintf(pszMemberName,"%s$PST%08X", psz,FuncToken );
		}
		else
		{
			pszMemberName = new char[strlen(psz)+3];
			sprintf(pszMemberName,"%s", psz );
		}
		psz1 = ProperName(pszMemberName);
		delete [] pszMemberName;
		pszMemberName = new char[strlen(psz1)+1];
		strcpy(pszMemberName,psz1);
	}
    pComSig = g_pImport->GetSigOfFieldDef(FuncToken, &cComSig);
	if(cComSig == 0)
	{
		char sz[2048];
		sprintf(sz,"%sERROR: field '%s' has no signature",g_szAsmCodeIndent,pszMemberName);
		delete [] pszMemberName;
		printError(GUICookie,sz);
		return FALSE;
	}
    
    szStr = PrettyPrintSig(pComSig, cComSig, (DumpBody ? pszMemberName : ""), &qbMemberSig, g_pImport,NULL);

    if (g_Mode == MODE_DUMP_CLASS_METHOD || g_Mode == MODE_DUMP_CLASS_METHOD_SIG)
    {
        if (strcmp(pszMemberName, g_pszMethodToDump) != 0)
        {
                delete pszMemberName;
                return FALSE;
        }

        if (g_Mode == MODE_DUMP_CLASS_METHOD_SIG)
        {
            if (strcmp(szStr, g_pszSigToDump) != 0)
            {
                    delete pszMemberName;
                    return FALSE;
            }
        }
    }
    delete pszMemberName;
	L = 2048+(ULONG)strlen(szStr);
	szString = new char[L];
	if(szString == NULL)
	{
		printError(GUICookie, "ERROR: not enough memory");
		return FALSE;
	}
    memset(szString,0,L);
    szptr = &szString[0];
    if(DumpBody)
    {
        szptr+=sprintf(szptr,"%s.field ", g_szAsmCodeIndent);
        if(g_fDumpTokens) szptr+=sprintf(szptr,"/*%08X*/ ",FuncToken);
    }

    {   // put offset (if any)
        for(ULONG i=0; i < g_cFieldOffsets; i++)
        {
            if(g_rFieldOffset[i].ridOfField == FuncToken)
            {
                if(g_rFieldOffset[i].ulOffset != 0xFFFFFFFF) szptr+=sprintf(szptr,"[%d] ",g_rFieldOffset[i].ulOffset);
                break;
            }
        }
    }
    if(IsFdPublic(dwAttrs))         szptr+=sprintf(szptr,"public ");
    if(IsFdPrivate(dwAttrs))        szptr+=sprintf(szptr,"private ");
    if(IsFdStatic(dwAttrs))         szptr+=sprintf(szptr,"static ");
    if(IsFdFamily(dwAttrs))         szptr+=sprintf(szptr,"family ");
    if(IsFdAssembly(dwAttrs))       szptr+=sprintf(szptr,"assembly ");
    if(IsFdFamANDAssem(dwAttrs))    szptr+=sprintf(szptr,"famandassem ");
    if(IsFdFamORAssem(dwAttrs))     szptr+=sprintf(szptr,"famorassem ");
    if(IsFdPrivateScope(dwAttrs))   szptr+=sprintf(szptr,"privatescope ");
    if(IsFdInitOnly(dwAttrs))       szptr+=sprintf(szptr,"initonly ");
    if(IsFdLiteral(dwAttrs))        szptr+=sprintf(szptr,"literal ");
    if(IsFdNotSerialized(dwAttrs))  szptr+=sprintf(szptr,"notserialized ");
    if(IsFdSpecialName(dwAttrs))    szptr+=sprintf(szptr,"specialname ");
    if(IsFdRTSpecialName(dwAttrs))  szptr+=sprintf(szptr,"rtspecialname ");
    if (IsFdPinvokeImpl(dwAttrs))
    {
        DWORD   dwMappingFlags;
        const char  *szImportName;
        mdModuleRef mrImportDLL;

        szptr+=sprintf(szptr,"pinvokeimpl(");
        if(FAILED(g_pImport->GetPinvokeMap(FuncToken,&dwMappingFlags,
            &szImportName,&mrImportDLL)))  szptr+=sprintf(szptr,"/* No map */");
        else
            szptr = DumpPinvokeMap(dwMappingFlags,  (strcmp(szImportName,psz)? szImportName : NULL), 
                mrImportDLL, szString,GUICookie);
        szptr+=sprintf(szptr,") ");
    }
    szptr = DumpMarshaling(g_pImport,szString,FuncToken);

	szptr+=sprintf(szptr,"%s",szStr);

    if (IsFdHasFieldRVA(dwAttrs))       // Do we have an RVA associated with this?
    {
        szptr+=sprintf(szptr, " at ");

        ULONG fieldRVA;
        HRESULT hr = g_pImport->GetFieldRVA(FuncToken, &fieldRVA);
        if (SUCCEEDED(hr))
            szptr = DumpDataPtr(&szString[strlen(szString)], fieldRVA, SizeOfField(FuncToken,g_pImport));
        else 
            szptr+=sprintf(szptr,RstrA(IDS_E_NORVA));
    }
    
    // dump default value (if any):
    if(IsFdHasDefault(dwAttrs) && DumpBody)  DumpDefaultValue(FuncToken,szString,GUICookie);
    printLine(GUICookie, szString); 
	delete [] szString;
    if(DumpBody)
    {
        DumpCustomAttributes(FuncToken,GUICookie);      
        DumpPermissions(FuncToken,GUICookie);
    }

    return TRUE;

}

BOOL DumpEvent(mdToken FuncToken, const char *pszClassName, DWORD dwClassAttrs, void *GUICookie, BOOL DumpBody)
{
    DWORD           dwAttrs;
    mdToken         tkEventType;
    LPCSTR          psz;
    HENUMInternal   hAssoc;
    ASSOCIATE_RECORD rAssoc[128];
    CQuickBytes     qbMemberSig;
    //PCCOR_SIGNATURE pComSig;
    //ULONG           cComSig;
    ULONG           nAssoc;
    //char            *buff;
    char            szString[4096];
    char*           szptr;

    
    g_pImport->GetEventProps(FuncToken,&psz,&dwAttrs,&tkEventType);
	MAKE_NAME_IF_NONE(psz,FuncToken);
    if (g_Mode == MODE_DUMP_CLASS_METHOD || g_Mode == MODE_DUMP_CLASS_METHOD_SIG)
    {
        if (strcmp(psz, g_pszMethodToDump) != 0)  return FALSE;
    }

    g_pImport->EnumAssociateInit(FuncToken,&hAssoc);
    if((nAssoc = hAssoc.m_ulCount))
    {
        memset(rAssoc,0,sizeof(rAssoc));
        g_pImport->GetAllAssociates(&hAssoc,rAssoc,nAssoc);

                if(g_fLimitedVisibility)
                {
                        unsigned i;
                        for(i=0; i < nAssoc;i++)
                        {
                                //if(IsMsAddOn(rAssoc[i].m_dwSemantics) || IsMsRemoveOn(rAssoc[i].m_dwSemantics) || IsMsFire(rAssoc[i].m_dwSemantics))
                                {
                                        DWORD dwAttrs = g_pImport->GetMethodDefProps(rAssoc[i].m_memberdef);
                                        if(g_fHidePub && IsMdPublic(dwAttrs)) continue;
                                        if(g_fHidePriv && IsMdPrivate(dwAttrs)) continue;
                                        if(g_fHideFam && IsMdFamily(dwAttrs)) continue;
                                        if(g_fHideAsm && IsMdAssem(dwAttrs)) continue;
                                        if(g_fHideFOA && IsMdFamORAssem(dwAttrs)) continue;
                                        if(g_fHideFAA && IsMdFamANDAssem(dwAttrs)) continue;
                                        if(g_fHidePrivScope && IsMdPrivateScope(dwAttrs)) continue;
                                        break;
                                        //if(IsMdPublic(dwAttrs) || IsMdFamily(dwAttrs) || IsMdFamORAssem(dwAttrs)) break;
                                }
                        }
                        if(i >= nAssoc) return FALSE;
                }
        }
    
    szptr = &szString[0];
    if(DumpBody)
    {
        szptr+=sprintf(szptr,"%s.event ", g_szAsmCodeIndent);
        if(g_fDumpTokens) szptr+=sprintf(szptr,"/*%08X*/ ",FuncToken);
    }
    else
    {
        szptr+=sprintf(szptr,"%s : ",ProperName((char*)psz));
    }

    if(IsEvSpecialName(dwAttrs))    szptr+=sprintf(szptr,"specialname ");
    if(IsEvRTSpecialName(dwAttrs))  szptr+=sprintf(szptr,"rtspecialname ");

    if(RidFromToken(tkEventType))
    {
            switch(TypeFromToken(tkEventType))
            {
                    case mdtTypeRef:
                    case mdtTypeDef:
                    case mdtTypeSpec:
                        {
                            CQuickBytes     out;
                            szptr+=sprintf(szptr,"%s",PrettyPrintClass(&out,tkEventType,g_pImport));
                            if(g_fDumpTokens) szptr+=sprintf(szptr," /*%08X*/",tkEventType);
                        }
                        break;
                    default:
                        break;
            }
    }

    if(!DumpBody)
    {
        printLine(GUICookie,szString);
        return TRUE;
    }

    
    szptr+=sprintf(szptr," %s", ProperName((char*)psz));
    printLine(GUICookie,szString);
    sprintf(szString,"%s{",g_szAsmCodeIndent);
    printLine(GUICookie,szString);
    strcat(g_szAsmCodeIndent,"  ");

    DumpCustomAttributes(FuncToken,GUICookie);    
    DumpPermissions(FuncToken,GUICookie);
    
    if(nAssoc)
    {
        for(unsigned i=0; i < nAssoc;i++)
        {
            szptr = &szString[0];
            if(IsMsAddOn(rAssoc[i].m_dwSemantics))          szptr+=sprintf(szptr,"%s.addon ",g_szAsmCodeIndent);
            else if(IsMsRemoveOn(rAssoc[i].m_dwSemantics))  szptr+=sprintf(szptr,"%s.removeon ",g_szAsmCodeIndent);
            else if(IsMsFire(rAssoc[i].m_dwSemantics))      szptr+=sprintf(szptr,"%s.fire ",g_szAsmCodeIndent);
            else if(IsMsOther(rAssoc[i].m_dwSemantics))     szptr+=sprintf(szptr,"%s.other ",g_szAsmCodeIndent);

            if (TypeFromToken(rAssoc[i].m_memberdef) == mdtMethodDef)
                PrettyPrintMethodDef(szString,rAssoc[i].m_memberdef,g_pImport,GUICookie);
            else
            {
                _ASSERTE(TypeFromToken(rAssoc[i].m_memberdef) == mdtMemberRef);
                PrettyPrintMemberRef(szString,rAssoc[i].m_memberdef,g_pImport,GUICookie);
            }
            printLine(GUICookie,szString);
        }
    }
    if(g_szAsmCodeIndent[0]) g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
    sprintf(szString,"%s} // end of event %s::",g_szAsmCodeIndent, ProperName((char*)pszClassName));
    sprintf(&szString[strlen(szString)],"%s",ProperName((char*)psz));
    printLine(GUICookie,szString);
    return TRUE;

}

BOOL DumpProp(mdToken FuncToken, const char *pszClassName, DWORD dwClassAttrs, void *GUICookie, BOOL DumpBody)
{
    DWORD           dwAttrs;
    LPCSTR          psz;
    HENUMInternal   hAssoc;
    ASSOCIATE_RECORD rAssoc[128];
    CQuickBytes     qbMemberSig;
    PCCOR_SIGNATURE pComSig;
    ULONG           cComSig, nAssoc;
    char*           szString=NULL;
    unsigned	    uStringLen = 4096;
    char*           szptr;

    
    g_pImport->GetPropertyProps(FuncToken,&psz,&dwAttrs,&pComSig,&cComSig);
	MAKE_NAME_IF_NONE(psz,FuncToken);
	if(cComSig == 0)
	{
		char sz[2048];
		sprintf(sz,"%sERROR: property '%s' has no signature",g_szAsmCodeIndent,psz);
		printError(GUICookie,sz);
		return FALSE;
	}

    if (g_Mode == MODE_DUMP_CLASS_METHOD || g_Mode == MODE_DUMP_CLASS_METHOD_SIG)
    {
        if (strcmp(psz, g_pszMethodToDump) != 0)  return FALSE;
    }

    g_pImport->EnumAssociateInit(FuncToken,&hAssoc);
    if((nAssoc = hAssoc.m_ulCount))
    {
        memset(rAssoc,0,sizeof(rAssoc));
        g_pImport->GetAllAssociates(&hAssoc,rAssoc,nAssoc);

        if(g_fLimitedVisibility)
        {
            unsigned i;
            for(i=0; i < nAssoc;i++)
            {
                //if(IsMsSetter(rAssoc[i].m_dwSemantics) || IsMsGetter(rAssoc[i].m_dwSemantics))
                {
                    DWORD dwAttrs = g_pImport->GetMethodDefProps(rAssoc[i].m_memberdef);
                    if(g_fHidePub && IsMdPublic(dwAttrs)) continue;
                    if(g_fHidePriv && IsMdPrivate(dwAttrs)) continue;
                    if(g_fHideFam && IsMdFamily(dwAttrs)) continue;
                    if(g_fHideAsm && IsMdAssem(dwAttrs)) continue;
                    if(g_fHideFOA && IsMdFamORAssem(dwAttrs)) continue;
                    if(g_fHideFAA && IsMdFamANDAssem(dwAttrs)) continue;
                    if(g_fHidePrivScope && IsMdPrivateScope(dwAttrs)) continue;
                    break;
                }
            }
            if(i >= nAssoc) return FALSE;
        }
    }
	szString = (char*)malloc(uStringLen);
	if(szString==NULL) return FALSE;
    szptr = &szString[0];
    if(DumpBody)
    {
        szptr+=sprintf(szptr,"%s.property ", g_szAsmCodeIndent);
        if(g_fDumpTokens) szptr+=sprintf(szptr,"/*%08X*/ ",FuncToken);
    }
    else
    {
        szptr+=sprintf(szptr,"%s : ",ProperName((char*)psz));
    }

    if(IsPrSpecialName(dwAttrs))        szptr+=sprintf(szptr,"specialname ");
    if(IsPrRTSpecialName(dwAttrs))      szptr+=sprintf(szptr,"rtspecialname ");

	{
		char *pch = "";
		if(DumpBody)
		{
			pch = szptr+1;
			strcpy(pch,ProperName((char*)psz));
		}
	    qbMemberSig.ReSize(0);
		PrettyPrintMethodSig(szString, &uStringLen, &qbMemberSig, pComSig, cComSig,
							  pch, NULL, GUICookie);
		if(IsPrHasDefault(dwAttrs) && DumpBody) DumpDefaultValue(FuncToken,szString,GUICookie);
	}
    printLine(GUICookie,szString);

    if(DumpBody) 
	{
		sprintf(szString,"%s{",g_szAsmCodeIndent);
		printLine(GUICookie,szString);
		strcat(g_szAsmCodeIndent,"  ");

		DumpCustomAttributes(FuncToken,GUICookie);    
		DumpPermissions(FuncToken,GUICookie);
    
		if(nAssoc)
		{
			for(unsigned i=0; i < nAssoc;i++)
			{
				szptr = &szString[0];
				if(IsMsSetter(rAssoc[i].m_dwSemantics))         szptr+=sprintf(szptr,"%s.set ",g_szAsmCodeIndent);
				else if(IsMsGetter(rAssoc[i].m_dwSemantics))    szptr+=sprintf(szptr,"%s.get ",g_szAsmCodeIndent);
				else if(IsMsOther(rAssoc[i].m_dwSemantics))     szptr+=sprintf(szptr,"%s.other ",g_szAsmCodeIndent);
				if(g_fDumpTokens) szptr+=sprintf(szptr,"/*%08X*/ ",rAssoc[i].m_memberdef);
				if (TypeFromToken(rAssoc[i].m_memberdef) == mdtMethodDef)
					PrettyPrintMethodDef(szString,rAssoc[i].m_memberdef,g_pImport,GUICookie);
				else
				{
					_ASSERTE(TypeFromToken(rAssoc[i].m_memberdef) == mdtMemberRef);
					PrettyPrintMemberRef(szString,rAssoc[i].m_memberdef,g_pImport,GUICookie);
				}
				printLine(GUICookie,szString);
			}
		}
		if(g_szAsmCodeIndent[0]) g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
		sprintf(szString,"%s} // end of property %s::",g_szAsmCodeIndent, ProperName((char*)pszClassName));
		sprintf(&szString[strlen(szString)],"%s",ProperName((char*)psz));
		printLine(GUICookie,szString);
	} // end if(DumpBody)
	free(szString);
    return TRUE;

}

BOOL DumpMembers(mdTypeDef cl, const char *pszClassNamespace, const char *pszClassName, 
                 DWORD dwClassAttrs, DWORD dwEntryPointToken, void* GUICookie)
{
    HRESULT         hr;
    mdToken         *pMemberList = NULL;
    DWORD           NumMembers, NumFields,NumMethods,NumEvents,NumProps;
    DWORD           i;
    HENUMInternal   hEnumMethod;
    HENUMInternal   hEnumField;
    HENUMInternal   hEnumEvent;
    HENUMInternal   hEnumProp;
    CQuickBytes     qbMemberSig;
    BOOL            ret;

    // Get the total count of methods + fields
    hr = g_pImport->EnumInit(mdtMethodDef, cl, &hEnumMethod);
    if (FAILED(hr))
    {
FailedToEnum:
        printLine(GUICookie,RstrA(IDS_E_MEMBRENUM));
        ret = FALSE;
        goto CloseHandlesAndReturn;
    }
    NumMembers = NumMethods = g_pImport->EnumGetCount(&hEnumMethod);


    if (FAILED(g_pImport->EnumInit(mdtFieldDef, cl, &hEnumField)))   goto FailedToEnum;
    NumFields = g_pImport->EnumGetCount(&hEnumField);
    NumMembers += NumFields;

    if (FAILED(g_pImport->EnumInit(mdtEvent, cl, &hEnumEvent))) goto FailedToEnum;
    NumEvents = g_pImport->EnumGetCount(&hEnumEvent);
    NumMembers += NumEvents;

    if (FAILED(g_pImport->EnumInit(mdtProperty, cl, &hEnumProp))) goto FailedToEnum;
    NumProps = g_pImport->EnumGetCount(&hEnumProp);
    NumMembers += NumProps;
    ret = TRUE;
    if(NumMembers)
    {
        pMemberList = (mdToken*) _alloca(sizeof(mdToken) * NumMembers);
        if(pMemberList == NULL) ret = FALSE;
    }
    if ((NumMembers == 0)||(pMemberList == NULL)) goto CloseHandlesAndReturn;

    for (i = 0; g_pImport->EnumNext(&hEnumField, &pMemberList[i]); i++);
    for (; g_pImport->EnumNext(&hEnumMethod, &pMemberList[i]); i++);
    for (; g_pImport->EnumNext(&hEnumEvent, &pMemberList[i]); i++);
    for (; g_pImport->EnumNext(&hEnumProp, &pMemberList[i]); i++);
    _ASSERTE(i == NumMembers);

    for (i = 0; i < NumMembers; i++)
    {
        switch (TypeFromToken(pMemberList[i]))
        {
            case mdtFieldDef:
                ret = DumpField(pMemberList[i], pszClassName, GUICookie,TRUE);
                break;

            case mdtMethodDef:
                ret = DumpMethod(pMemberList[i], pszClassName, dwEntryPointToken,GUICookie,TRUE);
                break;

            case mdtEvent:
                ret = DumpEvent(pMemberList[i], pszClassName, dwClassAttrs,GUICookie,TRUE);
                break;

            case mdtProperty:
                ret = DumpProp(pMemberList[i], pszClassName, dwClassAttrs,GUICookie,TRUE);
                break;

            default:
                {
                    char szStr[256];
                    sprintf(szStr,RstrA(IDS_E_ODDMEMBER),pMemberList[i],pszClassName);
                    printLine(GUICookie,szStr);
                }
                ret = FALSE;
                break;
        } // end switch
        if(ret && (g_Mode == MODE_DUMP_CLASS_METHOD_SIG)) break;
    } // end for
    ret = TRUE;
CloseHandlesAndReturn:
    g_pImport->EnumClose(&hEnumMethod);
    g_pImport->EnumClose(&hEnumField);
    g_pImport->EnumClose(&hEnumEvent);
    g_pImport->EnumClose(&hEnumProp);
    return ret;
}
BOOL GetClassLayout(mdTypeDef cl, ULONG* pulPackSize, ULONG* pulClassSize)
{ // Dump class layout
    HENUMInternal   hEnumField;
    BOOL ret = FALSE;

    if(g_rFieldOffset) {delete g_rFieldOffset; g_rFieldOffset = NULL; }
    g_cFieldOffsets = 0;
    g_cFieldsMax = 0;
    
    if(RidFromToken(cl)==0) return TRUE;

    if (SUCCEEDED(g_pImport->EnumInit(mdtFieldDef, cl, &hEnumField)))
    {
        g_cFieldsMax = g_pImport->EnumGetCount(&hEnumField);
        g_pImport->EnumClose(&hEnumField);
    }

	if(SUCCEEDED(g_pImport->GetClassPackSize(cl,pulPackSize))) ret = TRUE;
	else *pulPackSize = 0xFFFFFFFF;
	if(SUCCEEDED(g_pImport->GetClassTotalSize(cl,pulClassSize))) ret = TRUE;
	else *pulClassSize = 0xFFFFFFFF;

    if(g_cFieldsMax)
	{
		MD_CLASS_LAYOUT Layout;
		if(SUCCEEDED(g_pImport->GetClassLayoutInit(cl,&Layout)))
		{
			g_rFieldOffset = new COR_FIELD_OFFSET[g_cFieldsMax+1];
			if(g_rFieldOffset)
			{
				COR_FIELD_OFFSET* pFO = g_rFieldOffset;
				for(g_cFieldOffsets=0; 
					SUCCEEDED(g_pImport->GetClassLayoutNext(&Layout,&(pFO->ridOfField),&(pFO->ulOffset)))
						&&RidFromToken(pFO->ridOfField);
					g_cFieldOffsets++, pFO++) ret = TRUE;
			}
		}
	}
    return ret;
}

BOOL DumpClass(mdTypeDef cl, DWORD dwEntryPointToken, void* GUICookie, ULONG WhatToDump)
// WhatToDump: 0-title,flags,extends,implements;
//            +1-pack,size and custom attrs; 
//			  +2-nested classes
//			  +4-members
{
    char            *pszClassName; // name associated with this CL
    char            *pszNamespace;
    const char      *pc1,*pc2;
    DWORD           dwClassAttrs;
    mdTypeRef       crExtends;
    HRESULT         hr;
    mdInterfaceImpl ii;
    DWORD           NumInterfaces;
    DWORD           i;
    HENUMInternal   hEnumII;            // enumerator for interface impl
    char            *szString;
    char*           szptr;


    g_pImport->GetNameOfTypeDef(
        cl,
        &pc1,   //&pszClassName,
        &pc2    //&pszNamespace
    );
	MAKE_NAME_IF_NONE(pc1,cl);

    if (g_Mode == MODE_DUMP_CLASS || g_Mode == MODE_DUMP_CLASS_METHOD || g_Mode == MODE_DUMP_CLASS_METHOD_SIG)
    {
	    LPUTF8              szFullName;
		MAKE_FULL_PATH_ON_STACK_UTF8(szFullName, pc2, pc1);
        if (strcmp(szFullName, g_pszClassToDump) != 0)
            return TRUE;
    }
    
    g_pImport->GetTypeDefProps(
        cl,
        &dwClassAttrs,
        &crExtends
    );
    if(g_fLimitedVisibility)
    {
            if(g_fHidePub && (IsTdPublic(dwClassAttrs)||IsTdNestedPublic(dwClassAttrs))) return FALSE;
            if(g_fHidePriv && (IsTdNotPublic(dwClassAttrs)||IsTdNestedPrivate(dwClassAttrs))) return FALSE;
            if(g_fHideFam && IsTdNestedFamily(dwClassAttrs)) return FALSE;
            if(g_fHideAsm && IsTdNestedAssembly(dwClassAttrs)) return FALSE;
            if(g_fHideFOA && IsTdNestedFamORAssem(dwClassAttrs)) return FALSE;
            if(g_fHideFAA && IsTdNestedFamANDAssem(dwClassAttrs)) return FALSE;
    }


    pszClassName = (char*)(pc1 ? pc1 : "");
    pszNamespace = (char*)(pc2 ? pc2 : "");
	szString = new char[4096];//(char*)malloc(4096);

    if(!IsTdNested(dwClassAttrs)) // don't dump namespaces in GUI mode!
    {
        // take care of namespace, if any
        if(strcmp(pszNamespace,g_szNamespace))
        {
            if(strlen(g_szNamespace))
            {
                if(g_szAsmCodeIndent[0]) g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
                sprintf(szString,"%s} // end of namespace %s",g_szAsmCodeIndent, ProperName(g_szNamespace));
                printLine(GUICookie,szString);
                printLine(GUICookie,"");
            }
            strcpy(g_szNamespace,pszNamespace);
            if(strlen(g_szNamespace))
            {
                sprintf(szString,"%s.namespace %s",
					g_szAsmCodeIndent, ProperName(g_szNamespace));
                printLine(GUICookie,szString);
                sprintf(szString,"%s{",g_szAsmCodeIndent);
                printLine(GUICookie,szString);
                strcat(g_szAsmCodeIndent,"  ");
            }
        }
    }

    szptr = &szString[0];
    szptr+=sprintf(szptr,"%s.class ",g_szAsmCodeIndent);
    if(g_fDumpTokens) szptr+=sprintf(szptr,"/*%08X*/ ",cl);

	if (IsTdInterface(dwClassAttrs))                szptr+=sprintf(szptr,"interface ");
	if (IsTdPublic(dwClassAttrs))                   szptr+=sprintf(szptr,"public ");
	if (IsTdNotPublic(dwClassAttrs))                szptr+=sprintf(szptr,"private ");
	if (IsTdAbstract(dwClassAttrs))                 szptr+=sprintf(szptr,"abstract ");
	if (IsTdAutoLayout(dwClassAttrs))               szptr+=sprintf(szptr,"auto ");
	if (IsTdSequentialLayout(dwClassAttrs))         szptr+=sprintf(szptr,"sequential ");
	if (IsTdExplicitLayout(dwClassAttrs))           szptr+=sprintf(szptr,"explicit ");
	if (IsTdAnsiClass(dwClassAttrs))                szptr+=sprintf(szptr,"ansi ");
	if (IsTdUnicodeClass(dwClassAttrs))             szptr+=sprintf(szptr,"unicode ");
	if (IsTdAutoClass(dwClassAttrs))                szptr+=sprintf(szptr,"autochar ");
	if (IsTdImport(dwClassAttrs))                   szptr+=sprintf(szptr,"import ");
	if (IsTdSerializable(dwClassAttrs))             szptr+=sprintf(szptr,"serializable ");
	if (IsTdSealed(dwClassAttrs))                   szptr+=sprintf(szptr,"sealed ");
	if (IsTdNestedPublic(dwClassAttrs))             szptr+=sprintf(szptr,"nested public ");
	if (IsTdNestedPrivate(dwClassAttrs))            szptr+=sprintf(szptr,"nested private ");
	if (IsTdNestedFamily(dwClassAttrs))             szptr+=sprintf(szptr,"nested family ");
	if (IsTdNestedAssembly(dwClassAttrs))           szptr+=sprintf(szptr,"nested assembly ");
	if (IsTdNestedFamANDAssem(dwClassAttrs))        szptr+=sprintf(szptr,"nested famandassem ");
	if (IsTdNestedFamORAssem(dwClassAttrs))         szptr+=sprintf(szptr,"nested famorassem ");
	if (IsTdBeforeFieldInit(dwClassAttrs))          szptr+=sprintf(szptr,"beforefieldinit ");
	if (IsTdSpecialName(dwClassAttrs))              szptr+=sprintf(szptr,"specialname ");
	if (IsTdRTSpecialName(dwClassAttrs))            szptr+=sprintf(szptr,"rtspecialname ");

    szptr+=sprintf(szptr,"%s", ProperName(pszClassName));
	printLine(GUICookie,szString);
	if (!IsNilToken(crExtends))
	{
		CQuickBytes out;
		sprintf(szString,"%s       extends %s",g_szAsmCodeIndent,PrettyPrintClass(&out,crExtends,g_pImport));
		printLine(GUICookie,szString);
	}

	hr = g_pImport->EnumInit(
		mdtInterfaceImpl, 
		cl, 
		&hEnumII);
	if (FAILED(hr))
	{
		printLine(GUICookie,RstrA(IDS_E_ENUMINIT));
		delete [] szString; //free(szString);
		return FALSE;
	}

	NumInterfaces = g_pImport->EnumGetCount(&hEnumII);

	if (NumInterfaces > 0)
	{
		CQuickBytes out;
		mdTypeRef   crInterface;
		for (i=0; g_pImport->EnumNext(&hEnumII, &ii); i++)
		{
			szptr = szString;
			szptr+=sprintf(szptr, (i == 0 ? "%s       implements " : "%s                  "),g_szAsmCodeIndent);
			crInterface = g_pImport->GetTypeOfInterfaceImpl(ii);
			szptr += sprintf(szptr,"%s",PrettyPrintClass(&out,crInterface,g_pImport));
			if(i < NumInterfaces-1) strcat(szString,",");
			printLine(GUICookie,szString);
			out.ReSize(0);
		}
		// The assertion will fire if the enumerator is bad
		_ASSERTE(NumInterfaces == i);

		g_pImport->EnumClose(&hEnumII);
	}
    if(WhatToDump == 0) // 0 = title only
	{
		sprintf(szString,"%s{ }",g_szAsmCodeIndent);
		printLine(GUICookie,szString);
		delete [] szString; //free(szString);
		return TRUE;
	}
    sprintf(szString,"%s{",g_szAsmCodeIndent);
    printLine(GUICookie,szString);
    strcat(g_szAsmCodeIndent,"  ");

	ULONG ulPackSize=0xFFFFFFFF,ulClassSize=0xFFFFFFFF;
	if(WhatToDump & 1)
	{
		if(GetClassLayout(cl,&ulPackSize,&ulClassSize))
		{ // Dump class layout
			if(ulPackSize != 0xFFFFFFFF)
			{
				sprintf(szString,"%s.pack %d",g_szAsmCodeIndent,ulPackSize);
				printLine(GUICookie,szString);
			}
			if(ulClassSize != 0xFFFFFFFF)
			{
				sprintf(szString,"%s.size %d",g_szAsmCodeIndent,ulClassSize);
				printLine(GUICookie,szString);
			}
		}
		DumpCustomAttributes(cl,GUICookie);
		DumpPermissions(cl,GUICookie);
		for(i=0; i<g_LocalComTypeNum; i++)
		{
			if(cl == (*g_pLocalComType)[i]->tkTypeDef)
			{
				mdToken tkImpl;
				(*g_pLocalComType)[i]->tkTypeDef = mdTokenNil; // to avoid printing ".class 0x1234ABCD"
				tkImpl = (*g_pLocalComType)[i]->tkImplementation;
				(*g_pLocalComType)[i]->tkImplementation = mdTokenNil; // to avoid printing implementation
						sprintf(szString,"%s.export ",g_szAsmCodeIndent);
				DumpComType((*g_pLocalComType)[i],szString,GUICookie);
				(*g_pLocalComType)[i]->tkTypeDef = cl;
				(*g_pLocalComType)[i]->tkImplementation = tkImpl;
			}
		}
	}

    // Dump method impls declared in this class whose implementing methods belong somewhere else:
	if(WhatToDump & 4) // 4 - dump members
	{
		for(i = 0; i < g_NumMI; i++)
		{
			if(((*g_pmi_list)[i].tkClass == cl)&&((*g_pmi_list)[i].tkBodyParent != cl))
			{
				const char *    pszMemberName;
				mdToken         tkDeclParent;
				szptr = &szString[0];
				szptr+=sprintf(szptr,"%s.override ",g_szAsmCodeIndent);

				if(TypeFromToken((*g_pmi_list)[i].tkDecl) == mdtMethodDef)
					pszMemberName = g_pImport->GetNameOfMethodDef((*g_pmi_list)[i].tkDecl);
				else 
				{
					PCCOR_SIGNATURE pComSig;
					ULONG       cComSig;

					pszMemberName = g_pImport->GetNameAndSigOfMemberRef((*g_pmi_list)[i].tkDecl,
						&pComSig,
						&cComSig
					);
				}
				MAKE_NAME_IF_NONE(pszMemberName,(*g_pmi_list)[i].tkDecl);

				if(FAILED(g_pImport->GetParentToken((*g_pmi_list)[i].tkDecl,&tkDeclParent))) continue;
				if(TypeFromToken(tkDeclParent) == mdtMethodDef) //get the parent's parent
				{
					mdTypeRef cr1;
					if(FAILED(g_pImport->GetParentToken(tkDeclParent,&cr1))) cr1 = mdTypeRefNil;
					tkDeclParent = cr1;
				}
				if(RidFromToken(tkDeclParent))
				{
					CQuickBytes     out;
					szptr+=sprintf(szptr,"%s::",PrettyPrintClass(&out,tkDeclParent,g_pImport));
				}
				szptr+=sprintf(szptr,"%s with ",ProperName((char*)pszMemberName));
				if (TypeFromToken((*g_pmi_list)[i].tkBody) == mdtMethodDef)
					PrettyPrintMethodDef(szString,(*g_pmi_list)[i].tkBody,g_pImport,GUICookie);
				else
				{
					_ASSERTE(TypeFromToken((*g_pmi_list)[i].tkBody) == mdtMemberRef);
					PrettyPrintMemberRef(szString,(*g_pmi_list)[i].tkBody,g_pImport,GUICookie);
				}
				printLine(GUICookie,szString);
			}
		}
	}
	delete [] szString; //free(szString); 
    if(WhatToDump & 2) // nested classes
    {
        BOOL    fRegetClassLayout=FALSE;
        for(i = 0; i < g_NumClasses; i++)
        {
            if(g_cl_enclosing[i] == cl)
            {
                DumpClass(g_cl_list[i],dwEntryPointToken,GUICookie,WhatToDump);
                if(g_fAbortDisassembly)
				{
					return FALSE;
				}
                fRegetClassLayout = TRUE;
            }
        }
        if(fRegetClassLayout) GetClassLayout(cl,&ulPackSize,&ulClassSize);
    }

	if(WhatToDump & 4)
	{
		DumpMembers(cl, pszNamespace, pszClassName, dwClassAttrs, dwEntryPointToken,GUICookie);
	}

	szString = new char[4096]; //(char*)malloc(4096);
    if(g_szAsmCodeIndent[0]) g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
    sprintf(szString,"%s} // end of class %s",g_szAsmCodeIndent, ProperName(pszClassName));
    printLine(GUICookie,szString);
	delete [] szString; //free(szString); 
    {
        printLine(GUICookie,"");
    }
    return TRUE;
}



void DumpGlobalMethods(DWORD dwEntryPointToken)
{
    BOOL            fPrintedAny = FALSE;
    HENUMInternal   hEnumMethod;
    mdToken         FuncToken;
    DWORD           i;
    CQuickBytes     qbMemberSig;

    if (FAILED(g_pImport->EnumGlobalFunctionsInit(&hEnumMethod)))
        return;

    for (i = 0; g_pImport->EnumNext(&hEnumMethod, &FuncToken); i++)
    {
        if (fPrintedAny == FALSE)
        {
            fPrintedAny = TRUE;

            printLine(g_pFile,"//Global methods");
            printLine(g_pFile,"//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        }
        if(DumpMethod(FuncToken, NULL, dwEntryPointToken, g_pFile, TRUE)&&
            (g_Mode == MODE_DUMP_CLASS_METHOD || g_Mode == MODE_DUMP_CLASS_METHOD_SIG)) break;

    }
    g_pImport->EnumClose(&hEnumMethod);
}

void DumpGlobalFields()
{
    BOOL            fPrintedAny = FALSE;
    HENUMInternal   hEnum;
    mdToken         FieldToken;
    DWORD           i;
    CQuickBytes     qbMemberSig;

    if (FAILED(g_pImport->EnumGlobalFieldsInit(&hEnum)))
        return;

    for (i = 0; g_pImport->EnumNext(&hEnum, &FieldToken); i++)
    {
        if (fPrintedAny == FALSE)
        {
            fPrintedAny = TRUE;

            printLine(g_pFile,"//Global fields");
            printLine(g_pFile,"//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        }
        if(DumpField(FieldToken, NULL, g_pFile, TRUE)&&
            (g_Mode == MODE_DUMP_CLASS_METHOD || g_Mode == MODE_DUMP_CLASS_METHOD_SIG)) break;
    }
    g_pImport->EnumClose(&hEnum);
}

extern COR_ILMETHOD_SECT_EH_CLAUSE_FAT* g_ehInfo; // declared in dis.cpp
extern ULONG    g_ehCount;                  // declared in dis.cpp

void    DumpExceptions(IMDInternalImport *      pImport, 
                       const BYTE *             exceptions)
{
    const COR_ILMETHOD_SECT_EH* eh = (const COR_ILMETHOD_SECT_EH*)exceptions;
    COR_ILMETHOD_SECT_EH_CLAUSE_FAT ehBuff;
    const COR_ILMETHOD_SECT_EH_CLAUSE_FAT* ehInfo;
    if(g_ehInfo) delete g_ehInfo;
    g_ehInfo = NULL;
    g_ehCount = 0;
    if(!eh) return;
    if((g_ehCount = eh->EHCount()))
    {
        g_ehInfo = new COR_ILMETHOD_SECT_EH_CLAUSE_FAT[g_ehCount];
        _ASSERTE(g_ehInfo != NULL);
        for (unsigned i = 0; i < g_ehCount; i++)  
        {   
            ehInfo = eh->EHClause(i, &ehBuff);
            memcpy(&g_ehInfo[i],ehInfo,sizeof(COR_ILMETHOD_SECT_EH_CLAUSE_FAT));
            g_ehInfo[i].SetFlags((CorExceptionFlag)((int)g_ehInfo[i].GetFlags() | NEW_TRY_BLOCK));
            _ASSERTE((ehInfo->GetFlags() & SEH_NEW_PUT_MASK) == 0); // we are using 0x80000000 and 0x40000000
        }
            printLine(g_pFile,"/*----------------------------");
        dumpEHInfo(pImport, g_pFile);
            printLine(g_pFile,"/*----------------------------");

    }
}


void DumpNativeInfo(const BYTE * ipmap, DWORD IPMapSize)
{
    const IMAGE_COR_X86_RUNTIME_FUNCTION_ENTRY * pIPMap 
      = (const IMAGE_COR_X86_RUNTIME_FUNCTION_ENTRY *) ipmap;

    DWORD   dwNumEntries = (IPMapSize / sizeof(*pIPMap));
    char szString[1024];

    if (IPMapSize)
    {
        printLine(g_pFile,"// GC/EH info for managed native functions");
        printLine(g_pFile,"//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    }

    for(DWORD count=0; count<dwNumEntries; count++)
    {
        DWORD   dwMethodRVA = VAL32(pIPMap[count].BeginAddress);
        DWORD   dwMethodEndRVA = VAL32(pIPMap[count].EndAddress);

        sprintf(szString,"// IPMap[0x%x]", count);
        printLine(g_pFile,szString);
        printLine(g_pFile,"//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~n");
        sprintf(szString,"// Method at RVA 0x%x-0x%x", dwMethodRVA, dwMethodEndRVA);
        printLine(g_pFile,szString);

        const IMAGE_COR_MIH_ENTRY *   pMIHEntry;

        g_pPELoader->getVAforRVA(VAL32(pIPMap[count].MIH), (void **) &pMIHEntry);  
        pMIHEntry = (const IMAGE_COR_MIH_ENTRY *)(((BYTE *) pMIHEntry) - offsetof(IMAGE_COR_MIH_ENTRY,Flags));  

        /*---------------------------------------------------------------------
         * Exception info
         */

        if (pMIHEntry->Flags & IMAGE_COR_MIH_EHRVA)
        {
            unsigned    EH_RVA  = VAL32(pMIHEntry->EHRVA);
    
            if (!EH_RVA)
            {
                printLine(g_pFile,"// Exception count 0");
            }
            else
            {
                BYTE *EH_VA;    
                g_pPELoader->getVAforRVA(EH_RVA, (void **) &EH_VA); 
                DumpExceptions(g_pImport, EH_VA);
            }
        }
    }
}



void DumpVTables(IMAGE_COR20_HEADER *CORHeader, void* GUICookie)
{
    IMAGE_COR_VTABLEFIXUP *pFixup;
    DWORD       iCount;
    DWORD       i;
    USHORT      iSlot;
    char szString[1024];
    char* szStr = &szString[0];
    
	if (VAL32(CORHeader->VTableFixups.VirtualAddress) == 0) return;

    
    sprintf(szString,"// VTableFixup Directory:");
    printLine(GUICookie,szStr);    

    // Pull back a pointer to the guy.
    iCount = VAL32(CORHeader->VTableFixups.Size) / sizeof(IMAGE_COR_VTABLEFIXUP);
    if (g_pPELoader->getVAforRVA(VAL32(CORHeader->VTableFixups.VirtualAddress), (void **) &pFixup) == FALSE)
    {
        printLine(GUICookie,RstrA(IDS_E_VTFUTABLE));    
        goto exit;
    }

    // Walk every v-table fixup entry and dump the slots.
    for (i=0;  i<iCount;  i++)
    {
        sprintf(szString,"//   IMAGE_COR_VTABLEFIXUP[%d]:", i);
        printLine(GUICookie,szStr);    
        sprintf(szString,"//       RVA:               %08x", VAL32(pFixup->RVA));
        printLine(GUICookie,szStr);    
        sprintf(szString,"//       Count:             %04x", VAL16(pFixup->Count));
        printLine(GUICookie,szStr);    
        sprintf(szString,"//       Type:              %04x", VAL16(pFixup->Type));
        printLine(GUICookie,szStr);    

        BYTE *pSlot;
        if (g_pPELoader->getVAforRVA(VAL32(pFixup->RVA), (void **) &pSlot) == FALSE)
        {
            printLine(GUICookie,RstrA(IDS_E_BOGUSRVA));    
            goto NextEntry;
        }

        for (iSlot=0;  iSlot<pFixup->Count;  iSlot++)
        {
            mdMethodDef tkMethod = VAL32(*(DWORD *) pSlot);
            if ((VAL16(pFixup->Type) & COR_VTABLE_32BIT) == COR_VTABLE_32BIT)
            {
                sprintf(szString,"//         [%04x]            (%08x)", iSlot, tkMethod);
                pSlot += sizeof(DWORD);
            }
            else
            {
                sprintf(szString,"//         [%04x]            (%16x)", iSlot, VAL64(*(unsigned __int64 *) pSlot));
                pSlot += sizeof(unsigned __int64);
            }
            printLine(GUICookie,szStr);    

            ValidateToken(tkMethod, mdtMethodDef);
        }

        // Pointer to next fixup entry.
NextEntry:        
        ++pFixup;
    }

exit:
    printLine(GUICookie,"");    
}


void DumpEATTable(IMAGE_COR20_HEADER *CORHeader, void* GUICookie)
{
    BYTE        *pFixup;
    DWORD       iCount;
    DWORD       BufferRVA;
    DWORD       i;
    char szString[1024];
    char* szStr = &szString[0];
    
    sprintf(szString,"// Export Address Table Jumps:");
    printLine(GUICookie,szStr);    
    
    if (VAL32(CORHeader->ExportAddressTableJumps.VirtualAddress) == 0)
    {
        printLine(GUICookie,RstrA(IDS_E_NODATA));    
        return;
    }

    // Pull back a pointer to the guy.
    iCount = VAL32(CORHeader->ExportAddressTableJumps.Size) / IMAGE_COR_EATJ_THUNK_SIZE;
    if (g_pPELoader->getVAforRVA(VAL32(CORHeader->ExportAddressTableJumps.VirtualAddress), (void **) &pFixup) == FALSE)
    {
        printLine(GUICookie,RstrA(IDS_E_EATJTABLE));    
        goto exit;
    }

    // Quick sanity check on the linker.
    if (VAL32(CORHeader->ExportAddressTableJumps.Size) % IMAGE_COR_EATJ_THUNK_SIZE)
    {
        sprintf(szString,RstrA(IDS_E_EATJSIZE),
                VAL32(CORHeader->ExportAddressTableJumps.Size), IMAGE_COR_EATJ_THUNK_SIZE);
        printLine(GUICookie,szStr);    
    }

    // Walk every v-table fixup entry and dump the slots.
    BufferRVA = VAL32(CORHeader->ExportAddressTableJumps.VirtualAddress);
    for (i=0;  i<iCount;  i++)
    {
        ULONG ReservedFlag = VAL32(*(ULONG *) (pFixup + sizeof(ULONG)));
        sprintf(szString,"//   Fixup Jump Entry [%d], at RVA %08x:", i, BufferRVA);
        printLine(GUICookie,szStr);    
        sprintf(szString,"//       RVA of slot:       %08x", VAL32(*(ULONG *) pFixup));
        printLine(GUICookie,szStr);    
        sprintf(szString,"//       Reserved flag:     %08x", ReservedFlag);
        printLine(GUICookie,szStr);    
        if (ReservedFlag != 0)
        {
            printLine(GUICookie,RstrA(IDS_E_RESFLAGS));    
        }

        pFixup += IMAGE_COR_EATJ_THUNK_SIZE;
        BufferRVA += IMAGE_COR_EATJ_THUNK_SIZE;
    }

exit:
    printLine(GUICookie,"");    
}


void DumpCodeManager(IMAGE_COR20_HEADER *CORHeader, void* GUICookie)
{
    char szString[1024];
    char* szStr = &szString[0];

    sprintf(szString,"// Code Manager Table:");
    printLine(GUICookie,szStr);
    if (!VAL32(CORHeader->CodeManagerTable.Size))
    {        
        sprintf(szString,"//  default");
        printLine(GUICookie,szStr);
        return;
    }

    const GUID *pcm;
    if (g_pPELoader->getVAforRVA(VAL32(CORHeader->CodeManagerTable.VirtualAddress), (void **) &pcm) == FALSE)
    {
        printLine(GUICookie,RstrA(IDS_E_CODEMGRTBL));
        return;
    }

    sprintf(szString,"//   [index]       ID");
    printLine(GUICookie,szStr);
    ULONG iCount = VAL32(CORHeader->CodeManagerTable.Size) / sizeof(GUID);
    for (ULONG i=0;  i<iCount;  i++)
    {
        WCHAR        rcguid[128];
        GUID         Guid = *pcm;
        SwapGuid(&Guid);
        StringFromGUID2(Guid, rcguid, NumItems(rcguid));
        sprintf(szString,"//   [%08x]    %S", i, rcguid);
        printLine(GUICookie,szStr);
        pcm++;
    }
    printLine(GUICookie,"");
}

void DumpIAT(const char *szName, IMAGE_DATA_DIRECTORY *pDir, void* GUICookie)
{
    char szString[1024];
    char* szStr = &szString[0];
    sprintf(szString,"// %s", szName);
    printLine(GUICookie,szStr);
    if (!VAL32(pDir->Size))
    {
        printLine(GUICookie,RstrA(IDS_E_NODATA));
        return;
    }

    const char *szDLLName;
    const IMAGE_IMPORT_DESCRIPTOR *pImportDesc;
    
    if (g_pPELoader->getVAforRVA(VAL32(pDir->VirtualAddress), (void **) &pImportDesc) == FALSE)
    {
        printLine(GUICookie,RstrA(IDS_E_IMPORTDATA));
        return;
    }

    const DWORD *pImportTableID;
    while (VAL32(pImportDesc->FirstThunk))
    {
        if (g_pPELoader->getVAforRVA(VAL32(pImportDesc->Name), (void **) &szDLLName) == FALSE ||
            g_pPELoader->getVAforRVA(VAL32(pImportDesc->FirstThunk), (void **) &pImportTableID) == FALSE)
        {
            printLine(GUICookie,RstrA(IDS_E_IMPORTDATA));
            return;
        }
    
        sprintf(szString,"//     %s", szDLLName);
        printLine(GUICookie,szStr);
        sprintf(szString,"//              %08x Import Address Table", VAL32(pImportDesc->FirstThunk));
        printLine(GUICookie,szStr);
        sprintf(szString,"//              %08x Import Name Table", VAL32(pImportDesc->Name));
        printLine(GUICookie,szStr);
        sprintf(szString,"//              %-8d time date stamp", VAL32(pImportDesc->TimeDateStamp));
        printLine(GUICookie,szStr);
        sprintf(szString,"//              %-8d Index of first forwarder reference", VAL32(pImportDesc->ForwarderChain));
        printLine(GUICookie,szStr);
        sprintf(szString,"//");
        printLine(GUICookie,szStr);
        
        for ( ; VAL32(*pImportTableID);  pImportTableID++)
        {
            const IMAGE_IMPORT_BY_NAME *pName;
            if(g_pPELoader->getVAforRVA(VAL32(*pImportTableID )& 0x7fffffff, (void **) &pName))
			{
				if (VAL32(*pImportTableID) & 0x80000000)
					sprintf(szString,"//             %8x  by ordinal %d", VAL16(pName->Hint), (VAL32(*pImportTableID) & 0x7fffffff));
				else
					sprintf(szString,"//             %8x  %s", VAL16(pName->Hint), pName->Name);
				printLine(GUICookie,szStr);
			}
			else
			{
				printLine(GUICookie,RstrA(IDS_E_IMPORTDATA));
				break;
			}
       }
       printLine(GUICookie,"");
    
        // Next import descriptor.
        pImportDesc++;
    }
}


#define DUMP_DIRECTORY(szName, Directory) \
    sprintf(szString,"// %-8x [%-8x] address [size] of " szName,  \
            VAL32(Directory.VirtualAddress), VAL32(Directory.Size)); \
    printLine(GUICookie,szStr)

void DumpHeader(IMAGE_COR20_HEADER *CORHeader, void* GUICookie)
{
    char szString[1024];
    char* szStr = &szString[0];
    sprintf(szString,"// PE Header:");
    printLine(GUICookie,szStr);

	if (g_pPELoader->IsPE32())
	{
		IMAGE_NT_HEADERS32 *pNTHeader = g_pPELoader->ntHeaders32();
		IMAGE_OPTIONAL_HEADER32 *pOptHeader = &pNTHeader->OptionalHeader;

		sprintf(szString,"// Subsystem:                      %08x", VAL16(pOptHeader->Subsystem));
		printLine(GUICookie,szStr);
		sprintf(szString,"// Native entry point address:     %08x", VAL32(pOptHeader->AddressOfEntryPoint));
		printLine(GUICookie,szStr);
		sprintf(szString,"// Image base:                     %08x", VAL32(pOptHeader->ImageBase));
		printLine(GUICookie,szStr);
		sprintf(szString,"// Section alignment:              %08x", VAL32(pOptHeader->SectionAlignment));
		printLine(GUICookie,szStr);
		sprintf(szString,"// File alignment:                 %08x", VAL32(pOptHeader->FileAlignment));
		printLine(GUICookie,szStr);
		sprintf(szString,"// Stack reserve size:             %08x", VAL32(pOptHeader->SizeOfStackReserve));
		printLine(GUICookie,szStr);
		sprintf(szString,"// Stack commit size:              %08x", VAL32(pOptHeader->SizeOfStackCommit));
		printLine(GUICookie,szStr);
		sprintf(szString,"// Directories:                    %08x", VAL32(pOptHeader->NumberOfRvaAndSizes));
		printLine(GUICookie,szStr);
		DUMP_DIRECTORY("Export Directory:          ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
		DUMP_DIRECTORY("Import Directory:          ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]);
		DUMP_DIRECTORY("Resource Directory:        ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE]);
		DUMP_DIRECTORY("Exception Directory:       ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION]);
		DUMP_DIRECTORY("Security Directory:        ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]);
		DUMP_DIRECTORY("Base Relocation Table:     ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]);
		DUMP_DIRECTORY("Debug Directory:           ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG]);
		DUMP_DIRECTORY("Architecture Specific:     ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_ARCHITECTURE]);
		DUMP_DIRECTORY("Global Pointer:            ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_GLOBALPTR]);
		DUMP_DIRECTORY("TLS Directory:             ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS]);
		DUMP_DIRECTORY("Load Config Directory:     ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG]);
		DUMP_DIRECTORY("Bound Import Directory:    ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT]);
		DUMP_DIRECTORY("Import Address Table:      ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT]);
		DUMP_DIRECTORY("Delay Load IAT:            ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT]);
		DUMP_DIRECTORY("CLR Header:               ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR]);
		printLine(GUICookie,"");

		DumpIAT("Import Address Table", &pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT],GUICookie);
		DumpIAT("Delay Load Import Address Table", &pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT],GUICookie);
	}
	else
	{
		IMAGE_NT_HEADERS64 *pNTHeader = g_pPELoader->ntHeaders64();
		IMAGE_OPTIONAL_HEADER64 *pOptHeader = &pNTHeader->OptionalHeader;
    
    sprintf(szString,"// Subsystem:                      %08x", VAL16(pOptHeader->Subsystem));
    printLine(GUICookie,szStr);
    sprintf(szString,"// Native entry point address:     %08x", VAL32(pOptHeader->AddressOfEntryPoint));
    printLine(GUICookie,szStr);
    sprintf(szString,"// Image base:                     %08x", VAL64(pOptHeader->ImageBase));
    printLine(GUICookie,szStr);
    sprintf(szString,"// Section alignment:              %08x", VAL32(pOptHeader->SectionAlignment));
    printLine(GUICookie,szStr);
    sprintf(szString,"// File alignment:                 %08x", VAL32(pOptHeader->FileAlignment));
    printLine(GUICookie,szStr);
    sprintf(szString,"// Stack reserve size:             %08x", VAL64(pOptHeader->SizeOfStackReserve));
    printLine(GUICookie,szStr);
    sprintf(szString,"// Stack commit size:              %08x", VAL64(pOptHeader->SizeOfStackCommit));
    printLine(GUICookie,szStr);
    sprintf(szString,"// Directories:                    %08x", VAL32(pOptHeader->NumberOfRvaAndSizes));
    printLine(GUICookie,szStr);
    DUMP_DIRECTORY("Export Directory:          ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    DUMP_DIRECTORY("Import Directory:          ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]);
    DUMP_DIRECTORY("Resource Directory:        ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE]);
    DUMP_DIRECTORY("Exception Directory:       ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION]);
    DUMP_DIRECTORY("Security Directory:        ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]);
    DUMP_DIRECTORY("Base Relocation Table:     ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]);
    DUMP_DIRECTORY("Debug Directory:           ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG]);
    DUMP_DIRECTORY("Architecture Specific:     ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_ARCHITECTURE]);
    DUMP_DIRECTORY("Global Pointer:            ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_GLOBALPTR]);
    DUMP_DIRECTORY("TLS Directory:             ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS]);
    DUMP_DIRECTORY("Load Config Directory:     ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG]);
    DUMP_DIRECTORY("Bound Import Directory:    ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT]);
    DUMP_DIRECTORY("Import Address Table:      ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT]);
    DUMP_DIRECTORY("Delay Load IAT:            ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT]);
    DUMP_DIRECTORY("CLR Header:               ", pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR]);
    printLine(GUICookie,"");

    DumpIAT("Import Address Table", &pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT],GUICookie);
    DumpIAT("Delay Load Import Address Table", &pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT],GUICookie);
	}

    if (!CORHeader)
    {
        printLine(GUICookie,RstrA(IDS_E_COMIMAGE));
        return;
    }
    
    sprintf(szString,"// CLR Header:");
    printLine(GUICookie,szStr);
    sprintf(szString,"// %-8d Header Size", VAL32(CORHeader->cb));
    printLine(GUICookie,szStr);
    sprintf(szString,"// %-8d Major Runtime Version", VAL16(CORHeader->MajorRuntimeVersion));
    printLine(GUICookie,szStr);
    sprintf(szString,"// %-8d Minor Runtime Version", VAL16(CORHeader->MinorRuntimeVersion));
    printLine(GUICookie,szStr);
    sprintf(szString,"// %-8x Flags", VAL32(CORHeader->Flags));
    printLine(GUICookie,szStr);
    sprintf(szString,"// %-8x Entrypoint Token", VAL32(CORHeader->EntryPointToken));
    printLine(GUICookie,szStr);

    // Metadata
    DUMP_DIRECTORY("Metadata Directory:        ", CORHeader->MetaData);

    // Binding
    DUMP_DIRECTORY("Resources Directory:       ", CORHeader->Resources);
    DUMP_DIRECTORY("Strong Name Signature:     ", CORHeader->StrongNameSignature);
    DUMP_DIRECTORY("CodeManager Table:         ", CORHeader->CodeManagerTable);

    // Fixups
    DUMP_DIRECTORY("VTableFixups Directory:    ", CORHeader->VTableFixups);
    DUMP_DIRECTORY("Export Address Table:      ", CORHeader->ExportAddressTableJumps);
    
    // Managed Native Code
    DUMP_DIRECTORY("Precompile Header:         ", CORHeader->ManagedNativeHeader);

}


void DumpHeaderDetails(IMAGE_COR20_HEADER *CORHeader, void* GUICookie)
{
    DumpCodeManager(CORHeader,GUICookie);
    DumpVTables(CORHeader,GUICookie);
    DumpEATTable(CORHeader,GUICookie);
    printLine(GUICookie,"");
}


void WritePerfData(const char *KeyDesc, const char *KeyName, const char *UnitDesc, const char *UnitName, void* Value, BOOL IsInt) 
{

    DWORD BytesWritten;

    if(!g_fDumpToPerfWriter) return;

    if (!g_PerfDataFilePtr)
    {
#if !defined(PLATFORM_UNIX)
        if((g_PerfDataFilePtr = CreateFile("c:\\temp\\perfdata.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL) ) == INVALID_HANDLE_VALUE)
#else
        if((g_PerfDataFilePtr = CreateFile("/tmp/perfdata.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL) ) == INVALID_HANDLE_VALUE)
#endif 
        {
         printLine(NULL,"PefTimer::LogStoppedTime(): Unable to open the FullPath file. No performance data will be generated");
         g_fDumpToPerfWriter = FALSE;
         return;
        }
        WriteFile(g_PerfDataFilePtr,"ExecTime=0\r\n",14,&BytesWritten,NULL);
        WriteFile(g_PerfDataFilePtr,"ExecUnit=bytes\r\n",18,&BytesWritten,NULL);
        WriteFile(g_PerfDataFilePtr,"ExecUnitDescr=File Size\r\n",27,&BytesWritten,NULL);
        WriteFile(g_PerfDataFilePtr,"ExeciDirection=False\r\n",24,&BytesWritten,NULL);
    }

    char ValueStr[10];
    char TmpStr[201];

    if (IsInt)
    {
        sprintf(ValueStr,"%d",(int)*(int*)Value);
    }
    else
    {
        sprintf(ValueStr,"%5.2f",(float)*(float*)Value);
    }
    sprintf(TmpStr, "%s=%s\r\n", KeyName, ValueStr);
    WriteFile(g_PerfDataFilePtr, TmpStr, (DWORD)strlen(TmpStr), &BytesWritten, NULL);

    sprintf(TmpStr, "%s Descr=%s\r\n", KeyName, KeyDesc);
    WriteFile(g_PerfDataFilePtr, TmpStr, (DWORD)strlen(TmpStr), &BytesWritten, NULL);

    sprintf(TmpStr, "%s Unit=%s\r\n", KeyName, UnitName);
    WriteFile(g_PerfDataFilePtr, TmpStr, (DWORD)strlen(TmpStr), &BytesWritten, NULL);

    sprintf(TmpStr, "%s Unit Descr=%s\r\n", KeyName, UnitDesc);
    WriteFile(g_PerfDataFilePtr, TmpStr, (DWORD)strlen(TmpStr), &BytesWritten, NULL);

    sprintf(TmpStr, "%s IDirection=%s\r\n", KeyName, "False");
    WriteFile(g_PerfDataFilePtr, TmpStr, (DWORD)strlen(TmpStr), &BytesWritten, NULL);
}

void WritePerfDataInt(const char *KeyDesc, const char *KeyName, const char *UnitDesc, const char *UnitName, int Value)
{
    WritePerfData(KeyDesc,KeyName,UnitDesc,UnitName, (void*)&Value, TRUE);
}
void WritePerfDataFloat(const char *KeyDesc, const char *KeyName, const char *UnitDesc, const char *UnitName, float Value)
{
    WritePerfData(KeyDesc,KeyName,UnitDesc,UnitName, (void*)&Value, FALSE); 
}


IMetaDataTables *pITables = NULL;
ULONG sizeRec, count;
int   size, size2;
int   metaSize = 0;
__int64 fTableSeen;
inline void TableSeen(unsigned long n) { fTableSeen |= (I64(1) << n); }
inline int IsTableSeen(unsigned long n) { return (fTableSeen & (I64(1) << n)) ? 1 : 0;}
inline void TableSeenReset() { fTableSeen = 0;}

void DumpTable(unsigned long Table, const char *TableName, void* GUICookie) 
{
    char szString[1024];
    char *szStr = &szString[0];
    const char **ppTableName = 0;

    // Record that this table has been seen.
    TableSeen(Table);
    
    // If no name passed in, get from table info.
    if (!TableName)
        ppTableName = &TableName;
    
    pITables->GetTableInfo(Table, &sizeRec, &count, NULL, NULL, ppTableName);
    if(count > 0) 
    {
        metaSize += size = count * sizeRec;                                   
        WritePerfDataInt(TableName,TableName,"count","count",count);
        WritePerfDataInt(TableName,TableName,"bytes","bytes",size);
        sprintf(szString,"//   %-14s- %4d (%d bytes)", TableName, count, size);
        printLine(GUICookie,szStr);
    }
}


   
void DumpStatistics(IMAGE_COR20_HEADER *CORHeader, void* GUICookie) 
{

   int    fileSize, miscPESize, miscCOMPlusSize, methodHeaderSize, methodBodySize;
   int    methodBodies, fatHeaders, tinyHeaders, deprecatedHeaders;
   int    size, size2;
   int    fatSections, smallSections;
   ULONG  methodDefs;
   ULONG  i;
   ULONG  sizeRec, count;
   char   buf[MAX_MEMBER_LENGTH];
    char szString[1024];
    char* szStr = &szString[0];

    TableSeenReset();
    metaSize = 0;

   sprintf(szString,"// File size            : %d", fileSize = SafeGetFileSize(g_pPELoader->getHFile(), NULL));
    printLine(GUICookie,szStr);

   WritePerfDataInt("FileSize","FileSize","standard byte","bytes",fileSize);

   if (g_pPELoader->IsPE32())
   {
       size = VAL32(((IMAGE_DOS_HEADER*) g_pPELoader->getHModule())->e_lfanew) +
            sizeof(IMAGE_NT_HEADERS) - sizeof(IMAGE_OPTIONAL_HEADER32) +
				VAL16(g_pPELoader->ntHeaders32()->FileHeader.SizeOfOptionalHeader) +
				VAL16(g_pPELoader->ntHeaders32()->FileHeader.NumberOfSections) * sizeof(IMAGE_SECTION_HEADER);
	   size2 = (size + VAL32(g_pPELoader->ntHeaders32()->OptionalHeader.FileAlignment) - 1) & ~(VAL32(g_pPELoader->ntHeaders32()->OptionalHeader.FileAlignment) - 1);
   }
   else
   {
	   size = VAL32(((IMAGE_DOS_HEADER*) g_pPELoader->getHModule())->e_lfanew) +
				sizeof(IMAGE_NT_HEADERS) - sizeof(IMAGE_OPTIONAL_HEADER64) +
				VAL16(g_pPELoader->ntHeaders64()->FileHeader.SizeOfOptionalHeader) +
				VAL16(g_pPELoader->ntHeaders64()->FileHeader.NumberOfSections) * sizeof(IMAGE_SECTION_HEADER);
	   size2 = (size + VAL32(g_pPELoader->ntHeaders64()->OptionalHeader.FileAlignment) - 1) & ~(VAL32(g_pPELoader->ntHeaders64()->OptionalHeader.FileAlignment) - 1);
   }

    if (g_pPELoader->IsPE32())
    {
        DWORD   SizeOfHeaders = VAL32(g_pPELoader->ntHeaders32()->OptionalHeader.SizeOfHeaders);

        WritePerfDataInt("PE header size", "PE header size", "standard byte", "bytes", SizeOfHeaders);
        WritePerfDataInt("PE header size used", "PE header size used", "standard byte", "bytes", size);
        WritePerfDataFloat("PE header size", "PE header size", "percentage", "percentage", (float) ((SizeOfHeaders * 100) / fileSize));
        sprintf(szString,"// PE header size       : %d (%d used)    (%5.2f%%)", 
	            SizeOfHeaders, size,  
	            (double) (SizeOfHeaders * 100) / fileSize);

        printLine(GUICookie,szStr);
        miscPESize = 0;

        for (i=0; i < VAL32(g_pPELoader->ntHeaders32()->OptionalHeader.NumberOfRvaAndSizes); ++i)
        {
            // Skip the CLR header.
	        if (i != 15) miscPESize += (int) VAL32(g_pPELoader->ntHeaders32()->OptionalHeader.DataDirectory[i].Size);
        }
    }
    else
    {
        DWORD   SizeOfHeaders = VAL32(g_pPELoader->ntHeaders64()->OptionalHeader.SizeOfHeaders);

        WritePerfDataInt("PE+ header size", "PE header size", "standard byte", "bytes", SizeOfHeaders);
        WritePerfDataInt("PE+ header size used", "PE header size used", "standard byte", "bytes", size);
        WritePerfDataFloat("PE+ header size", "PE header size", "percentage", "percentage", (float) ((SizeOfHeaders * 100) / fileSize));

        sprintf(szString,"// PE header size       : %d (%d used)    (%5.2f%%)", 
		  SizeOfHeaders, size,  
		  (double) (SizeOfHeaders * 100) / fileSize);

        printLine(GUICookie,szStr);
	    miscPESize = 0;

        for (i=0; i < VAL32(g_pPELoader->ntHeaders64()->OptionalHeader.NumberOfRvaAndSizes); ++i) 
	    {
            // Skip the CLR header.
		    if (i != IMAGE_DIRECTORY_ENTRY_COMHEADER) miscPESize += (int) VAL32(g_pPELoader->ntHeaders64()->OptionalHeader.DataDirectory[i].Size);
	   }
    }

    WritePerfDataInt("PE additional info", "PE additional info", "standard byte", "bytes",miscPESize);
    WritePerfDataFloat("PE additional info", "PE additional info", "percentage", "percent", (float) ((miscPESize * 100) / fileSize));

    sprintf(buf, "PE additional info   : %d", miscPESize);
    sprintf(szString,"// %-40s (%5.2f%%)", buf, (double) (miscPESize * 100) / fileSize);
    printLine(GUICookie,szStr);

    WORD    NumberOfSections;
    if (g_pPELoader->IsPE32())
    {
        NumberOfSections = VAL16(g_pPELoader->ntHeaders32()->FileHeader.NumberOfSections);
    }
    else
    {
        NumberOfSections = VAL16(g_pPELoader->ntHeaders64()->FileHeader.NumberOfSections);
    }

    WritePerfDataInt("Num.of PE sections", "Num.of PE sections", "Nbr of sections", "sections",VAL16(g_pPELoader->ntHeaders32()->FileHeader.NumberOfSections));
    sprintf(szString,"// Num.of PE sections   : %d", NumberOfSections);

    printLine(GUICookie,szStr);

    WritePerfDataInt("CLR header size", "CLR header size", "byte", "bytes",VAL32(CORHeader->cb));
    WritePerfDataFloat("CLR header size", "CLR header size", "percentage", "percent",(float) ((VAL32(CORHeader->cb) * 100) / fileSize));

    sprintf(buf, "CLR header size     : %d", VAL32(CORHeader->cb));
    sprintf(szString,"// %-40s (%5.2f%%)", buf, (double) (VAL32(CORHeader->cb) * 100) / fileSize);
    printLine(GUICookie,szStr);

    DWORD dwMetaSize = VAL32(CORHeader->MetaData.Size);
    WritePerfDataInt("CLR meta-data size", "CLR meta-data size", "bytes", "bytes",dwMetaSize);
    WritePerfDataFloat("CLR meta-data size", "CLR meta-data size", "percentage", "percent",(float) ((dwMetaSize * 100) / fileSize));

    sprintf(buf, "CLR meta-data size  : %d", dwMetaSize);
    sprintf(szString,"// %-40s (%5.2f%%)", buf, (double) (dwMetaSize * 100) / fileSize);
    printLine(GUICookie,szStr);

    IMAGE_DATA_DIRECTORY *pFirst = &CORHeader->Resources;
    ULONG32 iCount = (ULONG32)((BYTE *) &CORHeader->ManagedNativeHeader - (BYTE *) &CORHeader->Resources) / sizeof(IMAGE_DATA_DIRECTORY) + 1;
    miscCOMPlusSize = 0;
    for (ULONG32 iDir=0;  iDir<iCount;  iDir++)
    {
        miscCOMPlusSize += VAL32(pFirst->Size);
        pFirst++;
    }

    WritePerfDataInt("CLR Additional info", "CLR Additional info", "bytes", "bytes",miscCOMPlusSize);
    WritePerfDataFloat("CLR Additional info", "CLR Additional info", "percentage", "percent",(float) ((miscCOMPlusSize * 100) / fileSize));

    sprintf(buf, "CLR additional info : %d", miscCOMPlusSize);
    sprintf(szString,"// %-40s (%5.2f%%)", buf, (double) (miscCOMPlusSize * 100) / fileSize);
    printLine(GUICookie,szStr);

    //  _ASSERTE(g_pImport->GetCountWithTokenKind(mdtMethodImpl) == 0);

    // Go through each method def collecting some statistics.
    methodHeaderSize = methodBodySize = 0;
    methodBodies = fatHeaders = tinyHeaders = deprecatedHeaders = fatSections = smallSections = 0;
    methodDefs = g_pImport->GetCountWithTokenKind(mdtMethodDef);
    for (i=1; i <= methodDefs; ++i) {
        ULONG   rva;
        DWORD   flags;

        g_pImport->GetMethodImplProps(TokenFromRid(i, mdtMethodDef), &rva, &flags);
        if ((rva != 0)&&(IsMiIL(flags) || IsMiOPTIL(flags)))	// We don't handle native yet.
        {
            ++methodBodies;

            COR_ILMETHOD_FAT *pMethod;
            g_pPELoader->getVAforRVA(rva, (void **) &pMethod);
            if (pMethod->IsFat())
            {
                ++fatHeaders;

                methodHeaderSize += pMethod->GetSize() * 4;
                methodBodySize += pMethod->GetCodeSize();

                // Add in the additional sections.
                BYTE *sectsBegin = (BYTE *) (pMethod->GetCode() + pMethod->GetCodeSize());
                const COR_ILMETHOD_SECT *pSect = pMethod->GetSect();
                const COR_ILMETHOD_SECT *pOldSect;
                if (pSect != NULL) {
                    // Keep skipping a pointer past each section.
                    do
                    {
                        pOldSect = pSect;
                        if (((COR_ILMETHOD_SECT_FAT *) pSect)->GetKind() & CorILMethod_Sect_FatFormat)
                        {
                            ++fatSections;
                            pSect = (COR_ILMETHOD_SECT *)((BYTE *) pSect + ((COR_ILMETHOD_SECT_FAT *) pSect)->GetDataSize());
                        }
                        else
                        {
                            ++smallSections;
                            pSect = (COR_ILMETHOD_SECT *)((BYTE *) pSect + ((COR_ILMETHOD_SECT_SMALL *) pSect)->DataSize);
                        }
                        pSect = (COR_ILMETHOD_SECT *) (((UINT_PTR) pSect + 3) & ~3);
                    }
                    while (pOldSect->More());

                    // Add on the section sizes.
                    methodHeaderSize += (int) ((BYTE *) pSect - sectsBegin);
                }
            }
            else if (((COR_ILMETHOD_TINY *) pMethod)->IsTiny())
            {
                ++tinyHeaders;
                methodHeaderSize += sizeof(COR_ILMETHOD_TINY);
                methodBodySize += ((COR_ILMETHOD_TINY *) pMethod)->GetCodeSize();
            }
            else
                _ASSERTE(!"Unrecognized header type");
        }
    }


    WritePerfDataInt("CLR method headers", "CLR method headers", "bytes", "bytes",methodHeaderSize);
    WritePerfDataFloat("CLR method headers", "CLR method headers", "percentage", "percent",(float) ((methodHeaderSize * 100) / fileSize));

    sprintf(buf, "CLR method headers  : %d", methodHeaderSize);
    sprintf(szString,"// %-40s (%5.2f%%)", buf, (double) (methodHeaderSize * 100) / fileSize);
    printLine(GUICookie,szStr);

    WritePerfDataInt("Managed code", "Managed code", "bytes", "bytes",methodBodySize);
    WritePerfDataFloat("Managed code", "Managed code", "percentage", "percent",(float) ((methodBodySize * 100) / fileSize));

    sprintf(buf, "Managed code         : %d", methodBodySize);
    sprintf(szString,"// %-40s (%5.2f%%)", buf, (double) (methodBodySize * 100) / fileSize);
    printLine(GUICookie,szStr);

    if (g_pPELoader->IsPE32())
    {
        DWORD   SizeOfInitializedData = VAL32(g_pPELoader->ntHeaders32()->OptionalHeader.SizeOfInitializedData);

	    WritePerfDataInt("Data", "Data", "bytes", "bytes",SizeOfInitializedData);
	    WritePerfDataFloat("Data", "Data", "percentage", "percent",(float) ((SizeOfInitializedData * 100) / fileSize));
    	   
	    sprintf(buf, "Data                 : %d", SizeOfInitializedData);
	    sprintf(szString,"// %-40s (%5.2f%%)", buf, (double) (SizeOfInitializedData * 100) / fileSize);
	    printLine(GUICookie,szStr);
    	   
	    size = fileSize - VAL32(g_pPELoader->ntHeaders32()->OptionalHeader.SizeOfHeaders) - miscPESize - VAL32(CORHeader->cb) -
			    VAL32(CORHeader->MetaData.Size) - miscCOMPlusSize - 
			    SizeOfInitializedData -
			    methodHeaderSize - methodBodySize;
   }
   else
   {
        DWORD   SizeOfInitializedData = VAL32(g_pPELoader->ntHeaders64()->OptionalHeader.SizeOfInitializedData);
	    WritePerfDataInt("Data", "Data", "bytes", "bytes",SizeOfInitializedData);
	    WritePerfDataFloat("Data", "Data", "percentage", "percent",(float) ((SizeOfInitializedData * 100) / fileSize));
       
	    sprintf(buf, "Data                 : %d", SizeOfInitializedData);
	    sprintf(szString,"// %-40s (%5.2f%%)", buf, (double) (SizeOfInitializedData * 100) / fileSize);
        printLine(GUICookie,szStr);
       
	    size = fileSize - VAL32(g_pPELoader->ntHeaders64()->OptionalHeader.SizeOfHeaders) - miscPESize - VAL32(CORHeader->cb) -
                VAL32(CORHeader->MetaData.Size) - miscCOMPlusSize - 
			    SizeOfInitializedData -
                methodHeaderSize - methodBodySize;
    }

    WritePerfDataInt("Unaccounted", "Unaccounted", "bytes", "bytes",size);
    WritePerfDataFloat("Unaccounted", "Unaccounted", "percentage", "percent",(float) ((size * 100) / fileSize));

    sprintf(buf, "Unaccounted          : %d", size);
    sprintf(szString,"// %-40s (%5.2f%%)", buf, (double) (size * 100) / fileSize);
    printLine(GUICookie,szStr);


    // Detail...
    if (g_pPELoader->IsPE32())
    {
        WORD NumberOfSections = VAL16(g_pPELoader->ntHeaders32()->FileHeader.NumberOfSections);
		WritePerfDataInt("Num.of PE sections", "Num.of PE sections", "bytes", "bytes", NumberOfSections);
        printLine(GUICookie,"");
		sprintf(szString,"// Num.of PE sections   : %d", NumberOfSections);
        printLine(GUICookie,szStr);

		IMAGE_SECTION_HEADER *pSecHdr = IMAGE_FIRST_SECTION(g_pPELoader->ntHeaders32());

		for (i=0; i < NumberOfSections; ++i) 
		{
		  WritePerfDataInt((char*)pSecHdr->Name,(char*)pSecHdr->Name, "bytes", "bytes",VAL32(pSecHdr->SizeOfRawData));
		  sprintf(szString,"//   %-8s - %d", pSecHdr->Name, VAL32(pSecHdr->SizeOfRawData));
		  printLine(GUICookie,szStr);
		  ++pSecHdr;
		}
   }
   else
   {
        WORD    NumberOfSections = VAL16(g_pPELoader->ntHeaders64()->FileHeader.NumberOfSections);
		WritePerfDataInt("Num.of PE sections", "Num.of PE sections", "bytes", "bytes", NumberOfSections);
		printLine(GUICookie,"");
		sprintf(szString,"// Num.of PE sections   : %d", NumberOfSections);
		printLine(GUICookie,szStr);
            
		IMAGE_SECTION_HEADER *pSecHdr = IMAGE_FIRST_SECTION(g_pPELoader->ntHeaders64());

	    for (i=0; i < NumberOfSections; ++i) 
	    {
            WritePerfDataInt((char*)pSecHdr->Name,(char*)pSecHdr->Name, "bytes", "bytes",VAL32(pSecHdr->SizeOfRawData));
            sprintf(szString,"//   %-8s - %d", pSecHdr->Name, VAL32(pSecHdr->SizeOfRawData));
            printLine(GUICookie,szStr);
            ++pSecHdr;
        }
   }

//   if(FAILED(GetMetaDataPublicInterfaceFromInternal(g_pImport, IID_IMetaDataTables, (void**)&pITables)))
    if(FAILED(g_pPubImport->QueryInterface(IID_IMetaDataTables, (void**)&pITables)))
    {
	    sprintf(szString,"// Unable to get IMetaDataTables interface");
	    printLine(GUICookie,szStr);
	    return;
    }

    if (pITables == 0)
    {
        printLine(GUICookie,RstrA(IDS_E_MDDETAILS));
    }
    else
    {
        DWORD   Size = VAL32(CORHeader->MetaData.Size);
        WritePerfDataInt("CLR meta-data size", "CLR meta-data size", "bytes", "bytes",Size);
        printLine(GUICookie,"");
        sprintf(szString,"// CLR meta-data size  : %d", Size);
        printLine(GUICookie,szStr);
        metaSize = 0;

        pITables->GetTableInfo(TBL_Module, &sizeRec, &count, NULL, NULL, NULL);
        TableSeen(TBL_Module);
        metaSize += size = count * sizeRec;                                     \
        WritePerfDataInt("Module (count)", "Module (count)", "count", "count",count);
        WritePerfDataInt("Module (bytes)", "Module (bytes)", "bytes", "bytes",size);
        sprintf(szString,"//   %-14s- %4d (%d bytes)", "Module", count, size); \
        printLine(GUICookie,szStr);

        if ((count = g_pImport->GetCountWithTokenKind(mdtTypeDef)) > 0)
        {
            int     flags, interfaces = 0, explicitLayout = 0;
            for (i=1; i <= count; ++i)
            {
                g_pImport->GetTypeDefProps(TokenFromRid(i, mdtTypeDef), (ULONG *) &flags, NULL);
                if (flags & tdInterface) ++interfaces;
                if (flags & tdExplicitLayout)   ++explicitLayout;
            }
            // Get count from table -- count reported by GetCount... doesn't include the "global" typedef.
            pITables->GetTableInfo(TBL_TypeDef, &sizeRec, &count, NULL, NULL, NULL);
            TableSeen(TBL_TypeDef);
            metaSize += size = count * sizeRec;

            WritePerfDataInt("TypeDef (count)", "TypeDef (count)", "count", "count", count);
            WritePerfDataInt("TypeDef (bytes)", "TypeDef (bytes)", "bytes", "bytes", size);
            WritePerfDataInt("interfaces", "interfaces", "count", "count", interfaces);
            WritePerfDataInt("explicitLayout", "explicitLayout", "count", "count", explicitLayout);

            sprintf(buf, "  TypeDef       - %4d (%d bytes)", count, size);
            sprintf(szString,"// %-38s %d interfaces, %d explicit layout", buf, interfaces, explicitLayout);
            printLine(GUICookie,szStr);
        }
    }

    pITables->GetTableInfo(TBL_TypeRef, &sizeRec, &count, NULL, NULL, NULL);
    TableSeen(TBL_TypeRef);
    if (count > 0)
    {
        metaSize += size = count * sizeRec;                                      \
        WritePerfDataInt("TypeRef (count)", "TypeRef (count)", "count", "count", count);
        WritePerfDataInt("TypeRef (bytes)", "TypeRef (bytes)", "bytes", "bytes", size);
        sprintf(szString,"//   %-14s- %4d (%d bytes)", "TypeRef", count, size); \
        printLine(GUICookie,szStr);
    }

    if ((count = g_pImport->GetCountWithTokenKind(mdtMethodDef)) > 0)
    {
        int     flags, abstract = 0, native = 0;
        for (i=1; i <= count; ++i)
        {
            flags = g_pImport->GetMethodDefProps(TokenFromRid(i, mdtMethodDef));
            if (flags & mdAbstract) ++abstract;
        }
        pITables->GetTableInfo(TBL_Method, &sizeRec, NULL, NULL, NULL, NULL);
        TableSeen(TBL_Method);
        if (count > 0)
        {
            metaSize += size = count * sizeRec;

            WritePerfDataInt("MethodDef (count)", "MethodDef (count)", "count", "count", count);
            WritePerfDataInt("MethodDef (bytes)", "MethodDef (bytes)", "bytes", "bytes", size);
            WritePerfDataInt("abstract", "abstract", "count", "count", abstract);
            WritePerfDataInt("native", "native", "count", "count", native);
            WritePerfDataInt("methodBodies", "methodBodies", "count", "count", methodBodies);

            sprintf(buf, "  MethodDef     - %4d (%d bytes)", count, size);
            sprintf(szString,"// %-38s %d abstract, %d native, %d bodies", buf, abstract, native, methodBodies);
            printLine(GUICookie,szStr);
        }
    }

    if ((count = g_pImport->GetCountWithTokenKind(mdtFieldDef)) > 0)
    {
        int     flags, constants = 0;

        for (i=1; i <= count; ++i)
        {
            flags = g_pImport->GetFieldDefProps(TokenFromRid(i, mdtFieldDef));
            if ((flags & (fdStatic|fdInitOnly)) == (fdStatic|fdInitOnly)) ++constants;
        }
        pITables->GetTableInfo(TBL_Field, &sizeRec, NULL, NULL, NULL, NULL);
        metaSize += size = count * sizeRec;

        WritePerfDataInt("FieldDef (count)", "FieldDef (count)", "count", "count", count);
        WritePerfDataInt("FieldDef (bytes)", "FieldDef (bytes)", "bytes", "bytes", size);
        WritePerfDataInt("constant", "constant", "count", "count", constants);

        sprintf(buf, "  FieldDef      - %4d (%d bytes)", count, size);
        sprintf(szString,"// %-38s %d constant", buf, constants);
        printLine(GUICookie,szStr);
        TableSeen(TBL_Field);
    }

    DumpTable(TBL_MemberRef,          "MemberRef",              GUICookie);
    DumpTable(TBL_Param,              "ParamDef",               GUICookie);
    DumpTable(TBL_MethodImpl,         "MethodImpl",             GUICookie);
    DumpTable(TBL_Constant,           "Constant",               GUICookie);
    DumpTable(TBL_CustomAttribute,    "CustomAttribute",        GUICookie);
    DumpTable(TBL_FieldMarshal,       "NativeType",             GUICookie);
    DumpTable(TBL_ClassLayout,        "ClassLayout",            GUICookie);
    DumpTable(TBL_FieldLayout,        "FieldLayout",            GUICookie);
    DumpTable(TBL_StandAloneSig,      "StandAloneSig",          GUICookie);
    DumpTable(TBL_InterfaceImpl,      "InterfaceImpl",          GUICookie);
    DumpTable(TBL_PropertyMap,        "PropertyMap",            GUICookie);
    DumpTable(TBL_Property,           "Property",               GUICookie);
    DumpTable(TBL_MethodSemantics,    "MethodSemantic",         GUICookie);
    DumpTable(TBL_DeclSecurity,       "Security",               GUICookie);
    DumpTable(TBL_TypeSpec,           "TypeSpec",               GUICookie);
    DumpTable(TBL_ModuleRef,          "ModuleRef",              GUICookie);
    DumpTable(TBL_Assembly,           "Assembly",               GUICookie);
    DumpTable(TBL_AssemblyProcessor,  "AssemblyProcessor",      GUICookie);
    DumpTable(TBL_AssemblyOS,         "AssemblyOS",             GUICookie);
    DumpTable(TBL_AssemblyRef,        "AssemblyRef",            GUICookie);
    DumpTable(TBL_AssemblyRefProcessor, "AssemblyRefProcessor", GUICookie);
    DumpTable(TBL_AssemblyRefOS,      "AssemblyRefOS",          GUICookie);
    DumpTable(TBL_File,               "File",                   GUICookie);
    DumpTable(TBL_ExportedType,       "ExportedType",           GUICookie);
    DumpTable(TBL_ManifestResource,   "ManifestResource",       GUICookie);
    DumpTable(TBL_NestedClass,        "NestedClass",            GUICookie);

    // Rest of the tables.
    pITables->GetNumTables(&count);
    for (i=0; i<count; ++i)
    {
        if (!IsTableSeen(i))
            DumpTable(i, NULL, GUICookie);
    }

    // String heap
    pITables->GetStringHeapSize(&sizeRec);
    if (sizeRec > 0)
    {
        metaSize += sizeRec;
        WritePerfDataInt("Strings", "Strings", "bytes", "bytes",sizeRec);
        sprintf(szString,"//   Strings       - %5d bytes", sizeRec);
        printLine(GUICookie,szStr);
    }
    // Blob heap
    pITables->GetBlobHeapSize(&sizeRec);
    if (sizeRec > 0)
    {
        metaSize += sizeRec;
        WritePerfDataInt("Blobs", "Blobs", "bytes", "bytes",sizeRec);
        sprintf(szString,"//   Blobs         - %5d bytes", sizeRec);
        printLine(GUICookie,szStr);
    }
    // User String Heap
    pITables->GetUserStringHeapSize(&sizeRec);
    if (sizeRec > 0)
    {
        metaSize += sizeRec;
        WritePerfDataInt("UserStrings", "UserStrings", "bytes", "bytes",sizeRec);
        sprintf(szString,"//   UserStrings   - %5d bytes", sizeRec);
        printLine(GUICookie,szStr);
    }
    // Guid heap
    pITables->GetGuidHeapSize(&sizeRec);
    if (sizeRec > 0)
    {
        metaSize += sizeRec;
        WritePerfDataInt("Guids", "Guids", "bytes", "bytes", sizeRec);
        sprintf(szString,"//   Guids         - %5d bytes", sizeRec);
        printLine(GUICookie,szStr);
    }

    if (VAL32(CORHeader->MetaData.Size) - metaSize > 0)
    {
        WritePerfDataInt("Uncategorized", "Uncategorized", "bytes", "bytes",VAL32(CORHeader->MetaData.Size) - metaSize);
        sprintf(szString,"//   Uncategorized - %5d bytes", VAL32(CORHeader->MetaData.Size) - metaSize);
        printLine(GUICookie,szStr);
    }

    if (miscCOMPlusSize != 0)
    {
        WritePerfDataInt("CLR additional info", "CLR additional info", "bytes", "bytes", miscCOMPlusSize);
        sprintf(szString,"// CLR additional info : %d", miscCOMPlusSize);
        printLine(GUICookie,"");
        printLine(GUICookie,szStr);

        if (CORHeader->CodeManagerTable.Size != 0)
        {
            WritePerfDataInt("CodeManagerTable", "CodeManagerTable", "bytes", "bytes", VAL32(CORHeader->CodeManagerTable.Size));
            sprintf(szString,"//   CodeManagerTable  - %d", VAL32(CORHeader->CodeManagerTable.Size));
            printLine(GUICookie,szStr);
        }


        if (CORHeader->VTableFixups.Size != 0)
        {
            WritePerfDataInt("VTableFixups", "VTableFixups", "bytes", "bytes", VAL32(CORHeader->VTableFixups.Size));
            sprintf(szString,"//   VTableFixups      - %d", VAL32(CORHeader->VTableFixups.Size));
            printLine(GUICookie,szStr);
        }

        if (CORHeader->Resources.Size != 0)
        {
            WritePerfDataInt("Resources", "Resources", "bytes", "bytes", VAL32(CORHeader->Resources.Size));
            sprintf(szString,"//   Resources         - %d", VAL32(CORHeader->Resources.Size));
            printLine(GUICookie,szStr);
        }
    }
    WritePerfDataInt("CLR method headers", "CLR method headers", "count", "count", methodHeaderSize);
    sprintf(szString,"// CLR method headers : %d", methodHeaderSize);
    printLine(GUICookie,"");
    printLine(GUICookie,szStr);
    WritePerfDataInt("Num.of method bodies", "Num.of method bodies", "count", "count",methodBodies);
    sprintf(szString,"//   Num.of method bodies  - %d", methodBodies);
    printLine(GUICookie,szStr);
    WritePerfDataInt("Num.of fat headers", "Num.of fat headers", "count", "count", fatHeaders);
    sprintf(szString,"//   Num.of fat headers    - %d", fatHeaders);
    printLine(GUICookie,szStr);
    WritePerfDataInt("Num.of tiny headers", "Num.of tiny headers", "count", "count", tinyHeaders);
    sprintf(szString,"//   Num.of tiny headers   - %d", tinyHeaders);
    printLine(GUICookie,szStr);
    
    if (deprecatedHeaders > 0) {
        WritePerfDataInt("Num.of old headers", "Num.of old headers", "count", "count", deprecatedHeaders);
        sprintf(szString,"//   Num.of old headers    - %d", deprecatedHeaders);
        printLine(GUICookie,szStr);
    }

    if (fatSections != 0 || smallSections != 0) {
        WritePerfDataInt("Num.of fat sections", "Num.of fat sections", "count", "count", fatSections);
        sprintf(szString,"//   Num.of fat sections   - %d", fatSections);
        printLine(GUICookie,szStr);
   
        WritePerfDataInt("Num.of small section", "Num.of small section", "count", "count", smallSections);
        sprintf(szString,"//   Num.of small sections - %d", smallSections);
        printLine(GUICookie,szStr);
    }

    WritePerfDataInt("Managed code", "Managed code", "bytes", "bytes", methodBodySize);
    sprintf(szString,"// Managed code : %d", methodBodySize);
    printLine(GUICookie,"");
    printLine(GUICookie,szStr);
   
    if (methodBodies != 0) {
        WritePerfDataInt("Ave method size", "Ave method size", "bytes", "bytes", methodBodySize / methodBodies);
        sprintf(szString,"//   Ave method size - %d", methodBodySize / methodBodies);
        printLine(GUICookie,szStr);
    }
    
    if (pITables)
        pITables->Release();

    if(g_fDumpToPerfWriter) 
        CloseHandle((char*) g_PerfDataFilePtr);
}

void DumpHexbytes(char* szString,BYTE *pb, DWORD fromPtr, DWORD toPtr, DWORD limPtr)
{
    char sz[32];
    char* szptr = &szString[strlen(szString)];
    int k,i;
    DWORD curPtr;
    bool printsz = FALSE;
    BYTE zero = 0;
    *szptr = 0;
    for(i = 0,k = 0,curPtr=fromPtr; curPtr < toPtr; i++,k++,curPtr++,pb++)
    {

        if(k == 16)
        {
            if(printsz) szptr+=sprintf(szptr,"  // %s",sz);
            printLine(g_pFile,szString);
            szptr = &szString[0];
            szptr+=sprintf(szptr,"%s                ",g_szAsmCodeIndent);
            k = 0;
            printsz = FALSE;
        }
        if(curPtr >= limPtr) pb = &zero;    // at limPtr and after, pad with 0
        szptr+=sprintf(szptr," %2.2X", *pb);
        if(isprint(*pb))
        {
            sz[k] = *pb;
            printsz = TRUE;
        }
        else
        {
            sz[k] = '.';
        }
        sz[k+1] = 0;
    }
    szptr+=sprintf(szptr,") ");
    if(printsz)
    {
        for(i = k; i < 16; i++) szptr+=sprintf(szptr,"   ");
        szptr+=sprintf(szptr,"// %s",sz);
    }
    printLine(g_pFile,szString);
}

struct  VTableEntry
{
    DWORD   dwAddr;
    WORD    wCount;
    WORD    wType;
};

struct ExpDirTable
{
	DWORD	dwFlags;
	DWORD	dwDateTime;
	WORD	wVMajor;
	WORD	wVMinor;
	DWORD	dwNameRVA;
	DWORD	dwOrdinalBase;
	DWORD	dwNumATEntries;
	DWORD	dwNumNamePtrs;
	DWORD	dwAddrTableRVA;
	DWORD	dwNamePtrRVA;
	DWORD	dwOrdTableRVA;
};

void DumpEATEntries(
    void* GUICookie,
    IMAGE_NT_HEADERS32 *pNTHeader32, IMAGE_OPTIONAL_HEADER32 *pOptHeader32,
	IMAGE_NT_HEADERS64 *pNTHeader64, IMAGE_OPTIONAL_HEADER64 *pOptHeader64)
{
	IMAGE_DATA_DIRECTORY *pExportDir = NULL;
	IMAGE_SECTION_HEADER *pSecHdr = NULL;
	DWORD i,j,N;
	BOOL bpOpt = FALSE;
#ifdef _DEBUG
	char szString[4096];
#endif

	if (g_pPELoader->IsPE32())
	{
		pExportDir = pOptHeader32->DataDirectory;
		pSecHdr = IMAGE_FIRST_SECTION(pNTHeader32);
		N = VAL16(pNTHeader32->FileHeader.NumberOfSections);

		if (pOptHeader32->NumberOfRvaAndSizes)
			bpOpt = TRUE;
	}
	else
	{
		pExportDir = pOptHeader64->DataDirectory;
		pSecHdr = IMAGE_FIRST_SECTION(pNTHeader64);
		N = VAL16(pNTHeader64->FileHeader.NumberOfSections);

		if (pOptHeader64->NumberOfRvaAndSizes)
			bpOpt = TRUE;

	}
	if(bpOpt)
	{
		ExpDirTable *pExpTable = NULL;
		if(pExportDir->Size)
		{
#ifdef _DEBUG
			sprintf(szString,"// Export dir VA=%X size=%X ",VAL32(pExportDir->VirtualAddress),VAL32(pExportDir->Size));
			printLine(GUICookie,szString);
#endif
			DWORD vaExpTable = VAL32(pExportDir->VirtualAddress);
			for (i=0; i < N; i++,pSecHdr++)
			{
				if((vaExpTable >= VAL32(pSecHdr->VirtualAddress))&&
					(vaExpTable < VAL32(pSecHdr->VirtualAddress)+VAL32(pSecHdr->Misc.VirtualSize)))
				{
					pExpTable = (ExpDirTable*)( g_pPELoader->base()
						+ VAL32(pSecHdr->PointerToRawData) 
						+ vaExpTable - VAL32(pSecHdr->VirtualAddress));
#ifdef _DEBUG
					sprintf(szString,"// in section '%s': VA=%X Misc.VS=%X PRD=%X ",(char*)(pSecHdr->Name),
						VAL32(pSecHdr->VirtualAddress),VAL32(pSecHdr->Misc.VirtualSize),VAL32(pSecHdr->PointerToRawData));
					printLine(GUICookie,szString);
					sprintf(szString,"// Export Directory Table:"); printLine(GUICookie,szString);
					sprintf(szString,"// dwFlags = %X",VAL32(pExpTable->dwFlags)); printLine(GUICookie,szString);
					sprintf(szString,"// dwDateTime = %X",VAL32(pExpTable->dwDateTime)); printLine(GUICookie,szString);
					sprintf(szString,"// wVMajor = %X",VAL16(pExpTable->wVMajor)); printLine(GUICookie,szString);
					sprintf(szString,"// wVMinor = %X",VAL16(pExpTable->wVMinor)); printLine(GUICookie,szString);
					sprintf(szString,"// dwNameRVA = %X",VAL32(pExpTable->dwNameRVA)); printLine(GUICookie,szString);
					sprintf(szString,"// dwOrdinalBase = %X",VAL32(pExpTable->dwOrdinalBase)); printLine(GUICookie,szString);
					sprintf(szString,"// dwNumATEntries = %X",VAL32(pExpTable->dwNumATEntries)); printLine(GUICookie,szString);
					sprintf(szString,"// dwNumNamePtrs = %X",VAL32(pExpTable->dwNumNamePtrs)); printLine(GUICookie,szString);
					sprintf(szString,"// dwAddrTableRVA = %X",VAL32(pExpTable->dwAddrTableRVA)); printLine(GUICookie,szString);
					sprintf(szString,"// dwNamePtrRVA = %X",VAL32(pExpTable->dwNamePtrRVA)); printLine(GUICookie,szString);
					sprintf(szString,"// dwOrdTableRVA = %X",VAL32(pExpTable->dwOrdTableRVA)); printLine(GUICookie,szString);
#endif
					if(pExpTable->dwNameRVA)
					{
						char*	szName;
						g_pPELoader->getVAforRVA(VAL32(pExpTable->dwNameRVA), (void **) &szName);
#ifdef _DEBUG
						sprintf(szString,"// DLL Name: '%s'",szName); printLine(GUICookie,szString);
#endif
					}
					if(pExpTable->dwNumATEntries && pExpTable->dwAddrTableRVA)
					{
						DWORD* pExpAddr;
						BYTE *pCont;
						DWORD dwTokRVA;
						mdToken* pTok;
						g_pPELoader->getVAforRVA(VAL32(pExpTable->dwAddrTableRVA), (void **) &pExpAddr);
#ifdef _DEBUG
						sprintf(szString,"// Export Address Table:"); printLine(GUICookie,szString);
#endif
						g_nEATableRef = VAL32(pExpTable->dwNumATEntries);
						if (g_prEATableRef == NULL)
						{
							g_prEATableRef = new DynamicArray<EATableRef>;
						}

						(*g_prEATableRef)[g_nEATableRef].tkTok = 0; // to avoid multiple reallocations of DynamicArray
						for(j=0; j < VAL32(pExpTable->dwNumATEntries); j++,pExpAddr++)
						{
							g_pPELoader->getVAforRVA(VAL32(*pExpAddr), (void **) &pCont);
#ifdef _DEBUG
							sprintf(szString,"// [%d]: RVA=%X VA=%X(",j,VAL32(*pExpAddr),pCont); 
							DumpByteArray(szString,pCont,16,GUICookie);
							printLine(GUICookie,szString);
#endif
							(*g_prEATableRef)[j].tkTok = 0;
							if(*((WORD*)pCont) == VAL16(0x25FF))
							{
								dwTokRVA = VAL32(*((DWORD*)(pCont+2))); // first two bytes - JumpIndirect (0x25FF)
								if(g_pPELoader->IsPE32())
									dwTokRVA -= VAL32((DWORD)pOptHeader32->ImageBase);
								else
									dwTokRVA -= VAL32((DWORD)pOptHeader64->ImageBase);
								if(g_pPELoader->getVAforRVA(dwTokRVA,(void**)&pTok))
									(*g_prEATableRef)[j].tkTok = VAL32(*pTok);
							}
							(*g_prEATableRef)[j].pszName = NULL;

						}
					}
					if(pExpTable->dwNumNamePtrs && pExpTable->dwNamePtrRVA && pExpTable->dwOrdTableRVA)
					{
						DWORD *pNamePtr;
						WORD	*pOrd;
						char*	szName;
						g_pPELoader->getVAforRVA(VAL32(pExpTable->dwNamePtrRVA), (void **) &pNamePtr);
						g_pPELoader->getVAforRVA(VAL32(pExpTable->dwOrdTableRVA), (void **) &pOrd);
#ifdef _DEBUG
						sprintf(szString,"// Export Names:"); printLine(GUICookie,szString);
#endif
						for(j=0; j < VAL32(pExpTable->dwNumATEntries); j++,pNamePtr++,pOrd++)
						{
							g_pPELoader->getVAforRVA(VAL32(*pNamePtr), (void **) &szName);
#ifdef _DEBUG
							sprintf(szString,"// [%d]: NamePtr=%X Ord=%X Name='%s'",j,VAL32(*pNamePtr),*pOrd,szName);
                            printLine(GUICookie,szString);
#endif
							(*g_prEATableRef)[VAL16(*pOrd)].pszName = szName;
						}
					}
					g_nEATableBase = VAL32(pExpTable->dwOrdinalBase);
					break;
				}
			}
		}
	}
}
// helper to avoid mixing of SEH and stack objects with destructors
void DumpEATEntriesWrapper(
    void* GUICookie,
    IMAGE_NT_HEADERS32 *pNTHeader32,
    IMAGE_OPTIONAL_HEADER32 *pOptHeader32,
	IMAGE_NT_HEADERS64 *pNTHeader64,
    IMAGE_OPTIONAL_HEADER64 *pOptHeader64)
{
	PAL_TRY
	{
		DumpEATEntries(GUICookie, pNTHeader32, pOptHeader32, pNTHeader64, pOptHeader64);
	}
	PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
		printLine(GUICookie,"// ERROR READING EXPORT ADDRESS TABLE");
		if (g_prEATableRef != NULL)
		{
			delete g_prEATableRef;
			g_prEATableRef = NULL;
		}
		g_nEATableRef = 0;
	}
	PAL_ENDTRY
}

void DumpVtable(void* GUICookie)
{
    // VTable : primary processing
    DWORD  pVTable=0;
    VTableEntry*    pVTE;
    DWORD i,j,k;
    char szString[4096];
    char* szptr;
    
    IMAGE_NT_HEADERS32 *pNTHeader32 = NULL;
    IMAGE_OPTIONAL_HEADER32 *pOptHeader32 = NULL;

    IMAGE_NT_HEADERS64 *pNTHeader64 = NULL;
    IMAGE_OPTIONAL_HEADER64 *pOptHeader64 = NULL;

	if (g_pPELoader->IsPE32())
	{
		pNTHeader32 = g_pPELoader->ntHeaders32();
		pOptHeader32 = &pNTHeader32->OptionalHeader;


		sprintf(szString,"%s.imagebase 0x%08x", g_szAsmCodeIndent,VAL32(pOptHeader32->ImageBase));
		printLine(GUICookie,szString);
		sprintf(szString,"%s.subsystem 0x%08x", g_szAsmCodeIndent,VAL16(pOptHeader32->Subsystem));
		printLine(GUICookie,szString);
		sprintf(szString,"%s.file alignment %d", g_szAsmCodeIndent,VAL32(pOptHeader32->FileAlignment));
		printLine(GUICookie,szString);
	}
	else
	{
		pNTHeader64 = g_pPELoader->ntHeaders64();
		pOptHeader64 = &pNTHeader64->OptionalHeader;

		sprintf(szString,"%s.imagebase 0x%08x", g_szAsmCodeIndent,VAL64(pOptHeader64->ImageBase));
		printLine(GUICookie,szString);
		sprintf(szString,"%s.subsystem 0x%08x", g_szAsmCodeIndent,VAL16(pOptHeader64->Subsystem));
		printLine(GUICookie,szString);
		sprintf(szString,"%s.file alignment %d", g_szAsmCodeIndent,VAL32(pOptHeader64->FileAlignment));
		printLine(GUICookie,szString);
	}

	sprintf(szString,"%s.corflags 0x%08x", g_szAsmCodeIndent,VAL32(g_CORHeader->Flags));
	printLine(GUICookie,szString);
	sprintf(szString,"%s// Image base: 0x%08x",g_szAsmCodeIndent,g_pPELoader->base());
	printLine(GUICookie,szString);

	DumpEATEntriesWrapper(GUICookie, pNTHeader32, pOptHeader32, pNTHeader64, pOptHeader64);

	g_nVTableRef = 0;
    if(VAL32(g_CORHeader->VTableFixups.Size))
    {
        IMAGE_SECTION_HEADER *pSecHdr = NULL;
		DWORD dwNumberOfSections;

		if (g_pPELoader->IsPE32())
		{
	        pSecHdr = IMAGE_FIRST_SECTION(g_pPELoader->ntHeaders32());
			dwNumberOfSections = VAL16(g_pPELoader->ntHeaders32()->FileHeader.NumberOfSections);
		}
		else
		{
		    pSecHdr = IMAGE_FIRST_SECTION(g_pPELoader->ntHeaders64());
			dwNumberOfSections = VAL16(g_pPELoader->ntHeaders64()->FileHeader.NumberOfSections);
		}

        pVTable = VAL32(g_CORHeader->VTableFixups.VirtualAddress);

        for (i=0; i < dwNumberOfSections; i++,pSecHdr++)
        {
            if(((DWORD)pVTable >= VAL32(pSecHdr->VirtualAddress))&&
                ((DWORD)pVTable < VAL32(pSecHdr->VirtualAddress)+VAL32(pSecHdr->Misc.VirtualSize)))
            {
                pVTE = (VTableEntry*)( g_pPELoader->base()
                    + VAL32(pSecHdr->PointerToRawData) 
                    + pVTable - VAL32(pSecHdr->VirtualAddress));
                for(j=VAL32(g_CORHeader->VTableFixups.Size),k=0; j > 0; pVTE++, j-=sizeof(VTableEntry),k++)
                {
                    szptr = &szString[0];
                    szptr+=sprintf(szptr,"%s.vtfixup [%d] ",g_szAsmCodeIndent,VAL16(pVTE->wCount));
					DWORD dwSize = VAL16(pVTE->wCount) * 4;
                    if(pVTE->wType & COR_VTABLE_32BIT) szptr+=sprintf(szptr,"int32 ");
                    else if(VAL16(pVTE->wType) & COR_VTABLE_64BIT)
					{
						szptr+=sprintf(szptr,"int64 ");
						dwSize <<= 1;
					}
                    if(VAL16(pVTE->wType) & COR_VTABLE_FROM_UNMANAGED)
                        szptr+=sprintf(szptr,"fromunmanaged ");
                    if(VAL16(pVTE->wType) & COR_VTABLE_CALL_MOST_DERIVED)
                        szptr+=sprintf(szptr,"callmostderived ");
                    szptr+=sprintf(szptr,"at ");
                    szptr = DumpDataPtr(szptr,VAL32(pVTE->dwAddr), dwSize);
                    // Walk every v-table fixup entry and dump the slots.
                    {
                        BYTE *pSlot;
                        if (g_pPELoader->getVAforRVA(VAL32(pVTE->dwAddr), (void **) &pSlot))
                        {
                            szptr+=sprintf(szptr," //");
                            for (WORD iSlot=0;  iSlot<VAL16(pVTE->wCount);  iSlot++)
                            {
                                mdMethodDef tkMethod = VAL32(*(DWORD *) pSlot);
                                if (VAL16(pVTE->wType) & COR_VTABLE_32BIT)
                                {
                                    szptr+=sprintf(szptr," %08X", VAL32(*(DWORD *)pSlot));
                                    pSlot += sizeof(DWORD);
                                }
                                else
                                {
                                    szptr+=sprintf(szptr," %016I64X", VAL64(*(unsigned __int64 *)pSlot));
                                    pSlot += sizeof(unsigned __int64);
                                }
                                if (g_prVTableRef == NULL)
                                {
                                    g_prVTableRef = new DynamicArray<VTableRef>;
                                }
                                (*g_prVTableRef)[g_nVTableRef].tkTok = tkMethod;
                                (*g_prVTableRef)[g_nVTableRef].wEntry = (WORD)k;
                                (*g_prVTableRef)[g_nVTableRef].wSlot = iSlot;
                                g_nVTableRef++;

                                //ValidateToken(tkMethod, mdtMethodDef);
                            }
                        }
                        else
                            szptr+=sprintf(szptr," %s",RstrA(IDS_E_BOGUSRVA));
                    }
                    printLine(GUICookie,szString);
                }
                break;
            }
        }
    }
}
// MetaInfo integration:
void DumpMI(char *str)
{
    static char szStringBuf[8192];
    static BOOL fInit = TRUE;
    static char* szString = &szStringBuf[0];
    static void* GUICookie;
    char* pch;

    if(fInit)
    {
        memset(szStringBuf,0,8192);
        strcpy(szStringBuf,"// ");
        fInit = FALSE;
        GUICookie = (void*)str;
        return;
    }
    strcat(szString,str);
    if((pch = strchr(szString,'\n')))
    {
        *pch = 0;
        printLine(GUICookie,szString);
        pch++;
        memcpy(&szStringBuf[3], pch, strlen(pch)+1);
    }
}

HRESULT VEHandlerReporter( // Return status.
    LPCWSTR     szMsg,                  // Error message.
    VEContext   Context,                // Error context (offset,token)
    HRESULT     hrRpt)                  // HRESULT for the message
{
    WCHAR* wzMsg;
    char szString[8192];
    if(szMsg)
    {
        wzMsg = (WCHAR*)malloc(sizeof(WCHAR)*(lstrlenW(szMsg)+256)); //new WCHAR[lstrlenW(szMsg)+256];
        lstrcpyW(wzMsg,szMsg);
        // include token and offset from Context
        if(Context.Token) swprintf(&wzMsg[lstrlenW(wzMsg)],L" [token:0x%08X]",Context.Token);
        if(Context.uOffset) swprintf(&wzMsg[lstrlenW(wzMsg)],L" [at:0x%X]",Context.uOffset);
        swprintf(&wzMsg[lstrlenW(wzMsg)],L" [hr:0x%08X]",hrRpt);
        //wprintf(L"%s\n", wzMsg);
        sprintf(szString,"%ls\n",wzMsg);
        //delete wzMsg;
		free(wzMsg);
        DumpMI(szString);
    }
    return S_OK;
}

void DumpMetaInfo(char* pszFileName, char* pszObjFileName, void* GUICookie)
{
    char* pch = strrchr(pszFileName,'.');
    static BOOL fInit = TRUE;
    if(fInit)
    {
        DumpMI((char*)GUICookie); // initialize the print function for DumpMetaInfo
        fInit = FALSE;
    }

	ULONG32 L = (ULONG32) strlen(pszFileName)+1;
    WCHAR *pwzFileName = (WCHAR*)malloc(sizeof(WCHAR)*L); //new WCHAR[L];
	memset(pwzFileName,0,sizeof(WCHAR)*L);
	WszMultiByteToWideChar(CP_UTF8,0,pszFileName,-1,pwzFileName,L);


    if(pch && (!_stricmp(pch+1,"lib") || !_stricmp(pch+1,"obj")))
    {   // This works only when all the rest does not
        // Init and run.
        CoInitializeCor(0);
        if(SUCCEEDED(PAL_CoCreateInstance(CLSID_CorMetaDataDispenser, IID_IMetaDataDispenserEx, (void **) &g_pDisp)))
        {
			WCHAR *pwzObjFileName=NULL;
			if (pszObjFileName)
			{
				int L = (int) strlen(pszObjFileName)+1;
				pwzObjFileName = new WCHAR[L];
				memset(pwzObjFileName,0,sizeof(WCHAR)*L);
				WszMultiByteToWideChar(CP_UTF8,0,pszObjFileName,-1,pwzObjFileName,L);
			}
            DisplayFile(pwzFileName, true, g_ulMetaInfoFilter, pwzObjFileName, DumpMI);
            g_pDisp->Release();
			g_pDisp = NULL;
		    if (pwzObjFileName) delete pwzObjFileName;
        }
        CoUninitializeCor();
    }
    else
    {
		HRESULT hr = S_OK;
		if(g_pDisp == NULL)
		{
			hr  = PAL_CoCreateInstance(CLSID_CorMetaDataDispenser,
						   IID_IMetaDataDispenserEx, (void **) &g_pDisp);
		}
        if(SUCCEEDED(hr))
        {
			g_ValModuleType = ValidatorModuleTypePE;
			if(g_pAssemblyImport==NULL) g_pAssemblyImport = GetAssemblyImport(NULL);
                printLine(GUICookie,RstrA(IDS_E_MISTART));

			if(!g_fCheckOwnership) g_ulMetaInfoFilter |= 0x10000000;
			//MDInfo metaDataInfo(g_pPubImport, g_pAssemblyImport, (LPCWSTR)pwzFileName, DumpMI, g_ulMetaInfoFilter);
			MDInfo metaDataInfo(g_pDisp,(LPCWSTR)pwzFileName, DumpMI, g_ulMetaInfoFilter);
			g_ulMetaInfoFilter &= ~0x10000000;
			metaDataInfo.SetVEHandlerReporter((__int64) (size_t) VEHandlerReporter);
			metaDataInfo.DisplayMD();
                printLine(GUICookie,RstrA(IDS_E_MIEND));
		}
    }
    //delete pwzFileName;
	free(pwzFileName);
}

void DumpPreamble()
{
	char szString[1024];
    if((g_uCodePage == CP_UTF8)&&g_pFile) fwrite("\357\273\277",3,1,g_pFile);
    else if((g_uCodePage == 0xFFFFFFFF)&&g_pFile) fwrite("\377\376",2,1,g_pFile);
	printLine(g_pFile,"");
    sprintf(szString,"//  Microsoft (R) Shared Source CLI IL Disassembler.  Version " VER_FILEVERSION_STR);
    printLine(g_pFile,szString);
    sprintf(szString,"//  %s", VER_LEGALCOPYRIGHT_DOS_STR);
    printLine(g_pFile,szString);
    printLine(g_pFile,"");
    if(g_fLimitedVisibility || (!g_fShowCA) || (!g_fDumpAsmCode) 
        || (g_Mode & (MODE_DUMP_CLASS | MODE_DUMP_CLASS_METHOD | MODE_DUMP_CLASS_METHOD_SIG)))
    {
        printLine(g_pFile,"");
        printError(g_pFile,RstrA(IDS_E_PARTDASM));
        printLine(g_pFile,"");
    }

    if(g_fLimitedVisibility)
    {
        strcpy(szString, RstrA(IDS_E_ONLYITEMS));
        if(!g_fHidePub) strcat(szString," Public");
        if(!g_fHidePriv) strcat(szString," Private");
        if(!g_fHideFam) strcat(szString," Family");
        if(!g_fHideAsm) strcat(szString," Assembly");
        if(!g_fHideFAA) strcat(szString," FamilyANDAssembly");
        if(!g_fHidePrivScope) strcat(szString," PrivateScope");
        printLine(g_pFile,szString);
    }
}

void DumpSummary()
{
	ULONG i;
    const char      *pcClass,*pcNS,*pcMember, *pcSig;
	char szString[4096];
	char szFQN[4096];
    HENUMInternal   hEnum;
	mdToken tkMember;
    CQuickBytes     qbMemberSig;
    PCCOR_SIGNATURE pComSig;
    ULONG           cComSig;
	DWORD dwAttrs;
	mdToken tkEventType;

	printLine(g_pFile,"//============ S U M M A R Y =================================");
    if (SUCCEEDED(g_pImport->EnumGlobalFunctionsInit(&hEnum)))
	{
		while(g_pImport->EnumNext(&hEnum, &tkMember))
		{
			pcMember = g_pImport->GetNameOfMethodDef(tkMember);
			pComSig = g_pImport->GetSigOfMethodDef(tkMember, &cComSig);
			qbMemberSig.ReSize(0);
			pcSig = cComSig ? PrettyPrintSig(pComSig, cComSig, "", &qbMemberSig, g_pImport,NULL) : "NO SIGNATURE";
			sprintf(szString,"// %08X [GLM] %s : %s", tkMember,ProperName((char*)pcMember),pcSig);
			printLine(g_pFile,szString);
		}
	}
    g_pImport->EnumClose(&hEnum);
    if (SUCCEEDED(g_pImport->EnumGlobalFieldsInit(&hEnum)))
	{
		while(g_pImport->EnumNext(&hEnum, &tkMember))
		{
			pcMember = g_pImport->GetNameOfFieldDef(tkMember);
			pComSig = g_pImport->GetSigOfFieldDef(tkMember, &cComSig);
			qbMemberSig.ReSize(0);
			pcSig = cComSig ? PrettyPrintSig(pComSig, cComSig, "", &qbMemberSig, g_pImport,NULL) : "NO SIGNATURE";
			sprintf(szString,"// %08X [GLF] %s : %s", tkMember,ProperName((char*)pcMember),pcSig);
			printLine(g_pFile,szString);
		}
	}
    g_pImport->EnumClose(&hEnum);

    for (i = 0; i < g_NumClasses; i++)
    {
	    g_pImport->GetNameOfTypeDef(g_cl_list[i],&pcClass,&pcNS);
		if(*pcNS) sprintf(szFQN,"%s.%s", ProperName((char*)pcNS),ProperName((char*)pcClass));
		else sprintf(szFQN,"%s",ProperName((char*)pcClass));
		sprintf(szString,"// %08X [CLS] %s", g_cl_list[i],szFQN);
		printLine(g_pFile,szString);
		if(SUCCEEDED(g_pImport->EnumInit(mdtMethodDef, g_cl_list[i], &hEnum)))
		{
			while(g_pImport->EnumNext(&hEnum, &tkMember))
			{
				pcMember = g_pImport->GetNameOfMethodDef(tkMember);
				pComSig = g_pImport->GetSigOfMethodDef(tkMember, &cComSig);
				qbMemberSig.ReSize(0);
			    pcSig = cComSig ? PrettyPrintSig(pComSig, cComSig, "", &qbMemberSig, g_pImport,NULL) : "NO SIGNATURE";
				sprintf(szString,"// %08X [MET] %s::%s : %s", tkMember,szFQN,ProperName((char*)pcMember),pcSig);
				printLine(g_pFile,szString);
			}
		}
	    g_pImport->EnumClose(&hEnum);
		if(SUCCEEDED(g_pImport->EnumInit(mdtFieldDef, g_cl_list[i], &hEnum)))
		{
			while(g_pImport->EnumNext(&hEnum, &tkMember))
			{
				pcMember = g_pImport->GetNameOfFieldDef(tkMember);
				pComSig = g_pImport->GetSigOfFieldDef(tkMember, &cComSig);
				qbMemberSig.ReSize(0);
			    pcSig = cComSig ? PrettyPrintSig(pComSig, cComSig, "", &qbMemberSig, g_pImport,NULL) : "NO SIGNATURE";
				sprintf(szString,"// %08X [FLD] %s::%s : %s", tkMember,szFQN,ProperName((char*)pcMember),pcSig);
				printLine(g_pFile,szString);
			}
		}
	    g_pImport->EnumClose(&hEnum);
		if(SUCCEEDED(g_pImport->EnumInit(mdtEvent, g_cl_list[i], &hEnum)))
		{
			while(g_pImport->EnumNext(&hEnum, &tkMember))
			{
			    g_pImport->GetEventProps(tkMember,&pcMember,&dwAttrs,&tkEventType);
				qbMemberSig.ReSize(0);
				pcSig = "NO TYPE";
				if(RidFromToken(tkEventType))
				{
						switch(TypeFromToken(tkEventType))
						{
								case mdtTypeRef:
								case mdtTypeDef:
								case mdtTypeSpec:
										pcSig = PrettyPrintClass(&qbMemberSig,tkEventType,g_pImport);
									break;
								default:
									break;
						}
				}
				sprintf(szString,"// %08X [EVT] %s::%s : %s", tkMember,szFQN,ProperName((char*)pcMember),pcSig);
				printLine(g_pFile,szString);
			}
		}
	    g_pImport->EnumClose(&hEnum);
		if(SUCCEEDED(g_pImport->EnumInit(mdtProperty, g_cl_list[i], &hEnum)))
		{
			while(g_pImport->EnumNext(&hEnum, &tkMember))
			{
			    g_pImport->GetPropertyProps(tkMember,&pcMember,&dwAttrs,&pComSig,&cComSig);
				qbMemberSig.ReSize(0);
			    pcSig = cComSig ? PrettyPrintSig(pComSig, cComSig, "", &qbMemberSig, g_pImport,NULL) : "NO SIGNATURE";
				sprintf(szString,"// %08X [PRO] %s::%s : %s", tkMember,szFQN,ProperName((char*)pcMember),pcSig);
				printLine(g_pFile,szString);
			}
		}
	    g_pImport->EnumClose(&hEnum);
    }
	printLine(g_pFile,"//=============== END SUMMARY ==================================");
}
//
// Init PELoader, dump file header info
//
BOOL DumpFile(char *pszFilename)
{
 // DWORD       dwEntryPointRVA;
    BOOL        fSuccess = FALSE;
    DWORD       i;
    char		szString[1024];
    WCHAR       wzInputFileName[MAX_FILENAME_LENGTH];
	char		szFilenameANSI[MAX_FILENAME_LENGTH*3];

        DumpPreamble();
    {
        char* pch = strrchr(g_szInputFile,'.');
        if(pch && (!_stricmp(pch+1,"lib") || !_stricmp(pch+1,"obj")))
        {
            if(g_pFile) DumpMetaInfo(g_szInputFile,g_pszObjFileName,g_pFile);
            return FALSE;
        }
    }

    if(g_pPELoader) goto DumpTheSucker; // skip initialization, it's already done

    g_pPELoader = new PELoader();
    if (g_pPELoader == NULL)
    {
        printError(g_pFile,RstrA(IDS_E_INITLDR));
        goto exit;
    }

	memset(wzInputFileName,0,sizeof(WCHAR)*MAX_FILENAME_LENGTH);
	WszMultiByteToWideChar(CP_UTF8,0,pszFilename,-1,wzInputFileName,MAX_FILENAME_LENGTH);
	memset(szFilenameANSI,0,MAX_FILENAME_LENGTH*3);
	WszWideCharToMultiByte(CP_ACP,0,wzInputFileName,-1,szFilenameANSI,MAX_FILENAME_LENGTH*3,NULL,NULL);
	if(g_fOnUnicode)
	{
		fSuccess = g_pPELoader->open(wzInputFileName);
	}
	else
	{
		fSuccess = g_pPELoader->open(szFilenameANSI);
	}

    if (fSuccess == FALSE)
    {
        sprintf(szString,RstrA(IDS_E_FILEOPEN), szFilenameANSI);
        printError(g_pFile,szString);
        delete(g_pPELoader);
        g_pPELoader = NULL;
        goto exit;
    }
    fSuccess = FALSE;

    if (g_pPELoader->getCOMHeader(&g_CORHeader) == FALSE)
    {
        sprintf(szString,RstrA(IDS_E_NOCORHDR), szFilenameANSI);
        printError(g_pFile,szString);
        if (g_fDumpHeader)
            DumpHeader(g_CORHeader,NULL);
        goto exit;
    }

    if (VAL16(g_CORHeader->MajorRuntimeVersion) == 1 || VAL16(g_CORHeader->MajorRuntimeVersion) > COR_VERSION_MAJOR)
    {
        sprintf(szString,"CORHeader->MajorRuntimeVersion = %d",VAL16(g_CORHeader->MajorRuntimeVersion));
        printError(g_pFile,szString);
        printError(g_pFile,RstrA(IDS_E_BADCORHDR));
        goto exit;
    }
    g_tkEntryPoint = VAL32(g_CORHeader->EntryPointToken); // integration with MetaInfo


    if (g_pPELoader->getVAforRVA(VAL32(g_CORHeader->MetaData.VirtualAddress),&g_pMetaData) == FALSE)
    {
        printError(g_pFile,RstrA(IDS_E_OPENMD));
        goto exit;
    }

    if (FAILED(GetMetaDataInternalInterface(
        g_pMetaData, 
        VAL32(g_CORHeader->MetaData.Size), 
        ofRead, 
        IID_IMDInternalImport,
        (void **)&g_pImport)))
    {
        printError(g_pFile,RstrA(IDS_E_OPENMD));
        goto exit;
    }

    GetMetaDataPublicInterfaceFromInternal(g_pImport, IID_IMetaDataImport, (void**)&g_pPubImport);
    // Get a symbol binder.
    ISymUnmanagedBinder *binder;
    HRESULT hr;

    hr = PAL_CoCreateInstance(CLSID_CorSymBinder,
                                  IID_ISymUnmanagedBinder,
                                  (void**)&binder);

    if (SUCCEEDED(hr))
    {
        hr = binder->GetReaderForFile(g_pPubImport,
                                      wzInputFileName,
                                      NULL,
                                      &g_pSymReader);

        // Release the binder
        binder->Release();
    }

    if (FAILED(hr))
        g_fShowSource = FALSE;

    if(g_fCheckOwnership)
    {
        mdModule mdm;
        BOOL fPassed = TRUE;
        if(SUCCEEDED(g_pPubImport->GetModuleFromScope(&mdm)))
        {
            HCORENUM        hEnum = NULL;
            mdCustomAttribute rCA[4096];
            ULONG           ulCAs;

            g_pPubImport->EnumCustomAttributes(&hEnum, mdm, 0, rCA, 4096, &ulCAs);
            for(ULONG i = 0; i < ulCAs; i++)
            {
                mdToken         tkType;
                BYTE*           pBlob;
                ULONG           ulLen;

                g_pPubImport->GetCustomAttributeProps(              // S_OK or error.
                                                        rCA[i],     // [IN] CustomValue token.
                                                        NULL,       // [OUT, OPTIONAL] Object token.
                                                        &tkType,    // [OUT, OPTIONAL] Put TypeDef/TypeRef token here.
                                         (const void **)&pBlob,     // [OUT, OPTIONAL] Put pointer to data here.
                                                        &ulLen);    // [OUT, OPTIONAL] Put size of date here.

                if(tkType == 0x02000001)
                {
                    fPassed = FALSE;
                    if(pBlob && ulLen && g_pszOwner)
                        fPassed = (0 == memcmp(pBlob,g_pszOwner,ulLen));
                }
            }
            g_pPubImport->CloseEnum( hEnum);
        }
        if(!fPassed)
        {
            printError(g_pFile,RstrA(IDS_E_COPYRIGHT));
            goto exit;
        }
    }
    if((g_uNCA = g_pImport->GetCountWithTokenKind(mdtCustomAttribute)))
	{
		g_rchCA = new char[g_uNCA+1];
		_ASSERTE(g_rchCA);
	}
	
    EnumClasses();

DumpTheSucker:
	if(g_uNCA)
	{
		_ASSERTE(g_rchCA);
		memset(g_rchCA,0,g_uNCA+1);
	}

    {
        // Dump the CLR header info if requested.
        if (g_fDumpHeader)
        {
            DumpHeader(g_CORHeader,g_pFile);
            DumpHeaderDetails(g_CORHeader,g_pFile);
        }
        else
            DumpVTables(g_CORHeader,g_pFile);
        if (g_fDumpStats)
            DumpStatistics(g_CORHeader,g_pFile);

        if(g_fDumpClassList) PrintClassList();
        // MetaInfo integration:
        if(g_fDumpMetaInfo) DumpMetaInfo(pszFilename,NULL,g_pFile);

		if(g_fDumpSummary) DumpSummary();

        if (g_fDumpAsmCode)
        {
            g_szNamespace[0] = 0;
            if(g_tkClassToDump) //g_tkClassToDump is set in EnumClasses
            {
                DumpClass(g_tkClassToDump, g_tkEntryPoint,g_pFile,7); //7-dump everything at once
                goto CloseNamespace;
            }
            {
                HENUMInternal   hEnumMethod;
                ULONG           ulNumGlobalFunc=0;
                if (SUCCEEDED(g_pImport->EnumGlobalFunctionsInit(&hEnumMethod)))
                {
                    ulNumGlobalFunc = g_pImport->EnumGetCount(&hEnumMethod);
                    g_pImport->EnumClose(&hEnumMethod);
                }
            }
            g_fAbortDisassembly = FALSE;
            //DumpVtable(g_pFile);
            DumpManifest(g_pFile);

            /* First dump the classes w/o members*/
			if(g_NumClasses)
			{
				printLine(g_pFile,"//");
				printLine(g_pFile,"// ============== CLASS STRUCTURE DECLARATION ==================");
				printLine(g_pFile,"//");
				for (i = 0; i < g_NumClasses; i++)
				{
					if(g_cl_enclosing[i] == mdTypeDefNil) // nested classes are dumped within enclosing ones
					{
						DumpClass(g_cl_list[i], g_tkEntryPoint,g_pFile,2); // 2=header+nested classes
					}
				}
				if(strlen(g_szNamespace))
				{
					if(g_szAsmCodeIndent[0]) g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
					sprintf(szString,"%s} // end of namespace %s",g_szAsmCodeIndent, ProperName(g_szNamespace));
					printLine(g_pFile,szString);
					printLine(g_pFile,"");
					g_szNamespace[0] = 0;
				}
				printLine(g_pFile,"");
				printLine(g_pFile,"// =============================================================");
				printLine(g_pFile,"");
			}
            /* Second, dump the global methods */
			printLine(g_pFile,"");
			printLine(g_pFile,"// =============== GLOBAL FIELDS AND METHODS ===================");
			printLine(g_pFile,"");
            DumpGlobalFields();
            DumpGlobalMethods(g_tkEntryPoint);
			printLine(g_pFile,"");
			printLine(g_pFile,"// =============================================================");
			printLine(g_pFile,"");
			/* Third, dump the classes with members */
			if(g_NumClasses)
			{
				printLine(g_pFile,"");
				printLine(g_pFile,"// =============== CLASS MEMBERS DECLARATION ===================");
				printLine(g_pFile,"//   note that class flags, 'extends' and 'implements' clauses");
				printLine(g_pFile,"//          are provided here for information only");
				printLine(g_pFile,"");
				for (i = 0; i < g_NumClasses; i++)
				{
					if(g_cl_enclosing[i] == mdTypeDefNil) // nested classes are dumped within enclosing ones
					{
						DumpClass(g_cl_list[i], g_tkEntryPoint,g_pFile,7); //7=everything
						if(g_fAbortDisassembly) 
						{
							printError(g_pFile,"");
							printError(g_pFile,RstrA(IDS_E_DASMABORT));
							fSuccess = FALSE; 
							goto CloseFileAndExit; 
						}
					}
				}
				printLine(g_pFile,"");
				printLine(g_pFile,"// =============================================================");
				printLine(g_pFile,"");
			}

			if(g_uNCA)	_ASSERTE(g_rchCA);
			for(i=1; i<= g_uNCA; i++)
			{
				if(g_rchCA[i] == 0) DumpCustomAttribute(TokenFromRid(i,mdtCustomAttribute),g_pFile,true);
			}
            if(g_fAbortDisassembly) 
            {
                printError(g_pFile,"");
                printError(g_pFile,RstrA(IDS_E_DASMABORT));
                fSuccess = FALSE; 
                goto CloseFileAndExit; 
            }

            /* Third, dump GC/EH info about the native methods, using the IPMap */
            IMAGE_DATA_DIRECTORY *pIPMap;
			if (g_pPELoader->IsPE32())
			{
	            pIPMap = &g_pPELoader->ntHeaders32()->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
			}
			else
			{
	            pIPMap = &g_pPELoader->ntHeaders64()->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
			}
            DWORD        IPMapSize;
            const BYTE * ipmap;
            IPMapSize  = VAL32(pIPMap->Size);
            g_pPELoader->getVAforRVA(VAL32(pIPMap->VirtualAddress), (void **) &ipmap);   

            DumpNativeInfo(ipmap, IPMapSize);

            // If there were "ldptr", dump the .rdata section with labels
            if(g_iPtrCount)
            {
                //first, sort the pointers
                int i,j;
                bool swapped;
                do {
                    swapped = FALSE;

                    for(i = 1; i < g_iPtrCount; i++)
                    {
                        if((*g_pPtrTags)[i-1] > (*g_pPtrTags)[i])
                        {
                            j = (*g_pPtrTags)[i-1];
                            (*g_pPtrTags)[i-1] = (*g_pPtrTags)[i];
                            (*g_pPtrTags)[i] = j;
                            j = (*g_pPtrSize)[i-1];
                            (*g_pPtrSize)[i-1] = (*g_pPtrSize)[i];
                            (*g_pPtrSize)[i] = j;
                            swapped = TRUE;
                        }
                    }
                } while(swapped);

                //second, dump data for each ptr as binarray
                
                IMAGE_SECTION_HEADER *pSecHdr = NULL;
				if(g_pPELoader->IsPE32())
					pSecHdr = IMAGE_FIRST_SECTION(g_pPELoader->ntHeaders32());
				else
					pSecHdr = IMAGE_FIRST_SECTION(g_pPELoader->ntHeaders64());

                DWORD fromPtr,toPtr,limPtr;
                for(j = 0; j < g_iPtrCount; j++)
                {
                    BYTE *pb;
					DWORD dwNumberOfSections;
					if(g_pPELoader->IsPE32())
						dwNumberOfSections = VAL16(g_pPELoader->ntHeaders32()->FileHeader.NumberOfSections);
					else
						dwNumberOfSections = VAL16(g_pPELoader->ntHeaders64()->FileHeader.NumberOfSections);

                    fromPtr = (*g_pPtrTags)[j];
                    for (i=0; i < (int)dwNumberOfSections; i++,pSecHdr++)
                    {
                        if((fromPtr >= VAL32(pSecHdr->VirtualAddress))&&
                            (fromPtr < VAL32(pSecHdr->VirtualAddress)+VAL32(pSecHdr->Misc.VirtualSize))) break;
                    }
                    if(i == (int)dwNumberOfSections)
                    {
                        sprintf(szString,RstrA(IDS_E_ROGUEPTR), fromPtr);
                        printLine(g_pFile,szString);
                        break;
                    }
                    // OK, now we have the section; what about end of BLOB?
                    const char* szTls = (strcmp((char*)(pSecHdr->Name),".tls") ? "D_" : "tls T_");
                    if(j == g_iPtrCount-1) toPtr = VAL32(pSecHdr->VirtualAddress)+VAL32(pSecHdr->Misc.VirtualSize);
                    else
                    {
                        toPtr = (*g_pPtrTags)[j+1];
                        if(toPtr > VAL32(pSecHdr->VirtualAddress)+VAL32(pSecHdr->Misc.VirtualSize))
                            toPtr = VAL32(pSecHdr->VirtualAddress)+VAL32(pSecHdr->Misc.VirtualSize);
                    }
					if(toPtr - fromPtr > (*g_pPtrSize)[j]) toPtr = fromPtr + (*g_pPtrSize)[j];
                    limPtr = toPtr; // at limPtr and after, pad with 0
                    if(limPtr > VAL32(pSecHdr->VirtualAddress)+VAL32(pSecHdr->SizeOfRawData))
                            limPtr = VAL32(pSecHdr->VirtualAddress)+VAL32(pSecHdr->SizeOfRawData);
                    if(fromPtr >= limPtr)
                    {   // uninitialized data
                        sprintf(szString,"%s.data %s%8.8X = int8[%d]",g_szAsmCodeIndent,szTls,fromPtr,toPtr-fromPtr);
                        printLine(g_pFile,szString);
                    }
                    else
                    {   // initialized data
                        sprintf(szString,"%s.data %s%8.8X = bytearray (",g_szAsmCodeIndent,szTls,fromPtr);
                        printLine(g_pFile,szString);
                        sprintf(szString,"%s                ",g_szAsmCodeIndent);
                        pb =  g_pPELoader->base()
                                + VAL32(pSecHdr->PointerToRawData) 
                                + fromPtr - VAL32(pSecHdr->VirtualAddress);
                        // now fromPtr is the beginning of the BLOB, and toPtr is [exclusive] end of it
                        // dump the sucker!
                        DumpHexbytes(szString, pb, fromPtr, toPtr, limPtr);
                    }
                }
            }
CloseNamespace:
            if(strlen(g_szNamespace))
            {
                if(g_szAsmCodeIndent[0]) g_szAsmCodeIndent[strlen(g_szAsmCodeIndent)-2] = 0;
                sprintf(szString,"%s} // end of namespace %s",g_szAsmCodeIndent, ProperName(g_szNamespace));
                printLine(g_pFile,szString);
                printLine(g_pFile,"");

            }
            printLine(g_pFile,RstrA(IDS_E_DASMOK));
			fSuccess = TRUE;
        }
		fSuccess = TRUE;
		if(g_pFile) // dump .RES file (if any), if not to console
		{
			WCHAR wzResFileName[2048], *pwc;
			memset(wzResFileName,0,sizeof(wzResFileName));
			WszMultiByteToWideChar(CP_UTF8,0,g_szOutputFile,-1,wzResFileName,2048);
			pwc = wcsrchr(wzResFileName,L'.');
			if(pwc == NULL) pwc = &wzResFileName[wcslen(wzResFileName)];
			wcscpy(pwc,L".res");
			DWORD ret = DumpResourceToFile(wzResFileName);
			switch(ret)
			{
				case 0: szString[0] = 0; break;
				case 1: sprintf(szString,"// WARNING: Created Win32 resource file %ls", wzResFileName); break;
				case 0xEFFFFFFF: sprintf(szString,"// ERROR: Unable to open file %ls", wzResFileName); break;
				case 0xFFFFFFFF: sprintf(szString,"// ERROR: Unable access Win32 resources"); break;
			}
			if(szString[0]) printError(g_pFile,szString);
		}

CloseFileAndExit:
        if(g_pFile)
		{
			fclose(g_pFile);
	        g_pFile = NULL;
		}
    }

exit:
    return fSuccess;
}

