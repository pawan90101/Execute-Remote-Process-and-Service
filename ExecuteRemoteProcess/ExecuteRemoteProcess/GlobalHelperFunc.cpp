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

extern CRITICAL_SECTION g_CriticalSection;
void ErrorHandling::PopError(DWORD dwError)
{
    CString strErrorDescription = ErrorHandling::FormatError(dwError);
    
   ::AfxMessageBox(strErrorDescription);

}


CString ErrorHandling::FormatError(DWORD dwError)
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


void ErrorHandling::PopLastError()
{
    DWORD dwLastError = ::GetLastError();
    ErrorHandling::PopError(dwLastError);
}


CString ErrorHandling::FormatLastError()
{
    DWORD dwLastError = ::GetLastError();

    CString strErrorDescription = ErrorHandling::FormatError(dwLastError);

    return strErrorDescription;
}


// Converts a string table ID to an application speific error message
CString ErrorHandling::ConvertStringTableIDToErrorMsg(CString strMachineIP, UINT iStringTableID)
{
	::EnterCriticalSection(&g_CriticalSection);
    TCHAR szMessage[_MAX_PATH] = _T("");
    CString strFormattedErrorMsg;

    // Load the partial string from the string table
    ::LoadString(AfxGetApp()->m_hInstance, iStringTableID, szMessage, _MAX_PATH);

    // Append and create a complete message combining the IP address and the actual 
    // message from the string table
    strFormattedErrorMsg.Format(L"IP %s : %s", strMachineIP.GetBuffer(0), szMessage);
	
	::LeaveCriticalSection(&g_CriticalSection);
    return strFormattedErrorMsg;
}


void ProcessHandling::LaunchApp(CString strAppPath, CString strCommandLine)
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
       ErrorHandling::PopLastError();
    }
}
