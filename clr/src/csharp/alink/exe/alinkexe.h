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
// File: alinkexe.h
// 
// standard header for ALink front end.
// ===========================================================================

#ifndef __alinkexe_h__
#define __alinkexe_h__

struct resource {
    LPWSTR pszIdent;    // Do this first so we can use _wcsicmp as comparison function
    LPWSTR pszSource;
    bool   bVis;
    bool   bEmbed;
    union {
        LPWSTR pszTarget;
        DWORD  dwOffset;
    };
};

struct addfile {
    LPWSTR pszSource;
    LPWSTR pszTarget;
    addfile() { pszSource = pszTarget = NULL; }
};

enum TargetType {
    ttDll,
    ttConsole,
    ttWinApp
};

struct _options {
    AssemblyOptions opt;
    LPCWSTR         ShortName;
    LPCWSTR         LongName;
    LPCWSTR         CA;
    int             HelpId;
    VARTYPE         vt;
    BYTE            flag;
};

struct _optionused {
#define OPTION(name, type, flags, id, shortname, longname, CA) bool b_##name : 1;
#include "options.h"
    bool get(AssemblyOptions opt) {
        switch (opt) {
#define OPTION(name, type, flags, id, shortname, longname, CA) case name: return b_##name;
#include "options.h"
            default:
                break;
        }
        return false;
    }
    void set(AssemblyOptions opt, bool val = true) {
        switch (opt) {
#define OPTION(name, type, flags, id, shortname, longname, CA) case name: b_##name = val; break;
#include "options.h"
            default:
                break;
        }
    }

    _optionused() {
        memset(this, 0, sizeof(*this));
    }
    static _optionused *GetOptionUsed()
    {
        if (m_pOptions == NULL)
        {
            m_pOptions = new _optionused();
        }
        return m_pOptions;
    }
    static void FreeOptionUsed()
    {
        if (m_pOptions != NULL)
        {
            delete m_pOptions;
            m_pOptions = NULL;
        }
    }
    static _optionused *m_pOptions;
};

enum ERRORKIND {
    ERROR_FATAL,
    ERROR_ERROR,
    ERROR_WARNING
};

typedef tree<LPWSTR> b_tree;
typedef list<WCHAR*> WStrList;
typedef list<addfile> FileList;
typedef list<resource*> ResList;

// Options
extern LPWSTR          g_szAssemblyFile;
extern LPWSTR          g_szAppMain;
extern LPWSTR          g_szCopyAssembly;
extern VARIANT         g_Options[optLastAssemOption];
extern FileList       *g_pFileListHead;
extern FileList      **g_ppFileListEnd;
extern ResList        *g_pResListHead;
extern ResList       **g_ppResListEnd;
extern DWORD           g_dwDllBase;
extern DWORD           g_dwCodePage;
extern bool            g_bNoLogo;
extern bool            g_bShowHelp;
extern bool            g_bTime;
extern bool            g_bFullPaths;
extern bool            g_bHadError;
extern bool            g_bMerge;
extern bool            g_bInstall;
extern bool            g_bMake;
extern TargetType      g_Target;
// If non-NULL - the following is the output file for a repro case.
extern FILE           *g_reproFile;
extern bool            g_reproOutputEnabled;
extern bool            g_reproOutputBannerPrinted;
extern bool            g_fullPaths;
extern bool            g_bUnicode;
extern bool            g_bOnErrorReported;
extern HINSTANCE       g_hinstMessages;


#define OPTION(name, type, flags, id, shortname, longname, CA) {name, shortname, longname, CA, id, type, flags},
const _options Options [] = {
#include "options.h"
    {optLastAssemOption,   L"", L"", L"", IDS_InternalError, VT_EMPTY, 0x00}
};

// Linker error numbers.
#define ERRORDEF(num, level, name, strid) name,
enum ERRORIDS {
    ERR_NONE,
    #include "errors.h"
    ERR_COUNT
};
#undef ERRORDEF

// Information about each error or warning.
struct ERROR_INFO {
    short number;       // error number
    short level;        // warning level; 0 means error
    int   resid;        // resource id.
};
#define ERRORDEF(num, level, name, strid) {num, level, strid},
static ERROR_INFO errorInfo[ERR_COUNT] = {
    {0000, -1, 0},          // ERR_NONE - no error.
    #include "errors.h"
};
#undef ERRORDEF

enum ILCODE {
#define OPDEF(id, name, pop, push, operand, type, len, b1, b2, cf) id = ((b1 << 8) | b2),
#include "opcode.def"
#undef OPDEF
};


void __cdecl PrintReproInfo(unsigned id, ...);
void GetFileVersion(HINSTANCE hinst, CHAR *szVersion);
void GetCLRVersion(CHAR *szVersion, int maxBuf);
ICeeFileGen* CreateCeeFileGen(IMetaDataDispenserEx *pDisp);
void DestroyCeeFileGen(ICeeFileGen *ceefilegen);
HRESULT SetDispenserOptions(IMetaDataDispenserEx * dispenser);
HRESULT SetAssemblyOptions(mdAssembly assemID, IALink *pLinker);
void MakeAssembly(ICeeFileGen *pFileGen, IMetaDataDispenserEx *pDisp, IALink *pLinker, IMetaDataError *pErrHandler);
HRESULT CopyAssembly(mdAssembly assemID, IALink * pLinker);
HRESULT ImportCA( const BYTE * pvData, DWORD cbSize, AssemblyOptions opt, mdAssembly assemID, IALink *pLinker);
void AddVariantToList( VARIANT &var, VARIANT &val);
bool GetFullFileName(LPWSTR szFilename, DWORD cchFilename);
bool GetFullFileName(LPCWSTR szSource, LPWSTR szFilename, DWORD cchFilename);
HRESULT FindMain( IMetaDataImport *pImport, IMetaDataEmit *pEmit, LPCWSTR pszTypeName, LPCWSTR pszMethodName, mdMemberRef &tkMain, mdMethodDef &tkNewMain, int &countArgs, bool &bHadBogusMain);
HRESULT MakeMain(ICeeFileGen *pFileGen, HCEEFILE file, HCEESECTION ilSection, IMetaDataEmit *pEmit, mdMethodDef tkMain, mdMemberRef tkEntryPoint, int iArgs);
void ShowFormattedError( int id, LPCWSTR text);
void ShowErrorId(int id, ERRORKIND kind);
void _cdecl ShowErrorIdString(int id, ERRORKIND kind, ...);
void _cdecl ShowErrorIdLocation(int id, ERRORKIND kind, LPCWSTR szLocation, ...);
LPCWSTR ErrorHR(HRESULT hr);
LPCWSTR ErrorLastError();
HRESULT CheckHR(HRESULT hr);
void AddBinaryFile(LPCWSTR filename);


class CErrors :
    public IMetaDataError
{
public:
    STDMETHOD(QueryInterface)(const GUID &iid, void** ppUnk) {
        if (iid == IID_IMetaDataError) {
            *ppUnk = (IMetaDataError*)this;
            return S_OK;
        } else {
            *ppUnk = NULL;
            return E_FAIL;
        }
    }
    STDMETHOD_(DWORD, AddRef)() { return 1; }
    STDMETHOD_(DWORD, Release)() { return 1; }

    // Interface methods here
    STDMETHOD(OnError)(HRESULT hr, mdToken tkLocation);
};

#endif //__alinkexe_h__

