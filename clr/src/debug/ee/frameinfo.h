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
//*****************************************************************************
// File: frameinfo.h
//
// Debugger stack walker
//
//*****************************************************************************

#ifndef FRAMEINFO_H_
#define FRAMEINFO_H_

/* ========================================================================= */

/* ------------------------------------------------------------------------- *
 * Classes
 * ------------------------------------------------------------------------- */

enum
{
    // Add an extra interception reason
    INTERCEPTION_THREAD_START = Frame::INTERCEPTION_COUNT
};

// struct FrameInfo:  Contains the information that will be handed to
// DebuggerStackCallback functions (along with their own, individual
// pData pointers).
//
// Frame *frame:  The current frame.  NULL implies that
//      the frame is frameless, meaning either unmanaged or managed.  This
//      is set to be FRAME_TOP (0xFFffFFff) if the frame is the topmost, EE
//      placed frame.
//
// MethodDesc *md:  MetdhodDesc for the method that's
//      executing in this frame.  Will be NULL if there is no MethodDesc
// 
// void *fp:  frame pointer.  Actually filled in from
//      caller (parent) frame, so the DebuggerStackWalkProc must delay
//      the user callback for one frame.
//
// SIZE_T reOffset:  Native offset from the beginning of the method.
struct FrameInfo
{
    Frame               *frame; 
    MethodDesc          *md; 
    REGDISPLAY           registers;
    void                *fp; 
    bool                 quickUnwind; 
    bool                 internal; 
    bool                 managed; 
    Context             *context; 
    CorDebugChainReason  chainReason; 
    ULONG                relOffset; 
    IJitManager         *pIJM;
    METHODTOKEN          MethodToken;
    AppDomain           *currentAppDomain;
};

//StackWalkAction (*DebuggerStackCallback):  This callback will
// be invoked by DebuggerWalkStackProc at each frame, passing the FrameInfo
// and callback-defined pData to the method.  The callback then returns a
// SWA - if SWA_ABORT is returned then the walk stops immediately.  If
// SWA_CONTINUE is called, then the frame is walked & the next higher frame
// will be used.  If the current frame is at the top of the stack, then
// in the next iteration, DSC will be invoked with frame->frame == FRAME_TOP
typedef StackWalkAction (*DebuggerStackCallback)(FrameInfo *frame, void *pData);

//StackWalkAction DebuggerWalkStack():  Sets up everything for a
// stack walk for the debugger, starts the stack walk (via
// g_pEEInterface->StackWalkFramesEx), then massages the output.  Note that it
// takes a DebuggerStackCallback as an argument, but at each frame
// DebuggerWalkStackProc gets called, which in turn calls the
// DebuggerStackCallback.
// Thread * thread:  Thread
// void *targetFP:  If you're looking for a specific frame, then
//  this should be set to the fp for that frame, and the callback won't
//  be called until that frame is reached.  Otherwise, set it to NULL &
//  the callback will be called on every frame.
// CONTEXT *context:  Never NULL, b/c the callbacks require the
//  CONTEXT as a place to store some information.  Either it points to an
//  uninitialized CONTEXT (contextValid should be false), or
//  a pointer to a valid CONTEXT for the thread.  If it's NULL, InitRegDisplay
//  will fill it in for us, so we shouldn't go out of our way to set this up.
// bool contextValid:  TRUE if context points to a valid CONTEXT, FALSE
//  otherwise.
// DebuggerStackCallback pCallback:  User supplied callback to
//  be invoked at every frame that's at targetFP or higher.
// void *pData:   User supplied data that we shuffle around,
//  and then hand to pCallback.
StackWalkAction DebuggerWalkStack(Thread *thread, 
                                  void *targetFP,
								  CONTEXT *pContext, 
								  BOOL contextValid,
								  DebuggerStackCallback pCallback,
                                  void *pData, 
                                  BOOL fIgnoreNonmethodFrames,
                                  IpcTarget iWhich = IPC_TARGET_OUTOFPROC);

#endif // FRAMEINFO_H_
