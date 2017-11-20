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
// File: fileiter.h
//
// Defines various iterators for C# files
// ===========================================================================

#ifndef __fileiter_h__
#define __fileiter_h__

/*
 * Iterators for OUTFILESYMs
 */
class OutFileIteratorBase
{
public:

    virtual OUTFILESYM *Reset(COMPILER * compiler)
    {
        current = compiler->GetFirstOutFile();
        AdvanceToValid();
        return Current();
    }

    virtual OUTFILESYM *Next()
    {
        current = current->nextOutfile();
        AdvanceToValid();
        return Current();
    }

            OUTFILESYM *Current() { return current; }

protected:
    void AdvanceToValid()
    {
        if (current && !IsValid()) {
            Next();
        }
    }

    virtual bool IsValid() = 0;

    OutFileIteratorBase() { current = 0; }

    OUTFILESYM *current;
};

class OutFileIterator : public OutFileIteratorBase
{
protected:
    virtual bool IsValid() { return true; }
};

class SourceOutFileIterator : public OutFileIteratorBase
{
protected:
    bool IsValid()
    {
        return (!current->isResource && current->name && *current->name->text);
    }
};

/*
 * Iterators for INFILESYMs
 */

class IInfileIterator
{
public:
    virtual INFILESYM *Next() = 0;
    virtual INFILESYM *Current() = 0;
};

class InFileIteratorBase : public IInfileIterator
{
public:

    INFILESYM *Reset(OUTFILESYM *outfile)
    {
        current = outfile->firstInfile();
        AdvanceToValid();
        return Current();
    }

    INFILESYM *Next()
    {
        current = current->nextInfile();
        AdvanceToValid();
        return Current();
    }

    INFILESYM *Current()
    {
        return current;
    }

protected:
    void AdvanceToValid()
    {
        if (current && !IsValid()) {
            Next();
        }
    }

    virtual bool IsValid() = 0;

    InFileIteratorBase() { current = 0; }

    INFILESYM *current;
};

class InFileIterator : public InFileIteratorBase
{
protected:
    bool IsValid() { return true; }
};

class ChangedInFileIterator : public InFileIteratorBase
{
protected:
    bool IsValid() { return current->hasChanged; }
};

/*
 * Combined Out/In iterator
 */
class CombinedFileIterator : public IInfileIterator
{
public:

    CombinedFileIterator(OutFileIteratorBase *   outIterator,
                         InFileIteratorBase *    inIterator) :
        outIterator(outIterator),
        inIterator(inIterator)
    {}

    INFILESYM *Reset(COMPILER *compiler)
    {
        outIterator->Reset(compiler);
        inIterator->Reset(outIterator->Current());
        return Current();
    }

    INFILESYM *Next()
    {
        inIterator->Next();
        if (!inIterator->Current()) {
            outIterator->Next();
            if (outIterator->Current()) {
                inIterator->Reset(outIterator->Current());
            }
        }
        return Current();
    }

    INFILESYM *Current()
    {
        return inIterator->Current();
    }

private:

    OutFileIteratorBase *   outIterator;
    InFileIteratorBase *    inIterator;

};

class SourceFileIterator : public CombinedFileIterator
{
public:
    SourceFileIterator() :
        CombinedFileIterator(&outIterator, &inIterator)
    {}

private:
    SourceOutFileIterator outIterator;
    InFileIterator inIterator;
};

class ChangedFileIterator : public CombinedFileIterator
{
public:
    ChangedFileIterator() :
        CombinedFileIterator(&outIterator, &inIterator)
    {}

private:
    SourceOutFileIterator outIterator;
    ChangedInFileIterator inIterator;
};

class AllInFileIterator : public CombinedFileIterator
{
public:
    AllInFileIterator() :
        CombinedFileIterator(&outIterator, &inIterator)
    {}

private:
    OutFileIterator outIterator;
    InFileIterator inIterator;
};

/*
 * Iterators for AGGSYMs
 */
class TypeIteratorBase
{
public:

    AGGSYM *Reset(INFILESYM *infile)
    {
        current = GetFirstOfNamespaceDecl(infile->rootDeclaration);
        AdvanceToValid();
        return Current();
    }

    AGGSYM *Next()
    {
        current = GetNext(current);
        AdvanceToValid();
        return Current();
    }

    AGGSYM *Current()
    {
        return current;
    }

protected:

    static AGGSYM *GetNextSibling(AGGSYM *cls)
    {
        SYM *sibling = cls->nextChild;
        while (sibling) {
            if (sibling->kind == SK_AGGSYM) {
                return sibling->asAGGSYM();
            } else if (sibling->kind == SK_NSDECLSYM) {
                AGGSYM *next = GetFirstOfNamespaceDecl(sibling->asNSDECLSYM());
                if (next) {
                    return next;
                }
            }

            sibling = sibling->nextChild;
        }

        return NULL;
    }

    static AGGSYM *GetNext(AGGSYM *cls)
    {
        if (cls) {
            // children first
            FOREACHCHILD(cls, child)
                if (child->kind == SK_AGGSYM) {
                    return child->asAGGSYM();

                }
            ENDFOREACHCHILD

            // next sibling
            PPARENTSYM parent = cls;
            do 
            {
                AGGSYM *next = GetNextSibling(parent->asAGGSYM());
                if (next) {
                    return next;
                }
                parent = parent->containingDeclaration();
            } while (parent->kind == SK_AGGSYM);

            // parent is containing namespace declaration
            do {
                // check the next child
                while (parent->nextChild) {
                    // skip global attributes
                    SYM *nextSym = parent->nextChild;
                    while (nextSym && nextSym->kind == SK_GLOBALATTRSYM) {
                        nextSym = nextSym->nextChild;
                    }

                    if (nextSym) {
                        parent = nextSym->asPARENTSYM();
                        if (parent->kind == SK_AGGSYM) {
                            return parent->asAGGSYM();
                        }
                        AGGSYM *next = GetFirstOfNamespaceDecl(parent->asNSDECLSYM());
                        if (next) {
                            return next;
                        }
                    } else {
                        break;
                    }
                }

                // check containing nsdecl
                parent = parent->containingDeclaration();
            } while (parent);
        }

        return 0;
    }

    static AGGSYM *GetFirstOfNamespaceDecl(NSDECLSYM *nsdecl)
    {
        if (!nsdecl) return 0;

        FOREACHCHILD(nsdecl, child)
            switch (child->kind)
            {
            case SK_AGGSYM:
                return child->asAGGSYM();
            case SK_NSDECLSYM:
                {
                AGGSYM *first = GetFirstOfNamespaceDecl(child->asNSDECLSYM());
                if (first) {
                    return first;
                }
                }
                break;
            case SK_GLOBALATTRSYM:
                break;
            default:
                ASSERT(!"Bad SK");
            }
        ENDFOREACHCHILD

        return 0;
    }

    void AdvanceToValid()
    {
        if (current && !IsValid()) {
            Next();
        }
    }

    virtual bool IsValid() = 0;

    TypeIteratorBase() { current = 0; }

    AGGSYM *current;
};

class TypeIterator : public TypeIteratorBase
{
protected:
    bool IsValid() { return true; }
};

#endif // __fileiter_h__

