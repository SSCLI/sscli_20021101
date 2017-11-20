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
// ============================================================================
// File: cppmunge.c
//
// ============================================================================
// cppmunge reads in preprocessed C and C++ source files, munges their
// contents, and writes out a new version of the preprocessed file with
// assorted changes.
//
// cppmunge currently does these tasks:AddEscapeCharToString
//   -Converts wide strings (L"...") from native wchar_t characters to
//    two-byte characters, casting the result to a PAL-defined
//    wchar_t *.
//
// As input, it takes a relative or absolute path to the preprocessed
// file. It is intended to be called from a makefile.
//
// To test, create a file containing the following line:
//  WCHAR* p = L"te\x5544\033\x22\n";
// and run cppmunge on it.  The file should be replaced by this line on
// little-endian machines:
//  WCHAR* p = ((WCHAR *) ("t\000e\000\x55\x44\033\000\x22\000\n\000\0\000"));
// and on big-endian, with this:
//  WCHAR* p = ((WCHAR *) ("\000t\000e\x44\x55\000\033\000\x22\000\n\000\0"));


#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

typedef enum {
    eStateNormal,
    eStateEscape,
    eStateOctal,
    eStateHex,
    eStateNotInString,
    eStateNewStringPending,
    eStateDone
} CharacterState;

typedef enum {
    eModeLiteral,       // L"foo"
    eModeSizeOf,        // sizeof(L"foo")
    eModeInitializer    // WCHAR bar[] = L"foo"
} ConvertMode;

static int ProcessFile(const char *filename);
static int ConvertFile(const char *filename, FILE *file);
static int CopyFile(const char *source, const char *destination);
static int ConvertCharacters(FILE *original, FILE *destination);
static int ConvertOneString(FILE *original, FILE *destination, ConvertMode mode);
static FILE *GetTempFile(char nameBuffer[], int bufferLength);
static int AddCharToString(char *string, int stringLength, char ch, ConvertMode mode);
static int AddEscapeCharToString(char *string, int stringLength,
                                    char escapedChar[],
                                    int charLength,
                                    CharacterState state,
                                    ConvertMode mode);
static int IsBaseDigit(char ch, CharacterState type);
static int MaxSizeForBase(CharacterState base);
static char *CheckStringBufferSize(char *buffer, int *allocatedSize,
                                    int bytesUsed);
static int StripPCH(FILE *original, FILE *destination);

char SourceFile[PATH_MAX];
char PrecompiledHeader[PATH_MAX];

int main(int argc, char **argv) {
    if (argc != 4) {
Usage:
        fprintf(stderr, "Usage: cppmunge -s<SourceFile> -p[PrecompiledHeader] file\n");
        exit(1);
    }
    if (*argv[1] != '-' || *(argv[1]+1) != 's' || *(argv[1]+2) == '\0') {
        goto Usage;
    }
    if (realpath(argv[1]+2, SourceFile) == NULL) {
        fprintf(stderr, "Error: realpath(%s, ...) failed with %d %s\n",
                argv[1]+2, errno, strerror(errno));
        exit(1);
    }
    if (*argv[2] != '-' || *(argv[2]+1) != 'p') {
        goto Usage;
    }
    if (*(argv[2]+2) && realpath(argv[2]+2, PrecompiledHeader) == NULL) {
        fprintf(stderr, "Error: realpath(%s, ...) failed with %d %s\n",
                argv[2]+2, errno, strerror(errno));
        exit(1);
    }

    return ProcessFile(argv[3]);
}

/* ProcessFile
 * -----------
 * Reads the specified file, converts it, and writes it out. Returns 0 if
 * successful and an error code otherwise.
 */
static int ProcessFile(const char *filename) {
    FILE *file;
    int err;

    file = fopen(filename, "r");
    if (file == NULL) {
        return errno;
    }
    err = ConvertFile(filename, file);
    fclose(file);
    return err;
}

/* ConvertFile
 * -----------
 * Takes a filename and a FILE * to that file that is already opened for
 * reading. Creates a temporary file to write output to, and converts the
 * given file, character by character, writing the result into the
 * temporary file. The temporary file is renamed to the specified filename
 * on successful completion. If renaming fails, the specified filename is
 * overwritten with the contents of the temporary file. If renaming fails
 * or the function itself fails, the temporary file is deleted.
 *
 * Returns 0 on success and an error code otherwise.
 */
static int ConvertFile(const char *filename, FILE *file) {
    char tempFileName[PATH_MAX + 1];
    FILE *tempFile;
    int err;
    int removeErr;

    tempFile = GetTempFile(tempFileName, sizeof(tempFileName));
    if (tempFile == NULL) {
        return errno;
    }

    err = StripPCH(file, tempFile);
    if (err != 0) {
        fclose(tempFile);
        remove(tempFileName);
        return err;
    }

    err = ConvertCharacters(file, tempFile);
    if (err != 0) {
        fclose(tempFile);
        remove(tempFileName);
        return err;
    }

    // Close the files.
    fclose(tempFile);

    // Move the temporary file over the original.
    err = rename(tempFileName, filename);
    if (err != 0) {
        err = CopyFile(tempFileName, filename);
        removeErr = remove(tempFileName);
        if (err == 0 && removeErr != 0) {
            fprintf(stderr, "cppmunge: error: delete of %s failed\n", tempFileName);
            err = errno;
        }
    }
    return err;
}

/* CopyFile
 * --------
 * Copies the source file to the destination. Called if renaming doesn't
 * work. Returns 0 on success and an error code on failure.
 */
static int CopyFile(const char *source, const char *destination) {
    char buffer[1024];  // A good size
    int err;
    FILE *sourceFile;
    FILE *destinationFile;

    // Darn. Try to overwrite the original instead.
    sourceFile = fopen(source, "r");
    if (sourceFile == NULL) {
        return errno;
    }
    destinationFile = fopen(destination, "w");
    if (destinationFile == NULL) {
        return errno;
    }
    while (fgets(buffer, sizeof(buffer), sourceFile) != NULL) {
        err = fputs(buffer, destinationFile);
        if (err == EOF) {
            return errno;
        }
    }
    if (!feof(sourceFile)) {
        // fgets() returned NULL due to an error
        return errno;
    }
    fclose(destinationFile);
    fclose(sourceFile);
    return 0;
}

/* ConvertCharacters
 * -----------------
 * Takes two FILE *s, one to the original file that is opened for reading
 * and one to the destination that is open for writing. Copies the contents
 * of the original to the destination, converting wide strings in the
 * process. Returns 0 on success and an error code on failure.
 */
static int ConvertCharacters(FILE *original, FILE *destination) {
    int ch;
    int err;
    int escape;
    int previousCh;
    int matched;
    int terminator;
    ConvertMode mode;

    previousCh = EOF;
    err = 0;

    while ((ch = fgetc(original)) != EOF) {

    Restart:
        switch (ch) {
        case '\"':
        case '\'':
            // regular string or char - pass it through
            if (fputc(ch, destination) == EOF) {
                return ferror(destination);
            }
            terminator = ch;

            escape = 0;
            while ((ch = fgetc(original)) != EOF) {
                if (fputc(ch, destination) == EOF) {
                    return ferror(destination);
                }

                switch (ch) {
                case '\"':
                case '\'':
                    if (!escape) {
                        if (ch == terminator)
                            break;
                    }
                    escape = 0;
                    continue;
                case '\\':
                    escape = !escape;
                    continue;
                default:
                    escape = 0;
                    continue;
                }
                break;
            }
            previousCh = ch;
            break;

        case 'L':
        case 'l':
            mode = eModeLiteral;

        UnicodeString:
            previousCh = ch;

            if ((ch = fgetc(original)) == EOF)
                break;

            if (ch != '\"') {
                if (fputc(previousCh, destination) == EOF) {
                    return ferror(destination);
                }
                goto Restart;
            }

            // The start of a string
            err = ConvertOneString(original, destination, mode);
            previousCh = EOF;
            break;

#define trymatch(expected, ignorewhitespace) \
    matched = 0; \
    for (;;) { \
        if (ch == EOF) { \
            if ((ch = fgetc(original)) == EOF) \
                break; \
        } \
        if (ignorewhitespace && isspace(ch)) { \
            if (fputc(ch, destination) == EOF) { \
                return ferror(destination); \
            } \
            ch = EOF; \
            continue; \
        } \
        if (ch == expected) { \
            if (fputc(ch, destination) == EOF) { \
                return ferror(destination); \
            } \
            matched = 1; \
            ch = EOF; \
        } \
        break; \
    }

#define match(c, ignorewhitespace) \
    trymatch(c, ignorewhitespace); \
    if (!matched) goto Restart;

        case 's':
            // look for "sizeof ( L"

            if (isalnum(previousCh))
                goto FallBack;

            match('s', 0);
            match('i', 0);
            match('z', 0);
            match('e', 0);
            match('o', 0);
            match('f', 0);
            do {
                trymatch('(', 1);
            } while (matched);

            if (ch != 'l' && ch != 'L')
                goto Restart;

            mode = eModeSizeOf;
            goto UnicodeString;

        case '[':
            // look for "[ ] = ( L"

            match('[', 1);
            match(']', 1);
            match('=', 1);

            do {
                trymatch('(', 1);
            } while (matched);

            if (ch != 'l' && ch != 'L')
                goto Restart;

            mode = eModeInitializer;
            goto UnicodeString;

        default:
        FallBack:
            if (fputc(ch, destination) == EOF) {
                return ferror(destination);
            }
            previousCh = ch;
            break;
        }
    }

    return err;
}

/* ConvertOneString
 * ----------------
 * Takes in a FILE * to the original file and another FILE * to the
 * destination. Assumes that the previous two characters read from
 * the original file were L", so the mark is at the start of a wide
 * string. This function reads the rest of the string, including any
 * immediately following strings that should be concatenated with
 * it, and writes out a CLR-compatible version of the string. Whitespace
 * between concatenated strings will be skipped, but an equivalent number
 * of newlines will be written out if any are part of that whitespace.
 * Whitespace that follows a wide string will be decreased to a single
 * space.
 * Returns 0 on success and an error code on failure.
 */
static int ConvertOneString(FILE *original, FILE *destination, ConvertMode mode) {
    char *newString;
    int newStringLength;
    int ch = '\0';
    int bytesCopied;
    int err;
    char numberBuffer[8]; // A pending octal or hex number
    int numberOffset;
    CharacterState state;
    int newlines;
    int previousCh;
    int hasTrailingWhitespace;
    int i;

    err = 0;
    newStringLength = 1024;
    bytesCopied = 0;
    state = eStateNormal;
    numberOffset = 0;
    newlines = 0;
    previousCh = EOF;
    hasTrailingWhitespace = 0;

    newString = malloc(newStringLength);
    if (newString == NULL) {
        return errno;
    }

    while (err == 0 && state != eStateDone && (ch = fgetc(original)) != EOF) {
        switch (state) {
            case eStateNormal:
                if (ch == '\\') {
                    state = eStateEscape;
                } else if (ch == '"') {
                    // Done with the string. Read on till we
                    // know we don't have a concatenated string.
                    state = eStateNotInString;
                } else {
                    bytesCopied += AddCharToString(newString, bytesCopied, ch, mode);
                }
                break;
            case eStateEscape:
                if (IsBaseDigit(ch, eStateOctal)) {
                    // Octal number. Put it back so it gets handled
                    // in the octal case.
                    if (ungetc(ch, original) == EOF) {
                        err = EIO;  // As good as anything else
                    }
                    state = eStateOctal;
                    memset(numberBuffer, 0, sizeof(numberBuffer));
                } else if (ch == 'x') {
                    // Hex number.
                    state = eStateHex;
                    memset(numberBuffer, 0, sizeof(numberBuffer));
                } else {
                    // Escape sequences that aren't numbers are
                    // two bytes. Write those out as a single
                    // character.
                    char curCh = ch;
                    bytesCopied += AddEscapeCharToString(newString,
                                            bytesCopied, &curCh, 1,
                                            eStateNormal,
                                            mode);
                    state = eStateNormal;
                }
                break;
            case eStateOctal:
            case eStateHex:
                if (IsBaseDigit(ch, state) &&
                                numberOffset < MaxSizeForBase(state)) {
                    // Add the number to our buffer.
                       numberBuffer[numberOffset++] = ch;
                } else if (numberOffset == 0) {
                    // Weird.
                    err = EINVAL;
                } else {
                    // The number's done.
                    // Write it out and put the character back.
                    bytesCopied += AddEscapeCharToString(newString,
                                            bytesCopied,
                                            numberBuffer, numberOffset,
                                            state,
                                            mode);
                    numberOffset = 0;
                    state = eStateNormal;
                    if (ungetc(ch, original) == EOF) {
                        err = EIO;  // As good as anything else
                    }
                }
                break;
            case eStateNotInString:
                if (toupper(ch) == 'L') {
                    previousCh = ch;
                    state = eStateNewStringPending;
                } else if (ch == '\n') {
                    newlines++;
                } else if (!isspace(ch)) {
                    // Put the character back
                    if (ungetc(ch, original) == EOF) {
                        err = EIO;  // As good as anything else
                       }
                    // We're done!
                    state = eStateDone;
                } else {
                    // Bypass whitespace, but mark it for later.
                    hasTrailingWhitespace = 1;
                }
                break;
            case eStateNewStringPending:
                if (ch == '"') {
                    // New string. Add it to the previous one.
                    state = eStateNormal;
                } else {
                    // Write out the previous character.
                    if (fputc(previousCh, destination) != EOF) {
                        // Put the current one back.
                        if (ungetc(ch, original) == EOF) {
                            err = EIO;  // As good as anything else
                        }
                    } else {
                        err = ferror(destination);
                    }
                    // We're done!
                    state = eStateDone;
                }
                break;
            default:
                break;
        }
        // Reallocate our string if necessary
        newString = CheckStringBufferSize(newString, &newStringLength,
                                            bytesCopied);
        if (newString == NULL) {
            err = errno;
        }
    }

    if (ch == EOF) {
        // Hmm...that shouldn't have happened. We're only supposed
        // to run on preprocessor output, and the preprocessor will
        // catch an unterminated string constant.
        err = EIO;  // As good a value as any
    }

    if (err == 0) {
        // Write out the string. First, add the trailing null character.
        char zeroCh = '0';
        bytesCopied += AddEscapeCharToString(newString, bytesCopied,
                                                &zeroCh, 1, eStateNormal, mode);

        // Reallocate our string if necessary
        newString = CheckStringBufferSize(newString, &newStringLength,
                                         bytesCopied);
        if (newString == NULL) {
            err = errno;
        }
    }

    if (err == 0) {

        if (mode == eModeInitializer) {
            // eat the last comma
            bytesCopied -= 1;
        }
        else {
            // eat the last \000
            bytesCopied -= 4;
        }

        // Terminate the buffer so we can use it as a real string.
        newString[bytesCopied] = '\0';

        // Now, write it out.
        switch (mode) {
        case eModeLiteral:
            fprintf(destination, "((WCHAR *) (\"%s\"))", newString);
            break;

        case eModeSizeOf:
            fprintf(destination, "(\"%s\")", newString);
            break;

        case eModeInitializer:
            fprintf(destination, "{%s}", newString);
            break;

        default:
            break;
        }

        // If there's trailing whitespace, print that.
        if (hasTrailingWhitespace) {
            fprintf(destination, " ");
        }

        // And print all of the newlines.
        for(i = 0; i < newlines; i++) {
            fprintf(destination, "\n");
        }
    }

    return err;
}

/* GetTempFile
 * -----------
 * Returns a FILE * that points to a temporary file that is open for
 * writing. Fills in the given buffer with the path to that file. It
 * is recommended that the buffer be at least MAX_PATH + 1 bytes.
 * Returns NULL on failure, with errno set.
 */
static FILE *GetTempFile(char nameBuffer[], int bufferLength) {
    int tempFileDes;
    const char *tempDir;

    tempDir = getenv("TEMP");
    if (tempDir == NULL) {
        tempDir = getenv("TMP");
    }
    if (tempDir == NULL) {
        tempDir = "/tmp";
    }

    snprintf(nameBuffer, bufferLength - 1, "%s/cppmunge.XXXXXX", tempDir);
    tempFileDes = mkstemp(nameBuffer);
    if (tempFileDes == -1) {
        return NULL;
    }

    return fdopen(tempFileDes, "w");
}

/* AddCharToString
 * ---------------
 * Writes a two-byte version of the given character to the specified
 * location in the string buffer. This should not fail, so there is
 * no return value.
 *
 */
static int AddCharToString(char *string, int stringLength, char ch, ConvertMode mode) {
    int initialStringLength = stringLength;
    if (mode != eModeInitializer) {
#if BIGENDIAN
        // Octal zero
        string[stringLength++] = '\\';
        string[stringLength++] = '0';
        string[stringLength++] = '0';
        string[stringLength++] = '0';
        // The character
        string[stringLength++] = ch;
#else
        string[stringLength++] = ch;
        // Octal zero
        string[stringLength++] = '\\';
        string[stringLength++] = '0';
        string[stringLength++] = '0';
        string[stringLength++] = '0';
#endif
    }
    else {
        string[stringLength++] = '\'';
        if (ch == '\'') {
            string[stringLength++] = '\\';
        }
        string[stringLength++] = ch;
        string[stringLength++] = '\'';
        string[stringLength++] = ',';
    }
    return stringLength - initialStringLength;
}

/* AddEscapeCharToString
 * ---------------------
 * Writes each character in the given buffer to the specified location
 * in the string buffer, along with an appropriately-placed null
 * character. Appends a backslash to the string prior to writing any
 * characters in the buffer to produce an escape sequence in the
 * resulting string. If the state is eStateHex, the character buffer
 * is divided into two-character chunks, and each of those is written
 * as a separate hex character.
 *
 */
static int AddEscapeCharToString(char *string, int stringLength,
                                    char escapedChar[],
                                    int charLength,
                                    CharacterState state,
                                    ConvertMode mode) {
    int i;
    int initialStringLength = stringLength;

    if (mode != eModeInitializer) {
#if BIGENDIAN
        // Only write a zero if the last character to write isn't zero
        // and, if we're writing hex characters, we aren't about to write
        // an even block of four.
        if (escapedChar[charLength - 1] != '\0' &&
            (state != eStateHex || (charLength % 4 != 0))) {
            // Octal zero
            string[stringLength++] = '\\';
            string[stringLength++] = '0';
            string[stringLength++] = '0';
            string[stringLength++] = '0';
        }
#endif

        if (state == eStateHex) {
            // Put characters into the string four at a time.
            // If a pair is left over, add a trailing zero for it.
            // We only support hexadecimal characters that have
            // an even number of bytes.
            assert(charLength % 2 == 0);
            assert(charLength <= 4);
#if BIGENDIAN
            if (charLength == 4) {
                // Byte-swap the character
                char c;
                c = escapedChar[0];
                escapedChar[0] = escapedChar[2];
                escapedChar[2] = c;
                c = escapedChar[1];
                escapedChar[1] = escapedChar[3];
                escapedChar[3] = c;
            }
#endif
            for(i = 0; i < charLength; i++) {
                string[stringLength++] = '\\';
                string[stringLength++] = 'x';
                string[stringLength++] = escapedChar[i];
                i++;
                string[stringLength++] = escapedChar[i];
            }
        } else {
            string[stringLength++] = '\\';
            for(i = 0; i < charLength; i++) {
                string[stringLength++] = escapedChar[i];
            }
        }

#if !BIGENDIAN
        // Only append a zero if our last character was not zero
        // and, if we're writing hex characters, we didn't write
        // an even block of four.
        if (escapedChar[i - 1] != '\0' &&
            (state != eStateHex || (i % 4 != 0))) {
            // Octal zero
            string[stringLength++] = '\\';
            string[stringLength++] = '0';
            string[stringLength++] = '0';
            string[stringLength++] = '0';
        }
#endif
    }
    else {
        if (state == eStateHex) {
            string[stringLength++] = '0';
            string[stringLength++] = 'x';
            for(i = 0; i < charLength; i++) {
                string[stringLength++] = escapedChar[i];
            }
        } 
        else
        if (state == eStateOctal) {
            string[stringLength++] = '0';
            for(i = 0; i < charLength; i++) {
                string[stringLength++] = escapedChar[i];
            }
        }
        else {
            string[stringLength++] = '\'';
            string[stringLength++] = '\\';
            for(i = 0; i < charLength; i++) {
                string[stringLength++] = escapedChar[i];
            }
            string[stringLength++] = '\'';
        }

        string[stringLength++] = ',';
    }

    return stringLength - initialStringLength;
}

/* IsBaseDigit
 * -----------
 * Returns whether the character is valid for a number of the specified
 * type.
 */
static int IsBaseDigit(char ch, CharacterState type) {
    if (type == eStateOctal) {
        return (ch >= '0' && ch <= '7');
    } else if (type == eStateHex) {
        return isxdigit((unsigned char) ch);
    } else {
        return 0;
    }
}

/* MaxSizeForBase
 * --------------
 * Returns the maximum number of bytes after the escape sequence that
 * can be used to represent a character in the given base.
 */
static int MaxSizeForBase(CharacterState base) {
    if (base == eStateOctal) {
        return 3;
    } else if (base == eStateHex) {
        return 8;   /* OK, so it's technically infinite, but there's
                     * a limit on just how big of an infinity is
                     * likely. */
    } else {
        return 0;
    }
}

/* CheckStringBufferSize
 * ---------------------
 * Compares the size of the buffer to the size of the buffer's contents
 * and realloc's to twice the original size if the contents are within
 * six of the buffer size. Returns NULL and sets errno if reallocation
 * fails.
 */
static char *CheckStringBufferSize(char *buffer, int *allocatedSize,
                                    int bytesUsed) {
    char *newBuffer;

    // The actual max size of the margin should be 12 (an eight-digit hex 
    // character, such as * \x1234abcd, followed by a trailing null character 
    // to make a wide char)

    // We will be generous about the margin. 32 should cover 
    // any border case that we have missed.

    if (bytesUsed >= *allocatedSize - 32) {
        newBuffer = realloc(buffer, 2 * (*allocatedSize));
        if (newBuffer != NULL) {
            *allocatedSize *= 2;
        }
    } else {
        newBuffer = buffer;
    }
    return newBuffer;
}

static int StripPCH(FILE *original, FILE *destination)
{
    char LineBuffer[10240];
    char LineDirectiveFile[PATH_MAX];
    char *p;
    char *EndQuote;
    int FoundPrecompiledHeaderStart = 0;

    if (PrecompiledHeader[0] == '\0') {
        // No precompiled header specified
        return 0;
    }

    while (fgets(LineBuffer, sizeof(LineBuffer), original) != NULL) {

        if (LineBuffer[0] != '#' || LineBuffer[1] != ' ' || !isdigit((unsigned char) LineBuffer[2])) {
            // Not a line directive
            continue;
        }
        p = &LineBuffer[3];
        while (isdigit((unsigned char) *p)) {
            p++;
        }
        if (*p != ' ' || *(p+1) != '\"') {
            // A '# line' directive without a filename
            continue;
        }
        p+=2;
        EndQuote = strchr(p, '\"');
        if (!EndQuote) {
            fprintf(stderr, "Malformed directive '%s'\n", LineBuffer);
            return 1;
        }
        *EndQuote = '\0';
        if (realpath(p, LineDirectiveFile) == NULL) {
            fprintf(stderr, "Error: realpath(%s, ...) failed with %d %s\n",
                    p+2, errno, strerror(errno));
            return 1;
        }
        if (!strcmp(LineDirectiveFile, PrecompiledHeader)) {
            // Found the beginning of the precompiled header file
            FoundPrecompiledHeaderStart = 1;
        } else if (FoundPrecompiledHeaderStart &&
                   !strcmp(LineDirectiveFile, SourceFile)) {
            // Found the first line of the source file following
            // the end of the precompiled header.
            fprintf(destination, "# 1 \"%s\"\n", p);
            fprintf(destination, "# 1 \"%s\" 1\n", PrecompiledHeader);
            *EndQuote = '\"';
            fputs(LineBuffer, destination);
            fputc('\n', destination);
            return 0;
        }
    }
    // Hit end-of-file without finding the expected line directives
    if (!FoundPrecompiledHeaderStart) {
        fprintf(stderr, "Error: precompiled header filename not found\n");
    } else {
        fprintf(stderr, "Error: source filename not found after PCH name\n");
    }
    return 1;
}


