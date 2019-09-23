/*
File Name:		LogClass.cpp
Created By:		Pawan Kumar
Date:			23-9-2019  02:17 PM
Last Modified:
Desc:
Usage:
Copyright (c)2019 Pawan90101@gmail.com
*/

// LogClass.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "LogClass.h"
#include <time.h>
#include <stdlib.h>

void LogClass::log(const char* logv, const char* path, int mode)
{
	std::fstream  fs;
	std::fstream::openmode OpenMode;


	switch (mode)
	{
	case 1:
		OpenMode = std::fstream::out;
		break;
	case 2:
		OpenMode = std::fstream::app;
		break;
	case 3:
		OpenMode = std::fstream::trunc;
		break;
	}

	char szTime[30];
	time_t rawtime;
//	size_t   i;
	struct tm timeinfo;
	wchar_t buffer[30];
	time(&rawtime);
	localtime_s(&timeinfo,&rawtime);
	wcsftime(buffer, 30, L"%Y-%m-%d %H:%M:%S", &timeinfo);
	wcstombs_s(NULL,szTime,30, buffer,30);

	fs.open(path, std::fstream::out | OpenMode);
	if (fs.is_open())
	{
		fs << szTime <<" - "<< logv <<std::endl;
		fs.close();
	}
}

void LogClass::log(const wchar_t* logv, const wchar_t* path, int mode)
{
	std::wfstream  fs;
	std::wfstream::openmode OpenMode;


	switch (mode)
	{
	case 1:
		OpenMode = std::wfstream::out;
		break;
	case 2:
		OpenMode = std::wfstream::app;
		break;
	case 3:
		OpenMode = std::wfstream::trunc;
		break;
	}

	//char szTime[30];
	time_t rawtime;
	//	size_t   i;
	struct tm timeinfo;
	wchar_t buffer[30];
	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);
	wcsftime(buffer, 30, L"%Y-%m-%d %H:%M:%S", &timeinfo);
	//wcstombs_s(NULL, szTime, 30, buffer, 30);

	fs.open(path, std::wfstream::out | OpenMode);
	if (fs.is_open())
	{
		fs << buffer << L" - " << logv << std::endl;
		fs.close();
	}
}


void LogClass::log(int logv, const char* path, int mode)
{
	std::fstream  fs;
	std::fstream::openmode OpenMode;


	switch (mode)
	{
	case 1:
		OpenMode = std::fstream::out;
		break;
	case 2:
		OpenMode = std::fstream::app;
		break;
	case 3:
		OpenMode = std::fstream::trunc;
		break;
	}

	char szTime[30];
	time_t rawtime;
	//	size_t   i;
	struct tm timeinfo;
	wchar_t buffer[30];
	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);
	wcsftime(buffer, 30, L"%Y-%m-%d %H:%M:%S", &timeinfo);
	wcstombs_s(NULL, szTime, 30, buffer, 30);

	fs.open(path, std::fstream::out | OpenMode);
	if (fs.is_open())
	{
		fs << szTime << " - " << logv << std::endl;
		fs.close();
	}
}

void LogClass::log(int logv, const wchar_t* path, int mode)
{
	std::wfstream  fs;
	std::wfstream::openmode OpenMode;


	switch (mode)
	{
	case 1:
		OpenMode = std::wfstream::out;
		break;
	case 2:
		OpenMode = std::wfstream::app;
		break;
	case 3:
		OpenMode = std::wfstream::trunc;
		break;
	}

	//char szTime[30];
	time_t rawtime;
	//	size_t   i;
	struct tm timeinfo;
	wchar_t buffer[30];
	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);
	wcsftime(buffer, 30, L"%Y-%m-%d %H:%M:%S", &timeinfo);
	//wcstombs_s(NULL, szTime, 30, buffer, 30);

	fs.open(path, std::wfstream::out | OpenMode);
	if (fs.is_open())
	{
		fs << buffer << L" - " << logv << std::endl;
		fs.close();
	}
}


