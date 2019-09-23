/*
File Name:		MachineInfo.cpp
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/
#include "stdafx.h"
#include "ExecuteRemoteProcess.h"
#include "MachineInfo.h"
#include "command.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_SERIAL(CMachineInfo, CObject, 1)

extern CRITICAL_SECTION g_CriticalSection;

CMachineInfo::CMachineInfo()
{
    m_strPwd = _T("");
    m_strIP  = _T("");

    m_hRemoteAdminPipe               = NULL;
    m_hRemoteAdminProcessExecutePipe = NULL;
    m_hRemoteAdminProcessInfoPipe    = NULL;
    m_hRemoteAdminProcessKillPipe    = NULL;
    m_hRemoteAdminSysShutDownPipe    = NULL;
}

CMachineInfo::~CMachineInfo()
{
	// Free the m_pilProcessList
	POSITION pos = m_pilProcessList.GetHeadPosition();

	while(pos != NULL)
	{
		//PROCESSENTRY32* pProcessEntry = m_pilProcessList.GetNext(pos);
        SProcessInfo* pProcessEntry = m_pilProcessList.GetNext(pos);

		if (pProcessEntry)
		{
			delete pProcessEntry;
		}
	}

	m_pilProcessList.RemoveAll();
}

CMachineInfo& CMachineInfo::operator = (CMachineInfo& miMachineInfo)
{
    m_strPwd   = miMachineInfo.GetPassword();
    m_strIP    = miMachineInfo.GetIP();
    m_strLogon = miMachineInfo.GetLogon();

    return *this;
}

BOOL CMachineInfo::operator == (CMachineInfo& miMachineInfo)
{
    if (m_strIP == miMachineInfo.GetIP() && 
        m_strPwd == miMachineInfo.GetPassword() &&
        m_strLogon == miMachineInfo.GetLogon())
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void CMachineInfo::RefreshProcessList(CProcessInfoList& pilList)
{
	::EnterCriticalSection(&g_CriticalSection);

    int iProcessCount = m_pilProcessList.GetCount();
    POSITION pos = NULL;
    //PROCESSENTRY32* pProcessEntry = NULL;
    SProcessInfo* pProcessEntry = NULL;

    // Free all allocated memeory in the existing inner list
	/*for (int i = 0; i < iProcessCount; ++i)
    {
        pos = m_pilProcessList.FindIndex(i);
        
		if (pos != (POSITION)0xcdcdcdcd && pos != NULL)
		{
			pProcessEntry = m_pilProcessList.GetAt(pos);

			if (pProcessEntry != NULL)
			{
				delete pProcessEntry;
			}
		}
    }*/

	pos = m_pilProcessList.GetHeadPosition();
	while (pos != NULL)
	{
		pProcessEntry = m_pilProcessList.GetNext(pos);
		if (pProcessEntry != NULL)
		{
			delete pProcessEntry;
		}
		
	}
	
	// Remove all PROCESSENTRY32 from the list
    m_pilProcessList.RemoveAll();

    
    // Update the prcoessinfolist with new PROCESSENTRY32's, i.e make copies
    // of the new PROCESSENTRY32's and store them in the m_pilProcessList
    iProcessCount = pilList.GetCount();

    // Keep making new copies of new PROCESSENTRY32's and add to the process list
   /* for (int i = 0; i < iProcessCount; ++i)
    {
        pProcessEntry = new PROCESSENTRY32;

        if (pProcessEntry != NULL)
        {
            pos = pilList.FindIndex(i);

            ::memcpy(pProcessEntry, pilList.GetAt(pos), sizeof(PROCESSENTRY32));
		
			m_pilProcessList.AddTail(pProcessEntry);
		}
        else
        {
            ::AfxMessageBox(IDS_NO_MEMORY_FOR_NEW_PROCESSINFO);
        }
    }*/

	pos = pilList.GetHeadPosition();
	while (pos != NULL)
	{
		//pProcessEntry = new PROCESSENTRY32;
        pProcessEntry = new SProcessInfo;
		if (pProcessEntry != NULL)
		{
			::memcpy(pProcessEntry, pilList.GetAt(pos), sizeof(SProcessInfo));
			m_pilProcessList.AddTail(pProcessEntry);
		}
		else
        {
            MessageBox(NULL , L"Alert", L"No memory could be allocated for process info !" , MB_OK | MB_ICONWARNING);
        }

		// Move ahead to next iteration
		pilList.GetNext(pos);
	}

	::LeaveCriticalSection(&g_CriticalSection);
}

CProcessInfoList* CMachineInfo::GetProcessInfoList()
{
	::EnterCriticalSection(&g_CriticalSection);
    // Make a copy of the existing list and return the copy
    CProcessInfoList* pProcessInfoList = new CProcessInfoList;

    POSITION pos = m_pilProcessList.GetHeadPosition();
    while (pos != (POSITION)0xcdcdcdcd && pos != NULL)
    {
        //PROCESSENTRY32* pPe = pProcessInfoList->GetNext(pos);
		//PROCESSENTRY32* pPe = m_pilProcessList.GetNext(pos);
        //PROCESSENTRY32* p = new PROCESSENTRY32;
        SProcessInfo* pPe = m_pilProcessList.GetNext(pos);
        SProcessInfo* p = new SProcessInfo;
        
		if (p != NULL)
		{
			// make a copy and add to the list which the copy of the original list
			if (pPe != NULL)
			{
				::memcpy(p, pPe, sizeof(SProcessInfo));

				pProcessInfoList->AddTail(p);
			}
		}
    }

	::LeaveCriticalSection(&g_CriticalSection);
    return pProcessInfoList;
}

void CMachineInfo::ClosePipeHandles()
{
    //::CloseHandle(m_hRemoteAdminPipe);
    ::CloseHandle(m_hRemoteAdminProcessInfoPipe);
    ::CloseHandle(m_hRemoteAdminProcessExecutePipe);
    ::CloseHandle(m_hRemoteAdminProcessKillPipe);
    ::CloseHandle(m_hRemoteAdminSysShutDownPipe);
}

void CMachineInfo::SendEndThreadMessage()
{
    DWORD dwRead    = 0;
    DWORD dwWritten = 0;
    SCommand cmd    = {0};
    BOOL bOk        = FALSE;

    cmd.m_bThreadExit = TRUE;

    // Send the server process to end the threads
   // bOk = ::WriteFile(m_hRemoteAdminPipe,               &cmd, sizeof(SCommand), &dwWritten, NULL);
    bOk = ::WriteFile(m_hRemoteAdminProcessInfoPipe,    &cmd, sizeof(SCommand), &dwWritten, NULL);
    bOk = ::WriteFile(m_hRemoteAdminProcessExecutePipe, &cmd, sizeof(SCommand), &dwWritten, NULL);
    bOk = ::WriteFile(m_hRemoteAdminProcessKillPipe,    &cmd, sizeof(SCommand), &dwWritten, NULL);
    bOk = ::WriteFile(m_hRemoteAdminSysShutDownPipe,    &cmd, sizeof(SCommand), &dwWritten, NULL);
}


void CMachineInfo::Serialize(CArchive& ar)
{
    CObject::Serialize(ar);

    // Only three variables are enough to be
    // serialized. Rest variables are not important.
    if (ar.IsLoading())
    {
        ar >> m_strIP;
        ar >> m_strPwd;
        ar >> m_strLogon;
    }
    else
    {
        ar << m_strIP;
        ar << m_strPwd;
        ar << m_strLogon;
    }
}

