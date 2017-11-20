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
/*************************************************************************************/
/*                                   StressLog.h                                     */
/*************************************************************************************/

/* StressLog is a binary, memory based cirular queue of logging messages.  It is 
   intended to be used in retail, non-golden builds during stress runs (activated
   by registry key), so to help find bugs that only turn up during stress runs.  

   It is meant to have very low overhead and can not cause deadlocks, etc.  It is
   however thread safe */

/* The log has a very simple structure, and it meant to be dumped from a NTSD 
   extention (eg. strike). There is no memory allocation system calls etc to purtub things */

/*************************************************************************************/

#ifndef StressLog_h 
#define StressLog_h  1

#define STRESS_LOG0(facility, msg)	0
#define STRESS_LOG1(facility, level, msg, data1)	0


#endif // StressLog_h 
