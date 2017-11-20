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
// File: metaattr.h
//
// ===========================================================================

#ifndef __metaattr_h__
#define __metaattr_h__

const CorAttributeTargets catReturnValue = (CorAttributeTargets) 0x2000;
const CorAttributeTargets catAllOld = catAll;
#define catAll ((CorAttributeTargets) (catAllOld | catReturnValue))

/*
 * ATTRBIND
 */
class ATTRBIND
{
public:

    void CompileSingleAttribute(ATTRNODE *attr);
    virtual void    ValidateAttributes();

protected:

    ATTRBIND(COMPILER *compiler, bool haveAttributeUsage);

    void Init(SYM * sym);
    void Init(CorAttributeTargets ek, PARENTSYM * context);

    void Compile(BASENODE *attributes);
    void CompileSynthetizedAttribute(AGGSYM * attributeClass, EXPR * ctorExpression, EXPR * namedArgs);

    //
    // the main loop methods
    //
    virtual bool    BindAttribute(ATTRNODE *attr);
    bool            BindPredefinedAttribute(ATTRNODE *attr);
    virtual bool    BindClassAttribute(ATTRNODE *attr);
    void            VerifyAndEmit(ATTRNODE *attr);
    virtual void    VerifyAndEmitClassAttribute(ATTRNODE *attr);
    virtual void    VerifyAndEmitPredefinedAttribute(ATTRNODE *attr) = 0;
    virtual mdToken GetToken() { return sym->getTokenEmit(); }
    BYTE *          addAttributeArg(EXPR *arg, BYTE* buffer, BYTE* bufferEnd);
    virtual bool    IsCLSContext();

    //
    // helpers for predefined attributes which can occur on more than one SK
    //
    bool compileDeprecatedAttribute(ATTRNODE * attr);
    void compileObsolete(ATTRNODE * attr);
    void compileCLS(ATTRNODE * attr);
    void emitCLS(ATTRNODE * attr);

    //
    // error helpers
    //
    void error(BASENODE *tree, int id);
    void errorNameRef(BASENODE *tree, PARENTSYM *context, int id);
    void errorNameRef(NAME *name, BASENODE *tree, PARENTSYM *context, int id);
    void errorNameRefStr(NAME *name, BASENODE *tree, PARENTSYM *context, LPCWSTR str, int id);
    void errorStrFile(BASENODE *tree, INFILESYM *infile, int id, LPCWSTR str);
    void errorStrStrFile(BASENODE *tree, INFILESYM *infile, int id, LPCWSTR str1, LPCWSTR str2);
    void errorSymbol(SYM *sym, int id);
    void errorSymbolStr(SYM *symbol, int id, LPCWSTR str);
    void errorFile(BASENODE *tree, INFILESYM *infile, int id);
    void errorFileSymbol(BASENODE *tree, INFILESYM *infile, int id, SYM *sym);
    void errorBadSymbolKind(BASENODE *tree);

    // stuff for the symbol being attributed
    SYM *               sym;
    CorAttributeTargets ek;
    ATTRLOC             attrloc;
    PARENTSYM *         context;

    // stuff for a single attribute
    AGGSYM *        attributeClass;
    EXPR *          ctorExpression;
    EXPR *          namedArguments;
    unsigned        estimatedBlobSize;
    unsigned        numberOfNamedArguments;
    SYMLIST *       customAttributeList;
    PREDEFATTRSYM * predefinedAttribute;

    bool            haveAttributeUsage;     // true if attribute usage has been computed for all types
    COMPILER *      compiler;

    // return the given predefined type (including void)
    TYPESYM *       getPDT(PREDEFTYPE tp);
    NAME *          GetPredefName(PREDEFNAME pn);
    bool            isAttributeType(TYPESYM *);

    bool            getValue(EXPR *argument, bool * rval);
    bool            getValue(EXPR *argument, int * rval);
    bool            getValue(EXPR *argument, STRCONST ** rval);
    bool            getValue(EXPR *argument, TYPESYM **type);

    static NAME *   getNamedArgumentName(EXPR *expr);
    static EXPR *   getNamedArgumentValue(EXPR *expr);

    static STRCONST *   getKnownString(EXPR *expr);
    static bool         getKnownBool(EXPR *expr);

private:

    bool verifyAttributeArg(EXPR *arg, unsigned &totalSize);

};

/* 
 * AGGATTRBIND
 */
class AGGATTRBIND : public ATTRBIND
{
public:

    static void Compile(COMPILER *compiler, AGGSYM *cls, AGGINFO *info);
    void ValidateAttributes();

protected:

    AGGATTRBIND(COMPILER * compiler, AGGSYM *cls, AGGINFO *info, NAME * defaultMemberName);
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);
    void VerifyAndEmitClassAttribute(ATTRNODE *attr);

    void compileStructLayout(ATTRNODE * attr);
    void compileCoClass(ATTRNODE * attr);

private:

    bool checkAggForRefMembers(ATTRNODE * attr, AGGSYM *type);

    AGGSYM *        cls;
    AGGINFO *       info;
    NAME *          defaultMemberName;

};

/* 
 * ATTRATTRBIND - binder for the predefined Attribute class
 * which must be compiled early in the prepare stage
 */
class ATTRATTRBIND : public ATTRBIND
{
public:

    static void Compile(COMPILER *compiler, AGGSYM *cls);

protected:

    ATTRATTRBIND(COMPILER * compiler, AGGSYM *cls);
    bool BindAttribute(ATTRNODE *attr);
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);

private:

    AGGSYM *        cls;

};

/* 
 * OBSOLETEATTRBIND - binder for the predefined obsolete attribute
 * which must be compiled early in the prepare stage
 */
class OBSOLETEATTRBIND : public ATTRBIND
{
public:

    static void Compile(COMPILER *compiler, SYM *sym);

protected:

    OBSOLETEATTRBIND(COMPILER * compiler, SYM *sym);
    bool BindAttribute(ATTRNODE *attr);
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);

};

/* 
 * CONDITIONALATTRBIND
 */
class CONDITIONALATTRBIND : public ATTRBIND
{
public:

    static void Compile(COMPILER *compiler, METHSYM *method);

protected:

    CONDITIONALATTRBIND(COMPILER *compiler, METHSYM *method);
    bool BindAttribute(ATTRNODE *attr);
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);

protected:

    METHSYM *       method;
    NAMELIST **     list;
};

/* 
 * PARAMATTRBIND
 */
class PARAMATTRBIND : public ATTRBIND
{
public:

    static void CompileParamList(COMPILER *compiler, METHSYM *method, METHINFO *info);
    void ValidateAttributes();

    // for return values 
    PARAMATTRBIND(COMPILER *compiler, METHSYM *method, TYPESYM *type, PARAMINFO *info, int index);

    void EmitPredefinedAttributes();

protected:

    // for regular parameter lists
    PARAMATTRBIND(COMPILER *compiler, METHSYM *method);
    void Init(TYPESYM *type, PARAMINFO *info, int index);

    mdToken GetToken();
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);
    bool    IsCLSContext();

    void EmitParamProps();

private:

    METHSYM *       method;
    PARAMINFO *     paramInfo;
    TYPESYM *       parameterType;
    int             index;
    
};

/* 
 * METHATTRBIND
 */
class METHATTRBIND : public ATTRBIND
{
public:

    static void Compile(COMPILER *compiler, METHSYM *method, METHINFO *info, AGGINFO *agginfo);

protected:

    METHATTRBIND(COMPILER *compiler, METHSYM *method, METHINFO *info, AGGINFO *agginfo);
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);

    void compileMethodAttribDllImport(ATTRNODE * attr);

protected:

    METHSYM *       method;
    METHINFO *      info;
    AGGINFO *       agginfo;

};

/* 
 * ACCESSORATTRBIND
 */
class ACCESSORATTRBIND : public METHATTRBIND
{
public:

    static void Compile(COMPILER *compiler, METHSYM *method, METHINFO *info, AGGINFO *agginfo);

protected:

    ACCESSORATTRBIND(COMPILER *compiler, METHSYM *method, METHINFO *info, AGGINFO *agginfo);
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);

};

/* 
 * PROPATTRBIND
 */
class PROPATTRBIND : public ATTRBIND
{
public:

    static void Compile(COMPILER *compiler, PROPSYM *property, PROPINFO *info);

protected:

    PROPATTRBIND(COMPILER *compiler, PROPSYM *property, PROPINFO *info);
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);

private:

    PROPSYM *       property;
    PROPINFO *      info;

};

/* 
 * EVENTATTRBIND
 */
class EVENTATTRBIND : public ATTRBIND
{
public:

    static void Compile(COMPILER *compiler, EVENTSYM *event, EVENTINFO *info);

protected:

    EVENTATTRBIND(COMPILER *compiler, EVENTSYM *event, EVENTINFO *info);
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);

private:

    EVENTSYM *      event;
    EVENTINFO *     info;

};

/* 
 * MEMBVARATTRBIND
 */
class MEMBVARATTRBIND : public ATTRBIND
{
public:

    static void Compile(COMPILER *compiler, MEMBVARSYM *field, MEMBVARINFO *info, AGGINFO *agginfo);
    void ValidateAttributes();

protected:

    MEMBVARATTRBIND(COMPILER *compiler, MEMBVARSYM *field, MEMBVARINFO *info, AGGINFO *agginfo);
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);
    void compileStructOffset(ATTRNODE *attr);

private:

    static void ValidateAttributes(COMPILER *compiler, MEMBVARSYM *field, MEMBVARINFO *info, AGGINFO *agginfo);

    MEMBVARSYM *    field;
    MEMBVARINFO *   info;
    AGGINFO *       agginfo;

};

/* 
 * GLOBALATTRBIND
 */
class GLOBALATTRBIND : public ATTRBIND
{
public:

    static void Compile(COMPILER *compiler, PARENTSYM * context, GLOBALATTRSYM *globalAttribute, mdToken tokenEmit);

protected:

    GLOBALATTRBIND(COMPILER * compiler, GLOBALATTRSYM *globalAttribute, mdToken tokenEmit);
    GLOBALATTRBIND(COMPILER * compiler, mdToken tokenEmit);
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);
    mdToken GetToken() { return tokenEmit; }

private:

    GLOBALATTRSYM *     globalAttribute;
    mdToken             tokenEmit;

};


/* 
 * CLSGLOBALATTRBIND
 */
class CLSGLOBALATTRBIND : public ATTRBIND
{
public:

    static void Compile(COMPILER *compiler, GLOBALATTRSYM *globalAttribute);

protected:

    CLSGLOBALATTRBIND(COMPILER * compiler, GLOBALATTRSYM *globalAttribute);
    bool BindAttribute(ATTRNODE *attr);
    void VerifyAndEmitPredefinedAttribute(ATTRNODE *attr);

private:
};

#endif //__metaattr_h__

