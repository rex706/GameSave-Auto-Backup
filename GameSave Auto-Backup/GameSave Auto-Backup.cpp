// GameSave Auto-Backup.cpp : Copies save files to and from a Google Drive account to keep saves up to date.

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "stdafx.h"

#include <dirent.h>
#include <boost/filesystem.hpp>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>
#include <wininet.h>
#pragma comment(lib,"Wininet.lib")
#include <shlobj.h>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;
stringstream ss;

bool CopyDir(boost::filesystem::path const & source, boost::filesystem::path const & destination){
	try
	{
		// Check whether the function call is valid
		if (!boost::filesystem::exists(source) || !boost::filesystem::is_directory(source)){

			std::cerr << "Source directory "
				<< source.string()
				<< " does not exist or is not a directory."
				<< '\n';
			return false;
		}

		if (boost::filesystem::exists(destination)){

			std::cerr << "Destination directory '"
				<< destination.string()
				<< "' already exists." << '\n'
			    << " | Removing... \n";
			boost::filesystem::remove_all(destination);
			Sleep(4000);
		}

		// Create the destination directory
		if (!boost::filesystem::create_directory(destination)){

			std::cerr << "Unable to create destination directory"
				<< destination.string() << '\n';
			return false;
		}
	}

	catch (boost::filesystem::filesystem_error const & e){
		std::cerr << e.what() << '\n';
		return false;
	}

	// Iterate through the source directory
	for (boost::filesystem::directory_iterator file(source); file != boost::filesystem::directory_iterator(); ++file){

		try
		{
			boost::filesystem::path current(file->path());
			if (boost::filesystem::is_directory(current)){

				// Found directory: Recursion
				if (!CopyDir(current, destination / current.filename())){
					return false;
				}
			}
			else{
				// Found file: Copy
				boost::filesystem::copy_file(current, destination / current.filename());
				cout << " | Copying files...\n";
			}
		}catch (boost::filesystem::filesystem_error const & e){
			std::cerr << e.what() << '\n\n';
		}
	}

	return true;
}

int dirExists(const char *path){

	struct stat info;

	if (stat(path, &info) != 0) {
		printf("\nCannot access %s\n", path);
		return 0;
	}
	else if(info.st_mode & S_IFDIR) {
		printf("\nGoogle Drive directory found! '%s'\n", path);
		return 1;
	}
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
	strcat_s(path, "\\Google Drive\\Game Saves");
	int dirReturn = dirExists(path);
	if (dirReturn == 0) {
		cout << "\nERROR: Could not locate Google Drive directory. Make sure it is installed and signed in, then try again.\n";
		system("PAUSE");
		return 0;
	}

	//list foldres in Google Drive 'Game Saves' directory
	//figure out how to make these selectable - maybe load a saved file settings.txt in selected directory, which contains path info for game exe and local save location
	//if file not found, make one and ask for directories. write and save the file
	DIR *dir = opendir(path);
	int k = 0;
	std::vector<string> games;
	string temp;

	struct dirent *entry = readdir(dir);
	cout << "\nSave folders found:\n";

	while (entry != NULL){

		if (entry->d_type == DT_DIR) {
			
			temp = entry->d_name;

			if (temp.length() > 2) {

				games.push_back(entry->d_name);

				cout << k + 1 << ". " << games[k] << "\n";
				k++;
			}
		}
		entry = readdir(dir);
	}
	closedir(dir);

	string gameChoice;
	string localSavePath;
	string exePath;
	char newPath[MAX_PATH];
	char settings[MAX_PATH];
	strcpy_s(newPath, path);
	int choice;

	std::ifstream in_stream;

	//if there is more than one option, have the user pick
	if (k >= 2) {
		cout << "\nEnter the corresponding number of the game you would like to load: ";
		cin >> choice;
		choice = choice - 1;

		strcat_s(newPath, "\\");
		strcat_s(newPath, games[choice].c_str());

		strcpy_s(settings, newPath);
		strcat_s(settings, "\\settings.txt");

		in_stream.open(settings);
		if (!in_stream.good()) {
			std::cout << "Cannot read " << settings << endl;
			system("PAUSE");
			return 0;
		}

		getline(in_stream, localSavePath);
		getline(in_stream, exePath);

		printf("\n NEW PATH:   %s\n", newPath);
	}
	//if there is only one option, use as default
	else {
		strcat_s(newPath, "\\");
		strcat_s(newPath, games[0].c_str());

		strcpy_s(settings, newPath);
		strcat_s(settings, "\\settings.txt");

		in_stream.open(settings);
		if (!in_stream.good()) {
			std::cout << "Cannot read " << settings << endl;
			system("PAUSE");
			return 0;
		}

		getline(in_stream, localSavePath);
		getline(in_stream, exePath);

		printf("\n NEW PATH:   %s\n", newPath);
	}
	in_stream.close();

	//fetch files from Google Drive
	cout << "\nLoading save files from Google Drive...\n";
	if (CopyDir(boost::filesystem::path(newPath), boost::filesystem::path(localSavePath.c_str()))) {
		cout << "\nCopy complete!\n";
	}
	else {
		cout << "ERROR: Copy failed! Something went wrong! Try again?\n";
		system("PAUSE");
		return 0;
	}
	//convert string to stupid LPWSTR type so it is compatable with CreateProcess function
	int bufferlen = ::MultiByteToWideChar(CP_ACP, 0, exePath.c_str(), exePath.size(), NULL, 0);
	LPWSTR widestr = new WCHAR[bufferlen + 1];
	::MultiByteToWideChar(CP_ACP, 0, exePath.c_str(), exePath.size(), &widestr[0], bufferlen);
	widestr[bufferlen] = 0;

	//launch game and wait for it to close
	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;
	if (CreateProcess(widestr, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
	{
		cout << "\nStarting Game! Do not close this window!\n";
		::WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		cout << "\nGame closing...\n";
	}
	else {
		cout << "\nERROR: Could not find game executable at: '" <<  exePath << "'\nCheck exe path in settings file?\n\n";
		system("PAUSE");
		return 0;
	}

	delete[] widestr;

	//find a way to deal with multiple users playing and saving at the same time
	//check to make sure there is enough space to transfer files. 50 mb at least?

	//push files to Google Drive
	cout << "\nUploading save files to Google Drive...\n";
	if (CopyDir(boost::filesystem::path(localSavePath.c_str()), boost::filesystem::path(newPath))) {
		cout << "\nYour save(s) have been copied! It is now safe to exit.\n\nMAKE SURE GOOGLE DRIVE SYNCS BEFORE SHUTTING DOWN\n";
	}
	else {
		cout << "\nERROR: Copy failed! Something went wrong! Try again?\n";
		system("PAUSE");
		return 0;
	}

	system("PAUSE"); //debug
    return 0;
}