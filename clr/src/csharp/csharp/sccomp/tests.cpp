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
// File: tests.cpp
//
// Contains internal tests for subsystems of the compiler
// ===========================================================================

#include "stdafx.h"
#include "compiler.h"

#ifdef DEBUG

/*
 * Create a random name.
 */
static void RandName(LPWSTR buffer, int length)
{
    for (int i = 0; i < length - 1; ++i) {
        buffer[i] = (rand() % 0xf000) + 33;
    }
    buffer[length - 1] = 0;
}

/*
 * Constructor.
 */
TESTS::TESTS(COMPILER * comp)
{
    compiler = comp;
}


/* 
 * Some code useful for testing emitter constructs that aren't 
 * available yet from source code.
 */
void TESTS::TestEmitter()
{
    HRESULT hr;
    DWORD d;
    PINFILESYM pInfile;
    POUTFILESYM pOutfile;
    PNSSYM ns;
    PNSDECLSYM nsdecl, nsdeclRoot;
    PAGGSYM cls, intf, enm;
    PMETHSYM meth1, meth2, meth3, imeth;
    PMEMBVARSYM fld1;
    PPROPSYM prop;

    pOutfile = compiler->symmgr.CreateOutFile(L"test.dll", true, false, NULL, NULL, NULL, false);

    pInfile = compiler->symmgr.CreateSourceFile(L"test.cs", pOutfile, FILETIME());

    nsdeclRoot = compiler->symmgr.CreateNamespaceDeclaration(compiler->symmgr.GetRootNS(), NULL, pInfile, NULL);
    ns = compiler->symmgr.CreateNamespace(compiler->namemgr->AddString(L"MyNamespace"),
                                          compiler->symmgr.GetRootNS());
    nsdecl = compiler->symmgr.CreateNamespaceDeclaration(ns, nsdeclRoot, pInfile, NULL);

    intf = compiler->symmgr.CreateAggregate(compiler->namemgr->AddString(L"MyInterface"), nsdecl);
    intf->isInterface = true;
    intf->isPrepared = true;
    intf->access = ACC_PUBLIC;

    cls = compiler->symmgr.CreateAggregate(compiler->namemgr->AddString(L"MyClass"), nsdecl);
    cls->isClass = true;
    cls->baseClass = compiler->symmgr.GetPredefType(PT_OBJECT, true);
    cls->isPrepared = true;
    cls->access = ACC_PUBLIC;
    cls->ifaceList = 0;                     // Start with empty interface list.
    PSYMLIST * pLink = & cls->allIfaceList;
    compiler->symmgr.AddToGlobalSymList(intf, & pLink);

    enm = compiler->symmgr.CreateAggregate(compiler->namemgr->AddString(L"MyEnum"), nsdecl);
    enm->isEnum = true;
    enm->isPrepared = true;
    enm->hasResolvedBaseClasses = true;
    enm->baseClass = compiler->symmgr.GetPredefType(PT_ENUM, true);
    enm->underlyingType = compiler->symmgr.GetPredefType(PT_SHORT, true);
    enm->access = ACC_PUBLIC;

    imeth = compiler->symmgr.CreateMethod(compiler->namemgr->AddString(L"IMeth"), intf);
    imeth->retType = compiler->symmgr.GetPredefType(PT_INT);
    imeth->access = ACC_PUBLIC;
    meth1 = compiler->symmgr.CreateMethod(compiler->namemgr->AddString(L"MyProp_get"), cls);
    meth1->retType = compiler->symmgr.GetPredefType(PT_INT);
    meth1->access = ACC_PUBLIC;
    meth2 = compiler->symmgr.CreateMethod(compiler->namemgr->AddString(L"MyProp_set"), cls);
    meth2->retType = compiler->symmgr.GetVoid();
    meth2->access = ACC_PUBLIC;
    meth3 = compiler->symmgr.CreateMethod(NULL, cls);
    meth3->retType = compiler->symmgr.GetPredefType(PT_INT);
    meth3->explicitImpl = imeth;
    meth3->access = ACC_PUBLIC;
    meth3->isMetadataVirtual = true;
    prop = compiler->symmgr.CreateProperty(compiler->namemgr->AddString(L"MyProp"), cls);
    prop->retType = compiler->symmgr.GetPredefType(PT_INT);
    prop->methGet = meth1;
    prop->methSet = meth2;
    prop->access = ACC_PUBLIC;
    fld1 = compiler->symmgr.CreateMembVar(compiler->namemgr->AddString(L"enumerator1"), enm);
    fld1->type = enm;
    fld1->access = ACC_PUBLIC;
    fld1->isConst = fld1->isReadOnly = fld1->isStatic = true;
    fld1->constVal.iVal = 42;

    // Restart the linker with the test stuff
    if (FAILED(hr = compiler->linker->CloseAssembly(AssemblyIsUBM)))
        compiler->Error( NULL, ERR_ALinkFailed, compiler->ErrHR(hr));
    compiler->assemFile.Init(compiler);
    compiler->assemFile.BeginOutputFile(pOutfile);
//    compiler->assem.BeginAssembly();
    if (FAILED(hr = compiler->linker->SetAssemblyFile(pOutfile->name->text, compiler->assemFile.GetEmit(), afNone, &compiler->assemID)))
        compiler->Error( NULL, ERR_ALinkFailed, compiler->ErrHR(hr));
    else
        pOutfile->idFile = compiler->assemID;
    compiler->curFile = &compiler->assemFile;
    compiler->emitter.BeginOutputFile();
    if (FAILED(hr = compiler->linker->EmitManifest(compiler->assemID, &d, NULL)))
        compiler->Error( NULL, ERR_ALinkFailed, compiler->ErrHR(hr));
    else
        ASSERT(d == 0);

    AGGINFO  agginfo;
    memset(&agginfo, 0, sizeof(AGGINFO));
    compiler->emitter.EmitAggregateDef(intf);
    compiler->emitter.EmitAggregateDef(cls);
    compiler->emitter.EmitAggregateDef(enm);
    compiler->emitter.EmitAggregateSpecialFields(enm);

    METHINFO methinfo;
    memset(&methinfo, 0, sizeof(METHINFO));
    compiler->emitter.EmitMethodDef(imeth);
    compiler->emitter.EmitMethodInfo(imeth, &methinfo);

    compiler->emitter.EmitMethodDef(meth1);
    compiler->emitter.EmitMethodInfo(meth1, &methinfo);
    compiler->emitter.EmitMethodDef(meth2);
    compiler->emitter.EmitMethodInfo(meth2, &methinfo);
    compiler->emitter.EmitMethodDef(meth3);
    compiler->emitter.EmitMethodInfo(meth3, &methinfo);

    PROPINFO propinfo;
    memset(&propinfo, 0, sizeof(PROPINFO));
    compiler->emitter.EmitPropertyDef(prop);

    MEMBVARINFO membvarinfo;
    memset(&membvarinfo, 0, sizeof(MEMBVARINFO));
    compiler->emitter.EmitMembVarDef(fld1);

    compiler->linker->PreCloseAssembly(compiler->assemID);
    compiler->emitter.EndOutputFile(true);
//    compiler->assem.EndAssembly();
    compiler->assemFile.EndOutputFile(true);
    compiler->curFile = NULL;
    compiler->assemFile.Term();
    compiler->linker->CloseAssembly(compiler->assemID);
}

/*
 * Run tests on the MEMALLOC
 */
void TESTS::TestMemAlloc()
{
    const int NITER = 10000;
    BYTE * a[NITER];
    int s[NITER];
    int i, j;

    for (i = 0; i < NITER; ++i) {
        s[i] = rand() % 4000;
        if (i % 2) {
            a[i] = (BYTE *) compiler->globalHeap.Alloc(s[i]);
        }
        else {
            a[i] = (BYTE *) compiler->globalHeap.AllocZero(s[i]);
            for (j = 0; j < s[i]; ++j) {
                ASSERT((0xFF & a[i][j]) == 0);
            }
        }
        ASSERT(a[i] != NULL);
        memset(a[i], i, s[i]);
        if (i % 3 == 0) {
            compiler->globalHeap.Free(a[i/3]);
            a[i/3] = NULL;
        }
    }

    for (i = 0; i < NITER; ++i) {
        if (a[i] != NULL) {
            for (j = 0; j < s[i]; ++j) {
                ASSERT((0xFF & a[i][j]) == (0xFF & i));
            }
            compiler->globalHeap.Free(a[i]);
        }
    }

    for (i = 0; i < NITER; ++i) {
        s[i] = rand() % 4000;
        if (i % 2) {
            a[i] = (BYTE *) compiler->globalHeap.Alloc(s[i]);
        }
        else {
            a[i] = (BYTE *) compiler->globalHeap.AllocZero(s[i]);
            for (j = 0; j < s[i]; ++j) {
                ASSERT((0xFF & a[i][j]) == 0);
            }
        }
        ASSERT(a[i] != NULL);
        memset(a[i], i, s[i]);
        if (i % 3 == 0) {
            int oldsize = s[i/3];
            bool zero = (rand() % 2) != 0;
            s[i/3] = rand() % 4000;
            if (zero)
                a[i/3] = (BYTE *) compiler->globalHeap.ReallocZero(a[i/3], s[i/3]);
            else
                a[i/3] = (BYTE *) compiler->globalHeap.Realloc(a[i/3], s[i/3]);
            for (j = 0; j < min(oldsize, s[i/3]); ++j) {
                ASSERT((0xFF & a[i/3][j]) == (0xFF & (i/3)));
            }
            for (j = oldsize; j < s[i/3]; ++j) {
                if (zero) 
                    ASSERT((0xFF & a[i/3][j]) == 0);
                a[i/3][j] = (0xFF & (i/3));
            }
        }
    }

    for (i = 0; i < NITER; ++i) {
        if (a[i] != NULL) {
            for (j = 0; j < s[i]; ++j) {
                ASSERT((0xFF & a[i][j]) == (0xFF & i));
            }
            compiler->globalHeap.Free(a[i]);
        }
    }


    WCHAR buffer[10];
    WCHAR * str;
    for (i = 0; i < NITER; ++i) {
        RandName(buffer, lengthof(buffer));
        str = compiler->globalHeap.AllocStr(buffer);
        ASSERT(wcscmp(buffer, str) == 0);
        a[i] = (BYTE *) str;
    }

    for (i = 0; i < NITER; ++i) {
        compiler->globalHeap.Free(a[i]);
    }

    ASSERT(compiler->globalHeap.AllocStr(NULL) == NULL);
}

/*
 * Run tests on the PAGEHEAP
 */
void TESTS::TestPageHeap()
{
    const int NITER = 5000;
    BYTE * a[NITER];
    int s[NITER];
    int i, j;

    // Make sure I can allocate large blocks.
    void * big = compiler->pageheap.AllocPages(0x1000000);
    compiler->pageheap.FreePages(big, 0x1000000);

    for (i = 0; i < NITER; ++i) {
        int np;
        if (rand() % 6 < 4)
            np = 1;
        else if (rand() % 6 < 4)
            np = rand() % 4 + 1;
        else if (rand() % 6 < 4)
            np = rand() % 10 + 1;
        else
            np = rand() % 123 + 1;
        s[i] = np * (int)PAGEHEAP::pageSize;

        a[i] = (BYTE *) compiler->pageheap.AllocPages(s[i]);

        ASSERT(a[i] != NULL);
        memset(a[i], i, s[i]);
        if (i % 3 == 0) {
            compiler->pageheap.FreePages(a[i/3], s[i/3]);
            a[i/3] = NULL;
        }
    }

    for (i = 0; i < NITER; ++i) {
        if (a[i] != NULL) {
            for (j = 0; j < s[i]; ++j) {
                ASSERT((0xFF & a[i][j]) == (0xFF & i));
            }
            compiler->pageheap.FreePages(a[i], s[i]);
        }
    }

}


/* Run tests on the no-release allocator
 */

void TESTS::TestNRHeap()
{
    NRHEAP heap(compiler);

    const int NITER = 5000;
    BYTE * a[NITER];
    int s[NITER];
    int i, j;
    NRMARK mark[NITER / 20];


    for (i = 0; i < NITER; ++i) {
        if (i % 20 == 0)
            heap.Mark(& mark[i / 20]);

        if (rand() %20 == 0)
            s[i] = (rand() % 17000 + 1);
        else
            s[i] = (rand() % 31 + 1);

        a[i] = (BYTE *) heap.Alloc(s[i]);

        ASSERT(a[i] != NULL);
        memset(a[i], i, s[i]);
    }

    for (i = 0; i < NITER; ++i) {
        for (j = 0; j < s[i]; ++j) {
            ASSERT((0xFF & a[i][j]) == (0xFF & i));
        }
    }

    for (int iMark = NITER / 20 - 2; iMark >= 0; iMark -= 2) {
        heap.Free(&mark[iMark]);

        for (i = 0; i < 10; ++i) {
            int sz = rand() % 5000;
            BYTE * ptr = (BYTE *) heap.AllocZero(sz);
            for (j = 0; j < sz; ++j) {
                ASSERT(ptr[j] == 0);
            }
        }

        for (i = 0; i < iMark * 20; ++i) {
            for (j = 0; j < s[i]; ++j) {
                ASSERT((0xFF & a[i][j]) == (0xFF & i));
            }
        }
    }

    WCHAR buffer[10];
    WCHAR * str;
    for (i = 0; i < NITER; ++i) {
        RandName(buffer, lengthof(buffer));
        str = heap.AllocStr(buffer);
        ASSERT(wcscmp(buffer, str) == 0);
        a[i] = (BYTE *) str;
    }

    ASSERT(heap.AllocStr(NULL) == NULL);

    heap.FreeHeap();
}

/*
 * Test the allocators.
 */
void TESTS::TestAlloc()
{
    TestPageHeap();
    TestNRHeap();
    TestMemAlloc();
}


/*
 * Test the name manager component.
 */
void TESTS::TestNamemgr()
{
    const int NITER = 100000;
    int i, iKwd;
    static LPWSTR globals[NITER];
    static PNAME globalNames[NITER];
    NRMARK markLocal, markGlobal;

    compiler->globalSymAlloc.Mark(&markGlobal);
    compiler->localSymAlloc.Mark(&markLocal);

    // Add a bunch of strings to the global table.
    for (i = 0; i < NITER; ++i) {
        int cch = rand() % 8 + 2;
        globals[i] = (LPWSTR) compiler->globalSymAlloc.Alloc(cch * sizeof(WCHAR));
        RandName(globals[i], cch);

        PNAME pname = compiler->namemgr->LookupString(globals[i]);
        globalNames[i] = compiler->namemgr->AddString(globals[i]);
        ASSERT(globalNames[i]);
        ASSERT(! compiler->namemgr->IsKeyword(globalNames[i], &iKwd));
        ASSERT(pname == NULL || pname == globalNames[i]);
    }

    // Make sure all the strings can be looked up.
    for (i = 0; i < NITER; ++i) {
        ASSERT(globalNames[i] == compiler->namemgr->LookupString(globals[i]));
    }

    // Check some keywords.
    PNAME k;
    k = compiler->namemgr->LookupString(L"static");
    ASSERT(compiler->namemgr->IsKeyword(k, &iKwd));
    ASSERT(iKwd == TID_STATIC);
    k = compiler->namemgr->LookupString(L"int");
    ASSERT(compiler->namemgr->IsKeyword(k, &iKwd));
    ASSERT(iKwd == TID_INT);
    ASSERT(compiler->namemgr->KeywordName(TID_PUBLIC) == compiler->namemgr->LookupString(L"public"));

    // Check alternate forms.
    PNAME nam;
    bool dummy;
    nam = compiler->namemgr->AddString(L"foo");
    ASSERT(nam == compiler->namemgr->AddString(L"foobar", 3));
    ASSERT(nam == compiler->namemgr->AddString(L"foobar", 3, compiler->namemgr->HashString(L"foobar", 3), &dummy));
    ASSERT(nam == compiler->namemgr->LookupString(L"foo"));
    ASSERT(nam == compiler->namemgr->LookupString(L"foobar", 3));
    ASSERT(nam == compiler->namemgr->LookupString(L"foobar", 3, compiler->namemgr->HashString(L"foobar", 3)));
    nam = compiler->namemgr->AddString(L"foob");
    ASSERT(nam == compiler->namemgr->AddString(L"foobar", 4));
    ASSERT(nam == compiler->namemgr->AddString(L"foobar", 4, compiler->namemgr->HashString(L"foobar", 4), &dummy));
    ASSERT(nam == compiler->namemgr->LookupString(L"foob"));
    ASSERT(nam == compiler->namemgr->LookupString(L"foobar", 4));
    ASSERT(nam == compiler->namemgr->LookupString(L"foobar", 4, compiler->namemgr->HashString(L"foobar", 4)));

    compiler->localSymAlloc.Free(&markLocal);
    compiler->globalSymAlloc.Free(&markGlobal);
}

/*
 * Test the symbol manager
 */
void TESTS::TestSymmgr()
{
    NRMARK markGlobal, markLocal;
    PNSSYM ns1, ns2, ns3;
    PAGGSYM a11, a12, a21, a22, a31, a32, a33;
    PAGGSYM b11, b12, b21, b31, x11, x31;
    PNSSYM na11, nb21, nb22, nc11;
    PSCOPESYM s1, s2;
    PLOCVARSYM la11, la12, la21, lb21;

    // need to call this before the mark so that the infile lives after the test
    compiler->symmgr.GetPredefInfile();

    compiler->globalSymAlloc.Mark(&markGlobal);
    compiler->localSymAlloc.Mark(&markLocal);

    // Create a bunch of global symbols.

    ns1 = compiler->symmgr.CreateGlobalSym(SK_NSSYM, compiler->namemgr->AddString(L"NS1"), NULL)->asNSSYM();
    ns2 = compiler->symmgr.CreateGlobalSym(SK_NSSYM, compiler->namemgr->AddString(L"NS2"), NULL)->asNSSYM();
    ns3 = compiler->symmgr.CreateGlobalSym(SK_NSSYM, compiler->namemgr->AddString(L"NS3"), NULL)->asNSSYM();

    a21  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, compiler->namemgr->AddString(L"A"), ns2)->asAGGSYM();
    nb21 = compiler->symmgr.CreateGlobalSym(SK_NSSYM,  compiler->namemgr->AddString(L"B"), ns2)->asNSSYM();
    a11  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, compiler->namemgr->AddString(L"A"),  ns1)->asAGGSYM();
    na11 = compiler->symmgr.CreateGlobalSym(SK_NSSYM,  compiler->namemgr->AddString(L"A"), ns1)->asNSSYM();
    b11  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, compiler->namemgr->AddString(L"B"), ns1)->asAGGSYM();
    x31  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, NULL                                   , ns3)->asAGGSYM();
    a22  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, compiler->namemgr->AddString(L"A"), ns2)->asAGGSYM();
    b12  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, compiler->namemgr->AddString(L"B"),  ns1)->asAGGSYM();
    a31  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, compiler->namemgr->AddString(L"A"), ns3)->asAGGSYM();
    nc11 = compiler->symmgr.CreateGlobalSym(SK_NSSYM,  compiler->namemgr->AddString(L"C"),  ns1)->asNSSYM();
    b21  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, compiler->namemgr->AddString(L"B"), ns2)->asAGGSYM();
    nb22 = compiler->symmgr.CreateGlobalSym(SK_NSSYM,  compiler->namemgr->AddString(L"B"),  ns2)->asNSSYM();
    a32  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, compiler->namemgr->AddString(L"A"), ns3)->asAGGSYM();
    a12  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, compiler->namemgr->AddString(L"A"),  ns1)->asAGGSYM();
    x11  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, NULL                                   , ns1)->asAGGSYM();
    a33  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, compiler->namemgr->AddString(L"A"),  ns3)->asAGGSYM();
    b31  = compiler->symmgr.CreateGlobalSym(SK_AGGSYM, compiler->namemgr->AddString(L"B"), ns3)->asAGGSYM();

    // Make sure child symbols are all there in the right order.

    ASSERT(ns1->firstChild == a11);
    ASSERT(a11->nextChild == na11);
    ASSERT(na11->nextChild == b11);
    ASSERT(b11->nextChild == b12);
    ASSERT(b12->nextChild == nc11);
    ASSERT(nc11->nextChild == a12);
    ASSERT(a12->nextChild == x11);
    ASSERT(x11->nextChild == NULL);

    ASSERT(ns2->firstChild == a21);
    ASSERT(a21->nextChild == nb21);
    ASSERT(nb21->nextChild == a22);
    ASSERT(a22->nextChild == b21);
    ASSERT(b21->nextChild == nb22);
    ASSERT(nb22->nextChild == NULL);

    ASSERT(ns3->firstChild == x31);
    ASSERT(x31->nextChild == a31);
    ASSERT(a31->nextChild == a32);
    ASSERT(a32->nextChild == a33);
    ASSERT(a33->nextChild == b31);
    ASSERT(b31->nextChild == NULL);

    // Make sure that child symbols can be looked up successfully.
    ASSERT(compiler->symmgr.LookupGlobalSym(compiler->namemgr->AddString(L"A") , ns1, MASK_AGGSYM) == a11);
    ASSERT(compiler->symmgr.LookupGlobalSym(compiler->namemgr->AddString(L"A"), ns1, MASK_NSSYM) == na11);
    ASSERT(compiler->symmgr.LookupGlobalSym(compiler->namemgr->AddString(L"A"), ns1, MASK_ALL) == a11);
    ASSERT(compiler->symmgr.LookupNextSym(a11, ns1, MASK_ALL) == na11);
    ASSERT(compiler->symmgr.LookupNextSym(a11, ns1, MASK_AGGSYM) == a12);
    ASSERT(compiler->symmgr.LookupNextSym(na11, ns1, MASK_ALL) == a12);
    ASSERT(compiler->symmgr.LookupNextSym(a12, ns1, MASK_ALL) == NULL);
    ASSERT(compiler->symmgr.LookupGlobalSym(compiler->namemgr->AddString(L"B") , ns2, MASK_AGGSYM) == b21);
    ASSERT(compiler->symmgr.LookupGlobalSym(compiler->namemgr->AddString(L"C"), ns2, MASK_ALL) == NULL);
    ASSERT(compiler->symmgr.LookupGlobalSym(compiler->namemgr->AddString(L"A"),  ns2, MASK_AGGSYM) == a21);
    ASSERT(compiler->symmgr.LookupGlobalSym(compiler->namemgr->AddString(L"A"),  ns3, MASK_AGGSYM) == a31);
    ASSERT(compiler->symmgr.LookupNextSym(a31, ns3, MASK_AGGSYM) == a32);
    ASSERT(compiler->symmgr.LookupNextSym(a32, ns3, MASK_AGGSYM) == a33);
    ASSERT(compiler->symmgr.LookupNextSym(a33, ns3, MASK_AGGSYM) == NULL);
    ASSERT(compiler->symmgr.LookupGlobalSym(compiler->namemgr->AddString(L"A"),  ns3, MASK_NSSYM) == NULL);
    ASSERT(compiler->symmgr.LookupGlobalSym(compiler->namemgr->AddString(L"D"),  ns3, MASK_ALL) == NULL);

    // try some local symbols.
    s1 = compiler->symmgr.CreateLocalSym(SK_SCOPESYM, NULL, NULL)->asSCOPESYM();
    s2 = compiler->symmgr.CreateLocalSym(SK_SCOPESYM, compiler->namemgr->AddString(L"SCOPE"), NULL)->asSCOPESYM();

    la11  = compiler->symmgr.CreateLocalSym(SK_LOCVARSYM, compiler->namemgr->AddString(L"A"), s1)->asLOCVARSYM();
    la12  = compiler->symmgr.CreateLocalSym(SK_LOCVARSYM, compiler->namemgr->AddString(L"A"), s1)->asLOCVARSYM();
    la21  = compiler->symmgr.CreateLocalSym(SK_LOCVARSYM, compiler->namemgr->AddString(L"A"),  s2)->asLOCVARSYM();
    lb21  = compiler->symmgr.CreateLocalSym(SK_LOCVARSYM, compiler->namemgr->AddString(L"B"), s2)->asLOCVARSYM();

    ASSERT(s1->firstChild == la11);
    ASSERT(la11->nextChild == la12);
    ASSERT(la12->nextChild == NULL);
    ASSERT(s2->firstChild == la21);
    ASSERT(la21->nextChild == lb21);
    ASSERT(lb21->nextChild == NULL);

    ASSERT(compiler->symmgr.LookupLocalSym(compiler->namemgr->AddString(L"A") , s1, MASK_LOCVARSYM) == la11);
    ASSERT(compiler->symmgr.LookupLocalSym(compiler->namemgr->AddString(L"A"), s1, MASK_LOCVARSYM) == la11);
    ASSERT(compiler->symmgr.LookupNextSym(la11, s1, MASK_LOCVARSYM) == la12);
    ASSERT(compiler->symmgr.LookupNextSym(la12, s1, MASK_LOCVARSYM) == NULL);
    ASSERT(compiler->symmgr.LookupLocalSym(compiler->namemgr->AddString(L"B") , s2, MASK_ALL) == lb21);
    ASSERT(compiler->symmgr.LookupLocalSym(compiler->namemgr->AddString(L"C"), s2, MASK_ALL) == NULL);
    ASSERT(compiler->symmgr.LookupLocalSym(compiler->namemgr->AddString(L"E"), s2, MASK_ALL) == NULL);

    // Stress test -- try many symbols.
    const int NITER = 100000;
    static LPWSTR strings[NITER];
    static PNAME names[NITER];
    static PSYM syms[NITER];
    PSYM s;
    int i;

    // Add a bunch of symbols.
    for (i = 0; i < NITER; ++i) {
        int cch = rand() % 8 + 2;
        strings[i] = (LPWSTR) compiler->globalSymAlloc.Alloc(cch * sizeof(WCHAR));
        RandName(strings[i], cch);

        names[i] = compiler->namemgr->AddString(strings[i]);
        ASSERT(names[i]);
        syms[i] = compiler->symmgr.CreateLocalSym(SK_LOCVARSYM, names[i], (i % 2) ? s1 : s2);
    }

    // Make sure all the symbols can be looked up.
    for (i = 0; i < NITER; ++i) {
        s = compiler->symmgr.LookupLocalSym(names[i], (i % 2) ? s1 : s2, MASK_LOCVARSYM);
        bool found = false;
        do {
            if (s == syms[i])
                found = true;  // OK - found it.
            s = compiler->symmgr.LookupNextSym(s, (i % 2) ? s1 : s2, MASK_LOCVARSYM);
        } while (s);

        ASSERT(found);
    }

    // Test the typearray code.
    PTYPESYM t[5];
    static BYTE sizes[NITER];
    static PTYPESYM * typearrays[NITER];

    t[0] = compiler->symmgr.CreateAggregate(compiler->namemgr->AddString(L"agg1"), compiler->symmgr.GetPredefInfile()->rootDeclaration);
    t[1] = compiler->symmgr.CreateAggregate(compiler->namemgr->AddString(L"agg2"), compiler->symmgr.GetPredefInfile()->rootDeclaration);
    t[2] = compiler->symmgr.CreateAggregate(compiler->namemgr->AddString(L"agg3"), compiler->symmgr.GetPredefInfile()->rootDeclaration);
    t[3] = compiler->symmgr.CreateAggregate(compiler->namemgr->AddString(L"agg4"), compiler->symmgr.GetPredefInfile()->rootDeclaration);
    t[4] = compiler->symmgr.CreateAggregate(compiler->namemgr->AddString(L"agg5"), compiler->symmgr.GetPredefInfile()->rootDeclaration);

    // Add a bunch of type arrays.
    for (i = 0; i < NITER; ++i) {
        PTYPESYM a[9];
        int cType = rand() % 9;
        for (int j = 0; j < cType; ++j) 
            a[j] = t[rand() % 5];

        typearrays[i] = compiler->symmgr.AllocParams(cType, a);
        sizes[i] = cType;
    }

    // Add again and make sure we get the same ones..
    for (i = 0; i < NITER; ++i) {
        ASSERT(typearrays[i] == compiler->symmgr.AllocParams(sizes[i], typearrays[i]));
    }

    compiler->localSymAlloc.Free(&markLocal);
    compiler->globalSymAlloc.Free(&markGlobal);
}

/*
 * Test error reporting.
 */
void TESTS::TestErrors()
{
    //FILELOC loc;
    PNSSYM ns1, ns2, ns3;

    compiler->options.m_fWARNINGSAREERRORS = true;
    compiler->options.warnLevel = 2;

    compiler->Error(NULL, ERR_NoImplicitConv, L"Hello, world", 123);
    compiler->Error(NULL, ERR_NoImplicitConv, compiler->ErrHR(E_HANDLE, false), 123);
    compiler->Error(NULL, ERR_AmbigBinaryOps, compiler->ErrHR(E_HANDLE), compiler->ErrHR(E_POINTER, false), compiler->ErrHR(0x81234567, false));

    ns1 = compiler->symmgr.CreateNamespace(compiler->namemgr->AddString(L"foo"),
                                           compiler->symmgr.GetRootNS());
    ns2 = compiler->symmgr.CreateNamespace(compiler->namemgr->AddString(L"bar"),
                                           ns1);
    ns3 = compiler->symmgr.CreateNamespace(compiler->namemgr->AddString(L"baz"),
                                           ns2);
    compiler->Error(NULL, ERR_AmbigBinaryOps, compiler->ErrSym(ns1), compiler->ErrSym(ns2), compiler->ErrSym(ns3));


    //compiler->SetCurrentFile((LPWSTR)NULL);
    compiler->Error(NULL, ERR_NoImplicitConv, L"Should not have file/line", 123);

    //compiler->SetCurrentFile(L"anotherfile.sc");
    compiler->Error(NULL, ERR_NoImplicitConv, L"Should have file only", 123);

    //loc.line = 126785; loc.col = 134; loc.extent = 5;
    //compiler->Error(&loc, ERR_ClassNameTooLong, L"Should have file and line", 123);

    /* This code will test fatal errors if you want.
    int * p = 0;
    *p = 1;  // Should cause fatal internal compiler error.

    compiler->Error(&loc, ERR_ClassNameTooLong, L"Shouldn't get here", 123);
    */
}

/*
 *  Run all tests
 */
void TESTS::AllTests()
{
    TestAlloc();
    TestNamemgr();
    TestSymmgr();
}

#endif //DEBUG
