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
// File: InternalOnly.cpp
//
//*****************************************************************************
#include "stdafx.h"


#include "cordbpriv.h"
#include "internalonly.h"




DisassembleDebuggerCommand::DisassembleDebuggerCommand(const WCHAR *name,
                                                       int minMatchLength)
	: DebuggerCommand(name, minMatchLength)
{
}

void DisassembleDebuggerCommand::Do(DebuggerShell *shell,
                                    ICorDebug *cor,
                                    const WCHAR *args)
{
    // If there is no process, cannot execute this command
    if (shell->m_currentProcess == NULL)
    {
        shell->Write(L"No current process.\n");
        return;
    }

    static int lastCount = 5;
    int count;
    int offset = 0;
    int startAddr = 0;
    
    while ((*args == L' ') && (*args != L'\0'))
        args++;

    if (*args == L'-')
    {
        args++;

        shell->GetIntArg(args, offset);
        offset *= -1;
    }
    else if (*args == L'+')
    {
        args++;

        shell->GetIntArg(args, offset);
    }
    else if ((*args == L'0') && ((*(args + 1) == L'x') ||
                                 (*(args + 1) == L'X')))
    {
        shell->GetIntArg(args, startAddr);
    }

    // Get the number of lines to print on top and bottom of current IP
    if (!shell->GetIntArg(args, count))
        count = lastCount;
    else
        lastCount = count;

    // Don't do anything if there isn't a current thread.
    if ((shell->m_currentThread == NULL) &&
        (shell->m_currentUnmanagedThread == NULL))
    {
        shell->Write(L"Thread no longer exists.\n");
        return;
    }

    // Only show the version info if EnC is enabled.
    if ((shell->m_rgfActiveModes & DSM_ENHANCED_DIAGNOSTICS) &&
        (shell->m_rawCurrentFrame != NULL))
    {
        ICorDebugCode *icode;
        HRESULT hr = shell->m_rawCurrentFrame->GetCode(&icode);

        if (FAILED(hr))
        {
            shell->Write(L"Code information unavailable\n");
        }
        else
        {
            CORDB_ADDRESS codeAddr;
            ULONG32 codeSize;

            hr = icode->GetAddress(&codeAddr);

            if (SUCCEEDED(hr))
                hr = icode->GetSize(&codeSize);

            if (SUCCEEDED(hr))
            {
                shell->Write(L"Code at 0x%08x", codeAddr);
                shell->Write(L" size %d\n", codeSize);
            }
            else
                shell->Write(L"Code address and size not available\n");
            
            ULONG32 nVer;
            hr = icode->GetVersionNumber(&nVer);
            RELEASE(icode);
            
            if (SUCCEEDED(hr))
                shell->Write(L"Version %d\n", nVer);
            else
                shell->Write(L"Code version not available\n");
        }

    }

    // Print out the disassembly around the current IP.
    shell->PrintCurrentInstruction(count,
                                   offset,
                                   startAddr);

    // Indicate that we are in disassembly display mode
    shell->m_showSource = false;
}

// Provide help specific to this command
void DisassembleDebuggerCommand::Help(Shell *shell)
{
	ShellCommand::Help(shell);
	shell->Write(L"[0x<address>] [{+|-}<delta>] [<line count>]\n");
    shell->Write(L"Displays native or IL disassembled instructions for the current instruction\n");
    shell->Write(L"pointer (ip) or a given address, if specified. The default number of\n");
    shell->Write(L"instructions displayed is five (5). If a line count argument is provided,\n");
    shell->Write(L"the specified number of extra instructions will be shown before and after\n");
    shell->Write(L"the current ip or address. The last line count used becomes the default\n");
    shell->Write(L"for the current session. If a delta is specified then the number specified\n");
    shell->Write(L"will be added to the current ip or given address to begin disassembling.\n");
    shell->Write(L"\n");
    shell->Write(L"Examples:\n");
    shell->Write(L"   dis 20\n");
    shell->Write(L"   dis 0x31102500 +5 20\n");
    shell->Write(L"\n");
}

const WCHAR *DisassembleDebuggerCommand::ShortHelp(Shell *shell)
{
    return L"Display native or IL disassembled instructions";
}


ClearUnmanagedExceptionCommand::ClearUnmanagedExceptionCommand(const WCHAR *name, int minMatchLength)
    : DebuggerCommand(name, minMatchLength)
{
}

void ClearUnmanagedExceptionCommand::Do(DebuggerShell *shell,
                                        ICorDebug *cor,
                                        const WCHAR *args)
{
    if (shell->m_currentProcess == NULL)
    {
        shell->Error(L"Process not running.\n");
        return;
    }
    
    // We're given the thread id as the only param
    int dwThreadId;
    if (!shell->GetIntArg(args, dwThreadId))
    {
        Help(shell);
        return;
    }

    // Find the unmanaged thread
    DebuggerUnmanagedThread *ut =
        (DebuggerUnmanagedThread*) shell->m_unmanagedThreads.GetBase(dwThreadId);

    if (ut == NULL)
    {
        shell->Write(L"Thread 0x%x (%d) does not exist.\n",
                     dwThreadId, dwThreadId);
        return;
    }
    
    HRESULT hr =
        shell->m_currentProcess->ClearCurrentException(dwThreadId);

    if (!SUCCEEDED(hr))
        shell->ReportError(hr);
}

	// Provide help specific to this command
void ClearUnmanagedExceptionCommand::Help(Shell *shell)
{
	ShellCommand::Help(shell);
    shell->Write(L"<tid>\n");
    shell->Write(L"Clear the current unmanaged exception for the given tid\n");
    shell->Write(L"\n");
}

const WCHAR *ClearUnmanagedExceptionCommand::ShortHelp(Shell *shell)
{
    return L"Clear the current unmanaged exception (Win32 mode only)";
}

// Unmanaged commands

