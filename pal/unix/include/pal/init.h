/*++


 Copyright (c) 2002 Microsoft Corporation.  All rights reserved.

 The use and distribution terms for this software are contained in the file
 named license.txt, which can be found in the root of this distribution.
 By using this software in any fashion, you are agreeing to be bound by the
 terms of this license.

 You must not remove this notice, or any other, from this software.


Module Name:

    include/pal/init.h

Abstract:
    Header file for PAL init utility functions. Those functions
    are only use by the PAL itself.

Revision History:

--*/

#ifndef _PAL_INIT_H_
#define _PAL_INIT_H_


/*++
Function:
  PALShutdown

Utility function to free any resource used by the PAL. This function
is called by TerminateProcess just before exiting.

--*/

void PALShutdown( void );


/*++
Function:
  PALGetPalConfigDir

Retrieves the path of directory in which per-user PAL data is placed
(file mapping page files, etc)

Parameters :
    LPSTR dest : buffer in which path is stored (must be at least MAX_PATH)

Returns TRUE on success, FALSE otherwise.
--*/
BOOL PALGetPalConfigDir(LPSTR dest, UINT nBufferLength );

extern INT init_count;

/*++
MACRO:
  PALIsInitialized

Returns TRUE if the PAL is in an initialized state
(#calls to PAL_Initialize > #calls to PAL_Terminate)

Warning : this will only report the PAL's state at the moment it is called. 
If it is necessary to ensure the PAL remains initialized (or not) while doing 
some work, the Initialization lock (PALInitLock()) should be held.
--*/
#define PALIsInitialized() (0 < init_count)


/*++
Function:
  PALInitLock

Take the initializaiton critical section (init_critsec). necessary to serialize 
TerminateProcess along with PAL_Terminate and PAL_Initialize

(no parameters)

Return value :
    TRUE if critical section existed (and was acquired)
    FALSE if critical section doens't exist yet
--*/
BOOL PALInitLock(void);

/*++
Function:
  PALInitUnlock

Release the initialization critical section (init_critsec). 

(no parameters, no return value)
--*/
void PALInitUnlock(void);


/*++
INIT_InitPalConfigDir

Create the PAL configuration directory if necessary

returns TRUE on success, FALSE on failure; in case of failure, PAL_Initialize
must fail

--*/
BOOL INIT_InitPalConfigDir(void);


/*++
INIT_RemovePalConfigDir

Delete the PAL configuration directory if it's empty

(no parameters, no return value)

--*/
void INIT_RemovePalConfigDir(void);


/*++
MangleObjectNameW

Mangle an object name to be side-by-side with other Rotors

lplpObjectName - [in out] pointer to pointer to object name to mangle
lpMangledObjectName - buffer of MAX_PATH characters to store the mangled
                      object name in

Returns TRUE on success.  On failure, returns FALSE with LastError set.

--*/
BOOL MangleObjectNameW(LPCWSTR *lplpObjectName, LPWSTR lpMangledObjectName);

#endif /* _PAL_INIT_H_ */













