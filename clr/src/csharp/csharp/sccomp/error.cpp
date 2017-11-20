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
// File: error.cpp
//
// Error handling for the compiler
// ===========================================================================

#include "stdafx.h"

#include "compiler.h"
#include "controller.h"
#include "srcmod.h"
#include "srcdata.h"
#include <stdarg.h>



/*
 * Information about each error or warning.
 */
struct ERROR_INFO {
    short number;       // error number
    short level;        // warning level; 0 means error
    int   resid;        // resource id.
};

#define ERRORDEF(num, level, name, strid) {num, level, strid},

static const ERROR_INFO errorInfo[ERR_COUNT] = {
    {0000, -1, 0},          // ERR_NONE - no error.
    #include "errors.h"
};

#undef ERRORDEF

#ifdef DEBUG
// DEBUG-only function to check the integrity of the error info -- 
// check that all error messages exist, and that no error numbers or
// messages are duplicated. This detects common mistakes when editing
// errors.h
void CheckErrorMessageInfo(MEMHEAP * heap, bool dumpAll)
{
    int * messageIds = (int *) heap->AllocZero(sizeof(int *) * 0x10000);
    int * errorNos = (int *) heap->AllocZero(sizeof(int *) * 0x10000);
    char dummy[256];

    for (int iErr = 1; iErr < ERR_COUNT; ++iErr) {
        if (iErr != ERR_MetadataCantOpenFile &&  // a few messages are duplicated intentionally.
            iErr != ERR_DeprecatedSymbolStr &&
            errorInfo[iErr].resid != IDS_FeatureDeprecated) 
        {
            ASSERT(messageIds[errorInfo[iErr].resid] == 0); // duplicate message ID
            messageIds[errorInfo[iErr].resid] = iErr;
        }

        if (iErr != FTL_NoMessagesDLL) // intentionally no mesage for this one!
            ASSERT(LoadString(hModuleMessages, errorInfo[iErr].resid, dummy, sizeof(dummy)) != 0); // missing message

        if (iErr != ERR_MetadataCantOpenFile) { // intentional duplicate!
            ASSERT(errorNos[errorInfo[iErr].number] == 0); // duplicate error number
            errorNos[errorInfo[iErr].number] = iErr;
        }

        if (dumpAll) {
            dummy[0] = '\0';
            LoadString(hModuleMessages, errorInfo[iErr].resid, dummy, sizeof(dummy));
            printf("%d\t%d\t%s\n", errorInfo[iErr].number, errorInfo[iErr].level, dummy);
        }
    }

    heap->Free(messageIds);
    heap->Free(errorNos);
}

#endif //DEBUG


////////////////////////////////////////////////////////////////////////////////
// CAutoFree -- helper class to allocate memory from the heap if possible, and
// free on destruction.  Intended to use in possible low-stack conditions; if
// allocation fails, attempt allocation on the stack, as in the following:
//
// CAutoFree<WCHAR>    f;
// PWSTR               p;
//
// p = SAFEALLOC (f, 128);
//

template <class T>
class CAutoFree
{
    T   *m_pMem;
public:
    CAutoFree () : m_pMem(NULL) {}
    ~CAutoFree () { Free(); }
    void    Free() { if (m_pMem) { VSFree (m_pMem); m_pMem = NULL; } }
    bool    AFAlloc(int iSize) { Free(); return !!(m_pMem = (T *)VSAlloc (iSize * sizeof (T))); }
    T       *Mem() { return m_pMem; }
    T       *Cast(void *p) { return (T*)p; }
    long    ElementSize () { return sizeof (T); }
};

#define SAFEALLOC(mem,size) (mem.AFAlloc(size) ? mem.Mem() : mem.Cast(_alloca(size*mem.ElementSize())))

/*
 * Helper function and load and format a message. Uses Unicode
 * APIs, so it won't work on Win95/98.
 */
static bool LoadAndFormatW(int resid, va_list args, LPWSTR buffer, int cchMax)
{
    CAutoFree<WCHAR>    mem;
    LPWSTR              formatString = SAFEALLOC (mem, cchMax);
    return (LoadStringW (hModuleMessages, resid, formatString, cchMax) != 0) &&
           (FormatMessageW (FORMAT_MESSAGE_FROM_STRING, formatString, 0, 0, buffer, cchMax, &args) != 0);
}


/*
 * Helper function to load and format a message using ANSI functions.
 * Used as a backup when Unicode ones are not available.
 */
static bool LoadAndFormatA(int resid, va_list args, LPWSTR buffer, int cchMax)
{
    CAutoFree<CHAR>     memFormat, memResult;
    LPSTR               pszFormat = SAFEALLOC (memFormat, cchMax * 2);
    LPSTR               pszResult = SAFEALLOC (memResult, cchMax * 2);

    return (LoadStringA (hModuleMessages, resid, pszFormat, cchMax * 2) != 0) &&
           (FormatMessageA (FORMAT_MESSAGE_FROM_STRING, pszFormat, 0, 0, pszResult, cchMax * 2, &args) != 0) &&
           (MultiByteToWideChar (CP_ACP, 0, pszResult, -1, buffer, cchMax) != 0);
}

bool __cdecl LoadAndFormatMessage(int resid, LPWSTR buffer, int cchMax, ...)
{
    va_list args;
    bool success;

    va_start(args, cchMax);

    success = (LoadAndFormatW(resid, args, buffer, cchMax) || LoadAndFormatA(resid, args, buffer, cchMax));

    va_end(args);

    return success;
}

////////////////////////////////////////////////////////////////////////////////
// CController::DisplayWarning
//
// This function determines whether a warning should be displayed or suppressed,
// taking into account the warning level and "no warn" list.

BOOL CController::DisplayWarning(long iErrorIndex, int warnLevel)
{
    ASSERT(warnLevel > 0);  // this must be a warning.

    if (warnLevel > m_OptionData.warnLevel)
        return FALSE;   // warning level suppressed the warning.

    int cNoWarn = m_OptionData.noWarnNumbers.Length();
    unsigned short warnNumber = errorInfo[iErrorIndex].number;

    for (int i = 0; i < cNoWarn; ++i) {
        if (warnNumber == m_OptionData.noWarnNumbers[i])
            return FALSE; // warning suppressed by noWarn list
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////
// CController::CreateError
//
// This function creates a CError object from the given information.  The CError
// object is created with no location information.  The CError object is returned
// with a ref count of 0.
//
// If "warnLevelOverride" is non-zero (defaults to zero), and this error is usually
// an error (not warning or fatal), then the error is transformed into a warning
// at the given level.
HRESULT CController::CreateError (long iErrorIndex, va_list args, CError **ppError, int warnLevelOverride)
{
    // Make some assertions...
    ASSERT (iErrorIndex < ERR_COUNT && iErrorIndex > 0);

    ERRORKIND   iKind;
    int warnLevel;

    // Determine the error kind
    warnLevel = errorInfo[iErrorIndex].level;

    if (warnLevel == 0) {
        iKind = ERROR_ERROR;
        if (warnLevelOverride) {
            iKind = ERROR_WARNING;
            warnLevel = warnLevelOverride;
        }
    }
    else if (warnLevel > 0) {
        iKind = ERROR_WARNING;
    }
    else
    {
        ASSERT (warnLevel == -1);
        iKind = ERROR_FATAL;
    }

    // If it's a warning, does it meet the warning level criteria?
    if ((iKind == ERROR_WARNING) && ! DisplayWarning(iErrorIndex, warnLevel))
    {
        // No, so we don't want an error in this case.
        *ppError = NULL;
        return S_FALSE;
    }

    // Finally, if it's a warning and warnings are treated as errors, change it to an error
    if (iKind == ERROR_WARNING && m_OptionData.m_fWARNINGSAREERRORS)
        iKind = ERROR_ERROR;

    HRESULT hr = E_OUTOFMEMORY;

    CComObject<CError>  *pObj;

    // Okay, create the error node and return it
    if (SUCCEEDED (hr = CComObject<CError>::CreateInstance (&pObj)))
    {
        if (FAILED (hr = pObj->Initialize (iKind, errorInfo[iErrorIndex].number, errorInfo[iErrorIndex].resid, args)))
        {
            m_spHost->OnCatastrophicError (FALSE, 0, NULL);
            delete pObj;
        }
        else
        {
            *ppError = pObj;
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CController::SubmitError
//
// This function places a fully-constructed CError object into an error container
// and sends it to the compiler host (this would be the place to batch these guys
// up if we decide to.
//
// Note that if the error can't be put into a container (if, for example, we
// can't create a container) the error is destroyed and the host is notified via
// OnCatastrophicError.

void CController::SubmitError (CError *pError)
{
    // Allow NULL -- this is often called with a function that returns an error as
    // an argument; it may not actually be an error.
    if (pError == NULL)
        return;

    // Remember that we had an error (if this isn't a warning)
    if (pError->Kind() != ERROR_WARNING)
        m_iErrorsReported++;

    // Make sure we have an error container we can use.  Note that we (somewhat hackily)
    // check the ref count on any existing container, and if 1, re-use it.  (If it's
    // 1, it means we have the only ref on it, so nobody will be hurt by re-using it).
    if (m_pCompilerErrors != NULL)
    {
        if (m_pCompilerErrors->RefCount() == 1)
        {
            // This one can be re-used -- just empty it.
            m_pCompilerErrors->ReleaseAllErrors();
        }
        else
        {
            m_pCompilerErrors->Release();
            m_pCompilerErrors = NULL;
        }
    }

    if (m_pCompilerErrors == NULL)
    {
        // Create a new container for the errors
        if (FAILED (CErrorContainer::CreateInstance (EC_COMPILATION, 0, &m_pCompilerErrors)))
        {
            m_spHost->OnCatastrophicError (FALSE, 0, NULL);
            pError->Release();
            return;
        }
    }

    // We must have a container by now!  Add the error and push it to the host.
    ASSERT (m_pCompilerErrors != NULL);
    if (SUCCEEDED (m_pCompilerErrors->AddError (pError)))
        m_spHost->ReportErrors (m_pCompilerErrors);
}

////////////////////////////////////////////////////////////////////////////////
// CController::ReportErrorsToHost
//
// This function reports non-empty error containers to the host. It also keeps
// track of whether non-empty containers have actual errors (i.e. something
// other than warnings).

void CController::ReportErrorsToHost (ICSErrorContainer *pErrors)
{
    if (pErrors == NULL)
        return;

    long    iCount, iWarnings;

    if (SUCCEEDED (pErrors->GetErrorCount (&iWarnings, NULL, NULL, &iCount)))
    {
        if (iCount > 0)
        {
            // This container has errors/warnings.  Give it to the host.
            m_spHost->ReportErrors (pErrors);

            // Count the ones that aren't warnings
            m_iErrorsReported += (iCount - iWarnings);
        }
    }
}

/*
 * ThrowFatalException. After a fatal error occurs, this calls to throw
 * an exception out to the outer-most code to abort the compilation.
 */
void COMPILER::ThrowFatalException()
{
    RaiseException(FATAL_EXCEPTION_CODE, 0, 0, NULL);
}

/*
 * Code has caught an exception. Handle it. If the exception code is
 * FATAL_EXCEPTION_CODE, this is the result of a previously reported
 * fatal error and we do nothing. Otherwise, we report an internal
 * compiler error.
 */
void COMPILER::HandleException(DWORD exceptionCode)
{
    Error (NULL, FTL_InternalError, exceptionCode);
}

/*
 * The following routines create "fill-in" strings to be used as insertion
 * strings in an error message. They allocate memory from a single static
 * buffer used for the purpose, and freed when the error message is
 * reported.
 * The buffer initially reserves 2MB worth of memory, but only commits
 * 1 page at a time.  It commits new pages as needed.  However, currently the
 * LoadAndFormat routines used to actually create the error message are
 * limited to 4K.
 */

/*
 * Create a fill-in string describing an HRESULT.
 */
LPWSTR COMPILER::ErrHR(HRESULT hr, bool useGetErrorInfo)
{
    size_t r = 0;
    LPWSTR errstr;
    bool repeat;

    do {
        if (useGetErrorInfo) {
            // See if there is more detailed error message available via GetErrorInfo.
            CComPtr<IErrorInfo> err;
            CComBSTR str;
            if (FAILED(GetErrorInfo( 0, &err)) || err == NULL)
                r = 0;
            else {
                if (FAILED(err->GetDescription( &str)))
                    r = 0;
                else
                    r = wcslen(str);
                if (r < (size_t)ErrBufferLeft() && r > 0)
                    wcscpy(errBufferNext, str);
            }
        } 
        
        if (r == 0) {
            // Check for some well-known HRESULTS that aren't in the system database.
            if (hr >= CLDB_E_FILE_BADREAD && hr <= CLDB_E_BADUPDATEMODE) {
                if (LoadAndFormatMessage(IDS_CLB_ERROR_FIRST + (hr - CLDB_E_FILE_BADREAD), errBufferNext, ErrBufferLeft()))
                    r = wcslen(errBufferNext);
            }
        }

        if (r == 0) {
            // Use FormatMessage to get the error string for the HRESULT from the system.
            r = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL, hr, 0,
                               errBufferNext, ErrBufferLeft(),
                               NULL);

        }

        // Check for errors, and possibly repeat.
        repeat = false;
        if (r == 0) {
            if (hr != E_FAIL) {
                // Use E_FAIL as generic error code if looking up the real code didn't work, and repeat.
                hr = E_FAIL;
                repeat = true;
            }
            else {
                // Something really extreme. I don't understand why this would happen.
                ASSERT(0);      // investigate please.
                return L"";
            }
        }
    } while (repeat);

    // Advange pointer to the next routine.
    errstr = errBufferNext;
    errBufferNext += (r + 1);

    return errstr;
}


/*
 * Create a fill-in string describing the last Win32 error.
 */
LPWSTR COMPILER::ErrGetLastError()
{
    return ErrHR(HRESULT_FROM_WIN32(GetLastError()), false);
}

/*
 * For now, just return the text of the name...
 */
LPCWSTR COMPILER::ErrName(PNAME name)
{
    if (name == namemgr->GetPredefName(PN_INDEXERINTERNAL)) {
        return L"this";
    } else {
        return name->text;
    }
}


/*
 * For now, just return the text of the access modifier
 */
LPCWSTR COMPILER::ErrAccess(ACCESS acc)
{
    switch (acc) {
    case ACC_PUBLIC:
        return CParser::GetTokenInfo( TID_PUBLIC)->pszText;

    case ACC_PROTECTED:
        return CParser::GetTokenInfo( TID_PROTECTED)->pszText;

    case ACC_INTERNAL:
        return CParser::GetTokenInfo( TID_INTERNAL)->pszText;

    case ACC_INTERNALPROTECTED:
    {
        LPWSTR errstr = errBufferNext;  // error string under construction
        LPWSTR cur = errstr;            // current location.

        wcscpy(cur, CParser::GetTokenInfo( TID_PROTECTED)->pszText);
        wcscat(cur, L" ");
        wcscat(cur, CParser::GetTokenInfo( TID_INTERNAL)->pszText);
        cur = cur + wcslen(cur);
        errBufferNext = cur + 1;
        return errstr;
    }

    case ACC_PRIVATE:
        return CParser::GetTokenInfo( TID_PRIVATE)->pszText;

    default:
        ASSERT( !"Unknown access modifier");
        return L"";
    }
}

/*
 * Create a fill-in string describing a possibly fully qualified name.
 */
LPCWSTR COMPILER::ErrNameNode(BASENODE *name)
{
    if (name->kind == NK_NAME) {
        // non-dotted name, just do the regular name thing
        return ErrName(name->asNAME()->pName);
    }

    // we have a dotted name

    // now, find the first name:
    BASENODE *first = name->asDOT()->p1;
    while (first->kind == NK_DOT) {
        first = first->asDOT()->p1;
    }

    LPWSTR errorString = errBufferNext;

    //
    // add the first name, unless this is a fully qualified name
    //
    if (first->asNAME()->pName != namemgr->GetPredefName(PN_EMPTY)) {
        wcscpy(errBufferNext, first->asNAME()->pName->text);
        errBufferNext += wcslen(errBufferNext);
    }

    //
    // add the remaining names
    //
    do {
        // loop until we add all the names
        first = first->pParent;
        ASSERT(first->kind == NK_DOT && first->asDOT()->p2->kind == NK_NAME);

        wcscpy(errBufferNext, L".");
        errBufferNext += 1;
        wcscpy(errBufferNext, first->asDOT()->p2->asNAME()->pName->text);
        errBufferNext += wcslen(errBufferNext);

        // is this the rightmost name?
    } while (first != name);

    // reserve space for the null terminator
    errBufferNext += 1;

    return errorString;
}

/*
 * Create a fill-in string describing a parsed type node
 * Note: currently only works for predefined types, and named types
 *       does not work for arrays, pointers, etc.
 */
LPCWSTR COMPILER::ErrTypeNode(TYPENODE *type)
{
    switch (type->TypeKind()) {
    case TK_PREDEFINED:
        // NOTE: we may note have predefined types installed when we call this
        return symmgr.GetNiceName((PREDEFTYPE)type->iType);
    case TK_NAMED:
        return ErrNameNode(type->pName);
    default:
        ASSERT(!"NYI. Handle other type node types");
    }

    return NULL;
}

LPWSTR COMPILER::ErrId(int id)
{
    LPWSTR errstr = errBufferNext;  // error string under construction
    LPWSTR cur = errstr;            // current location.

    *cur = 0;
    if (!(LoadAndFormatMessage(id, errBufferNext, ErrBufferLeft())))
    {
        ASSERT(0);
        return L"";
    } else {
        cur = errBufferNext + wcslen(errBufferNext);
    }

    errBufferNext = cur + 1;
    return errstr;
}

LPWSTR COMPILER::ErrSK(SYMKIND sk)
{
    LPWSTR errstr = errBufferNext;  // error string under construction
    LPWSTR cur = errstr;            // current location.

    int id;
    switch (sk) {
    case SK_METHSYM: id = IDS_SK_METHOD; break;
    case SK_AGGSYM: id = IDS_SK_CLASS; break;
    case SK_NSSYM: id = IDS_SK_NAMESPACE; break;
    case SK_MEMBVARSYM: id = IDS_SK_FIELD; break;
    case SK_LOCVARSYM: id = IDS_SK_VAIRABLE; break;
    case SK_PROPSYM: id = IDS_SK_PROPERTY; break;
    case SK_EVENTSYM: id = IDS_SK_EVENT; break;
    default : ASSERT(!"impossible sk"); id = IDS_SK_UNKNOWN;
    }

    *cur = 0;
    if (!(LoadAndFormatMessage(id, errBufferNext, ErrBufferLeft())))
    {
        ASSERT(0);
        return L"";
    } else {
        cur = errBufferNext + wcslen(errBufferNext);
    }

    errBufferNext = cur + 1;
    return errstr;

}

/*
 * Create a fill-in string describing a parameter list.
 * Does NOT include ()
 */
LPWSTR COMPILER::ErrParamList(int cParams, TYPESYM **params, bool isVarargs, bool isParamArray)
{
    LPWSTR cur = errBufferNext;

    TYPESYM **argType = params;
    while (argType < (params + cParams)) {
        if (argType != params) {
            *cur++ = L',';
            *cur++ = L' ';
        }
        
        if (isParamArray && argType == (params + cParams - 1)) {
            wcscpy(cur, L"params ");
            cur += 7;
        }

        // parameter type name
        errBufferNext = cur;
        ErrSym(*argType);
        cur += wcslen(cur);

        argType += 1;
    }

    if (isVarargs) {
        if (cParams) {
            *cur++ = L',';
            *cur++ = L' ';
        }

        *cur++ = L'.';
        *cur++ = L'.';
        *cur++ = L'.';
        *cur++ = 0;
    }

    return errBufferNext;
}

/*
 * Create a fill-in string describing a delegate.
 * Different from ErrSym because this one also adds the return type and the arguments
 */
LPWSTR COMPILER::ErrDelegate(PAGGSYM sym)
{
    ASSERT(sym->isDelegate);
    LPWSTR errstr = errBufferNext;  // error string under construction
    LPWSTR cur = errstr;            // current location.
    PMETHSYM pInvoke;

    {
        SYM * temp = symmgr.LookupGlobalSym(namemgr->GetPredefName(PN_INVOKE), sym, MASK_METHSYM);
        if (!temp || temp->kind != SK_METHSYM) {
            // We can't find the Invoke method, so fall-back to reporting the plain delegate name
            return ErrSym(sym);
        } else {
            pInvoke = temp->asMETHSYM();
        }
    }

    *cur = 0;
    // return type
    ErrSym(pInvoke->retType);
    cur = errstr + wcslen(errstr);
    *cur++ = L' ';

    // Delegate Name
    errBufferNext = cur;
    ErrSym(sym);
    cur = errstr + wcslen(errstr);

    // Parameter list
    *cur++ = L'(';
	if (!funcBRec.checkBogus(pInvoke)) {
		*cur = 0;
        errBufferNext = cur;
		ErrParamList(pInvoke->cParams, pInvoke->params, pInvoke->isVarargs, pInvoke->isParamArray);
		cur += wcslen(cur);
	}
    *cur++ = L')';
    *cur = 0;

    errBufferNext = cur + 1;
    return errstr;
}

/*
 * Create a fill-in string describing a symbol.
 */
LPWSTR COMPILER::ErrSym(PSYM sym)
{
    LPWSTR errstr = errBufferNext;  // error string under construction
    LPWSTR cur = errstr;            // current location.
    LPCWSTR text;
    PPARENTSYM parent;

    *cur = 0;

    switch (sym->kind) {
    case SK_NSDECLSYM:
        // for namespace declarations just convert the namespace
        return ErrSym(sym->asNSDECLSYM()->namespaceSymbol);
        break;

    case SK_ALIASSYM:
        cur = ErrSym(sym->asALIASSYM()->parent);
        if (*cur) {
            cur += wcslen(cur);
            *cur++ = L'.';
        }
        goto APPENDNAME;

    case SK_GLOBALATTRSYM:
        return (LPWSTR) ErrName(sym->name);

    case SK_EXPANDEDPARAMSSYM:
        return ErrSym(sym->asEXPANDEDPARAMSSYM()->realMethod);
        
    case SK_AGGSYM:
        // Check for a predefined class with a special "nice" name for
        // error reported.
        text = symmgr.GetNiceName(sym->asAGGSYM());
        if (text) {
            // Found a nice name.
            wcscpy(cur, text);
            cur += wcslen(cur);
            break;
        }

        // FALL THROUGH -- generate the current symbol name.

    case SK_NSSYM:
    case SK_MEMBVARSYM:
    case SK_METHSYM:
    case SK_EVENTSYM:
    case SK_PROPSYM:
        // Qualify with parent symbol, if any.
        parent = sym->parent;
        if (parent && parent->name->text[0] != '\0') {
            ErrSym(parent);
            cur = errstr + wcslen(errstr);
            *cur++ = L'.';
        }

        if (sym->name == NULL) {
            // Probably an explicit interface implementation
            PSYM explicitImpl = NULL;
            if (sym->kind == SK_METHSYM) {
                explicitImpl = sym->asMETHSYM()->explicitImpl;
            }
            else if (sym->kind == SK_PROPSYM) {
                explicitImpl = sym->asPROPSYM()->explicitImpl;
            }
            else if (sym->kind == SK_EVENTSYM) {
                explicitImpl = sym->asEVENTSYM()->getExplicitImpl();
            }

            if (explicitImpl) {
                errBufferNext = cur;
                ErrSym(explicitImpl);
                cur += wcslen(cur);
                break;
            }
        }

        if (sym->kind == SK_METHSYM) {
            METHSYM *method = sym->asMETHSYM();

            //
            // property accessors are special
            //
            if (method->isPropertyAccessor) {
                PROPSYM *property = method->getProperty();
                //
                // this overwrites the existing string ( which contained the parent class )
                //
                errBufferNext = errstr;
                cur = ErrSym(property);
                cur += wcslen(cur);

                //
                // add .get or .set
                //
                if (property->methGet == sym) {
                    wcscpy(cur, L".get");
                } else {
                    ASSERT(sym == property->methSet);
                    wcscpy(cur, L".set");
                }
                cur += 4;
                break;
            }

            //
            // event accessors are special
            //
            if (method->isEventAccessor) {
                EVENTSYM *event = method->getEvent();
                //
                // this overwrites the existing string ( which contained the parent class )
                //
                errBufferNext = errstr;
                cur = ErrSym(event);
                cur += wcslen(cur);

                //
                // add .get or .set
                //
                if (event->methAdd == sym) {
                    wcscpy(cur, L".add");
                    cur += 4;
                } else {
                    ASSERT(sym == event->methRemove);
                    wcscpy(cur, L".remove");
                    cur += 7;
                }
                break;
            }

            //
            // generate pretty method name
            //
            if (method->isCtor) {
                // Use the name of the parent class instead of the name "<ctor>".
                wcscpy(cur, sym->parent->name->text);
                cur += wcslen(cur);
            }
            else if (method->isDtor) {
                // Use the name of the parent class instead of the name "Finalize".
                *cur++ = L'~';
                wcscpy(cur, sym->parent->name->text);
                cur += wcslen(cur);
            }
            else if (method->isConversionOperator()) {
                // implicit/explicit
                wcscpy(cur, (method->isImplicit ? L"implicit" : L"explicit"));
                cur += wcslen(cur);

                wcscpy(cur, L" operator ");
                cur += wcslen(cur);

                // destination type name
                errBufferNext = cur;
                ErrSym(method->retType);
                cur += wcslen(cur);
            }
            else if (method->isOperator) {
                // handle user defined operators
                // map from CLS predefined names to "operator <X>"
                wcscpy(cur, L"operator ");
                cur += wcslen(cur);

                LPCWSTR operatorName = CParser::GetTokenInfo((TOKENID) CParser::GetOperatorInfo(clsDeclRec.operatorOfName(sym->name))->iToken)->pszText;
                if (operatorName[0] == 0) {
                    //
                    // either equals or compare
                    //
                    if (sym->name == namemgr->GetPredefName(PN_OPEQUALS)) {
                        operatorName = L"equals";
                    } else {
                        ASSERT(sym->name == namemgr->GetPredefName(PN_OPCOMPARE));
                        operatorName = L"compare";
                    }
                }
                wcscpy(cur, operatorName);
                cur += wcslen(cur);
            }
            else if (!sym->name) {
                // explicit impl that hasn't been prepared yet
                // can't be a property accessor
                cur = (LPWSTR) ErrNameNode(sym->asMETHSYM()->parseTree->asANYMETHOD()->pName);
                cur += wcslen(cur);
            }
            else{
                // regular method
                wcscpy(cur, sym->name->text);
                cur += wcslen(cur);
            }

            //
            // append argument types
            //
            *cur++ = L'(';

			if (!funcBRec.checkBogusNoError(method)) {
				*cur = 0;
				errBufferNext = cur;
				ErrParamList(method->cParams, method->params, method->isVarargs, method->isParamArray);
				cur += wcslen(cur);
			}

            *cur++ = L')';
            *cur = 0;

            break;
        }
        else if ((sym->kind == SK_PROPSYM) && sym->asPROPSYM()->isIndexer()) {
DO_INDEXER:
            // handle indexers
            wcscpy(cur, L"this[");
            cur += wcslen(cur);

            // add parameter types
            *cur = 0;
            errBufferNext = cur;
            ErrParamList(sym->asPROPSYM()->cParams, sym->asPROPSYM()->params, sym->asPROPSYM()->isVarargs, sym->asPROPSYM()->isParamArray);
            cur += wcslen(cur);

            *cur++ = L']';
            *cur = 0;

            break;
        }
        else if (!sym->name) {
            // must be explicit impl not prepared yet.
            if (sym->kind == SK_PROPSYM) {
            cur = (LPWSTR) ErrNameNode(sym->asPROPSYM()->parseTree->asPROPERTY()->pName);
                if (sym->asPROPSYM()->isIndexer()) {
                    cur += wcslen(cur);
                    *cur++ = '.';
                    goto DO_INDEXER;
                } else {
                    break;
                }
            }
            else {
                ASSERT(sym->kind == SK_EVENTSYM);
                cur = (LPWSTR) ErrNameNode(sym->asEVENTSYM()->parseTree->asPROPERTY()->pName);
                break;
            }
        }
        // FALL THROUGH -- generate the current symbol name.

    case SK_INFILESYM:
    case SK_OUTFILESYM:
    case SK_LOCVARSYM:
APPENDNAME:
        // Generate symbol name.
        text = sym->name->text;

        if (cur + wcslen(text) + 1 >= errBuffer + ERROR_BUFFER_MAX_WCHARS) {
            ASSERT( cur + 19 <= errBuffer + ERROR_BUFFER_MAX_WCHARS);
            wcsncpy(cur, text, 12);
            cur += 12;
            wcscpy( cur, L"...");
            cur += 3;
            wcscpy( cur, text + (wcslen(text) - 3));
            cur += 3;
        } else {
            wcscpy(cur, text);
            cur += wcslen(cur);
        }
        break;

    case SK_ERRORSYM:
        // Load the string "<error>".
        if (!(LoadAndFormatMessage(IDS_ERRORSYM, errBufferNext, ErrBufferLeft())))
        {
            ASSERT(0);
            return L"";
        }
        else
            cur = errBufferNext + wcslen(errBufferNext);
        break;

    case SK_NULLSYM:
        // Load the string "<null>".
        if (!(LoadAndFormatMessage(IDS_NULL, errBufferNext, ErrBufferLeft())))
        {
            ASSERT(0);
            return L"";
        }
        else
            cur = errBufferNext + wcslen(errBufferNext);
        break;

    case SK_ARRAYSYM: {

        PTYPESYM elementType;
        int rank;

        // Generate the element type..
        elementType = sym->asARRAYSYM()->elementType();
        rank = sym->asARRAYSYM()->rank;
        ErrSym(elementType);

        // Advance beyond the element type just generated.
        cur = errstr + wcslen(errstr);
        if (cur + (rank*2) + 2 >= errBuffer + ERROR_BUFFER_MAX_WCHARS) {
            ASSERT(0);
            return L"";         // Not enough room.
        }

        // Add [] with (rank-1) commas inside
        *cur++ = L'[';

        // known rank.
        if (rank > 1)
            *cur++ = L'*';
        for (int i = rank; i > 1; --i) {
            *cur++ = L',';
            *cur++ = L'*';
        }

        *cur++ = L']';
        *cur = L'\0';
        break;
    }

    case SK_VOIDSYM:
        text = namemgr->KeywordName(TID_VOID)->text;

        if (cur + wcslen(text) + 1 >= errBuffer + ERROR_BUFFER_MAX_WCHARS) {
            ASSERT(0);
            return L"";         // Not enough room.
        }

        wcscpy(cur, text);
        cur += wcslen(cur);

        break;

    case SK_PARAMMODSYM:
        //
        // add ref or out
        //
        if (sym->asPARAMMODSYM()->isRef) {
            wcscpy(cur, L"ref ");
        } else {
            ASSERT(sym->asPARAMMODSYM()->isOut);
            wcscpy(cur, L"out ");
        }
        cur += 4;
        errBufferNext = cur;

        //
        // add base type name
        //
        cur = ErrSym(sym->asPARAMMODSYM()->paramType());
        cur += wcslen(cur);

        break;

    case SK_PTRSYM:

        // Generate the base type..
        ErrSym(sym->asPTRSYM()->baseType());
        cur += wcslen(cur);

        // add the trailing *
        *cur++ = L'*';
        *cur = 0;

        break;

    case SK_SCOPESYM:
    default:
        // Shouldn't happen.
        return L"";
    }

    errBufferNext = cur + 1;
    return errstr;
}


////////////////////////////////////////////////////////////////////////////////
// COMPILER::MakeError
//
// This function creates and returns a CError object for the given error index.
// If the error index is a warning beyond the warning level, this will return
// NULL (as it will if creation fails, but the host will be notified in that
// case).  The CError object's ref count will be zero, and the error will NOT
// have been submitted to the host.

CError * __cdecl COMPILER::MakeError (int id, ...)
{
    CError  *pError;
    va_list args;

    va_start (args, id);
    pError = MakeError(id, args);
    va_end (args);
    return pError;
}

CError * COMPILER::MakeError (int id, va_list args)
{
    CError  *pError;
    int clsWarnOverride = 0;

    if (FAILED (pController->CreateError (id, args, &pError, clsWarnOverride)))
        pError = NULL;
    return pError;
}

CError * COMPILER::MakeErrorWithWarnOverride (int id, va_list args, int warnLevelOverride)
{
    CError  *pError;

    if (FAILED (pController->CreateError (id, args, &pError, warnLevelOverride)))
        pError = NULL;
    return pError;
}

////////////////////////////////////////////////////////////////////////////////
// COMPILER::AddLocationToError
//
// This function adds the given ERRLOC data as a location to the given error.
// If there is any kind of failure, the host is told that things are toast via
// OnCatastrophicError().

void COMPILER::AddLocationToError (CError *pError, const ERRLOC& pErrLoc)
{
    AddLocationToError(pError, &pErrLoc);
}

void COMPILER::AddLocationToError (CError *pError, const ERRLOC *pErrLoc)
{
    // No file name means no location.  Also, don't freak if no error is given.
    if (pError == NULL || pErrLoc == NULL || pErrLoc->fileName() == NULL)
        return;

    POSDATA     posStart, posEnd, mapStart, mapEnd;

    // See if there's a line/column location
    if (pErrLoc->hasLocation())
    {
        posStart.iLine = pErrLoc->line();
        posStart.iChar = pErrLoc->column();
        posEnd.iLine = pErrLoc->line();
        posEnd.iChar = posStart.iChar + pErrLoc->extent();
        mapStart.iLine = pErrLoc->mapLine();
        mapStart.iChar = pErrLoc->column();
        mapEnd.iLine = pErrLoc->mapLine();
        mapEnd.iChar = mapStart.iChar + pErrLoc->extent();
    }
    else
    {
        posStart.SetEmpty();
        posEnd.SetEmpty();
        mapStart.SetEmpty();
        mapEnd.SetEmpty();
    }

    if (FAILED (pError->AddLocation (pErrLoc->fileName(), &posStart, &posEnd, pErrLoc->mapFile(), &mapStart, &mapEnd)))
        host->OnCatastrophicError (FALSE, 0, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// COMPILER::SubmitError
//
// This function submits the given error to the controller, and if it's a fatal
// error, throws the fatal exception.

void COMPILER::SubmitError (CError *pError)
{
    ResetErrorBuffer ();

    if (pError != NULL)
    {
        pController->SubmitError (pError);

#if DEBUG
        if (GetRegDWORD("Error"))
        {
            if (MessageBoxW(0, pError->GetText(), L"ASSERT?", MB_YESNO) == IDYES)
            {
                BASSERT(FALSE);
            }
        }
#endif

        if (pError->Kind() == ERROR_FATAL)
            ThrowFatalException();
    } else {
        //
        // need to free the error message buffer even if no error was
        // posted because we prepared the arguments anyways.
        //
        ResetErrorBuffer ();
    }
}

////////////////////////////////////////////////////////////////////////////////
// COMPILER::Error
//
// A shortcut for making, adding a location to, and submitting an error

void __cdecl COMPILER::Error(const ERRLOC& location, int id, ...)
{
    va_list args;

    va_start(args, id);
    ErrorWithList(&location, id, args);
    va_end(args);
}

void __cdecl COMPILER::Error(const ERRLOC *location, int id, ...)
{
    va_list args;

    va_start(args, id);
    ErrorWithList(location, id, args);
    va_end(args);
}

void COMPILER::ErrorWithList(const ERRLOC *location, int id, va_list args)
{
    CError  *pError;

    if (SUCCEEDED (pController->CreateError (id, args, &pError)))
    {
        AddLocationToError (pError, location);
        SubmitError (pError);
    }
}

/*
 * Recursively searches parse-tree for correct left-most node
 */
void ERRLOC::SetStart(BASENODE* node)
{
    TOKENDATA data;
    ASSERT(!start.IsEmpty());
    m_sourceData->GetSingleTokenData(node->tokidx, &data);
    if (data.posTokenStart.iLine == start.iLine) {
        start = data.posTokenStart;
    } else
        return;

    switch (node->kind) {
    case NK_ACCESSOR:
        // Get and Set always are the token before the '{'
        if (node->asACCESSOR()->iOpen - 1 >= 0) {
            m_sourceData->GetSingleTokenData(node->asACCESSOR()->iOpen - 1, &data);
            start = data.posTokenStart;
        }
        return;
    case NK_ARROW:              // BINOPNODE
        if (node->asARROW()->p1)
            SetStart(node->asARROW()->p1);
        return;
    case NK_ATTR:               // ATTRNODE
        if (node->asATTR()->pName)
            SetStart(node->asATTR()->pName);
        return;
    case NK_BINOP:              // BINOPNODE
        if (node->asBINOP()->p1)
            SetStart(node->asBINOP()->p1);
        return;
    case NK_CLASS:              // CLASSNODE
    case NK_INTERFACE:
    case NK_STRUCT:
        if (node->asAGGREGATE()->pName)
            SetStart(node->asAGGREGATE()->pName);
        return;
    case NK_CTOR:
    case NK_DTOR:
        {
            long idx = node->tokidx;
            // FInd the indentifier which must be the ctor/dtor name
            do {
                m_sourceData->GetSingleTokenData(idx++, &data);
            } while( data.iToken != TID_IDENTIFIER );
            start = data.posTokenStart;
            return;
        }
    case NK_DELEGATE:
        if (node->asDELEGATE()->pName)
            SetStart(node->asDELEGATE()->pName);
        return;
    case NK_DOT:                // BINOPNODE
        if (node->asDOT()->p1)
            SetStart(node->asDOT()->p1);
        return;
    case NK_ENUM:
        if (node->asENUM()->pName)
            SetStart(node->asENUM()->pName);
        return;
    case NK_INDEXER:
        if (node->asANYPROPERTY()->pName)
            SetStart(node->asANYPROPERTY()->pName);
        else {
            long idx = node->tokidx;
            // find the 'this'
            do {
                m_sourceData->GetSingleTokenData(idx++, &data);
            } while( data.iToken != TID_THIS );
            start = data.posTokenStart;
        }
        return;
    case NK_METHOD:             // METHODNODE
        // use the name to keep the the same as fields
        if (node->asANYMETHOD()->pName)
            SetStart(node->asANYMETHOD()->pName);
        return;
    case NK_NAMESPACE:
        // use the name to keep the the same as fields
        if (node->asNAMESPACE()->pName)
            SetStart(node->asNAMESPACE()->pName);
        return;
    case NK_OPERATOR:
        {
            long idx = node->tokidx;
            // find the begining of the operator 'name'
            do {
                m_sourceData->GetSingleTokenData(idx++, &data);
            } while( data.iToken != TID_OPERATOR  &&
                data.iToken != TID_EXPLICIT &&
                data.iToken != TID_IMPLICIT);
            start = data.posTokenStart;
            return;
        }
    case NK_PROPERTY:
        if (node->asANYPROPERTY()->pName)
            SetStart(node->asANYPROPERTY()->pName);
        return;
    case NK_UNOP:
        if ((node->Op() == OP_POSTINC || node->Op() == OP_POSTDEC) && node->asUNOP()->p1)
            SetStart(node->asUNOP()->p1);
        else
            break;
        return;
    }
}

/*
 * Sets start to correct line for given node
 */
void ERRLOC::SetLine(BASENODE* node)
{
    TOKENDATA data;

    if (!node) return;

    // default if no special processing below.
    m_sourceData->GetSingleTokenData(node->tokidx, &data);
    start = data.posTokenStart;

    switch (node->kind) {
    case NK_ACCESSOR:
        // Get and Set always are the token before the '{'
        if (node->asACCESSOR()->iOpen - 1 >= 0) {
            m_sourceData->GetSingleTokenData(node->asACCESSOR()->iOpen - 1, &data);
            start = data.posTokenStart;
        }
        return;
    case NK_CLASS:              // CLASSNODE
    case NK_INTERFACE:
    case NK_STRUCT:
        if (node->asAGGREGATE()->pName)
            SetLine(node->asAGGREGATE()->pName);
        return;
    case NK_CTOR:
    case NK_DTOR:
        {
            long idx = node->tokidx;
            // Find the indentifier which must be the ctor/dtor name
            do {
                m_sourceData->GetSingleTokenData(idx++, &data);
            } while( data.iToken != TID_IDENTIFIER );
            start = data.posTokenStart;
            return;
        }
    case NK_DELEGATE:
        if (node->asDELEGATE()->pName)
            SetLine(node->asDELEGATE()->pName);
        return;
    case NK_ENUM:
        if (node->asENUM()->pName)
            SetLine(node->asENUM()->pName);
        return;
    case NK_INDEXER:
        if (node->asANYPROPERTY()->pName)
            SetLine(node->asANYPROPERTY()->pName);
        else {
            long idx = node->tokidx;
            // find the 'this'
            do {
                m_sourceData->GetSingleTokenData(idx++, &data);
            } while( data.iToken != TID_THIS );
            start = data.posTokenStart;
        }
        return;
    case NK_METHOD:             // METHODNODE
        // use the name to keep the the same as fields
        if (node->asANYMETHOD()->pName)
            SetLine(node->asANYMETHOD()->pName);
        return;
    case NK_NAMESPACE:
        // use the name to keep the the same as fields
        if (node->asNAMESPACE()->pName)
            SetLine(node->asNAMESPACE()->pName);
        else if (node->asNAMESPACE()->pUsing)
            SetLine(node->asNAMESPACE()->pUsing);
        return;
    case NK_OPERATOR:
        {
            long idx = node->tokidx;
            // find the 'operator'
            do {
                m_sourceData->GetSingleTokenData(idx++, &data);
            } while( data.iToken != TID_OPERATOR );
            start = data.posTokenStart;
            return;
        }
    case NK_PROPERTY:
        if (node->asANYPROPERTY()->pName)
            SetLine(node->asANYPROPERTY()->pName);
        return;
    }
}

/*
 * Recursively searches parse-tree for correct right-most node
 */
void ERRLOC::SetEnd(BASENODE* node)
{
    TOKENDATA data;
    ASSERT(!start.IsEmpty());
    m_sourceData->GetSingleTokenData(node->tokidx, &data);
    if (data.posTokenStart.iLine == start.iLine) {
        end = data.posTokenStop;
    } else
        return;

    switch (node->kind) {
    case NK_ACCESSOR:
        // Get and Set always are the token before the '{'
        if (node->asACCESSOR()->iOpen - 1 >= 0) {
            m_sourceData->GetSingleTokenData(node->asACCESSOR()->iOpen - 1, &data);
            end = data.posTokenStop;
        }
        return;
    case NK_ARROW:              // BINOPNODE
        if (node->asARROW()->p2)
            SetEnd(node->asARROW()->p2);
        return;
    case NK_ATTR:               // ATTRNODE
        if (node->asATTR()->pName)
            SetEnd(node->asATTR()->pName);
        return;
    case NK_BINOP:              // BINOPNODE
        if (node->Op() == OP_CALL) {
            m_sourceData->GetSingleTokenData(((CALLNODE*)node)->iClose, &data);
            end = data.posTokenStop;
        } else if (node->asBINOP()->p2)
            SetEnd(node->asBINOP()->p2);
        return;
    case NK_CLASS:              //CLASSNODE
    case NK_INTERFACE:
    case NK_STRUCT:
        if (node->asAGGREGATE()->pName)
            SetEnd(node->asAGGREGATE()->pName);
        return;
    case NK_CTOR:
    case NK_DTOR:
        {
            long idx = node->tokidx;
            // find the next ident which is the ctor/dtor name
            do {
                m_sourceData->GetSingleTokenData(idx++, &data);
            } while( data.iToken != TID_IDENTIFIER );
            end = data.posTokenStop;
            return;
        }
    case NK_DELEGATE:
        if (node->asDELEGATE()->pName)
            SetEnd(node->asDELEGATE()->pName);
        return;
    case NK_DOT:                // BINOPNODE
        if (node->asDOT()->p2)
            SetEnd(node->asDOT()->p2);
        return;
    case NK_ENUM:
        if (node->asENUM()->pName)
            SetEnd(node->asENUM()->pName);
        return;
    case NK_INDEXER:
        {
            long idx = node->tokidx;
            // find the 'this'
            do {
                m_sourceData->GetSingleTokenData(idx++, &data);
                if (data.posTokenStart.iLine == start.iLine)
                    end = data.posTokenStop;
            } while( data.iToken != TID_THIS && data.posTokenStart.iLine == start.iLine);
        }
        return;
    case NK_METHOD:             // METHODNODE
        // use the name to keep the the same as fields
        if (node->asANYMETHOD()->pName)
            SetEnd(node->asANYMETHOD()->pName);
        return;
    case NK_NAMESPACE:
        // use the name to keep the the same as fields
        if (node->asNAMESPACE()->pName)
            SetEnd(node->asNAMESPACE()->pName);
        return;
    case NK_OPERATOR:
        {
            long idx = node->tokidx;
            // Find the token after 'operator' which is the end of the 'name'
            do {
                m_sourceData->GetSingleTokenData(idx++, &data);
            } while( data.iToken != TID_OPERATOR);
            m_sourceData->GetSingleTokenData(idx, &data);
            end = data.posTokenStop;
            return;
        }
    case NK_PROPERTY:
        if (node->asANYPROPERTY()->pName)
            SetEnd(node->asANYPROPERTY()->pName);
        return;
    }
}

/*
 * This controls how we handle all fatal errors, asserts, and exceptions
 */
LONG CompilerExceptionFilter(EXCEPTION_POINTERS* exceptionInfo, LPVOID pvData)
{
    COMPILER *compiler = ((CompilerExceptionFilterData *)pvData)->pCompiler;
    DWORD exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
    ((CompilerExceptionFilterData *)pvData)->ExceptionCode = exceptionCode;

    // Don't stop here for fatal errors
    if (exceptionCode == FATAL_EXCEPTION_CODE)
        return EXCEPTION_CONTINUE_SEARCH;

    // If it's an AV in our error buffer range, it might be because we need to grow our error buffer.
    // If so, then just commit another page an allow execution to continue
    if (exceptionCode == EXCEPTION_ACCESS_VIOLATION && compiler && compiler->errBuffer &&
        (ULONG_PTR)compiler->errBuffer < exceptionInfo->ExceptionRecord->ExceptionInformation[1] && 
        (ULONG_PTR)(compiler->errBuffer + ERROR_BUFFER_MAX_WCHARS) >= exceptionInfo->ExceptionRecord->ExceptionInformation[1]) {

        void * temp = NULL;
        if (((compiler->errBufferNextPage - (BYTE*)compiler->errBuffer) < (int) ERROR_BUFFER_MAX_BYTES) &&
            (NULL != (temp = VirtualAlloc( compiler->errBufferNextPage, compiler->pageheap.pageSize, MEM_COMMIT, PAGE_READWRITE)))) {
            compiler->errBufferNextPage += compiler->pageheap.pageSize;
            return EXCEPTION_CONTINUE_EXECUTION;
        } else {
            // We either ran out of reserved memory, or couldn't commit what we've already reserved!?!?!?
            // Normally we shouldn't throw another exception inside the exception filter
            // but this really is a fatal condition
            compiler->Error(NULL, FTL_NoMemory);
        }
    }

    if (compiler && compiler->pController)
        compiler->pController->SetExceptionData (exceptionInfo);

#ifdef _DEBUG

    if (COMPILER::GetRegDWORD("GPF")) 
        return EXCEPTION_CONTINUE_SEARCH;


    if (exceptionCode != EXCEPTION_BREAKPOINT &&
        !(exceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)) {
        static int count = 0;
        static DWORD code = 0;
        static void * addr = NULL;
        if (exceptionCode == code &&
            exceptionInfo->ExceptionRecord->ExceptionAddress == addr) {
            if (count++ > 3) {
                // Prevent looping (this stops it on the 4th exception)
                goto DO_HANDLE;
            }
        } else {
            count = 0;
            code = exceptionCode;
            addr = exceptionInfo->ExceptionRecord->ExceptionAddress;
        }
        CHAR buffer[1024];
        sprintf(buffer, "Exception 0x%8.8x occured at address %p\r\n\r\n"
            "Press (Ignore) to let the compiler handle the exception, or "
            "press (Debugger) to launch the debugger and continue execution.\r\n"
            "Make sure the debugger is set to catch exceptions of this type.",
            exceptionCode, exceptionInfo->ExceptionRecord->ExceptionAddress);
        _ASSERTE(!buffer);
        { // This keeps us from throwing the same exception without a debugger
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

DO_HANDLE:
#endif // _DEBUG

    if (compiler) {
        PAL_TRY
        {
            compiler->ReportICE(exceptionInfo);
        }
        PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        PAL_ENDTRY
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

/*
 *  Set a breakpoint here to control 2nd chance exceptions
 */
#if _DEBUG
LONG DebugExceptionFilter(EXCEPTION_POINTERS * exceptionInfo, LPVOID pv)
{
    if (pv)
    {
        *(DWORD *)pv = exceptionInfo->ExceptionRecord->ExceptionCode;
    }
    DWORD ret;
    ret = (DWORD) EXCEPTION_CONTINUE_EXECUTION;
    ret = EXCEPTION_CONTINUE_SEARCH;
    ret = EXCEPTION_EXECUTE_HANDLER;
    if (exceptionInfo->ExceptionRecord->ExceptionCode != FATAL_EXCEPTION_CODE && COMPILER::GetRegDWORD("GPF")) {
        ret = EXCEPTION_CONTINUE_SEARCH;
    }
    return ret;
    if (exceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_CSHARP_ASSERT) {
        return EXCEPTION_CONTINUE_EXECUTION;
    }
}
#endif


// IsValidWarningNumber -- determine if a warning number is valid.

bool IsValidWarningNumber(int id)
{
    // Just search the whole error table for the id.

    for (unsigned int i = 0; i < lengthof(errorInfo); ++i) {
        if (errorInfo[i].number == id && errorInfo[i].level > 0)
            return true;
    }

    return false;
}





////////////////////////////////////////////////////////////////////////////////
// CError::CError

CError::CError () :
    m_iLocations(0),
    m_pLocations(NULL),
    m_pMappedLocations(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////
// CError::~CError

CError::~CError ()
{
    for (short i = 0; i<m_iLocations; i++)
    {
        if (m_pLocations[i].bstrFile != NULL)
            SysFreeString (m_pLocations[i].bstrFile);
        if (m_pMappedLocations[i].bstrFile != NULL)
            SysFreeString (m_pMappedLocations[i].bstrFile);
    }

    VSFree (m_pLocations);
    VSFree (m_pMappedLocations);
}

////////////////////////////////////////////////////////////////////////////////
// CError::Initialize

HRESULT CError::Initialize (ERRORKIND iKind, short iErrorID, long iResourceID, va_list args)
{
    // Save the kind and error ID
    m_iKind = iKind;
    m_iID = iErrorID;

    CAutoFree<WCHAR>    mem;
    PWSTR               pszBuffer = SAFEALLOC (mem, bufferSize);

    // Load the message and fill in arguments. Try using Unicode function first,
    // then back off to ANSI.
    if (!(LoadAndFormatW (iResourceID, args, pszBuffer, bufferSize) ||
          LoadAndFormatA (iResourceID, args, pszBuffer, bufferSize)))
    {
        // Try twice more with bigger buffers (same size as the max compiler buffer)
        if ((GetLastError() == ERROR_MORE_DATA &&
            NULL != (pszBuffer = SAFEALLOC (mem, bufferSize * 8)) &&
            (LoadAndFormatW (iResourceID, args, pszBuffer, bufferSize * 8) ||
             LoadAndFormatA (iResourceID, args, pszBuffer, bufferSize * 8))) ||
            (GetLastError() == ERROR_MORE_DATA &&
            NULL != (pszBuffer = SAFEALLOC (mem, ERROR_BUFFER_MAX_WCHARS)) &&
            (LoadAndFormatW (iResourceID, args, pszBuffer, ERROR_BUFFER_MAX_WCHARS) ||
             LoadAndFormatA (iResourceID, args, pszBuffer, ERROR_BUFFER_MAX_WCHARS)))) {
        } else {
            // Not a lot we can do if we can't report an error. Assert in debug so we know
            // what is going on.
            VSFAIL("FormatMessage failed to load error message");
            return E_FAIL;
        }
    }

    // This will probably fail if we're in low-memory conditions.
    m_sbstrText = pszBuffer;
    return m_sbstrText == NULL ? E_OUTOFMEMORY : S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CError::AddLocation

HRESULT CError::AddLocation (PCWSTR pszFileName, const POSDATA *pposStart, const POSDATA *pposEnd, PCWSTR pszMapFile, const POSDATA *pMapStart, const POSDATA *pMapEnd)
{
    // All locations must have a file name
    if (pszFileName == NULL)
        return E_POINTER;

    // Check for duplicates before adding the location
    for (short i = 0; i < m_iLocations; i++) {
        const LOCATION * p = m_pLocations + i;
        const LOCATION * mp = m_pMappedLocations + i;
        // Match up the locations and then the filenames (hope for an early out)
        if (((!pposStart && p->posStart.IsEmpty() && p->posEnd.IsEmpty()) ||
             (p->posStart == *pposStart && p->posEnd == *pposEnd)) &&
            ((!pMapStart && mp->posStart.IsEmpty() && mp->posEnd.IsEmpty()) ||
             (mp->posStart == *pMapStart && mp->posEnd == *pMapEnd )) &&
            wcscmp(p->bstrFile, pszFileName) == 0 &&
            wcscmp(mp->bstrFile, pszMapFile) == 0) 
            return S_OK; // We've already got this location
    }

    void    *pNew = (m_pLocations == NULL) ? VSAllocZero (sizeof (LOCATION)) : VSReallocZero (m_pLocations, (m_iLocations + 1) * sizeof (LOCATION), m_iLocations*sizeof(LOCATION));
    void    *pNewMap = (m_pMappedLocations == NULL) ? VSAllocZero (sizeof (LOCATION)) : VSReallocZero (m_pMappedLocations, (m_iLocations + 1) * sizeof (LOCATION), m_iLocations*sizeof(LOCATION));

    if (pNew == NULL || pNewMap == NULL)
        return E_OUTOFMEMORY;

    m_pLocations = (LOCATION *)pNew;
    m_pMappedLocations = (LOCATION *)pNewMap;
    m_pLocations[m_iLocations].bstrFile = SysAllocString (pszFileName);
    m_pMappedLocations[m_iLocations].bstrFile = SysAllocString (pszMapFile);
    if (m_pLocations[m_iLocations].bstrFile == NULL || m_pMappedLocations[m_iLocations].bstrFile == NULL)
        return E_OUTOFMEMORY;

    if (pposStart != NULL)
    {
        // Passing one means passing both!
        ASSERT (pposEnd != NULL);
        m_pLocations[m_iLocations].posStart = *pposStart;
        m_pLocations[m_iLocations].posEnd = *pposEnd;
    }
    else
    {
        ASSERT (pposEnd == NULL);
        m_pLocations[m_iLocations].posStart.SetEmpty();
        m_pLocations[m_iLocations].posEnd.SetEmpty();
    }

    if (pMapStart != NULL)
    {
        // Passing one means passing both!
        ASSERT (pMapEnd != NULL);
        m_pMappedLocations[m_iLocations].posStart = *pMapStart;
        m_pMappedLocations[m_iLocations].posEnd = *pMapEnd;
    }
    else
    {
        ASSERT (pMapEnd == NULL);
        m_pMappedLocations[m_iLocations].posStart.SetEmpty();
        m_pMappedLocations[m_iLocations].posEnd.SetEmpty();
    }

    m_iLocations++;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CError::GetErrorInfo

STDMETHODIMP CError::GetErrorInfo (long *piErrorID, ERRORKIND *pKind, PCWSTR *ppszText)
{
    if (piErrorID != NULL)
        *piErrorID = m_iID;
    if (pKind != NULL)
        *pKind = m_iKind;
    if (ppszText != NULL)
        *ppszText = m_sbstrText;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CError::GetLocationCount

STDMETHODIMP CError::GetLocationCount (long *piLocations)
{
    if (piLocations == NULL)
        return E_POINTER;

    *piLocations = m_iLocations;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CError::GetUnmappedLocationAt - returns the actual input location

STDMETHODIMP CError::GetUnmappedLocationAt (long iIndex, PCWSTR *ppszFileName, POSDATA *pposStart, POSDATA *pposEnd)
{
    if (iIndex < 0 || iIndex >= (long)m_iLocations)
        return E_INVALIDARG;

    if (ppszFileName != NULL)
        *ppszFileName = m_pLocations[iIndex].bstrFile;

    if (pposStart != NULL)
        *pposStart = m_pLocations[iIndex].posStart;

    if (pposEnd != NULL)
        *pposEnd = m_pLocations[iIndex].posEnd;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// CError::GetLocationAt - returns the mapped (via #line) location

STDMETHODIMP CError::GetLocationAt (long iIndex, PCWSTR *ppszFileName, POSDATA *pposStart, POSDATA *pposEnd)
{
    if (iIndex < 0 || iIndex >= (long)m_iLocations)
        return E_INVALIDARG;

    if (ppszFileName != NULL)
        *ppszFileName = m_pMappedLocations[iIndex].bstrFile;

    if (pposStart != NULL)
        *pposStart = m_pMappedLocations[iIndex].posStart;

    if (pposEnd != NULL)
        *pposEnd = m_pMappedLocations[iIndex].posEnd;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::CErrorContainer

CErrorContainer::CErrorContainer () :
    m_ppErrors(NULL),
    m_iErrors(0),
    m_iWarnings(0),
    m_iCount(0),
    m_pReplacements(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::~CErrorContainer

CErrorContainer::~CErrorContainer ()
{
    if (m_ppErrors != NULL)
    {
        for (long i=0; i<m_iCount; i++)
        {
            ASSERT (m_ppErrors[i] != NULL);
            if (m_ppErrors[i] != NULL)
                m_ppErrors[i]->Release();
        }

        VSFree (m_ppErrors);
    }

    if (m_pReplacements != NULL)
        m_pReplacements->Release();
}

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::CreateInstance

HRESULT CErrorContainer::CreateInstance (ERRORCATEGORY iCategory, DWORD_PTR dwID, CErrorContainer **ppContainer)
{
    HRESULT                     hr = E_OUTOFMEMORY;
    CComObject<CErrorContainer> *pObj;

    if (SUCCEEDED (hr = CComObject<CErrorContainer>::CreateInstance (&pObj)))
    {
        if (FAILED (hr = pObj->Initialize (iCategory, dwID)))
        {
            delete pObj;
        }
        else
        {
            *ppContainer = pObj;
            pObj->AddRef();
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::Initialize

HRESULT CErrorContainer::Initialize (ERRORCATEGORY iCategory, DWORD_PTR dwID)
{
    m_iCategory = iCategory;
    m_dwID = dwID;
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::Clone

HRESULT CErrorContainer::Clone (CErrorContainer **ppContainer)
{
    // Can't clone a container while it is in replacement mode!
    if (m_pReplacements != NULL)
    {
        VSFAIL ("Can't clone an error container while it is in replace mode!");
        return E_FAIL;
    }

    CErrorContainer *pNew;
    HRESULT         hr;

    if (SUCCEEDED (hr = CreateInstance (m_iCategory, m_dwID, &pNew)))
    {
        pNew->m_ppErrors = (ICSError **)VSAlloc (((m_iCount + 7) & ~7) * sizeof (ICSError *));
        if (pNew->m_ppErrors == NULL)
        {
            delete pNew;
            return E_OUTOFMEMORY;
        }

        pNew->m_iCount = m_iCount;
        for (long i=0; i<m_iCount; i++)
        {
            pNew->m_ppErrors[i] = m_ppErrors[i];
            pNew->m_ppErrors[i]->AddRef();
        }

        pNew->m_iErrors = m_iErrors;
        pNew->m_iWarnings = m_iWarnings;
        *ppContainer = pNew;            // NOTE:  Ref count is transferred here...
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::BeginReplacement
//
// This function turns "replace mode" on for this container.  All incoming
// errors get added to the replacement container instead of this one, and get
// retrofitted into this when EndReplacement is called (replacing errors that
// fall within the range supplied).

HRESULT CErrorContainer::BeginReplacement ()
{
    if (m_pReplacements != NULL)
    {
        VSFAIL ("Can't enter replace mode a second time!");
        return E_FAIL;
    }

    return CreateInstance (m_iCategory, m_dwID, &m_pReplacements);
}

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::EndReplacement
//
// This function is called to terminate "replace mode" and place all errors (if
// any) in the replacement container into this container, first removing all
// existing errors that fall within the given range.
//
// Returns S_FALSE if nothing changed.

HRESULT CErrorContainer::EndReplacement (const POSDATA &posStart, const POSDATA &posEnd)
{
    if (m_pReplacements == NULL)
    {
        VSFAIL ("Can't terminate a replacement that hasn't started!");
        return E_FAIL;
    }

    long        iFirst = m_iCount, iLast = m_iCount;
    POSDATA     pos;

    // Okay, find the span of errors that fall within the given range.  It MUST
    // be a contiguous range (because errors are sorted by position when added).
    // Note that this is a linear search -- tolerable because the number of
    // errors coming from EC_TOKENIZATION (the only category that uses this so
    // far) is expected to be small, and because replacement doesn't happen often.
    if (posEnd.iLine == 0 && posEnd.iChar == 0)
    {
        // In this case, we replace all existing errors
        iFirst = 0;
    }
    else
    {
        for (long i=0; i<m_iCount; i++)
        {
            ASSERT (m_ppErrors[i] != NULL);
            if (FAILED (m_ppErrors[i]->GetLocationAt (0, NULL, &pos, NULL)))
                continue;

            if (iFirst == m_iCount && pos >= posStart)
                iFirst = i;

            if (pos > posEnd)
            {
                ASSERT (iFirst != m_iCount);    // If this fires, then posStart > posEnd -- how'd that happen?!?
                iLast = i;
                break;
            }
        }
    }

    // Determine the new size of our array (which is our current count, plus the
    // new ones, minus the ones between iFirst and iLast).  Note that iLast actually
    // refers to the first error past the replacement range that we'll keep.
    BOOL    fChanged = (m_pReplacements->m_iCount + (iLast - iFirst)) > 0;
    long    iDelta = m_pReplacements->m_iCount - (iLast - iFirst);
    long    iNewCount = m_iCount + iDelta;

    // Resize our array (grow only)
    if (iDelta > 0)
    {
        long    iNewSize = ((iNewCount + 7) & ~7) * sizeof (ICSError *);
        void    *pNew = (m_ppErrors == NULL) ? VSAlloc (iNewSize) : VSRealloc (m_ppErrors, iNewSize);

        if (pNew == NULL)
            return E_OUTOFMEMORY;

        m_ppErrors = (ICSError **)pNew;
    }

    // Release the errors that are being replaced, keeping track of warning/error/fatal counts.
    long i;
    for (i=iFirst; i<iLast; i++)
    {
        ERRORKIND   kind;
        m_ppErrors[i]->GetErrorInfo (NULL, &kind, NULL);
        if (kind == ERROR_ERROR)
            m_iErrors--;
        else if (kind == ERROR_WARNING)
            m_iWarnings--;
        m_ppErrors[i]->Release();
#ifdef _DEBUG
        memset(&m_ppErrors[i], 0xafafafaf, sizeof(ICSError*));
#endif
    }

    // Move the errors past the replaced ones to their new positions
    memmove (m_ppErrors + iLast + iDelta, m_ppErrors + iLast, (m_iCount - iLast) * sizeof (ICSError *));

    // Copy the errors from the replacement container into the array -- again, keeping counts of
    // warnings/errors/fatals.
    for (i=0; i<m_pReplacements->m_iCount; i++)
    {
        ERRORKIND   kind;
        m_ppErrors[iFirst + i] = m_pReplacements->m_ppErrors[i];
        m_ppErrors[iFirst + i]->AddRef();
        m_ppErrors[iFirst + i]->GetErrorInfo (NULL, &kind, NULL);
        if (kind == ERROR_ERROR)
            m_iErrors++;
        else if (kind == ERROR_WARNING)
            m_iWarnings++;
    }

#ifdef _DEBUG
    // do some validation
    for (long q=0; q<iNewCount; q++)
        ASSERT (m_ppErrors[q] != (ICSError *)0xafafafafafafafaf);
#endif

    // Adjust our count
    m_iCount = iNewCount;

    // Get rid of the replacement container
    m_pReplacements->Release();
    m_pReplacements = NULL;

    // Fin!
    return fChanged ? S_OK : S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::AddError

HRESULT CErrorContainer::AddError (ICSError *pError)
{
    // If we're in replacement mode, this goes in our replacement container
    if (m_pReplacements != NULL)
        return m_pReplacements->AddError (pError);

    if ((m_iCount & 7) == 0)
    {
        // Time to grow
        long    iSize = (m_iCount + 8) * sizeof (ICSError *);
        void    *pNew = (m_ppErrors == NULL) ? VSAlloc (iSize) : VSRealloc (m_ppErrors, iSize);

        if (pNew != NULL)
            m_ppErrors = (ICSError **)pNew;
        else
            return E_OUTOFMEMORY;
    }

    POSDATA pos, posNew;
    long    iInsert = m_iCount;

    if (SUCCEEDED (pError->GetLocationAt (0, NULL, &posNew, NULL)))
    {
        // Search backwards to see if we have to slide any errors down.  This doesn't
        // happen very often, but just in case...
        for (long i=m_iCount - 1; i >= 0; i--)
        {
            ASSERT (m_ppErrors[i] != NULL);
            if (FAILED (m_ppErrors[i]->GetLocationAt (0, NULL, &pos, NULL)))
                continue;

            if (pos <= posNew)
            {
                iInsert = i + 1;
                break;
            }
        }

        if (iInsert < m_iCount)
            memmove (m_ppErrors + iInsert + 1, m_ppErrors + iInsert, (m_iCount - iInsert) * sizeof (ICSError *));
    }

    m_ppErrors[iInsert] = pError;
    pError->AddRef();
    m_iCount++;

    ERRORKIND   kind;

    // Keep track of errors/warnings/fatals (fatals are calculated)
    pError->GetErrorInfo (NULL, &kind, NULL);
    if (kind == ERROR_WARNING)
        m_iWarnings++;
    else if (kind == ERROR_ERROR)
        m_iErrors++;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::ReleaseAllErrors

void CErrorContainer::ReleaseAllErrors ()
{
    for (long i=0; i<m_iCount; i++)
    {
        ASSERT (m_ppErrors[i] != NULL);
        if (m_ppErrors[i] != NULL)
            m_ppErrors[i]->Release();
    }

    m_iCount = m_iErrors = m_iWarnings = 0;
}

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::GetContainerInfo

STDMETHODIMP CErrorContainer::GetContainerInfo (ERRORCATEGORY *pCategory, DWORD_PTR *pdwID)
{
    if (pCategory != NULL)
        *pCategory = m_iCategory;

    if (pdwID != NULL)
        *pdwID = m_dwID;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::GetErrorCount

STDMETHODIMP CErrorContainer::GetErrorCount (long *piWarnings, long *piErrors, long *piFatals, long *piTotal)
{
    if (piWarnings != NULL)
        *piWarnings = m_iWarnings;

    if (piErrors != NULL)
        *piErrors = m_iErrors;

    // NOTE:  Fatals is calculated by the total minus the errors and warnings
    if (piFatals != NULL)
        *piFatals = m_iCount - (m_iWarnings + m_iErrors);

    if (piTotal != NULL)
        *piTotal = m_iCount;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CErrorContainer::GetErrorAt

STDMETHODIMP CErrorContainer::GetErrorAt (long iIndex, ICSError **ppError)
{
    if (iIndex < 0 || iIndex >= m_iCount)
        return E_INVALIDARG;

    *ppError = m_ppErrors[iIndex];
    (*ppError)->AddRef();
    return S_OK;
}
