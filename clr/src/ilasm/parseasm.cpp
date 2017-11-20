/*
 * Created by Microsoft VCBU Internal YACC from "asmparse.y"
 */

#line 2 "asmparse.y"

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

#include "asmparse.h"
#include <crtdbg.h>             // FOR ASSERTE
#include <string.h>             // for strcmp    
#include <mbstring.h>           // for _mbsinc    
#include <ctype.h>                      // for isspace    
#include <stdlib.h>             // for strtoul
#include "openum.h"             // for CEE_*
#include <stdarg.h>         // for vararg macros 

#define YYMAXDEPTH 65535

// #define DEBUG_PARSING
#ifdef DEBUG_PARSING
bool parseDBFlag = true;
#define dbprintf(x)     if (parseDBFlag) printf x
#define YYDEBUG 1
#else
#define dbprintf(x)     
#endif

#define FAIL_UNLESS(cond, msg) if (!(cond)) { parser->success = false; parser->error msg; }

static AsmParse* parser = 0;
#define PASM    (parser->assem)
#define PASMM   (parser->assem->m_pManifest)

static char* newStringWDel(char* str1, char* str2, char* str3 = 0);
static char* newString(char* str1);
static void corEmitInt(BinStr* buff, unsigned data);
bool bParsingByteArray = FALSE;
bool bExternSource = FALSE;
int  nExtLine,nExtCol;
int iOpcodeLen = 0;

ARG_NAME_LIST *palDummy;

int  nTemp=0;

unsigned int g_uCodePage = CP_ACP;
extern DWORD    g_dwSubsystem,g_dwComImageFlags,g_dwFileAlignment;
extern size_t    g_stBaseAddress;
unsigned int uMethodBeginLine,uMethodBeginColumn;
extern BOOL    g_bOnUnicode;

#line 54 "asmparse.y"

#define UNION 1
typedef union  {
        CorRegTypeAttr classAttr;
        CorMethodAttr methAttr;
        CorFieldAttr fieldAttr;
        CorMethodImpl implAttr;
        CorEventAttr  eventAttr;
        CorPropertyAttr propAttr;
        CorPinvokeMap pinvAttr;
        CorDeclSecurity secAct;
        CorFileFlags fileAttr;
        CorAssemblyFlags asmAttr;
        CorAssemblyFlags asmRefAttr;
        CorTypeAttr comtAttr;
        CorManifestResourceFlags manresAttr;
        double*  float64;
        __int64* int64;
        __int32  int32;
        char*    string;
        BinStr*  binstr;
        Labels*  labels;
        Instr*   instr;         // instruction opcode
        NVPair*  pair;
} YYSTYPE;
# define ERROR_ 257 
# define BAD_COMMENT_ 258 
# define BAD_LITERAL_ 259 
# define ID 260 
# define DOTTEDNAME 261 
# define QSTRING 262 
# define SQSTRING 263 
# define INT32 264 
# define INT64 265 
# define FLOAT64 266 
# define HEXBYTE 267 
# define DCOLON 268 
# define ELIPSES 269 
# define VOID_ 270 
# define BOOL_ 271 
# define CHAR_ 272 
# define UNSIGNED_ 273 
# define INT_ 274 
# define INT8_ 275 
# define INT16_ 276 
# define INT32_ 277 
# define INT64_ 278 
# define FLOAT_ 279 
# define FLOAT32_ 280 
# define FLOAT64_ 281 
# define BYTEARRAY_ 282 
# define OBJECT_ 283 
# define STRING_ 284 
# define NULLREF_ 285 
# define DEFAULT_ 286 
# define CDECL_ 287 
# define VARARG_ 288 
# define STDCALL_ 289 
# define THISCALL_ 290 
# define FASTCALL_ 291 
# define CONST_ 292 
# define CLASS_ 293 
# define TYPEDREF_ 294 
# define UNMANAGED_ 295 
# define NOT_IN_GC_HEAP_ 296 
# define FINALLY_ 297 
# define HANDLER_ 298 
# define CATCH_ 299 
# define FILTER_ 300 
# define FAULT_ 301 
# define EXTENDS_ 302 
# define IMPLEMENTS_ 303 
# define TO_ 304 
# define AT_ 305 
# define TLS_ 306 
# define TRUE_ 307 
# define FALSE_ 308 
# define VALUE_ 309 
# define VALUETYPE_ 310 
# define NATIVE_ 311 
# define INSTANCE_ 312 
# define SPECIALNAME_ 313 
# define STATIC_ 314 
# define PUBLIC_ 315 
# define PRIVATE_ 316 
# define FAMILY_ 317 
# define FINAL_ 318 
# define SYNCHRONIZED_ 319 
# define INTERFACE_ 320 
# define SEALED_ 321 
# define NESTED_ 322 
# define ABSTRACT_ 323 
# define AUTO_ 324 
# define SEQUENTIAL_ 325 
# define EXPLICIT_ 326 
# define WRAPPER_ 327 
# define ANSI_ 328 
# define UNICODE_ 329 
# define AUTOCHAR_ 330 
# define IMPORT_ 331 
# define ENUM_ 332 
# define VIRTUAL_ 333 
# define NOTREMOTABLE_ 334 
# define SPECIAL_ 335 
# define NOINLINING_ 336 
# define UNMANAGEDEXP_ 337 
# define BEFOREFIELDINIT_ 338 
# define METHOD_ 339 
# define FIELD_ 340 
# define PINNED_ 341 
# define MODREQ_ 342 
# define MODOPT_ 343 
# define SERIALIZABLE_ 344 
# define ASSEMBLY_ 345 
# define FAMANDASSEM_ 346 
# define FAMORASSEM_ 347 
# define PRIVATESCOPE_ 348 
# define HIDEBYSIG_ 349 
# define NEWSLOT_ 350 
# define RTSPECIALNAME_ 351 
# define PINVOKEIMPL_ 352 
# define _CTOR 353 
# define _CCTOR 354 
# define LITERAL_ 355 
# define NOTSERIALIZED_ 356 
# define INITONLY_ 357 
# define REQSECOBJ_ 358 
# define CIL_ 359 
# define OPTIL_ 360 
# define MANAGED_ 361 
# define FORWARDREF_ 362 
# define PRESERVESIG_ 363 
# define RUNTIME_ 364 
# define INTERNALCALL_ 365 
# define _IMPORT 366 
# define NOMANGLE_ 367 
# define LASTERR_ 368 
# define WINAPI_ 369 
# define AS_ 370 
# define INSTR_NONE 371 
# define INSTR_VAR 372 
# define INSTR_I 373 
# define INSTR_I8 374 
# define INSTR_R 375 
# define INSTR_BRTARGET 376 
# define INSTR_METHOD 377 
# define INSTR_FIELD 378 
# define INSTR_TYPE 379 
# define INSTR_STRING 380 
# define INSTR_SIG 381 
# define INSTR_RVA 382 
# define INSTR_TOK 383 
# define INSTR_SWITCH 384 
# define INSTR_PHI 385 
# define _CLASS 386 
# define _NAMESPACE 387 
# define _METHOD 388 
# define _FIELD 389 
# define _DATA 390 
# define _EMITBYTE 391 
# define _TRY 392 
# define _MAXSTACK 393 
# define _LOCALS 394 
# define _ENTRYPOINT 395 
# define _ZEROINIT 396 
# define _PDIRECT 397 
# define _EVENT 398 
# define _ADDON 399 
# define _REMOVEON 400 
# define _FIRE 401 
# define _OTHER 402 
# define PROTECTED_ 403 
# define _PROPERTY 404 
# define _SET 405 
# define _GET 406 
# define READONLY_ 407 
# define _PERMISSION 408 
# define _PERMISSIONSET 409 
# define REQUEST_ 410 
# define DEMAND_ 411 
# define ASSERT_ 412 
# define DENY_ 413 
# define PERMITONLY_ 414 
# define LINKCHECK_ 415 
# define INHERITCHECK_ 416 
# define REQMIN_ 417 
# define REQOPT_ 418 
# define REQREFUSE_ 419 
# define PREJITGRANT_ 420 
# define PREJITDENY_ 421 
# define NONCASDEMAND_ 422 
# define NONCASLINKDEMAND_ 423 
# define NONCASINHERITANCE_ 424 
# define _LINE 425 
# define P_LINE 426 
# define _LANGUAGE 427 
# define _CUSTOM 428 
# define INIT_ 429 
# define _SIZE 430 
# define _PACK 431 
# define _VTABLE 432 
# define _VTFIXUP 433 
# define FROMUNMANAGED_ 434 
# define CALLMOSTDERIVED_ 435 
# define _VTENTRY 436 
# define _FILE 437 
# define NOMETADATA_ 438 
# define _HASH 439 
# define _ASSEMBLY 440 
# define _PUBLICKEY 441 
# define _PUBLICKEYTOKEN 442 
# define ALGORITHM_ 443 
# define _VER 444 
# define _LOCALE 445 
# define EXTERN_ 446 
# define _MRESOURCE 447 
# define _LOCALIZED 448 
# define IMPLICITCOM_ 449 
# define IMPLICITRES_ 450 
# define NOAPPDOMAIN_ 451 
# define NOPROCESS_ 452 
# define NOMACHINE_ 453 
# define _MODULE 454 
# define _EXPORT 455 
# define MARSHAL_ 456 
# define CUSTOM_ 457 
# define SYSSTRING_ 458 
# define FIXED_ 459 
# define VARIANT_ 460 
# define CURRENCY_ 461 
# define SYSCHAR_ 462 
# define DECIMAL_ 463 
# define DATE_ 464 
# define BSTR_ 465 
# define TBSTR_ 466 
# define LPSTR_ 467 
# define LPWSTR_ 468 
# define LPTSTR_ 469 
# define OBJECTREF_ 470 
# define IUNKNOWN_ 471 
# define IDISPATCH_ 472 
# define STRUCT_ 473 
# define SAFEARRAY_ 474 
# define BYVALSTR_ 475 
# define LPVOID_ 476 
# define ANY_ 477 
# define ARRAY_ 478 
# define LPSTRUCT_ 479 
# define IN_ 480 
# define OUT_ 481 
# define OPT_ 482 
# define LCID_ 483 
# define RETVAL_ 484 
# define _PARAM 485 
# define _OVERRIDE 486 
# define WITH_ 487 
# define NULL_ 488 
# define HRESULT_ 489 
# define CARRAY_ 490 
# define USERDEFINED_ 491 
# define RECORD_ 492 
# define FILETIME_ 493 
# define BLOB_ 494 
# define STREAM_ 495 
# define STORAGE_ 496 
# define STREAMED_OBJECT_ 497 
# define STORED_OBJECT_ 498 
# define BLOB_OBJECT_ 499 
# define CF_ 500 
# define CLSID_ 501 
# define VECTOR_ 502 
# define _SUBSYSTEM 503 
# define _CORFLAGS 504 
# define ALIGNMENT_ 505 
# define _IMAGEBASE 506 
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
#ifndef YYFARDATA
#define    YYFARDATA    /*nothing*/
#endif
#if ! defined YYSTATIC
#define    YYSTATIC    /*nothing*/
#endif
#if ! defined YYCONST
#define    YYCONST    /*nothing*/
#endif
#ifndef    YYACT
#define    YYACT    yyact
#endif
#ifndef    YYPACT
#define    YYPACT    yypact
#endif
#ifndef    YYPGO
#define    YYPGO    yypgo
#endif
#ifndef    YYR1
#define    YYR1    yyr1
#endif
#ifndef    YYR2
#define    YYR2    yyr2
#endif
#ifndef    YYCHK
#define    YYCHK    yychk
#endif
#ifndef    YYDEF
#define    YYDEF    yydef
#endif
#ifndef    YYV
#define    YYV    yyv
#endif
#ifndef    YYS
#define    YYS    yys
#endif
#ifndef    YYLOCAL
#define    YYLOCAL
#endif
#ifndef YYR_T
#define    YYR_T    int
#endif
typedef    YYR_T    yyr_t;
#ifndef YYEXIND_T
#define    YYEXIND_T    unsigned int
#endif
typedef    YYEXIND_T    yyexind_t;
#ifndef YYOPTTIME
#define    YYOPTTIME    0
#endif
# define YYERRCODE 256

#line 1374 "asmparse.y"

/********************************************************************************/
/* Code goes here */

/********************************************************************************/

void yyerror(char* str) {
    char tokBuff[64];
    char *ptr;
    size_t len = parser->curPos - parser->curTok;
    if (len > 63) len = 63;
    memcpy(tokBuff, parser->curTok, len);
    tokBuff[len] = 0;
    fprintf(stderr, "%s(%d) : error : %s at token '%s' in: %s\n", 
            parser->in->name(), parser->curLine, str, tokBuff, (ptr=parser->getLine(parser->curLine)));
    parser->success = false;
    delete ptr;
}

struct Keywords {
    const char* name;
    unsigned short token;
    unsigned short tokenVal;// this holds the instruction enumeration for those keywords that are instrs
};

#define NO_VALUE        ((unsigned short)-1)              // The token has no value

static Keywords keywords[] = {
// Attention! Because of aliases, the instructions MUST go first!
// Redefine all the instructions (defined in assembler.h <- asmenum.h <- opcode.def)
#undef InlineNone
#undef InlineVar        
#undef ShortInlineVar
#undef InlineI          
#undef ShortInlineI     
#undef InlineI8         
#undef InlineR          
#undef ShortInlineR     
#undef InlineBrTarget
#undef ShortInlineBrTarget
#undef InlineMethod
#undef InlineField 
#undef InlineType 
#undef InlineString
#undef InlineSig        
#undef InlineRVA        
#undef InlineTok        
#undef InlineSwitch
#undef InlinePhi        
#undef InlineVarTok     


#define InlineNone              INSTR_NONE
#define InlineVar               INSTR_VAR
#define ShortInlineVar          INSTR_VAR
#define InlineI                 INSTR_I
#define ShortInlineI            INSTR_I
#define InlineI8                INSTR_I8
#define InlineR                 INSTR_R
#define ShortInlineR            INSTR_R
#define InlineBrTarget          INSTR_BRTARGET
#define ShortInlineBrTarget             INSTR_BRTARGET
#define InlineMethod            INSTR_METHOD
#define InlineField             INSTR_FIELD
#define InlineType              INSTR_TYPE
#define InlineString            INSTR_STRING
#define InlineSig               INSTR_SIG
#define InlineRVA               INSTR_RVA
#define InlineTok               INSTR_TOK
#define InlineSwitch            INSTR_SWITCH
#define InlinePhi               INSTR_PHI

#define InlineVarTok            0
#define NEW_INLINE_NAMES
                // The volatile instruction collides with the volatile keyword, so 
                // we treat it as a keyword everywhere and modify the grammar accordingly (Yuck!) 
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) { s, args, c },
#define OPALIAS(alias_c, s, c) { s, NO_VALUE, c },
#include "opcode.def"
#undef OPALIAS
#undef OPDEF

                /* keywords */
#define KYWD(name, sym, val)    { name, sym, val},
#include "il_kywd.h"
#undef KYWD

        // These are deprecated
        { "float",                      FLOAT_ },
};

/********************************************************************************/
/* used by qsort to sort the keyword table */
static int __cdecl keywordCmp(const void *op1, const void *op2)
{
    return  strcmp(((Keywords*) op1)->name, ((Keywords*) op2)->name);
}

/********************************************************************************/
/* looks up the keyword 'name' of length 'nameLen' (name does not need to be 
   null terminated)   Returns 0 on failure */

static int findKeyword(const char* name, size_t nameLen, Instr** value) 
{
    Keywords* low = keywords;
    Keywords* high = &keywords[sizeof(keywords) / sizeof(Keywords)];

    _ASSERTE (high > low);          // Table is non-empty
    for(;;) 
        {
        Keywords* mid = &low[(high - low) / 2];

                // compare the strings
        int cmp = strncmp(name, mid->name, nameLen);
        if (cmp == 0 && nameLen < strlen(mid->name)) --cmp;
        if (cmp == 0)
        {
            //printf("Token '%s' = %d opcode = %d\n", mid->name, mid->token, mid->tokenVal);
            if (mid->tokenVal != NO_VALUE)
            {
                if((*value = new Instr))
                {
                    (*value)->opcode = mid->tokenVal;
                    (*value)->linenum = (bExternSource ? nExtLine : parser->curLine);
                    (*value)->column = (bExternSource ? nExtCol : 1);
                }
            }
            else *value = NULL;

            return(mid->token);
        }

        if (mid == low)  return(0);

        if (cmp > 0) low = mid;
        else        high = mid;
    }
}

/********************************************************************************/
/* convert str to a uint64 */

static unsigned __int64 str2uint64(const char* str, const char** endStr, unsigned radix) 
{
    static unsigned digits[256];
    static BOOL initialize=TRUE;
    unsigned __int64 ret = 0;
    unsigned digit;
    _ASSERTE(radix <= 36);
    if(initialize)
    {
        int i;
        memset(digits,255,sizeof(digits));
        for(i='0'; i <= '9'; i++) digits[i] = i - '0';
        for(i='A'; i <= 'Z'; i++) digits[i] = i + 10 - 'A';
        for(i='a'; i <= 'z'; i++) digits[i] = i + 10 - 'a';
        initialize = FALSE;
    }
    for(;;str++) 
        {
        digit = digits[*str];
        if (digit >= radix) 
                {
            *endStr = str;
            return(ret);
        }
        ret = ret * radix + digit;
    }
}

/********************************************************************************/
/* fetch the next token, and return it   Also set the yylval.union if the
   lexical token also has a value */

#define IsValidStartingSymbol(x) (isalpha((x)&0xFF)||((x)=='#')||((x)=='_')||((x)=='@')||((x)=='$'))
#define IsValidContinuingSymbol(x) (isalnum((x)&0xFF)||((x)=='_')||((x)=='@')||((x)=='$')||((x)=='?'))
char* nextchar(char* pos)
{
    return (g_uCodePage == CP_ACP) ? (char *)_mbsinc((const unsigned char *)pos) : ++pos;
}
int yylex() 
{
    char* curPos = parser->curPos;

        // Skip any leading whitespace and comments
    const unsigned eolComment = 1;
    const unsigned multiComment = 2;
    unsigned state = 0;
    for(;;) 
    {   // skip whitespace and comments
        if (curPos >= parser->limit) 
        {
            curPos = parser->fillBuff(curPos);
            if(strlen(curPos) < (unsigned)(parser->endPos - curPos))
            {
                yyerror("Not a text file");
                return 0;
            }
        }
        
        switch(*curPos) 
        {
            case 0: 
                if (state & multiComment) return (BAD_COMMENT_);
                return 0;       // EOF
            case '\n':
                state &= ~eolComment;
                parser->curLine++;
                PASM->m_ulCurLine = (bExternSource ? nExtLine : parser->curLine);
                PASM->m_ulCurColumn = (bExternSource ? nExtCol : 1);
                break;
            case '\r':
            case ' ' :
            case '\t':
            case '\f':
                break;

            case '*' :
                if(state == 0) goto PAST_WHITESPACE;
                if(state & multiComment)
                {
                    if (curPos[1] == '/') 
                    {
                        curPos++;
                        state &= ~multiComment;
                    }
                }
                break;

            case '/' :
                if(state == 0)
                {
                    if (curPos[1] == '/')  state |= eolComment;
                    else if (curPos[1] == '*') 
                    {
                        curPos++;
                        state |= multiComment;
                    }
                    else goto PAST_WHITESPACE;
                }
                break;

            default:
                if (state == 0)  goto PAST_WHITESPACE;
        }
        //curPos++;
        curPos = nextchar(curPos);
    }
PAST_WHITESPACE:

    char* curTok = curPos;
    parser->curTok = curPos;
    parser->curPos = curPos;
    int tok = ERROR_;
    yylval.string = 0;

    if(bParsingByteArray) // only hexadecimals w/o 0x, ')' and white space allowed!
    {
        int i,s=0;
        char ch;
        for(i=0; i<2; i++, curPos++)
        {
            ch = *curPos;
            if(('0' <= ch)&&(ch <= '9')) s = s*16+(ch - '0');
            else if(('A' <= ch)&&(ch <= 'F')) s = s*16+(ch - 'A' + 10);
            else if(('a' <= ch)&&(ch <= 'f')) s = s*16+(ch - 'a' + 10);
            else break; // don't increase curPos!
        }
        if(i)
        {
            tok = HEXBYTE;
            yylval.int32 = s;
        }
        else
        {
            if(ch == ')') 
            {
                bParsingByteArray = FALSE;
                goto Just_A_Character;
            }
        }
        parser->curPos = curPos;
        return(tok);
    }
    if(*curPos == '?') // '?' may be part of an identifier, if it's not followed by punctuation
    {
        if(IsValidContinuingSymbol(*(curPos+1))) goto Its_An_Id;
        goto Just_A_Character;
    }

    if (IsValidStartingSymbol(*curPos)) 
    { // is it an ID
Its_An_Id:
        size_t offsetDot = (size_t)-1; // first appearance of '.'
        size_t offsetDotDigit = (size_t)-1; // first appearance of '.<digit>' (not DOTTEDNAME!)
        do 
        {
            if (curPos >= parser->limit) 
            {
                size_t offsetInStr = curPos - curTok;
                curTok = parser->fillBuff(curTok);
                curPos = curTok + offsetInStr;
            }
            //curPos++;
            curPos = nextchar(curPos);
            if (*curPos == '.') 
            {
                if (offsetDot == (size_t)-1) offsetDot = curPos - curTok;
                curPos++;
                if((offsetDotDigit==(size_t)-1)&&(*curPos >= '0')&&(*curPos <= '9')) 
                    offsetDotDigit = curPos - curTok - 1;
            }
        } while(IsValidContinuingSymbol(*curPos));
        size_t tokLen = curPos - curTok;

        // check to see if it is a keyword
        int token = findKeyword(curTok, tokLen, &yylval.instr);
        if (token != 0) 
        {
            //printf("yylex: TOK = %d, curPos=0x%8.8X\n",token,curPos);
            parser->curPos = curPos;
            parser->curTok = curTok;
            return(token);
        }
        if(*curTok == '#') 
        {
            parser->curPos = curPos;
            parser->curTok = curTok;
            return(ERROR_);
        }
        // Not a keyword, normal identifiers don't have '.' in them
        if (offsetDot < (size_t)-1) 
        {
            if(offsetDotDigit < (size_t)-1)
            {
                curPos = curTok+offsetDotDigit;
                tokLen = offsetDotDigit;
            }
            while((*(curPos-1)=='.')&&(tokLen))
            {
                curPos--;
                tokLen--;
            }
        }

        if((yylval.string = new char[tokLen+1]))
        {
        memcpy(yylval.string, curTok, tokLen);
        yylval.string[tokLen] = 0;
        tok = (offsetDot == (size_t)(-1))? ID : DOTTEDNAME;
        //printf("yylex: ID = '%s', curPos=0x%8.8X\n",yylval.string,curPos);
    }
        else return BAD_LITERAL_;
    }
    else if (isdigit((*curPos)&0xFF) 
        || (*curPos == '.' && isdigit(curPos[1]&0xFF))
        || (*curPos == '-' && isdigit(curPos[1]&0xFF))) 
        {
        // Refill buffer, we may be close to the end, and the number may be only partially inside
        if(parser->endPos - curPos < AsmParse::IN_OVERLAP)
        {
            curTok = parser->fillBuff(curPos);
            curPos = curTok;
        }
        const char* begNum = curPos;
        unsigned radix = 10;

        bool neg = (*curPos == '-');    // always make it unsigned 
        if (neg) curPos++;

        if (curPos[0] == '0' && curPos[1] != '.') 
                {
            curPos++;
            radix = 8;
            if (*curPos == 'x' || *curPos == 'X') 
                        {
                curPos++;
                radix = 16;
            }
        }
        begNum = curPos;
        {
            unsigned __int64 i64 = str2uint64(begNum, const_cast<const char**>(&curPos), radix);
            yylval.int64 = new __int64(i64);
            tok = INT64;                    
            if (neg) *yylval.int64 = -*yylval.int64;
        }
        if (radix == 10 && ((*curPos == '.' && curPos[1] != '.') || *curPos == 'E' || *curPos == 'e')) 
                {
            yylval.float64 = new double(strtod(begNum, &curPos));
            if (neg) *yylval.float64 = -*yylval.float64;
            tok = FLOAT64;
        }
    }
    else 
    {   //      punctuation
        if (*curPos == '"' || *curPos == '\'') 
        {
            //char quote = *curPos++;
            char quote = *curPos;
            curPos = nextchar(curPos);
            char* fromPtr = curPos;
            char* prevPos;
            bool escape = false;
            BinStr* pBuf = new BinStr(); 
            for(;;) 
            {     // Find matching quote
                if (curPos >= parser->limit)
                { 
                    curTok = parser->fillBuff(curPos);
                    curPos = curTok;
                }
                
                if (*curPos == 0) { parser->curPos = curPos; delete pBuf; return(BAD_LITERAL_); }
                if (*curPos == '\r') curPos++;  //for end-of-line \r\n
                if (*curPos == '\n') 
                {
                    parser->curLine++;
                    PASM->m_ulCurLine = (bExternSource ? nExtLine : parser->curLine);
                    PASM->m_ulCurColumn = (bExternSource ? nExtCol : 1);
                    if (!escape) { parser->curPos = curPos; delete pBuf; return(BAD_LITERAL_); }
                }
                if ((*curPos == quote) && (!escape)) break;
                escape =(!escape) && (*curPos == '\\');
                //pBuf->appendInt8(*curPos++);
                prevPos = curPos;
                curPos = nextchar(curPos);
                while(prevPos < curPos) pBuf->appendInt8(*prevPos++);
            }
            //curPos++;               // skip closing quote
            curPos = nextchar(curPos);
                                
            // translate escaped characters
            unsigned tokLen = pBuf->length();
            char* toPtr = new char[tokLen+1];
            if(toPtr==NULL) return BAD_LITERAL_;
            yylval.string = toPtr;
            fromPtr = (char *)(pBuf->ptr());
            char* endPtr = fromPtr+tokLen;
            while(fromPtr < endPtr) 
            {
                if (*fromPtr == '\\') 
                {
                    fromPtr++;
                    switch(*fromPtr) 
                    {
                        case 't':
                                *toPtr++ = '\t';
                                break;
                        case 'n':
                                *toPtr++ = '\n';
                                break;
                        case 'b':
                                *toPtr++ = '\b';
                                break;
                        case 'f':
                                *toPtr++ = '\f';
                                break;
                        case 'v':
                                *toPtr++ = '\v';
                                break;
                        case '?':
                                *toPtr++ = '\?';
                                break;
                        case 'r':
                                *toPtr++ = '\r';
                                break;
                        case 'a':
                                *toPtr++ = '\a';
                                break;
                        case '\n':
                                do      fromPtr++;
                                while(isspace(*fromPtr));
                                --fromPtr;              // undo the increment below   
                                break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                                if (isdigit(fromPtr[1]&0xFF) && isdigit(fromPtr[2]&0xFF)) 
                                {
                                    *toPtr++ = ((fromPtr[0] - '0') * 8 + (fromPtr[1] - '0')) * 8 + (fromPtr[2] - '0');
                                    fromPtr+= 2;                                                            
                                }
                                else if(*fromPtr == '0') *toPtr++ = 0;
                                break;
                        default:
                                *toPtr++ = *fromPtr;
                    }
                    fromPtr++;
                }
                else
                //  *toPtr++ = *fromPtr++;
                {
                    char* tmpPtr = fromPtr;
                    fromPtr = nextchar(fromPtr);
                    while(tmpPtr < fromPtr) *toPtr++ = *tmpPtr++;
                }

            } //end while(fromPtr < endPtr)
            *toPtr = 0;                     // terminate string
            if(quote == '"')
            {
                BinStr* pBS = new BinStr();
                unsigned size = (unsigned)(toPtr - yylval.string);
                memcpy(pBS->getBuff(size),yylval.string,size);
                delete yylval.string;
                yylval.binstr = pBS;
                tok = QSTRING;
            }
            else tok = SQSTRING;
            delete pBuf;
        } // end if (*curPos == '"' || *curPos == '\'')
        else if (strncmp(curPos, "::", 2) == 0) 
        {
            curPos += 2;
            tok = DCOLON;
        }       
        else if (strncmp(curPos, "...", 3) == 0) 
        {
            curPos += 3;
            tok = ELIPSES;
        }
        else if(*curPos == '.') 
        {
            do
            {
                curPos++;
                if (curPos >= parser->limit) 
                {
                    size_t offsetInStr = curPos - curTok;
                    curTok = parser->fillBuff(curTok);
                    curPos = curTok + offsetInStr;
                }
            }
            while(isalnum(*curPos) || *curPos == '_' || *curPos == '$'|| *curPos == '@'|| *curPos == '?');
            size_t tokLen = curPos - curTok;

            // check to see if it is a keyword
            int token = findKeyword(curTok, tokLen, &yylval.instr);
            if(token)
            {
                //printf("yylex: TOK = %d, curPos=0x%8.8X\n",token,curPos);
                parser->curPos = curPos;
                parser->curTok = curTok; 
                return(token);
            }
            tok = '.';
            curPos = curTok + 1;
        }
        else 
        {
Just_A_Character:
            tok = *curPos++;
        }
        //printf("yylex: PUNCT curPos=0x%8.8X\n",curPos);
    }
    dbprintf(("    Line %d token %d (%c) val = %s\n", parser->curLine, tok, 
            (tok < 128 && isprint(tok)) ? tok : ' ', 
            (tok > 255 && tok != INT32 && tok != INT64 && tok!= FLOAT64) ? yylval.string : ""));

    parser->curPos = curPos;
    parser->curTok = curTok; 
    return(tok);
}

/**************************************************************************/
static char* newString(char* str1) 
{
    char* ret = new char[strlen(str1)+1];
    if(ret) strcpy(ret, str1);
    return(ret);
}

/**************************************************************************/
/* concatinate strings and release them */

static char* newStringWDel(char* str1, char* str2, char* str3) 
{
    size_t len = strlen(str1) + strlen(str2)+1;
    if (str3) len += strlen(str3);
    char* ret = new char[len];
    if(ret)
    {
    strcpy(ret, str1);
    delete [] str1;
    strcat(ret, str2);
    delete [] str2;
    if (str3)
    {
        strcat(ret, str3);
        delete [] str3;
    }
    }
    return(ret);
}

/**************************************************************************/
static void corEmitInt(BinStr* buff, unsigned data) 
{
    unsigned cnt = CorSigCompressData(data, buff->getBuff(5));
    buff->remove(5 - cnt);
}

/**************************************************************************/
/* move 'ptr past the exactly one type description */

static unsigned __int8* skipType(unsigned __int8* ptr) 
{
    mdToken  tk;
AGAIN:
    switch(*ptr++) {
        case ELEMENT_TYPE_VOID         :
        case ELEMENT_TYPE_BOOLEAN      :
        case ELEMENT_TYPE_CHAR         :
        case ELEMENT_TYPE_I1           :
        case ELEMENT_TYPE_U1           :
        case ELEMENT_TYPE_I2           :
        case ELEMENT_TYPE_U2           :
        case ELEMENT_TYPE_I4           :
        case ELEMENT_TYPE_U4           :
        case ELEMENT_TYPE_I8           :
        case ELEMENT_TYPE_U8           :
        case ELEMENT_TYPE_R4           :
        case ELEMENT_TYPE_R8           :
        case ELEMENT_TYPE_U            :
        case ELEMENT_TYPE_I            :
        case ELEMENT_TYPE_R            :
        case ELEMENT_TYPE_STRING       :
        case ELEMENT_TYPE_OBJECT       :
        case ELEMENT_TYPE_TYPEDBYREF   :
                /* do nothing */
                break;

        case ELEMENT_TYPE_VALUETYPE   :
        case ELEMENT_TYPE_CLASS        :
                ptr += CorSigUncompressToken(ptr, &tk);
                break;

        case ELEMENT_TYPE_CMOD_REQD    :
        case ELEMENT_TYPE_CMOD_OPT     :
                ptr += CorSigUncompressToken(ptr, &tk);
                goto AGAIN;

        /*                                                                   
                */

        case ELEMENT_TYPE_ARRAY         :
                {
                    ptr = skipType(ptr);                    // element Type
                    unsigned rank = CorSigUncompressData((PCCOR_SIGNATURE&) ptr);
                    if (rank != 0)
                    {
                        unsigned numSizes = CorSigUncompressData((PCCOR_SIGNATURE&) ptr);
                        while(numSizes > 0)
                        {
                            CorSigUncompressData((PCCOR_SIGNATURE&) ptr);
                            --numSizes;
                        }
                        unsigned numLowBounds = CorSigUncompressData((PCCOR_SIGNATURE&) ptr);
                        while(numLowBounds > 0)
                        {
                            CorSigUncompressData((PCCOR_SIGNATURE&) ptr);
                            --numLowBounds;
                        }
                    }
                }
                break;

                // Modifiers or depedant types
        case ELEMENT_TYPE_PINNED                :
        case ELEMENT_TYPE_PTR                   :
        case ELEMENT_TYPE_BYREF                 :
        case ELEMENT_TYPE_SZARRAY               :
                // tail recursion optimization
                // ptr = skipType(ptr);
                // break
                goto AGAIN;

        case ELEMENT_TYPE_VAR:
                CorSigUncompressData((PCCOR_SIGNATURE&) ptr);  // bound
                break;

        case ELEMENT_TYPE_FNPTR: 
                {
                    CorSigUncompressData((PCCOR_SIGNATURE&) ptr);    // calling convention
                    unsigned argCnt = CorSigUncompressData((PCCOR_SIGNATURE&) ptr);    // arg count
                    ptr = skipType(ptr);    // return type
                    while(argCnt > 0)
                    {
                        ptr = skipType(ptr);
                        --argCnt;
                    }
                }
                break;

        default:
        case ELEMENT_TYPE_SENTINEL              :
        case ELEMENT_TYPE_END                   :
                _ASSERTE(!"Unknown Type");
                break;
    }
    return(ptr);
}

/**************************************************************************/
static unsigned corCountArgs(BinStr* args) 
{
    unsigned __int8* ptr = args->ptr();
    unsigned __int8* end = &args->ptr()[args->length()];
    unsigned ret = 0;
    while(ptr < end) 
        {
        if (*ptr != ELEMENT_TYPE_SENTINEL)
                {
            ptr = skipType(ptr);
            ret++;
        }
        else ptr++;
    }
    return(ret);
}

/********************************************************************************/
AsmParse::AsmParse(ReadStream* aIn, Assembler *aAssem) 
{
#ifdef DEBUG_PARSING
    extern int yydebug;
    yydebug = 1;
#endif

    in = aIn;
    assem = aAssem;
    assem->SetErrorReporter((ErrorReporter *)this);

    char* buffBase = new char[IN_READ_SIZE+IN_OVERLAP+1];                // +1 for null termination
    _ASSERTE(buffBase);
    if(buffBase)
    {
    curTok = curPos = endPos = limit = buff = &buffBase[IN_OVERLAP];     // Offset it 
    curLine = 1;
    assem->m_ulCurLine = curLine;
    assem->m_ulCurColumn = 1;

    hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hstderr = GetStdHandle(STD_ERROR_HANDLE);

    success = true; 
    _ASSERTE(parser == 0);          // Should only be one parser instance at a time

    // Resolve aliases
    for (unsigned int i = 0; i < sizeof(keywords) / sizeof(Keywords); i++)
    {
        if (keywords[i].token == NO_VALUE)
            keywords[i].token = keywords[keywords[i].tokenVal].token;
    }

    // Sort the keywords for fast lookup 
    qsort(keywords, sizeof(keywords) / sizeof(Keywords), sizeof(Keywords), keywordCmp);
    parser = this;
    //yyparse();
}
    else
    {
        assem->report->error("Failed to allocate parsing buffer\n");
        delete this;
    }
}

/********************************************************************************/
AsmParse::~AsmParse() 
{
    parser = 0;
    delete [] &buff[-IN_OVERLAP];
}

/**************************************************************************/
DWORD IsItUnicode(CONST LPVOID pBuff, int cb, LPINT lpi)
{
    if(*((WORD*)pBuff) == 0xFEFF)
    {
        if(lpi) *lpi = IS_TEXT_UNICODE_SIGNATURE;
        return 1;
    }
    return 0;
}
/**************************************************************************/
char* AsmParse::fillBuff(char* pos) 
{
    static bool bUnicode = false;
    int iRead,iPutToBuffer,iOrdered;
    static char* readbuff = buff;

    _ASSERTE((buff-IN_OVERLAP) <= pos && pos <= &buff[IN_READ_SIZE]);
    curPos = pos;
    size_t tail = endPos - curPos; // endPos points just past the end of valid data in the buffer
    _ASSERTE(tail <= IN_OVERLAP);
    if(tail) memcpy(buff-tail, curPos, tail);    // Copy any stuff to the begining 
    curPos = buff-tail;
    iOrdered = m_iReadSize;
    if(m_bFirstRead)
    {
        int iOptions = IS_TEXT_UNICODE_UNICODE_MASK;
        m_bFirstRead = false;
        g_uCodePage = CP_ACP;
        if(bUnicode) // leftover fron previous source file
        {
            delete [] readbuff;
            readbuff = buff;
        }
        bUnicode = false;

        iRead = in->read(readbuff, iOrdered);

        if(IsItUnicode(buff,iRead,&iOptions))
        {
            bUnicode = true;
            g_uCodePage = CP_UTF8;
            if((readbuff = new char[iOrdered+2])) // buffer for reading Unicode chars
            {
            if(iOptions & IS_TEXT_UNICODE_SIGNATURE)
                memcpy(readbuff,buff+2,iRead-2);   // only first time, next time it will be read into new buffer
            else
                memcpy(readbuff,buff,iRead);   // only first time, next time it will be read into new buffer
            printf("Source file is UNICODE\n\n");
        }
        else
                assem->report->error("Failed to allocate read buffer\n");
        }
        else
        {
            m_iReadSize = IN_READ_SIZE;
            if(((buff[0]&0xFF)==0xEF)&&((buff[1]&0xFF)==0xBB)&&((buff[2]&0xFF)==0xBF))
            {
                g_uCodePage = CP_UTF8;
                curPos += 3;
                printf("Source file is UTF-8\n\n");
            }
            else
                printf("Source file is ANSI\n\n");
        }
    }
    else  iRead = in->read(readbuff, iOrdered);

    if(bUnicode)
    {
        WCHAR* pwc = (WCHAR*)readbuff;
        pwc[iRead/2] = 0;
        memset(buff,0,IN_READ_SIZE);
        WszWideCharToMultiByte(CP_UTF8,0,pwc,-1,(LPSTR)buff,IN_READ_SIZE,NULL,NULL);
        iPutToBuffer = (int)strlen(buff);
    }
    else iPutToBuffer = iRead;

    endPos = buff + iPutToBuffer;
    *endPos = 0;                        // null Terminate the buffer

    limit = endPos; // endPos points just past the end of valid data in the buffer
    if (iRead == iOrdered) 
    {
        limit-=4; // max look-ahead without reloading - 3 (checking for "...")
    }
    return(curPos);
}

/********************************************************************************/
BinStr* AsmParse::MakeSig(unsigned callConv, BinStr* retType, BinStr* args) 
{
    BinStr* ret = new BinStr();
    if(ret)
    {
    //if (retType != 0) 
            ret->insertInt8(callConv); 
    corEmitInt(ret, corCountArgs(args));

    if (retType != 0) 
        {
        ret->append(retType); 
        delete retType;
    }
    ret->append(args); 
    }
    else
        assem->report->error("\nOut of memory!\n");

    delete args;
    return(ret);
}

/********************************************************************************/
BinStr* AsmParse::MakeTypeArray(BinStr* elemType, BinStr* bounds) 
{
    // 'bounds' is a binary buffer, that contains an array of 'struct Bounds' 
    struct Bounds {
        int lowerBound;
        unsigned numElements;
    };

    _ASSERTE(bounds->length() % sizeof(Bounds) == 0);
    unsigned boundsLen = bounds->length() / sizeof(Bounds);
    _ASSERTE(boundsLen > 0);
    Bounds* boundsArr = (Bounds*) bounds->ptr();

    BinStr* ret = new BinStr();

    ret->appendInt8(ELEMENT_TYPE_ARRAY);
    ret->append(elemType);
    corEmitInt(ret, boundsLen);                     // emit the rank

    unsigned lowerBoundsDefined = 0;
    unsigned numElementsDefined = 0;
    unsigned i;
    for(i=0; i < boundsLen; i++) 
    {
        if(boundsArr[i].lowerBound < 0x7FFFFFFF) lowerBoundsDefined = i+1;
        else boundsArr[i].lowerBound = 0;

        if(boundsArr[i].numElements < 0x7FFFFFFF) numElementsDefined = i+1;
        else boundsArr[i].numElements = 0;
    }

    corEmitInt(ret, numElementsDefined);                    // emit number of bounds

    for(i=0; i < numElementsDefined; i++) 
    {
        _ASSERTE (boundsArr[i].numElements >= 0);               // enforced at rule time
        corEmitInt(ret, boundsArr[i].numElements);

    }

    corEmitInt(ret, lowerBoundsDefined);    // emit number of lower bounds
    for(i=0; i < lowerBoundsDefined; i++)
    {
        unsigned cnt = CorSigCompressSignedInt(boundsArr[i].lowerBound, ret->getBuff(5));
        ret->remove(5 - cnt);
    }
    delete elemType;
    delete bounds;
    return(ret);
}

/********************************************************************************/
BinStr* AsmParse::MakeTypeClass(CorElementType kind, char* name) 
{

    BinStr* ret = new BinStr();
    _ASSERTE(kind == ELEMENT_TYPE_CLASS || kind == ELEMENT_TYPE_VALUETYPE ||
                     kind == ELEMENT_TYPE_CMOD_REQD || kind == ELEMENT_TYPE_CMOD_OPT);
    ret->appendInt8(kind);
    mdToken tk = PASM->ResolveClassRef(name,NULL);
    unsigned cnt = CorSigCompressToken(tk, ret->getBuff(5));
    ret->remove(5 - cnt);
    delete [] name;
    return(ret);
}
/**************************************************************************/
void PrintANSILine(FILE* pF, char* sz)
{
    WCHAR wz[4096];
    if(g_uCodePage != CP_ACP)
    {
        memset(wz,0,sizeof(WCHAR)*4096);
        WszMultiByteToWideChar(g_uCodePage,0,sz,-1,wz,4096);

        memset(sz,0,4096);
        WszWideCharToMultiByte(CP_ACP,0,wz,-1,sz,4096,NULL,NULL);
    }
    fprintf(pF,sz);
}
/**************************************************************************/
void AsmParse::error(const char* fmt, ...)
{
    char sz[4096], *psz=&sz[0];
    success = false;
    va_list args;
    va_start(args, fmt);

    if(in) psz+=sprintf(psz, "%s(%d) : ", in->name(), curLine);
    psz+=sprintf(psz, "error -- ");
    vsprintf(psz, fmt, args);
    PrintANSILine(stderr,sz);
}

/**************************************************************************/
void AsmParse::warn(const char* fmt, ...)
{
    char sz[4096], *psz=&sz[0];
    va_list args;
    va_start(args, fmt);

    if(in) psz+=sprintf(psz, "%s(%d) : ", in->name(), curLine);
    psz+=sprintf(psz, "warning -- ");
    vsprintf(psz, fmt, args);
    PrintANSILine(stderr,sz);
}
/**************************************************************************/
void AsmParse::msg(const char* fmt, ...)
{
    char sz[4096];
    va_list args;
    va_start(args, fmt);

    vsprintf(sz, fmt, args);
    PrintANSILine(stdout,sz);
}


/**************************************************************************/
/*
#include <stdio.h>

int main(int argc, char* argv[]) {
    printf ("Beginning\n");
    if (argc != 2)
        return -1;

    FileReadStream in(argv[1]);
    if (!in) {
        printf("Could not open %s\n", argv[1]);
        return(-1);
        }

    Assembler assem;
    AsmParse parser(&in, &assem);
    printf ("Done\n");
    return (0);
}
*/

//#undef __cplusplus
YYSTATIC YYCONST short yyexca[] = {
#if !(YYOPTTIME)
-1, 1,
#endif
    0, -1,
    -2, 0,
#if !(YYOPTTIME)
-1, 325,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 498,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 519,
#endif
    268, 340,
    46, 340,
    47, 340,
    -2, 321,
#if !(YYOPTTIME)
-1, 596,
#endif
    268, 340,
    46, 340,
    47, 340,
    -2, 107,
#if !(YYOPTTIME)
-1, 598,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 607,
#endif
    123, 113,
    -2, 340,
#if !(YYOPTTIME)
-1, 631,
#endif
    40, 182,
    -2, 346,
#if !(YYOPTTIME)
-1, 635,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 780,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 789,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 867,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 871,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 873,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 938,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 949,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 969,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1016,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1018,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1020,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1022,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1024,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1026,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1028,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1056,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1085,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1087,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1089,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1091,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1093,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1095,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1097,
#endif
    41, 331,
    -2, 183,
#if !(YYOPTTIME)
-1, 1109,
#endif
    41, 331,
    -2, 183,

};

# define YYNPROD 602
#if YYOPTTIME
YYSTATIC YYCONST yyexind_t yyexcaind[] = {
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    4,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    8,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   12,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   20,    0,   28,    0,
    0,    0,    0,    0,    0,    0,    0,   32,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   36,    0,    0,    0,   40,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   44,    0,    0,    0,    0,    0,    0,    0,    0,   48,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   52,    0,    0,
    0,   56,    0,   60,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   64,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   68,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   72,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   76,    0,   80,    0,
   84,    0,   88,    0,   92,    0,   96,    0,  100,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  104,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  108,    0,  112,    0,  116,
    0,  120,    0,  124,    0,  128,    0,  132,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  136
};
#endif
# define YYLAST 2389

YYSTATIC YYCONST short YYFARDATA YYACT[] = {
  493,  577,  773, 1002,  678,  400,  267,  559,  575,  260,
  113,  854,  280,   83,   75,  582,  523,  495,    6,    5,
  348,  620,   56,    3,  174,  257,  259,  142,  918,   23,
  140,  882,   17,  127,  915,   58,  803,  141,   62,   10,
   54,  850,  505,  848,  111,  849,   54,  545,   92,  372,
  137,  110,    7,  120,  121,  285,  804,   90,   65,   64,
   61,   66,  530,  388,   65,   64,  143,   66,   18,  222,
  177,  401,  325,   65,   64,  452,   66,  385,  201,  453,
   63,  916,  452,  772,   65,   64,  453,   66,  743,  226,
  520,  147,  395,  124,  332,  200,   65,   64,  200,   66,
  859,  860,  247,  251,  252,  580,  255,   65,   64,  764,
   66,  199,  390,  677,  776,  775,  200,  403,  529,  287,
   48,  504,  550,  551,  552,  290,  291,  528,  451,  139,
  548,  460,  722,  723,  350,  451,  383,  361,  469,  468,
  289,  179,  459,  170,  470,  592,  308,  304,  351,  352,
  256,  305,  553,  554,  555,  317,   23,   82,  310,   17,
  721,  805,  496,  359,  733,  311,   10,   59,  350,  356,
  353,  287, 1078,  362, 1027, 1025,  360, 1023,  309,    7,
 1021,  228,  351,  352,  452,   54,  357, 1019,  453,  583,
  346,  349,  452, 1017,  312,   18,  738,  525, 1015,  870,
  621,  622,  330,  375,  299,  340,  747,  338,  342,  341,
  779,  369,  410,  414,   54,  742,  358,   53,   55,  292,
  632,  293,  294,  295,  964,  965,  966,  542,  605,   87,
   88,  597,  501,  344,  745,   85,  242,  451,  355,  381,
  381,  394,  399,   54,  363,  451,  911,  417,  438,  370,
   59,  440,  589,  361,  176,  659,  660,  661,  450,  231,
  233,  235,  237,  239,  248,  249,  250,  221,  441,  573,
   93,  449,   65,   64,  178,   66,  633,   54,  463,   65,
   64,  583,   66,   65,  243,  471,   66,  458,  368,  724,
  725,  461,  404,  405,  406,  112,  909,   65,  474,  117,
   66,  118,  534,  756,   65,   64,  436,   66,  119,  479,
  245,   65,   64,  361,   66,  475,   54,  122,  506,  246,
  869,  433,  407,  408,  409,  114,  472,  488,  856,  323,
  485,  324,   36,   45,  412,  411,  497,   65,   64,  115,
   66,  413,  484,  329,  483,  476,  464,  465,  466,  467,
  343,  345,   38,  376,   52,  354,  580,   51,  859,  860,
  347,  364,   50,  374,  510,  382,  371,   49,  378,  379,
  393,   47,  489,  393,  521,  652,   46, 1057,  454,  455,
  456,  533,  511,  517,  284,  454,  455,  456,  200,  522,
 1008,  531,  253,  254,  535,   38,  954,  621,  622,  537,
 1007,  538,  732, 1005,  397,  541,   65,  398,  962,   66,
  544,  147,   38,  518,  117,   38,  118,  663,  558,  956,
  512,  392,  447,  119,  392,  482,  459,  439,  503,  549,
  794,  793,  442,  492,  443,  866,  444,  563,  519,   38,
  114,  612,  556,  446,  401,  792,  796,  524,  914,  139,
  388,  731,  382,  389,  115,  378,  379,  507,  508,  509,
  574,  326,   65,  579,  462,   66,  450,   54,   54,  955,
  587,  791,  588,  923,  919,  920,  921,  922,  546,  611,
  749,  750,  751,  752,  624,  618,  590,  454,  455,  456,
  591,  449,  917,  770,  566,  454,  455,  456,  560,  617,
  658,  594,  610,  361,  562,  613,   31,   32,   41,   38,
  200,  486,  487,  604,  623,  244,  170,  601,  593,  810,
  806,  807,  808,  809,  616,  147,  499,  386,   54,  502,
  631,  595,  583,   65,   64,  200,   66,  513,  415,  630,
  445,  117,  361,  118,  432,  200,  606,  913,  361,  365,
  119,  628,  229,  331,  328,  651,  596,  736,  653,   86,
  654,   69,  238,  139,  236,  526,  664,  114,  711,  623,
  774,  607,  665,  629,  656,  739,  532,  540,  657,  746,
  760,  115,  730,  758,  759,   59,  350,  625,  729,  539,
  520,  543,  578,  988,  800,   65,   64,  234,   66,  744,
  351,  352,  737,   31,   32,   41,   38,  631,  728,  639,
  452,  734,  735,  229,  453,  229,  741,  536,  623,  480,
  308,  304,  953,  800,  452,  305,  635,  765,  453,  317,
  755,  763,  310,  768,  301,  232,  782,  767,  230,  311,
  753,  762,  672,  801,  673,  674,  675, 1012,  229,  584,
  740,  227,  309,   65,  783,  784,   66,  283,   54,  799,
  800,  125, 1011,  451, 1010,  795, 1001,  903,  312,  754,
  761,  394,  801,  643,  500,  327,  584,  451,    1,  797,
  457,  200, 1120,  667,  668,  669,  229,  766, 1059,  229,
  520, 1058,  672,   94,  673,  674,  675,  769,  619,  520,
 1101, 1121,  229,  520,  626,  520,  958,  788,  727,  801,
  638,  637,  627,  781,  714,  615,  520,  520,  524,  524,
  599,  434,  666,  670,  671,  298,  128,  640,  641,  224,
  857, 1122, 1118,  667,  668,  669,  852, 1117, 1116,  126,
  861,  868, 1115, 1114, 1113, 1112,  729,  715,  862,  716,
  717,  718,  719,  720,  858,  662, 1100, 1098,  798, 1096,
  875,  876,  877,  878, 1094, 1092, 1090, 1088, 1086,  879,
  880,  881,  666,  670,  671, 1084,  895, 1083, 1082, 1063,
 1042,  901, 1041, 1040, 1039, 1038, 1037, 1036, 1035,  896,
  906, 1034,  623,  904, 1033,  907, 1032,  711, 1030, 1013,
 1009,  900,  851,  908,  912,  999,  971,  970,  968,  952,
  950,  902,  874,  865,  905,  864,  787,  778,  672,  777,
  673,  674,  675,  872,   95,   96,   97,   98,   99,  100,
  101,  102,  103,  104,  105,  106,  107,  108,  109,  824,
  318,  785,   42,   27,   43,  771,  726,  648,  883,  647,
  786,  645,  319,  885, 1049,  322,  898, 1109,  320,  667,
  668,  669,   36,   45,  642,  636,  634,  614,  926,  571,
 1045,  570,  928,  569,  929,  568,  861,  567, 1054,   31,
   32,   41,   38,  623,  313,  314,  565,  564,  516,  939,
  473,  711,  927,  937,  437, 1055,  297,  296,  666,  670,
  671,  282,  930,  931,  932,  933,  934,  935,  936,  321,
  948, 1044,  241,  454,  455,  456, 1097,  961, 1046, 1047,
 1048, 1050, 1051, 1052, 1053, 1095,  855,  454,  455,  456,
  584,  863, 1093, 1091, 1089, 1087, 1085, 1056, 1028,  987,
  316, 1026, 1024,  631,  631,  631,  631,  631,  631,  631,
 1000, 1022,  973,  975,  977,  979,  981,  983,  985,  998,
 1006,  989,  991, 1020,  986, 1018,  623, 1016,  969,  949,
 1014,  947,  946,  396,  899, 1003,  990,  992,  993,  994,
  995,  996,  997,  945,  944,  943,  972,  974,  976,  978,
  980,  982,  984,  361,  942,  941,  940,  938,  925,  910,
  924,  884,  894,  892,  873,  871,  893,  891,  890,  889,
  867,  887,  888,   82,  802,  790,  897, 1065,  789, 1067,
  780, 1069,  650, 1071,  649, 1073,  646, 1075,  623, 1077,
  623,  644,  623, 1079,  623,  598,  623, 1064,  623, 1066,
  623, 1068, 1029, 1070,  586, 1072,  585, 1074,  561, 1076,
  515,  514,  498,  481,  834,  448,  170, 1080,  600,  435,
 1081,  416,  366,  300,  240,  225,  391,  815,  816, 1031,
  823,  833,  817,  818,  819,  820,  223,  821,  822,  384,
  387,  380,  377, 1049,  373,  123, 1102,   70, 1103,   28,
 1104,  623, 1105,  339, 1106,  170, 1107,  527, 1108, 1045,
 1099,  951,  337, 1111, 1110,   72,  336, 1054,  335,  957,
 1119,  959,  960,  334,  168,  333,   24,   25,   42,   27,
   43,  149,  963,  967, 1055,  167,  138,  132,  130,  134,
   26,  757,  748,  315,  603,  307,  602,  306,   36,   45,
  303,  655,  547,  402,   29,   16,  175, 1046, 1047, 1048,
 1050, 1051, 1052, 1053, 1004,   31,   32,   41,   38,   15,
   14,  173,   44,   30,   13,  172,   12,   21,   11,    9,
   33,    8,   74,    4,    2,  164,  156,   34,   91,   89,
   57,   37,  576,  491,   35,  490,  220,   67,   60,  853,
  886,   84,  367,   65,  581,  494,   66,   68,   74, 1043,
  811,  572,   24,   25,   42,   27,   43,  286, 1060, 1061,
 1062,   40,   39,  116,  676,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   36,   45,    0,    0,    0,    0,
    0,    0,   65,   19,   20,   66,   22,  170,    0,  129,
    0,   31,   32,   41,   38,    0,    0,    0,   44,   30,
    0,    0,    0,   21,    0,    0,   33,  813,  814,    0,
  825,  826,  827,   34,  828,  829,    0,    0,  830,  831,
   35,  832,  302,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   71,    0,  812,  835,  836,  837,  838,
  839,  840,  841,  842,  843,  844,  845,  846,  847,    0,
    0,    0,    0,    0,  151,  152,  153,  154,  155,  157,
  158,  159,  160,  161,  162,  163,  169,  165,  166,   19,
   20,    0,   22,   43,  131,  171,  133,  150,  135,  136,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   36,   45,  151,  152,  153,  154,  155,  157,  158,
  159,  160,  161,  162,  163,  169,  165,  166,   31,   32,
   41,   38,   43,  131,  171,  133,  150,  135,  136,  145,
    0,    0,    0,    0,   65,    0,    0,   66,    0,    0,
   36,   45,    0,    0,    0,    0,    0,    0,  144,    0,
    0,    0,    0,    0,    0,    0,    0,   31,   32,   41,
   38,    0,    0,    0,    0,    0,   73,    0,  145,   81,
   80,   79,   78,    0,   76,   77,   82,    0,  148,  146,
    0,    0,    0,    0,    0,    0,    0,  144,    0,    0,
    0,    0,   73,    0,  266,   81,   80,   79,   78,    0,
   76,   77,   82,    0,    0,    0,    0,    0,  692,    0,
    0,    0,    0,    0,    0,    0,    0,  148,  146,    0,
    0,  684,  685,    0,  693,  706,  686,  687,  688,  689,
    0,  690,  691,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  151,  152,  153,  154,  155,
  157,  158,  159,  160,  161,  162,  163,  169,  165,  166,
    0,    0,    0,    0,   43,  131,  171,  133,  150,  135,
  136,  704,    0,  707,    0,    0,    0,    0,    0,  709,
    0,    0,   36,   45,    0,    0,    0,    0,    0,    0,
  281,    0,    0,  318,    0,   42,   27,   43,  266,   31,
   32,   41,   38,  452,    0,  319,    0,  453,    0,    0,
  145,  320,    0,    0,    0,   36,   45,  117,    0,  118,
    0,  712,    0,    0,    0,    0,  119,    0,    0,  144,
    0,    0,   31,   32,   41,   38,    0,  313,  314,    0,
    0,    0,    0,  114,  207,  202,  203,  204,  205,  206,
    0,    0,    0,    0,  209,    0,  478,  115,    0,  148,
  146,    0,  321,    0,  208,    0,    0,    0,  217,    0,
    0,    0,    0,    0,    0,    0,  210,  211,  212,  213,
  214,  215,  216,  219,    0,  266,    0,    0,    0,  218,
  452,    0,    0,  316,  738,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  679,    0,
  680,  681,  682,  683,  694,  695,  696,  710,  697,  698,
  699,  700,  701,  702,  703,  705,  708,    0,    0,    0,
  713,  270,  271,  269,  278,    0,  272,  273,  274,  275,
    0,  276,  277,  478,  262,  263,    0,    0,    0,    0,
    0,    0,    0,    0,  261,  268,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  264,  265,  279,    0,  424,  418,  419,  420,  421,    0,
    0,  266,    0,    0,    0,    0,  452,    0,    0,    0,
  453,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  281,   65,    0,    0,   66,    0,  426,  427,  428,  429,
    0,    0,  423,    0,    0,    0,  430,  431,  422,    0,
    0,    0,    0,    0,    0,   65,   64,    0,   66,    0,
    0,    0,    0,  266,    0,  270,  271,  269,  278,  478,
  272,  273,  274,  275,    0,  276,  277,    0,  262,  263,
  182,    0,    0,    0,  197,    0,  180,  181,  261,  268,
    0,  184,  185,  195,  186,  187,  188,  189,    0,  190,
  191,  192,  193,  183,  264,  265,  279,    0,    0,  196,
    0,    0,    0,    0,    0,  194,    0,    0,    0,    0,
    0,  258,  198,  266,    0,    0,    0,    0,  452,    0,
    0,    0,  453,    0,  281,    0,  454,  455,  456,    0,
    0,    0,   65,   64,    0,   66,    0,  425,  621,  622,
    0,    0,  270,  271,  269,  278,    0,  272,  273,  274,
  275,    0,  276,  277,    0,  262,  263,    0,    0,    0,
    0,    0,    0,    0,    0,  261,  268,    0,    0,    0,
    0,  478,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  264,  265,  279,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  266,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  281,    0,  454,  455,  456,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  621,  622,    0,   65,   64,
    0,   66,    0,    0,    0,    0,    0,    0,  270,  271,
  269,  278,    0,  272,  273,  274,  275,    0,  276,  277,
  266,  262,  263,    0,    0,  258,    0,    0,    0,    0,
    0,  261,  268,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  264,  265,  279,
   65,   64,    0,   66,    0,    0,    0,    0,    0,    0,
  270,  271,  269,  278,    0,  272,  273,  274,  275,    0,
  276,  277,    0,  262,  263,    0,    0,  281,  258,  454,
  455,  456,    0,  261,  268,    0,    0,    0,    0,    0,
  266,  477,    0,    0,    0,    0,    0,    0,    0,  264,
  265,  279,    0,  609,    0,    0,    0,    0,    0,    0,
   65,   64,    0,   66,    0,    0,    0,    0,    0,    0,
  270,  271,  269,  278,    0,  272,  273,  274,  275,  281,
  276,  277,  266,  262,  263,    0,    0,    0,    0,    0,
    0,  608,    0,  261,  268,    0,    0,    0,  557,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  264,
  265,  279,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  281,
    0,  454,  455,  456,   65,   64,    0,   66,    0,    0,
    0,    0,    0,    0,  270,  271,  269,  278,    0,  272,
  273,  274,  275,    0,  276,  277,    0,  262,  263,    0,
    0,    0,    0,    0,    0,    0,    0,  261,  268,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  264,  265,  279,    0,   65,   64,    0,
   66,    0,    0,    0,    0,    0,    0,  270,  271,  269,
  278,    0,  272,  273,  274,  275,    0,  276,  277,    0,
  262,  263,    0,  281,  288,    0,    0,    0,    0,    0,
  261,  268,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  264,  265,  279,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  281,  270,  271,  269,
  278,    0,  272,  273,  274,  275,    0,  276,  277,    0,
  262,  263,    0,    0,    0,    0,    0,    0,    0,    0,
  261,  268,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  264,  265,  279,  270,
  271,  269,  278,    0,  272,  273,  274,  275,    0,  276,
  277,    0,  262,  263,    0,    0,    0,    0,    0,    0,
    0,    0,  261,  268,    0,    0,  281,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  264,  265,
  279,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  281
};

YYSTATIC YYCONST short YYFARDATA YYPACT[] = {
-1000,  816,-1000,  253,  248,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,  244,  239,  234,  231,-1000,-1000,-1000,  -22,
  -22, -483,  -15,-1000, -386,  273,-1000,  470, 1160,  -32,
  468,  -22,  -22, -389,-1000, -176,  414,  -32,  255,  -32,
  -32,   54,-1000, -213,  600,  414,-1000,-1000, 1114,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,  -22, -164,-1000,-1000,
 1481,-1000,  635,-1000,-1000,-1000,-1000, 1271,-1000,  -22,
-1000, 1134,-1000,  687, 1025,  -32,  611,  598,  595,  557,
  524,  522, 1024,  871,  -31,-1000,  -22,  252,   57, -187,
  273,   77,  635,  273, 1937,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
  860,  596, 1884, 2049,   13,   13,-1000,-1000,-1000,  -68,
  856,  855,  681,   23,-1000, 1023,  573, 1147,  730,-1000,
-1000,  -22,-1000,  -22,   32,-1000,-1000,-1000,-1000,  617,
-1000,-1000,-1000,-1000,  463,  -22, 1937,-1000,  462,  -92,
-1000,-1000,  202,  -22,  -15,  320,  -32,  202,   13, 2049,
 1937, -125,   13,  202, 1884, 1022,-1000,-1000,  393,-1000,
-1000,-1000,  -76,   11,  -13,  -33,-1000,   49,-1000, -185,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,  -23,-1000,-1000,-1000,   19,
  273,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000, 1021,
 1401,  451,  196,  677, 1019,   23,  853, -132,-1000,  -22,
 -132,-1000,  -15,-1000,  -22,-1000,  -22,-1000,  -22,-1000,
-1000,-1000,-1000,  447,-1000,  -22,-1000,  635,-1000,-1000,
-1000,   52,  635,-1000,-1000,  635, 1015,-1000, -196,  572,
  633,  335,-1000,-1000, -162,  335,  -22,   13,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,   71, -135,
  635,-1000,-1000,  286,  849,-1000,-1000,   13, 2049, 1688,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,   46,  558,
-1000, 1013,-1000,-1000,-1000,  221,  219,  207,-1000,-1000,
-1000,-1000,-1000,  -22,  -22,  204, 1937,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000, -107, 1012,-1000,  -22,  616,
  -36,  -22,-1000,  -92,   20,   20,   20,   20,  335,  393,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000, 1011, 1010,  847,-1000,-1000, 2049, 1800,-1000,  674,
  -32,-1000, 2049,-1000,-1000,-1000,  202,  -22,  972,-1000,
 -177, -186,-1000,-1000, -381,-1000,-1000,  -32,  -22,  241,
  -32,-1000,  556,-1000,-1000,  -32,-1000,  -32,  528,  516,
-1000,-1000,  273, -219,-1000,-1000,-1000,  273, -399,-1000,
 -376,-1000, -173,  335,-1000,-1000,-1000,-1000,-1000,-1000,
  635,-1000,-1000, -193,  635, 2007,   -9,  146,-1000,-1000,
-1000,-1000,-1000,-1000,-1000, 1008,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000, 1134,   -9,  846,-1000,  845,  401,
  836,  834,  832,  830,  828,-1000,    6,  273,   -9,  499,
  273,  263,-1000,-1000,-1000, 1006, 1004,  273,-1000, -202,
  335,-1000,-1000, 2049,-1000,-1000,-1000,-1000,-1000, -129,
-1000,  674,-1000,   13, 2049, 1800,  -37,  995,   12,  676,
-1000,-1000,  933,-1000,-1000,-1000,-1000,-1000,-1000,  -40,
 1740,  128,   19,  826,  671,-1000,-1000, 2007, -107,  392,
  -22, -153,  391,-1000,-1000,-1000,  202,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,  -22,  -15,-1000, 1505,  -48,-1000,
   14,  825,  586,  824,  667,  666,-1000,-1000,   23,  -22,
  -22,  823,  615,  674,  991,  810,  986,  808,  806,  984,
  982,  635,  273,-1000,   70,  273,  -32,-1000,  335,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,   44, -225,   47,  531,
 -192, 1191,-1000,  673,-1000,  461,-1000,  461,  461,  461,
  461,  461, -145,-1000,  635,  805,  664,  547,  273,  489,
-1000,  358,-1000,-1000, -105,  335,  335,  635,  464,  273,
-1000,  154,-1000,  514, 1592,  -53,-1000, -265, -107,  -29,
-1000,  454,   81,  178,  -16, -153,   23,-1000,-1000,-1000,
 2049,-1000,-1000,  635,-1000, -107,   37,  804, -287,-1000,
-1000,-1000,-1000,  635,  509, -189, -190,  778,  776,  -58,
  980,  635,   23,-1000,-1000, -107,-1000,  202,  202,-1000,
-1000,-1000,-1000,  -22,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,  635,  -22,  635,  775,  663,-1000,  978,  975,  378,
  352,  338,  337,   -9,  405,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,  509,   23,  618,  974,
 -422, -110,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,  245,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,  797,-1000, -430,-1000, -420,
-1000,-1000, -436,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,   23,-1000,-1000,-1000,-1000,-1000,   -9,   51,  633,
  273,-1000,  -80,  -22,  774,  772,  273,  342,  970,  280,
  -69,  965,   23,  964,  771,-1000,-1000,-1000,-1000,   13,
   13,   13,   13,-1000,-1000,-1000,-1000,-1000,   13,   13,
   13,-1000,-1000,-1000,-1000, -456,-1000,  146,-1000,-1000,
  961,-1000,   23,-1000,  731,   23,  -22,-1000,-1000, -153,
 -107,-1000,  770,-1000,-1000,  609,-1000, -324,  335, -107,
 1191,-1000,-1000,-1000,-1000,  674,-1000,-1000,-1000,-1000,
-1000,  203,   -9,  456,  357,-1000,-1000,-1000,-1000,-1000,
-1000,  -10,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,  199,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,  960,  674,  958,-1000,
-1000,  633,-1000,-1000,-1000,-1000,  273, -107,  674,-1000,
 -153, -107,-1000, -107,-1000, 2049, 2049, 2049, 2049, 2049,
 2049, 2049,   13,  957, 1191,-1000,-1000,  956,  955,  954,
  945,  944,  943,  932,  931,  674,  -32,-1000,-1000,-1000,
  929,  769,-1000,  -22,-1000,-1000,  768,  581,  355,-1000,
  376,  -22,  662,  -22,  -22,   -9,  315,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,  -22,  -51,  767,  928,  766,  765,
 1505, 1505, 1505, 1505, 1505, 1505, 1505, 2049, -107,  552,
  -98,  -98,  -15,  -15,  -15,  -15,  -15, -207,  764, -107,
-1000,  608,-1000, -153,-1000,-1000,  -22,  310,   -9,  307,
  297,  674,-1000,  759,  606,  604,  589,  758,-1000, -107,
-1000,-1000,  -70,  927,  -75,  925,  -81,  923,  -88,  911,
  -91,  902,  -93,  901,  -94,  898, 1800,  757,   23,  755,
  753,  750,  747,  746,  745,  744,  743,  742,  741,-1000,
  739,  -22,  788,  897,  284,-1000,  647,-1000,-1000,-1000,
  -22,  -22,  -22,-1000,  738, -153, -107, -153, -107, -153,
 -107, -153, -107, -153, -107, -153, -107, -153, -107,  -96,
  509,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000, -107,-1000,   -9,-1000,
  737,  736,  734,-1000,  896,  727,  895,  726,  894,  725,
  893,  724,  892,  723,  885,  718,  876,  716, -153,-1000,
  715,  656,-1000,-1000,-1000, -107,-1000, -107,-1000, -107,
-1000, -107,-1000, -107,-1000, -107,-1000, -107,-1000,  817,
-1000,   -9,  704,  703,  702,  701,  697,  696,  691, -107,
  559,  660,-1000,-1000,-1000,-1000,-1000,-1000,-1000,  690,
-1000,-1000,-1000
};

YYSTATIC YYCONST short YYFARDATA YYPGO[] = {
    0,   12,   80,   25,   21, 1214,    9,   16,   10, 1213,
  197, 1212, 1211,   44,  384, 1207, 1201,  336,  181, 1200,
 1197,   11,   20,   35,    0, 1195,   17,   26,   15, 1194,
 1192,   55,   13, 1191, 1190,    4,    2,    1, 1189, 1188,
 1187, 1186,    3, 1185, 1183,    7,    8, 1182,  693, 1181,
 1180,    5, 1179,  111, 1178, 1176, 1175,  678, 1174,   23,
   33, 1173,   19,  120,   18,   50, 1171, 1169,   37, 1168,
 1166, 1165, 1164, 1161, 1160,   24, 1159, 1146, 1145,   30,
   66,   27, 1144, 1143, 1142, 1141, 1140, 1137, 1136, 1135,
 1134, 1133,    6, 1132, 1131, 1130, 1129, 1128, 1127, 1126,
   42, 1125, 1121,   94, 1115, 1114, 1113,  121, 1108, 1106,
 1102, 1093, 1089, 1087, 1085,   69, 1076,   14,   77, 1084,
  353, 1082, 1081, 1080, 1079, 1066,  973
};
YYSTATIC YYCONST yyr_t YYFARDATA YYR1[]={

   0,  57,  57,  58,  58,  58,  58,  58,  58,  58,
  58,  58,  58,  58,  58,  58,  58,  58,  58,  58,
  58,  58,  58,  58,  37,  37,  81,  81,  81,  80,
  80,  80,  80,  80,  80,  78,  78,  78,  67,  16,
  16,  16,  16,  16,  66,  82,  61,  59,  39,  39,
  39,  39,  39,  39,  39,  39,  39,  39,  39,  39,
  39,  39,  39,  39,  39,  39,  39,  39,  39,  39,
  39,  39,  39,  83,  83,  84,  84,  85,  85,  60,
  60,  86,  86,  86,  86,  86,  86,  86,  86,  86,
  86,  86,  86,  86,  86,  64,   5,   5,  36,  36,
  20,  20,  11,  12,  15,  15,  15,  15,  13,  13,
  14,  14,  87,  87,  43,  43,  43,  88,  88,  93,
  93,  93,  93,  93,  93,  93,  93,  93,  93,  93,
  89,  44,  44,  44,  90,  90,  94,  94,  94,  94,
  94,  94,  94,  94,  94,  95,  62,  62,  40,  40,
  40,  40,  40,  40,  40,  40,  40,  40,  40,  40,
  40,  40,  40,  40,  40,  40,  40,  40,  40,  45,
  45,  45,  45,  45,  45,  45,  45,  45,  45,  45,
   4,   4,   4,  17,  17,  17,  17,  17,  41,  41,
  41,  41,  41,  41,  41,  41,  41,  41,  41,  41,
  41,  41,  41,  42,  42,  42,  42,  42,  42,  42,
  42,  42,  42,  42,  42,  96,  97,  97,  97,  97,
  97,  97,  97,  97,  97,  97,  97,  97,  97,  97,
  97,  97,  97,  97,  97,  97, 100, 101,  98, 103,
 103, 102, 102, 102, 105, 104, 104, 104, 104, 108,
 108, 108, 111, 106, 109, 110, 107, 107, 107,  63,
  63,  65, 112, 112, 114, 114, 113, 113, 115, 115,
  18,  18, 116, 116, 116, 116, 116, 116, 116, 116,
 116, 116, 116, 116, 116, 116, 116,  34,  34,  34,
  34,  34,  34,  34,  34,  34,  34,  34,  34,  34,
 117,  32,  32,  33,  33,  55,  56,  92,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,  99,  99,
  99,  24,  24,  25,  25,  26,  26,  26,  26,  26,
   1,   1,   1,   3,   3,   3,   6,   6,  31,  31,
  31,  31,   8,   8,   8,   9,   9,   9,   9,   9,
   9,   9,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  19,  19,  19,  19,  19,  19,  19,  19,  19,
  19,  19,  19,  19,  19,  19,  19,  19,  19,  19,
  19,  19,  19,  19,  19,  19,  19,  19,  19,  19,
  19,  19,  19,  19,  19,  19,  19,  19,  19,  19,
  19,  19,  19,  19,  19,  27,  27,  27,  27,  27,
  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,
  27,  27,  27,  27,  27,  27,  27,  27,  27,  27,
  27,  27,  27,  27,  27,  27,  29,  29,  28,  28,
  28,  28,  28,   7,   7,   7,   7,   7,   2,   2,
  30,  30,  10,  23,  22,  22,  22,  79,  79,  79,
  49,  46,  46,  47,  21,  21,  38,  38,  38,  38,
  38,  38,  38,  38,  48,  48,  48,  48,  48,  48,
  48,  48,  48,  48,  48,  48,  48,  48,  48,  68,
  68,  68,  68,  68,  69,  69,  50,  50,  51,  51,
 118,  70,  52,  52,  52,  52,  71,  71, 119, 119,
 119, 120, 120, 120, 120, 120, 121, 123, 122,  72,
  72,  73,  73, 124, 124, 124,  74,  91,  53,  53,
  53,  53,  53,  53,  53,  53,  53,  75,  75, 125,
 125, 125, 125,  76,  54,  54,  54,  77,  77, 126,
 126, 126 };
YYSTATIC YYCONST yyr_t YYFARDATA YYR2[]={

   0,   0,   2,   4,   4,   3,   1,   1,   1,   1,
   1,   1,   4,   4,   4,   4,   1,   1,   1,   2,
   2,   3,   2,   1,   1,   3,   2,   4,   6,   2,
   4,   3,   5,   7,   3,   1,   2,   3,   7,   0,
   2,   2,   2,   2,   3,   3,   2,   5,   0,   2,
   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,
   2,   2,   2,   0,   2,   0,   2,   3,   1,   0,
   2,   3,   4,   4,   4,   1,   1,   1,   1,   1,
   2,   2,   4,  13,   1,   7,   0,   2,   0,   2,
   0,   3,   4,   7,   9,   7,   5,   3,   8,   6,
   1,   1,   4,   3,   0,   2,   2,   0,   2,   9,
   7,   9,   7,   9,   7,   9,   7,   1,   1,   1,
   9,   0,   2,   2,   0,   2,   9,   7,   9,   7,
   9,   7,   1,   1,   1,   1,  11,  15,   0,   2,
   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   2,   2,   2,   2,   8,   6,   5,   0,
   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
   1,   1,   1,   0,   4,   4,   4,   4,   0,   2,
   2,   2,   2,   2,   2,   2,   5,   2,   2,   2,
   2,   2,   2,   0,   2,   2,   2,   2,   2,   2,
   2,   2,   2,   2,   2,   1,   2,   1,   2,   4,
   5,   1,   1,   1,   1,   2,   1,   1,   1,   1,
   4,   6,   4,   4,   1,   5,   3,   1,   2,   2,
   1,   2,   4,   4,   1,   2,   2,   2,   2,   2,
   2,   2,   1,   2,   1,   1,   1,   4,   4,   0,
   2,   2,   4,   2,   0,   1,   3,   1,   3,   1,
   0,   3,   5,   4,   3,   5,   5,   5,   5,   5,
   5,   2,   2,   2,   2,   2,   2,   4,   4,   4,
   4,   4,   4,   4,   4,   4,   4,   1,   3,   1,
   2,   0,   1,   1,   2,   2,   1,   1,   1,   2,
   2,   2,   2,   2,   2,   3,   2,   2,   9,   7,
   5,   3,   2,   2,   4,   6,   2,   2,   2,   4,
   2,   0,   1,   1,   3,   1,   2,   3,   6,   7,
   1,   1,   3,   4,   5,   1,   1,   3,   1,   3,
   4,   1,   2,   2,   1,   0,   1,   1,   2,   2,
   2,   2,   0,  10,   6,   5,   5,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,
   2,   2,   2,   2,   3,   4,   6,   5,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,
   4,   1,   2,   2,   1,   2,   1,   2,   1,   2,
   1,   0,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   2,   2,   2,   2,   1,   3,   2,
   2,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   2,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   2,   1,   1,   3,   2,
   3,   4,   2,   2,   2,   5,   5,   2,   7,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,
   2,   2,   2,   2,   3,   2,   1,   3,   0,   1,
   1,   3,   2,   0,   3,   3,   1,   1,   1,   1,
   0,   2,   1,   1,   1,   4,   4,   6,   3,   3,
   4,   1,   3,   3,   1,   1,   1,   1,   4,   1,
   6,   6,   6,   4,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   3,
   2,   5,   4,   3,   8,   4,   0,   2,   0,   1,
   3,   3,   0,   2,   2,   2,   0,   2,   3,   1,
   1,   3,   8,   2,   3,   1,   3,   3,   3,   3,
   5,   0,   2,   3,   1,   3,   4,   3,   0,   2,
   2,   3,   3,   3,   3,   3,   3,   0,   2,   2,
   3,   2,   1,   3,   0,   2,   2,   0,   2,   4,
   3,   1 };
YYSTATIC YYCONST short YYFARDATA YYCHK[]={

-1000, -57, -58, -59, -61, -62, -64, -65, -66, -67,
 -68, -69, -70, -72, -74, -76, -78, -79, -80, 503,
 504, 437, 506, -81, 386, 387, -95, 389,-112, -82,
 433, 425, 426, 440, 447, 454, 408, -49, 428, -11,
 -12, 427, 388, 390, 432, 409, 123, 123, -63, 123,
 123, 123, 123, -10, 265, -10, 505, -50, -23, 265,
 -39, 446,  -1,  -2, 261, 260, 263, -40, -20,  91,
-113, 123,-116, 272,  38,-117, 280, 281, 278, 277,
 276, 275, 282, -32, -33, 267,  91, -10, -10, -52,
 446, -54,  -1, 446, -48, 410, 411, 412, 413, 414,
 415, 416, 417, 418, 419, 420, 421, 422, 423, 424,
 -32, -13,  40,  -8, 312, 326,  -9, 286, 288, 295,
 -32, -32, 263,-114, 306,  61, -48, -60, -57, 125,
 -97, 391, -98, 393, -96, 395, 396, -65, -99,  -2,
 -79, -68, -81, -80, 455, 436, 486,-100, 485,-102,
 394, 371, 372, 373, 374, 375, -55, 376, 377, 378,
 379, 380, 381, 382, -56, 384, 385,-101,-105, 383,
 123, 392, -71, -73, -75, -77, -10,  -1, 438,  -2,
 315, 316, 309, 332, 320, 321, 323, 324, 325, 326,
 328, 329, 330, 331, 344, 322, 338, 313, 351, -53,
  46,  -8, 314, 315, 316, 317, 318, 313, 333, 323,
 345, 346, 347, 348, 349, 350, 351, 337, 358, 352,
 -41, -10,-115,-116,  42,  40, -32,  40, -18,  91,
  40, -18,  40, -18,  40, -18,  40, -18,  40, -18,
  40,  41, 267, -10, 263,  58, 262,  -1, 451, 452,
 453,  -1,  -1, 315, 316,  -1, -31,  -3,  91, -27,
  -6, 293, 283, 284, 309, 310,  33, -92, 294, 272,
 270, 271, 275, 276, 277, 278, 280, 281, 273, 311,
  -1, 339,  41,  61, -14, -31, -15, -92, 340, -27,
  -8,  -8, 287, 289, 290, 291,  41,  41,  44,  -2,
  40,  61, 125, -86, -62, -59, -87, -89, -64, -65,
 -79, -68, -80, 430, 431, -91, 486, -81, 386, 398,
 404, 455, 125, -10, -10,  40, 429,  58,  91, -10,
 -31,  91,-103,-104,-106,-108,-109,-110, 299,-111,
 297, 301, 300, -10,  -2, -10, -23,  40, -22, -23,
 266, 280, 281, -32, -10,  -2,  -8, -27, -31, -37,
-117, 262,  -8,  -2, -10, -14,  40, -30, -63,-100,
  -2, -10, 125,-119, 439, -79,-120,-121, 444, 445,
-122, -80, 441, 125,-124,-118,-120,-123, 439, 442,
 125,-125, 437, 386, -80, 125,-126, 437, 440, -80,
 -51, 395, -83, 302, 315, 316, 317, 345, 346, 347,
  -1, 316, 315, 322,  -1, -17,  40, -27, 314, 315,
 316, 317, 357, 351, 313, 456, 345, 346, 347, 348,
 355, 356,  93, 125,  44,  40,  -2,  41, -22, -10,
 -22, -23, -10, -10, -10,  93, -10, 370,  40,  -1,
 454,  91,  38,  42, 341, 342, 343,  47,  -3,  91,
 293,  -3, -10,  -8, 275, 276, 277, 278, 274, 273,
 279, -37,  40,  41,  -8, -27, -31, 353,  91, 263,
  61,  40, -63, 123, 123, 123, -10, -10, 123, -31,
 -43, -44, -53, -24, -25, -26, 269, -17,  40, -10,
  58, 268, -10,-103,-107,-100, 298,-107,-107,-107,
  -3,-100,  -2, -10,  40,  40,  41, -27, -31,  -2,
  43, -32, -27,  -7,  -2, -10, -10, 125, 304, 304,
 443, -32, -10, -37,  61, -32,  61, -32, -32,  61,
  61,  -1, 446, -10,  -1, 446,-118, -84, 303,  -3,
 315, 316, 317, 345, 346, 347, -27,  91, -37, -45,
  -2,  40,-115, -37,  41,  41,  93,  41,  41,  41,
  41,  41, -16, 263,  -1, -46, -47, -37,  93,  -1,
  93, -29, -28, 269, -10,  40,  40,  -1,  -1, 454,
  -3, -27, 274, -13, -27, -31,  -2, 268,  40,  44,
 125, -60, -88, -90, -75, 268, -31,  -2, 351, 313,
  -8, 351, 313,  -1,  41,  44, -27, -24,  93, -10,
  -4, 353, 354,  -1,  93,  -2, -10, -10, -23, -31,
  -4,  -1, 268, 262,  41,  40,  41,  44,  44,  -2,
 -10, -10,  41,  58,  40,  41,  40,  41,  41,  40,
  40,  -1, 305,  -1, -32, -85,  -3,  -4, 456, 480,
 481, 482, -10, 370, -45,  41, 367, 328, 329, 330,
 368, 369, 287, 289, 290, 291,  -5, 305, -35, 457,
 459, 460, 461, 462, 270, 271, 275, 276, 277, 278,
 280, 281, 257, 273, 463, 464, 465, 467, 468, 469,
 470, 471, 472, 473, 320, 474, 274, 322, 475, 328,
 466, -92, 370, 479,  41, -18, -18, -18, -18, -18,
 -18, 305, 277, 278, 434, 435,  41,  44,  61,  -6,
  93,  93,  44, 269,  -3,  -3,  93,  -1,  42,  61,
 -31,  -4, 268, 353, -24, 263, 125, 125, -93, 399,
 400, 401, 402, -68, -80, -81, 125, -94, 405, 406,
 402, -80, -68, -81, 125,  -4,  -2, -27, -26,  -2,
 456,  41, 370, -36,  61, 304, 304,  41,  41, 268,
  40,  -2, -24,  -7,  -7, -10, -10,  41,  44,  40,
  40,  93,  93,  93,  93, -37,  41, -36,  -2,  41,
  42,  91,  40, 458, 478, 271, 275, 276, 277, 278,
 274, -19, 488, 460, 461, 270, 271, 275, 276, 277,
 278, 280, 281, 273,  42, 463, 464, 465, 467, 468,
 471, 472, 474, 274, 257, 489, 490, 491, 492, 493,
 494, 495, 496, 497, 498, 499, 500, 501, 473, 465,
 477,  -2, -46, -38, -21, -10, 277, -37,  -3, 307,
 308,  -6, -28, -10,  41,  41,  93,  40, -37,  40,
 268,  40,  -2,  40,  41,  -8,  -8,  -8,  -8,  -8,
  -8,  -8, 487,  -2,  40,  -2, -34, 280, 281, 278,
 277, 276, 272, 275, 271, -37,-117, 285,  -2, -10,
  -4, -24,  41,  58, -51,  -3, -24, -35, -45,  93,
 -10,  43, -37,  91,  91,  44,  91, 502,  38, 275,
 276, 277, 278, 274,  40,  40, -24,  -4, -24, -24,
 -27, -27, -27, -27, -27, -27, -27,  -8,  40, -35,
  40,  40,  40,  40,  40,  40,  40,  40, -32,  40,
  41, -10,  41,  41,  41,  93,  43, -10,  44, -10,
 -10, -37,  93, -10, 275, 276, 277, -10,  41,  40,
  41,  41, -31,  -4, -31,  -4, -31,  -4, -31,  -4,
 -31,  -4, -31,  -4, -31,  -4, -27, -24,  41, -22,
 -23, -22, -23, -23, -23, -23, -23, -23, -21,  41,
 -24,  58, -42,  -4, -10,  93, -37,  93,  93,  41,
  58,  58,  58,  41, -24, 268,  40, 268,  40, 268,
  40, 268,  40, 268,  40, 268,  40, 268,  40, -31,
  41,  -2,  41,  41,  41,  41,  41,  41,  41,  41,
  41,  41,  41, -10, 123, 311, 359, 360, 361, 295,
 362, 363, 364, 365, 319, 336,  40,  93,  44,  41,
 -10, -10, -10,  41,  -4, -24,  -4, -24,  -4, -24,
  -4, -24,  -4, -24,  -4, -24,  -4, -24, 268, -36,
 -24, -37,  41,  41,  41,  40,  41,  40,  41,  40,
  41,  40,  41,  40,  41,  40,  41,  40,  41,  -4,
  41,  44, -24, -24, -24, -24, -24, -24, -24,  40,
 -42, -37,  41,  41,  41,  41,  41,  41,  41, -24,
 123,  41,  41 };
YYSTATIC YYCONST short YYFARDATA YYDEF[]={

   1,  -2,   2,   0,   0, 259,   6,   7,   8,   9,
  10,  11,   0,   0,   0,   0,  16,  17,  18,   0,
   0, 546,   0,  23,  48,   0, 148, 100,   0, 301,
   0,   0,   0, 552, 594,  35,   0, 301, 355, 301,
 301,   0, 145, 264,   0,   0,  79,   1,   0, 556,
 571, 587, 597,  19, 502,  20,   0,   0,  22, 503,
   0, 578,  46, 340, 341, 498, 499, 355, 188,   0,
 261,   0, 267,   0,   0, 301, 270, 270, 270, 270,
 270, 270,   0,   0, 302, 303,   0, 540,   0,   0,
   0,   0,  36,   0,   0, 524, 525, 526, 527, 528,
 529, 530, 531, 532, 533, 534, 535, 536, 537, 538,
   0,  29,   0,   0, 355, 355, 354, 356, 357,   0,
   0,   0,  26, 263, 265,   0,   0,   0,   0,   5,
 260,   0, 217,   0,   0, 221, 222, 223, 224,   0,
 226, 227, 228, 229,   0,   0,   0, 234,   0,   0,
 215, 308,   0,   0,   0,   0, 301,   0, 355,   0,
   0,   0, 355,   0,   0,   0, 500, 259,   0, 306,
 237, 244,   0,   0,   0,   0,  21, 548, 547,  73,
  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,
  59,  60,  61,  62,  63,   0,  70,  71,  72,   0,
   0, 183, 149, 150, 151, 152, 153, 154, 155, 156,
 157, 158, 159, 160, 161, 162, 163, 164, 165,   0,
   0,   0,   0, 269,   0,   0,   0,   0, 281,   0,
   0, 282,   0, 283,   0, 284,   0, 285,   0, 286,
 300,  44, 304,   0, 539,   0, 543, 551, 553, 554,
 555, 569, 593, 595, 596,  37, 508, 348,   0, 351,
 345,   0, 456, 457,   0,   0,   0, 355, 469, 470,
 471, 472, 473, 474, 475, 476, 477, 478,   0,   0,
 346, 307, 509,   0,   0, 110, 111, 355,   0,   0,
 352, 353, 358, 359, 360, 361,  31,  34,   0,   0,
  45,   0,   3,  80, 259,   0,   0,   0,  85,  86,
  87,  88,  89,   0,   0,   0,   0,  94,  48, 114,
 131, 578,   4, 216, 218,  -2,   0, 225,   0,   0,
   0,   0, 238, 240,   0,   0,   0,   0,   0,   0,
 254, 255, 252, 309, 310, 311, 312, 305, 313, 314,
 504,   0,   0,   0, 316, 317,   0,   0, 322, 323,
 301,  24,   0, 326, 327, 328, 493, 330,   0, 241,
   0,   0,  12, 557,   0, 559, 560, 301,   0,   0,
 301, 565,   0,  13, 572, 301, 574, 301,   0,   0,
  14, 588,   0,   0, 592,  15, 598,   0,   0, 601,
 545, 549,  75,   0,  64,  65,  66,  67,  68,  69,
 576, 579, 580,   0, 342,   0, 169,   0, 189, 190,
 191, 192, 193, 194, 195,   0, 197, 198, 199, 200,
 201, 202, 101, 266,   0,   0,   0, 274,   0,   0,
   0,   0,   0,   0,   0,  39, 542,   0,   0,   0,
   0, 488, 462, 463, 464,   0,   0,   0, 455,   0,
   0, 459, 467,   0, 479, 480, 481, 482, 483,   0,
 485,  30, 102, 355,   0,   0,   0,   0, 488,  27,
 262, 510,   0,  79, 117, 134,  90,  91, 587,   0,
   0, 355,   0,   0, 332, 333, 335,   0,  -2,   0,
   0,   0,   0, 239, 245, 256,   0, 246, 247, 248,
 253, 249, 250, 251,   0,   0, 315,   0,   0,  -2,
   0,   0,   0,   0, 496, 497, 501, 236,   0,   0,
   0,   0,   0, 563,   0,   0,   0,   0,   0,   0,
   0, 589,   0, 591,   0,   0, 301,  47,   0,  74,
 581, 582, 583, 584, 585, 586,   0,   0, 169,   0,
  96, 362, 268,   0, 273, 270, 271, 270, 270, 270,
 270, 270,   0, 541, 570,   0, 511,   0, 349,   0,
 460,   0, 486, 489, 490,   0,   0, 347,   0,   0,
 458,   0, 484,  32,   0,   0,  -2,   0,  -2,   0,
  81,   0,   0,   0,   0,   0,   0,  -2, 115, 116,
   0, 132, 133, 577, 219, 183, 336,   0, 230, 232,
 233, 180, 181, 182,  98,   0,   0,   0,   0,   0,
   0,  -2,   0,  25, 324,  -2, 329, 493, 493, 242,
 243, 558, 561,   0, 568, 564, 566, 573, 575, 550,
 567, 590,   0, 600,   0,  76,  78,   0,   0,   0,
   0,   0,   0,   0,   0, 168, 170, 171, 172, 173,
 174, 175, 176, 177, 178, 179,  98,   0,   0,   0,
   0, 367, 368, 369, 370, 371, 372, 373, 374, 375,
 376, 377, 378,   0, 388, 389, 390, 391, 392, 393,
 394, 395, 396, 397, 398, 411, 401,   0, 404,   0,
 406, 408,   0, 410, 272, 275, 276, 277, 278, 279,
 280,   0,  40,  41,  42,  43, 507,   0,   0, 343,
 350, 461, 488, 492,   0,   0,   0,   0, 463,   0,
   0,   0,   0,   0,   0,  28,  82,  83, 118, 355,
 355, 355, 355, 127, 128, 129,  84, 135, 355, 355,
 355, 142, 143, 144,  92,   0, 112,   0, 334, 337,
   0, 220,   0, 235,   0,   0,   0, 505, 506,   0,
  -2, 320,   0, 494, 495,   0, 599, 548,   0,  -2,
 362, 184, 185, 186, 187, 169, 167,  95,  97, 196,
 383,   0,   0,   0,   0, 407, 379, 380, 381, 382,
 402, 399, 412, 413, 414, 415, 416, 417, 418, 419,
 420, 421, 422,   0, 427, 431, 432, 433, 434, 435,
 436, 437, 438, 439, 441, 442, 443, 444, 445, 446,
 447, 448, 449, 450, 451, 452, 453, 454, 403, 405,
 409,  38, 512, 513, 516, 517,   0, 519,   0, 514,
 515, 344, 487, 491, 465, 466,   0,  -2,  33, 103,
   0,  -2, 106,  -2, 109,   0,   0,   0,   0,   0,
   0,   0, 355,   0, 362, 231,  99,   0,   0,   0,
   0,   0,   0,   0,   0, 297, 301, 299, 257, 258,
   0,   0, 325,   0, 544,  77,   0,   0,   0, 384,
   0,   0,   0,   0,   0,   0,   0, 429, 430, 423,
 424, 425, 426, 440,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  -2,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  -2,
 319,   0, 203,   0, 166, 385,   0,   0,   0,   0,
   0, 400, 428,   0,   0,   0,   0,   0, 468,  -2,
 105, 108,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0, 338,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 298,
   0,   0,   0,   0,   0, 387,   0, 365, 366, 518,
   0,   0,   0, 523,   0,   0,  -2,   0,  -2,   0,
  -2,   0,  -2,   0,  -2,   0,  -2,   0,  -2,   0,
  98, 339, 287, 289, 288, 290, 291, 292, 293, 294,
 295, 296, 318, 562, 146, 204, 205, 206, 207, 208,
 209, 210, 211, 212, 213, 214,  -2, 386,   0, 364,
   0,   0,   0, 104,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 130,
   0,   0, 520, 521, 522,  -2, 120,  -2, 122,  -2,
 124,  -2, 126,  -2, 137,  -2, 139,  -2, 141,   0,
 203,   0,   0,   0,   0,   0,   0,   0,   0,  -2,
   0,   0, 119, 121, 123, 125, 136, 138, 140,   0,
 147, 363,  93 };
#ifdef YYRECOVER
YYSTATIC YYCONST short yyrecover[] = {
-1000
};
#endif

/* SCCSWHAT( "@(#)yypars.c    3.1 88/11/16 22:00:49    " ) */
#line 3 "yypars.c"
#if ! defined(YYAPI_PACKAGE)
/*
**  YYAPI_TOKENNAME        : name used for return value of yylex    
**    YYAPI_TOKENTYPE        : type of the token
**    YYAPI_TOKENEME(t)    : the value of the token that the parser should see
**    YYAPI_TOKENNONE        : the representation when there is no token
**    YYAPI_VALUENAME        : the name of the value of the token
**    YYAPI_VALUETYPE        : the type of the value of the token (if null, then the value is derivable from the token itself)
**    YYAPI_VALUEOF(v)    : how to get the value of the token.
*/
#define    YYAPI_TOKENNAME        yychar
#define    YYAPI_TOKENTYPE        int
#define    YYAPI_TOKENEME(t)    (t)
#define    YYAPI_TOKENNONE        -1
#define    YYAPI_TOKENSTR(t)    (sprintf(yytokbuf, "%d", t), yytokbuf)
#define    YYAPI_VALUENAME        yylval
#define    YYAPI_VALUETYPE        YYSTYPE
#define    YYAPI_VALUEOF(v)    (v)
#endif
#if ! defined(YYAPI_CALLAFTERYYLEX)
#define    YYAPI_CALLAFTERYYLEX(x)
#endif

# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

#ifdef YYDEBUG                /* RRR - 10/9/85 */
char yytokbuf[20];
# ifndef YYDBFLG
#  define YYDBFLG (yydebug)
# endif
# define yyprintf(a, b, c, d) if (YYDBFLG) YYPRINT(a, b, c, d)
#else
# define yyprintf(a, b, c, d)
#endif

#ifndef YYPRINT
#define    YYPRINT    printf
#endif

/*    parser for yacc output    */

#ifdef YYDUMP
int yydump = 1; /* 1 for dumping */
void yydumpinfo(void);
#endif
#ifdef YYDEBUG
YYSTATIC int yydebug = 0; /* 1 for debugging */
#endif
YYSTATIC YYSTYPE yyv[YYMAXDEPTH];    /* where the values are stored */
YYSTATIC short    yys[YYMAXDEPTH];    /* the parse stack */

#if ! defined(YYRECURSIVE)
YYSTATIC YYAPI_TOKENTYPE    YYAPI_TOKENNAME = YYAPI_TOKENNONE;
#if defined(YYAPI_VALUETYPE)
// YYSTATIC YYAPI_VALUETYPE    YYAPI_VALUENAME;     FIX 
#endif
YYSTATIC int yynerrs = 0;            /* number of errors */
YYSTATIC short yyerrflag = 0;        /* error recovery flag */
#endif

#ifdef YYRECOVER
/*
**  yyscpy : copy f onto t and return a ptr to the null terminator at the
**  end of t.
*/
YYSTATIC    char    *yyscpy(register char*t, register char*f)
    {
    while(*t = *f++)
        t++;
    return(t);    /*  ptr to the null char  */
    }
#endif

#ifndef YYNEAR
#define YYNEAR
#endif
#ifndef YYPASCAL
#define YYPASCAL
#endif
#ifndef YYLOCAL
#define YYLOCAL
#endif
#if ! defined YYPARSER
#define YYPARSER yyparse
#endif
#if ! defined YYLEX
#define YYLEX yylex
#endif

#if defined(YYRECURSIVE)

    YYSTATIC YYAPI_TOKENTYPE    YYAPI_TOKENNAME = YYAPI_TOKENNONE;
  #if defined(YYAPI_VALUETYPE)
    YYSTATIC YYAPI_VALUETYPE    YYAPI_VALUENAME;  
  #endif
    YYSTATIC int yynerrs = 0;            /* number of errors */
    YYSTATIC short yyerrflag = 0;        /* error recovery flag */

    YYSTATIC short    yyn;
    YYSTATIC short    yystate = 0;
    YYSTATIC short    *yyps= &yys[-1];
    YYSTATIC YYSTYPE    *yypv= &yyv[-1];
    YYSTATIC short    yyj;
    YYSTATIC short    yym;

#endif

#ifdef _MSC_VER
#pragma warning(disable:102)
#endif
int YYLOCAL YYNEAR YYPASCAL YYPARSER()
{
#if ! defined(YYRECURSIVE)

    register    short    yyn;
    short        yystate, *yyps;
    YYSTYPE        *yypv;
    short        yyj, yym;

    YYAPI_TOKENNAME = YYAPI_TOKENNONE;
    yystate = 0;
    yyps= &yys[-1];
    yypv= &yyv[-1];
#endif

#ifdef YYDUMP
    yydumpinfo();
#endif
 yystack:     /* put a state and value onto the stack */

#ifdef YYDEBUG
    if(YYAPI_TOKENNAME == YYAPI_TOKENNONE) {
        yyprintf( "state %d, token # '%d'\n", yystate, -1, 0 );
        }
    else {
        yyprintf( "state %d, token # '%s'\n", yystate, YYAPI_TOKENSTR(YYAPI_TOKENNAME), 0 );
        }
#endif
    if( ++yyps > &yys[YYMAXDEPTH] ) {
        yyerror( "yacc stack overflow" );
        return(1);
    }
    *yyps = yystate;
    ++yypv;

    *yypv = yyval;

yynewstate:

    yyn = YYPACT[yystate];

    if( yyn <= YYFLAG ) {    /*  simple state, no lookahead  */
        goto yydefault;
    }
    if( YYAPI_TOKENNAME == YYAPI_TOKENNONE ) {    /*  need a lookahead */
        YYAPI_TOKENNAME = YYLEX();
        YYAPI_CALLAFTERYYLEX(YYAPI_TOKENNAME);
    }
    if( ((yyn += YYAPI_TOKENEME(YYAPI_TOKENNAME)) < 0) || (yyn >= YYLAST) ) {
        goto yydefault;
    }
    if( YYCHK[ yyn = YYACT[ yyn ] ] == YYAPI_TOKENEME(YYAPI_TOKENNAME) ) {        /* valid shift */
        yyval = YYAPI_VALUEOF(YYAPI_VALUENAME);
        yystate = yyn;
         yyprintf( "SHIFT: saw token '%s', now in state %4d\n", YYAPI_TOKENSTR(YYAPI_TOKENNAME), yystate, 0 );
        YYAPI_TOKENNAME = YYAPI_TOKENNONE;
        if( yyerrflag > 0 ) {
            --yyerrflag;
        }
        goto yystack;
    }

 yydefault:
    /* default state action */

    if( (yyn = YYDEF[yystate]) == -2 ) {
        register    YYCONST short    *yyxi;

        if( YYAPI_TOKENNAME == YYAPI_TOKENNONE ) {
            YYAPI_TOKENNAME = YYLEX();
            YYAPI_CALLAFTERYYLEX(YYAPI_TOKENNAME);
             yyprintf("LOOKAHEAD: token '%s'\n", YYAPI_TOKENSTR(YYAPI_TOKENNAME), 0, 0);
        }
/*
**  search exception table, we find a -1 followed by the current state.
**  if we find one, we'll look through terminal,state pairs. if we find
**  a terminal which matches the current one, we have a match.
**  the exception table is when we have a reduce on a terminal.
*/

#if YYOPTTIME
        yyxi = yyexca + yyexcaind[yystate];
        while(( *yyxi != YYAPI_TOKENEME(YYAPI_TOKENNAME) ) && ( *yyxi >= 0 )){
            yyxi += 2;
        }
#else
        for(yyxi = yyexca;
            (*yyxi != (-1)) || (yyxi[1] != yystate);
            yyxi += 2
        ) {
            ; /* VOID */
            }

        while( *(yyxi += 2) >= 0 ){
            if( *yyxi == YYAPI_TOKENEME(YYAPI_TOKENNAME) ) {
                break;
                }
        }
#endif
        if( (yyn = yyxi[1]) < 0 ) {
            return(0);   /* accept */
            }
        }

    if( yyn == 0 ){ /* error */
        /* error ... attempt to resume parsing */

        switch( yyerrflag ){

        case 0:        /* brand new error */
#ifdef YYRECOVER
            {
            register    int        i,j;

            for(i = 0;
                (yyrecover[i] != -1000) && (yystate > yyrecover[i]);
                i += 3
            ) {
                ;
            }
            if(yystate == yyrecover[i]) {
                yyprintf("recovered, from state %d to state %d on token # %d\n",
                        yystate,yyrecover[i+2],yyrecover[i+1]
                        );
                j = yyrecover[i + 1];
                if(j < 0) {
                /*
                **  here we have one of the injection set, so we're not quite
                **  sure that the next valid thing will be a shift. so we'll
                **  count it as an error and continue.
                **  actually we're not absolutely sure that the next token
                **  we were supposed to get is the one when j > 0. for example,
                **  for(+) {;} error recovery with yyerrflag always set, stops
                **  after inserting one ; before the +. at the point of the +,
                **  we're pretty sure the guy wants a 'for' loop. without
                **  setting the flag, when we're almost absolutely sure, we'll
                **  give him one, since the only thing we can shift on this
                **  error is after finding an expression followed by a +
                */
                    yyerrflag++;
                    j = -j;
                    }
                if(yyerrflag <= 1) {    /*  only on first insertion  */
                    yyrecerr(YYAPI_TOKENNAME, j);    /*  what was, what should be first */
                }
                yyval = yyeval(j);
                yystate = yyrecover[i + 2];
                goto yystack;
                }
            }
#endif
        yyerror("syntax error");

        yyerrlab:
            ++yynerrs;

        case 1:
        case 2: /* incompletely recovered error ... try again */

            yyerrflag = 3;

            /* find a state where "error" is a legal shift action */

            while ( yyps >= yys ) {
               yyn = YYPACT[*yyps] + YYERRCODE;
               if( yyn>= 0 && yyn < YYLAST && YYCHK[YYACT[yyn]] == YYERRCODE ){
                  yystate = YYACT[yyn];  /* simulate a shift of "error" */
                   yyprintf( "SHIFT 'error': now in state %4d\n", yystate, 0, 0 );
                  goto yystack;
                  }
               yyn = YYPACT[*yyps];

               /* the current yyps has no shift onn "error", pop stack */

                yyprintf( "error recovery pops state %4d, uncovers %4d\n", *yyps, yyps[-1], 0 );
               --yyps;
               --yypv;
               }

            /* there is no state on the stack with an error shift ... abort */

    yyabort:
            return(1);


        case 3:  /* no shift yet; clobber input char */

             yyprintf( "error recovery discards token '%s'\n", YYAPI_TOKENSTR(YYAPI_TOKENNAME), 0, 0 );

            if( YYAPI_TOKENEME(YYAPI_TOKENNAME) == 0 ) goto yyabort; /* don't discard EOF, quit */
            YYAPI_TOKENNAME = YYAPI_TOKENNONE;
            goto yynewstate;   /* try again in the same state */
            }
        }

    /* reduction by production yyn */
        {
        register    YYSTYPE    *yypvt;
        yypvt = yypv;
        yyps -= YYR2[yyn];
        yypv -= YYR2[yyn];
        yyval = yypv[1];
         yyprintf("REDUCE: rule %4d, popped %2d tokens, uncovered state %4d, ",yyn, YYR2[yyn], *yyps);
        yym = yyn;
        yyn = YYR1[yyn];        /* consult goto table to find next state */
        yyj = YYPGO[yyn] + *yyps + 1;
        if( (yyj >= YYLAST) || (YYCHK[ yystate = YYACT[yyj] ] != -yyn) ) {
            yystate = YYACT[YYPGO[yyn]];
            }
         yyprintf("goto state %4d\n", yystate, 0, 0);
#ifdef YYDUMP
        yydumpinfo();
#endif
        switch(yym){
            
case 3:
#line 192 "asmparse.y"
{ PASM->EndClass(); } break;
case 4:
#line 193 "asmparse.y"
{ PASM->EndNameSpace(); } break;
case 5:
#line 194 "asmparse.y"
{ if(PASM->m_pCurMethod->m_ulLines[1] ==0)
                                                                                  {  PASM->m_pCurMethod->m_ulLines[1] = PASM->m_ulCurLine;
                                                                                     PASM->m_pCurMethod->m_ulColumns[1]=PASM->m_ulCurColumn;}
                                                                                    PASM->EndMethod(); } break;
case 12:
#line 204 "asmparse.y"
{ PASMM->EndAssembly(); } break;
case 13:
#line 205 "asmparse.y"
{ PASMM->EndAssembly(); } break;
case 14:
#line 206 "asmparse.y"
{ PASMM->EndComType(); } break;
case 15:
#line 207 "asmparse.y"
{ PASMM->EndManifestRes(); } break;
case 19:
#line 211 "asmparse.y"
{ if(!g_dwSubsystem) PASM->m_dwSubsystem = yypvt[-0].int32; } break;
case 20:
#line 212 "asmparse.y"
{ if(!g_dwComImageFlags) PASM->m_dwComImageFlags = yypvt[-0].int32; } break;
case 21:
#line 213 "asmparse.y"
{ if(!g_dwFileAlignment) PASM->m_dwFileAlignment = yypvt[-0].int32; } break;
case 22:
#line 214 "asmparse.y"
{ if(!g_stBaseAddress) PASM->m_stBaseAddress = (size_t)(*(yypvt[-0].int64)); delete yypvt[-0].int64; } break;
case 24:
#line 218 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 25:
#line 219 "asmparse.y"
{ yyval.binstr = yypvt[-2].binstr; yyval.binstr->append(yypvt[-0].binstr); delete yypvt[-0].binstr; } break;
case 26:
#line 222 "asmparse.y"
{ LPCSTRToGuid(yypvt[-0].string,&(PASM->m_guidLang)); } break;
case 27:
#line 223 "asmparse.y"
{ LPCSTRToGuid(yypvt[-2].string,&(PASM->m_guidLang)); 
                                                                                  LPCSTRToGuid(yypvt[-0].string,&(PASM->m_guidLangVendor));} break;
case 28:
#line 225 "asmparse.y"
{ LPCSTRToGuid(yypvt[-4].string,&(PASM->m_guidLang)); 
                                                                                  LPCSTRToGuid(yypvt[-2].string,&(PASM->m_guidLangVendor));
                                                                                  LPCSTRToGuid(yypvt[-2].string,&(PASM->m_guidDoc));} break;
case 29:
#line 230 "asmparse.y"
{ if(PASM->m_tkCurrentCVOwner) 
                                                                                    PASM->DefineCV(PASM->m_tkCurrentCVOwner, yypvt[-0].int32, NULL);
                                                                                  else if(PASM->m_pCustomDescrList)
                                                                                    PASM->m_pCustomDescrList->PUSH(new CustomDescr(yypvt[-0].int32, NULL)); } break;
case 30:
#line 234 "asmparse.y"
{ if(PASM->m_tkCurrentCVOwner) 
                                                                                    PASM->DefineCV(PASM->m_tkCurrentCVOwner, yypvt[-2].int32, yypvt[-0].binstr);
                                                                                  else if(PASM->m_pCustomDescrList)
                                                                                    PASM->m_pCustomDescrList->PUSH(new CustomDescr(yypvt[-2].int32, yypvt[-0].binstr)); } break;
case 31:
#line 238 "asmparse.y"
{ if(PASM->m_tkCurrentCVOwner) 
                                                                                    PASM->DefineCV(PASM->m_tkCurrentCVOwner, yypvt[-2].int32, yypvt[-1].binstr);
                                                                                   else if(PASM->m_pCustomDescrList)
                                                                                    PASM->m_pCustomDescrList->PUSH(new CustomDescr(yypvt[-2].int32, yypvt[-1].binstr)); } break;
case 32:
#line 242 "asmparse.y"
{ PASM->DefineCV(yypvt[-2].int32, yypvt[-0].int32, NULL); } break;
case 33:
#line 243 "asmparse.y"
{ PASM->DefineCV(yypvt[-4].int32, yypvt[-2].int32, yypvt[-0].binstr); } break;
case 34:
#line 244 "asmparse.y"
{ PASM->DefineCV(PASM->m_tkCurrentCVOwner, yypvt[-2].int32, yypvt[-1].binstr); } break;
case 35:
#line 247 "asmparse.y"
{ PASMM->SetModuleName(NULL); PASM->m_tkCurrentCVOwner=1; } break;
case 36:
#line 248 "asmparse.y"
{ PASMM->SetModuleName(yypvt[-0].string); PASM->m_tkCurrentCVOwner=1; } break;
case 37:
#line 249 "asmparse.y"
{ BinStr* pbs = new BinStr();
                                                                                  strcpy((char*)(pbs->getBuff((unsigned)strlen(yypvt[-0].string)+1)),yypvt[-0].string);
                                                                                  PASM->EmitImport(pbs); delete pbs;} break;
case 38:
#line 254 "asmparse.y"
{ /*PASM->SetDataSection(); PASM->EmitDataLabel($7);*/
                                                                                  PASM->m_VTFList.PUSH(new VTFEntry((USHORT)yypvt[-4].int32, (USHORT)yypvt[-2].int32, yypvt[-0].string)); } break;
case 39:
#line 258 "asmparse.y"
{ yyval.int32 = 0; } break;
case 40:
#line 259 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | COR_VTABLE_32BIT; } break;
case 41:
#line 260 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | COR_VTABLE_64BIT; } break;
case 42:
#line 261 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | COR_VTABLE_FROM_UNMANAGED; } break;
case 43:
#line 262 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | COR_VTABLE_CALL_MOST_DERIVED; } break;
case 44:
#line 265 "asmparse.y"
{ PASM->m_pVTable = yypvt[-1].binstr; } break;
case 45:
#line 268 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 46:
#line 271 "asmparse.y"
{ PASM->StartNameSpace(yypvt[-0].string); } break;
case 47:
#line 274 "asmparse.y"
{ PASM->StartClass(yypvt[-2].string, yypvt[-3].classAttr); } break;
case 48:
#line 277 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) 0; } break;
case 49:
#line 278 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdVisibilityMask) | tdPublic); } break;
case 50:
#line 279 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdVisibilityMask) | tdNotPublic); } break;
case 51:
#line 280 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | 0x80000000); } break;
case 52:
#line 281 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | 0x40000000); } break;
case 53:
#line 282 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdInterface | tdAbstract); } break;
case 54:
#line 283 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdSealed); } break;
case 55:
#line 284 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdAbstract); } break;
case 56:
#line 285 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdLayoutMask) | tdAutoLayout); } break;
case 57:
#line 286 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdLayoutMask) | tdSequentialLayout); } break;
case 58:
#line 287 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdLayoutMask) | tdExplicitLayout); } break;
case 59:
#line 288 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdStringFormatMask) | tdAnsiClass); } break;
case 60:
#line 289 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdStringFormatMask) | tdUnicodeClass); } break;
case 61:
#line 290 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-1].classAttr & ~tdStringFormatMask) | tdAutoClass); } break;
case 62:
#line 291 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdImport); } break;
case 63:
#line 292 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdSerializable); } break;
case 64:
#line 293 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedPublic); } break;
case 65:
#line 294 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedPrivate); } break;
case 66:
#line 295 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedFamily); } break;
case 67:
#line 296 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedAssembly); } break;
case 68:
#line 297 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedFamANDAssem); } break;
case 69:
#line 298 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) ((yypvt[-2].classAttr & ~tdVisibilityMask) | tdNestedFamORAssem); } break;
case 70:
#line 299 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdBeforeFieldInit); } break;
case 71:
#line 300 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr | tdSpecialName); } break;
case 72:
#line 301 "asmparse.y"
{ yyval.classAttr = (CorRegTypeAttr) (yypvt[-1].classAttr); } break;
case 74:
#line 305 "asmparse.y"
{ strcpy(PASM->m_szExtendsClause,yypvt[-0].string); } break;
case 77:
#line 312 "asmparse.y"
{ PASM->AddToImplList(yypvt[-0].string); } break;
case 78:
#line 313 "asmparse.y"
{ PASM->AddToImplList(yypvt[-0].string); } break;
case 81:
#line 320 "asmparse.y"
{ if(PASM->m_pCurMethod->m_ulLines[1] ==0)
                                                              {  PASM->m_pCurMethod->m_ulLines[1] = PASM->m_ulCurLine;
                                                                 PASM->m_pCurMethod->m_ulColumns[1]=PASM->m_ulCurColumn;}
                                                              PASM->EndMethod(); } break;
case 82:
#line 324 "asmparse.y"
{ PASM->EndClass(); } break;
case 83:
#line 325 "asmparse.y"
{ PASM->EndEvent(); } break;
case 84:
#line 326 "asmparse.y"
{ PASM->EndProp(); } break;
case 90:
#line 332 "asmparse.y"
{ PASM->m_pCurClass->m_ulSize = yypvt[-0].int32; } break;
case 91:
#line 333 "asmparse.y"
{ PASM->m_pCurClass->m_ulPack = yypvt[-0].int32; } break;
case 92:
#line 334 "asmparse.y"
{ PASMM->EndComType(); } break;
case 93:
#line 336 "asmparse.y"
{ PASM->AddMethodImpl(yypvt[-11].binstr,yypvt[-9].string,parser->MakeSig(yypvt[-7].int32,yypvt[-6].binstr,yypvt[-1].binstr),yypvt[-5].binstr,yypvt[-3].string); } break;
case 95:
#line 341 "asmparse.y"
{ yypvt[-3].binstr->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD);
                                                              PASM->AddField(yypvt[-2].string, yypvt[-3].binstr, yypvt[-4].fieldAttr, yypvt[-1].string, yypvt[-0].binstr, yypvt[-5].int32); } break;
case 96:
#line 346 "asmparse.y"
{ yyval.string = 0; } break;
case 97:
#line 347 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 98:
#line 350 "asmparse.y"
{ yyval.binstr = NULL; } break;
case 99:
#line 351 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 100:
#line 354 "asmparse.y"
{ yyval.int32 = 0xFFFFFFFF; } break;
case 101:
#line 355 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32; } break;
case 102:
#line 358 "asmparse.y"
{ yyval.int32 = yypvt[-2].int32; bParsingByteArray = TRUE; } break;
case 103:
#line 362 "asmparse.y"
{ PASM->m_pCustomDescrList = NULL;
                                                              PASM->m_tkCurrentCVOwner = yypvt[-4].int32;
                                                              yyval.int32 = yypvt[-2].int32; bParsingByteArray = TRUE; } break;
case 104:
#line 368 "asmparse.y"
{ yyval.int32 = PASM->MakeMemberRef(yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr),iOpcodeLen); 
                                                               delete PASM->m_firstArgName;
                                                               PASM->m_firstArgName = palDummy;
                                                            } break;
case 105:
#line 373 "asmparse.y"
{ yyval.int32 = PASM->MakeMemberRef(NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr),iOpcodeLen); 
                                                               delete PASM->m_firstArgName;
                                                               PASM->m_firstArgName = palDummy;
                                                            } break;
case 106:
#line 378 "asmparse.y"
{ yypvt[-3].binstr->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               yyval.int32 = PASM->MakeMemberRef(yypvt[-2].binstr, yypvt[-0].string, yypvt[-3].binstr, 0); } break;
case 107:
#line 381 "asmparse.y"
{ yypvt[-1].binstr->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               yyval.int32 = PASM->MakeMemberRef(NULL, yypvt[-0].string, yypvt[-1].binstr, 0); } break;
case 108:
#line 386 "asmparse.y"
{ yyval.int32 = PASM->MakeMemberRef(yypvt[-5].binstr, newString(COR_CTOR_METHOD_NAME), parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr),0); } break;
case 109:
#line 388 "asmparse.y"
{ yyval.int32 = PASM->MakeMemberRef(NULL, newString(COR_CTOR_METHOD_NAME), parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr),0); } break;
case 110:
#line 391 "asmparse.y"
{ yyval.int32 = PASM->MakeTypeRef(yypvt[-0].binstr); } break;
case 111:
#line 392 "asmparse.y"
{ yyval.int32 = yypvt[-0].int32; } break;
case 112:
#line 395 "asmparse.y"
{ PASM->ResetEvent(yypvt[-0].string, yypvt[-1].binstr, yypvt[-2].eventAttr); } break;
case 113:
#line 396 "asmparse.y"
{ PASM->ResetEvent(yypvt[-0].string, NULL, yypvt[-1].eventAttr); } break;
case 114:
#line 400 "asmparse.y"
{ yyval.eventAttr = (CorEventAttr) 0; } break;
case 115:
#line 401 "asmparse.y"
{ yyval.eventAttr = yypvt[-1].eventAttr; } break;
case 116:
#line 402 "asmparse.y"
{ yyval.eventAttr = (CorEventAttr) (yypvt[-1].eventAttr | evSpecialName); } break;
case 119:
#line 410 "asmparse.y"
{ PASM->SetEventMethod(0, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 120:
#line 412 "asmparse.y"
{ PASM->SetEventMethod(0, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 121:
#line 414 "asmparse.y"
{ PASM->SetEventMethod(1, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 122:
#line 416 "asmparse.y"
{ PASM->SetEventMethod(1, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 123:
#line 418 "asmparse.y"
{ PASM->SetEventMethod(2, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 124:
#line 420 "asmparse.y"
{ PASM->SetEventMethod(2, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 125:
#line 422 "asmparse.y"
{ PASM->SetEventMethod(3, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 126:
#line 424 "asmparse.y"
{ PASM->SetEventMethod(3, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 130:
#line 431 "asmparse.y"
{ PASM->ResetProp(yypvt[-4].string, 
                                                              parser->MakeSig((IMAGE_CEE_CS_CALLCONV_PROPERTY |
                                                              (yypvt[-6].int32 & IMAGE_CEE_CS_CALLCONV_HASTHIS)),yypvt[-5].binstr,yypvt[-2].binstr), yypvt[-7].propAttr, yypvt[-0].binstr); } break;
case 131:
#line 436 "asmparse.y"
{ yyval.propAttr = (CorPropertyAttr) 0; } break;
case 132:
#line 437 "asmparse.y"
{ yyval.propAttr = yypvt[-1].propAttr; } break;
case 133:
#line 438 "asmparse.y"
{ yyval.propAttr = (CorPropertyAttr) (yypvt[-1].propAttr | prSpecialName); } break;
case 136:
#line 447 "asmparse.y"
{ PASM->SetPropMethod(0, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 137:
#line 449 "asmparse.y"
{ PASM->SetPropMethod(0, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 138:
#line 451 "asmparse.y"
{ PASM->SetPropMethod(1, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 139:
#line 453 "asmparse.y"
{ PASM->SetPropMethod(1, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 140:
#line 455 "asmparse.y"
{ PASM->SetPropMethod(2, yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr)); } break;
case 141:
#line 457 "asmparse.y"
{ PASM->SetPropMethod(2, NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr)); } break;
case 145:
#line 464 "asmparse.y"
{ PASM->ResetForNextMethod(); 
                                                              uMethodBeginLine = PASM->m_ulCurLine;
                                                              uMethodBeginColumn=PASM->m_ulCurColumn;} break;
case 146:
#line 470 "asmparse.y"
{ PASM->StartMethod(yypvt[-5].string, parser->MakeSig(yypvt[-8].int32, yypvt[-6].binstr, yypvt[-3].binstr), yypvt[-9].methAttr, NULL, yypvt[-7].int32);
                                                              PASM->SetImplAttr((USHORT)yypvt[-1].implAttr);  
                                                              PASM->m_pCurMethod->m_ulLines[0] = uMethodBeginLine;
                                                              PASM->m_pCurMethod->m_ulColumns[0]=uMethodBeginColumn; } break;
case 147:
#line 475 "asmparse.y"
{ PASM->StartMethod(yypvt[-5].string, parser->MakeSig(yypvt[-12].int32, yypvt[-10].binstr, yypvt[-3].binstr), yypvt[-13].methAttr, yypvt[-7].binstr, yypvt[-11].int32);
                                                              PASM->SetImplAttr((USHORT)yypvt[-1].implAttr);
                                                              PASM->m_pCurMethod->m_ulLines[0] = uMethodBeginLine;
                                                              PASM->m_pCurMethod->m_ulColumns[0]=uMethodBeginColumn; } break;
case 148:
#line 482 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) 0; } break;
case 149:
#line 483 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdStatic); } break;
case 150:
#line 484 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdPublic); } break;
case 151:
#line 485 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdPrivate); } break;
case 152:
#line 486 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdFamily); } break;
case 153:
#line 487 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdFinal); } break;
case 154:
#line 488 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdSpecialName); } break;
case 155:
#line 489 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdVirtual); } break;
case 156:
#line 490 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdAbstract); } break;
case 157:
#line 491 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdAssem); } break;
case 158:
#line 492 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdFamANDAssem); } break;
case 159:
#line 493 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdFamORAssem); } break;
case 160:
#line 494 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) ((yypvt[-1].methAttr & ~mdMemberAccessMask) | mdPrivateScope); } break;
case 161:
#line 495 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdHideBySig); } break;
case 162:
#line 496 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdNewSlot); } break;
case 163:
#line 497 "asmparse.y"
{ yyval.methAttr = yypvt[-1].methAttr; } break;
case 164:
#line 498 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdUnmanagedExport); } break;
case 165:
#line 499 "asmparse.y"
{ yyval.methAttr = (CorMethodAttr) (yypvt[-1].methAttr | mdRequireSecObject); } break;
case 166:
#line 502 "asmparse.y"
{ PASM->SetPinvoke(yypvt[-4].binstr,0,yypvt[-2].binstr,yypvt[-1].pinvAttr); 
                                                              yyval.methAttr = (CorMethodAttr) (yypvt[-7].methAttr | mdPinvokeImpl); } break;
case 167:
#line 505 "asmparse.y"
{ PASM->SetPinvoke(yypvt[-2].binstr,0,NULL,yypvt[-1].pinvAttr); 
                                                              yyval.methAttr = (CorMethodAttr) (yypvt[-5].methAttr | mdPinvokeImpl); } break;
case 168:
#line 508 "asmparse.y"
{ PASM->SetPinvoke(new BinStr(),0,NULL,yypvt[-1].pinvAttr); 
                                                              yyval.methAttr = (CorMethodAttr) (yypvt[-4].methAttr | mdPinvokeImpl); } break;
case 169:
#line 512 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) 0; } break;
case 170:
#line 513 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmNoMangle); } break;
case 171:
#line 514 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCharSetAnsi); } break;
case 172:
#line 515 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCharSetUnicode); } break;
case 173:
#line 516 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCharSetAuto); } break;
case 174:
#line 517 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmSupportsLastError); } break;
case 175:
#line 518 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCallConvWinapi); } break;
case 176:
#line 519 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCallConvCdecl); } break;
case 177:
#line 520 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCallConvStdcall); } break;
case 178:
#line 521 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCallConvThiscall); } break;
case 179:
#line 522 "asmparse.y"
{ yyval.pinvAttr = (CorPinvokeMap) (yypvt[-1].pinvAttr | pmCallConvFastcall); } break;
case 180:
#line 525 "asmparse.y"
{ yyval.string = newString(COR_CTOR_METHOD_NAME); } break;
case 181:
#line 526 "asmparse.y"
{ yyval.string = newString(COR_CCTOR_METHOD_NAME); } break;
case 182:
#line 527 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 183:
#line 530 "asmparse.y"
{ yyval.int32 = 0; } break;
case 184:
#line 531 "asmparse.y"
{ yyval.int32 = yypvt[-3].int32 | pdIn; } break;
case 185:
#line 532 "asmparse.y"
{ yyval.int32 = yypvt[-3].int32 | pdOut; } break;
case 186:
#line 533 "asmparse.y"
{ yyval.int32 = yypvt[-3].int32 | pdOptional; } break;
case 187:
#line 534 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 + 1; } break;
case 188:
#line 537 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) 0; } break;
case 189:
#line 538 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) (yypvt[-1].fieldAttr | fdStatic); } break;
case 190:
#line 539 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdPublic); } break;
case 191:
#line 540 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdPrivate); } break;
case 192:
#line 541 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdFamily); } break;
case 193:
#line 542 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) (yypvt[-1].fieldAttr | fdInitOnly); } break;
case 194:
#line 543 "asmparse.y"
{ yyval.fieldAttr = yypvt[-1].fieldAttr; } break;
case 195:
#line 544 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) (yypvt[-1].fieldAttr | fdSpecialName); } break;
case 196:
#line 557 "asmparse.y"
{ PASM->m_pMarshal = yypvt[-1].binstr; } break;
case 197:
#line 558 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdAssembly); } break;
case 198:
#line 559 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdFamANDAssem); } break;
case 199:
#line 560 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdFamORAssem); } break;
case 200:
#line 561 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) ((yypvt[-1].fieldAttr & ~mdMemberAccessMask) | fdPrivateScope); } break;
case 201:
#line 562 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) (yypvt[-1].fieldAttr | fdLiteral); } break;
case 202:
#line 563 "asmparse.y"
{ yyval.fieldAttr = (CorFieldAttr) (yypvt[-1].fieldAttr | fdNotSerialized); } break;
case 203:
#line 566 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (miIL | miManaged); } break;
case 204:
#line 567 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) ((yypvt[-1].implAttr & 0xFFF4) | miNative); } break;
case 205:
#line 568 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) ((yypvt[-1].implAttr & 0xFFF4) | miIL); } break;
case 206:
#line 569 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) ((yypvt[-1].implAttr & 0xFFF4) | miOPTIL); } break;
case 207:
#line 570 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) ((yypvt[-1].implAttr & 0xFFFB) | miManaged); } break;
case 208:
#line 571 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) ((yypvt[-1].implAttr & 0xFFFB) | miUnmanaged); } break;
case 209:
#line 572 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miForwardRef); } break;
case 210:
#line 573 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miPreserveSig); } break;
case 211:
#line 574 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miRuntime); } break;
case 212:
#line 575 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miInternalCall); } break;
case 213:
#line 576 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miSynchronized); } break;
case 214:
#line 577 "asmparse.y"
{ yyval.implAttr = (CorMethodImpl) (yypvt[-1].implAttr | miNoInlining); } break;
case 215:
#line 580 "asmparse.y"
{ PASM->delArgNameList(PASM->m_firstArgName); PASM->m_firstArgName = NULL; } break;
case 216:
#line 584 "asmparse.y"
{ PASM->EmitByte(yypvt[-0].int32); } break;
case 217:
#line 586 "asmparse.y"
{ delete PASM->m_SEHD; PASM->m_SEHD = PASM->m_SEHDstack.POP(); } break;
case 218:
#line 587 "asmparse.y"
{ PASM->EmitMaxStack(yypvt[-0].int32); } break;
case 219:
#line 588 "asmparse.y"
{ PASM->EmitLocals(parser->MakeSig(IMAGE_CEE_CS_CALLCONV_LOCAL_SIG, 0, yypvt[-1].binstr)); } break;
case 220:
#line 589 "asmparse.y"
{ PASM->EmitZeroInit(); 
                                                              PASM->EmitLocals(parser->MakeSig(IMAGE_CEE_CS_CALLCONV_LOCAL_SIG, 0, yypvt[-1].binstr)); } break;
case 221:
#line 591 "asmparse.y"
{ PASM->EmitEntryPoint(); } break;
case 222:
#line 592 "asmparse.y"
{ PASM->EmitZeroInit(); } break;
case 225:
#line 595 "asmparse.y"
{ PASM->EmitLabel(yypvt[-1].string); } break;
case 230:
#line 600 "asmparse.y"
{ if(PASM->m_pCurMethod->m_dwExportOrdinal == 0xFFFFFFFF)
                                                                  PASM->m_pCurMethod->m_dwExportOrdinal = yypvt[-1].int32;
                                                              else
                                                                  PASM->report->warn("Duplicate .export directive, ignored\n");
                                                            } break;
case 231:
#line 605 "asmparse.y"
{ if(PASM->m_pCurMethod->m_dwExportOrdinal == 0xFFFFFFFF)
                                                              {
                                                                  PASM->m_pCurMethod->m_dwExportOrdinal = yypvt[-3].int32;
                                                                  PASM->m_pCurMethod->m_szExportAlias = yypvt[-0].string;
                                                              }
                                                              else
                                                                  PASM->report->warn("Duplicate .export directive, ignored\n");
                                                            } break;
case 232:
#line 613 "asmparse.y"
{ PASM->m_pCurMethod->m_wVTEntry = (WORD)yypvt[-2].int32;
                                                              PASM->m_pCurMethod->m_wVTSlot = (WORD)yypvt[-0].int32; } break;
case 233:
#line 616 "asmparse.y"
{ PASM->AddMethodImpl(yypvt[-2].binstr,yypvt[-0].string,NULL,NULL,NULL); } break;
case 235:
#line 619 "asmparse.y"
{ if( yypvt[-2].int32 ) {
                                                                ARG_NAME_LIST* pAN=PASM->findArg(PASM->m_pCurMethod->m_firstArgName, yypvt[-2].int32 - 1);
                                                                if(pAN)
                                                                {
                                                                    PASM->m_pCustomDescrList = &(pAN->CustDList);
                                                                    pAN->pValue = yypvt[-0].binstr;
                                                                }
                                                                else
                                                                {
                                                                    PASM->m_pCustomDescrList = NULL;
                                                                    if(yypvt[-0].binstr) delete yypvt[-0].binstr;
                                                                }
                                                              } else {
                                                                PASM->m_pCustomDescrList = &(PASM->m_pCurMethod->m_RetCustDList);
                                                                PASM->m_pCurMethod->m_pRetValue = yypvt[-0].binstr;
                                                              }
                                                              PASM->m_tkCurrentCVOwner = 0;
                                                            } break;
case 236:
#line 639 "asmparse.y"
{ PASM->m_pCurMethod->CloseScope(); } break;
case 237:
#line 642 "asmparse.y"
{ PASM->m_pCurMethod->OpenScope(); } break;
case 241:
#line 652 "asmparse.y"
{ PASM->m_SEHD->tryTo = PASM->m_CurPC; } break;
case 242:
#line 653 "asmparse.y"
{ PASM->SetTryLabels(yypvt[-2].string, yypvt[-0].string); } break;
case 243:
#line 654 "asmparse.y"
{ if(PASM->m_SEHD) {PASM->m_SEHD->tryFrom = yypvt[-2].int32;
                                                              PASM->m_SEHD->tryTo = yypvt[-0].int32;} } break;
case 244:
#line 658 "asmparse.y"
{ PASM->NewSEHDescriptor();
                                                              PASM->m_SEHD->tryFrom = PASM->m_CurPC; } break;
case 245:
#line 663 "asmparse.y"
{ PASM->EmitTry(); } break;
case 246:
#line 664 "asmparse.y"
{ PASM->EmitTry(); } break;
case 247:
#line 665 "asmparse.y"
{ PASM->EmitTry(); } break;
case 248:
#line 666 "asmparse.y"
{ PASM->EmitTry(); } break;
case 249:
#line 670 "asmparse.y"
{ PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 250:
#line 671 "asmparse.y"
{ PASM->SetFilterLabel(yypvt[-0].string); 
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 251:
#line 673 "asmparse.y"
{ PASM->m_SEHD->sehFilter = yypvt[-0].int32; 
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 252:
#line 677 "asmparse.y"
{ PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_FILTER;
                                                               PASM->m_SEHD->sehFilter = PASM->m_CurPC; } break;
case 253:
#line 681 "asmparse.y"
{ PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_NONE;
                                                               PASM->SetCatchClass(yypvt[-0].string); 
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 254:
#line 686 "asmparse.y"
{ PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_FINALLY;
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 255:
#line 690 "asmparse.y"
{ PASM->m_SEHD->sehClause = COR_ILEXCEPTION_CLAUSE_FAULT;
                                                               PASM->m_SEHD->sehHandler = PASM->m_CurPC; } break;
case 256:
#line 694 "asmparse.y"
{ PASM->m_SEHD->sehHandlerTo = PASM->m_CurPC; } break;
case 257:
#line 695 "asmparse.y"
{ PASM->SetHandlerLabels(yypvt[-2].string, yypvt[-0].string); } break;
case 258:
#line 696 "asmparse.y"
{ PASM->m_SEHD->sehHandler = yypvt[-2].int32;
                                                               PASM->m_SEHD->sehHandlerTo = yypvt[-0].int32; } break;
case 262:
#line 708 "asmparse.y"
{ PASM->EmitDataLabel(yypvt[-1].string); } break;
case 264:
#line 712 "asmparse.y"
{ PASM->SetDataSection(); } break;
case 265:
#line 713 "asmparse.y"
{ PASM->SetTLSSection(); } break;
case 270:
#line 724 "asmparse.y"
{ yyval.int32 = 1; } break;
case 271:
#line 725 "asmparse.y"
{ FAIL_UNLESS(yypvt[-1].int32 > 0, ("Illegal item count: %d\n",yypvt[-1].int32)); yyval.int32 = yypvt[-1].int32; } break;
case 272:
#line 728 "asmparse.y"
{ PASM->EmitDataString(yypvt[-1].binstr); } break;
case 273:
#line 729 "asmparse.y"
{ PASM->EmitDD(yypvt[-1].string); } break;
case 274:
#line 730 "asmparse.y"
{ PASM->EmitData(yypvt[-1].binstr->ptr(),yypvt[-1].binstr->length()); } break;
case 275:
#line 732 "asmparse.y"
{ float f = (float) (*yypvt[-2].float64); float* pf = new float[yypvt[-0].int32];
                                                               for(int i=0; i < yypvt[-0].int32; i++) pf[i] = f;
                                                               PASM->EmitData(pf, sizeof(float)*yypvt[-0].int32); delete yypvt[-2].float64; delete pf; } break;
case 276:
#line 736 "asmparse.y"
{ double* pd = new double[yypvt[-0].int32];
                                                               for(int i=0; i<yypvt[-0].int32; i++) pd[i] = *(yypvt[-2].float64);
                                                               PASM->EmitData(pd, sizeof(double)*yypvt[-0].int32); delete yypvt[-2].float64; delete pd; } break;
case 277:
#line 740 "asmparse.y"
{ __int64* pll = new __int64[yypvt[-0].int32];
                                                               for(int i=0; i<yypvt[-0].int32; i++) pll[i] = *(yypvt[-2].int64);
                                                               PASM->EmitData(pll, sizeof(__int64)*yypvt[-0].int32); delete yypvt[-2].int64; delete pll; } break;
case 278:
#line 744 "asmparse.y"
{ __int32* pl = new __int32[yypvt[-0].int32];
                                                               for(int i=0; i<yypvt[-0].int32; i++) pl[i] = yypvt[-2].int32;
                                                               PASM->EmitData(pl, sizeof(__int32)*yypvt[-0].int32); delete pl; } break;
case 279:
#line 748 "asmparse.y"
{ __int16 i = (__int16) yypvt[-2].int32; FAIL_UNLESS(i == yypvt[-2].int32, ("Value %d too big\n", yypvt[-2].int32));
                                                               __int16* ps = new __int16[yypvt[-0].int32];
                                                               for(int j=0; j<yypvt[-0].int32; j++) ps[j] = i;
                                                               PASM->EmitData(ps, sizeof(__int16)*yypvt[-0].int32); delete ps; } break;
case 280:
#line 753 "asmparse.y"
{ __int8 i = (__int8) yypvt[-2].int32; FAIL_UNLESS(i == yypvt[-2].int32, ("Value %d too big\n", yypvt[-2].int32));
                                                               __int8* pb = new __int8[yypvt[-0].int32];
                                                               for(int j=0; j<yypvt[-0].int32; j++) pb[j] = i;
                                                               PASM->EmitData(pb, sizeof(__int8)*yypvt[-0].int32); delete pb; } break;
case 281:
#line 757 "asmparse.y"
{ float* pf = new float[yypvt[-0].int32];
                                                               PASM->EmitData(pf, sizeof(float)*yypvt[-0].int32); delete pf; } break;
case 282:
#line 759 "asmparse.y"
{ double* pd = new double[yypvt[-0].int32];
                                                               PASM->EmitData(pd, sizeof(double)*yypvt[-0].int32); delete pd; } break;
case 283:
#line 761 "asmparse.y"
{ __int64* pll = new __int64[yypvt[-0].int32];
                                                               PASM->EmitData(pll, sizeof(__int64)*yypvt[-0].int32); delete pll; } break;
case 284:
#line 763 "asmparse.y"
{ __int32* pl = new __int32[yypvt[-0].int32];
                                                               PASM->EmitData(pl, sizeof(__int32)*yypvt[-0].int32); delete pl; } break;
case 285:
#line 765 "asmparse.y"
{ __int16* ps = new __int16[yypvt[-0].int32];
                                                               PASM->EmitData(ps, sizeof(__int16)*yypvt[-0].int32); delete ps; } break;
case 286:
#line 767 "asmparse.y"
{ __int8* pb = new __int8[yypvt[-0].int32];
                                                               PASM->EmitData(pb, sizeof(__int8)*yypvt[-0].int32); delete pb; } break;
case 287:
#line 771 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R4); 
                                                               float f = (float) (*yypvt[-1].float64); yyval.binstr->appendInt32(*((int*)&f)); delete yypvt[-1].float64; } break;
case 288:
#line 773 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R8); 
                                                               yyval.binstr->appendInt64((__int64 *)yypvt[-1].float64); delete yypvt[-1].float64; } break;
case 289:
#line 775 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R4); 
                                                               int f = *((int*)yypvt[-1].int64); 
                                                               yyval.binstr->appendInt32(f); delete yypvt[-1].int64; } break;
case 290:
#line 778 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R8); 
                                                               yyval.binstr->appendInt64((__int64 *)yypvt[-1].int64); delete yypvt[-1].int64; } break;
case 291:
#line 780 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I8); 
                                                               yyval.binstr->appendInt64((__int64 *)yypvt[-1].int64); delete yypvt[-1].int64; } break;
case 292:
#line 782 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I4); 
                                                               yyval.binstr->appendInt32(*((__int32*)yypvt[-1].int64)); delete yypvt[-1].int64;} break;
case 293:
#line 784 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I2); 
                                                               yyval.binstr->appendInt16(*((__int16*)yypvt[-1].int64)); delete yypvt[-1].int64;} break;
case 294:
#line 786 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_CHAR); 
                                                               yyval.binstr->appendInt16((int)*((unsigned __int16*)yypvt[-1].int64)); delete yypvt[-1].int64;} break;
case 295:
#line 788 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I1); 
                                                               yyval.binstr->appendInt8(*((__int8*)yypvt[-1].int64)); delete yypvt[-1].int64; } break;
case 296:
#line 790 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_BOOLEAN); 
                                                               yyval.binstr->appendInt8(yypvt[-1].int32);} break;
case 297:
#line 792 "asmparse.y"
{ yyval.binstr = BinStrToUnicode(yypvt[-0].binstr); yyval.binstr->insertInt8(ELEMENT_TYPE_STRING);} break;
case 298:
#line 793 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_STRING);
                                                               yyval.binstr->append(yypvt[-1].binstr); delete yypvt[-1].binstr;} break;
case 299:
#line 795 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_CLASS); 
                                                                yyval.binstr->appendInt32(0); } break;
case 300:
#line 799 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 301:
#line 802 "asmparse.y"
{ yyval.binstr = new BinStr(); } break;
case 302:
#line 803 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 303:
#line 806 "asmparse.y"
{ __int8 i = (__int8) yypvt[-0].int32; yyval.binstr = new BinStr(); yyval.binstr->appendInt8(i); } break;
case 304:
#line 807 "asmparse.y"
{ __int8 i = (__int8) yypvt[-0].int32; yyval.binstr = yypvt[-1].binstr; yyval.binstr->appendInt8(i); } break;
case 305:
#line 810 "asmparse.y"
{ yyval.instr = yypvt[-1].instr; bParsingByteArray = TRUE; } break;
case 306:
#line 813 "asmparse.y"
{ yyval.instr = yypvt[-0].instr; iOpcodeLen = PASM->OpcodeLen(yypvt[-0].instr); } break;
case 307:
#line 816 "asmparse.y"
{ palDummy = PASM->m_firstArgName;
                                                               PASM->m_firstArgName = NULL; } break;
case 308:
#line 820 "asmparse.y"
{ PASM->EmitOpcode(yypvt[-0].instr); } break;
case 309:
#line 821 "asmparse.y"
{ PASM->EmitInstrVar(yypvt[-1].instr, yypvt[-0].int32); } break;
case 310:
#line 822 "asmparse.y"
{ PASM->EmitInstrVarByName(yypvt[-1].instr, yypvt[-0].string); } break;
case 311:
#line 823 "asmparse.y"
{ PASM->EmitInstrI(yypvt[-1].instr, yypvt[-0].int32); } break;
case 312:
#line 824 "asmparse.y"
{ PASM->EmitInstrI8(yypvt[-1].instr, yypvt[-0].int64); } break;
case 313:
#line 825 "asmparse.y"
{ PASM->EmitInstrR(yypvt[-1].instr, yypvt[-0].float64); delete (yypvt[-0].float64);} break;
case 314:
#line 826 "asmparse.y"
{ double f = (double) (*yypvt[-0].int64); PASM->EmitInstrR(yypvt[-1].instr, &f); } break;
case 315:
#line 827 "asmparse.y"
{ unsigned L = yypvt[-1].binstr->length();
                                                               FAIL_UNLESS(L >= sizeof(float), ("%d hexbytes, must be at least %d\n",
                                                                           L,sizeof(float))); 
                                                               if(L < sizeof(float)) {YYERROR; } 
                                                               else {
                                                                   double f = (L >= sizeof(double)) ? *((double *)(yypvt[-1].binstr->ptr()))
                                                                                    : (double)(*(float *)(yypvt[-1].binstr->ptr())); 
                                                                   PASM->EmitInstrR(yypvt[-2].instr,&f); }
                                                               delete yypvt[-1].binstr; } break;
case 316:
#line 836 "asmparse.y"
{ PASM->EmitInstrBrOffset(yypvt[-1].instr, yypvt[-0].int32); } break;
case 317:
#line 837 "asmparse.y"
{ PASM->EmitInstrBrTarget(yypvt[-1].instr, yypvt[-0].string); } break;
case 318:
#line 839 "asmparse.y"
{ if(yypvt[-8].instr->opcode == CEE_NEWOBJ || yypvt[-8].instr->opcode == CEE_CALLVIRT)
                                                                   yypvt[-7].int32 = yypvt[-7].int32 | IMAGE_CEE_CS_CALLCONV_HASTHIS; 
                                                               mdToken mr = PASM->MakeMemberRef(yypvt[-5].binstr, yypvt[-3].string, parser->MakeSig(yypvt[-7].int32, yypvt[-6].binstr, yypvt[-1].binstr),PASM->OpcodeLen(yypvt[-8].instr));
                                                               PASM->EmitInstrI(yypvt[-8].instr,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             } break;
case 319:
#line 847 "asmparse.y"
{ if(yypvt[-6].instr->opcode == CEE_NEWOBJ || yypvt[-6].instr->opcode == CEE_CALLVIRT)
                                                                   yypvt[-5].int32 = yypvt[-5].int32 | IMAGE_CEE_CS_CALLCONV_HASTHIS; 
                                                               mdToken mr = PASM->MakeMemberRef(NULL, yypvt[-3].string, parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr),PASM->OpcodeLen(yypvt[-6].instr));
                                                               PASM->EmitInstrI(yypvt[-6].instr,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             } break;
case 320:
#line 855 "asmparse.y"
{ yypvt[-3].binstr->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               mdToken mr = PASM->MakeMemberRef(yypvt[-2].binstr, yypvt[-0].string, yypvt[-3].binstr, PASM->OpcodeLen(yypvt[-4].instr));
                                                               PASM->EmitInstrI(yypvt[-4].instr,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             } break;
case 321:
#line 862 "asmparse.y"
{ yypvt[-1].binstr->insertInt8(IMAGE_CEE_CS_CALLCONV_FIELD); 
                                                               mdToken mr = PASM->MakeMemberRef(NULL, yypvt[-0].string, yypvt[-1].binstr, PASM->OpcodeLen(yypvt[-2].instr));
                                                               PASM->EmitInstrI(yypvt[-2].instr,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             } break;
case 322:
#line 868 "asmparse.y"
{ mdToken mr = PASM->MakeTypeRef(yypvt[-0].binstr);
                                                               PASM->EmitInstrI(yypvt[-1].instr, mr); 
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                             } break;
case 323:
#line 873 "asmparse.y"
{ PASM->EmitInstrStringLiteral(yypvt[-1].instr, yypvt[-0].binstr,TRUE); } break;
case 324:
#line 875 "asmparse.y"
{ PASM->EmitInstrStringLiteral(yypvt[-3].instr, yypvt[-1].binstr,FALSE); } break;
case 325:
#line 877 "asmparse.y"
{ PASM->EmitInstrSig(yypvt[-5].instr, parser->MakeSig(yypvt[-4].int32, yypvt[-3].binstr, yypvt[-1].binstr)); } break;
case 326:
#line 878 "asmparse.y"
{ PASM->EmitInstrRVA(yypvt[-1].instr, yypvt[-0].string, TRUE); 
                                                               PASM->report->warn("Deprecated instruction 'ldptr'\n"); } break;
case 327:
#line 880 "asmparse.y"
{ PASM->EmitInstrRVA(yypvt[-1].instr, (char *)yypvt[-0].int32, FALSE); 
                                                               PASM->report->warn("Deprecated instruction 'ldptr'\n"); } break;
case 328:
#line 883 "asmparse.y"
{ mdToken mr = yypvt[-0].int32;
                                                               PASM->EmitInstrI(yypvt[-1].instr,mr);
                                                               PASM->m_tkCurrentCVOwner = mr;
                                                               PASM->m_pCustomDescrList = NULL;
                                                               iOpcodeLen = 0;
                                                             } break;
case 329:
#line 889 "asmparse.y"
{ PASM->EmitInstrSwitch(yypvt[-3].instr, yypvt[-1].labels); } break;
case 330:
#line 890 "asmparse.y"
{ PASM->EmitInstrPhi(yypvt[-1].instr, yypvt[-0].binstr); } break;
case 331:
#line 893 "asmparse.y"
{ yyval.binstr = new BinStr(); } break;
case 332:
#line 894 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr;} break;
case 333:
#line 897 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 334:
#line 898 "asmparse.y"
{ yyval.binstr = yypvt[-2].binstr; yyval.binstr->append(yypvt[-0].binstr); delete yypvt[-0].binstr; } break;
case 335:
#line 901 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_SENTINEL); } break;
case 336:
#line 902 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->append(yypvt[-0].binstr); PASM->addArgName("", yypvt[-0].binstr, NULL, yypvt[-1].int32); } break;
case 337:
#line 903 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->append(yypvt[-1].binstr); PASM->addArgName(yypvt[-0].string, yypvt[-1].binstr, NULL, yypvt[-2].int32); } break;
case 338:
#line 905 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->append(yypvt[-4].binstr); PASM->addArgName("", yypvt[-4].binstr, yypvt[-1].binstr, yypvt[-5].int32); } break;
case 339:
#line 907 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->append(yypvt[-5].binstr); PASM->addArgName(yypvt[-0].string, yypvt[-5].binstr, yypvt[-2].binstr, yypvt[-6].int32); } break;
case 340:
#line 910 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 341:
#line 911 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 342:
#line 912 "asmparse.y"
{ yyval.string = newStringWDel(yypvt[-2].string, newString("."), yypvt[-0].string); } break;
case 343:
#line 915 "asmparse.y"
{ yyval.string = newStringWDel(yypvt[-2].string, newString("^"), yypvt[-0].string); } break;
case 344:
#line 916 "asmparse.y"
{ yyval.string = newStringWDel(yypvt[-2].string, newString("~"), yypvt[-0].string); } break;
case 345:
#line 917 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 346:
#line 920 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 347:
#line 921 "asmparse.y"
{ yyval.string = newStringWDel(yypvt[-2].string, newString("/"), yypvt[-0].string); } break;
case 348:
#line 924 "asmparse.y"
{ unsigned len = (unsigned int)strlen(yypvt[-0].string)+1;
                                                                yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_NAME);
                                                                memcpy(yyval.binstr->getBuff(len), yypvt[-0].string, len);
                                                                delete yypvt[-0].string;
                                                              } break;
case 349:
#line 929 "asmparse.y"
{ unsigned len = (unsigned int)strlen(yypvt[-1].string);
                                                                yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_NAME);
                                                                memcpy(yyval.binstr->getBuff(len), yypvt[-1].string, len);
                                                                delete yypvt[-1].string;
                                                                yyval.binstr->appendInt8('^');
                                                                yyval.binstr->appendInt8(0); 
                                                              } break;
case 350:
#line 936 "asmparse.y"
{ unsigned len = (unsigned int)strlen(yypvt[-1].string);
                                                                yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_NAME);
                                                                memcpy(yyval.binstr->getBuff(len), yypvt[-1].string, len);
                                                                delete yypvt[-1].string;
                                                                yyval.binstr->appendInt8('~');
                                                                yyval.binstr->appendInt8(0); 
                                                              } break;
case 351:
#line 943 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 352:
#line 946 "asmparse.y"
{ yyval.int32 = (yypvt[-0].int32 | IMAGE_CEE_CS_CALLCONV_HASTHIS); } break;
case 353:
#line 947 "asmparse.y"
{ yyval.int32 = (yypvt[-0].int32 | IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS); } break;
case 354:
#line 948 "asmparse.y"
{ yyval.int32 = yypvt[-0].int32; } break;
case 355:
#line 951 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_DEFAULT; } break;
case 356:
#line 952 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_DEFAULT; } break;
case 357:
#line 953 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_VARARG; } break;
case 358:
#line 954 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_C; } break;
case 359:
#line 955 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_STDCALL; } break;
case 360:
#line 956 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_THISCALL; } break;
case 361:
#line 957 "asmparse.y"
{ yyval.int32 = IMAGE_CEE_CS_CALLCONV_FASTCALL; } break;
case 362:
#line 960 "asmparse.y"
{ yyval.binstr = new BinStr(); } break;
case 363:
#line 962 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_CUSTOMMARSHALER);
                                                                corEmitInt(yyval.binstr,yypvt[-7].binstr->length()); yyval.binstr->append(yypvt[-7].binstr);
                                                                corEmitInt(yyval.binstr,yypvt[-5].binstr->length()); yyval.binstr->append(yypvt[-5].binstr);
                                                                corEmitInt(yyval.binstr,yypvt[-3].binstr->length()); yyval.binstr->append(yypvt[-3].binstr);
                                                                corEmitInt(yyval.binstr,yypvt[-1].binstr->length()); yyval.binstr->append(yypvt[-1].binstr); 
                                                                PASM->report->warn("Deprecated 4-string form of custom marshaler, first two strings ignored\n");} break;
case 364:
#line 969 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_CUSTOMMARSHALER);
                                                                corEmitInt(yyval.binstr,0);
                                                                corEmitInt(yyval.binstr,0);
                                                                corEmitInt(yyval.binstr,yypvt[-3].binstr->length()); yyval.binstr->append(yypvt[-3].binstr);
                                                                corEmitInt(yyval.binstr,yypvt[-1].binstr->length()); yyval.binstr->append(yypvt[-1].binstr); } break;
case 365:
#line 974 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_FIXEDSYSSTRING);
                                                                corEmitInt(yyval.binstr,yypvt[-1].int32); } break;
case 366:
#line 976 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_FIXEDARRAY);
                                                                corEmitInt(yyval.binstr,yypvt[-1].int32); } break;
case 367:
#line 978 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_VARIANT); 
                                                                PASM->report->warn("Deprecated native type 'variant'\n"); } break;
case 368:
#line 980 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_CURRENCY); } break;
case 369:
#line 981 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_SYSCHAR); 
                                                                PASM->report->warn("Deprecated native type 'syschar'\n"); } break;
case 370:
#line 983 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_VOID); 
                                                                PASM->report->warn("Deprecated native type 'void'\n"); } break;
case 371:
#line 985 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_BOOLEAN); } break;
case 372:
#line 986 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_I1); } break;
case 373:
#line 987 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_I2); } break;
case 374:
#line 988 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_I4); } break;
case 375:
#line 989 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_I8); } break;
case 376:
#line 990 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_R4); } break;
case 377:
#line 991 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_R8); } break;
case 378:
#line 992 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_ERROR); } break;
case 379:
#line 993 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_U1); } break;
case 380:
#line 994 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_U2); } break;
case 381:
#line 995 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_U4); } break;
case 382:
#line 996 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_U8); } break;
case 383:
#line 997 "asmparse.y"
{ yyval.binstr = yypvt[-1].binstr; yyval.binstr->insertInt8(NATIVE_TYPE_PTR); 
                                                                PASM->report->warn("Deprecated native type '*'\n"); } break;
case 384:
#line 999 "asmparse.y"
{ yyval.binstr = yypvt[-2].binstr; if(yyval.binstr->length()==0) yyval.binstr->appendInt8(NATIVE_TYPE_MAX);
                                                                yyval.binstr->insertInt8(NATIVE_TYPE_ARRAY); } break;
case 385:
#line 1001 "asmparse.y"
{ yyval.binstr = yypvt[-3].binstr; if(yyval.binstr->length()==0) yyval.binstr->appendInt8(NATIVE_TYPE_MAX); 
                                                                yyval.binstr->insertInt8(NATIVE_TYPE_ARRAY);
                                                                corEmitInt(yyval.binstr,0);
                                                                corEmitInt(yyval.binstr,yypvt[-1].int32); } break;
case 386:
#line 1005 "asmparse.y"
{ yyval.binstr = yypvt[-5].binstr; if(yyval.binstr->length()==0) yyval.binstr->appendInt8(NATIVE_TYPE_MAX); 
                                                                yyval.binstr->insertInt8(NATIVE_TYPE_ARRAY);
                                                                corEmitInt(yyval.binstr,yypvt[-1].int32);
                                                                corEmitInt(yyval.binstr,yypvt[-3].int32); } break;
case 387:
#line 1009 "asmparse.y"
{ yyval.binstr = yypvt[-4].binstr; if(yyval.binstr->length()==0) yyval.binstr->appendInt8(NATIVE_TYPE_MAX); 
                                                                yyval.binstr->insertInt8(NATIVE_TYPE_ARRAY);
                                                                corEmitInt(yyval.binstr,yypvt[-1].int32); } break;
case 388:
#line 1012 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_DECIMAL); 
                                                                PASM->report->warn("Deprecated native type 'decimal'\n"); } break;
case 389:
#line 1014 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_DATE); 
                                                                PASM->report->warn("Deprecated native type 'date'\n"); } break;
case 390:
#line 1016 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_BSTR); } break;
case 391:
#line 1017 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_LPSTR); } break;
case 392:
#line 1018 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_LPWSTR); } break;
case 393:
#line 1019 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_LPTSTR); } break;
case 394:
#line 1020 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_OBJECTREF); 
                                                                PASM->report->warn("Deprecated native type 'objectref'\n"); } break;
case 395:
#line 1022 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_IUNKNOWN); } break;
case 396:
#line 1023 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_IDISPATCH); } break;
case 397:
#line 1024 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_STRUCT); } break;
case 398:
#line 1025 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_INTF); } break;
case 399:
#line 1026 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_SAFEARRAY); 
                                                                corEmitInt(yyval.binstr,yypvt[-0].int32); 
                                                                corEmitInt(yyval.binstr,0);} break;
case 400:
#line 1029 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_SAFEARRAY); 
                                                                corEmitInt(yyval.binstr,yypvt[-2].int32); 
                                                                corEmitInt(yyval.binstr,yypvt[-0].binstr->length()); yyval.binstr->append(yypvt[-0].binstr); } break;
case 401:
#line 1033 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_INT); } break;
case 402:
#line 1034 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_UINT); } break;
case 403:
#line 1035 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_NESTEDSTRUCT); 
                                                                PASM->report->warn("Deprecated native type 'nested struct'\n"); } break;
case 404:
#line 1037 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_BYVALSTR); } break;
case 405:
#line 1038 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_ANSIBSTR); } break;
case 406:
#line 1039 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_TBSTR); } break;
case 407:
#line 1040 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_VARIANTBOOL); } break;
case 408:
#line 1041 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_FUNC); } break;
case 409:
#line 1042 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_ASANY); } break;
case 410:
#line 1043 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(NATIVE_TYPE_LPSTRUCT); } break;
case 411:
#line 1046 "asmparse.y"
{ yyval.int32 = VT_EMPTY; } break;
case 412:
#line 1047 "asmparse.y"
{ yyval.int32 = VT_NULL; } break;
case 413:
#line 1048 "asmparse.y"
{ yyval.int32 = VT_VARIANT; } break;
case 414:
#line 1049 "asmparse.y"
{ yyval.int32 = VT_CY; } break;
case 415:
#line 1050 "asmparse.y"
{ yyval.int32 = VT_VOID; } break;
case 416:
#line 1051 "asmparse.y"
{ yyval.int32 = VT_BOOL; } break;
case 417:
#line 1052 "asmparse.y"
{ yyval.int32 = VT_I1; } break;
case 418:
#line 1053 "asmparse.y"
{ yyval.int32 = VT_I2; } break;
case 419:
#line 1054 "asmparse.y"
{ yyval.int32 = VT_I4; } break;
case 420:
#line 1055 "asmparse.y"
{ yyval.int32 = VT_I8; } break;
case 421:
#line 1056 "asmparse.y"
{ yyval.int32 = VT_R4; } break;
case 422:
#line 1057 "asmparse.y"
{ yyval.int32 = VT_R8; } break;
case 423:
#line 1058 "asmparse.y"
{ yyval.int32 = VT_UI1; } break;
case 424:
#line 1059 "asmparse.y"
{ yyval.int32 = VT_UI2; } break;
case 425:
#line 1060 "asmparse.y"
{ yyval.int32 = VT_UI4; } break;
case 426:
#line 1061 "asmparse.y"
{ yyval.int32 = VT_UI8; } break;
case 427:
#line 1062 "asmparse.y"
{ yyval.int32 = VT_PTR; } break;
case 428:
#line 1063 "asmparse.y"
{ yyval.int32 = yypvt[-2].int32 | VT_ARRAY; } break;
case 429:
#line 1064 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | VT_VECTOR; } break;
case 430:
#line 1065 "asmparse.y"
{ yyval.int32 = yypvt[-1].int32 | VT_BYREF; } break;
case 431:
#line 1066 "asmparse.y"
{ yyval.int32 = VT_DECIMAL; } break;
case 432:
#line 1067 "asmparse.y"
{ yyval.int32 = VT_DATE; } break;
case 433:
#line 1068 "asmparse.y"
{ yyval.int32 = VT_BSTR; } break;
case 434:
#line 1069 "asmparse.y"
{ yyval.int32 = VT_LPSTR; } break;
case 435:
#line 1070 "asmparse.y"
{ yyval.int32 = VT_LPWSTR; } break;
case 436:
#line 1071 "asmparse.y"
{ yyval.int32 = VT_UNKNOWN; } break;
case 437:
#line 1072 "asmparse.y"
{ yyval.int32 = VT_DISPATCH; } break;
case 438:
#line 1073 "asmparse.y"
{ yyval.int32 = VT_SAFEARRAY; } break;
case 439:
#line 1074 "asmparse.y"
{ yyval.int32 = VT_INT; } break;
case 440:
#line 1075 "asmparse.y"
{ yyval.int32 = VT_UINT; } break;
case 441:
#line 1076 "asmparse.y"
{ yyval.int32 = VT_ERROR; } break;
case 442:
#line 1077 "asmparse.y"
{ yyval.int32 = VT_HRESULT; } break;
case 443:
#line 1078 "asmparse.y"
{ yyval.int32 = VT_CARRAY; } break;
case 444:
#line 1079 "asmparse.y"
{ yyval.int32 = VT_USERDEFINED; } break;
case 445:
#line 1080 "asmparse.y"
{ yyval.int32 = VT_RECORD; } break;
case 446:
#line 1081 "asmparse.y"
{ yyval.int32 = VT_FILETIME; } break;
case 447:
#line 1082 "asmparse.y"
{ yyval.int32 = VT_BLOB; } break;
case 448:
#line 1083 "asmparse.y"
{ yyval.int32 = VT_STREAM; } break;
case 449:
#line 1084 "asmparse.y"
{ yyval.int32 = VT_STORAGE; } break;
case 450:
#line 1085 "asmparse.y"
{ yyval.int32 = VT_STREAMED_OBJECT; } break;
case 451:
#line 1086 "asmparse.y"
{ yyval.int32 = VT_STORED_OBJECT; } break;
case 452:
#line 1087 "asmparse.y"
{ yyval.int32 = VT_BLOB_OBJECT; } break;
case 453:
#line 1088 "asmparse.y"
{ yyval.int32 = VT_CF; } break;
case 454:
#line 1089 "asmparse.y"
{ yyval.int32 = VT_CLSID; } break;
case 455:
#line 1092 "asmparse.y"
{ if((strcmp(yypvt[-0].string,"System.String")==0) ||
                                                                   (strcmp(yypvt[-0].string,"mscorlib^System.String")==0))
                                                                {     yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_STRING); }
                                                                else if((strcmp(yypvt[-0].string,"System.Object")==0) ||
                                                                   (strcmp(yypvt[-0].string,"mscorlib^System.Object")==0))
                                                                {     yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_OBJECT); }
                                                                else yyval.binstr = parser->MakeTypeClass(ELEMENT_TYPE_CLASS, yypvt[-0].string); } break;
case 456:
#line 1099 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_OBJECT); } break;
case 457:
#line 1100 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_STRING); } break;
case 458:
#line 1101 "asmparse.y"
{ yyval.binstr = parser->MakeTypeClass(ELEMENT_TYPE_VALUETYPE, yypvt[-0].string); } break;
case 459:
#line 1102 "asmparse.y"
{ yyval.binstr = parser->MakeTypeClass(ELEMENT_TYPE_VALUETYPE, yypvt[-0].string); } break;
case 460:
#line 1103 "asmparse.y"
{ yyval.binstr = yypvt[-2].binstr; yyval.binstr->insertInt8(ELEMENT_TYPE_SZARRAY); } break;
case 461:
#line 1104 "asmparse.y"
{ yyval.binstr = parser->MakeTypeArray(yypvt[-3].binstr, yypvt[-1].binstr); } break;
case 462:
#line 1108 "asmparse.y"
{ yyval.binstr = yypvt[-1].binstr; yyval.binstr->insertInt8(ELEMENT_TYPE_BYREF); } break;
case 463:
#line 1109 "asmparse.y"
{ yyval.binstr = yypvt[-1].binstr; yyval.binstr->insertInt8(ELEMENT_TYPE_PTR); } break;
case 464:
#line 1110 "asmparse.y"
{ yyval.binstr = yypvt[-1].binstr; yyval.binstr->insertInt8(ELEMENT_TYPE_PINNED); } break;
case 465:
#line 1111 "asmparse.y"
{ yyval.binstr = parser->MakeTypeClass(ELEMENT_TYPE_CMOD_REQD, yypvt[-1].string);
                                                                yyval.binstr->append(yypvt[-4].binstr); } break;
case 466:
#line 1113 "asmparse.y"
{ yyval.binstr = parser->MakeTypeClass(ELEMENT_TYPE_CMOD_OPT, yypvt[-1].string);
                                                                yyval.binstr->append(yypvt[-4].binstr); } break;
case 467:
#line 1115 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_VAR); yyval.binstr->appendInt8(yypvt[-0].int32); 
                                                                PASM->report->warn("Deprecated type modifier '!'(ELEMENT_TYPE_VAR)\n"); } break;
case 468:
#line 1118 "asmparse.y"
{ yyval.binstr = parser->MakeSig(yypvt[-5].int32, yypvt[-4].binstr, yypvt[-1].binstr);
                                                                yyval.binstr->insertInt8(ELEMENT_TYPE_FNPTR); 
                                                                delete PASM->m_firstArgName;
                                                                PASM->m_firstArgName = palDummy;
                                                              } break;
case 469:
#line 1123 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_TYPEDBYREF); } break;
case 470:
#line 1124 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_CHAR); } break;
case 471:
#line 1125 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_VOID); } break;
case 472:
#line 1126 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_BOOLEAN); } break;
case 473:
#line 1127 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I1); } break;
case 474:
#line 1128 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I2); } break;
case 475:
#line 1129 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I4); } break;
case 476:
#line 1130 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I8); } break;
case 477:
#line 1131 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R4); } break;
case 478:
#line 1132 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R8); } break;
case 479:
#line 1133 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_U1); } break;
case 480:
#line 1134 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_U2); } break;
case 481:
#line 1135 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_U4); } break;
case 482:
#line 1136 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_U8); } break;
case 483:
#line 1137 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_I); } break;
case 484:
#line 1138 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_U); } break;
case 485:
#line 1139 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt8(ELEMENT_TYPE_R); } break;
case 486:
#line 1142 "asmparse.y"
{ yyval.binstr = yypvt[-0].binstr; } break;
case 487:
#line 1143 "asmparse.y"
{ yyval.binstr = yypvt[-2].binstr; yypvt[-2].binstr->append(yypvt[-0].binstr); delete yypvt[-0].binstr; } break;
case 488:
#line 1146 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt32(0x7FFFFFFF); yyval.binstr->appendInt32(0x7FFFFFFF);  } break;
case 489:
#line 1147 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt32(0x7FFFFFFF); yyval.binstr->appendInt32(0x7FFFFFFF);  } break;
case 490:
#line 1148 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt32(0); yyval.binstr->appendInt32(yypvt[-0].int32); } break;
case 491:
#line 1149 "asmparse.y"
{ FAIL_UNLESS(yypvt[-2].int32 <= yypvt[-0].int32, ("lower bound %d must be <= upper bound %d\n", yypvt[-2].int32, yypvt[-0].int32));
                                                                if (yypvt[-2].int32 > yypvt[-0].int32) { YYERROR; };        
                                                                yyval.binstr = new BinStr(); yyval.binstr->appendInt32(yypvt[-2].int32); yyval.binstr->appendInt32(yypvt[-0].int32-yypvt[-2].int32+1); } break;
case 492:
#line 1152 "asmparse.y"
{ yyval.binstr = new BinStr(); yyval.binstr->appendInt32(yypvt[-1].int32); yyval.binstr->appendInt32(0x7FFFFFFF); } break;
case 493:
#line 1155 "asmparse.y"
{ yyval.labels = 0; } break;
case 494:
#line 1156 "asmparse.y"
{ yyval.labels = new Labels(yypvt[-2].string, yypvt[-0].labels, TRUE); } break;
case 495:
#line 1157 "asmparse.y"
{ yyval.labels = new Labels((char *)yypvt[-2].int32, yypvt[-0].labels, FALSE); } break;
case 496:
#line 1158 "asmparse.y"
{ yyval.labels = new Labels(yypvt[-0].string, NULL, TRUE); } break;
case 497:
#line 1159 "asmparse.y"
{ yyval.labels = new Labels((char *)yypvt[-0].int32, NULL, FALSE); } break;
case 498:
#line 1163 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 499:
#line 1164 "asmparse.y"
{ yyval.string = yypvt[-0].string; } break;
case 500:
#line 1167 "asmparse.y"
{ yyval.binstr = new BinStr();  } break;
case 501:
#line 1168 "asmparse.y"
{ FAIL_UNLESS((yypvt[-0].int32 == (__int16) yypvt[-0].int32), ("Value %d too big\n", yypvt[-0].int32));
                                                                yyval.binstr = yypvt[-1].binstr; yyval.binstr->appendInt8(yypvt[-0].int32); yyval.binstr->appendInt8(yypvt[-0].int32 >> 8); } break;
case 502:
#line 1172 "asmparse.y"
{ yyval.int32 = (__int32)(*yypvt[-0].int64); delete yypvt[-0].int64; } break;
case 503:
#line 1175 "asmparse.y"
{ yyval.int64 = yypvt[-0].int64; } break;
case 504:
#line 1178 "asmparse.y"
{ yyval.float64 = yypvt[-0].float64; } break;
case 505:
#line 1179 "asmparse.y"
{ float f; *((__int32*) (&f)) = yypvt[-1].int32; yyval.float64 = new double(f); } break;
case 506:
#line 1180 "asmparse.y"
{ yyval.float64 = (double*) yypvt[-1].int64; } break;
case 507:
#line 1184 "asmparse.y"
{ PASM->AddPermissionDecl(yypvt[-4].secAct, yypvt[-3].binstr, yypvt[-1].pair); } break;
case 508:
#line 1185 "asmparse.y"
{ PASM->AddPermissionDecl(yypvt[-1].secAct, yypvt[-0].binstr, NULL); } break;
case 509:
#line 1186 "asmparse.y"
{ PASM->AddPermissionSetDecl(yypvt[-2].secAct, yypvt[-1].binstr); } break;
case 510:
#line 1189 "asmparse.y"
{ yyval.secAct = yypvt[-2].secAct; bParsingByteArray = TRUE; } break;
case 511:
#line 1192 "asmparse.y"
{ yyval.pair = yypvt[-0].pair; } break;
case 512:
#line 1193 "asmparse.y"
{ yyval.pair = yypvt[-2].pair->Concat(yypvt[-0].pair); } break;
case 513:
#line 1196 "asmparse.y"
{ yypvt[-2].binstr->appendInt8(0); yyval.pair = new NVPair(yypvt[-2].binstr, yypvt[-0].binstr); } break;
case 514:
#line 1199 "asmparse.y"
{ yyval.int32 = 1; } break;
case 515:
#line 1200 "asmparse.y"
{ yyval.int32 = 0; } break;
case 516:
#line 1203 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_BOOLEAN);
                                                                yyval.binstr->appendInt8(yypvt[-0].int32); } break;
case 517:
#line 1206 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_I4);
                                                                yyval.binstr->appendInt32(yypvt[-0].int32); } break;
case 518:
#line 1209 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_I4);
                                                                yyval.binstr->appendInt32(yypvt[-1].int32); } break;
case 519:
#line 1212 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_STRING);
                                                                yyval.binstr->append(yypvt[-0].binstr); delete yypvt[-0].binstr;
                                                                yyval.binstr->appendInt8(0); } break;
case 520:
#line 1216 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)yyval.binstr->getBuff((unsigned)strlen(yypvt[-5].string) + 1), yypvt[-5].string);
                                                                yyval.binstr->appendInt8(1);
                                                                yyval.binstr->appendInt32(yypvt[-1].int32); } break;
case 521:
#line 1221 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)yyval.binstr->getBuff((unsigned)strlen(yypvt[-5].string) + 1), yypvt[-5].string);
                                                                yyval.binstr->appendInt8(2);
                                                                yyval.binstr->appendInt32(yypvt[-1].int32); } break;
case 522:
#line 1226 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)yyval.binstr->getBuff((unsigned)strlen(yypvt[-5].string) + 1), yypvt[-5].string);
                                                                yyval.binstr->appendInt8(4);
                                                                yyval.binstr->appendInt32(yypvt[-1].int32); } break;
case 523:
#line 1231 "asmparse.y"
{ yyval.binstr = new BinStr();
                                                                yyval.binstr->appendInt8(SERIALIZATION_TYPE_ENUM);
                                                                strcpy((char *)yyval.binstr->getBuff((unsigned)strlen(yypvt[-3].string) + 1), yypvt[-3].string);
                                                                yyval.binstr->appendInt8(4);
                                                                yyval.binstr->appendInt32(yypvt[-1].int32); } break;
case 524:
#line 1238 "asmparse.y"
{ yyval.secAct = dclRequest; } break;
case 525:
#line 1239 "asmparse.y"
{ yyval.secAct = dclDemand; } break;
case 526:
#line 1240 "asmparse.y"
{ yyval.secAct = dclAssert; } break;
case 527:
#line 1241 "asmparse.y"
{ yyval.secAct = dclDeny; } break;
case 528:
#line 1242 "asmparse.y"
{ yyval.secAct = dclPermitOnly; } break;
case 529:
#line 1243 "asmparse.y"
{ yyval.secAct = dclLinktimeCheck; } break;
case 530:
#line 1244 "asmparse.y"
{ yyval.secAct = dclInheritanceCheck; } break;
case 531:
#line 1245 "asmparse.y"
{ yyval.secAct = dclRequestMinimum; } break;
case 532:
#line 1246 "asmparse.y"
{ yyval.secAct = dclRequestOptional; } break;
case 533:
#line 1247 "asmparse.y"
{ yyval.secAct = dclRequestRefuse; } break;
case 534:
#line 1248 "asmparse.y"
{ yyval.secAct = dclPrejitGrant; } break;
case 535:
#line 1249 "asmparse.y"
{ yyval.secAct = dclPrejitDenied; } break;
case 536:
#line 1250 "asmparse.y"
{ yyval.secAct = dclNonCasDemand; } break;
case 537:
#line 1251 "asmparse.y"
{ yyval.secAct = dclNonCasLinkDemand; } break;
case 538:
#line 1252 "asmparse.y"
{ yyval.secAct = dclNonCasInheritance; } break;
case 539:
#line 1255 "asmparse.y"
{ bExternSource = TRUE; nExtLine = yypvt[-1].int32; nExtCol=1;
                                                                PASM->SetSourceFileName(yypvt[-0].string); delete yypvt[-0].string;} break;
case 540:
#line 1257 "asmparse.y"
{ bExternSource = TRUE; nExtLine = yypvt[-0].int32; nExtCol=1;} break;
case 541:
#line 1258 "asmparse.y"
{ bExternSource = TRUE; nExtLine = yypvt[-3].int32; nExtCol=yypvt[-1].int32;
                                                                PASM->SetSourceFileName(yypvt[-0].string); delete yypvt[-0].string;} break;
case 542:
#line 1260 "asmparse.y"
{ bExternSource = TRUE; nExtLine = yypvt[-2].int32; nExtCol=yypvt[-0].int32;} break;
case 543:
#line 1261 "asmparse.y"
{ bExternSource = TRUE; nExtLine = yypvt[-1].int32; nExtCol=1;
                                                                PASM->SetSourceFileName(yypvt[-0].binstr); delete yypvt[-0].binstr; } break;
case 544:
#line 1266 "asmparse.y"
{ PASMM->AddFile(yypvt[-5].string, yypvt[-6].fileAttr|yypvt[-4].fileAttr|yypvt[-0].fileAttr, yypvt[-2].binstr); } break;
case 545:
#line 1267 "asmparse.y"
{ PASMM->AddFile(yypvt[-1].string, yypvt[-2].fileAttr|yypvt[-0].fileAttr, NULL); } break;
case 546:
#line 1270 "asmparse.y"
{ yyval.fileAttr = (CorFileFlags) 0; } break;
case 547:
#line 1271 "asmparse.y"
{ yyval.fileAttr = (CorFileFlags) (yypvt[-1].fileAttr | ffContainsNoMetaData); } break;
case 548:
#line 1274 "asmparse.y"
{ yyval.fileAttr = (CorFileFlags) 0; } break;
case 549:
#line 1275 "asmparse.y"
{ yyval.fileAttr = (CorFileFlags) 0x80000000; } break;
case 550:
#line 1278 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 551:
#line 1281 "asmparse.y"
{ PASMM->StartAssembly(yypvt[-0].string, NULL, (DWORD)yypvt[-1].asmAttr, FALSE); } break;
case 552:
#line 1284 "asmparse.y"
{ yyval.asmAttr = (CorAssemblyFlags) 0; } break;
case 553:
#line 1285 "asmparse.y"
{ yyval.asmAttr = (CorAssemblyFlags) (yypvt[-1].asmAttr | afNonSideBySideAppDomain); } break;
case 554:
#line 1286 "asmparse.y"
{ yyval.asmAttr = (CorAssemblyFlags) (yypvt[-1].asmAttr | afNonSideBySideProcess); } break;
case 555:
#line 1287 "asmparse.y"
{ yyval.asmAttr = (CorAssemblyFlags) (yypvt[-1].asmAttr | afNonSideBySideMachine); } break;
case 558:
#line 1294 "asmparse.y"
{ PASMM->SetAssemblyHashAlg(yypvt[-0].int32); } break;
case 561:
#line 1299 "asmparse.y"
{ PASMM->SetAssemblyPublicKey(yypvt[-1].binstr); } break;
case 562:
#line 1301 "asmparse.y"
{ PASMM->SetAssemblyVer((USHORT)yypvt[-6].int32, (USHORT)yypvt[-4].int32, (USHORT)yypvt[-2].int32, (USHORT)yypvt[-0].int32); } break;
case 563:
#line 1302 "asmparse.y"
{ yypvt[-0].binstr->appendInt8(0); PASMM->SetAssemblyLocale(yypvt[-0].binstr,TRUE); } break;
case 564:
#line 1303 "asmparse.y"
{ PASMM->SetAssemblyLocale(yypvt[-1].binstr,FALSE); } break;
case 566:
#line 1307 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 567:
#line 1310 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 568:
#line 1313 "asmparse.y"
{ bParsingByteArray = TRUE; } break;
case 569:
#line 1316 "asmparse.y"
{ PASMM->StartAssembly(yypvt[-0].string, NULL, 0, TRUE); } break;
case 570:
#line 1317 "asmparse.y"
{ PASMM->StartAssembly(yypvt[-2].string, yypvt[-0].string, 0, TRUE); } break;
case 573:
#line 1324 "asmparse.y"
{ PASMM->SetAssemblyHashBlob(yypvt[-1].binstr); } break;
case 575:
#line 1326 "asmparse.y"
{ PASMM->SetAssemblyPublicKeyToken(yypvt[-1].binstr); } break;
case 576:
#line 1329 "asmparse.y"
{ PASMM->StartComType(yypvt[-0].string, yypvt[-1].comtAttr);} break;
case 577:
#line 1332 "asmparse.y"
{ PASMM->StartComType(yypvt[-0].string, yypvt[-1].comtAttr); } break;
case 578:
#line 1335 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) 0; } break;
case 579:
#line 1336 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-1].comtAttr | tdNotPublic); } break;
case 580:
#line 1337 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-1].comtAttr | tdPublic); } break;
case 581:
#line 1338 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedPublic); } break;
case 582:
#line 1339 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedPrivate); } break;
case 583:
#line 1340 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedFamily); } break;
case 584:
#line 1341 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedAssembly); } break;
case 585:
#line 1342 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedFamANDAssem); } break;
case 586:
#line 1343 "asmparse.y"
{ yyval.comtAttr = (CorTypeAttr) (yypvt[-2].comtAttr | tdNestedFamORAssem); } break;
case 589:
#line 1350 "asmparse.y"
{ PASMM->SetComTypeFile(yypvt[-0].string); } break;
case 590:
#line 1351 "asmparse.y"
{ PASMM->SetComTypeComType(yypvt[-0].string); } break;
case 591:
#line 1352 "asmparse.y"
{ PASMM->SetComTypeClassTok(yypvt[-0].int32); } break;
case 593:
#line 1356 "asmparse.y"
{ PASMM->StartManifestRes(yypvt[-0].string, yypvt[-1].manresAttr); } break;
case 594:
#line 1359 "asmparse.y"
{ yyval.manresAttr = (CorManifestResourceFlags) 0; } break;
case 595:
#line 1360 "asmparse.y"
{ yyval.manresAttr = (CorManifestResourceFlags) (yypvt[-1].manresAttr | mrPublic); } break;
case 596:
#line 1361 "asmparse.y"
{ yyval.manresAttr = (CorManifestResourceFlags) (yypvt[-1].manresAttr | mrPrivate); } break;
case 599:
#line 1368 "asmparse.y"
{ PASMM->SetManifestResFile(yypvt[-2].string, (ULONG)yypvt[-0].int32); } break;
case 600:
#line 1369 "asmparse.y"
{ PASMM->SetManifestResAsmRef(yypvt[-0].string); } break;/* End of actions */
#line 329 "yypars.c"
            }
        }
        goto yystack;  /* stack new state and value */
    }
#ifdef _MSC_VER
#pragma warning(default:102)
#endif

#ifdef YYDUMP
YYLOCAL void YYNEAR YYPASCAL yydumpinfo(void)
{
    short stackindex;
    short valindex;

    //dump yys
    printf("short yys[%d] {\n", YYMAXDEPTH);
    for (stackindex = 0; stackindex < YYMAXDEPTH; stackindex++){
        if (stackindex)
            printf(", %s", stackindex % 10 ? "\0" : "\n");
        printf("%6d", yys[stackindex]);
        }
    printf("\n};\n");

    //dump yyv
    printf("YYSTYPE yyv[%d] {\n", YYMAXDEPTH);
    for (valindex = 0; valindex < YYMAXDEPTH; valindex++){
        if (valindex)
            printf(", %s", valindex % 5 ? "\0" : "\n");
        printf("%#*x", 3+sizeof(YYSTYPE), yyv[valindex]);
        }
    printf("\n};\n");
    }
#endif
