/*
File Name:		RemoteAdministrator.h
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/
#ifndef REMOTEADMINISTRATOR_H
#define REMOTEADMINISTRATOR_H

#pragma once
//#include "stdafx.h"
#include <afxtempl.h>
#include "MachineInfo.h"

typedef CTypedPtrList<CPtrList, CString*>      CConnectionPendingList;
typedef CTypedPtrList<CPtrList, CMachineInfo*> CMachineInfoList;

#define  REMOTE_ADMIN_SERVICE              _T("ExecuteRemoteService")
#define  REMOTE_ADMIN_SERVICE_EXE          _T("ExecuteRemoteService.exe")
#define  REMOTE_ADMIN_SERVICE_PIPE         _T("ExecuteRemoteServicePipe")
#define  SERVICENAME                       _T("ExecuteRemoteService")
#define  LONGSERVICENAME                   _T("ExecuteRemoteService")
#define  REMOTE_ADMIN_PIPE                 _T("ExecuteRemotePipe")
#define  REMOTE_ADMIN_PROCESS_INFO_PIPE    _T("ExecuteRemoteProcessInfoPipe")
#define  REMOTE_ADMIN_PROCESS_EXECUTE_PIPE _T("ExecuteRemoteProcessExecutePipe")
#define  REMOTE_ADMIN_PROCESS_KILL_PIPE    _T("ExecuteRemoteProcessKillPipe")
#define  REMOTE_ADMIN_SYS_SHUTDOWN_PIPE    _T("ExecuteRemoteSysShutDownPipe")
 

class CRemoteAdministrator  
{
public:
	CRemoteAdministrator();
	virtual ~CRemoteAdministrator();

    BOOL EstablishAllConnections(CString strRemoteMachineIP, CString strPwd, BOOL bEstablish);
    void AddMachine(CMachineInfo& miMachineInfo);
    void DeleteMachine(CMachineInfo& miMachineInfo);
    BOOL CheckIfMachinePresent(CString strIP);
    void RefreshProcessList(CString strIP, CProcessInfoList& pilProcessList);
    CMachineInfo* GetMachineInfo(CString strIP);
    BOOL CopyServiceExeToRemoteMachine(CString strRenoteMachineIP);
    BOOL ConnectToRemoteService(CString strRemoteMachineIP, DWORD dwRetry, DWORD dwRetryTimeOut);
    BOOL InstallAndStartRemoteService(CString strRemoteMachineIP , CString AdminName, CString AdminPass);
	BOOL RemoveRemoteService(CString strRemoteMachineIP);
    CProcessInfoList* GetProcessInfoList(CString strRemoteMachineIP);
    void DeleteAndDisconnectAllMachines();
    void DeleteAndDisconnectMachine(CString strRemoteAdminMachine);
    void GetConnectedMachinesIP(CString** pstrConnctedMachinesIP /*out*/, int* piNumberOfConnectedMachines/*out*/);
    int GetTotalMachinesMonitored();
    BOOL AddToConnectionPendingList(CString strIP);
    BOOL RemoveFromConnecionPendingList(CString strIP);
    BOOL IsConnectionPending(CString strIP);
	BOOL OnExecuteProcess(CString RemoteIp , CString AdminName, CString AdminPass, CString Domain, CString ExeName , CString CommandLine , CString ProcessType);
    HANDLE GetRemoteAdminPipe(CString strRemoteMachineIP);
    HANDLE GetRemoteAdminProcessInfoPipe(CString strRemoteMachineIP);
    HANDLE GetRemoteAdminProcessExecutePipe(CString strRemoteMachineIP);
    HANDLE GetRemoteAdminProcessKillPipe(CString strRemoteMachineIP);
    HANDLE GetRemoteAdminSysShutDownPipe(CString strRemoteMachineIP);
	CString GetPassword(CString strIP);
	UINT GetNumberOfMachineConnectionsStillInProgress();
	CString GetComputerNameFromIP(CString& strIP);
	CString GetComputerIPFromName(CString& strComputerName);
    
protected:
    CMachineInfoList m_milConnectedMachines;
    CConnectionPendingList m_cplPendingConnectionList;

    HANDLE m_hCommandPipe; 
    BOOL EstablishAdminConnection(CString strRemoteMachineIP, CString strPwd, BOOL bEstablish);
    BOOL EstablishIPCConnection(CString strRemoteMachineIP, CString strPwd, BOOL bEstablish);
    BOOL EstablishConnection(CString strResource, CString strLogon, CString strRemoteMachineIP, CString strPwd, BOOL bEstablish);
};


inline HANDLE CRemoteAdministrator::GetRemoteAdminPipe(CString strRemoteMachineIP)
{
    CMachineInfo* pMachineInfo = GetMachineInfo(strRemoteMachineIP);

    if (pMachineInfo != NULL)
        return pMachineInfo->GetRemoteAdminPipe();
    else
        return NULL;
}

inline HANDLE CRemoteAdministrator::GetRemoteAdminProcessInfoPipe(CString strRemoteMachineIP)
{
    CMachineInfo* pMachineInfo = GetMachineInfo(strRemoteMachineIP);

    if (pMachineInfo != NULL)
        return pMachineInfo->GetRemoteAdminProcessInfoPipe();
    else
        return NULL;
}

inline HANDLE CRemoteAdministrator::GetRemoteAdminProcessExecutePipe(CString strRemoteMachineIP)
{
    CMachineInfo* pMachineInfo = GetMachineInfo(strRemoteMachineIP);
    
    if (pMachineInfo != NULL)
       return pMachineInfo->GetRemoteAdminProcessExecutePipe();
    else
      return NULL;
}

inline HANDLE CRemoteAdministrator::GetRemoteAdminProcessKillPipe(CString strRemoteMachineIP)
{
    CMachineInfo* pMachineInfo = GetMachineInfo(strRemoteMachineIP);

    if (pMachineInfo != NULL)
        return pMachineInfo->GetRemoteAdminProcessKillPipe();
    else
        return NULL;
}

inline HANDLE CRemoteAdministrator::GetRemoteAdminSysShutDownPipe(CString strRemoteMachineIP)
{
    CMachineInfo* pMachineInfo = GetMachineInfo(strRemoteMachineIP);

    if (pMachineInfo != NULL)
        return pMachineInfo->GetRemoteAdminSysShutDownPipe();
    else
        return NULL;
}

#endif // REMOTEADMINISTRATOR_H
