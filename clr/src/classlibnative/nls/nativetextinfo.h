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
#ifndef __NATIVE_TEXTINFO_H
#define __NATIVE_TEXTINFO_H

typedef  P844_TABLE    PCASE;        // ptr to Lower or Upper Case table

class NativeTextInfo {
    public:
        NativeTextInfo(PCASE pUpperCase844, PCASE pLowerCase844, PCASE pTitleCase844);
        ~NativeTextInfo();
        void DeleteData();
        
        WCHAR  ChangeCaseChar(BOOL bIsToUpper, WCHAR wch);
        LPWSTR ChangeCaseString(BOOL bIsToUpper, int nStrLen, LPWSTR source, LPWSTR target);
        WCHAR GetTitleCaseChar(WCHAR wch);
        
    private:        
        inline WCHAR GetIncrValue(LPWORD p844Tbl, WCHAR wch)
        {
             return ((WCHAR)(wch + Traverse844Word(p844Tbl, wch)));
        }

        inline WCHAR GetLowerUpperCase(PCASE pCase844, WCHAR wch)
        {
            return (GetIncrValue(pCase844, wch));
        };
        
    private:
        PCASE   m_pUpperCase844;
        PCASE   m_pLowerCase844; 
        PCASE   m_pTitleCase844;
};
#endif
