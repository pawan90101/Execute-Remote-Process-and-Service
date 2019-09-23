/*
File Name:		MachineInfo.h
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/
#ifndef MACHINEINFO_H
#define MACHINEINFO_H

#pragma once


#include <afxtempl.h>
#include <Tlhelp32.h>

struct SProcessInfo
{
    PROCESSENTRY32 peProcessEntry;
    SIZE_T         stMemUsage;
};

//typedef CTypedPtrList<CPtrList, PROCESSENTRY32*> CProcessInfoList;
typedef CTypedPtrList<CPtrList, SProcessInfo*> CProcessInfoList;



class CMachineInfo : public CObject  
{
    DECLARE_SERIAL(CMachineInfo)
public:
	CMachineInfo();
	virtual ~CMachineInfo();

    void SetIP(CString strIP);
    CString GetIP();

    void SetPassword(CString strPwd);
    CString GetPassword();

    void SetLogon(CString m_strLogon);
    CString GetLogon();

    void SetRemoteAdminPipe(HANDLE handle);
    HANDLE GetRemoteAdminPipe();

    void SetRemoteAdminProcessInfoPipe(HANDLE handle);
    HANDLE GetRemoteAdminProcessInfoPipe();

    void SetRemoteAdminProcessExecutePipe(HANDLE handle);
    HANDLE GetRemoteAdminProcessExecutePipe();

    void SetRemoteAdminProcessKillPipe(HANDLE handle);
    HANDLE GetRemoteAdminProcessKillPipe();

    void SetRemoteAdminSysShutDownPipe(HANDLE handle);
    HANDLE GetRemoteAdminSysShutDownPipe();

    CMachineInfo& operator = (CMachineInfo& miMachineInfo); /* Assignment operator */
    BOOL operator == (CMachineInfo& miMachineInfo);

    void RefreshProcessList(CProcessInfoList& pilList);
    CProcessInfoList* GetProcessInfoList();

    void ClosePipeHandles();
    void SendEndThreadMessage();
    
    virtual void Serialize(CArchive& ar);

protected:
    CString m_strIP;
    CString m_strPwd;
    CString m_strLogon;
    CProcessInfoList m_pilProcessList;
    HANDLE m_hRemoteAdminPipe;
    HANDLE m_hRemoteAdminProcessInfoPipe;
    HANDLE m_hRemoteAdminProcessExecutePipe;
    HANDLE m_hRemoteAdminProcessKillPipe;
    HANDLE m_hRemoteAdminSysShutDownPipe;
};


inline void CMachineInfo::SetIP(CString strIP)
{
    m_strIP = strIP;
}

inline CString CMachineInfo::GetIP()
{
    return m_strIP;
}

inline void CMachineInfo::SetPassword(CString strPwd)
{
    m_strPwd = strPwd;
}

inline CString CMachineInfo::GetPassword()
{
    return m_strPwd;
}

inline void CMachineInfo::SetLogon(CString strLogon)
{
    m_strLogon = strLogon;
}

inline CString CMachineInfo::GetLogon()
{
    return m_strLogon;
}

inline void CMachineInfo::SetRemoteAdminPipe(HANDLE handle)
{
    m_hRemoteAdminPipe = handle;
}

inline HANDLE CMachineInfo::GetRemoteAdminPipe()
{
    return m_hRemoteAdminPipe;
}

inline void CMachineInfo::SetRemoteAdminProcessInfoPipe(HANDLE handle)
{
    m_hRemoteAdminProcessInfoPipe = handle;
}

inline HANDLE CMachineInfo::GetRemoteAdminProcessInfoPipe()
{
    return m_hRemoteAdminProcessInfoPipe;
}

inline void CMachineInfo::SetRemoteAdminProcessExecutePipe(HANDLE handle)
{
    m_hRemoteAdminProcessExecutePipe = handle;
}

inline HANDLE CMachineInfo::GetRemoteAdminProcessExecutePipe()
{
    return m_hRemoteAdminProcessExecutePipe;
}

inline void CMachineInfo::SetRemoteAdminProcessKillPipe(HANDLE handle)
{
    m_hRemoteAdminProcessKillPipe = handle;
}

inline HANDLE CMachineInfo::GetRemoteAdminProcessKillPipe()
{
    return m_hRemoteAdminProcessKillPipe;
}

inline void CMachineInfo::SetRemoteAdminSysShutDownPipe(HANDLE handle)
{
    m_hRemoteAdminSysShutDownPipe = handle;
}

inline HANDLE CMachineInfo::GetRemoteAdminSysShutDownPipe()
{
    return m_hRemoteAdminSysShutDownPipe;
}
#endif // MACHINEINFO_H
