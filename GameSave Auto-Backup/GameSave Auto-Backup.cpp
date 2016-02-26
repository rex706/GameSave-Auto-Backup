// GameSave Auto-Backup.cpp : Copies save files to and from a Google Drive account to keep saves up to date.

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "stdafx.h"

#include <boost/filesystem.hpp>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>
#include <Shellapi.h>
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

string ExePath() {

	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	string::size_type pos = string(buffer).find_last_of("\\/");
	return string(buffer).substr(0, pos);
}

bool CopyDir(boost::filesystem::path const & source, boost::filesystem::path const & destination){
	try
	{
		// Check whether the function call is valid
		if (!boost::filesystem::exists(source) || !boost::filesystem::is_directory(source)){

			cerr << "Source directory " << source.string() << " does not exist or is not a directory." << '\n';
			return false;
		}
		if (!boost::filesystem::exists(destination)) {

			cout << "Destination folder not found. Creating directory...\n";

			boost::filesystem::create_directory(destination);
		}
		//compares date modified between source and destination
		/*if (boost::filesystem::exists(destination)) {

			std::cerr << "\nDestination directory '" << destination.string() << "' already exists." << '\n'
                      << " | Checking date modified... \n";


			time_t sourceTime = boost::filesystem::last_write_time(source);
			time_t destTime = boost::filesystem::last_write_time(destination);

			cout << " | Source: " << sourceTime << "    Destination: " << destTime << "\n\n";

			if (sourceTime <= destTime) {
				cout << " | Files up to date!\n";
				return true;
			}
			else
				cout << " | Files are out of date! Attempting to update...\n";
		}*/
	}

	catch (boost::filesystem::filesystem_error const & e){
		cerr << e.what() << '\n';
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
				boost::filesystem::copy_file(current, destination / current.filename(), boost::filesystem::copy_option::overwrite_if_exists);
				cout << " | Copying files...\n";
			}
		}catch (boost::filesystem::filesystem_error const & e){
			cerr << e.what() << '\n\n';
		}
	}

	return true;
}

int main(int argc, char* argv[]){

	//check for internet connection. if no internet display error and ask if still want to launch game 
	if (InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0))
		cout << "Connected to the Internet!\n";

	else {
		cout << "WARNING: No Internet connection found. \n | Save files cannot be fetched but will still attempt to update on exit. Play anyway? Y/N\n";
	
		char response;
		cin >> response;

		if (response == 'N' || response == 'n' || response == 'No' || response == 'no') 
			return 0;
	}

	//check if Google Drive directory exists
	char googlePath[MAX_PATH];
	char path[MAX_PATH];
	SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, googlePath);
	strcpy_s(path, googlePath);
	strcat_s(googlePath, "\\Google Drive");
	strcat_s(path, "\\Google Drive\\Game Saves");
	
	if (!boost::filesystem::exists(googlePath)) {
		cout << "\nERROR: Could not locate Google Drive directory. Make sure it is installed and signed in, then try again.\n";
		system("PAUSE");
		return 0;
	}
	else {
		cout << "\nGoogle Drive save directory found! '" << googlePath << "'\n";
	}

	if (!boost::filesystem::exists(path)) {

		cout << "\nGame Save folder not found! Creating directory...\n";
		boost::filesystem::create_directory(path);
	}

	string localSavePath;
	string gameName;
	string cloudSavePath;
	string exeName;
	string params;
	string settings;

	ifstream in_stream;

	settings = ExePath() + "\\backup_info.txt";

	if (!boost::filesystem::exists(settings)) {

		cout << "Info file not found! Create one? Y/N: ";
		char response;
		cin >> response;

		if (response == 'N' || response == 'n' || response == 'No' || response == 'no') {

			cout << "\n\nYou must create an info file!";
			system("PAUSE");
			return 0;
		}

		else {

			string localOut;
			string cloudOut;
			string exeOut;
			string paramOut;

			cin.sync();
			cin.get();

			ofstream output("backup_info.txt");
			cout << "\n\nEnter local save folder path (use '\\\' format):\n";
			getline(cin, localOut);
			output << localOut + "\n";
			cout << "\nEnter name of game:\n";
			getline(cin, cloudOut);
			output << cloudOut + "\n";
			cout << "\nEnter game exe name:\n";
			getline(cin, exeOut);
			output << exeOut + "\n";
			cout << "\n(OPTIONAL) Enter launch parameters:\n";
			getline(cin, paramOut);
			output << paramOut;
			output.close();
			
			cout << "\nInfo file created successfully!\n";
		}
		
	}

	in_stream.open(settings);
	if (!in_stream.good()) {
		cout << "Cannot read " << settings << endl;
		system("PAUSE");
		return 0;
	}

	cout << "Reading backup_settings.txt...\n";
	getline(in_stream, localSavePath);
	getline(in_stream, gameName);
	getline(in_stream, exeName);
	getline(in_stream, params);
	in_stream.close();

	cloudSavePath = string(path) + "\\" + gameName;

	//fetch files from Google Drive
	if (boost::filesystem::exists(cloudSavePath)) {
		cout << "\nLoading save files from Google Drive...\n";
		if (CopyDir(boost::filesystem::path(cloudSavePath), boost::filesystem::path(localSavePath))) {
			cout << "\nCopy complete!\n";
		}
		else {
			cout << "ERROR: Copy failed! Something went wrong! Try again?\n";
			system("PAUSE");
			return 0;
		}
	}
	string exePath = ExePath() + "\\" + exeName;

	//convert strings to LPWSTR type so it is compatable with ShellExecute
	int bufferlen = MultiByteToWideChar(CP_ACP, 0, exePath.c_str(), exePath.size(), NULL, 0);
	LPWSTR widestr = new WCHAR[bufferlen + 1];
	MultiByteToWideChar(CP_ACP, 0, exePath.c_str(), exePath.size() , &widestr[0], bufferlen);
	widestr[bufferlen] = 0;

	bufferlen = MultiByteToWideChar(CP_ACP, 0, params.c_str(), params.size(), NULL, 0);
	LPWSTR widestrParams = new WCHAR[bufferlen + 1];
	MultiByteToWideChar(CP_ACP, 0, params.c_str(), params.size(), &widestrParams[0], bufferlen);
	widestrParams[bufferlen] = 0;

	//launch game and wait for it to close
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = _T("runas");
	ShExecInfo.lpFile = widestr;
	ShExecInfo.lpParameters = widestrParams;
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;

	if (ShellExecuteEx(&ShExecInfo)) {
		cout << "\nStarting Game...\n";
		WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
		CloseHandle(ShExecInfo.hProcess);
		cout << "\nGame closing...\n";
	}
	else {
		cout << "\nERROR: Could not find game executable at: '" <<  exePath << "'\nCheck exe path in settings file and the launcher is in the game folder!\n\n";
		system("PAUSE");
		return 0;
	}

	delete[] widestr;
	delete[] widestrParams;

	//find a way to deal with multiple users playing and saving at the same time

	//check to make sure there is enough space to transfer files. 100 mb at least
	boost::filesystem::space_info s = boost::filesystem::space(path);
	cout << "\nBytes\n"
	     << "Total: " << s.capacity << '\n'
	     << "Free: " << s.free << '\n'
	     << "Available: " << s.available << '\n';

	//push files to Google Drive
	cout << "\nUploading save files to Google Drive...\n";
	if (CopyDir(boost::filesystem::path(localSavePath), boost::filesystem::path(cloudSavePath))) {
		cout << "\nYour save(s) have been copied! It is now safe to exit.\n\nMAKE SURE GOOGLE DRIVE SYNCS\n";
	}
	else {
		cout << "\nERROR: Copy failed! Something went wrong! Try again?\n";
		system("PAUSE");
		return 0;
	}

	system("PAUSE");
    return 0;
}