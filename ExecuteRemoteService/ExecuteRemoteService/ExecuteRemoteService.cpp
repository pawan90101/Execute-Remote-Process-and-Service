/*
File Name:		ExecuteRemoteService.cpp
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/
#include "stdafx.h"
#include "GlobalHelperFunc.h"
#include <psapi.h>
#include <afxtempl.h>
#include <afxwin.h>
#include <tchar.h>
#include <winsvc.h>
#include <Tlhelp32.h>
#include <process.h>
#include "LogClass.h"
#include "psapi.h"

#define  REMOTE_ADMIN_SERVICE              _T("ExecuteRemoteService")
#define  REMOTE_ADMIN_PIPE                 _T("ExecuteRemotePipe")
#define  REMOTE_ADMIN_PROCESS_INFO_PIPE    _T("ExecuteRemoteProcessInfoPipe")
#define  REMOTE_ADMIN_PROCESS_EXECUTE_PIPE _T("ExecuteRemoteProcessExecutePipe")
#define  REMOTE_ADMIN_PROCESS_KILL_PIPE    _T("ExecuteRemoteProcessKillPipe")
#define  REMOTE_ADMIN_SYS_SHUTDOWN_PIPE    _T("ExecuteRemoteSysShutDownPipe")
#define  SERVICENAME                       _T("ExecuteRemoteService")
#define  LONGSERVICENAME                   _T("ExecuteRemoteService")

struct SProcessInfo
{
    PROCESSENTRY32 peProcessEntry;
    SIZE_T         stMemUsage;
};

//typedef CTypedPtrList<CPtrList, PROCESSENTRY32*> CProcessInfoList;
typedef CTypedPtrList<CPtrList, SProcessInfo*> CProcessInfoList;

VOID WINAPI StartExecuteRemoteService(DWORD, LPTSTR*);
void WINAPI ExecuteRemoteHandler(DWORD Opcode);
DWORD IsService(BOOL& isService);
void UpdateProcessInfoList(void* pArgument);
void CleanProcessInfoList();
void ServiceMain(void*);
void ExecuteRemoteThreadProc(void*);
void ExecuteRemoteThread(void*);
void ExecuteRemoteProcessInfoThread(void*);
void ExecuteRemoteExecuteProcessThread(void*);
void ExecuteRemoteKillProcessThread(void*);
void ExecuteRemoteSysShutdownThread(void* pParam);
void DeleteTheService();
BOOL SystemShutdown(LPTSTR lpMsg, BOOL bReboot, UINT iTimeOut /*Secs*/);
BOOL PreventSystemShutdown();

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid);
BOOL AddAceToDesktop(HDESK hdesk, PSID psid);
VOID FreeLogonSID (PSID *ppsid);
BOOL GetLogonSID (HANDLE hToken, PSID *ppsid);
BOOL StartInteractiveClientProcess (
    LPTSTR lpszUsername,    // client to log on
    LPTSTR lpszDomain,      // domain of client's account
    LPTSTR lpszPassword,    // client's password
	LPTSTR lpApplication,    // application name
    LPTSTR lpCommandLine    // command line to execute
); 

BOOL StartNonInteractiveClientProcess(
	LPTSTR lpszUsername,    // client to log on
	LPTSTR lpszDomain,      // domain of client's account
	LPTSTR lpszPassword,    // client's password
	LPTSTR lpApplication,    // application name
	LPTSTR lpCommandLine    // command line to execute
);


SIZE_T GetPhysicalMemUsage(DWORD dwProcessID);

#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW |DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL |  DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)
#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN | STANDARD_RIGHTS_REQUIRED)
#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)



struct SCommand
{
    BOOL m_bThreadExit;
};

struct SExecuteCommand
{
    TCHAR m_szProcessPath[_MAX_PATH]; // Process to start 
    TCHAR m_szUsername[_MAX_PATH];    // client to log on
    TCHAR m_szDomain[_MAX_PATH];      // domain of client's account
    TCHAR m_szPassword[_MAX_PATH];    // client's password
	TCHAR m_szCommandLine[_MAX_PATH];    // Command Line
	TCHAR m_szService[_MAX_PATH];		// choose process is intractive or not(like service or other application)
};

struct SSysShutDownInfo
{
    BOOL bShutDown;   // TRUE if you want to shutdown,FALSE if want to cancel shutdown
    BOOL bReboot;     // Reboot if TRUE, else HALT if FALSE
    UINT iTimeToShutDown; // Time given to user before shutdown in secs
};

CProcessInfoList pilProcessInfoList;
HANDLE hStopServiceEvent;
long lServicePipeInstanceCount = 0;
SERVICE_STATUS          ServiceStatus; 
SERVICE_STATUS_HANDLE   ServiceStatusHandle;
CRITICAL_SECTION g_CriticalSection;
 

int main(int argc, char* argv[])
{
	LogClass::log("In MAin", "test.txt");
    SERVICE_TABLE_ENTRY DispatchTable[] = { 
        {REMOTE_ADMIN_SERVICE, StartExecuteRemoteService}, 
        {NULL, NULL}
    }; 

    BOOL bIsService = FALSE;
    ::IsService(bIsService);
    
    if (bIsService)
    {
		::InitializeCriticalSection(&g_CriticalSection);
        return StartServiceCtrlDispatcher(DispatchTable);
    }
	else
	{
		LogClass::log("This process is not a Service","test.txt");
	}
    
   	return 0;
}


// This process is a service or is not ?
DWORD IsService(BOOL& isService)
{
    DWORD pID = GetCurrentProcessId(); 
	HANDLE hProcessToken = NULL;
	DWORD groupLength = 50;
	PTOKEN_GROUPS groupInfo = NULL;

	SID_IDENTIFIER_AUTHORITY siaNt = SECURITY_NT_AUTHORITY;
	PSID pInteractiveSid = NULL;
	PSID pServiceSid = NULL;

	DWORD dwRet = NO_ERROR;
    
    // reset flags
	BOOL isInteractive = FALSE;
	isService = FALSE;

	DWORD ndx;

    HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);

	// open the token
	if (!::OpenProcessToken(hProcess, TOKEN_QUERY, &hProcessToken))
	{
		dwRet = ::GetLastError();
		goto closedown;
	}

	// allocate a buffer of default size
	groupInfo = (PTOKEN_GROUPS)::LocalAlloc(0, groupLength);
	if (groupInfo == NULL)
	{
		dwRet = ::GetLastError();
		goto closedown;
	}

	// try to get the info
	if (!::GetTokenInformation(hProcessToken, TokenGroups, groupInfo, groupLength, &groupLength))
	{
		// if buffer was too small, allocate to proper size, otherwise error
		if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			dwRet = ::GetLastError();
			goto closedown;
		}

		::LocalFree(groupInfo);

		groupInfo = (PTOKEN_GROUPS)::LocalAlloc(0, groupLength);
		if (groupInfo == NULL)
		{
			dwRet = ::GetLastError();
			goto closedown;
		}

		if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo, groupLength, &groupLength))
		{
			dwRet = ::GetLastError();
			goto closedown;
		}
	}

	// create comparison sids
	if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_INTERACTIVE_RID, 0, 0, 0, 0, 0, 0, 0, &pInteractiveSid))
	{
		dwRet = ::GetLastError();
		goto closedown;
	}

	if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_SERVICE_RID, 0, 0, 0, 0, 0, 0, 0, &pServiceSid))
	{
		dwRet = ::GetLastError();
		goto closedown;
	}

	// try to match sids
	for (ndx = 0; ndx < groupInfo->GroupCount ; ndx += 1)
	{
		SID_AND_ATTRIBUTES sanda = groupInfo->Groups[ndx];
		PSID pSid = sanda.Sid;

		if (::EqualSid(pSid, pInteractiveSid))
		{
			isInteractive = TRUE;
			isService = FALSE;
			break;
		}
		else if (::EqualSid(pSid, pServiceSid))
		{
			isService = TRUE;
			isInteractive = FALSE;
			break;
		}
	}

   if ( !(isService || isInteractive ))
		isService = TRUE;
        
closedown:
		if (pServiceSid)
        {
            ::FreeSid(pServiceSid);
        }
		
		if (pInteractiveSid)
        {
            ::FreeSid(pInteractiveSid);
        }
		
		if (groupInfo)
        {
            ::LocalFree(groupInfo);
        }
			

		if (hProcessToken)
        {
            ::CloseHandle(hProcessToken);
        }
			

		if (hProcess)
        {
            ::CloseHandle(hProcess);
        }
			

	return dwRet;
}

void UpdateProcessInfoList(void* pArgument)
{
    for (;;)
    {
        CleanProcessInfoList();

		::EnterCriticalSection(&g_CriticalSection);
		
        DWORD dwCurrentProcessId = ::GetCurrentProcessId();
    
        // Take this process's snapshot
        HANDLE hProcessSnapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, dwCurrentProcessId);

        //PROCESSENTRY32* pPe = new PROCESSENTRY32;
		//pPe->dwSize = sizeof(PROCESSENTRY32); 
        SProcessInfo* pPi = new SProcessInfo;
        BOOL bOk = ::Process32First(hProcessSnapShot, &pPi->peProcessEntry);
        
        // Get the process memeory usage
        pPi->stMemUsage = GetPhysicalMemUsage(pPi->peProcessEntry.th32ProcessID);
        
        if (!bOk)
        {
            delete pPi;
            pPi = NULL;
        }
        
        while (bOk)
        {
            pilProcessInfoList.AddTail(pPi);

            //pPe = new PROCESSENTRY32;
            pPi = new SProcessInfo;
            bOk = ::Process32Next(hProcessSnapShot, &pPi->peProcessEntry);
            
            // Get the process memeory usage
            pPi->stMemUsage = GetPhysicalMemUsage(pPi->peProcessEntry.th32ProcessID);

            if (!bOk)
            {
                delete pPi;
                pPi = NULL;
            }
        }

        // We didn't forget to clean up the snapshot object.
        ::CloseHandle(hProcessSnapShot);
        ::Sleep(2000);

		::LeaveCriticalSection(&g_CriticalSection);
    }

    return ;
}

void CleanProcessInfoList()
{
	//PROCESSENTRY32* pPe = NULL;
    SProcessInfo* pPi = NULL;

	::EnterCriticalSection(&g_CriticalSection);

    POSITION pos = pilProcessInfoList.GetHeadPosition();
    while (pos)
    {
        pPi = pilProcessInfoList.GetNext(pos);

		if (pPi != NULL)
		{
			delete pPi;
		}
    }
            
    pilProcessInfoList.RemoveAll();
	
	::LeaveCriticalSection(&g_CriticalSection);

    return;
}


// Start service
VOID WINAPI StartExecuteRemoteService(DWORD, LPTSTR*) 
{
   DWORD status = 0; 
   DWORD specificError = 0;
   LogClass::log("In Start Remote Service", "test.txt");
   // Prepare the ServiceStatus structure that will be used for the
   // comunication with SCM(Service Control Manager).
   // If you fully under stand the members of this structure, feel
   // free to change these values :o)
   ServiceStatus.dwServiceType        = SERVICE_WIN32; 
   ServiceStatus.dwCurrentState       = SERVICE_START_PENDING; 
   ServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP; 
   ServiceStatus.dwWin32ExitCode      = 0; 
   ServiceStatus.dwServiceSpecificExitCode = 0; 
   ServiceStatus.dwCheckPoint         = 0; 
   ServiceStatus.dwWaitHint           = 0; 

   // Here we register the control handler for our service.
   // We tell the SCM about a call back function that SCM will
   // call when user tries to Start, Stop or Pause your service.
   ServiceStatusHandle = RegisterServiceCtrlHandler( 
         TEXT("Service"), ExecuteRemoteHandler); 

   if (ServiceStatusHandle == (SERVICE_STATUS_HANDLE)0) 
      return; 
   
   // Handle error condition 
   if (status != NO_ERROR) 
   { 
      ServiceStatus.dwCurrentState       = SERVICE_STOPPED; 
      ServiceStatus.dwCheckPoint         = 0; 
      ServiceStatus.dwWaitHint           = 0; 
      ServiceStatus.dwWin32ExitCode      = status; 
      ServiceStatus.dwServiceSpecificExitCode = specificError; 

      SetServiceStatus (ServiceStatusHandle, &ServiceStatus); 
      return; 
   } 

   // Initialization complete - report running status. 
   ServiceStatus.dwCurrentState       = SERVICE_RUNNING; 
   ServiceStatus.dwCheckPoint         = 0; 
   ServiceStatus.dwWaitHint           = 0; 

   if (!SetServiceStatus (ServiceStatusHandle, &ServiceStatus)) 
      status = GetLastError(); 
   else
   {
      // Start the main thread
      hStopServiceEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
      _beginthread(ServiceMain, 0, NULL );
   }
   
   return; 
} 


void ServiceMain(void*)
{
	LogClass::log("In Service main", "test.txt");
    ::_beginthread(UpdateProcessInfoList, 0, NULL);

    // Start CommunicationPoolThread, which handles the incoming instances
    ::_beginthread(ExecuteRemoteThread, 0, NULL );

    // Waiting for stop the service
    while(::WaitForSingleObject(hStopServiceEvent, 10) != WAIT_OBJECT_0)
    {
    }
   
    // Let's delete itself, after the service stopped
    ::DeleteTheService();

    ::CloseHandle(hStopServiceEvent);
}

void ExecuteRemoteThread(void*)
{
	LogClass::log("In Remote Admin thread.", "test.txt");
    HANDLE hPipe = NULL;
    
    for (;;)
    {
        SECURITY_ATTRIBUTES SecAttrib = {0};
        SECURITY_DESCRIPTOR SecDesc;
        InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&SecDesc, TRUE, NULL, TRUE);

        SecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecAttrib.lpSecurityDescriptor = &SecDesc;;
        SecAttrib.bInheritHandle = TRUE;

        // Create communication pipe
        hPipe = ::CreateNamedPipe(
                    _T("\\\\.\\pipe\\")REMOTE_ADMIN_PIPE, 
                    PIPE_ACCESS_DUPLEX, 
                    PIPE_TYPE_MESSAGE | PIPE_WAIT, 
                    PIPE_UNLIMITED_INSTANCES,
                    0,
                    0,
                    (DWORD)-1,
                    &SecAttrib);

        if (hPipe != NULL)
        {
            // Waiting for client to connect to this pipe
            ::ConnectNamedPipe(hPipe, NULL);
            ::_beginthread(ExecuteRemoteThreadProc, 0, (void*)hPipe);
        }
    }
}

void ExecuteRemoteThreadProc(void* pParam)
{
    // Increment instance counter 
    ::InterlockedIncrement(&lServicePipeInstanceCount);

    SECURITY_ATTRIBUTES SecAttrib = {0};
    SECURITY_DESCRIPTOR SecDesc;
    InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&SecDesc, TRUE, NULL, TRUE);

    SecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecAttrib.lpSecurityDescriptor = &SecDesc;
    SecAttrib.bInheritHandle = TRUE;

    HANDLE hPipeProcessInfo    = INVALID_HANDLE_VALUE;
    HANDLE hPipeProcessKill    = INVALID_HANDLE_VALUE;
    HANDLE hPipeProcessExecute = INVALID_HANDLE_VALUE;
    HANDLE hPipeSysShutDown    = INVALID_HANDLE_VALUE;

    // Create communication pipe for writing the process information
    hPipeProcessInfo = ::CreateNamedPipe(
                            _T("\\\\.\\pipe\\")REMOTE_ADMIN_PROCESS_INFO_PIPE, 
                            PIPE_ACCESS_DUPLEX, 
                            PIPE_TYPE_MESSAGE | PIPE_WAIT, 
                            PIPE_UNLIMITED_INSTANCES,
                            1024,
                            1024,
                            (DWORD)-1,
                            &SecAttrib
                            );

    // Create communication pipe for receiving which process to execute
    hPipeProcessExecute = ::CreateNamedPipe(
                              _T("\\\\.\\pipe\\")REMOTE_ADMIN_PROCESS_EXECUTE_PIPE, 
                              PIPE_ACCESS_DUPLEX, 
                              PIPE_TYPE_MESSAGE | PIPE_WAIT, 
                              PIPE_UNLIMITED_INSTANCES,
                              1024,
                              1024,
                              (DWORD)-1,
                              &SecAttrib
                              );

    // Create communication pipe for receiving which process to kill
    hPipeProcessKill = ::CreateNamedPipe(
                           _T("\\\\.\\pipe\\")REMOTE_ADMIN_PROCESS_KILL_PIPE, 
                           PIPE_ACCESS_DUPLEX,
                           PIPE_TYPE_MESSAGE | PIPE_WAIT, 
                           PIPE_UNLIMITED_INSTANCES,
                           1024,
                           1024,
                           (DWORD)-1,
                           &SecAttrib
                           );

    // Create communication pipe for initiating system shutdown
    hPipeSysShutDown = ::CreateNamedPipe(
                           _T("\\\\.\\pipe\\")REMOTE_ADMIN_SYS_SHUTDOWN_PIPE, 
                           PIPE_ACCESS_DUPLEX,
                           PIPE_TYPE_MESSAGE | PIPE_WAIT, 
                           PIPE_UNLIMITED_INSTANCES,
                           1024,
                           1024,
                           (DWORD)-1,
                           &SecAttrib
                           );



    ::ConnectNamedPipe(hPipeProcessInfo,    NULL);
    ::ConnectNamedPipe(hPipeProcessExecute, NULL);
    ::ConnectNamedPipe(hPipeProcessKill,    NULL);
    ::ConnectNamedPipe(hPipeSysShutDown,    NULL);

    ::_beginthread(ExecuteRemoteProcessInfoThread,    0, hPipeProcessInfo);
    ::_beginthread(ExecuteRemoteExecuteProcessThread, 0, hPipeProcessExecute);
    ::_beginthread(ExecuteRemoteKillProcessThread,    0, hPipeProcessKill);
    ::_beginthread(ExecuteRemoteSysShutdownThread,    0, hPipeSysShutDown);
    
    //::Sleep(10000);
    // No more drama of of keeping the pipe waiting 
    SCommand cmd;
      HANDLE hPipe = reinterpret_cast<HANDLE>(pParam);
      DWORD dwRead;
//}//
    for(;;)
    {
        if (!::ReadFile(hPipe, &cmd, sizeof(SCommand), &dwRead, NULL ) || dwRead == 0)
        {
            goto cleanup;
        }
        else
        {
            if (cmd.m_bThreadExit == TRUE)
            {
                goto cleanup;
            }
        }
    }
    
cleanup:

    ::DisconnectNamedPipe(hPipe);
    ::CloseHandle(hPipe);

    // Decrement instance counter 
    ::InterlockedDecrement(&lServicePipeInstanceCount);

    // If this was the last client, let's stop ourself
    if (lServicePipeInstanceCount == 0)
    {
        ::SetEvent(hStopServiceEvent);
    }
}

void ExecuteRemoteProcessInfoThread(void* pParam)
{
    // Increment instance counter 
    ::InterlockedIncrement(&lServicePipeInstanceCount);

    HANDLE hPipe = reinterpret_cast<HANDLE>(pParam);
    SCommand cmd = {0};
    
    // Waiting for client to connect to this pipe
   ::ConnectNamedPipe(hPipe, NULL);
    
    DWORD dwWritten = 0;
    DWORD dwRead    = 0;
    POSITION pos    = NULL;

    for (;;)
    {
        if (!::ReadFile(hPipe, &cmd, sizeof(SCommand), &dwRead, NULL ) || dwRead == 0)
        {
            goto cleanup;
        }
        else
        {
            if (cmd.m_bThreadExit == TRUE)
            {
                goto cleanup;
            }
        }
        
		::EnterCriticalSection(&g_CriticalSection);

        int iProcessCount = pilProcessInfoList.GetCount();
        if (!::WriteFile(hPipe, &iProcessCount, sizeof(int), &dwWritten, NULL) || dwWritten == 0 )
        {
            goto cleanup;
        }
        for (int i = 0; i < iProcessCount; ++i)
        {
            pos = pilProcessInfoList.FindIndex(i);
			
			if (pos != NULL)
			{
				//PROCESSENTRY32* pPe = pilProcessInfoList.GetAt(pos);
                SProcessInfo* pPi = pilProcessInfoList.GetAt(pos);
            
				//if (!::WriteFile(hPipe, pPe, sizeof(PROCESSENTRY32), &dwWritten, NULL) || dwWritten == 0 )
                if (!::WriteFile(hPipe, pPi, sizeof(SProcessInfo), &dwWritten, NULL) || dwWritten == 0 )
				{
					goto cleanup;
				}
			}
        }
		::LeaveCriticalSection(&g_CriticalSection);

        ::Sleep(100);
    }

cleanup:

    ::DisconnectNamedPipe(hPipe);
    ::CloseHandle(hPipe);

    // Decrement instance counter 
    ::InterlockedDecrement(&lServicePipeInstanceCount);

    // If this was the last client, let's stop ourself
    if (lServicePipeInstanceCount == 0)
    {
        ::SetEvent(hStopServiceEvent);
    }

    _endthread();
}
void ExecuteRemoteExecuteProcessThread(void* pParam)
{
    // Increment instance counter 
    InterlockedIncrement(&lServicePipeInstanceCount);

    HANDLE hPipe = reinterpret_cast<HANDLE>(pParam);
    SCommand cmd = {0};
    SExecuteCommand ExeCmd = {0};
        
    // Waiting for client to connect to this pipe
    ::ConnectNamedPipe(hPipe, NULL);
    
    DWORD dwWritten                    = 0;
    DWORD dwRead                       = 0;
    POSITION pos                       = NULL;
    
    for (;;)
    {
        // Read whether to continue this thread?
        if (!::ReadFile(hPipe, &cmd, sizeof(SCommand), &dwRead, NULL ) || dwRead == 0)
        {
            goto cleanup;
        }
        else
        {
            if (cmd.m_bThreadExit == TRUE)
            {
                goto cleanup;
            }
        }

        // Read the process path
        if (!::ReadFile(hPipe, &ExeCmd, sizeof(SExecuteCommand), &dwRead, NULL ) || dwRead == 0)
        {
            goto cleanup;
        }
        else
        {

			LogClass::log(ExeCmd.m_szUsername, L"test.txt");
			LogClass::log(ExeCmd.m_szDomain, L"test.txt");
			LogClass::log(ExeCmd.m_szPassword, L"test.txt");
			LogClass::log(ExeCmd.m_szProcessPath, L"test.txt");
			LogClass::log(ExeCmd.m_szCommandLine, L"test.txt");
			LogClass::log(ExeCmd.m_szService, L"test.txt");

            DWORD dwWritten = 0;
			BOOL bCouldStartProcess;
			if (_tcscmp(ExeCmd.m_szService , _T("service")) == 0)
			{
				bCouldStartProcess = ::StartNonInteractiveClientProcess(ExeCmd.m_szUsername, ExeCmd.m_szDomain, ExeCmd.m_szPassword, ExeCmd.m_szProcessPath, ExeCmd.m_szCommandLine);
				LogClass::log("Non Intractive Process", "test.txt");
			}
			else
			{
				bCouldStartProcess = ::StartInteractiveClientProcess(ExeCmd.m_szUsername, ExeCmd.m_szDomain, ExeCmd.m_szPassword, ExeCmd.m_szProcessPath, ExeCmd.m_szCommandLine);
				LogClass::log("Intractive Process", "test.txt");
			}
            if (bCouldStartProcess)
            {
                TCHAR szMessage[_MAX_PATH] = _T("");
                BOOL bOk = ::WriteFile(hPipe, szMessage,  sizeof(szMessage), &dwWritten, NULL);
            }
            else
            {
                TCHAR szMessage[_MAX_PATH] = _T("Requested process started on remote machine");
                BOOL bOk = ::WriteFile(hPipe, szMessage,  sizeof(szMessage), &dwWritten, NULL);
            }
        }
    }

cleanup:

    ::DisconnectNamedPipe(hPipe);
    ::CloseHandle(hPipe);

    // Decrement instance counter 
    ::InterlockedDecrement(&lServicePipeInstanceCount);

    // If this was the last client, let's stop ourself
    if (lServicePipeInstanceCount == 0)
    {
        ::SetEvent(hStopServiceEvent);
    }
    _endthread();
}

void ExecuteRemoteKillProcessThread(void* pParam)
{
    // Increment instance counter 
    InterlockedIncrement(&lServicePipeInstanceCount);

    HANDLE hPipe = reinterpret_cast<HANDLE>(pParam);
    SCommand cmd = {0};
    
    // Waiting for client to connect to this pipe
    ::ConnectNamedPipe(hPipe, NULL);
    
    DWORD dwWritten = 0;
    DWORD dwRead    = 0;
    TCHAR* szProcessIDToBeKilled[10];

    for (;;)
    {
        if (!::ReadFile(hPipe, &cmd, sizeof(SCommand), &dwRead, NULL ) || dwRead == 0)
        {
            goto cleanup;
        }
        else
        {
            if (cmd.m_bThreadExit == TRUE)
            {
                goto cleanup;
            }
        }
        // Read the process path
        if (!::ReadFile(hPipe, &szProcessIDToBeKilled, sizeof(szProcessIDToBeKilled), &dwRead, NULL ) || dwRead == 0)
        {
            goto cleanup;
        }
        else
        {
            BOOL bOk                    = FALSE;
            TCHAR szMessage[_MAX_PATH]  = _T("");
            DWORD dwWritten             = 0;
            DWORD dwProcessIDToBeKilled = ::atoi((const char*)szProcessIDToBeKilled);
            HANDLE hProcessToBeKilled = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessIDToBeKilled);

            if (hProcessToBeKilled != NULL)
            {
                bOk = ::TerminateProcess(hProcessToBeKilled, 0);
                if (bOk)
                {
                    ::_tcscpy_s(szMessage, _T(""));
                    bOk = ::WriteFile(hPipe, szMessage,  sizeof(szMessage), &dwWritten, NULL);
                }
                else
                {
                    ::_tcscpy_s(szMessage, _T("Requested process started n remote machine"));
                    bOk = ::WriteFile(hPipe, szMessage,  sizeof(szMessage), &dwWritten, NULL);
                }
            }
            else
            {
                ::_tcscpy_s(szMessage, _T("Requested process started on remote machine"));
                bOk = ::WriteFile(hPipe, szMessage,  sizeof(szMessage), &dwWritten, NULL);
            }
        }
    }

cleanup:

    ::DisconnectNamedPipe(hPipe);
    ::CloseHandle(hPipe);

    // Decrement instance counter 
    ::InterlockedDecrement(&lServicePipeInstanceCount);

    // If this was the last client, let's stop ourself
    if (lServicePipeInstanceCount == 0)
    {
        ::SetEvent(hStopServiceEvent);
    }
    _endthread();
}


void ExecuteRemoteSysShutdownThread(void* pParam)
{
    // Increment instance counter 
    ::InterlockedIncrement(&lServicePipeInstanceCount);

    HANDLE hPipe = reinterpret_cast<HANDLE>(pParam);
    SCommand cmd = {0};
    SSysShutDownInfo shutdowninfo;   
    DWORD dwWritten = 0;
    DWORD dwRead    = 0;
    TCHAR szMessage[_MAX_PATH] = _T("");
        
    for(;;)
    {
        // Read for thread exit
        if (!::ReadFile(hPipe, &cmd, sizeof(SCommand), &dwRead, NULL ) || dwRead == 0)
        {
            goto cleanup;
        }
        else
        {
            if (cmd.m_bThreadExit == TRUE)
            {
                goto cleanup;
            }
        }
        
        // Read for system shutdown
        if (!::ReadFile(hPipe, &shutdowninfo, sizeof(SSysShutDownInfo), &dwRead, NULL ) || dwRead == 0)
        {
            goto cleanup;
        }
        else
        {
            if (shutdowninfo.bShutDown)
            {
                //BOOL bResult = ::InitiateSystemShutdown(NULL, NULL, 30, FALSE, shutdowninfo.bReboot);
                BOOL bResult = ::SystemShutdown(NULL, shutdowninfo.bReboot, shutdowninfo.iTimeToShutDown);
                if (bResult)
                {
                    ::_tcscpy_s(szMessage, _T(""));
                    BOOL bOk = ::WriteFile(hPipe, szMessage,  sizeof(szMessage), &dwWritten, NULL);
                }
                else
                {
                    CString strFailureMessage = ::FormatLastError();
                    ::_tcscpy_s(szMessage, strFailureMessage.GetBuffer(0));
                    BOOL bOk = ::WriteFile(hPipe, szMessage,  sizeof(szMessage), &dwWritten, NULL);
                }
            }
            else
            {
                BOOL bResult = ::PreventSystemShutdown();
                if (bResult)
                {
                    ::_tcscpy_s(szMessage, _T(""));
                    BOOL bOk = ::WriteFile(hPipe, szMessage,  sizeof(szMessage), &dwWritten, NULL);
                }
                else
                {
                    CString strFailureMessage = ::FormatLastError();
                    ::_tcscpy_s(szMessage, strFailureMessage.GetBuffer(0));
                    BOOL bOk = ::WriteFile(hPipe, szMessage,  sizeof(szMessage), &dwWritten, NULL);
                }
            }
        }
    }

cleanup:

    ::DisconnectNamedPipe(hPipe);
    ::CloseHandle(hPipe);

    // Decrement instance counter 
    ::InterlockedDecrement(&lServicePipeInstanceCount);

    // If this was the last client, let's stop ourself
    if (lServicePipeInstanceCount == 0)
    {
        ::SetEvent(hStopServiceEvent);
    }
    _endthread();
}
void WINAPI ExecuteRemoteHandler(DWORD Opcode) 
{
    DWORD status; 
	
    switch(Opcode) 
    { 
        case SERVICE_CONTROL_STOP: 
            // Signal the event to stop the main thread
            SetEvent( hStopServiceEvent );

            ServiceStatus.dwWin32ExitCode = 0; 
            ServiceStatus.dwCurrentState  = SERVICE_STOPPED; 
            ServiceStatus.dwCheckPoint    = 0; 
            ServiceStatus.dwWaitHint      = 0; 
 
            if (!SetServiceStatus (ServiceStatusHandle, 
                &ServiceStatus))
            { 
                status = GetLastError(); 
            } 
			return; 
 
        case SERVICE_CONTROL_INTERROGATE: 
        // Fall through to send current status. 
            break; 
    } 
 
    // Send current status. 
    if (!SetServiceStatus (ServiceStatusHandle,  &ServiceStatus)) 
    { 
        status = GetLastError(); 
    } 
    return; 
}

BOOL StartInteractiveClientProcess (
    LPTSTR lpszUsername,    // client to log on
    LPTSTR lpszDomain,      // domain of client's account
    LPTSTR lpszPassword,    // client's password
	LPTSTR lpApplication,    // execute application name
    LPTSTR lpCommandLine    // command line to execute
) 
{
   HANDLE      hToken;
   HDESK       hdesk = NULL;
   HWINSTA     hwinsta = NULL, hwinstaSave = NULL;
   PROCESS_INFORMATION pi;
   PSID pSid = NULL;
   STARTUPINFO si;
   BOOL bResult = FALSE;

// Log the client on to the local computer.

   if (!LogonUser(
           lpszUsername,
           lpszDomain,
           lpszPassword,
           LOGON32_LOGON_INTERACTIVE,
           LOGON32_PROVIDER_DEFAULT,
           &hToken) ) 
   {
      goto Cleanup;
   }

// Save a handle to the caller's current window station.

   if ( (hwinstaSave = GetProcessWindowStation() ) == NULL)
      goto Cleanup;

// Get a handle to the interactive window station.

   hwinsta = OpenWindowStationA(
       "winsta0",                   // the interactive window station 
       FALSE,                       // handle is not inheritable
       READ_CONTROL | WRITE_DAC);   // rights to read/write the DACL

   if (hwinsta == NULL) 
      goto Cleanup;

// To get the correct default desktop, set the caller's 
// window station to the interactive window station.

   if (!SetProcessWindowStation(hwinsta))
      goto Cleanup;

// Get a handle to the interactive desktop.

   hdesk = OpenDesktopA(
      "default",     // the interactive window station 
      0,             // no interaction with other desktop processes
      FALSE,         // handle is not inheritable
      READ_CONTROL | // request the rights to read and write the DACL
      WRITE_DAC | 
      DESKTOP_WRITEOBJECTS | 
      DESKTOP_READOBJECTS);

// Restore the caller's window station.

   if (!SetProcessWindowStation(hwinstaSave)) 
      goto Cleanup;

   if (hdesk == NULL) 
      goto Cleanup;

// Get the SID for the client's logon session.

   if (!GetLogonSID(hToken, &pSid)) 
      goto Cleanup;

// Allow logon SID full access to interactive window station.

   if (! AddAceToWindowStation(hwinsta, pSid) ) 
      goto Cleanup;

// Allow logon SID full access to interactive desktop.

if (!AddAceToDesktop(hdesk, pSid))
goto Cleanup;

// Impersonate client to ensure access to executable file.

if (!ImpersonateLoggedOnUser(hToken))
goto Cleanup;

// Initialize the STARTUPINFO structure.
// Specify that the process runs in the interactive desktop.

ZeroMemory(&si, sizeof(STARTUPINFO));
si.cb = sizeof(STARTUPINFO);
si.lpDesktop = TEXT("winsta0\\default");

// Launch the process in the client's logon session.

bResult = CreateProcessAsUser(
	hToken,            // client's access token
	NULL,              // file to execute
	lpCommandLine,     // command line
	NULL,              // pointer to process SECURITY_ATTRIBUTES
	NULL,              // pointer to thread SECURITY_ATTRIBUTES
	FALSE,             // handles are not inheritable
	NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,   // creation flags
	NULL,              // pointer to new environment block 
	NULL,              // name of current directory 
	&si,               // pointer to STARTUPINFO structure
	&pi                // receives information about new process
);

// End impersonation of client.

RevertToSelf();

// This makes the program to stop till the process started above is closed.
 if (bResult && pi.hProcess != INVALID_HANDLE_VALUE)
{
   WaitForSingleObject(pi.hProcess, INFINITE);
   CloseHandle(pi.hProcess);
}

if (pi.hThread != INVALID_HANDLE_VALUE)
CloseHandle(pi.hThread);

Cleanup:

if (hwinstaSave != NULL)
SetProcessWindowStation(hwinstaSave);

// Free the buffer for the logon SID.

if (pSid)
FreeLogonSID(&pSid);

// Close the handles to the interactive window station and desktop.

if (hwinsta)
CloseWindowStation(hwinsta);

if (hdesk)
CloseDesktop(hdesk);

// Close the handle to the client's access token.

if (hToken != INVALID_HANDLE_VALUE)
CloseHandle(hToken);

return bResult;
}


BOOL StartNonInteractiveClientProcess(
	LPTSTR lpszUsername,    // client to log on
	LPTSTR lpszDomain,      // domain of client's account
	LPTSTR lpszPassword,    // client's password
	LPTSTR lpApplication,    // execute application name
	LPTSTR lpCommandLine    // command line to execute
)
{
	BOOL bResult = FALSE;
	/*STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	
	bResult = CreateProcess(lpApplication,   // No module name (use command line)
		lpCommandLine,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		TRUE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi);          // Pointer to PROCESS_INFORMATION structure

	DWORD Error = GetLastError();
	LogClass::log((int)Error, "test.txt");

	if (bResult && pi.hProcess != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
	}

	if (pi.hThread != INVALID_HANDLE_VALUE)
		CloseHandle(pi.hThread);

	return bResult;
	*/
	_wspawnl(_P_WAIT, lpApplication, lpApplication, lpCommandLine, NULL);
	return TRUE;

}

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid)
{
   ACCESS_ALLOWED_ACE   *pace;
   ACL_SIZE_INFORMATION aclSizeInfo;
   BOOL                 bDaclExist;
   BOOL                 bDaclPresent;
   BOOL                 bSuccess = FALSE;
   DWORD                dwNewAclSize;
   DWORD                dwSidSize = 0;
   DWORD                dwSdSizeNeeded;
   PACL                 pacl;
   PACL                 pNewAcl;
   PSECURITY_DESCRIPTOR psd = NULL;
   PSECURITY_DESCRIPTOR psdNew = NULL;
   PVOID                pTempAce;
   SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
   unsigned int         i;

   __try
   {
      // Obtain the DACL for the window station.

      if (!GetUserObjectSecurity(
             hwinsta,
             &si,
             psd,
             dwSidSize,
             &dwSdSizeNeeded)
      )
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
         psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
               GetProcessHeap(),
               HEAP_ZERO_MEMORY,
               dwSdSizeNeeded);

         if (psd == NULL)
            __leave;

         psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
               GetProcessHeap(),
               HEAP_ZERO_MEMORY,
               dwSdSizeNeeded);

         if (psdNew == NULL)
            __leave;

         dwSidSize = dwSdSizeNeeded;

         if (!GetUserObjectSecurity(
               hwinsta,
               &si,
               psd,
               dwSidSize,
               &dwSdSizeNeeded)
         )
            __leave;
      }
      else
         __leave;

      // Create a new DACL.

      if (!InitializeSecurityDescriptor(
            psdNew,
            SECURITY_DESCRIPTOR_REVISION)
      )
         __leave;

      // Get the DACL from the security descriptor.

      if (!GetSecurityDescriptorDacl(
            psd,
            &bDaclPresent,
            &pacl,
            &bDaclExist)
      )
         __leave;

      // Initialize the ACL.

      ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
      aclSizeInfo.AclBytesInUse = sizeof(ACL);

      // Call only if the DACL is not NULL.

      if (pacl != NULL)
      {
         // get the file ACL size info
         if (!GetAclInformation(
               pacl,
               (LPVOID)&aclSizeInfo,
               sizeof(ACL_SIZE_INFORMATION),
               AclSizeInformation)
         )
            __leave;
      }

      // Compute the size of the new ACL.

      dwNewAclSize = aclSizeInfo.AclBytesInUse + (2*sizeof(ACCESS_ALLOWED_ACE)) + 
(2*GetLengthSid(psid)) - (2*sizeof(DWORD));

      // Allocate memory for the new ACL.

      pNewAcl = (PACL)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            dwNewAclSize);

      if (pNewAcl == NULL)
         __leave;

      // Initialize the new DACL.

      if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
         __leave;

      // If DACL is present, copy it to a new DACL.

      if (bDaclPresent)
      {
         // Copy the ACEs to the new ACL.
         if (aclSizeInfo.AceCount)
         {
            for (i=0; i < aclSizeInfo.AceCount; i++)
            {
               // Get an ACE.
               if (!GetAce(pacl, i, &pTempAce))
                  __leave;

               // Add the ACE to the new ACL.
               if (!AddAce(
                     pNewAcl,
                     ACL_REVISION,
                     MAXDWORD,
                     pTempAce,
                    ((PACE_HEADER)pTempAce)->AceSize)
               )
                  __leave;
            }
         }
      }

      // Add the first ACE to the window station.

      pace = (ACCESS_ALLOWED_ACE *)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) -
                  sizeof(DWORD));

      if (pace == NULL)
         __leave;

      pace->Header.AceType  = ACCESS_ALLOWED_ACE_TYPE;
      pace->Header.AceFlags = CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
      pace->Header.AceSize  = sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD);
      pace->Mask            = GENERIC_ACCESS;

      if (!CopySid(GetLengthSid(psid), &pace->SidStart, psid))
         __leave;

      if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, (LPVOID)pace, pace->Header.AceSize))
         __leave;

      // Add the second ACE to the window station.

      pace->Header.AceFlags = NO_PROPAGATE_INHERIT_ACE;
      pace->Mask = WINSTA_ALL;

      if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, (LPVOID)pace, pace->Header.AceSize))
         __leave;

      // Set a new DACL for the security descriptor.

      if (!SetSecurityDescriptorDacl(
            psdNew,
            TRUE,
            pNewAcl,
            FALSE)
      )
         __leave;

      // Set the new security descriptor for the window station.

      if (!SetUserObjectSecurity(hwinsta, &si, psdNew))
         __leave;

      // Indicate success.

      bSuccess = TRUE;
   }
   __finally
   {
      // Free the allocated buffers.

      if (pace != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)pace);

      if (pNewAcl != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

      if (psd != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

      if (psdNew != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
   }

   return bSuccess;

}

BOOL AddAceToDesktop(HDESK hdesk, PSID psid)
{
   ACL_SIZE_INFORMATION aclSizeInfo;
   BOOL                 bDaclExist;
   BOOL                 bDaclPresent;
   BOOL                 bSuccess = FALSE;
   DWORD                dwNewAclSize;
   DWORD                dwSidSize = 0;
   DWORD                dwSdSizeNeeded;
   PACL                 pacl;
   PACL                 pNewAcl;
   PSECURITY_DESCRIPTOR psd = NULL;
   PSECURITY_DESCRIPTOR psdNew = NULL;
   PVOID                pTempAce;
   SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
   unsigned int         i;

   __try
   {
      // Obtain the security descriptor for the desktop object.

      if (!GetUserObjectSecurity(
            hdesk,
            &si,
            psd,
            dwSidSize,
            &dwSdSizeNeeded))
      {
         if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
         {
            psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
                  GetProcessHeap(),
                  HEAP_ZERO_MEMORY,
                  dwSdSizeNeeded );

            if (psd == NULL)
               __leave;

            psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
                  GetProcessHeap(),
                  HEAP_ZERO_MEMORY,
                  dwSdSizeNeeded);

            if (psdNew == NULL)
               __leave;

            dwSidSize = dwSdSizeNeeded;

            if (!GetUserObjectSecurity(
                  hdesk,
                  &si,
                  psd,
                  dwSidSize,
                  &dwSdSizeNeeded)
            )
               __leave;
         }
         else
            __leave;
      }

      // Create a new security descriptor.

      if (!InitializeSecurityDescriptor(
            psdNew,
            SECURITY_DESCRIPTOR_REVISION)
      )
         __leave;

      // Obtain the DACL from the security descriptor.

      if (!GetSecurityDescriptorDacl(
            psd,
            &bDaclPresent,
            &pacl,
            &bDaclExist)
      )
         __leave;

      // Initialize.

      ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
      aclSizeInfo.AclBytesInUse = sizeof(ACL);

      // Call only if NULL DACL.

      if (pacl != NULL)
      {
         // Determine the size of the ACL information.

         if (!GetAclInformation(
               pacl,
               (LPVOID)&aclSizeInfo,
               sizeof(ACL_SIZE_INFORMATION),
               AclSizeInformation)
         )
            __leave;
      }

      // Compute the size of the new ACL.

      dwNewAclSize = aclSizeInfo.AclBytesInUse + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD);

      // Allocate buffer for the new ACL.

      pNewAcl = (PACL)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            dwNewAclSize);

      if (pNewAcl == NULL)
         __leave;

      // Initialize the new ACL.

      if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
         __leave;

      // If DACL is present, copy it to a new DACL.

      if (bDaclPresent)
      {
         // Copy the ACEs to the new ACL.
         if (aclSizeInfo.AceCount)
         {
            for (i=0; i < aclSizeInfo.AceCount; i++)
            {
               // Get an ACE.
               if (!GetAce(pacl, i, &pTempAce))
                  __leave;

               // Add the ACE to the new ACL.
               if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER)pTempAce)->AceSize))
                  __leave;
            }
         }
      }

      // Add ACE to the DACL.

      if (!AddAccessAllowedAce(pNewAcl, ACL_REVISION, DESKTOP_ALL, psid))
         __leave;

      // Set new DACL to the new security descriptor.

      if (!SetSecurityDescriptorDacl(
            psdNew,
            TRUE,
            pNewAcl,
            FALSE)
      )
         __leave;

      // Set the new security descriptor for the desktop object.

      if (!SetUserObjectSecurity(hdesk, &si, psdNew))
         __leave;

      // Indicate success.

      bSuccess = TRUE;
   }
   __finally
   {
      // Free buffers.

      if (pNewAcl != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

      if (psd != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

      if (psdNew != NULL)
         HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
   }

   return bSuccess;
}


BOOL GetLogonSID (HANDLE hToken, PSID *ppsid) 
{
   BOOL bSuccess = FALSE;
   DWORD dwIndex;
   DWORD dwLength = 0;
   PTOKEN_GROUPS ptg = NULL;

// Verify the parameter passed in isn't NULL.
    if (NULL == ppsid)
        goto Cleanup;

// Get required buffer size and allocate the TOKEN_GROUPS buffer.

   if (!GetTokenInformation(
         hToken,         // handle to the access token
         TokenGroups,    // get information about the token's groups 
         (LPVOID) ptg,   // pointer to TOKEN_GROUPS buffer
         0,              // size of buffer
         &dwLength       // receives required buffer size
      )) 
   {
      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
         goto Cleanup;

      ptg = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(),
         HEAP_ZERO_MEMORY, dwLength);

      if (ptg == NULL)
         goto Cleanup;
   }

// Get the token group information from the access token.

   if (!GetTokenInformation(
         hToken,         // handle to the access token
         TokenGroups,    // get information about the token's groups 
         (LPVOID) ptg,   // pointer to TOKEN_GROUPS buffer
         dwLength,       // size of buffer
         &dwLength       // receives required buffer size
         )) 
   {
      goto Cleanup;
   }

// Loop through the groups to find the logon SID.

   for (dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++) 
      if ((ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID)
             ==  SE_GROUP_LOGON_ID) 
      {
      // Found the logon SID; make a copy of it.

         dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
         *ppsid = (PSID) HeapAlloc(GetProcessHeap(),
                     HEAP_ZERO_MEMORY, dwLength);
         if (*ppsid == NULL)
             goto Cleanup;
         if (!CopySid(dwLength, *ppsid, ptg->Groups[dwIndex].Sid)) 
         {
             HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
             goto Cleanup;
         }
         break;
      }

   bSuccess = TRUE;

Cleanup: 

// Free the buffer for the token groups.

   if (ptg != NULL)
      HeapFree(GetProcessHeap(), 0, (LPVOID)ptg);

   return bSuccess;
}


VOID FreeLogonSID (PSID *ppsid) 
{
    HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
}

// Deletes service
void DeleteTheService()
{
    // Open service manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM == NULL)
    {
        return;
    }
       
    // OPen service
    SC_HANDLE hService = ::OpenService(hSCM, SERVICENAME, SERVICE_ALL_ACCESS);

    if (hService == NULL)
    {
       ::CloseServiceHandle(hSCM);
       return;
    }

    // Deletes service from service database
    DeleteService(hService);

    // Stop the service
    ServiceStatus.dwCurrentState            = SERVICE_STOPPED; 
    ServiceStatus.dwCheckPoint              = 0; 
    ServiceStatus.dwWaitHint                = 0; 
    ServiceStatus.dwWin32ExitCode           = 0; 
    ServiceStatus.dwServiceSpecificExitCode = 0; 
    SetServiceStatus(ServiceStatusHandle, &ServiceStatus); 

   ::CloseServiceHandle(hService);
   ::CloseServiceHandle(hSCM);
}

BOOL SystemShutdown(LPTSTR lpMsg, BOOL bReboot, UINT iTimeOut /*secs*/)
{
   HANDLE hToken;              // handle to process token 
   TOKEN_PRIVILEGES tkp;       // pointer to token structure 
 
   BOOL fResult;               // system shutdown flag 
 
   // Get the current process token handle so we can get shutdown 
   // privilege. 
 
   if (!OpenProcessToken(GetCurrentProcess(), 
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
      return FALSE; 
 
   // Get the LUID for shutdown privilege. 
 
   LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
        &tkp.Privileges[0].Luid); 
 
   tkp.PrivilegeCount = 1;  // one privilege to set    
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
   // Get shutdown privilege for this process. 
 
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
      (PTOKEN_PRIVILEGES) NULL, 0); 
 
   // Cannot test the return value of AdjustTokenPrivileges. 
 
   if (GetLastError() != ERROR_SUCCESS) 
      return FALSE; 
 
   // Display the shutdown dialog box and start the countdown. 
 
   fResult = InitiateSystemShutdown( 
      NULL,    // shut down local computer 
      lpMsg,   // message for user
      iTimeOut,      // time-out period in secs
      TRUE,   // ask user to close apps 
      bReboot);   // reboot after shutdown 
 
   if (!fResult) 
      return FALSE; 
 
   // Disable shutdown privilege. 
 
   tkp.Privileges[0].Attributes = 0; 
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
        (PTOKEN_PRIVILEGES) NULL, 0); 
 
   return TRUE; 
}

BOOL PreventSystemShutdown()
{
   HANDLE hToken;              // handle to process token 
   TOKEN_PRIVILEGES tkp;       // pointer to token structure 
 
   // Get the current process token handle  so we can get shutdown 
   // privilege. 
 
   if (!OpenProcessToken(GetCurrentProcess(), 
         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
      return FALSE; 
 
   // Get the LUID for shutdown privilege. 
 
   LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
         &tkp.Privileges[0].Luid); 
 
   tkp.PrivilegeCount = 1;  // one privilege to set    
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
   // Get shutdown privilege for this process. 
 
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
        (PTOKEN_PRIVILEGES)NULL, 0); 
 
   if (GetLastError() != ERROR_SUCCESS) 
      return FALSE; 
 
   // Prevent the system from shutting down. 
 
   if ( !AbortSystemShutdown(NULL) ) 
      return FALSE; 
 
   // Disable shutdown privilege. 
 
   tkp.Privileges[0].Attributes = 0; 
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
       (PTOKEN_PRIVILEGES) NULL, 0); 
 
   return TRUE;
}


// Get the physical RAM used by the process
SIZE_T GetPhysicalMemUsage(DWORD dwProcessID)
{
    HANDLE hProcess = ::OpenProcess(
                            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 
                            FALSE, 
                            dwProcessID
                            );

    PROCESS_MEMORY_COUNTERS pmc = {0};

    ::GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));
    
    ::CloseHandle(hProcess); 

    return pmc.WorkingSetSize;

}