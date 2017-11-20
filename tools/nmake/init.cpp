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
//  INIT.C -- routines to handle TOOLS.INI
//
// Purpose:
//  Module contains routines to deal with TOOLS.INI file. Functions in TOOLS.LIB
//  have not been used because NMAKE needs to be small and the overhead is too
//  much.

#include "precomp.h"
#ifdef _MSC_VER
#pragma hdrstop
#endif

//  findTag()
//
//  arguments:  tag     pointer to tag name to be searched for
//
//  actions:    reads tokens from file
//              whenever it sees a newline, checks the next token
//               to see if 1st char is opening paren
//              if no, reads and discards rest of line and
//               checks next token to see if it's newline or EOF
//               and if newline loops to check next token . . .
//              if yes ('[' found), looks on line for tag
//              if tag found, looks for closing paren
//              if ']' found, discards rest of line and returns
//              else keeps looking until end of file or error
//
//  returns:    if successful, returns TRUE
//              if tag never found, returns FALSE

BOOL
findTag(
    char *tag
    )
{
    BOOL endTag;                       // TRUE when find [...]
    size_t n;
    char *s;

    for (line = 0; fgets(buf, MAXBUF, file); ++line) {
        if (*buf == '[') {
            endTag = FALSE;
            for (s = _tcstok(buf+1," \t\n");
                 s && !endTag;
                 s = _tcstok(NULL," \t\n")
                ) {
                n = _tcslen(s) - 1;

                if (s[n] == ']') {
                    endTag = TRUE;
                    s[n] = '\0';
                }

                if (!_tcsicmp(s,tag)) {
                    return(TRUE);
                }
            }
        }
    }

    if (!feof(file)) {
        currentLine = line;
        makeError(0, CANT_READ_FILE);
    }

    return(FALSE);
}


//  tagOpen()
//
//  arguments:  where   pointer to name of environment variable
//                       containing path to search
//              name    pointer to name of initialization file
//              tag     pointer to name of tag to find in file
//
//  actions:    looks for file in current directory
//              if not found, looks in each dir in path (semicolons
//                separate each path from the next in the string)
//              if file is found and opened, looks for the given tag
//
//              (if ported to xenix, tagOpen() and searchPath()
//               should probably use access() and not findFirst().)
//
//  returns:    if file and tag are found, returns pointer to file,
//                opened for reading and positioned at the line
//                following the tag line
//              else returns NULL

BOOL
tagOpen(
    char *where,
    char *name,
    char *tag
    )
{
    char szPath[_MAX_PATH];

    // Look for 'name' in current directory
    if (_access(name, READ) != 0) {
	return FALSE;
    }
    strncpy(szPath, name, sizeof(szPath));

    if (!(file = FILEOPEN(szPath, "rt"))) {
        makeError(0, CANT_READ_FILE, szPath);
    }

    if (findTag(tag)) {
        return(TRUE);                   // look for tag in file
    }

    if (fclose(file) == EOF) {          // if tag not found, close
        makeError(0, ERROR_CLOSING_FILE, szPath);
    }

    return(FALSE);                      // file and pretend file not found
}



//  searchPath()
//
//  arguments:  p       pointer to string of paths to be searched
//              name    name of file being searched for
//
//  actions:    looks for name in current directory, then each
//                directory listed in string.
//
//  returns:    pointer to path spec of file found, else NULL
//
//  I don't use _tcstok() here because that modifies the string that it "token-
//  izes" and we cannot modify the environment-variable string.  I'd have to
//  make a local copy of the whole string, and then make another copy of each
//  directory to which I concatenate the filename to in order to test for the
//  file's existence.

char *
searchPath(
    char *p,
    char *name,
    void *findBuf,
    NMHANDLE *searchHandle
    )
{
    char *s;                           // since it's not in use


    // We use FindFirst() because the dateTime of file matters to us
    // We don't need it always but then access() probably uses findFirst()

    if (findFirst(name, findBuf, searchHandle)) {   // check current dir first
        return(makeString(name));
    }

    // Check if environment string is NULL.  Unnecessary if check is done
    // elsewhere, but it's more convenient and safer to do it here.

    if (p == NULL) {
        return(NULL);
    }

    for (s = buf; ;) {
		while (*p && '\"' == *p) {
			// Quotes should not be used in search paths. If we find any,
			// we ignore them. This way we can form the full path and the
			// filename without quotes and add an enclosing pair of quotes
			// later, if necessary.                            
			p++;
		}
        if (!*p || (*s = *p++) == ';') {    // found a dir separator
            if (s == buf) {                 // ignore ; w/out name
                if (*p) {
                    continue;
                }

                return(NULL);               // list exhausted ...
            }

            if (!IsPathSeparator(*(s-1))) {  // append path separator
                *s++ = PATH_SEPARATOR_CHAR;
            }

            *s = '\0';

            if (_tcspbrk(buf,"*?")) {      // wildcards not allowed
                s = buf;
                continue;
            }

            _tcscpy(s, name);              // append file name, zap

            if (findFirst(buf, findBuf, searchHandle)) {
                return(makeString(buf));
            }

            s = buf;                        // reset ptr to begin of
        }                                   // buf and check next dir
        else {
            ++s;                            // we keep copying chars
        }                                   //  until find ';' or '\0'
    }
}
