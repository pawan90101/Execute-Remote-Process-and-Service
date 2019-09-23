/*
File Name:		GlobalHelperFunc.cpp
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/
#include "stdafx.h"
#include "GlobalHelperFunc.h"

void PopError(DWORD dwError)
{
    CString strErrorDescription = ::FormatError(dwError);
    
   ::AfxMessageBox(strErrorDescription);

}


CString FormatError(DWORD dwError)
{
    HLOCAL hlocal = NULL;   // Buffer that gets the error message string
    CString strErrorDescription = _T("");

    // Get the error code's textual description 
    BOOL bOK = ::FormatMessage(
                    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
                    NULL, 
                    dwError, 
                    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 
                    reinterpret_cast<PTSTR>(&hlocal),
                    0,
                    NULL
                    );

    if (hlocal != NULL) 
    {
        strErrorDescription = static_cast<PCSTR>(hlocal);
        ::LocalFree(hlocal);
    }
    else
    {
        strErrorDescription = _T("Failed to retrive error description !");
    }

    return strErrorDescription;
}


void PopLastError()
{
    DWORD dwLastError = ::GetLastError();
    ::PopError(dwLastError);
}


CString FormatLastError()
{
    DWORD dwLastError = ::GetLastError();

    CString strErrorDescription = ::FormatError(dwLastError);

    return strErrorDescription;
}


void LaunchApp(CString strAppPath, CString strCommandLine)
{
    TCHAR* szAppPath     = strAppPath.GetBuffer(0);
    TCHAR* szCommandline = strCommandLine.GetBuffer(0);

    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;

    BOOL bSuccess = ::CreateProcess(
                          szAppPath,
                          szCommandline,
                          NULL,
                          NULL,
                          FALSE,
                          0,
                          NULL,
                          NULL,
                          &si,
                          &pi
                          );
                        
    if (bSuccess)
    {
        // --*******PREVENT MEMORY LEAKS---*************
        // This function was written for this purpose, as 
        // developers forget to call the next 2 calls to
        // close handle

        // Close the thread handle as soon as it is no longer needed!
        ::CloseHandle(pi.hThread);

        // Close the process handle as soon as it is no longer needed.
        ::CloseHandle(pi.hProcess);
    }

    else
    {
       ::PopLastError();
    }
}

