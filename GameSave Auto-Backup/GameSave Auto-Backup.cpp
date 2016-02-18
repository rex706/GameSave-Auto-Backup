// GameSave Auto-Backup.cpp : Copies save files from the communal dropbox account to keep saves up to date.

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "stdafx.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>
#include <wininet.h>
#pragma comment(lib,"Wininet.lib")
#include <shlobj.h>
#include <string>
#include <iostream>
using namespace std;

void dirExists(const char *path){

	struct stat info;

	if (stat(path, &info) != 0)
		printf("\nCannot access %s\n\n", path);
	else if (info.st_mode & S_IFDIR)
		printf("\nDropBox directory found! '%s'\n", path);
	else
		printf("\nUnable to locate DropBox directory. Make sure it is installed and signed in.\n");
}

//gets path where exe was opened from (should be the game folder)
string ExePath() {
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	string::size_type pos = string(buffer).find_last_of("\\/");
	return string(buffer).substr(0, pos);
}

int main(int argc, char** argv){

	//check for internet connection. if no internet display error and ask if still want to launch game 
	if (InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0))
		cout << "\nConnected to the Internet!\n";

	else {
		cout << "\nWARNING: No Internet connection found. Save files cannot be synced. Play anyway? Y/N\n";
	
		char response;
		cin >> response;

		if (response == 'N' || response == 'n' || response == 'No' || response == 'no') {
			return 0;
		}
	}

	//check if dropbox directory exists
	char path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
	strcat_s(path, "\\Dropbox\\saves");
	dirExists(path);

	//fetch files from DropBox
	cout << "\nLoading save files from DropBox...\n";
	system("xcopy \"\%UserProfile%\\Dropbox\\saves\" \"\%UserProfile%\\Documents\\Electronic Arts\\The Sims 4\\saves\" /e /i /j /d /y");
	
	//convert string to stupid LPWSTR type so it is compatable with CreateProcess function
	std::string gamePath = ExePath() + "\\ts4.exe";
	int bufferlen = ::MultiByteToWideChar(CP_ACP, 0, gamePath.c_str(), gamePath.size(), NULL, 0);
	LPWSTR widestr = new WCHAR[bufferlen + 1];
	::MultiByteToWideChar(CP_ACP, 0, gamePath.c_str(), gamePath.size(), &widestr[0], bufferlen);
	widestr[bufferlen] = 0;

	//launch game and wait for it to close
	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;
	if (CreateProcess(widestr, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
	{
		cout << "\nStarting Game!\n";
		::WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		cout << "\nGame closing...\n";
	}
	else {
		cout << "ERROR: Could not find game executable! Is the launcher in the correct directory?\n\n";
		system("PAUSE");
		return 0;
	}

	delete[] widestr;

	//find a way to deal with multiple people playing and saving at the same time

	//push files to DropBox
	cout << "\nUploading save files to DropBox...\n";
	system("xcopy \"\%UserProfile%\\Documents\\Electronic Arts\\The Sims 4\\saves\" \"\%UserProfile%\\Dropbox\\saves\" /e /i /j /d /y");
	cout << "\nYour saves have been copied! It is now safe to exit.";
	system("PAUSE"); //debug
    return 0;
}