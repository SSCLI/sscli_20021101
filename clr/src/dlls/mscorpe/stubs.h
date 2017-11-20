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
// Stubs.h
//
// This file contains a template for the default entry point stubs of a COM+
// IL only program.  One can emit these stubs (with some fix-ups) and make
// the code supplied the entry point value for the image.  The fix-ups will
// in turn cause mscoree.dll to be loaded and the correct entry point to be
// called.
//
// Note: Although these stubs contain x86 specific code, they are used
// for all platforms
//
//*****************************************************************************
#ifndef __STUBS_H__
#define __STUBS_H__

//*****************************************************************************
// This stub is designed for a x86 Windows application.  It will call the
// _CorExeMain function in mscoree.dll.  This entry point will in turn load
// and run the IL program.
//
// void ExeMain(void)
// {
//    _CorExeMain();
// }
//
// The code calls the imported functions through the iat, which must be
// emitted to the PE file.  The two addresses in the template must be replaced
// with the address of the corresponding iat entry which is fixed up by the
// loader when the image is paged in.
//*****************************************************************************
static const BYTE ExeMainTemplate[] =
{
	// Jump through IAT to _CorExeMain
	0xFF, 0x25,						// jmp iat[_CorDllMain entry]
		0x00, 0x00, 0x00, 0x00,	//	address to replace

};

#define ExeMainTemplateSize		sizeof(ExeMainTemplate)
#define CorExeMainIATOffset		2

//*****************************************************************************
// This stub is designed for a x86 Windows application.  It will call the 
// _CorDllMain function in mscoree.dll with with the base entry point 
// for the loaded DLL.
// This entry point will in turn load and run the IL program.
//
// BOOL APIENTRY DllMain( HANDLE hModule,
//                         DWORD ul_reason_for_call, 
//                         LPVOID lpReserved )
// {
// 	   return _CorDllMain(hModule, ul_reason_for_call, lpReserved);
// }

// The code calls the imported function through the iat, which must be
// emitted to the PE file.  The address in the template must be replaced
// with the address of the corresponding iat entry which is fixed up by the
// loader when the image is paged in.
//*****************************************************************************

static const BYTE DllMainTemplate[] = 
{
	// Call through IAT to CorDllMain 
	0xFF, 0x25,					// jmp iat[_CorDllMain entry]
		0x00, 0x00, 0x00, 0x00,	//   address to replace
};

#define DllMainTemplateSize		sizeof(DllMainTemplate)
#define CorDllMainIATOffset		2

#endif  // __STUBS_H__
