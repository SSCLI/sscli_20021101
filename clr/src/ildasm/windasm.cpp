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
/************************************************************************************************
 *                                                                                              *
 *  File:    winmain.cpp                                                                        *
 *                                                                                                               *
 *  Purpose: Main program for graphic COM+ 2.0 disassembler ILDASM.exe                          *
 *                                                                                              *
 ************************************************************************************************/


#define INITGUID

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <crtdbg.h>
#include <utilcode.h>
#include <malloc.h>
#include <string.h>
#include <palstartup.h>

#include "dynamicarray.h"

#define DO_DASM_GUI
#include "dasmgui.h"
#include "dasmenum.hpp"
#include "dis.h"
#include "version/__file__.ver"
#include "version/corver.h"
#include "resource.h"

#define MODE_DUMP_ALL               0
#define MODE_DUMP_CLASS             1
#define MODE_DUMP_CLASS_METHOD      2
#define MODE_DUMP_CLASS_METHOD_SIG  3

// All externs are defined in DASM.CPP
extern BOOL                    g_fDumpIL;
extern BOOL                    g_fDumpHeader;
extern BOOL                    g_fDumpAsmCode;
extern BOOL                    g_fDumpTokens;
extern BOOL                    g_fDumpStats;
extern BOOL                    g_fDumpClassList;
extern BOOL                    g_fDumpSummary;
extern BOOL                    g_fDecompile; // still in progress

extern BOOL                    g_fDumpToPerfWriter;

extern BOOL                    g_fShowBytes;
extern BOOL                    g_fShowSource;
extern BOOL                    g_fInsertSourceLines;
extern BOOL                    g_fTryInCode;
extern BOOL                    g_fQuoteAllNames;
extern BOOL                    g_fTDC;
extern BOOL                    g_fShowCA;

extern BOOL                    g_AddContextfulAttrib;
extern DynamicArray<char*>     g_ContextfulClasses;
extern int                     g_cContextfulClasses;
extern BOOL                    g_AddMarshaledByRefAttrib;
extern DynamicArray<char*>     g_MarshaledByRefClasses;
extern int                     g_cMarshaledByRefClasses;

extern char                    g_pszClassToDump[];
extern char                    g_pszMethodToDump[];
extern char                    g_pszSigToDump[];

extern char                    g_szAsmCodeIndent[];

extern DWORD                   g_Mode;

extern char		       g_szInputFile[]; // in UTF-8
extern char		       g_szOutputFile[]; // in UTF-8
extern char*		       g_pszObjFileName;
extern FILE*                   g_pFile;
extern BOOL                    g_fCheckOwnership;
extern char*                   g_pszOwner;

extern BOOL                 g_fLimitedVisibility;
extern BOOL                 g_fHidePub;
extern BOOL                 g_fHidePriv;
extern BOOL                 g_fHideFam;
extern BOOL                 g_fHideAsm;
extern BOOL                 g_fHideFAA;
extern BOOL                 g_fHideFOA;
extern BOOL                 g_fHidePrivScope;
extern unsigned		    g_uCodePage;
extern BOOL		    g_fOnUnicode;


#include "../tools/metainfo/mdinfo.h"
extern BOOL                 g_fDumpMetaInfo;
extern ULONG                g_ulMetaInfoFilter;
HANDLE			    hConsoleOut=NULL;
HANDLE			    hConsoleErr=NULL;
// These are implemented in DASM.CPP:
BOOL Init();
void Uninit();
void Cleanup();
void DumpMetaInfo(char* pszFileName, char* pszObjFileName, void* GUICookie);


void PrintLogo()
{
    printf("Microsoft (R) Shared Source CLI IL Disassembler.  Version " VER_FILEVERSION_STR);
    printf("\n%s\n\n", VER_LEGALCOPYRIGHT_DOS_STR);
}
void SyntaxCon()
{
	DWORD l;

	for(l=IDS_USAGE_01; l<= IDS_USAGE_23; l++) printf(Rstr(l));
	{
		for(l=IDS_USAGE_24; l<= IDS_USAGE_32; l++) printf(Rstr(l));
#ifdef _DEBUG
		for(l=IDS_USAGE_34; l<= IDS_USAGE_36; l++) printf(Rstr(l));
#endif
		for(l=IDS_USAGE_37; l<= IDS_USAGE_39; l++) printf(Rstr(l));
	}
	for(l=IDS_USAGE_41; l<= IDS_USAGE_42; l++) printf(Rstr(l));

}

char* CheckForDQuotes(char* sz)
{
	char* ret = sz;
	if(*sz == '"')
	{
		ret++;
		sz[strlen(sz)-1] = 0;
	}
	return ret;
}

char* EqualOrColon(char* szArg)
{
	char* pchE = strchr(szArg,'=');
	char* pchC = strchr(szArg,':');
	char* ret;
	if(pchE == NULL) ret = pchC;
	else if(pchC == NULL) ret = pchE;
	else ret = (pchE < pchC)? pchE : pchC;
	return ret;
}

int ProcessOneArg(char* szArg, char** ppszObjFileName)
{
    char		szOpt[128];
	if(strlen(szArg) == 0) return 0;
    if ((strcmp(szArg, "/?") == 0) || (strcmp(szArg, "-?") == 0)) return 1;

	if((szArg[0] == '/') || (szArg[0] == '-'))
    {
        strncpy(szOpt, &szArg[1],10);
        szOpt[3] = 0;
        if (_stricmp(szOpt, "dec") == 0)
        {
            g_fDecompile = TRUE;
        }
        else if (_stricmp(szOpt, "hea") == 0)
        {
            g_fDumpHeader = TRUE;
        }
        else if (_stricmp(szOpt, "adv") == 0)
        {
            g_fTDC = TRUE;
        }
        else if (_stricmp(szOpt, "tok") == 0)
        {
            g_fDumpTokens = TRUE;
        }
        else if (_stricmp(szOpt, "noi") == 0)
        {
            g_fDumpAsmCode = FALSE;
        }
        else if (_stricmp(szOpt, "noc") == 0)
        {
            g_fShowCA = FALSE;
        }
        else if (_stricmp(szOpt, "not") == 0)
        {
            g_fTryInCode = FALSE;
        }
        else if (_stricmp(szOpt, "raw") == 0)
        {
            g_fTryInCode = FALSE;
        }
        else if (_stricmp(szOpt, "byt") == 0)
        {
            g_fShowBytes = TRUE;
        }
        else if (_stricmp(szOpt, "sou") == 0)
        {
            g_fShowSource = TRUE;
        }
        else if (_stricmp(szOpt, "lin") == 0)
        {
            g_fInsertSourceLines = TRUE;
        }
        else if ((_stricmp(szOpt, "sta") == 0)&&g_fTDC)
        {
            g_fDumpStats = g_fTDC;
        }
        else if ((_stricmp(szOpt, "cla") == 0)&&g_fTDC)
        {
            g_fDumpClassList = g_fTDC;
        }
        else if (_stricmp(szOpt, "sum") == 0)
        {
            g_fDumpSummary = TRUE;
        }
        else if (_stricmp(szOpt, "per") == 0) 
        {
            g_fDumpToPerfWriter = TRUE;
        }
        else if (_stricmp(szOpt, "pub") == 0)
        {
            g_fLimitedVisibility = TRUE;
            g_fHidePub = FALSE;
        }
#ifdef _DEBUG
        else if (_stricmp(szOpt, "pre") == 0)
        {
            g_fPrettyPrint = TRUE;
        }
#endif
        else if (_stricmp(szOpt, "vis") == 0)
        {
            char *pc = EqualOrColon(szArg);
            char *pStr;
            if(pc == NULL) return -1;
            do {
                pStr = pc+1;
				pStr = CheckForDQuotes(pStr);
                if((pc = strchr(pStr,'+'))) *pc=0;
                if     (!_stricmp(pStr,"pub")) g_fHidePub = FALSE;
                else if(!_stricmp(pStr,"pri")) g_fHidePriv = FALSE;
                else if(!_stricmp(pStr,"fam")) g_fHideFam = FALSE;
                else if(!_stricmp(pStr,"asm")) g_fHideAsm = FALSE;
                else if(!_stricmp(pStr,"faa")) g_fHideFAA = FALSE;
                else if(!_stricmp(pStr,"foa")) g_fHideFOA = FALSE;
                else if(!_stricmp(pStr,"psc")) g_fHidePrivScope = FALSE;
            } while(pc);
            g_fLimitedVisibility = g_fHidePub || g_fHidePriv || g_fHideFam || g_fHideFAA 
                                                || g_fHideFOA || g_fHidePrivScope;
        }
        else if (_stricmp(szOpt, "quo") == 0)
        {
            g_fQuoteAllNames = TRUE;
        }
        else if (_stricmp(szOpt, "utf") == 0)
        {
            g_uCodePage = CP_UTF8;
        }
        else if (_stricmp(szOpt, "uni") == 0)
        {
            g_uCodePage = 0xFFFFFFFF;
        }
#ifdef _DEBUG
        else if (_stricmp(szOpt, "@#$") == 0)
        {
            g_fCheckOwnership = FALSE;
        }
#endif
        else if (_stricmp(szOpt, "all") == 0)
        {
            g_fDumpStats = g_fTDC;
            g_fDumpHeader = TRUE;
            g_fShowBytes = TRUE;
            g_fDumpClassList = g_fTDC;
            g_fDumpTokens = TRUE;
        }
        else if (_stricmp(szOpt, "ite") == 0)
        {
            char *pStr = EqualOrColon(szArg);
            char *p, *q;
            if(pStr == NULL) return -1;
            pStr++;
			pStr = CheckForDQuotes(pStr);
            // treat it as meaning "dump only class X" or "class X method Y"
            p = strchr(pStr, ':');

            if (p == NULL)
            {
                // dump one class
                g_Mode = MODE_DUMP_CLASS;
                strcpy(g_pszClassToDump, pStr);
            }
            else
            {
                *p++ = '\0';
                if (*p != ':') return -1;

                strcpy(g_pszClassToDump, pStr);

                p++;

                q = strchr(p, '(');
                if (q == NULL)
                {
                    // dump class::method
                    g_Mode = MODE_DUMP_CLASS_METHOD;
                    strcpy(g_pszMethodToDump, p);
                }
                else
                {
                    // dump class::method(sig)
                    g_Mode = MODE_DUMP_CLASS_METHOD_SIG;
                    *q = '\0';
                    strcpy(g_pszMethodToDump, p);
                    // get rid of external parentheses:
					q++;
                    strcpy(g_pszSigToDump, q);
					g_pszSigToDump[strlen(g_pszSigToDump)-1]=0;
                }
            }
        }
        else if ((_stricmp(szOpt, "met") == 0)&&g_fTDC)
        {
            if(g_fTDC)
            {
                char *pStr = EqualOrColon(szArg);
                g_fDumpMetaInfo = TRUE;
                if(pStr)
                {
                    char szOptn[64];
                    strncpy(szOptn, pStr+1,10);
                    szOptn[3] = 0; // recognize metainfo specifier by first 3 chars
                    if	   (_stricmp(szOptn, "hex") == 0) g_ulMetaInfoFilter |= MDInfo::dumpMoreHex;
                    else if(_stricmp(szOptn, "csv") == 0) g_ulMetaInfoFilter |= MDInfo::dumpCSV;
                    else if(_stricmp(szOptn, "mdh") == 0) g_ulMetaInfoFilter |= MDInfo::dumpHeader;
#ifdef _DEBUG
                    else if(_stricmp(szOptn, "raw") == 0) g_ulMetaInfoFilter |= MDInfo::dumpRaw;
                    else if(_stricmp(szOptn, "hea") == 0) g_ulMetaInfoFilter |= MDInfo::dumpRawHeaps;
                    else if(_stricmp(szOptn, "sch") == 0) g_ulMetaInfoFilter |= MDInfo::dumpSchema;
#endif
                    else if(_stricmp(szOptn, "unr") == 0) g_ulMetaInfoFilter |= MDInfo::dumpUnsat;
                    else if(_stricmp(szOptn, "val") == 0) g_ulMetaInfoFilter |= MDInfo::dumpValidate;
                    else if(_stricmp(szOptn, "sta") == 0) g_ulMetaInfoFilter |= MDInfo::dumpStats;
                    else return -1;
                }
            }
        }
        else if (_stricmp(szOpt, "out") == 0)
        {
            char *pStr = EqualOrColon(szArg);
            if(pStr == NULL) return -1;
            pStr++;
			pStr = CheckForDQuotes(pStr);
            if(*pStr == 0) return -1;
            if(_stricmp(pStr,"con"))
            {
                strncpy(g_szOutputFile, pStr,511);
                g_szOutputFile[511] = 0;
            }
        }
        else
		{
#ifdef PLATFORM_UNIX
            if (szArg[0] == L'/')
            {
                // On BSD it could be a fully qualified file that starts with '/'
                // Not a switch, so it might be a FileName
                goto AssumeFileName;
            }
            else
#endif
            {
			    PrintLogo();
			    printf("INVALID COMMAND LINE OPTION: %s\n\n",szArg);
			    return -1;
            }
		}
    }
    else
    {
#ifdef PLATFORM_UNIX
AssumeFileName:
#endif
        if(g_szInputFile[0])
		{
			PrintLogo();
			printf("MULTIPLE INPUT FILES SPECIFIED\n\n");
			return -1; // check if it was already specified
		}
		szArg = CheckForDQuotes(szArg);
        strncpy(g_szInputFile, szArg,511);
        g_szInputFile[511] = 0;
    }
    return 0;
}


FILE* OpenOutput(char* szFileName)
{
	FILE*	pFile = NULL;
	ULONG32 L = (ULONG32) strlen(szFileName)+16;
	WCHAR* wzFileName = new WCHAR[L];
	memset(wzFileName,0,L*sizeof(WCHAR));
	WszMultiByteToWideChar(CP_UTF8,0,szFileName,-1,wzFileName,L);
	if(g_fOnUnicode)
	{
		if(g_uCodePage == 0xFFFFFFFF) pFile = _wfopen(wzFileName,L"wb");
        else pFile = _wfopen(wzFileName,L"wt");
	}
	else
	{
		char* szFileNameANSI = new char[L*2];
		memset(szFileNameANSI,0,L*2);
		WszWideCharToMultiByte(CP_ACP,0,wzFileName,-1,szFileNameANSI,L*2,NULL,NULL);

		if(g_uCodePage == 0xFFFFFFFF) pFile = fopen(szFileNameANSI,"wb");
        else pFile = fopen(szFileNameANSI,"wt");
		delete [] szFileNameANSI;
	}
	delete [] wzFileName;
	return pFile;
}

int __cdecl main(int argc, char **argv)
{
	int		iCommandLineParsed = 0;

    if (!PAL_RegisterLibraryW(L"rotor_palrt") ||
        !PAL_RegisterLibraryW(L"sscoree"))
    {
        printError(NULL, "Unable to register libraries!");
        exit(1);
    }

    g_fOnUnicode = OnUnicodeSystem();
    
    g_pszClassToDump[0]=0;
    g_pszMethodToDump[0]=0;
    g_pszSigToDump[0]=0;
    memset(g_szInputFile,0,512);
    memset(g_szOutputFile,0,512);
    g_fTDC = TRUE;

#undef GetCommandLineW
#undef CreateProcessW
    g_pszObjFileName = NULL;
	
    g_szAsmCodeIndent[0] = 0;
	hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hConsoleErr = GetStdHandle(STD_ERROR_HANDLE);

    for(int i=1; i < argc; i++)
    {
        if((iCommandLineParsed = ProcessOneArg(argv[i],&g_pszObjFileName))) 
            break;
    }

	if(hConsoleOut != INVALID_HANDLE_VALUE) //First pass: console
	{
		if(iCommandLineParsed)
		{
			if(iCommandLineParsed > 0) PrintLogo();
			SyntaxCon();
			exit((iCommandLineParsed == 1) ? 0 : 1);
		}
		{
			DWORD	exitCode = 1;
			if(g_szInputFile[0] == 0) 
			{
				SyntaxCon();
				exit(1);
			}
			g_pFile = NULL;
			if(g_szOutputFile[0])
			{
				g_pFile = OpenOutput(g_szOutputFile);
				if(g_pFile == NULL)
				{
					char sz[1024];
					sprintf(sz,"Unable to open '%s' for output.",	g_szOutputFile);
					g_uCodePage = CP_ACP;
					printError(NULL,sz);
					exit(1);
				}
			}
			else // console output -- force the code page to ANSI
			{
				g_uCodePage = CP_ACP;
			}
			if (Init() == TRUE)
			{
				exitCode = DumpFile(g_szInputFile) ? 0 : 1;
				Cleanup();
			}
			Uninit();
			exit(exitCode);
		}
	}
    return 0;
}

