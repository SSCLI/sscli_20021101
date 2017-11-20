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
// dres.cpp
// 
// Win32 Resource extractor
//
#include <stdio.h>
#include <stdlib.h>
#include <utilcode.h>
#include "debugmacros.h"
#include "corpriv.h"
#include "dasmenum.hpp"
#include "dasmgui.h"
#include "formattype.h"
#include "dis.h"
#include "resource.h"
#include "ilformatter.h"
#include "outstring.h"
#include "utilcode.h" // for CQuickByte

#include "ceeload.h"
#include "dynamicarray.h"
extern IMAGE_COR20_HEADER *    g_CORHeader;
extern IMDInternalImport*      g_pImport;
extern PELoader * g_pPELoader;
extern IMetaDataImport*        g_pPubImport;
extern char g_szAsmCodeIndent[];

struct ResourceHeader
{
	DWORD	dwDataSize;
	DWORD	dwHeaderSize;
	DWORD	dwTypeID;
	DWORD	dwNameID;
	DWORD	dwDataVersion;
	WORD	wMemFlags;
	WORD	wLangID;
	DWORD	dwVersion;
	DWORD	dwCharacteristics;
	ResourceHeader() 
	{ 
		memset(this,0,sizeof(ResourceHeader)); 
		dwHeaderSize = sizeof(ResourceHeader);
		dwTypeID = dwNameID = 0xFFFF;
	};
};

struct ResourceNode
{
	ResourceHeader	ResHdr;
	IMAGE_RESOURCE_DATA_ENTRY DataEntry;
	ResourceNode(DWORD tid, DWORD nid, DWORD lid, PVOID ptr)
	{
		ResHdr.dwTypeID = (tid & 0x80000000) ? tid : (0xFFFF |((tid & 0xFFFF)<<16));
		ResHdr.dwNameID = (nid & 0x80000000) ? nid : (0xFFFF |((nid & 0xFFFF)<<16));
		ResHdr.wLangID = (WORD)lid;
		if(ptr) memcpy(&DataEntry,ptr,sizeof(IMAGE_RESOURCE_DATA_ENTRY));
		ResHdr.dwDataSize = DataEntry.Size;
	};
};

unsigned ulNumResNodes=0;
DynamicArray<ResourceNode*> *g_prResNodePtr = NULL;

#define RES_FILE_DUMP_ENABLED

DWORD	DumpResourceToFile(WCHAR*	wzFileName)
{

	BYTE*	pbResBase;
	FILE*	pF = NULL;
	DWORD ret = 0;
	DWORD	dwResDirRVA;
	DWORD	dwResDirSize;
	
	if (g_pPELoader->IsPE32())
	{
	    IMAGE_OPTIONAL_HEADER32 *pOptHeader = &(g_pPELoader->ntHeaders32()->OptionalHeader);
	
		dwResDirRVA = VAL32(pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress);
		dwResDirSize = VAL32(pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size);
	}
	else
	{
	    IMAGE_OPTIONAL_HEADER64 *pOptHeader = &(g_pPELoader->ntHeaders64()->OptionalHeader);

		dwResDirRVA = VAL32(pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress);
		dwResDirSize = VAL32(pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size);
	}

	if(dwResDirRVA && dwResDirSize)
	{
		ULONG L = (ULONG)wcslen(wzFileName)*3+3;
		char* szFileNameANSI = new char[L];
		memset(szFileNameANSI,0,L);
		WszWideCharToMultiByte(CP_ACP,0,wzFileName,-1,szFileNameANSI,L,NULL,NULL);
		pF = fopen(szFileNameANSI,"wb");
		delete [] szFileNameANSI;
 		if(pF)
		{
			if(g_pPELoader->getVAforRVA(dwResDirRVA, (void **) &pbResBase))
			{
				// First, pull out all resource nodes (tree leaves), see ResourceNode struct
				PIMAGE_RESOURCE_DIRECTORY pirdType = (PIMAGE_RESOURCE_DIRECTORY)pbResBase;
				PIMAGE_RESOURCE_DIRECTORY_ENTRY pirdeType = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pbResBase+sizeof(IMAGE_RESOURCE_DIRECTORY));
				DWORD	dwTypeID;
				unsigned short i,N = VAL16(pirdType->NumberOfNamedEntries)+VAL16(pirdType->NumberOfIdEntries);
				
				for(i=0; i < N; i++, pirdeType++)
				{
					dwTypeID = VAL32(IMAGE_RDE_NAME(pirdeType));
					if(IMAGE_RDE_OFFSET_FIELD(pirdeType, DataIsDirectory))
					{
						BYTE*	pbNameBase = pbResBase + VAL32(IMAGE_RDE_OFFSET_FIELD(pirdeType, OffsetToDirectory));
						PIMAGE_RESOURCE_DIRECTORY pirdName = (PIMAGE_RESOURCE_DIRECTORY)pbNameBase;
						PIMAGE_RESOURCE_DIRECTORY_ENTRY pirdeName = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pbNameBase+sizeof(IMAGE_RESOURCE_DIRECTORY));
						DWORD	dwNameID;
						unsigned short i,N = VAL16(pirdName->NumberOfNamedEntries)+VAL16(pirdName->NumberOfIdEntries);

						for(i=0; i < N; i++, pirdeName++)
						{
							dwNameID = VAL32(IMAGE_RDE_NAME(pirdeName));
							if(IMAGE_RDE_OFFSET_FIELD(pirdeName, DataIsDirectory))
							{
								BYTE*	pbLangBase = pbResBase + VAL32(IMAGE_RDE_OFFSET_FIELD(pirdeName, OffsetToDirectory));
								PIMAGE_RESOURCE_DIRECTORY pirdLang = (PIMAGE_RESOURCE_DIRECTORY)pbLangBase;
								PIMAGE_RESOURCE_DIRECTORY_ENTRY pirdeLang = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pbLangBase+sizeof(IMAGE_RESOURCE_DIRECTORY));
								DWORD	dwLangID;
								unsigned short i,N = VAL32(pirdLang->NumberOfNamedEntries)+VAL16(pirdLang->NumberOfIdEntries);

								for(i=0; i < N; i++, pirdeLang++)
								{
									dwLangID = IMAGE_RDE_NAME(pirdeLang);
									if(IMAGE_RDE_OFFSET_FIELD(pirdeLang, DataIsDirectory))
									{
										_ASSERTE(!"Resource hierarchy exceeds three levels");
									}
									else
									{
										if (g_prResNodePtr == NULL)
										{
											g_prResNodePtr = new DynamicArray<ResourceNode*>;
										}
										(*g_prResNodePtr)[ulNumResNodes++] = new ResourceNode(dwTypeID,dwNameID,dwLangID,pbResBase + VAL32(IMAGE_RDE_OFFSET(pirdeLang)));
									}
								}
							}
							else
							{
								if (g_prResNodePtr == NULL)
								{
									g_prResNodePtr = new DynamicArray<ResourceNode*>;
								}
								(*g_prResNodePtr)[ulNumResNodes++] = new ResourceNode(dwTypeID,dwNameID,0,pbResBase + VAL32(IMAGE_RDE_OFFSET(pirdeName)));
							}
						}
					}
					else
					{
						if (g_prResNodePtr == NULL)
						{
							g_prResNodePtr = new DynamicArray<ResourceNode*>;
						}
						(*g_prResNodePtr)[ulNumResNodes++] = new ResourceNode(dwTypeID,0,0,pbResBase + VAL32(IMAGE_RDE_OFFSET(pirdeType)));
					}
				}
				// OK, all tree leaves are in ResourceNode structs, and ulNumResNodes ptrs are in g_prResNodePtr
				// Dump them to pF
				if(ulNumResNodes)
				{
					BYTE* pbData;
					// Write dummy header
					ResourceHeader	*pRH = new ResourceHeader();
					fwrite(pRH,sizeof(ResourceHeader),1,pF);
					delete pRH;
					DWORD	dwFiller;
					BYTE	bNil[3] = {0,0,0};
					// For each resource write header and data
					for(i=0; i < ulNumResNodes; i++)
					{
						if(g_pPELoader->getVAforRVA(VAL32((*g_prResNodePtr)[i]->DataEntry.OffsetToData), (void **) &pbData))
						{
							fwrite(&((*g_prResNodePtr)[i]->ResHdr),sizeof(ResourceHeader),1,pF);
							fwrite(pbData,VAL32((*g_prResNodePtr)[i]->DataEntry.Size),1,pF);
							dwFiller = VAL32((*g_prResNodePtr)[i]->DataEntry.Size) & 3;
							if(dwFiller)
							{
								dwFiller = 4 - dwFiller;
								fwrite(bNil,dwFiller,1,pF);
							}
						}
						delete (*g_prResNodePtr)[i];
					}
				}
				if (g_prResNodePtr)
				{
					delete g_prResNodePtr;
					g_prResNodePtr = NULL;
				}
				ulNumResNodes = 0;
				ret = 1;
			}// end if got ptr to resource
			else ret = 0xFFFFFFFF;
			if(pF) fclose(pF);
		}// end if file opened
		else ret = 0xEFFFFFFF;
	} // end if there is resource
	else ret = 0;
	return ret;
}
