/*
File Name:		GlobalHelperFunc.h
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/

// Global helper functions

#ifndef GLOBALHELPERFUNC_H
#define GLOBALHELPERFUNC_H

#include <afxwin.h>

namespace ErrorHandling
{
	void    PopError(DWORD dwError);
	void    PopLastError();
	CString FormatError(DWORD dwError);
	CString FormatLastError();
	
	// Converts a string table ID to an application speific error message
	CString ConvertStringTableIDToErrorMsg(CString strMachineIP, UINT iStringTableID);
}

namespace ProcessHandling
{
	void LaunchApp(CString strAppPath, CString strCommandLine);
}



#endif // GLOBALHELPERFUNC_H