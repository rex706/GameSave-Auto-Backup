// GameSave Auto-Backup.cpp : Copies save files from the communal dropbox account to keep saves up to date.

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "stdafx.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>
#include <vector>
#include <string>
#include "time.h"
#include <iostream>
using namespace std;

void dirExists(const char *path){

	struct stat info;

	if (stat(path, &info) != 0)
		printf("Cannot access %s\n\n", path);
	else if (info.st_mode & S_IFDIR)
		printf("DropBox directory found!\n\n");
	else
		printf("Unable to locate DropBox directory. Are you connected to the Internet?\n\n");
}

//gets path where exe was opened from (should be the game folder)
string ExePath() {
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	string::size_type pos = string(buffer).find_last_of("\\/");
	return string(buffer).substr(0, pos);
}

int main()
{

	//check for internet connection. if no internet display error and ask if still want to launch game 
	//check if dropbox directory exists
	//should probably find a way to compare date modified to skip retrieving files if they are the same

	std::string gamePath = ExePath() + "\\ts4.exe";

	const char *path = "\%UserProfile%\\Dropbox\\saves\0";
	dirExists(path);

	cout << "Loading save files from DropBox...\n";
	system("xcopy \"\%UserProfile%\\Dropbox\\saves\" \"\%userProfile%\\Documents\\Electronic Arts\\Sims 4\\saves\" /e /i /j");

	//find way to bypass overwrite prompt or auto input 'a' to indicate 'overwrite all'
	
	//convert string to stupid LPWSTR type
	int bufferlen = ::MultiByteToWideChar(CP_ACP, 0, gamePath.c_str(), gamePath.size(), NULL, 0);
	LPWSTR widestr = new WCHAR[bufferlen + 1];
	::MultiByteToWideChar(CP_ACP, 0, gamePath.c_str(), gamePath.size(), &widestr[0], bufferlen);
	widestr[bufferlen] = 0;

	//launch game and wait for it to close
	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;
	if (CreateProcess(widestr, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
	{
		::WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else {
		cout << "ERROR could not find game executable!\n\n";
		system("PAUSE");
		return 0;
	}

	delete[] widestr;

	//need to figure out how to only upload files that changed instead of entire directory
	cout << "\nUploading save files to DropBox...\n";
	system("xcopy \"\%UserProfile%\\Documents\\Electronic Arts\\Sims 4\\saves\" \"\%userProfile%\\Dropbox\\saves\" /e /i /j");

	system("PAUSE"); //debug
    return 0;
}