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
// File: nodekind.h
//
// NOTE:  If you change this list, there are places that MUST ALSO be updated.
// NOTE:  These include:
//          dumpnode.h (all node dumping)
//          srcmod.cpp (GetFirstToken, GetLastToken, FindLeafNode)
//
// There may be others depending on the node you add (such as GetInteriorTree, etc.)
// ===========================================================================

//       NK_* constant        *NODE struct        NG_* flags (or'd if appropriate)
NODEKIND(ACCESSOR           , ACCESSOR          , NG_INTERIOR                                                                               )
NODEKIND(ARRAYINIT          , UNOP              , 0                                                                                         )
NODEKIND(ARROW              , BINOP             , NG_BINOP                                                                                  )
NODEKIND(ATTR               , ATTR              , 0                                                                                         )
NODEKIND(ATTRARG            , ATTR              , 0                                                                                         )
NODEKIND(ATTRDECL           , ATTRDECL          , 0                                                                                         )
NODEKIND(BINOP              , BINOP             , NG_BINOP                                                                                  )
NODEKIND(BLOCK              , BLOCK             , NG_STATEMENT                                                                              )
NODEKIND(BREAK              , EXPRSTMT          , NG_STATEMENT                                                                              )
NODEKIND(CALL               , CALL              , NG_BINOP                                                                                  )
NODEKIND(CASE               , CASE              , NG_BREAKABLE                                                                              )
NODEKIND(CASELABEL          , UNOP              , 0                                                                                         )
NODEKIND(CATCH              , CATCH             , 0                                                                                         )
NODEKIND(CHECKED            , LABELSTMT         , NG_STATEMENT | NG_EMBEDDEDSTMTOWNER                                                       )
NODEKIND(CLASS              , CLASS             , NG_TYPE | NG_AGGREGATE | NG_KEYED                                                         )
NODEKIND(CONST              , FIELD             , NG_FIELD | NG_MEMBER                                                                      )
NODEKIND(CONSTVAL           , CONSTVAL          , 0                                                                                         )
NODEKIND(CONTINUE           , EXPRSTMT          , NG_STATEMENT                                                                              )
NODEKIND(CTOR               , METHOD            , NG_METHOD | NG_KEYED | NG_INTERIOR | NG_MEMBER                                            )
NODEKIND(DECLSTMT           , DECLSTMT          , NG_STATEMENT                                                                              )
NODEKIND(DELEGATE           , DELEGATE          , NG_TYPE | NG_KEYED                                                                        )
NODEKIND(DO                 , LOOPSTMT          , NG_STATEMENT | NG_GLOBALCOMPLETION | NG_BREAKABLE | NG_CONTINUABLE | NG_EMBEDDEDSTMTOWNER )
NODEKIND(DOT                , BINOP             , NG_BINOP                                                                                  )
NODEKIND(DTOR               , METHOD            , NG_METHOD | NG_KEYED | NG_INTERIOR | NG_MEMBER                                            )
NODEKIND(EMPTYSTMT          , STATEMENT         , NG_STATEMENT | NG_GLOBALCOMPLETION                                                        )
NODEKIND(ENUM               , CLASS             , NG_TYPE | NG_AGGREGATE | NG_KEYED                                                         )
NODEKIND(ENUMMBR            , ENUMMBR           , NG_KEYED | NG_MEMBER                                                                      )
NODEKIND(EXPRSTMT           , EXPRSTMT          , NG_STATEMENT                                                                              )
NODEKIND(FIELD              , FIELD             , NG_FIELD | NG_MEMBER                                                                      )
NODEKIND(FOR                , FORSTMT           , NG_STATEMENT | NG_GLOBALCOMPLETION | NG_BREAKABLE | NG_CONTINUABLE | NG_EMBEDDEDSTMTOWNER )
NODEKIND(GOTO               , EXPRSTMT          , NG_STATEMENT                                                                              )
NODEKIND(IF                 , IFSTMT            , NG_STATEMENT | NG_GLOBALCOMPLETION | NG_EMBEDDEDSTMTOWNER                                 )
NODEKIND(INTERFACE          , CLASS             , NG_TYPE | NG_AGGREGATE | NG_KEYED                                                         )
NODEKIND(LABEL              , LABELSTMT         , NG_STATEMENT | NG_GLOBALCOMPLETION | NG_EMBEDDEDSTMTOWNER                                 )
NODEKIND(LIST               , BINOP             , NG_BINOP                                                                                  )
NODEKIND(LOCK               , LOOPSTMT          , NG_STATEMENT | NG_GLOBALCOMPLETION | NG_EMBEDDEDSTMTOWNER                                 )
NODEKIND(MEMBER             , MEMBER            , 0                                                                                         )
NODEKIND(METHOD             , METHOD            , NG_METHOD | NG_KEYED | NG_INTERIOR | NG_MEMBER                                            )
NODEKIND(NAME               , NAME              , 0                                                                                         )
NODEKIND(NAMESPACE          , NAMESPACE         , NG_KEYED                                                                                  )
NODEKIND(NESTEDTYPE         , NESTEDTYPE        , NG_MEMBER                                                                                 )
NODEKIND(NEW                , NEW               , 0                                                                                         )
NODEKIND(OP                 , BASE              , 0                                                                                         )
NODEKIND(OPERATOR           , METHOD            , NG_METHOD | NG_KEYED | NG_INTERIOR | NG_MEMBER                                            )
NODEKIND(PARAMETER          , PARAMETER         , 0                                                                                         )
NODEKIND(PARTIALMEMBER      , PARTIALMEMBER     , NG_MEMBER                                                                                 )
NODEKIND(PROPERTY           , PROPERTY          , NG_PROPERTY | NG_KEYED | NG_MEMBER                                                        )
NODEKIND(INDEXER            , PROPERTY          , NG_PROPERTY | NG_KEYED | NG_MEMBER                                                        )
NODEKIND(RETURN             , EXPRSTMT          , NG_STATEMENT                                                                              )
NODEKIND(THROW              , EXPRSTMT          , NG_STATEMENT                                                                              )
NODEKIND(TRY                , TRYSTMT           , NG_STATEMENT                                                                              )
NODEKIND(TYPE               , TYPE              , 0                                                                                         )
NODEKIND(STRUCT             , CLASS             , NG_TYPE | NG_AGGREGATE | NG_KEYED                                                         )
NODEKIND(SWITCH             , SWITCHSTMT        , NG_STATEMENT | NG_GLOBALCOMPLETION | NG_BREAKABLE                                         )
NODEKIND(UNOP               , UNOP              , 0                                                                                         )
NODEKIND(UNSAFE             , LABELSTMT         , NG_STATEMENT                                                                              )
NODEKIND(USING              , USING             , 0                                                                                         )
NODEKIND(VARDECL            , VARDECL           , NG_KEYED                                                                                  )
NODEKIND(WHILE              , LOOPSTMT          , NG_STATEMENT | NG_GLOBALCOMPLETION | NG_BREAKABLE | NG_CONTINUABLE | NG_EMBEDDEDSTMTOWNER )
#undef NODEKIND













