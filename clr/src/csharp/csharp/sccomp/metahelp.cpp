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
// File: metahelp.cpp
//
// Helper routines for importing/emitting CLR metadata.
// ===========================================================================

#include "stdafx.h"

#include "compiler.h"


/* 
 * Given an aggregate or namespace symbol, get fully qualified type name as used in metadata.
 *
 * Returns true if enough space is provided, false otherwise. (No error is reported.)
 */
bool METADATAHELPER::GetFullMetadataName(PSYM sym, 
                                         WCHAR * typeName, int cchTypeName,
                                         WCHAR chNestedClassSep, WCHAR chDotReplacement)
{
    PPARENTSYM parent;
    int cch;
    NAME * name;
    const WCHAR * text;  // text of name.
    WCHAR buffer[MAX_IDENT_SIZE];

    // Special case -- the root namespace.
    if (!sym->parent)
    {
        // At the root namespace.
        if (cchTypeName >= 1) {
            *typeName = L'\0';
            return true;
        }
        else
            return false;   // Not enough room.
    }

    parent = sym->parent;

    // If Our parent isn't the root, get the parent name and seperator and advance beyond it.
    if (parent->parent) {
        if (! GetFullMetadataName(parent, typeName, cchTypeName - 1, chNestedClassSep, chDotReplacement))
            return false;
        cch = (int)wcslen(typeName);
        typeName += cch;
        *typeName++ = (parent->kind == SK_AGGSYM) ? chNestedClassSep : L'.';    // Dot seperates namespace/type elements here.
        cchTypeName -= (cch + 1);
        if (cchTypeName > 0)
            *typeName = 0; // NULL terminate it so we'll always return a usefull string
        else
            typeName[-1] = 0;
    }

    // Get the current name and add it on
    if (sym->kind == SK_PROPSYM)
        name = sym->asPROPSYM()->getRealName();
    else
        name = sym->name;
    if (name == NULL) {
        GetExplicitImplName(sym, buffer, lengthof(buffer));
        text = buffer;
    }
    else {
        text = name->text;
    }

    cch = (int)wcslen(text);
    if (cchTypeName < cch + 2)
        return false;       // Not enough space.

    wcscpy(typeName, text);
    
    if (chDotReplacement != L'.') {
        for (WCHAR *p = wcschr( typeName, L'.'); p != NULL; p = wcschr( p, L'.'))
            *p++ = chDotReplacement;
    }

    return true;
}

/*
 * Determine the flags for a typedef definition in metadata
 */
DWORD METADATAHELPER::GetAggregateFlags(AGGSYM * sym)
{
    DWORD flags = 0;
    // Determine flags.

    // Set access flags.
    if (sym->parent->kind == SK_NSSYM) {
        // "Top-level" aggregate. Can only be public or internal.
        ASSERT(sym->access == ACC_PUBLIC || sym->access == ACC_INTERNAL);
        if (sym->access == ACC_PUBLIC)
            flags |= tdPublic;
        else
            flags |= tdNotPublic;
    }
    else {
        // nested aggregate. Can be any access.
        switch (sym->access) {
        case ACC_PUBLIC:     flags |= tdNestedPublic;   break;
        case ACC_INTERNAL:   flags |= tdNestedAssembly; break;
        case ACC_PROTECTED:  flags |= tdNestedFamily;   break;
        case ACC_INTERNALPROTECTED:
                             flags |= tdNestedFamORAssem; break;
        case ACC_PRIVATE:    flags |= tdNestedPrivate;  break;
        default: ASSERT(!"Bad access flag");            break;
        }
    }

    if (sym->isClass) {
        if (sym->isSealed)
            flags |= tdSealed;
        if (sym->isAbstract)
            flags |= tdAbstract;
    }
    else if (sym->isInterface) {
        flags |= tdInterface | tdAbstract;
    }
    else if (sym->isEnum) {
        flags |= (tdSealed);
    }
    else if (sym->isStruct) {
        flags |= (tdSealed);
    }
    else if (sym->isDelegate) {
        ASSERT(sym->isSealed);

        flags |= tdSealed;
    }
    else {
        ASSERT(0);
    }

    if ((sym->isClass || sym->isStruct) && !sym->asAGGSYM()->hasStaticCtor)
        flags |= tdBeforeFieldInit; 
    // The default is precise

    return flags;
}


/*
 * Get a synthesized name for explicit interface implementations. The name we use is:
 * "InterfaceName.MethodName", where InterfaceName is the fully qualified name of the
 * interface containing the implemented method. This name has a '.' in it, so it can't
 * conflict with any "real" name or be confused with one.
 * If count != -1 then we append count to the name, that way we can use it for
 * emitting global fields.
 */
void METADATAHELPER::GetExplicitImplName(SYM * sym, WCHAR * buffer, int cch)
{
    int len;

    ASSERT(sym->kind == SK_EVENTSYM ||
           (sym->asMETHPROPSYM() && sym->asMETHPROPSYM()->explicitImpl) || 
           (sym->asMETHSYM() && sym->asMETHSYM()->isPropertyAccessor && sym->asMETHSYM()->getProperty()->isEvent && sym->asMETHSYM()->getProperty()->explicitImpl));

    SYM * impl;
    
    if (sym->kind == SK_EVENTSYM)
        impl = sym->asEVENTSYM()->getExplicitImpl();
    else if (sym->asMETHPROPSYM()->explicitImpl)
        impl = sym->asMETHPROPSYM()->explicitImpl;
    else 
        impl = sym->asMETHSYM()->getProperty()->explicitImpl;

    AGGSYM * cls;
    cls = impl->parent->asAGGSYM();
    ASSERT(cls->isInterface || cls->getAssemblyIndex() != sym->getAssemblyIndex());

    // Get interface name
    METADATAHELPER::GetFullMetadataName(cls, buffer, cch);
    len = (int)wcslen(buffer);
    buffer += len;
    cch -= len;

    // Add '.' seperator.
    *buffer++ = '.';
    --cch;

    // Add appropriate prefix to differentiate get from set accessor.
    if (sym->kind != SK_EVENTSYM && !sym->asMETHPROPSYM()->explicitImpl) {
        if (cch < 5)
            return;

        wcscpy(buffer, sym->asMETHSYM()->isGetAccessor() ? L"get_" : L"set_");

        len = (int)wcslen(buffer);
        buffer += len;
        cch -= len;
    }

    // Append method name, making sure not to overflow the buffer.
    const WCHAR * methName = impl->name->text;
    if ((int) wcslen(methName) < cch) {
        wcscpy(buffer, methName);
    }
    else {
        memcpy(buffer, methName, (cch - 1) * sizeof(WCHAR));
        buffer[cch - 1] = L'\0';
    }
}

