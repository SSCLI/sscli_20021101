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
//+---------------------------------------------------------------------------
//
//  File:       build.h
//
//  Contents:   Main Include file for build.exe
//----------------------------------------------------------------------------

#ifndef __BUILD_H__
#define __BUILD_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <rotor_palrt.h>
#define _isatty(x) FALSE


#define UINT DWORD
#define HDIR HANDLE


#define ClearLine()


//
// Types and Constant Definitions
//

#if DBG
#define DEBUG_1 (fDebug & 1)
#else
#define DEBUG_1 FALSE
#endif

BOOL fDebug;
#define DEBUG_2 (fDebug & 3)
#define DEBUG_4 (fDebug & 4)

#if PLATFORM_UNIX
#define EOL     "\012"
#define PATH_SEPARATOR  "/"
#define PATH_SEPARATOR_CHAR  '/'
#else
#define EOL     "\015\012"
#define PATH_SEPARATOR  "\\"
#define PATH_SEPARATOR_CHAR  '\\'
#endif

// Accept '/' or '\\' as a path separator on all platforms.
BOOL IsPathSeparator(CHAR ch);

// Returns a pointer to the first path separator character
// in the string.
LPSTR FindFirstPathSeparator(LPSTR str);

// Returns a pointer to the last path separator character
// in the string.
LPSTR FindLastPathSeparator(LPSTR str);

//
// Target specific dirs file name.
//

extern LPSTR pszTargetDirs;

#define MAX_TARGET_MACHINES 3

typedef struct _TARGET_MACHINE_INFO {
    UCHAR SourceSubDirMask;     // TMIDIR_I386
    LPSTR Description;          // "i386"
    LPSTR Switch;               // "-386"
    LPSTR Switch2;              // "-x86"
    LPSTR MakeVariable;         // "386=1"
    LPSTR SourceVariable;       // "i386_SOURCES"
    LPSTR ObjectVariable;       // "386_OBJECTS"
    LPSTR AssociateDirectory;   // "i386"
    LPSTR SourceDirectory;      // "i386"
    LPSTR TargetDirs;           // "i386dirs"
    LPSTR ObjectDirectory[2];   // "i386" -- initialize only first entry
    ULONG DirIncludeMask;       // Platform/Group/etc.
    LPSTR ObjectMacro;          // don't initialize

} TARGET_MACHINE_INFO, *PTARGET_MACHINE_INFO;

typedef union DOS_TIME {
    struct {
#if BIGENDIAN
        WORD wDate;
        WORD wTime;
#else   // BIGENDIAN
        WORD wTime;
        WORD wDate;
#endif  // BIGENDIAN
    } time;
    ULONG value;
} DOS_TIME;

#define DIR_INCLUDE_NONE     0x00000000
#define DIR_INCLUDE_X86      0x00000001
//                           0x00000002
#define DIR_INCLUDE_IA64     0x00000004
//                           0x00000008
#define DIR_INCLUDE_WIN32    0x00000010
#define DIR_INCLUDE_WIN64    0x00000020
#define DIR_INCLUDE_RISC     0x00000040
#define DIR_INCLUDE_DYNAMIC  0x00000100
#define DIR_INCLUDE_ALL      0xffffffff

//If dynamic command line argument is used then these variables store platform information.
extern LPSTR DynamicProcessor;
extern LPSTR DynamicOpSys;
extern UINT MaxInitializedTargets;

// It's possible to have SOURCES= entries of the following forms:
//      entry           SourceSubDirMask
//      -----           ----------------
//      foo.c                    0
//      i386\foo.c               1
//      ia64\foo.c               4
//      ..\foo.c                80
//      ..\i386\foo.c           81
//      ..\ia64\foo.c           84

#define TMIDIR_I386     0x0001
#define TMIDIR_IA64     0x0004
#define TMIDIR_DYNAMIC  0x0008
#define TMIDIR_PARENT   0x0080  // or'd in with above bits


#define SIG_DIRREC      0x44644464      // "DdDd"

#ifdef SIG_DIRREC
#define SIG_FILEREC     0x46664666      // "FfFf"
#define SIG_INCLUDEREC  0x49694969      // "IiIi"
#define SIG_SOURCEREC   0x53735373      // "SsSs"
#define SigCheck(s)     s
#else
#define SigCheck(s)
#endif

#define AssertDir(pdr) \
        SigCheck(assert((pdr) != NULL && (pdr)->Sig == SIG_DIRREC))

#define AssertOptionalDir(pdr) \
        SigCheck(assert((pdr) == NULL || (pdr)->Sig == SIG_DIRREC))

#define AssertFile(pfr) \
        SigCheck(assert((pfr) != NULL && (pfr)->Sig == SIG_FILEREC))

#define AssertOptionalFile(pfr) \
        SigCheck(assert((pfr) == NULL || (pfr)->Sig == SIG_FILEREC))

#define AssertInclude(pir) \
        SigCheck(assert((pir) != NULL && (pir)->Sig == SIG_INCLUDEREC))

#define AssertOptionalInclude(pir) \
        SigCheck(assert((pir) == NULL || (pir)->Sig == SIG_INCLUDEREC))

#define AssertSource(psr) \
        SigCheck(assert((psr) != NULL && (psr)->Sig == SIG_SOURCEREC))

#define AssertOptionalSource(psr) \
        SigCheck(assert((psr) == NULL || (psr)->Sig == SIG_SOURCEREC))

//
// Information about source directories is stored an in-memory database.
// The information is saved on disk by writing the contents of the database
// to "build.dat".  It is reloaded from disk for subsequent invocations,
// and re-written only when it has been updated.
//


typedef struct _INCLUDEREC {
    SigCheck(ULONG Sig;)
    struct _INCLUDEREC *Next;     // static list describes original arcs
    struct _INCLUDEREC *NextTree; // dynamic list -- cycles are collapsed
    struct _FILEREC *pfrCycleRoot;
    struct _FILEREC *pfrInclude;
    USHORT Version;
    USHORT IncFlags;
    char Name[1];
} INCLUDEREC, *PINCLUDEREC;


#define INCLUDEDB_LOCAL         0x0001  // include "foo.h"
#define INCLUDEDB_POST_HDRSTOP  0x0002  // appears after #pragma hdrstop
#define INCLUDEDB_MISSING       0x0400  // include file was once missing
#define INCLUDEDB_GLOBAL        0x0800  // include file is in global directory
#define INCLUDEDB_SNAPPED       0x1000  // include file snapped
#define INCLUDEDB_CYCLEALLOC    0x2000  // allocated to flatten cycle
#define INCLUDEDB_CYCLEROOT     0x4000  // moved to root file to flatten cycle
#define INCLUDEDB_CYCLEORPHAN   0x8000  // orphaned to flatten cycle

// Flags preserved when loading build.dat:

#define INCLUDEDB_DBPRESERVE    (INCLUDEDB_LOCAL | INCLUDEDB_POST_HDRSTOP)



#define IsCleanTree(pir)        \
  ((pir)->NextTree == NULL &&   \
   ((pir)->IncFlags &           \
    (INCLUDEDB_CYCLEALLOC | INCLUDEDB_CYCLEROOT | INCLUDEDB_CYCLEORPHAN)) == 0)


#if DBG
VOID AssertCleanTree(INCLUDEREC *pir, OPTIONAL struct _FILEREC *pfr);
#else
#define AssertCleanTree(pir, pfr)       assert(IsCleanTree(pir))
#endif

//
// Make file description structure definition.
//

typedef struct _FILEDESC {
    LPSTR   pszPattern;         //  pattern to match file name
    LPSTR   pszCommentToEOL;    //  comment-to-eol string
    BOOL    fNeedFileRec;       //  TRUE => file needs a file record
    ULONG   FileFlags;          //  flags to be set in file record
    ULONG   DirFlags;           //  flags to be set in directory record
} FILEDESC;

extern FILEDESC FileDesc[];

typedef struct _FILEREC {
    SigCheck(ULONG Sig;)
    struct _FILEREC *Next;
    struct _DIRREC *Dir;
    INCLUDEREC *IncludeFiles;       // static list describes original arcs
    INCLUDEREC *IncludeFilesTree;   // dynamic list -- cycles are collapsed
    struct _FILEREC *NewestDependency;
    LPSTR  pszCommentToEOL;         // comment-to-eol string in source
    DOS_TIME DateTime;
    DOS_TIME DateTimeTree;            // Newest DateTime for included tree
    ULONG  TotalSourceLines;        // line count in all included files
    ULONG  FileFlags;
    ULONG  SourceLines;
    USHORT Attr;
    USHORT SubDirIndex;
    USHORT Version;
    USHORT GlobalSequence;          // Sequence number for dynamic include tree
    USHORT LocalSequence;           // Sequence number for dynamic include tree
    USHORT idScan;                  // id used for detecting multiple inclusion
    USHORT CheckSum;                // Name checksum
    UCHAR fDependActive;            // TRUE-> we're scanning at or below this file.
    char Name[1];
} FILEREC, *PFILEREC;

#define MAKE_DATE_TIME( date, time )    \
    ((ULONG)(((USHORT)(time)) | ((ULONG)((USHORT)(date))) << 16))

#define FILEDB_SOURCE           0x00000001
#define FILEDB_DIR              0x00000002
#define FILEDB_HEADER           0x00000004
#define FILEDB_ASM              0x00000008
#define FILEDB_MASM             0x00000010
#define FILEDB_RC               0x00000020
#define FILEDB_C                0x00000040
#define FILEDB_MIDL             0x00000080
#define FILEDB_ASN              0x00000100
#define FILEDB_JAVA             0x00000200
#define FILEDB_MOF              0x00000400
#define FILEDB_CSHARP           0x00000800
#define FILEDB_SCANNED          0x00001000
#define FILEDB_OBJECTS_LIST     0x00002000
#define FILEDB_FILE_MISSING     0x00004000
#define FILEDB_MKTYPLIB         0x00008000
#define FILEDB_MULTIPLEPASS     0x00010000
#define FILEDB_PASS0            0x00020000
#define FILEDB_SOURCEREC_EXISTS 0x00040000
#define FILEDB_VBP              0x00080000

// Flags preserved when loading build.dat:

#define FILEDB_DBPRESERVE       (FILEDB_SOURCE |       \
                                 FILEDB_DIR |          \
                                 FILEDB_HEADER |       \
                                 FILEDB_ASM |          \
                                 FILEDB_MASM |         \
                                 FILEDB_RC |           \
                                 FILEDB_C |            \
                                 FILEDB_MIDL |         \
                                 FILEDB_ASN |          \
                                 FILEDB_JAVA |         \
                                 FILEDB_MOF |          \
                                 FILEDB_VBP |          \
                                 FILEDB_CSHARP |       \
                                 FILEDB_MKTYPLIB |     \
                                 FILEDB_MULTIPLEPASS | \
                                 FILEDB_PASS0)


typedef struct _SOURCEREC {
    SigCheck(ULONG Sig;)
    struct _SOURCEREC *psrNext;
    FILEREC *pfrSource;
    UCHAR SourceSubDirMask;
    UCHAR SrcFlags;
} SOURCEREC;

#define SOURCEDB_SOURCES_LIST           0x01
#define SOURCEDB_FILE_MISSING           0x02
#define SOURCEDB_PCH                    0x04
#define SOURCEDB_OUT_OF_DATE            0x08
#define SOURCEDB_COMPILE_NEEDED         0x10


typedef struct _DIRSUP {
    LPSTR TestType;
    LPSTR LocalIncludePath;
    LPSTR UserIncludePath;
    LPSTR LastIncludePath;
    LPSTR PchIncludeDir;
    LPSTR PchInclude;
    LPSTR PchTargetDir;
    LPSTR PchTarget;
    LPSTR PassZeroHdrDir;
    LPSTR PassZeroSrcDir1;
    LPSTR PassZeroSrcDir2;
    LPSTR ConditionalIncludes;
    DOS_TIME DateTimeSources;
    ULONG IdlType;
    ULONG fNoTarget;
    LPSTR SourcesVariables[MAX_TARGET_MACHINES + 1];
    SOURCEREC *psrSourcesList[MAX_TARGET_MACHINES + 1];
} DIRSUP;


typedef struct _DIRREC {
    SigCheck(ULONG Sig;)
    struct _DIRREC *Next;
    DIRSUP *pds;                 // Used to preserve info from pass zero
    PFILEREC Files;
    LPSTR TargetPath;
    LPSTR TargetPathLib;
    LPSTR TargetName;
    LPSTR TargetExt;
    LPSTR KernelTest;
    LPSTR UserAppls;
    LPSTR UserTests;
    LPSTR NTTargetFile0;
    LPSTR Pch;
    LPSTR PchObj;
    LONG SourceLinesToCompile;
    LONG PassZeroLines;
    ULONG DirFlags;
    ULONG RecurseLevel;
    USHORT FindCount;
    USHORT CountSubDirs;
    SHORT CountOfFilesToCompile;
    SHORT CountOfPassZeroFiles;
    USHORT CheckSum;                // Name checksum
    char Name[1];
} DIRREC, *PDIRREC;


#define DIRDB_SOURCES           0x00000001
#define DIRDB_DIRS              0x00000002
#define DIRDB_MAKEFILE          0x00000004
#define DIRDB_MAKEFIL0          0x00000008
#define DIRDB_TARGETFILE0       0x00000010
#define DIRDB_TARGETFILES       0x00000020
#define DIRDB_RESOURCE          0x00000040
#define DIRDB_PASS0             0x00000080

#define DIRDB_SOURCES_SET       0x00000100


#define DIRDB_CHICAGO_INCLUDES  0x00000800

#define DIRDB_NEW               0x00001000
#define DIRDB_SCANNED           0x00002000
#define DIRDB_SHOWN             0x00004000
#define DIRDB_GLOBAL_INCLUDES   0x00008000

#define DIRDB_SYNCHRONIZE_BLOCK 0x00010000
#define DIRDB_SYNCHRONIZE_DRAIN 0x00020000
#define DIRDB_COMPILENEEDED     0x00040000
#define DIRDB_COMPILEERRORS     0x00080000

#define DIRDB_SOURCESREAD       0x00100000
#define DIRDB_DLLTARGET         0x00200000
#define DIRDB_LINKNEEDED        0x00400000
#define DIRDB_FORCELINK         0x00800000
#define DIRDB_PASS0NEEDED       0x01000000
#define DIRDB_MAKEFIL1          0x02000000
#define DIRDB_CHECKED_ALT_DIR   0x04000000

#define DIRDB_IDLTYPERPC        0x08000000

// Flags preserved when loading build.dat:

#define DIRDB_DBPRESERVE        0



typedef struct _TARGET {
    FILEREC *pfrCompiland;
    DIRREC *pdrBuild;
    LPSTR pszSourceDirectory;
    LPSTR ConditionalIncludes;
    DOS_TIME DateTime;
    ULONG DirFlags;
    char Name[1];
} TARGET, *PTARGET;


#define BUILD_VERSION           0x0422
#define DBMASTER_NAME           "build.dat"
#define DB_MAX_PATH_LENGTH      512     // There's some sick people out there using 250 byte paths with 100 byte filenames

// If you change or add any values to this enum,
// also fix MemTab in buildutl.c:

typedef enum _MemType {
    MT_TOTALS = 0,
    MT_UNKNOWN,

    MT_CHILDDATA,
    MT_CMDSTRING,
    MT_DIRDB,
    MT_DIRSUP,
    MT_DIRPATH,
    MT_DIRSTRING,
    MT_EVENTHANDLES,
    MT_FILEDB,
    MT_FILEREADBUF,
    MT_FRBSTRING,
    MT_INCLUDEDB,
    MT_IOBUFFER,
    MT_MACRO,
    MT_SOURCEDB,
    MT_TARGET,
    MT_THREADFILTER,
    MT_THREADHANDLES,
    MT_THREADSTATE,

    MT_INVALID = 255,
} MemType;

struct _THREADSTATE;

typedef BOOL (*FILTERPROC)(struct _THREADSTATE *ThreadState, LPSTR p);

typedef struct _THREADSTATE {
    USHORT cRowTotal;
    USHORT cColTotal;
    FILE *ChildOutput;
    UINT ChildState;
    UINT ChildFlags;
    LPSTR ChildTarget;
    UINT LinesToIgnore;
    FILTERPROC FilterProc;
    ULONG ThreadIndex;
    CHAR UndefinedId[ DB_MAX_PATH_LENGTH ];
    CHAR ChildCurrentDirectory[ DB_MAX_PATH_LENGTH ];
    CHAR ChildCurrentFile[ DB_MAX_PATH_LENGTH ];
    DIRREC *CompileDirDB;
} THREADSTATE, *PTHREADSTATE;

//
// Global Data (uninit will always be FALSE)
//

BOOL fUsage;                     // Set when usage message is to be displayed
BOOL fStatus;                    // Set by -s and -S options
BOOL fStatusTree;                // Set by -S option
BOOL fShowTree;                  // Set by -t and -T options
BOOL fShowTreeIncludes;          // Set by -T option
BOOL fClean;                     // Set by -c option
BOOL fCleanLibs;                 // Set by -C option
BOOL fCleanRestart;              // Set by -r option
BOOL fRestartClean;              // Set if -c and -r were both given
BOOL fRestartCleanLibs;          // Set if -C and -r were both given
BOOL fParallel;                  // Set on a multiprocessor machine or by -M
BOOL fQuery;                     // Set by -q option
BOOL fStopAfterPassZero;         // Set by -0 option
BOOL fQuicky;                    // Set by -z and -Z options
BOOL fQuickZero;                 // Set by -3
BOOL fSemiQuicky;                // Set by -Z option
BOOL fShowOutOfDateFiles;        // Set by -o option
BOOL fSyncLink;                  // Set by -a option
BOOL fForce;                     // Set by -f option
BOOL fEnableVersionCheck;        // Set by -v option
BOOL fSilentDependencies;        // Set by -i option
BOOL fKeep;                      // Set by -k option
BOOL fCompileOnly;               // Set by -L option
BOOL fLinkOnly;                  // Set by -l option
BOOL fErrorLog;                  // Set by -e option
BOOL fGenerateObjectsDotMacOnly; // Set by -O option
BOOL fShowWarningsOnScreen;      // Set by -w option
BOOL fNoisyScan;                 // Set by -y option
BOOL fFullErrors;                // Set by -b option
BOOL fWhyBuild;                  // Set by -why option
BOOL fChicagoProduct;            // Set if CHICAGO_PRODUCT is set in environment
extern BOOL fLineCleared;               // Current line on screen clear?
BOOL fPassZero;                  // Indicates we've found pass zero dirs
BOOL fFirstScan;                 // Indicates this is the first scan
BOOL fAlwaysPrintFullPath;       // Set by -F option
BOOL fTargetDirs;                // Set by -g option
BOOL fAlwaysKeepLogfile;         // Set by -E option
BOOL fShowUnusedDirs;            // Set by -u option
BOOL fCheckIncludePaths;         // Set by -# option
BOOL fErrorBaseline;             // Set by -B option
BOOL fNoThreadIndex;             // Set by -I option
BOOL fBuildAltDirSet;            // set when BUILD_ALT_DIR is defined.
BOOL fPrintElapsed;              // Set by -P option
BOOL fErrorCodeOnWarning;        // set by the -wx option
#define MAX_INCLUDE_PATTERNS 32

LPSTR AcceptableIncludePatternList[ MAX_INCLUDE_PATTERNS + 1 ];
LPSTR UnacceptableIncludePatternList[ MAX_INCLUDE_PATTERNS + 1 ];

LPSTR MakeProgram;
char MakeParameters[ 512 ];
LPSTR MakeParametersTail;
char MakeTargets[ 256 ];
char RestartDir[ 256 ];
char NtRoot[ 256 ];
char DbMasterName[ 256 ];
extern const char szNewLine[];

char BaselinePathName[DB_MAX_PATH_LENGTH];    // The file name for -B
BOOL bBaselineFailure;              // Indicates if there is a build failure that is not in the baseline file
DWORD dwLastBaselineSeekPos;        // Keeps track on the passed baseline failures

char *pszSdkLibDest;
char *pszDdkLibDest;
char *pszPublicInternalPath;
char *pszIncOak;
char *pszIncDdk;
char *pszIncWdm;
char *pszIncSdk;
char *pszIncMfc;
char *pszIncCrt;
char *pszIncPri;
char *pszIncOs2;
char *pszIncPosix;
char *pszIncChicago;

char *szBuildTag;
extern char *pszObjDir;
extern char *pszObjDirSlash;
extern char *pszObjDirSlashStar;
extern BOOL fCheckedBuild;
extern ULONG iObjectDir;
extern ULONG NumberProcesses;
CRITICAL_SECTION TTYCriticalSection;

CHAR const *cmdexe;

LONG TotalFilesToCompile;
LONG TotalFilesCompiled;

LONG TotalLinesToCompile;
LONG TotalLinesCompiled;

ULONG ElapsedCompileTime;
DIRREC *CurrentCompileDirDB;

// Fixed length arrays...

UINT CountTargetMachines;
TARGET_MACHINE_INFO *TargetMachines[MAX_TARGET_MACHINES];
extern TARGET_MACHINE_INFO *PossibleTargetMachines[MAX_TARGET_MACHINES];
extern TARGET_MACHINE_INFO i386TargetMachine;
extern TARGET_MACHINE_INFO ia64TargetMachine;
extern TARGET_MACHINE_INFO DynamicTargetMachine;
UINT TargetToPossibleTarget[MAX_TARGET_MACHINES];


#define MAX_OPTIONAL_DIRECTORIES        256
UINT CountOptionalDirs;
LPSTR OptionalDirs[MAX_OPTIONAL_DIRECTORIES];
BOOLEAN OptionalDirsUsed[MAX_OPTIONAL_DIRECTORIES];
BOOL BuildAllOptionalDirs;


#define MAX_EXCLUDE_DIRECTORIES         MAX_OPTIONAL_DIRECTORIES
UINT CountExcludeDirs;
LPSTR ExcludeDirs[MAX_EXCLUDE_DIRECTORIES];
BOOLEAN ExcludeDirsUsed[MAX_OPTIONAL_DIRECTORIES];


#define MAX_EXCLUDE_INCS                128
UINT CountExcludeIncs;
LPSTR ExcludeIncs[MAX_EXCLUDE_INCS];


#define MAX_INCLUDE_DIRECTORIES         128
UINT CountIncludeDirs;
UINT CountSystemIncludeDirs;
DIRREC *IncludeDirs[MAX_INCLUDE_DIRECTORIES];



#define MAX_BUILD_DIRECTORIES           8192

UINT CountPassZeroDirs;
DIRREC *PassZeroDirs[MAX_BUILD_DIRECTORIES];

UINT CountCompileDirs;
DIRREC *CompileDirs[MAX_BUILD_DIRECTORIES];

UINT CountLinkDirs;
DIRREC *LinkDirs[MAX_BUILD_DIRECTORIES];

UINT CountShowDirs;
DIRREC *ShowDirs[MAX_BUILD_DIRECTORIES];



DIRREC *AllDirs;
CHAR CurrentDirectory[DB_MAX_PATH_LENGTH];

BOOL AllDirsInitialized;
BOOL AllDirsModified;

USHORT GlobalSequence;
USHORT LocalSequence;

extern BOOLEAN fConsoleInitialized;
DWORD NewConsoleMode;

LPSTR BuildDefault;
LPSTR BuildParameters;

LPSTR SystemIncludeEnv;
LPSTR LocalIncludeEnv;

LPSTR BigBuf;
UINT BigBufSize;

UINT RecurseLevel;

FILE *LogFile;
FILE *WrnFile;
FILE *ErrFile;
FILE *IPGScriptFile;
FILE *IncFile;

EXTERN_C UINT NumberCompileWarnings;
EXTERN_C UINT NumberCompileErrors;
EXTERN_C UINT NumberCompiles;
EXTERN_C UINT NumberLibraries;
EXTERN_C UINT NumberLibraryWarnings;
EXTERN_C UINT NumberLibraryErrors;
EXTERN_C UINT NumberLinks;
EXTERN_C UINT NumberLinkWarnings;
EXTERN_C UINT NumberLinkErrors;
EXTERN_C UINT NumberBinplaces;
EXTERN_C UINT NumberBinplaceWarnings;
EXTERN_C UINT NumberBinplaceErrors;
EXTERN_C UINT NumberBSCMakes;
EXTERN_C UINT NumberBSCWarnings;
EXTERN_C UINT NumberBSCErrors;
EXTERN_C UINT NumberVSToolErrors;
EXTERN_C UINT NumberVSToolWarnings;

extern char szAsterisks[];
DOS_TIME BuildStartTime;

WORD DefaultConsoleAttributes;


#define COLOR_STATUS 0
#define COLOR_SUMMARY 1
#define COLOR_WARNING 2
#define COLOR_ERROR 3


VOID ReportDirsUsage(VOID);

VOID SetObjDir(BOOL fAlternate);

typedef struct _PipeData {
    HANDLE ProcHandle;
    FILE *pstream;
} PIPE_DATA, *PPIPE_DATA;

DWORD PipeDataTlsIndex;


//
// Data Base functions defined in builddb.c
//

PDIRREC
LoadDirDB(LPSTR DirName);

#if DBG
VOID
PrintAllDirs(VOID);
#endif

VOID
PrintSourceDBList(SOURCEREC *psr, int i);

VOID
PrintFileDB(FILE *pf, FILEREC *pfr, int DetailLevel);

VOID
PrintDirDB(DIRREC *pdr, int DetailLevel);

FILEREC *
FindSourceFileDB(DIRREC *pdr, LPSTR pszRelPath, DIRREC **ppdr);

DIRREC *
FindSourceDirDB(
    LPSTR pszDir,               // directory
    LPSTR pszRelPath,           // relative path
    BOOL fTruncateFileName);    // TRUE: drop last component of path

SOURCEREC *
FindSourceDB(
    SOURCEREC *psr,
    FILEREC *pfr);

SOURCEREC *
InsertSourceDB(
    SOURCEREC **ppsrNext,
    FILEREC *pfr,
    UCHAR SubDirMask,
    UCHAR SrcFlags);

VOID
FreeSourceDB(SOURCEREC **ppsr);

VOID
UnsnapIncludeFiles(FILEREC *pfr, BOOL fUnsnapGlobal);

VOID
UnsnapAllDirectories(VOID);

VOID
FreeAllDirs(VOID);

PFILEREC
LookupFileDB(
    PDIRREC DirDB,
    LPSTR FileName);


PFILEREC
InsertFileDB(
    PDIRREC DirDB,
    LPSTR FileName,
    DOS_TIME DateTime,
    USHORT Attr,
    ULONG  FileFlags);

VOID
DeleteUnscannedFiles(PDIRREC DirDB);

PINCLUDEREC
InsertIncludeDB(
    PFILEREC FileDB,
    LPSTR IncludeFileName,
    USHORT IncFlags);

VOID
LinkToCycleRoot(INCLUDEREC *pir, FILEREC *pfrRoot);

VOID
RemoveFromCycleRoot(INCLUDEREC *pir, FILEREC *pfrRoot);

VOID
MergeIncludeFiles(FILEREC *pfr, INCLUDEREC *pirList, FILEREC *pfrRoot);

VOID
MarkIncludeFileRecords(PFILEREC FileDB);

VOID
DeleteIncludeFileRecords(PFILEREC FileDB);

PFILEREC
FindIncludeFileDB(
    FILEREC *pfrSource,
    FILEREC *pfrCompiland,
    DIRREC *pdrBuild,
    LPSTR pszSourceDirectory,
    INCLUDEREC *IncludeDB);

BOOL
SaveMasterDB(VOID);

void
LoadMasterDB(VOID);

PDIRREC
LoadMasterDirDB(LPSTR s);

PFILEREC
LoadMasterFileDB(LPSTR s);

PINCLUDEREC
LoadMasterIncludeDB(LPSTR s);


//
// Scanning functions defined in buildscn.c
//

VOID
AddIncludeDir(DIRREC *pdr, UINT *pui);

VOID
AddShowDir(DIRREC *pdr);

VOID
ScanGlobalIncludeDirectory(LPSTR path);

VOID
ScanIncludeEnv(LPSTR IncludeEnv);

PDIRREC
ScanDirectory(LPSTR DirName);

BOOL
ScanFile(PFILEREC FileDB);


//
// Functions defined in buildmak.c
//

VOID
ScanSourceDirectories(LPSTR DirName);

VOID
CompilePassZeroDirectories(VOID);

VOID
CompileSourceDirectories(VOID);

VOID
LinkSourceDirectories(VOID);


VOID
FreeDirSupData(DIRSUP *pds);

VOID
FreeDirData(DIRREC *pdr);

BOOL
CheckDependencies(
    PTARGET Target,
    FILEREC *FileDB,
    BOOL CheckDate,
    FILEREC **ppFileDBRoot);

BOOL
CreateBuildDirectory(LPSTR Name);

VOID
CreatedBuildFile(LPSTR DirName, LPSTR FileName);

VOID
GenerateObjectsDotMac(DIRREC *DirDB, DIRSUP *pds, DOS_TIME DateTimeSources);

VOID
ExpandObjAsterisk(
    LPSTR pbuf,
    LPSTR pszpath,
    LPSTR *ppszObjectDirectory);

//
// Build -# functions defined in buildinc.c
//

LPCTSTR
FindCountedSequenceInString(
    IN LPCTSTR String,
    IN LPCTSTR Sequence,
    IN DWORD   Length);

BOOL
DoesInstanceMatchPattern(
    IN LPCTSTR Instance,
    IN LPCTSTR Pattern);

LPSTR
CombinePaths(
    IN  LPCSTR ParentPath,
    IN  LPCSTR ChildPath,
    OUT LPSTR  TargetPath);

VOID
CreateRelativePath(
    IN  LPCSTR SourceAbsName,
    IN  LPCSTR TargetAbsName,
    OUT LPSTR  RelativePath);

BOOL
ShouldWarnInclude(
    IN LPCSTR CompilandFullName,
    IN LPCSTR IncludeeFullName);

VOID
CheckIncludeForWarning(
    IN LPCSTR CompilandDir,
    IN LPCSTR CompilandName,
    IN LPCSTR IncluderDir,
    IN LPCSTR IncluderName,
    IN LPCSTR IncludeeDir,
    IN LPCSTR IncludeeName);

//
// Utility functions defined in buildutl.c
//

VOID
AllocMem(UINT cb, VOID **ppv, MemType mt);

VOID
FreeMem(VOID **ppv, MemType mt);

VOID
ReportMemoryUsage(VOID);


BOOL
MyOpenFile(
    LPSTR DirName,
    LPSTR FileName,
    LPSTR Access,
    FILE **Stream,
    BOOL fBufferedIO);

BOOL
OpenFilePush(
    LPSTR pszdir,
    LPSTR pszfile,
    LPSTR pszCommentToEOL,
    FILE **ppf
    );

BOOL
SetupReadFile(LPSTR pszdir, LPSTR pszfile, LPSTR pszCommentToEOL,
              FILE **ppf);

DOS_TIME
CloseReadFile(UINT *pcline);

LPSTR
ReadLine(FILE *pf);

UINT
ProbeFile(
    LPSTR DirName,
    LPSTR FileName);

BOOL
EnsureDirectoriesExist(
    LPSTR DirName);

DOS_TIME
DateTimeFile(
    LPSTR DirName,
    LPSTR FileName);

DOS_TIME
DateTimeFile2(
    LPSTR DirName,
    LPSTR FileName);

DOS_TIME (*pDateTimeFile)(LPSTR, LPSTR);

BOOL (WINAPI * pGetFileAttributesExA)(LPCSTR, GET_FILEEX_INFO_LEVELS, LPVOID);

BOOL
DeleteSingleFile(
    LPSTR DirName,
    LPSTR FileName,
    BOOL QuietFlag);

BOOL
DeleteMultipleFiles(
    LPSTR DirName,
    LPSTR FilePattern);

BOOL
CloseOrDeleteFile(
    FILE **Stream,
    LPSTR DirName,
    LPSTR FileName,
    ULONG SizeThreshold);

LPSTR
PushCurrentDirectory(LPSTR NewCurrentDirectory);

VOID
PopCurrentDirectory(LPSTR OldCurrentDirectory);

UINT
ExecuteProgram(
    LPSTR ProgramName,
    LPSTR CommandLine,
    LPSTR MoreCommandLine,
    BOOL MustBeSynchronous);

VOID
WaitForParallelThreads(VOID);


BOOL
CanonicalizePathName(
    LPSTR SourcePath,
    UINT Action,
    LPSTR FullPath);


#define CANONICALIZE_ONLY 0
#define CANONICALIZE_FILE 1
#define CANONICALIZE_DIR  2

LPSTR
FormatPathName(
    LPSTR DirName,
    LPSTR FileName);

#if DBG
VOID
AssertPathString(LPSTR pszPath);
#else
#define AssertPathString(p)
#endif

LPSTR
AppendString(
    LPSTR Destination,
    LPSTR Source,
    BOOL PrefixWithSpace);

LPSTR CopyString(LPSTR Destination, LPSTR Source, BOOL fPath);
VOID  MakeString(LPSTR *Destination, LPSTR Source, BOOL fPath, MemType mt);
VOID  MakeExpandedString(LPSTR *Destination, LPSTR Source);
VOID  FreeString(LPSTR *Source, MemType mt);
LPSTR FormatNumber(ULONG Number);
LPSTR FormatTime(ULONG Seconds);

BOOL AToX(LPSTR *pp, ULONG *pul);
BOOL AToD(LPSTR *pp, ULONG *pul);
EXTERN_C VOID __cdecl LogMsg(const char *pszfmt, ...);
EXTERN_C VOID __cdecl BuildMsg(const char *pszfmt, ...);
EXTERN_C VOID __cdecl BuildMsgRaw(const char *pszfmt, ...);
EXTERN_C VOID __cdecl BuildError(const char *pszfmt, ...);
EXTERN_C VOID __cdecl BuildErrorRaw(const char *pszfmt, ...);
EXTERN_C VOID __cdecl BuildColorMsg(WORD, const char *pszfmt, ...);
EXTERN_C VOID __cdecl BuildColorMsgRaw(WORD, const char *pszfmt, ...);
EXTERN_C VOID __cdecl BuildColorError(WORD, const char *pszfmt, ...);
EXTERN_C VOID __cdecl BuildColorErrorRaw(WORD, const char *pszfmt, ...);

VOID*
memfind(VOID* pvWhere, DWORD cbWhere, VOID* pvWhat, DWORD cbWhat);
                                                                  
//
// Functions in buildsrc.c
//

VOID
StartElapsedTime(VOID);

VOID
PrintElapsedTime(VOID);

BOOL
ReadDirsFile(DIRREC *DirDB);


VOID
ProcessLinkTargets(PDIRREC DirDB, LPSTR CurrentDirectory);

BOOL
SplitToken(LPSTR pbuf, char chsep, LPSTR *ppstr);

BOOL
MakeMacroString(LPSTR *pp, LPSTR p);

VOID
SaveMacro(LPSTR pszName, LPSTR pszValue);

VOID
FormatLinkTarget(
    LPSTR path,
    LPSTR *ObjectDirectory,
    LPSTR TargetPath,
    LPSTR TargetName,
    LPSTR TargetExt);

BOOL
ReadSourcesFile(DIRREC *DirDB, DIRSUP *pds, DOS_TIME *pDateTimeSources);

VOID
PostProcessSources(DIRREC *pdr, DIRSUP *pds);

VOID
PrintDirSupData(DIRSUP *pds);

//+---------------------------------------------------------------------------
//
//  Function:   IsFullPath
//
//----------------------------------------------------------------------------

static
__inline BOOL
IsFullPath(char *pszfile)
{
    return(IsPathSeparator(pszfile[0])
#if !PLATFORM_UNIX
        || (isalpha(pszfile[0]) && pszfile[1] == ':')
#endif  // !PLATFORM_UNIX
    );
}

#ifdef __cplusplus
}
#endif

#endif // __BUILD_H__
