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
// File: numlit.h
//
// This file contains code from the CLR sources for parsing numeric literals.
// ===========================================================================


float id (float f) {
    return f;
}


////////////////////////////////////////////////////////////////////////////////
// Decimal constant code
//
// NOTE: This code was ripped off from CLR VM\COMNumber.cpp
//

#define NUMBER_MAXDIGITS 31

struct NUMBER {
    int precision;
    int scale;
    int sign;
    char digits[NUMBER_MAXDIGITS + 1];
};


void DecShiftLeft(DECIMAL* value)
{
    unsigned int c0 = DECIMAL_LO32(*value) & 0x80000000? 1: 0;
    unsigned int c1 = DECIMAL_MID32(*value) & 0x80000000? 1: 0;
    DECIMAL_LO32(*value) <<= 1;
    DECIMAL_MID32(*value) = DECIMAL_MID32(*value) << 1 | c0;
    DECIMAL_HI32(*value) = DECIMAL_HI32(*value) << 1 | c1;
}

int D32AddCarry(ULONG* value, unsigned int i)
{
    unsigned int v = *value;
    unsigned int sum = v + i;
    *value = sum;
    return sum < v || sum < i? 1: 0;
}

void DecAdd(DECIMAL* value, DECIMAL* d)
{
    if (D32AddCarry(&DECIMAL_LO32(*value), DECIMAL_LO32(*d))) {
        if (D32AddCarry(&DECIMAL_MID32(*value), 1)) {
            D32AddCarry(&DECIMAL_HI32(*value), 1);
        }
    }
    if (D32AddCarry(&DECIMAL_MID32(*value), DECIMAL_MID32(*d))) {
        D32AddCarry(&DECIMAL_HI32(*value), 1);
    }
    D32AddCarry(&DECIMAL_HI32(*value), DECIMAL_HI32(*d));
}

void DecMul10(DECIMAL* value)
{
    DECIMAL d = *value;
    DecShiftLeft(value);
    DecShiftLeft(value);
    DecAdd(value, &d);
    DecShiftLeft(value);
}

void DecAddInt32(DECIMAL* value, unsigned int i)
{
    if (D32AddCarry(&DECIMAL_LO32(*value), i)) {
        if (D32AddCarry(&DECIMAL_MID32(*value), 1)) {
            D32AddCarry(&DECIMAL_HI32(*value), 1);
        }
    }
}


int NumberToDecimal(NUMBER* number, DECIMAL* value)
{
    DECIMAL d;
    d.wReserved = 0;
    DECIMAL_SIGNSCALE(d) = 0;
    DECIMAL_HI32(d) = 0;
    DECIMAL_LO32(d) = 0;
    DECIMAL_MID32(d) = 0;
    char* p = number->digits;
    if (*p) {
        int e = number->scale;
        if (e > 29 || e <= -28) return 0;
        while ((e > 0 || *p && e > -28) &&
            (DECIMAL_HI32(d) < 0x19999999 || DECIMAL_HI32(d) == 0x19999999 &&
            (DECIMAL_MID32(d) < 0x99999999 || DECIMAL_MID32(d) == 0x99999999 &&
            (DECIMAL_LO32(d) < 0x99999999 || DECIMAL_LO32(d) == 0x99999999 &&
            *p <= '5')))) {
            DecMul10(&d);
            if (*p) DecAddInt32(&d, *p++ - '0');
            e--;
        }
        if (*p >= '5') {
            DecAddInt32(&d, 1);
            if ((DECIMAL_HI32(d) | DECIMAL_MID32(d) | DECIMAL_LO32(d)) == 0) {
                DECIMAL_HI32(d) = 0x19999999;
                DECIMAL_MID32(d) = 0x99999999;
                DECIMAL_LO32(d) = 0x9999999A;
                e++;
            }
        }
        if (e > 0) return 0;
        DECIMAL_SCALE(d) = -e;
        DECIMAL_SIGN(d) = number->sign? DECIMAL_NEG: 0;
    }
    *value = d;
    return 1;
}

#define STATE_DIGITS   0x0004
#define STATE_NONZERO  0x0008
#define STATE_DECIMAL  0x0010

int ParseNumber(char** str, NUMBER* number)
{
    number->scale = 0;
    number->sign = 0;

    int state = 0;
    char* p = *str;
    char ch = *p;

    int digCount = 0;
    int digEnd = 0;
    while (true) {
        if (ch >= '0' && ch <= '9') {
            state |= STATE_DIGITS;
            if (ch != '0' || state & STATE_NONZERO) {
                if (digCount < NUMBER_MAXDIGITS) {
                    number->digits[digCount++] = ch;
                    if (ch != '0') digEnd = digCount;
                }
                if (!(state & STATE_DECIMAL)) number->scale++;
                state |= STATE_NONZERO;
            }
            else if (state & STATE_DECIMAL) number->scale--;
        }
        else if (!(state & STATE_DECIMAL) && *p == '.') {
            state |= STATE_DECIMAL;
        }
        else {
            break;
        }
        ch = *++p;
    }

    int negExp = 0;
    number->precision = digEnd;
    number->digits[digEnd] = 0;
    if (state & STATE_DIGITS) {
        if (ch == 'E' || ch == 'e') {
            char* temp = p;
            ch = *++p;
            if (*p == '+')
            {
                ch = *++p;
            }
            else if (*p == '-')
            {
                ch = *++p;
                negExp = 1;
            }
            if (ch >= '0' && ch <= '9') {
                int exp = 0;
                do {
                    exp = exp * 10 + (ch - '0');
                    ch = *++p;
                } while (ch >= '0' && ch <= '9' && exp < 100);
                if (negExp) exp = -exp;
                number->scale += exp;
            }
            else {
                p = temp;
                ch = *p;
            }
        }
        if (!(state & STATE_NONZERO)) {
            number->scale = 0;
            number->sign = 0;
        }
        *str = p;
        return 1;
    }
    *str = p;
    return 0;
}

