/*
File Name:		ExecuteRemoteProcess.cpp
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/

// ExecuteRemoteProcess.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ExecuteRemoteProcess.h"
#include "RemoteAdministrator.h"
#include "Command.h"
#include <sstream>
#include "Winbase.h"
#include "Windows.h"
#include "Winnls.h"
#include "Winsvc.h"
#include"Machineinfo.h "
#include "Winnetwk.h"
#include "io.h"

#define BUFSIZE 4096

#include <tchar.h>
#include <stdio.h>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

struct CammandParameter CammandParameter;
struct SExecuteCommand  RemoteProcess;
CRITICAL_SECTION g_CriticalSection;
//CammandParameter CammandParameter;
//BOOL CallMessageFunction(char *szMessage);
//void CloseExe();

// CExecuteRemoteProcessApp

BEGIN_MESSAGE_MAP(CExecuteRemoteProcessApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CExecuteRemoteProcessApp construction

CExecuteRemoteProcessApp::CExecuteRemoteProcessApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CExecuteRemoteProcessApp object

CExecuteRemoteProcessApp theApp;


// CExecuteRemoteProcessApp initialization

BOOL CExecuteRemoteProcessApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	CWinApp::InitInstance();
	AfxEnableControlContainer();



	if (AfxGetApp()->m_lpCmdLine[0] != 0)
	{
		wistringstream iss(AfxGetApp()->m_lpCmdLine);
		TCHAR AdminName[MAX_PATH];
		TCHAR AdminPass[MAX_PATH];
		TCHAR AdminDomain[MAX_PATH];
		TCHAR RemoteIp[MAX_PATH];
		TCHAR ApplicationName[MAX_PATH];
		TCHAR CommandLine[MAX_PATH];
		TCHAR Switch1[MAX_PATH];
		TCHAR ProcessType[MAX_PATH];
		iss >> RemoteIp>> AdminName >> AdminPass >> AdminDomain >> ApplicationName >> CommandLine >> ProcessType;
		CMachineInfo* pMachineInfo = new CMachineInfo;
		pMachineInfo->SetIP(RemoteIp);
		pMachineInfo->SetLogon(AdminName);
		pMachineInfo->SetPassword(AdminPass);

		//_tcscat_s(ApplicationName, L" -install");

		CRemoteAdministrator* pRemoteAdministrator = new CRemoteAdministrator();
		pRemoteAdministrator->AddMachine(*pMachineInfo);
		::InitializeCriticalSection(&g_CriticalSection);
		
		BOOL IfSrvRun = pRemoteAdministrator->ConnectToRemoteService(RemoteIp, 1, 0);
		if (!IfSrvRun)
		{
			BOOL bConnectionSuccess = pRemoteAdministrator->EstablishAllConnections(RemoteIp, AdminPass, 1);
			

			BOOL bCopyServiceExe = pRemoteAdministrator->CopyServiceExeToRemoteMachine(RemoteIp);

			//////////
			BOOL bStartAndInstallService = pRemoteAdministrator->InstallAndStartRemoteService(RemoteIp , AdminName, AdminPass);
			::Sleep(2000);



			BOOL bConnectToRemoteService = pRemoteAdministrator->ConnectToRemoteService(RemoteIp, 1, 0);

			if (bConnectionSuccess && bCopyServiceExe && bStartAndInstallService && bConnectToRemoteService)
			{
				BOOL rId = pRemoteAdministrator->OnExecuteProcess(RemoteIp, AdminName, AdminPass, AdminDomain, ApplicationName , CommandLine , ProcessType);
				if (rId == 1)
				{
					//pRemoteAdministrator->RemoveRemoteService(RemoteIp);
					return FALSE;
				}
			}
				//pRemoteAdministrator->RemoveRemoteService(RemoteIp);

			/*else if (!bConnectionSuccess && bCopyServiceExe && bStartAndInstallService && bConnectToRemoteService)
			{

				//InstallAndStartRemoteService(RemoteIp);
				CMachineInfo* pMachineInfo = new CMachineInfo;
				pMachineInfo->SetLogon(_T(SvrAdmin));
				pMachineInfo->SetIP(RemoteIp);
				pMachineInfo->SetPassword(SvrPass);
				pRemoteAdministrator->AddMachine(*pMachineInfo);
				BOOL service = ConnectToRemoteService(RemoteIp, 1, 0);
				OnExecuteProcess(RemUserName, RemPass, domain, AppName, RemoteIp);
			}*/
			
		}

	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}





