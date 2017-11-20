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
// File: hacks.h
//
// ===========================================================================

#if !defined(HACKBOOLOPT)
#error Must define HACKBOOLOPT macro before including hacks.h
#endif
#if !defined(HACKSTROPT)
#error Must define HACKBOOLOPT macro before including hacks.h
#endif

            // bool variable name           Option text
HACKBOOLOPT( runInternalTests,              L"internal")        // test code only: Run the internal tests?
HACKBOOLOPT( dumpParseTrees,                L"dumpparse")       // test code only: Dump parse trees?'
HACKBOOLOPT( dumpSymbols,                   L"dumpsymbols")     // test code only: dump the symbols in the symbol table?
HACKBOOLOPT( fullyImportAll,                L"fullimport")      // test code only: fully import all classes in imported metadata?
HACKBOOLOPT( testEmitter,                   L"emitter")         // test code only: run emitter test
HACKBOOLOPT( logIncrementalBuild,           L"logIncrementalBuild") // test code only: logs incremental build progress
HACKBOOLOPT( debugNoPath,                   L"debugnopath")     // Omit path names from debug info.
HACKBOOLOPT( trackMem,                      L"trackmem")        // track and report memory usage stats
HACKBOOLOPT( encUpdate,                     L"encUpdate")       // Call SetOption for metadata emit for fake ENC
HACKBOOLOPT( dumpEnc,                       L"dumpEnc")         // writes the PE File as a result of an EnC operation
HACKBOOLOPT( showAllErrors,                 L"showAllErrors")   // dump all compiler errors to console.
HACKBOOLOPT( m_fTIMING,                     L"timing")          // show mini-profile of timing info

           // string variable name          Option text
HACKSTROPT( moduleName,                     L"moduleName")      // module name (SetModuleProps)

#undef HACKBOOLOPT
#undef HACKSTROPT
