/*
File Name:		RemoteAdministrator.cpp
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/
#include "stdafx.h"
#include "RemoteAdministrator.h"
#include "GlobalHelperFunc.h"
#include "MachineInfo.h"
#include "Resource.h"
#include <winsvc.h>
#include <Sddl.h>
#include <iostream>
#include "Command.h"
#include <iostream>
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


extern CRITICAL_SECTION g_CriticalSection;

CRemoteAdministrator::CRemoteAdministrator()
{

}

CRemoteAdministrator::~CRemoteAdministrator()
{
	// Clean the connection pending list
	CString* pStr = NULL;
	POSITION pos = m_cplPendingConnectionList.GetHeadPosition();
	while (pos != NULL)
	{
		pStr = m_cplPendingConnectionList.GetNext(pos);

		ASSERT(pStr != NULL);

		if (pStr != NULL)
		{
			delete pStr;
		}
	}
	m_cplPendingConnectionList.RemoveAll();


	// Clean the connected machine list
	CMachineInfo* pMachineInfo = NULL;
	pos = m_milConnectedMachines.GetHeadPosition();
	while (pos != NULL)
	{
		pMachineInfo = m_milConnectedMachines.GetNext(pos);

		ASSERT(pMachineInfo != NULL);

		if (pMachineInfo != NULL)
		{
			delete pMachineInfo;
		}
	}
	m_milConnectedMachines.RemoveAll();
}

BOOL CRemoteAdministrator::EstablishConnection(CString strResource, CString strLogon, CString strRemoteMachineIP, CString strPwd, BOOL bEstablish)
{
	::EnterCriticalSection(&g_CriticalSection);

    TCHAR szRemoteResource[_MAX_PATH];
    DWORD dwRetVal = NO_ERROR;

    // Remote resource, \\remote\ipc$, remote\admin$, ...
    ::wsprintf(szRemoteResource, _T("\\\\%s\\%s"), strRemoteMachineIP.GetBuffer(0), strResource.GetBuffer(0));

    //
    // Disconnect or connect to the resource, based on bEstablish
    //
    if (bEstablish) 
    {
        NETRESOURCE nr;
        nr.dwType = RESOURCETYPE_ANY;
        nr.lpLocalName = NULL;
        nr.lpRemoteName = (LPTSTR)&szRemoteResource;
        nr.lpProvider = NULL;
      
        //Establish connection (using username/pwd)
        dwRetVal = ::WNetAddConnection2(
                        &nr, 
                        strPwd.GetBuffer(0), 
                        strLogon.GetBuffer(0), 
                        FALSE
                        );
        
        // Let the caller generate the error message
        /*if (dwRetVal != NO_ERROR)
        {
            //::PopError(dwRetVal);
            /*CString strFromatAPIMsg = ::FormatError(dwRetVal);
            CString strDisplayMsg;
            strDisplayMsg.Format("Machine IP %s: %s", strRemoteMachineIP.GetBuffer(0), strFromatAPIMsg.GetBuffer(0));
            
            ::AfxMessageBox(strDisplayMsg);*/
        //}*/
    }
    else
    {
        // Disconnect
        dwRetVal = ::WNetCancelConnection2(szRemoteResource, 0, NULL);

        // Let the caller generate the error message
        /*if (dwRetVal != NO_ERROR)
        {
            //::PopError(dwRetVal);
            /*CString strFromatAPIMsg = ::FormatError(dwRetVal);
            CString strDisplayMsg;
            strDisplayMsg.Format("Machine IP %s: %s", strRemoteMachineIP.GetBuffer(0), strFromatAPIMsg.GetBuffer(0));
            
            ::AfxMessageBox(strDisplayMsg);*/
        //}*/
     }
      
    // Prepare the return value
    if (dwRetVal == NO_ERROR) 
    {
		::LeaveCriticalSection(&g_CriticalSection);
        return TRUE; // indicate success
    }

	::LeaveCriticalSection(&g_CriticalSection);
    return FALSE;
}


void CRemoteAdministrator::AddMachine(CMachineInfo& miMachineInfo)
{
    CMachineInfo* pmiMachineInfo = new CMachineInfo;
    
    ASSERT(pmiMachineInfo);

    if (pmiMachineInfo != NULL)
    {
        (*pmiMachineInfo) = miMachineInfo;

        // Update the connected machines list
        m_milConnectedMachines.AddHead(pmiMachineInfo);
		::Sleep(1000);
    }
    else
    {
        ::AfxMessageBox(_T("No memory for new machine details !"));
    }
}

void CRemoteAdministrator::DeleteMachine(CMachineInfo& miMachineInfo)
{
    CMachineInfo* pMachineInfo = NULL;
    POSITION pos = m_milConnectedMachines.GetHeadPosition();
    
	::EnterCriticalSection(&g_CriticalSection);

    while (pos != (POSITION)0xcdcdcdcd && pos != NULL)
    {
        pMachineInfo = m_milConnectedMachines.GetAt(pos);
        
        ASSERT (pMachineInfo);

        if (miMachineInfo == *pMachineInfo) 
        {
            // delete the machine from list
            m_milConnectedMachines.RemoveAt(pos);

            delete pMachineInfo;
            pMachineInfo = NULL;

			::LeaveCriticalSection(&g_CriticalSection);
            return;

        }

        m_milConnectedMachines.GetNext(pos);
    }

	::LeaveCriticalSection(&g_CriticalSection);
}


BOOL CRemoteAdministrator::CheckIfMachinePresent(CString strIP)
{
    // Machine with same IP cannot be added to the monitoring list
    CMachineInfo* pMachineInfo = NULL;
    POSITION pos = m_milConnectedMachines.GetHeadPosition();
    
    while (pos != (POSITION)0xcdcdcdcd && pos != NULL)
    {
        pMachineInfo = m_milConnectedMachines.GetNext(pos);
        
        ASSERT (pMachineInfo);

        if (strIP == pMachineInfo->GetIP()) 
        {
            return TRUE;
        }
    }

    return FALSE;
}


void CRemoteAdministrator::RefreshProcessList(CString strIP, CProcessInfoList& pilProcessList)
{
    CMachineInfo* pMachineInfo = GetMachineInfo(strIP);
    
    ASSERT(pMachineInfo);
    
    if (pMachineInfo != NULL)
    {
        pMachineInfo->RefreshProcessList(pilProcessList);
    }
}

CMachineInfo* CRemoteAdministrator::GetMachineInfo(CString strIP)
{
    CMachineInfo* pMachineInfo = NULL;
    POSITION pos = NULL;

	::EnterCriticalSection(&g_CriticalSection);
    pos = m_milConnectedMachines.GetHeadPosition();

    while (pos != (POSITION)0xcdcdcdcd && pos != NULL)
    {
        pMachineInfo = m_milConnectedMachines.GetNext(pos);

        if (pMachineInfo->GetIP() == strIP)
        {
			::LeaveCriticalSection(&g_CriticalSection);
            return pMachineInfo;
        }
    }

	::LeaveCriticalSection(&g_CriticalSection);

    // No machine with IP strIP is present
    ASSERT (NULL);

    return NULL;
}

BOOL CRemoteAdministrator::EstablishAdminConnection(CString strRemoteMachineIP, CString strPwd, BOOL bEstablish)
{
	CString strResource = _T("ADMIN$");
	CString strLogon    = _T("Administrator");

	BOOL bConnectionSuccess = EstablishConnection(strResource, strLogon, strRemoteMachineIP, strPwd, bEstablish);
    
    if (!bConnectionSuccess)
    {
        CString strFormattedErrorMsg = ErrorHandling::ConvertStringTableIDToErrorMsg(strRemoteMachineIP, IDS_NO_ADMIN_CONNECTION);

        //::AfxMessageBox(strFormattedErrorMsg);
		//::GetMainFrame()->ShowBalloonMsgInTray(_T("Warning"), strFormattedErrorMsg);
    }
    return bConnectionSuccess;
}

BOOL CRemoteAdministrator::EstablishIPCConnection(CString strRemoteMachineIP, CString strPwd, BOOL bEstablish)
{
	CString strResource = _T("IPC$");
	CString strLogon    = _T("Administrator");

    BOOL bConnectionSuccess = EstablishConnection(strResource, strLogon, strRemoteMachineIP, strPwd, bEstablish);

    if (!bConnectionSuccess)
    {
        CString strFormattedErrorMsg = ErrorHandling::ConvertStringTableIDToErrorMsg(strRemoteMachineIP, IDS_NO_IPC_CONNECTION);
        
        //::AfxMessageBox(strFormattedErrorMsg);
		//::GetMainFrame()->ShowBalloonMsgInTray(_T("Warning"), strFormattedErrorMsg);
    }
    return bConnectionSuccess;
}


BOOL CRemoteAdministrator::EstablishAllConnections(CString strRemoteMachineIP, CString strPwd, BOOL bEstablish)
{
    BOOL bSuccessAdminConnection = EstablishAdminConnection(strRemoteMachineIP, strPwd, bEstablish);
    BOOL bSuccessIPCConnection   = EstablishIPCConnection(strRemoteMachineIP, strPwd, bEstablish);
    
    return (bSuccessAdminConnection && bSuccessIPCConnection);
}


BOOL CRemoteAdministrator::CopyServiceExeToRemoteMachine(CString strRemoteMachineIP)
{
   DWORD dwWritten = 0;

   HMODULE hInstance = ::GetModuleHandle(NULL);
/*#if defined(_M_X64)
   // Find the binary file in resources
   HRSRC hServiceExecutableRes = ::FindResource(
	   hInstance,
	   MAKEINTRESOURCE(IDR_EXECUTEREMOTE64),
	   _T("EXECUTABLES64")
   );
#else
   // Find the binary file in resources
   HRSRC hServiceExecutableRes = ::FindResource(
	   hInstance,
	   MAKEINTRESOURCE(IDR_EXECUTEREMOTE64),
	   _T("EXECUTABLES32")
   );
#endif // _M_X64

  */


  // Find the binary file in resources
   HRSRC hServiceExecutableRes = ::FindResource(
	   hInstance,
	   MAKEINTRESOURCE(IDR_EXECUTEREMOTE32),
	   _T("EXECUTABLES32")
   );

   HGLOBAL hServiceExecutable = ::LoadResource( 
                                    hInstance, 
                                    hServiceExecutableRes
                                    );

   LPVOID pServiceExecutable = ::LockResource(hServiceExecutable);

   if (pServiceExecutable == NULL)
      return FALSE;

   DWORD dwServiceExecutableSize = ::SizeofResource(
                                   hInstance,
                                   hServiceExecutableRes
                                   );

   TCHAR szServiceExePath[_MAX_PATH];

   ::wsprintf(
       szServiceExePath, 
       _T("\\\\%s\\ADMIN$\\%s"), 
       strRemoteMachineIP.GetBuffer(0), 
       REMOTE_ADMIN_SERVICE_EXE
       );

   // Copy binary file from resources to \\remote\ADMIN$\System32
   HANDLE hFileServiceExecutable = ::CreateFile( 
                                        szServiceExePath,
                                        GENERIC_WRITE,
                                        0,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL
                                        );

   if (hFileServiceExecutable == INVALID_HANDLE_VALUE)
   {
       return FALSE;
   }
         
   ::WriteFile(hFileServiceExecutable, pServiceExecutable, dwServiceExecutableSize, &dwWritten, NULL);

   ::CloseHandle(hFileServiceExecutable);
   
   return (dwWritten == dwServiceExecutableSize);
}
BOOL CRemoteAdministrator::OnExecuteProcess(CString RemoteIp, CString AdminName, CString AdminPass, CString Domain, CString ExeName , CString CommandLine , CString ProcessType)
{

	HANDLE hRemoteAdminProcessExecutePipe = GetRemoteAdminProcessExecutePipe(RemoteIp);

	if (hRemoteAdminProcessExecutePipe != NULL)
	{
		// Send a command to continue the thread that will execute the remote process
		SCommand cmd;
		cmd.m_bThreadExit = FALSE;

		// Execute the command on the remote machine with the following parameters
		SExecuteCommand ExeCmd = { 0 };
		
		//std::string strDomain = CW2A(Domain.GetString());
		//std::string strAdminName = CW2A(AdminName.GetString());
		//std::string strAdminPass = CW2A(AdminPass.GetString());
		//std::string strExeName = CW2A(ExeName.GetString());



		::memcpy(ExeCmd.m_szDomain, Domain.GetBuffer(0), _MAX_PATH);
		::memcpy(ExeCmd.m_szPassword, AdminPass.GetBuffer(0), _MAX_PATH);
		::memcpy(ExeCmd.m_szProcessPath, ExeName.GetBuffer(0), _MAX_PATH);
		::memcpy(ExeCmd.m_szUsername, AdminName.GetBuffer(0), _MAX_PATH);
		::memcpy(ExeCmd.m_szCommandLine, CommandLine.GetBuffer(0), _MAX_PATH);
		::memcpy(ExeCmd.m_szService, ProcessType.GetBuffer(0), _MAX_PATH);


		BOOL bOk = FALSE;
		DWORD dwWritten = 0;

		bOk = ::WriteFile(hRemoteAdminProcessExecutePipe, &cmd, sizeof(SCommand), &dwWritten, NULL);
		if (!bOk)
			return 1;
		bOk = ::WriteFile(hRemoteAdminProcessExecutePipe, &ExeCmd, sizeof(SExecuteCommand), &dwWritten, NULL);
		if (!bOk)
			return 1;

		TCHAR szMessage[_MAX_PATH] = _T("");
		DWORD dwRead = 0;

		// Wait for process triggering acknowledgement.
		bOk = ::ReadFile(hRemoteAdminProcessExecutePipe, szMessage, sizeof(szMessage), &dwRead, NULL);
 
		//handling new message from service
		if (::_tcscmp(szMessage, _T("")) == 0)
		{
			return 0;
		}
		else if (::_tcscmp(szMessage, _T("1")) == 0)
		{
			return 1;
		}
		//////////////////////////////////
	}
	//}
}


BOOL CRemoteAdministrator::ConnectToRemoteService(CString strRemoteMachineIP, DWORD dwRetry, DWORD dwRetryTimeOut)
{
    TCHAR szRemoteAdminPipeName[_MAX_PATH]               = _T("");
    TCHAR szRemoteAdminProcessInfoPipeName[_MAX_PATH]    = _T("");
    TCHAR szRemoteAdminProcessExecutePipeName[_MAX_PATH] = _T("");
    TCHAR szRemoteAdminProcessKillPipeName[_MAX_PATH]    = _T("");
    TCHAR szRemoteAdminSysShutdownPipe[_MAX_PATH]        = _T("");
    
    HANDLE hCommandPipe = INVALID_HANDLE_VALUE;

    // Remote service communication pipe name
    ::wsprintf(
        szRemoteAdminPipeName, 
        _T("\\\\%s\\pipe\\%s"), 
        strRemoteMachineIP.GetBuffer(0), 
        REMOTE_ADMIN_PIPE
        );

    // Remote service communication pipe name
    ::wsprintf(
        szRemoteAdminProcessInfoPipeName, 
        _T("\\\\%s\\pipe\\%s"), 
        strRemoteMachineIP.GetBuffer(0), 
        REMOTE_ADMIN_PROCESS_INFO_PIPE
        );

    // Remote service communication pipe name
    ::wsprintf(
        szRemoteAdminProcessExecutePipeName, 
        _T("\\\\%s\\pipe\\%s"), 
        strRemoteMachineIP.GetBuffer(0), 
        REMOTE_ADMIN_PROCESS_EXECUTE_PIPE
        );

    // Remote service communication pipe name
    ::wsprintf(
        szRemoteAdminProcessKillPipeName, 
        _T("\\\\%s\\pipe\\%s"), 
        strRemoteMachineIP.GetBuffer(0), 
        REMOTE_ADMIN_PROCESS_KILL_PIPE
        );

    // Remote shutdown pipe
    ::wsprintf(
        szRemoteAdminSysShutdownPipe,
        _T("\\\\%s\\pipe\\%s"), 
        strRemoteMachineIP.GetBuffer(0), 
        REMOTE_ADMIN_SYS_SHUTDOWN_PIPE
        );


    SECURITY_ATTRIBUTES SecAttrib = {0};
    SECURITY_DESCRIPTOR SecDesc;
    ::InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION);
    ::SetSecurityDescriptorDacl(&SecDesc, TRUE, NULL, TRUE);

    SecAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecAttrib.lpSecurityDescriptor = &SecDesc;;
    SecAttrib.bInheritHandle = TRUE;

    // Connects to the remote service's communication pipe
    while(dwRetry--)
    {
        if (::WaitNamedPipe(szRemoteAdminPipeName, 5000))
        {
            hCommandPipe = ::CreateFile( 
                                 szRemoteAdminPipeName,
                                 GENERIC_WRITE | GENERIC_READ, 
                                 0,
                                 &SecAttrib, 
                                 OPEN_EXISTING, 
                                 FILE_ATTRIBUTE_NORMAL, 
                                 NULL
                                 );

             ::CloseHandle(hCommandPipe);  

             CMachineInfo* pMachineInfo = GetMachineInfo(strRemoteMachineIP);
             //pMachineInfo->SetRemoteAdminPipe(hCommandPipe);

             ::Sleep(10000);
             if (::WaitNamedPipe(szRemoteAdminProcessInfoPipeName, 5000))
             {
                 hCommandPipe = ::CreateFile( 
                                 szRemoteAdminProcessInfoPipeName,
                                 GENERIC_WRITE | GENERIC_READ, 
                                 0,
                                 &SecAttrib, 
                                 OPEN_EXISTING, 
                                 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 
                                 NULL
                                 );

                 pMachineInfo->SetRemoteAdminProcessInfoPipe(hCommandPipe);
             }

             if (::WaitNamedPipe(szRemoteAdminProcessExecutePipeName, 5000))
             {
                 hCommandPipe = ::CreateFile( 
                                 szRemoteAdminProcessExecutePipeName,
                                 GENERIC_WRITE | GENERIC_READ, 
                                 0,
                                 &SecAttrib, 
                                 OPEN_EXISTING, 
                                 FILE_ATTRIBUTE_NORMAL, 
                                 NULL
                                 );

                 pMachineInfo->SetRemoteAdminProcessExecutePipe(hCommandPipe);
             }

             if (::WaitNamedPipe(szRemoteAdminProcessKillPipeName, 5000))
             {
                 hCommandPipe = ::CreateFile( 
                                 szRemoteAdminProcessKillPipeName,
                                 GENERIC_WRITE | GENERIC_READ, 
                                 0,
                                 &SecAttrib, 
                                 OPEN_EXISTING, 
                                 FILE_ATTRIBUTE_NORMAL, 
                                 NULL
                                 );

                 pMachineInfo->SetRemoteAdminProcessKillPipe(hCommandPipe);
             }

             if (::WaitNamedPipe(szRemoteAdminSysShutdownPipe, 5000))
             {
                 hCommandPipe = ::CreateFile( 
                                 szRemoteAdminSysShutdownPipe,
                                 GENERIC_WRITE | GENERIC_READ, 
                                 0,
                                 &SecAttrib, 
                                 OPEN_EXISTING, 
                                 FILE_ATTRIBUTE_NORMAL, 
                                 NULL
                                 );

                 pMachineInfo->SetRemoteAdminSysShutDownPipe(hCommandPipe);
             }

            
             break;
         }
         else
         {
             // Let's try it again
             ::Sleep(dwRetryTimeOut);
         }
    }

    return hCommandPipe != INVALID_HANDLE_VALUE;
}

BOOL CRemoteAdministrator::InstallAndStartRemoteService(CString strRemoteMachineIP, CString AdminName, CString AdminPass)
{
	/*NETRESOURCE NetResource;
	NetResource.dwScope = RESOURCE_GLOBALNET;
	NetResource.dwType = RESOURCETYPE_ANY;
	NetResource.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
	NetResource.dwUsage = RESOURCEUSAGE_CONNECTABLE;
	NetResource.lpLocalName = NULL;
	CString tempclientName = _T("\\\\") + strRemoteMachineIP;
	NetResource.lpRemoteName = tempclientName.GetBuffer(MAX_PATH);
	NetResource.lpComment = NULL;
	NetResource.lpProvider = NULL;

	CMachineInfo* pMachineInfo  = new CMachineInfo();


	if (WNetAddConnection2(&NetResource, AdminPass, AdminName, FALSE) == NO_ERROR)
	{
		printf("");
	}
	*/
	

    // Open remote Service Manager
    SC_HANDLE hSCM = ::OpenSCManager(
                         strRemoteMachineIP.GetBuffer(0), 
                         NULL, 
                         SC_MANAGER_ALL_ACCESS
                         );

    if (hSCM == NULL)
    {
        return FALSE;
    }
      
	
    // Maybe it's already there and installed, let's try to run
    SC_HANDLE hService =::OpenService(hSCM, SERVICENAME, SERVICE_ALL_ACCESS);

	
    // Creates service on remote machine, if it's not installed yet
    if (hService == NULL)
    {
        hService = ::CreateService(
                        hSCM, 
                        SERVICENAME, 
                        LONGSERVICENAME,
                        SERVICE_ALL_ACCESS, 
                        SERVICE_WIN32_OWN_PROCESS,
                        SERVICE_DEMAND_START, 
                        SERVICE_ERROR_NORMAL,
                       _T("%SystemRoot%\\")REMOTE_ADMIN_SERVICE_EXE,
                        NULL, 
                        NULL, 
                        NULL, 
                        NULL, 
                        NULL
                        );
    }
      
   
	if (hService == NULL)
	{
		::CloseServiceHandle(hSCM);
		return FALSE;
	}

	// Start service
	if (!::StartService(hService, 0, NULL))
	{
		DWORD temp = GetLastError();
		return FALSE;
	}

	::CloseServiceHandle(hService);
	::CloseServiceHandle(hSCM);


    return TRUE;
}


BOOL CRemoteAdministrator::RemoveRemoteService(CString strRemoteMachineIP)
{
	
	// Open remote Service Manager
	SC_HANDLE hSCM = ::OpenSCManager(
		strRemoteMachineIP.GetBuffer(0),
		NULL,
		SC_MANAGER_ALL_ACCESS
	);

	if (hSCM == NULL)
	{
		return FALSE;
	}


	// Maybe it's already there and installed, let's try to run
	SC_HANDLE hService = ::OpenService(hSCM, SERVICENAME, SERVICE_ALL_ACCESS);

	if (hService == NULL)
	{
		::CloseServiceHandle(hSCM);
		return FALSE;
	}

	if (!DeleteService(hService))
	{
		return FALSE;
	}
	::CloseServiceHandle(hService);
	::CloseServiceHandle(hSCM);

	return TRUE;
}


CProcessInfoList* CRemoteAdministrator::GetProcessInfoList(CString strRemoteMachineIP)
{
    CMachineInfo* pMachineInfo = GetMachineInfo(strRemoteMachineIP);
    
    return pMachineInfo->GetProcessInfoList();
}

void CRemoteAdministrator::DeleteAndDisconnectAllMachines()
{
    CMachineInfo* pMachineInfo = NULL;

    POSITION pos = m_milConnectedMachines.GetHeadPosition();
    while (pos != (POSITION)0xcdcdcdcd && pos != NULL)
    {
        pMachineInfo = m_milConnectedMachines.GetNext(pos);

        // Tell server threads to end
        pMachineInfo->SendEndThreadMessage();
        
        // Close the pipe handles
        pMachineInfo->ClosePipeHandles();

        // End the connection
        EstablishAllConnections(pMachineInfo->GetIP(), pMachineInfo->GetPassword(), FALSE);

        // Free memory 
        delete pMachineInfo;
    }

    m_milConnectedMachines.RemoveAll();
}

void CRemoteAdministrator::DeleteAndDisconnectMachine(CString strRemoteAdminMachine)
{
    CMachineInfo* pMachineInfo = GetMachineInfo(strRemoteAdminMachine);

	if (pMachineInfo)
	{
		pMachineInfo->SendEndThreadMessage();
		pMachineInfo->ClosePipeHandles();
		BOOL bConnectionDeleted = EstablishAllConnections(pMachineInfo->GetIP(), pMachineInfo->GetPassword(), FALSE);
        DeleteMachine(*pMachineInfo);
	}
}


void CRemoteAdministrator::GetConnectedMachinesIP(CString** pstrConnctedMachinesIP/*out*/, int* piNumberOfConnectedMachines /*out*/)
{
	::EnterCriticalSection(&g_CriticalSection);
    // Get the number of machines
    *piNumberOfConnectedMachines = m_milConnectedMachines.GetCount();

    // Allocate array of CStrings to hold IP of these machines
    *pstrConnctedMachinesIP = new CString[*piNumberOfConnectedMachines];

    // Now loop throught the machine info list and upadte the IP array
    CMachineInfo* pMachineInfo = NULL;
    POSITION pos = m_milConnectedMachines.GetHeadPosition();
    int iIndex = 0;
    while (pos != (POSITION)0xcdcdcdcd && pos != NULL)
    {
        pMachineInfo = m_milConnectedMachines.GetNext(pos);
        (*pstrConnctedMachinesIP)[iIndex] = pMachineInfo->GetIP();
        ++iIndex;
    }
	::LeaveCriticalSection(&g_CriticalSection);
}

int CRemoteAdministrator::GetTotalMachinesMonitored()
{
    return m_milConnectedMachines.GetCount();
}

BOOL CRemoteAdministrator::AddToConnectionPendingList(CString strIP)
{
    CString* pstrPendinConnectionIP = new CString;
    if (pstrPendinConnectionIP != NULL)
    {
        *pstrPendinConnectionIP = strIP;
        m_cplPendingConnectionList.AddTail(pstrPendinConnectionIP);

        return TRUE;
    }

    return FALSE;
}

BOOL CRemoteAdministrator::RemoveFromConnecionPendingList(CString strIP)
{
    POSITION pos = m_cplPendingConnectionList.GetHeadPosition();

    while (pos != (POSITION)0xcdcdcdcd && pos != NULL)
    {
        CString* pstrIP = m_cplPendingConnectionList.GetAt(pos);

        if (*pstrIP == strIP)
        {
            m_cplPendingConnectionList.RemoveAt(pos);

            delete pstrIP;

            return TRUE;
        }

        m_cplPendingConnectionList.GetNext(pos);
    }

    return FALSE;
}

BOOL CRemoteAdministrator::IsConnectionPending(CString strIP)
{
    POSITION pos = m_cplPendingConnectionList.GetHeadPosition();

    while (pos != (POSITION)0xcdcdcdcd && pos != NULL)
    {
        CString* pstrIP = m_cplPendingConnectionList.GetNext(pos);

        if (*pstrIP == strIP)
        {
             return TRUE;
        }
    }

    return FALSE;
}

CString CRemoteAdministrator::GetPassword(CString strIP)
{
	CMachineInfo* pMachineInfo = GetMachineInfo(strIP);

	ASSERT(pMachineInfo != NULL);

	if (pMachineInfo != NULL)
	{
		return pMachineInfo->GetPassword();
	}
	else
	{
		return _T("Could not retrieve the password");
	}
}

UINT CRemoteAdministrator::GetNumberOfMachineConnectionsStillInProgress()
{
	return m_cplPendingConnectionList.GetCount();
}


CString CRemoteAdministrator::GetComputerNameFromIP(CString& strIP)
{
	std::string tempIp = CT2CA(strIP);
	unsigned long ulAddr = ::inet_addr(tempIp.c_str());
	CString strHostName = _T("0");

	if (ulAddr != INADDR_NONE)  // Valid IP address passed, hence could resolve address
	{
		HOSTENT* pHostEnt    = ::gethostbyaddr((char*)&ulAddr, sizeof(ulAddr), AF_INET);
		if (pHostEnt != NULL)
		{
			strHostName = pHostEnt->h_addr;

			return strHostName;
		}
	}

	ASSERT(NULL); // Shouldn't reach here, if Winsock initialized properly
    return strHostName;	
}


CString CRemoteAdministrator::GetComputerIPFromName(CString& strComputerName)
{
	std::string tempComputerName = CT2CA(strComputerName);
	unsigned long ulAddr = ::inet_addr(tempComputerName.c_str());
	CString strIP = _T("0");

	if (ulAddr == INADDR_NONE) // Not a valid IP in dotted format, so can be a string
	{
		HOSTENT* pHostEnt = ::gethostbyname(tempComputerName.c_str());
		if (pHostEnt != NULL)
		{
			in_addr* addr = (in_addr*)pHostEnt->h_addr_list[0]; 
			
			int ip1 = addr->S_un.S_un_b.s_b1;
            int ip2 = addr->S_un.S_un_b.s_b2;
            int ip3 = addr->S_un.S_un_b.s_b3;
            int ip4 = addr->S_un.S_un_b.s_b4;

			strIP.Format(L"%d.%d.%d.%d", ip1, ip2, ip3, ip4);
			return strIP;
		}
	}

	ASSERT(NULL); // Shouldn't reach here, if Winsock initialized properly
    return strIP;	
}