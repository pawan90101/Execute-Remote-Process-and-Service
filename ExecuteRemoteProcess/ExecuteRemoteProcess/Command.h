/*
File Name:		Command.h
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/
#ifndef COMMAND_H
#define COMMAND_H


struct SCommand  // Server machine uses this for ending threads
{
    BOOL m_bThreadExit;  // TRUE == exit thread, FALSE == continue
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


// SConnectInfo is passed to the thread function, ConnectToMachine()
struct CammandParameter
{
	CString		AdminName;
	CString		ClientIp;
	CString		AdminPass;
	CString		AdminDomain;
	CString		ApplicationName;
};

struct SSysShutDownInfo
{
    BOOL bShutDown;   // TRUE if you want to shutdown,FALSE if want to cancel shutdown
    BOOL bReboot;     // Reboot if TRUE, else HALT if FALSE
    UINT iTimeToShutDown; // Time given to user before shutdown in secs
};


#endif // COMMAND_H