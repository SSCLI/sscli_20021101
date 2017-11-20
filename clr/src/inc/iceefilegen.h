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
/*****************************************************************************
 **                                                                         **
 ** ICeeFileGen.h - code generator interface.                               **
 **                                                                         **
 *****************************************************************************/


#ifndef _ICEEFILEGEN_H_
#define _ICEEFILEGEN_H_

#include <ole2.h>
#include <time.h> // time_t
#include "cor.h"

class ICeeFileGen;

typedef void *HCEEFILE;

EXTERN_C HRESULT __stdcall CreateICeeFileGen(ICeeFileGen **ceeFileGen); // call this to instantiate
EXTERN_C HRESULT __stdcall DestroyICeeFileGen(ICeeFileGen **ceeFileGen); // call this to delete

class ICeeFileGen {
  public:
    virtual HRESULT CreateCeeFile(HCEEFILE *ceeFile); // call this to instantiate

    virtual HRESULT EmitMetaData (HCEEFILE ceeFile, IMetaDataEmit *emitter, mdScope scope);
    virtual HRESULT EmitLibraryName (HCEEFILE ceeFile, IMetaDataEmit *emitter, mdScope scope);
    virtual HRESULT EmitMethod ();
    virtual HRESULT GetMethodRVA (HCEEFILE ceeFile, ULONG codeOffset, ULONG *codeRVA); 
    virtual HRESULT EmitSignature ();

    virtual HRESULT EmitString (HCEEFILE ceeFile,LPWSTR strValue, ULONG *strRef);
    virtual HRESULT GenerateCeeFile (HCEEFILE ceeFile);

    virtual HRESULT SetOutputFileName (HCEEFILE ceeFile, LPWSTR outputFileName);
    virtual HRESULT GetOutputFileName (HCEEFILE ceeFile, LPWSTR *outputFileName);

    virtual HRESULT SetResourceFileName (HCEEFILE ceeFile, LPWSTR resourceFileName);
    virtual HRESULT GetResourceFileName (HCEEFILE ceeFile, LPWSTR *resourceFileName);

    virtual HRESULT SetImageBase(HCEEFILE ceeFile, size_t imageBase);

    virtual HRESULT SetSubsystem(HCEEFILE ceeFile, DWORD subsystem, DWORD major, DWORD minor);

    virtual HRESULT SetEntryClassToken ();
    virtual HRESULT GetEntryClassToken ();

    virtual HRESULT SetEntryPointDescr ();
    virtual HRESULT GetEntryPointDescr ();

    virtual HRESULT SetEntryPointFlags ();
    virtual HRESULT GetEntryPointFlags ();

    virtual HRESULT SetDllSwitch (HCEEFILE ceeFile, BOOL dllSwitch);
    virtual HRESULT GetDllSwitch (HCEEFILE ceeFile, BOOL *dllSwitch);

    virtual HRESULT SetLibraryName (HCEEFILE ceeFile, LPWSTR LibraryName);
    virtual HRESULT GetLibraryName (HCEEFILE ceeFile, LPWSTR *LibraryName);

    virtual HRESULT SetLibraryGuid (HCEEFILE ceeFile, LPWSTR LibraryGuid);

    virtual HRESULT DestroyCeeFile(HCEEFILE *ceeFile); // call this to instantiate

    virtual HRESULT GetSectionCreate (HCEEFILE ceeFile, const char *name, DWORD flags, HCEESECTION *section);
    virtual HRESULT GetIlSection (HCEEFILE ceeFile, HCEESECTION *section);
    virtual HRESULT GetRdataSection (HCEEFILE ceeFile, HCEESECTION *section);

    virtual HRESULT GetSectionDataLen (HCEESECTION section, ULONG *dataLen);
    virtual HRESULT GetSectionBlock (HCEESECTION section, ULONG len, ULONG align=1, void **ppBytes=0);
    virtual HRESULT TruncateSection (HCEESECTION section, ULONG len);
    virtual HRESULT AddSectionReloc (HCEESECTION section, ULONG offset, HCEESECTION relativeTo, CeeSectionRelocType relocType);

    // deprecated: use SetDirectoryEntry instead
    virtual HRESULT SetSectionDirectoryEntry (HCEESECTION section, ULONG num);

    virtual HRESULT CreateSig ();
    virtual HRESULT AddSigArg ();
    virtual HRESULT SetSigReturnType ();
    virtual HRESULT SetSigCallingConvention ();
    virtual HRESULT DeleteSig ();

    virtual HRESULT SetEntryPoint (HCEEFILE ceeFile, mdMethodDef method);
    virtual HRESULT GetEntryPoint (HCEEFILE ceeFile, mdMethodDef *method);

    virtual HRESULT SetComImageFlags (HCEEFILE ceeFile, DWORD mask);
    virtual HRESULT GetComImageFlags (HCEEFILE ceeFile, DWORD *mask);

    // get IMapToken interface for tracking mapped tokens
    virtual HRESULT GetIMapTokenIface(HCEEFILE ceeFile, IMetaDataEmit *emitter, IUnknown **pIMapToken);
    virtual HRESULT SetDirectoryEntry (HCEEFILE ceeFile, HCEESECTION section, ULONG num, ULONG size, ULONG offset = 0);

    virtual HRESULT EmitMetaDataEx (HCEEFILE ceeFile, IMetaDataEmit *emitter); 
    virtual HRESULT EmitLibraryNameEx (HCEEFILE ceeFile, IMetaDataEmit *emitter);
    virtual HRESULT GetIMapTokenIfaceEx(HCEEFILE ceeFile, IMetaDataEmit *emitter, IUnknown **pIMapToken);

    virtual HRESULT EmitMacroDefinitions(HCEEFILE ceeFile, void *pData, DWORD cData);
    virtual HRESULT CreateCeeFileFromICeeGen(ICeeGen *pFromICeeGen, HCEEFILE *ceeFile); // call this to instantiate

    virtual HRESULT SetManifestEntry(HCEEFILE ceeFile, ULONG size, ULONG offset);

    virtual HRESULT SetEnCRVABase(HCEEFILE ceeFile, ULONG dataBase, ULONG rdataBase);
    virtual HRESULT GenerateCeeMemoryImage (HCEEFILE ceeFile, void **ppImage);

	virtual HRESULT ComputeSectionOffset(HCEESECTION section, char *ptr,
										 unsigned *offset);

	virtual HRESULT ComputeOffset(HCEEFILE file, char *ptr,
								  HCEESECTION *pSection, unsigned *offset);

	virtual HRESULT GetCorHeader(HCEEFILE ceeFile, 
								 IMAGE_COR20_HEADER **header);

    virtual HRESULT LinkCeeFile (HCEEFILE ceeFile);
    virtual HRESULT FixupCeeFile (HCEEFILE ceeFile);

    virtual HRESULT GetSectionRVA (HCEESECTION section, ULONG *rva);

	virtual HRESULT ComputeSectionPointer(HCEESECTION section, ULONG offset,
										  char **ptr);

    virtual HRESULT SetObjSwitch (HCEEFILE ceeFile, BOOL objSwitch);
    virtual HRESULT GetObjSwitch (HCEEFILE ceeFile, BOOL *objSwitch);
    virtual HRESULT SetVTableEntry(HCEEFILE ceeFile, ULONG size, ULONG offset);

    virtual HRESULT SetStrongNameEntry(HCEEFILE ceeFile, ULONG size, ULONG offset);

		// Emit the data.  If 'section != 0, it will put the data in 'buffer'.  This
		// buffer is assumed to be in 'section' at 'offset' and of size 'buffLen'
		// (should use GetSaveSize to insure that buffer is big enough
    virtual HRESULT EmitMetaDataAt (HCEEFILE ceeFile, IMetaDataEmit *emitter, HCEESECTION section, DWORD offset, BYTE* buffer, unsigned buffLen);

    virtual HRESULT GetFileTimeStamp (HCEEFILE ceeFile, time_t *pTimeStamp);

    // Add a notification handler. If it implements an interface that
    // the ICeeFileGen understands, S_OK is returned. Otherwise,
    // E_NOINTERFACE.
    virtual HRESULT AddNotificationHandler(HCEEFILE ceeFile,
                                           IUnknown *pHandler);

    virtual HRESULT SetFileAlignment(HCEEFILE ceeFile, ULONG fileAlignment);

    virtual HRESULT ClearComImageFlags (HCEEFILE ceeFile, DWORD mask);
};

#endif
