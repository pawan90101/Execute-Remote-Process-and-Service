/*
File Name:		LogClass.h
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/
#pragma once
/*
LPSTR = char*
LPCSTR = const char*
LPWSTR = wchar_t*
LPCWSTR = const wchar_t*
LPTSTR = char* or wchar_t* depending on _UNICODE
LPCTSTR =  const char* or const wchar_t* depending on _UNICODE
*/
/*
mode 1 = Open file for write or overwrite from start;
mode 2 = All output operations happen at the end of the file, appending to its existing contents. 
mode 3 = Any contents that existed in the file before it is open are discarded.
*/


#include  <Windows.h>

class LogClass
{
public:
	static void log(const char* logv,const char* path,int mode = 2);
	static void log(int logv, const char* path , int mode = 2);
	static void log(int logv, const wchar_t* path , int mode = 2);
	static void log(const wchar_t* logv, const wchar_t* path , int mod = 2);
};