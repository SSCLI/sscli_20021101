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
// File: tests.h
//
// Internal tests for the compiler.
// ===========================================================================

#ifdef DEBUG

class COMPILER;

class TESTS {
public:
    TESTS(COMPILER * comp);

    void TestEmitter();
    void TestMemAlloc();
    void TestPageHeap();
    void TestNRHeap();
    void TestAlloc();
    void TestErrors();
    void TestNamemgr();
    void TestSymmgr();
    void AllTests();

private:
    COMPILER * compiler;
};


#endif //DEBUG
