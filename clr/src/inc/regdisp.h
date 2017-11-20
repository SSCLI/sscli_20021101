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
#ifndef __REGDISP_H
#define __REGDISP_H


#if defined(_X86_)

typedef struct _REGDISPLAY {
    PCONTEXT pContext;          // points to current Context; either
                                // returned by GetContext or provided
                                // at exception time.

    DWORD * pEdi;
    DWORD * pEsi;
    DWORD * pEbx;
    DWORD * pEdx;
    DWORD * pEcx;
    DWORD * pEax;

    DWORD * pEbp;
    DWORD   SP;                 // (Esp) Stack Pointer
    SLOT  * pPC;                // processor neutral name

} REGDISPLAY;

inline LPVOID GetRegdisplaySP(REGDISPLAY *display) {
    return (LPVOID)(size_t)display->SP;
}

inline void SetRegdisplaySP(REGDISPLAY *display, LPVOID sp ) {
    (display->SP) = (DWORD)(size_t)sp;
}

inline LPVOID GetRegdisplayFP(REGDISPLAY *display) {
    return (LPVOID)(size_t)*(display->pEbp);
}

inline LPVOID GetRegdisplayFPAddress(REGDISPLAY *display) {
    return (LPVOID)display->pEbp;
}

inline LPVOID GetControlPC(REGDISPLAY *display) {
    return (LPVOID)(*(display->pPC));
}
inline void SetControlPC(REGDISPLAY* display, LPVOID ip)
{
    *(display->pPC) = (SLOT)ip;
}

// This function tells us if the given stack pointer is in one of the frames of the functions called by the given frame
inline BOOL IsInCalleesFrames(REGDISPLAY *display, LPVOID stackPointer) {
    return stackPointer < ((LPVOID)display->pPC);
}

#elif defined(_PPC_)

#ifndef NUM_CALLEESAVED_REGISTERS
#define NUM_CALLEESAVED_REGISTERS 19
#endif
#ifndef NUM_FLOAT_CALLEESAVED_REGISTERS
#define NUM_FLOAT_CALLEESAVED_REGISTERS 18
#endif

typedef struct _REGDISPLAY {
    PCONTEXT pContext;          // points to current Context; either
                                // returned by GetContext or provided
                                // at exception time.

    DWORD   * pR[NUM_CALLEESAVED_REGISTERS];        // r13 .. r31
    DOUBLE  * pF[NUM_FLOAT_CALLEESAVED_REGISTERS];  // fpr14 .. fpr31

    DWORD     CR;                                   // cr

    DWORD     SP;
    SLOT    * pPC;              // processor neutral name

} REGDISPLAY;

inline LPVOID GetRegdisplaySP(REGDISPLAY *display) {
    return (LPVOID)(size_t)display->SP;
}

inline void SetRegdisplaySP(REGDISPLAY *display, LPVOID sp ) {
    display->SP = (DWORD)(size_t)sp;
}

inline LPVOID GetRegdisplayFP(REGDISPLAY *display) {
    return (LPVOID)(size_t)*(display->pR[30 - 13]);
}

inline LPVOID GetRegdisplayFPAddress(REGDISPLAY *display) {
    return (LPVOID)display->pR[30 - 13];
}

inline LPVOID GetControlPC(REGDISPLAY *display) {
    return (LPVOID)(*(display->pPC));
}
inline void SetControlPC(REGDISPLAY* display, LPVOID ip)
{
    *(display->pPC) = (SLOT)ip;
}

// This function tells us if the given stack pointer is in one of the frames of the functions called by the given frame
inline BOOL IsInCalleesFrames(REGDISPLAY *display, LPVOID stackPointer) {
    return stackPointer < ((LPVOID)display->pPC);
}


#else // none of the above processors

PORTABILITY_WARNING("RegDisplay functions are not implemented on this platform.")

typedef struct _REGDISPLAY {
    PCONTEXT pContext;          // points to current Context
    size_t   SP;
    size_t * FramePtr;
    SLOT   * pPC;
} REGDISPLAY;

inline LPVOID GetControlPC(REGDISPLAY *display) {
    return (LPVOID) NULL;
}

inline LPVOID GetRegdisplaySP(REGDISPLAY *display) {
    return (LPVOID)display->SP;
}

inline void SetRegdisplaySP(REGDISPLAY *display, LPVOID sp ) {
    display->SP = (DWORD)(size_t)sp;
}

inline LPVOID GetRegdisplayFP(REGDISPLAY *display) {
    return (LPVOID)(size_t)*(display->FramePtr);
}

inline BOOL IsInCalleesFrames(REGDISPLAY *display, LPVOID stackPointer) {
    return FALSE;
}

#endif

typedef REGDISPLAY *PREGDISPLAY;

#endif  // __REGDISP_H


