/*
File Name:		ExecuteRemoteProcess.h
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/

// ExecuteRemoteProcess.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CExecuteRemoteProcessApp:
// See ExecuteRemoteProcess.cpp for the implementation of this class
//

class CExecuteRemoteProcessApp : public CWinApp
{
public:
	CExecuteRemoteProcessApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CExecuteRemoteProcessApp theApp;