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

// This tool scans through rotor_pal.h (must be in the current directory
// and finds all "#define ERROR_" lines.  For each of those, it calls
// Win32 FormatMessageW to find the error text, and produces a
// rotor_pal.satellite file containing the error text.  This satellite file
// can be used by the PAL on non-Win32 platforms.

#include <rotor_palrt.h>

#define DEFINE_PREFIX "#define "

#define STARTTAG    "/* STARTERRORCODES"
#define ENDTAG      "/* ENDERRORCODES"

char Line[4096];
WCHAR ErrorText[4096];

int __cdecl main(int argc, char *argv[]) {
    FILE *fpIn;
    FILE *fpOut;
    int LineNum;
    bool InErrorCodes;

    fpIn = fopen("rotor_pal.h", "r");
    if (!fpIn) {
        fprintf(stderr, 
                "Unable to open rotor_pal.h for read\n");
        return 1;
    }

    fpOut = fopen("rotor_pal.satellite", "w");
    if (!fpOut) {
        fprintf(stderr, 
                "Unable to open rotor_pal.satellite for write\n");
        return 1;
    }

    LineNum = 0;
    InErrorCodes = false;
    while (!feof(fpIn)) {
        char *p;
        int Value;
        DWORD dw;
        int i;

        LineNum++;
        fgets(Line, sizeof(Line), fpIn);

        if (!InErrorCodes) {
            if (strncmp(Line, STARTTAG, sizeof(STARTTAG)-1) == 0)
                InErrorCodes = true;
        }
        else {
            if (strncmp(Line, ENDTAG, sizeof(ENDTAG)-1) == 0)
                InErrorCodes = false;
        }

        if (!InErrorCodes) {
            // Line is not in the error codes block
            continue;
        }

        if (strncmp(Line, DEFINE_PREFIX, sizeof(DEFINE_PREFIX)-1)) {
            // Line doesn't start with "#define "
            continue;
        }

        p = strchr(Line+sizeof(DEFINE_PREFIX), ' ');
        if (!p) {
            // Line isn't "#define xxxx yyyy"
            continue;
        }

        Value = atoi(p+1);
        if (Value < 0 || Value > 65536) {
            fprintf(stderr,
                    "Win32 error value out of range at line %d\n", LineNum);
            return 1;
        }

        dw = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM|
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            Value,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            ErrorText,
                            sizeof(ErrorText)/sizeof(WCHAR),
                            NULL);
        if (!dw) {
            fprintf(stderr,
                    "FormatMessageW failed at line %d, Error=%d, LastError=%d\n", 
                    LineNum,
                    Value,
                    GetLastError());
        }

        i = WideCharToMultiByte(CP_UTF8,
                                0,
                                ErrorText,
                                -1,
                                Line,
                                sizeof(Line),
                                NULL,
                                NULL);
        if (!i) {
            fprintf(stderr,
                    "WCtoMB failed at line %d, LastError=%d\n",
                    LineNum,
                    GetLastError());
            return 1;
        }

        fprintf(fpOut, "%d \"%s\"\n",
                Value, Line);
    }
    fclose(fpOut);
    fclose(fpIn);
    return 0;
}
