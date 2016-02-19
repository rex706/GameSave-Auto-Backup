// GameSave Auto-Backup.cpp : Copies save files to and from a Google Drive account to keep saves up to date.

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
#include <cstring>
#include <sstream>
#include <iostream>
using namespace std;
stringstream ss;

int dirExists(const char *path){

	struct stat info;

	if (stat(path, &info) != 0) {
		printf("\nCannot access %s\n", path);
		return 0;
	}
	else if (info.st_mode & S_IFDIR) {
		printf("\nGoogle Drive directory found! '%s'\n", path);
		return 1;
	}
	else {
		printf("\nUnable to locate Google Drive directory. Make sure it is installed and signed in.\n");
		return 0;
	}
}

//gets path where exe was opened from (should be the game folder for now)
string ExePath() {
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	string::size_type pos = string(buffer).find_last_of("\\/");
	return string(buffer).substr(0, pos);
}

int main(int argc, char** argv){

	//check for internet connection. if no internet display error and ask if still want to launch game 
	if (InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0))
		cout << "Connected to the Internet!\n";

	else {
		cout << "WARNING: No Internet connection found. \n | Save files cannot be fetched but will still attempt to update on exit. Play anyway? Y/N\n";
	
		char response;
		cin >> response;

		if (response == 'N' || response == 'n' || response == 'No' || response == 'no') {
			return 0;
		}
	}

	//check if Google Drive directory exists
	char path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
	strcat_s(path, "\\Google Drive\\Game Saves\\The Sims 4");
	int dir = dirExists(path);
	if (dir == 0) {
		cout << "\nERROR: Could not locate Google Drive directory. Make sure it is installed and signed in, then try again.";
		system("PAUSE");
		return 0;
	}

	//fetch files from Google Drive
	cout << "\nLoading save files from Google Drive...\n";
	system("xcopy \"\%UserProfile%\\Google Drive\\Game Saves\\The Sims 4\" \"\%UserProfile%\\Documents\\Electronic Arts\\The Sims 4\\saves\" /e /i /j /d /y");
			
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
		cout << "\nERROR: Could not find game executable! Is the launcher in the correct directory?\n\n";
		system("PAUSE");
		return 0;
	}

	delete[] widestr;

	//find a way to deal with multiple people playing and saving at the same time

	//check to make sure there is enough space to transfer files. 50 mb at least

	//push files to Google Drive
	cout << "\nUploading save files to Google Drive...\n";
	system("xcopy \"\%UserProfile%\\Documents\\Electronic Arts\\The Sims 4\\saves\" \"\%UserProfile%\\Google Drive\\Game Saves\\The Sims 4\" /e /i /j /d /y");
	cout << "\nYour save(s) have been copied! It is now safe to exit.\n";
	system("PAUSE"); //debug
    return 0;
}