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
// File: incbuild.cpp
//
// Routines for handling incremental builds.
// ===========================================================================

#include "stdafx.h"

#include "compiler.h"

#include "fileiter.h"

#include <io.h>


/*

******* IMPORTANT: The INCBUILD_VERSION constant MUST be changed every
******* IMPORTANT: time a file format change is made to the incremental
******* IMPORTANT: build file.

The file format for the incremental build file:

ascii string "Incremental Build Data for C# Compiler -- this file stores data to enable fast rebuilds."
DWORD INCBUILD_ID
DWORD INCBUILD_VERSION
struct INCBUILD_HDR
length-prefixed unicode strings: preprocessor definitions(';' seperated list).

for each output file:
  struct INCBUILD_OUTFILE
  length-prefixed unicode string: output file name
  ALLOCMAP for outfile
  list of input files

for each input file:
  struct INCBUILD_INFILE
  length-prefixed unicode string: input file name.
  ALLOCMAP for inputfile
  list of types
  list of Global FieldDef tokens (from <PrivateImplementationDetails>)

for each type
  typedef token of the type
  bitfield of input files dependant on the type

*/

#define INCBUILD_HEADERSTRING "Incremental Build Data for C# Compiler -- this file stores data to enable fast rebuilds.\n"
#define INCBUILD_ID  0xC380A349     // Identify this as a incremental build file.
#define INCBUILD_VERSION 7          // Version number of this format.

// Fixed length information about the output file.
struct INCBUILD_HDR
{
    // These variable indicate compiler options that must
    // match to do an incremental build.
    bool emitDebugInfo: 1;      // Debug info on?
    bool optUnusedVars: 1;      // Optimization on?
    bool warnAsErrors: 1;       // Warn as errors on?
    bool assemble: 1;           // emit an assembly
    bool emitChecked: 1;        // chcked or unchecked casts
    bool emitUnsafe: 1;         // allow unsafe constructs

    DWORD cBuildCount;          // Number of incremental builds without a full-rebuild
    DWORD cOutputFiles;         // Number of output files to follow.
    DWORD cInputFiles;          // Number of input files
};


// Fixed length information about an input file
struct INCBUILD_OUTFILE
{
    FILETIME timeOutputWritten; // Time output file was written.
    FILETIME timeDebugWritten;  // Time pdb file was written. (or all 0 to indicate no PDB info)
    DWORD cInputFiles;          // Number of input files
    DWORD cResFiles;            // Number of resource files
    mdToken entryClassToken;    // TypeDef of class containing entrypoint
    bool bDll : 1;              // Is this a DLL (or module)
    bool bConsole : 1;          // Is this a Console or WinApp
    bool bExplicitMain: 1;      // have a /main switch on the command line
};

// Fixed length information about an input file
struct INCBUILD_INFILE
{
    bool isSource: 1;           // true = source, false = metadata
    bool hasGlobalAttr: 1;
    DWORD id;                   // Input file ID number
    FILETIME timeLastChange;    // Time input file was last changed
    DWORD cTypeDefs;            // Number of typedefs owned by this input file.
    DWORD cGlobalFields;        // Number of global FieldDef tokens owned by this file.
};

// Fixed length information about a resource file
struct INCBUILD_RESFILE
{
    bool isLinked:1;
    FILETIME timeLastChange;    // Time input file was last changed
};

/*
 * INCBUILDREADTALLY
 *
 * struct containing count information used during reading of the incremental
 * build file. Used to verify that the set of files hasn't changed since
 * the last build.
 */
struct INCBUILDREADTALLY
{
    BOOL readMetadataFiles;         // have we read the metadata file list
    BOOL canIncrementalBuild;       // have we not found any significant changes
    ULONG cChangedFiles;            // number of input files which have changed
    ULONG cUnchangedFiles;          // number of input files which haven't changed
    ULONG cMetadataFiles;           // number of metadatafiles
};

/*
 * Constructor
 */
INCBUILD::INCBUILD(COMPILER * compiler, OUTFILESYM *outfile)
{
    this->compiler = compiler;
    this->outfile = outfile;
    this->entryClassToken = 0;
    this->cBuildCount = 0;
}

/*
 * Get the name of the incremental build file name. Currently,
 * this is gotten by simply appening ".incr" to the output file
 * name. If the name is too long, false is returned.
 */
const static WCHAR *extension = L".incr";

bool INCBUILD::GetIncbuildFilename(LPWSTR filename, int cchMax)
{
    if (outfile->isUnnamed() || outfile->hasDefaultName) {
        VSFAIL("Cannot do incremental rebuild without a filename");
        return false;
    }

    // Check sizes to make sure it will fit.
    if ((int) (wcslen(outfile->name->text) + wcslen(extension) + 1) > cchMax)
        return false;

    // Copy.
    wcscpy(filename, outfile->name->text);
    wcscat(filename, extension);
    return true;
}


/*
 * Write an length-prefix Unicode string.
 */
void INCBUILD::WriteString(FILE * file, LPCWSTR s)
{
    size_t cch = s ? (int)wcslen(s) : 0;


    _putw((unsigned)cch, file);  // Write length.
    if (cch)
        fwrite(s, sizeof(WCHAR), cch, file);
}

/*
 * Read a length-prefix Unicode string.
 * Returns false if too long for the buffer.
 */
bool INCBUILD::ReadString(FILE * file, LPWSTR s, unsigned cbMax)
{
    unsigned cch = _getw(file);

    if ((cch + 1) * sizeof(WCHAR) > cbMax || cch * sizeof(WCHAR) > cbMax) {
        // Can't read it, just seek past it.
        fseek(file, cch * sizeof(WCHAR), SEEK_CUR);
        return false;
    }
    else {
        // Read filename and zero-terminate.
        fread(s, sizeof(WCHAR), cch, file);
        s[cch] = L'\0';
        return true;
    }
}


/*
 * Determine the last write time of a file. Returns
 * a time of all zeros if it can't be obtained.
 */
FILETIME INCBUILD::GetFileChangeTime(LPCWSTR filename)
{
    FILETIME time;
    HANDLE hFile;

    memset(&time, 0, sizeof(time));  // return zero'd time on error.

    // Open the metadata file.
    hFile = W_CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE || hFile == 0)
        return time;  // ERROR -- couldn't open file.

    // Get the file change time. On failure, time will be unchanged which is the right error indicator.
    GetFileTime(hFile, NULL, NULL, &time);
    CloseHandle(hFile);

    return time;
}

/*
 * Compare two FILETIMEs to see if there are equal, and not
 * the zero time (unavailable).
 */
bool UtilEqualTimes(FILETIME ft1, FILETIME ft2)
{
    if (ft1.dwHighDateTime == ft2.dwHighDateTime &&
        ft1.dwLowDateTime  == ft2.dwLowDateTime  &&
        (ft1.dwHighDateTime != 0 || ft1.dwLowDateTime != 0))
        return true;
    else
        return false;
}

/*
 * Read the incremental build information from the file
 * and perform the initial analysis of file times to
 * determine if files should be rebuilt. If the incremental
 * build file isn't there, or is out of date, then
 * this silently does nothing, which will mean we do
 * a full rebuild.
 */
void INCBUILD::ReadIncrementalInfo(INCBUILDREADTALLY * tally)
{
    WCHAR incbuildName[MAX_PATH];
    WCHAR buffer[MAX_PATH];
    HANDLE hFile;
    FILE * in = NULL;
    INCBUILD_HDR incfilehdr;

    // Get the output file name.
    if (! GetIncbuildFilename(incbuildName, sizeof(incbuildName)))
        return;  // out of luck.

    compiler->NotifyHostOfBinaryFile(incbuildName);

    // Open the file for writing.
    hFile = W_CreateFile(incbuildName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE && hFile != 0)
        in = _fdopen(_open_osfhandle((INT_PTR)hFile, 0), "rb");
    if (!in) {
        // Could not open file -- probably isn't one. We're done.
        goto ERROR_LABEL;
    }

    // Read and check signature/version.
    ASSERT(sizeof(INCBUILD_HEADERSTRING) < MAX_PATH);
    fread(buffer, sizeof(INCBUILD_HEADERSTRING), 1, in);
    if (_getw(in) != (int) INCBUILD_ID || _getw(in) != (int) INCBUILD_VERSION)
        goto ERROR_LABEL;

    // Get fixed size header and verify.
    fread(&incfilehdr, sizeof(incfilehdr), 1, in);
    if (!incfilehdr.emitDebugInfo != !compiler->options.m_fEMITDEBUGINFO)     goto ERROR_LABEL;
    if (!incfilehdr.optUnusedVars != !compiler->options.m_fOPTIMIZATIONS)     goto ERROR_LABEL;
    if (!incfilehdr.warnAsErrors != !compiler->options.m_fWARNINGSAREERRORS)  goto ERROR_LABEL;
    if (!incfilehdr.emitChecked != !compiler->options.m_fCHECKED)             goto ERROR_LABEL;
    if (!incfilehdr.emitUnsafe != !compiler->options.m_fUNSAFE)               goto ERROR_LABEL;
    if (!incfilehdr.assemble != !compiler->BuildAssembly())                   goto ERROR_LABEL;
    if (incfilehdr.cOutputFiles != compiler->cOutputFiles)                    goto ERROR_LABEL;
    if (incfilehdr.cInputFiles != compiler->cInputFiles)                      goto ERROR_LABEL;
    if (incfilehdr.cBuildCount > 30)                                          goto ERROR_LABEL;

    // read in preprocessor definitions(one bug ';' delimited string)
    if (!ReadString(in, buffer, sizeof(buffer)))
        goto ERROR_LABEL;

    // Check preprocessor strings match.
    if ((!!compiler->options.m_sbstrCCSYMBOLS || *buffer) && 
        (!compiler->options.m_sbstrCCSYMBOLS || !*buffer ||
        wcscmp(buffer, compiler->options.m_sbstrCCSYMBOLS)))
        goto ERROR_LABEL;


    // Go through each output file in the database.
    if (this->ReadOutFileInfo(in, tally)) {
        tally->cMetadataFiles = tally->cUnchangedFiles;
        ReadOutFileInfo(in, tally);
    }

    // if anything changed, then mark all files with global attributes as changed
    if (tally->canIncrementalBuild && tally->cChangedFiles != 0) {
        SourceFileIterator infiles;
        for (INFILESYM *infile = infiles.Reset(compiler); infile; infile = infiles.Next()) {
            if (!infile->hasChanged && infile->hasGlobalAttr) {
                infile->hasChanged = true;
                infile->hasGlobalAttr = false;

                tally->cChangedFiles += 1;
                tally->cUnchangedFiles -= 1;
            }
        }
    }

    cBuildCount = incfilehdr.cBuildCount;

DONE:

    if (in)
        fclose(in);
    return;

ERROR_LABEL:
    tally->canIncrementalBuild = false;
    goto DONE;
}

/*
 * Read the incremental build info for a single outfile.
 * May be the metadata outfile, or a regular source outfile
 */
bool INCBUILD::ReadOutFileInfo(FILE *in, INCBUILDREADTALLY * tally)
{
    ULONG cChangedFiles = 0;
    ULONG cUnchangedFiles = 0;
    ULONG cTypes = 0;
    WCHAR filename[MAX_PATH];
    DWORD i;
    OUTFILESYM *outfileCur;
    NAME *outfileName;
    RESFILESYM *res;
    ULONG cResFiles = 0;

    // read the output file header
    INCBUILD_OUTFILE outfilehdr;
    fread(&outfilehdr, sizeof(outfilehdr), 1, in);
    this->entryClassToken = outfilehdr.entryClassToken;

    //
    // read in the output file name, and find its symbol
    //
    if (!ReadString(in, filename, sizeof(filename)))
        goto ERROR_LABEL;
    outfileName = compiler->namemgr->LookupString(filename);
    if (outfileName) {
        outfileCur = compiler->symmgr.LookupGlobalSym(outfileName, compiler->symmgr.GetFileRoot(), MASK_ALL)->asOUTFILESYM();
    } else {
        outfileCur = NULL;
    }
    if (!outfileCur) {
        OutFileIterator outfiles;
        for (outfileCur = outfiles.Reset(compiler); outfileCur; outfileCur = outfiles.Next()) {
            if (outfileCur->isUnnamed() || outfileCur->hasDefaultName || !_wcsicmp(filename, outfileCur->name->text)) {
                break;
            }
        }
        if (!outfileCur)
            goto ERROR_LABEL;
    }
    else if (outfileName->text && *outfileName->text)
    {
        compiler->NotifyHostOfBinaryFile(outfileName->text);
    }
    ASSERT(outfileCur);

    // check the /main switch
    if (outfileName) {
        if (outfilehdr.bExplicitMain) {
            if (outfileCur->hasDefaultName || outfileCur->isUnnamed()) {
                // user removed the /main switch
                goto ERROR_LABEL;
            }
            // read in the old /main option
            if (!ReadString(in, filename, sizeof(filename)))
                goto ERROR_LABEL;
            if (wcscmp(outfile->entryClassName, filename))
                goto ERROR_LABEL;
        }

    }

    //
    // read the ALLOCMAP
    //
    if (*outfileCur->name->text) {
        outfileCur->allocMapAll = ALLOCMAP::Read(&compiler->globalSymAlloc, in);
        outfileCur->allocMapNoSource = ALLOCMAP::Read(&compiler->globalSymAlloc, in);
    }

    // attempting to read metadata files twice
    if (!*outfileCur->name->text && tally->readMetadataFiles)
        goto ERROR_LABEL;

    // check the target type (we don't have to check for module here)
    if (outfileCur->isDll != outfilehdr.bDll ||
        outfileCur->isConsoleApp != outfilehdr.bConsole)
        goto ERROR_LABEL;
    
    // check for the right output file
    if (*outfileCur->name->text && outfileCur != outfile)
        goto ERROR_LABEL;

    // check that the output file hasn't changed
    if (!outfileCur->isUnnamed() && !outfileCur->hasDefaultName && *outfileCur->name->text && ! UtilEqualTimes(outfilehdr.timeOutputWritten, GetFileChangeTime(outfileCur->name->text)))
        goto ERROR_LABEL;

    if (compiler->options.m_fEMITDEBUGINFO && !outfileCur->isUnnamed() && !outfileCur->hasDefaultName && *outfileCur->name->text) {
        // check that the pdb hasn't changed
        CPEFile::GetPDBFileName(outfileCur, filename);
        if (! UtilEqualTimes(outfilehdr.timeDebugWritten, GetFileChangeTime(filename)))
            goto ERROR_LABEL;
    }

    // check that the number of input files hasn't changed
    if (outfilehdr.cInputFiles != outfileCur->cInputFiles) goto ERROR_LABEL;

    // number of resource files hasn't changed
    for( res = outfileCur->firstResfile(); res != NULL; res = res->nextResfile()) {
        cResFiles++;
    }
    if (outfilehdr.cResFiles != cResFiles) goto ERROR_LABEL;

    // Go through each input file in the database.
    for (i = 0; i < outfilehdr.cInputFiles; ++i) {

        // Read the fixed size part, and the filename
        INCBUILD_INFILE infilehdr;
        fread(&infilehdr, sizeof(infilehdr), 1, in);
        ReadString(in, filename, sizeof(filename));

        // Find the infile symbol corresponding.
        NAME *infileName = compiler->namemgr->LookupString(filename);
        INFILESYM * infile = NULL;
        if (infileName) {
            infile = compiler->symmgr.LookupGlobalSym(infileName, outfileCur, MASK_ALL)->asINFILESYM();
        }
        if (!infile) {
            InFileIterator infiles;
            for (infiles.Reset(outfileCur); infiles.Current(); infiles.Next()) {
                if (!_wcsicmp(filename, infiles.Current()->name->text)) {
                    infile = infiles.Current();
                    break;
                }
            }
            if (!infile)
                goto ERROR_LABEL;
        }

        // can't change from a source to a metadata file
        if (!infile->isSource != !infilehdr.isSource) goto ERROR_LABEL;
        infile->hasGlobalAttr = infilehdr.hasGlobalAttr;

        // get file change time for metadata files
        ASSERT(infile->timeLastChange.dwHighDateTime != 0 && infile->timeLastChange.dwLowDateTime != 0);

        if (!UtilEqualTimes(infile->timeLastChange, infilehdr.timeLastChange)) {
            if (!infile->isSource) {
                //
                // external metadata file changed
                //
                goto ERROR_LABEL;
            }

            cChangedFiles += 1;
            infile->hasChanged = true;
        } else {
            cUnchangedFiles += 1;
            infile->hasChanged = false;
        }

        if (infile->isSource) {
            // read the allocmap
            infile->allocMap = ALLOCMAP::Read(&compiler->globalSymAlloc, in);

            // read the typedefs
            infile->cTypesPrevious = infilehdr.cTypeDefs;
            infile->typesPrevious = (INCREMENTALTYPE*) compiler->globalSymAlloc.AllocZero(sizeof(INCREMENTALTYPE) * infilehdr.cTypeDefs);
            for (ULONG k = 0; k < infilehdr.cTypeDefs; k ++ ) {
                INCREMENTALTYPE *inctype = &infile->typesPrevious[k];
                fread(&inctype->token, sizeof(inctype->token), 1, in);
                if (TypeFromToken(inctype->token) != mdtTypeDef)
                    goto ERROR_LABEL;
                inctype->dependantFiles = BITSET::create(compiler->cInputFiles, &compiler->globalSymAlloc, 0x00);
                ASSERT(inctype->dependantFiles->sizeInBytes() == BITSET::rawSizeInBytes(compiler->cInputFiles));
                fread(BITSET::rawBytes(&inctype->dependantFiles), BITSET::rawSizeInBytes(compiler->cInputFiles), 1, in);
            }

            // read the Global FieldDefs
            infile->cGlobalFields = infilehdr.cGlobalFields;
            ULONG count = infilehdr.cGlobalFields;
            while (count > 0) {
                ULONG cRead = min(count, (ULONG)lengthof(infile->globalFields->pTokens));
                ASSERT(cRead > 0);
                INFILESYM::TOKENLIST *pList = (INFILESYM::TOKENLIST*) compiler->globalSymAlloc.AllocZero(sizeof(INFILESYM::TOKENLIST));
                fread(pList->pTokens, sizeof(mdToken), cRead, in);
                pList->pNext = infile->globalFields;
                infile->globalFields = pList;
                count -= cRead;
            }
        }

        infile->idIncbuild = infilehdr.id;

        cTypes += infilehdr.cTypeDefs;
    }

    for (i = 0; i < outfilehdr.cResFiles; ++i) {
        // Read the fixed size part, and the filename
        INCBUILD_RESFILE resfilehdr;
        fread(&resfilehdr, sizeof(resfilehdr), 1, in);
        WCHAR resourceName[MAX_PATH];
        ReadString(in, resourceName, sizeof(resourceName));
        ReadString(in, filename, sizeof(filename));

        // Find the infile symbol corresponding.
        NAME *resName = compiler->namemgr->LookupString(resourceName);
        RESFILESYM * resfile = NULL;
        if (resName) {
            resfile = compiler->symmgr.LookupGlobalSym(resName, outfileCur, MASK_RESFILESYM)->asRESFILESYM();
        }
        if (!resfile || _wcsicmp(resfile->filename, filename)) {
            goto ERROR_LABEL;
        }

        // can't change from linked to embedded
        if (!resfile->isEmbedded != !!resfilehdr.isLinked) goto ERROR_LABEL;

        // get file change time for resource file
        if (!UtilEqualTimes(GetFileChangeTime(filename), resfilehdr.timeLastChange))
            goto ERROR_LABEL;
    }

    tally->cChangedFiles += cChangedFiles;
    tally->cUnchangedFiles += cUnchangedFiles;

    outfileCur->cOldTypes = cTypes;
    if (!*outfileCur->name->text) {
        tally->readMetadataFiles = true;
        return true;
    }

    return false;
ERROR_LABEL:

    tally->canIncrementalBuild = false;
    return false;
}

/*
 * Write the incremental build information to the incremental file.
 */
void INCBUILD::WriteIncrementalInfo(BOOL firstOutputFile)
{
    WCHAR incbuildName[MAX_PATH];
    HANDLE hFile;
    FILE * out = NULL;
    INCBUILD_HDR incfilehdr;

    // Get the output file name.
    if (! GetIncbuildFilename(incbuildName, sizeof(incbuildName)))
        return;  // out of luck.

    // Open the file for writing.
    hFile = W_CreateFile(incbuildName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE && hFile != 0)
        out = _fdopen(_open_osfhandle((INT_PTR)hFile, 0), "wb");
    if (!out) {
        // Could not open file. Report error.
        compiler->Error(NULL, ERR_IncBuildWrite, incbuildName);
        return;
    }

    // Signature/version.
    fwrite(INCBUILD_HEADERSTRING, sizeof(INCBUILD_HEADERSTRING), 1, out);
    _putw(INCBUILD_ID, out);
    _putw(INCBUILD_VERSION, out);

    // Fill in the output file header and write it.
    memset(&incfilehdr, 0, sizeof(incfilehdr));
    incfilehdr.emitDebugInfo = !!compiler->options.m_fEMITDEBUGINFO;
    incfilehdr.optUnusedVars = !!compiler->options.m_fOPTIMIZATIONS;
    incfilehdr.warnAsErrors = !!compiler->options.m_fWARNINGSAREERRORS;
    incfilehdr.assemble = compiler->BuildAssembly();
    incfilehdr.emitChecked = !!compiler->options.m_fCHECKED;
    incfilehdr.emitUnsafe = !!compiler->options.m_fUNSAFE;
    incfilehdr.cOutputFiles = compiler->cOutputFiles;
    incfilehdr.cInputFiles = compiler->cInputFiles;
    incfilehdr.cBuildCount = compiler->IsIncrementalRebuild() ? cBuildCount + 1 : 0;
    fwrite(&incfilehdr, sizeof(incfilehdr), 1, out);

    // Write preprocessor strings
    WriteString(out, compiler->options.m_sbstrCCSYMBOLS);

    
    // write included metadata file info for first out file only
    if (firstOutputFile) {
        WriteOutFileInfo(out, compiler->symmgr.GetMDFileRoot());
    }

    // Write information for the output file.
    WriteOutFileInfo(out, outfile);

    // Close the output file. Check for output error via ferror and delete the file if so.
    int errorOccurred = ferror(out);
    fclose(out);
    if (errorOccurred) {
        compiler->Error(NULL, ERR_IncBuildWriteIO, incbuildName);
        W_DeleteFile(incbuildName);
    }
}

/*
 * write the incrmental build information for a single outfile to a file
 * outfileCur could be the predefined metadata outfile or a regular outfile
 */
void INCBUILD::WriteOutFileInfo(FILE *out, OUTFILESYM *outfileCur)
{
    // write output file header
    INCBUILD_OUTFILE outfilehdr;
    outfilehdr.cInputFiles = outfileCur->cInputFiles;
#if DEBUG
    {
    InFileIterator infiles;
    INFILESYM *infileCur;
    int i = 0;
    for (infileCur = infiles.Reset(outfileCur); infileCur != NULL; infileCur = infiles.Next()) {
        i += 1;
    }
    ASSERT(i == outfilehdr.cInputFiles);
    }
#endif
    outfilehdr.cResFiles = 0;
    RESFILESYM *res;
    for( res = outfileCur->firstResfile(); res != NULL; res = res->nextResfile()) {
        outfilehdr.cResFiles++;
    }
    if (compiler->options.m_fEMITDEBUGINFO && !outfileCur->isUnnamed() && !outfileCur->hasDefaultName && *outfileCur->name->text) {
        WCHAR filename[MAX_PATH];
        CPEFile::GetPDBFileName(outfileCur, filename);
        outfilehdr.timeDebugWritten = GetFileChangeTime(filename);
    } else {
        memset(&outfilehdr.timeDebugWritten, 0, sizeof(outfilehdr.timeDebugWritten));
    }
    outfilehdr.timeOutputWritten = GetFileChangeTime(outfileCur->name->text);
    outfilehdr.entryClassToken = this->entryClassToken;
    outfilehdr.bDll = outfileCur->isDll;
    outfilehdr.bConsole = outfileCur->isConsoleApp;
    outfilehdr.bExplicitMain = (outfileCur->entryClassName != NULL);
    fwrite(&outfilehdr, sizeof(outfilehdr), 1, out);

    // Write output file name.
    WriteString(out, outfileCur->name->text);

    // Write the /main switch
    if (outfilehdr.bExplicitMain)
        WriteString(out, outfileCur->entryClassName);

    // Write allocmap
    if (outfileCur->allocMapAll)
        outfileCur->allocMapAll->Write(out);
    if (outfileCur->allocMapNoSource)
        outfileCur->allocMapNoSource->Write(out);



    INFILESYM * infileCur;
    InFileIterator infiles;
    for (infileCur = infiles.Reset(outfileCur); infileCur != NULL; infileCur = infiles.Next()) {
        INCBUILD_INFILE infilehdr;

        // Fill in the input file header and write it.
        memset(&infilehdr, 0, sizeof(infilehdr));
        infilehdr.isSource = infileCur->isSource;
        infilehdr.hasGlobalAttr = infileCur->hasGlobalAttr;
        infilehdr.id = infileCur->idIncbuild;
        ASSERT(infileCur->timeLastChange.dwHighDateTime != 0 && infileCur->timeLastChange.dwLowDateTime != 0);
        infilehdr.timeLastChange = infileCur->timeLastChange;
        infilehdr.cTypeDefs = infileCur->cTypes;
        infilehdr.cGlobalFields = infileCur->cGlobalFields;
        fwrite(&infilehdr, sizeof(infilehdr), 1, out);

        // Write input filename
        WriteString(out, infileCur->name->text);
        if (infileCur->isSource) {
            // alloc map
            infileCur->allocMap->Write(out);
            // list of types
            TypeIterator types;
            AGGSYM *cls;
            ULONG cTypesWritten = 0;
            for (cls = types.Reset(infileCur); cls != NULL; cls = types.Next()) 
            {
                ASSERT(TypeFromToken(cls->incrementalInfo->token) == mdtTypeDef);
                fwrite(&cls->incrementalInfo->token, sizeof(cls->incrementalInfo->token), 1, out);
                ASSERT(cls->incrementalInfo->dependantFiles->sizeInBytes() == BITSET::rawSizeInBytes(compiler->cInputFiles));
                fwrite(BITSET::rawBytes(&cls->incrementalInfo->dependantFiles), cls->incrementalInfo->dependantFiles->sizeInBytes(), 1, out);
                cTypesWritten += 1;
            }
            ASSERT(cTypesWritten == infileCur->cTypes);

            if (infileCur->cGlobalFields)
            {
                // only the first one is partially full
                ULONG index = infileCur->cGlobalFields & (lengthof(infileCur->globalFields->pTokens) - 1);
				if (index == 0) index = 8;
                fwrite( infileCur->globalFields->pTokens, sizeof(mdToken), index, out);
				for (INFILESYM::TOKENLIST * pList = infileCur->globalFields->pNext; pList != NULL; pList = pList->pNext) {
					ASSERT(index += lengthof(pList->pTokens));
                    fwrite( pList->pTokens, sizeof(mdToken), lengthof(infileCur->globalFields->pTokens), out);
				}
				ASSERT(index == infileCur->cGlobalFields);
            }
        }
    }

    for( res = outfileCur->firstResfile(); res != NULL; res = res->nextResfile()) {
        INCBUILD_RESFILE resfileHdr;
        resfileHdr.isLinked = !res->isEmbedded;
        resfileHdr.timeLastChange = GetFileChangeTime(res->filename);
        ASSERT(resfileHdr.timeLastChange.dwHighDateTime != 0 || resfileHdr.timeLastChange.dwLowDateTime != 0);
        fwrite(&resfileHdr, sizeof(resfileHdr), 1, out);
        WriteString(out, res->name->text);
        WriteString(out, res->filename);
    }
}

/*
 * Reads the incremental build information for a compilation
 */
bool INCBUILD::ReadIncrementalInfo(COMPILER *compiler, bool *canIncrementalRebuild)
{
    INCBUILDREADTALLY tally;

    ASSERT(compiler->IsIncrementalRebuild());

    memset(&tally, 0, sizeof(tally));
    tally.canIncrementalBuild = true;
    SourceOutFileIterator files;
    for (POUTFILESYM pOutfile = files.Reset(compiler); pOutfile != NULL; pOutfile = files.Next()) {
        // Incremental build is enabled. Create
        // the INCBUILD object to manage the process. Read the
        // incremental database and do the initial analysis of which files
        // to build or not.
        pOutfile->incbuild = new(&compiler->globalSymAlloc) INCBUILD(compiler, pOutfile);
        pOutfile->incbuild->ReadIncrementalInfo(&tally);
    }

    if (!tally.readMetadataFiles) {
        tally.canIncrementalBuild = false;
    }

    if (!tally.canIncrementalBuild)
    {
        // we may have read in a partial list of input file ids.
        // reset the input file ids to avoid chaos.
        // NULL out partially read incbuild info
        ULONG id = 0;
        AllInFileIterator infiles;
        for (infiles.Reset(compiler); infiles.Current(); infiles.Next()) {
            infiles.Current()->cTypesPrevious = 0;
            infiles.Current()->typesPrevious = 0;
            infiles.Current()->hasChanged = true;

            infiles.Current()->idIncbuild = id++;
        }

        ASSERT(compiler->cInputFiles == id);
    } else {
        compiler->inputFilesById = (INFILESYM**) compiler->globalSymAlloc.Alloc(compiler->cInputFiles * sizeof(INFILESYM*));
        AllInFileIterator infiles;
        for (infiles.Reset(compiler); infiles.Current(); infiles.Next()) {
            compiler->inputFilesById[infiles.Current()->idIncbuild] = infiles.Current();
        }
    }

    if (tally.canIncrementalBuild) {
        if (tally.cMetadataFiles == tally.cUnchangedFiles) 
        {
            tally.canIncrementalBuild = false;
        }
    }

    // return true if there is no build to do
    *canIncrementalRebuild = !!tally.canIncrementalBuild;
    return tally.canIncrementalBuild && (tally.cChangedFiles == 0);
}

//****************************************************************************

const ULONG ALLOC_INIT_SIZE = 2;
const ULONG ALLOC_GROW_SIZE = 4;

ULONG ALLOCMAP::SizeInBytes(ULONG numberOfAllocs)
{
    return offsetof(ALLOCMAP, allocs) + numberOfAllocs * sizeof(RVAALLOCATION);
}

ALLOCMAP *ALLOCMAP::Create(NRHEAP *heap)
{
    ALLOCMAP *map = (ALLOCMAP*) heap->Alloc(SizeInBytes(ALLOC_INIT_SIZE));
    map->heap = heap;
    map->nAllocated = ALLOC_INIT_SIZE;
    map->nUsed = 0;

    return map;
}

ALLOCMAP *ALLOCMAP::AddAlloc(ALLOCMAP *map, ULONG offset, ULONG size)
{
    //
    // handle the initial case
    //
    if (map->nUsed == 0)
    {
        map->nUsed = 1;
        map->allocs[0].offset = offset;
        map->allocs[0].size = size;

        return map;
    }

    //
    // check for common case of appending to last alloc
    //
    if (offset == map->MaxUsedOffset()) {
        map->allocs[map->nUsed-1].size += size;
        return map;
    }

    return AddAllocIndex(map, offset, size, map->FindIndex(offset));
}

ALLOCMAP *ALLOCMAP::AddAllocIndex(ALLOCMAP *map, ULONG offset, ULONG size, ULONG index)
{
    ASSERT(map->nUsed != 0 && (index == map->nUsed || 
                              ((index < map->nUsed) && (map->allocs[index].offset + size) >= offset)));
     
    //
    // case 1: new alloc is immediately before current alloc
    //
    if ((index < map->nUsed) && (offset + size) == map->allocs[index].offset)
    {
        map->allocs[index].offset -= size;
        map->allocs[index].size += size;

        //
        // check for coallescing with previous alloc
        //
        if (index > 0 && (map->allocs[index-1].endOffset() == map->allocs[index].offset))
        {
            map->allocs[index-1].size += map->allocs[index].size;
            map->nUsed -= 1;
            memmove(&map->allocs[index], &map->allocs[index+1], (map->nUsed - index) * sizeof(RVAALLOCATION));
        }
    }
    //
    // case 2: new alloc is immediately after previous alloc
    //
    else if ((index < map->nUsed) && (index > 0) && (map->allocs[index-1].endOffset() == offset)) 
    {
        map->allocs[index-1].size += size;
    }
    //
    // case 3: need a new alloc
    //
    else 
    {
        map = IncrementUsedAllocs(map);
        memmove(&map->allocs[index+1], &map->allocs[index], (map->nUsed - index - 1) * sizeof(RVAALLOCATION));
        map->allocs[index].offset = offset;
        map->allocs[index].size = size;
    }

    return map;
}

ALLOCMAP *ALLOCMAP::IncrementUsedAllocs(ALLOCMAP *map)
{
    if (map->nUsed == map->nAllocated)
    {
        ULONG newNumberOfAllocs = map->nAllocated + ALLOC_GROW_SIZE;
        ALLOCMAP *newMap = (ALLOCMAP*) map->heap->Alloc(SizeInBytes(newNumberOfAllocs));
        memcpy(newMap, map, SizeInBytes(map->nAllocated));
        map = newMap;
        map->nAllocated = newNumberOfAllocs;
    }
    else
    {
        ASSERT(map->nUsed < map->nAllocated);
    }

    map->nUsed += 1;
    return map;
}

ULONG ALLOCMAP::PadSize(ULONG offset, ULONG alignment)
{
    ASSERT((alignment & (alignment-1)) == 0);   // alignment must be a power of 2
    return (ULONG(-(int)offset) & (alignment - 1));
}

//
// returns the offset of the block, or INVALID_OFFSET on no more room
//
ALLOCMAP *ALLOCMAP::Allocate(ALLOCMAP *map, ULONG size, ULONG alignment, ULONG *offset, ULONG *realSize, ULONG *realOffset)
{
    // do a linear scan through the allocations looking
    // for a large enough free block
    //
    ULONG lastOffset = 0;
    for (ULONG index = 0; index < map->nUsed; index += 1) {
        ULONG padSize = PadSize(lastOffset, alignment);

        if (map->allocs[index].offset >= (lastOffset + size + padSize))
        {
            map = AddAllocIndex(map, lastOffset, size + padSize, index);

            *realSize = size + padSize;
            *offset = lastOffset + padSize;
            *realOffset = lastOffset;

            return map;
        }

        lastOffset = map->allocs[index].endOffset();
    }

    *offset = INVALID_OFFSET;
    return map;
}

ALLOCMAP *ALLOCMAP::Remove(ALLOCMAP *removeFrom, ALLOCMAP *toRemove)
{
    for (ULONG i = 0; i < toRemove->nUsed; i += 1)
    {
        removeFrom = Remove(removeFrom, toRemove->allocs[i].offset, toRemove->allocs[i].size);
    }

    return removeFrom;
}

ALLOCMAP *ALLOCMAP::Remove(ALLOCMAP *map, ULONG offset, ULONG size)
{
    ULONG index = map->FindIndex(offset);

    //
    // removing from middle of previous block
    //
    if (index == map->nUsed || map->allocs[index].offset > offset) 
    {
        ASSERT (offset > map->allocs[index - 1].offset && (offset + size) <= map->allocs[index - 1].endOffset());

        index -= 1;
        //
        // check for removing from the end of the block
        //
        if (map->allocs[index].endOffset() == (offset + size))
        {
            map->allocs[index].size -= size;
        }
        //
        // need to split the block
        //
        else
        {
            map = IncrementUsedAllocs(map);
            memmove(&map->allocs[index+2], &map->allocs[index+1], (map->nUsed - index - 2) * sizeof(RVAALLOCATION));
            map->allocs[index+1].offset = (offset + size);
            map->allocs[index+1].size = map->allocs[index].endOffset() - (offset + size);
            map->allocs[index].size = offset - map->allocs[index].offset;
        }
    }
    //
    // removing from start of block
    //
    else
    {
        ASSERT (index < map->nUsed && map->allocs[index].offset == offset &&
                map->allocs[index].size >= size);

        map->allocs[index].size -= size;
        map->allocs[index].offset += size;

        //
        // have we removed the entire allocation
        //
        if (map->allocs[index].size == 0)
        {
            map->nUsed -= 1;
            memmove(&map->allocs[index], &map->allocs[index+1], (map->nUsed - index) * sizeof(RVAALLOCATION));
        }
    }

    return map;
}

void ALLOCMAP::Clear()
{
    nUsed = 0;
}

ULONG ALLOCMAP::MaxUsedOffset()
{
    if (nUsed != 0) {
        return allocs[nUsed-1].endOffset();
    } else {
        return 0;
    }
}

ULONG ALLOCMAP::FindIndex(ULONG offset)
{
    //
    // find index of block immediately after the new offset
    //
    ULONG bottom = 0;
    ULONG top = nUsed;
    ULONG index = (bottom + top) >> 1;
    while (top > bottom) 
    {
        if (allocs[index].offset > offset) 
        {
            top = index;
        }
        else if (allocs[index].offset == offset)
        {
            return index;
        }
        else
        {
            ASSERT(allocs[index].offset < offset);
            bottom = index + 1;
        }
        index = (bottom + top) >> 1;
    }

    return index;
}

ALLOCMAP *ALLOCMAP::Read(NRHEAP *heap, FILE *file)
{
    ULONG nUsed = 0;
    fread(&nUsed, sizeof(nUsed), 1, file);
    ULONG nAlloced = max(nUsed, ALLOC_INIT_SIZE);
    ALLOCMAP *map = (ALLOCMAP*) heap->Alloc(SizeInBytes(nAlloced));
    map->nAllocated = nAlloced;
    map->heap = heap;
    map->nUsed = nUsed;
    fread(map->allocs, sizeof(RVAALLOCATION), nUsed, file);

    return map;
}

void ALLOCMAP::Write(FILE *file)
{
    fwrite(&nUsed, sizeof(nUsed), 1, file);
    fwrite(allocs, sizeof(RVAALLOCATION), nUsed, file);
}
