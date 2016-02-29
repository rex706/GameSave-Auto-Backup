//File renamed to GameSave-Auto-Backup.cpp. Source files and files used with source code should not have
//spaces in their names. This breaks some systems, and is generally not good practice. I won't correct all the
//files with spaces, just keep this in mind for later projects.

// GameSave-Auto-Backup.cpp : Copies save files to and from a Google Drive account to keep saves up to date.

//You are compiling for windows only, so _CRT_SECURE_NO_WARNINGS is not necessary. When using the C API
//functions, if the compiler asks you to use a "_s" (safe) version of the function, use that instead.
//#ifdef _MSC_VER
//#define _CRT_SECURE_NO_WARNINGS
//#endif

//Includes moved to stdafx.h. Remember, when using precompiled headers, stdafx.h (or whatever you name your
//precompiled header) must always be the very first line of code in a .cpp file.
#include "stdafx.h"
#include "COMInitializer.h"

#define pause system("pause")

using namespace std;

wstring ExePath() {

	wchar_t buffer[MAX_PATH];
	GetModuleFileName(nullptr, buffer, MAX_PATH);
	wstring::size_type pos = wstring(buffer).find_last_of(L"\\/");
	return wstring(buffer).substr(0, pos);
}

bool CopyDir(boost::filesystem::path const & source, boost::filesystem::path const & destination) {
	try
	{
		// Check whether the function call is valid
		if (!boost::filesystem::exists(source) || !boost::filesystem::is_directory(source)) {

			cerr << "Source directory " << source.string() << " does not exist or is not a directory." << '\n';
			return false;
		}
		if (!boost::filesystem::exists(destination)) {

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

	catch (boost::filesystem::filesystem_error const & e) {
		
		cerr << "Directory creation of \"" << destination.filename() << "\" has failed. Error: "
			<< e.what() << endl;
		return false;
	}

	// Iterate through the source directory
	for (boost::filesystem::directory_iterator file(source); file != boost::filesystem::directory_iterator(); ++file) {
		
		boost::filesystem::path current(file->path());
		try {
			
			if (boost::filesystem::is_directory(current)) {

				// Found directory: Recursion
				bool copyResult = CopyDir(current, destination / current.filename());
				//If something went wrong while copying the above directory, do not continue
				//trying to copy other files or directories.
				if (!copyResult)
					return false;
			}
			else {
				
				// Found file: Copy
				boost::filesystem::copy_file(current, destination / current.filename(), boost::filesystem::copy_option::overwrite_if_exists);
				//We can probably think of a better way to indicate progress than printing "copying files" every time a
				//file is copied. To be honest, this can probably be left out. Game save information is usually no more
				//than a few megabytes, if even that. Computers can copy that much data almost instantaneously.
				//cout << " | Copying files...\n";
			}
			
		}
		catch (boost::filesystem::filesystem_error const & e) {
			
			cerr << "File copy operation of \"" << current.filename() << "\" failed. Error: "
				<< e.what() << '\n\n';
			return false;
		}
	}

	return true;
}

HRESULT ShowFileSelectDialog(bool useFolderDialog, const wstring& dialogTitle, 
 	UINT fileFilterLength, COMDLG_FILTERSPEC* fileFilter, wstring& selectedItemPath) {
	
	//Create a file open dialog object.
	HRESULT result;
	CComPtr<IFileOpenDialog> openDialog;
	CComPtr<IShellItem> selectedItem;
	CComHeapPtr<WCHAR> filePathPtr;
	result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&openDialog));
	if (FAILED(result)) {
		cout << "Unable to create an instance of an IFileOpenDialog!" << endl;
		return result;
	}
	//Set a title.
	openDialog->SetTitle(dialogTitle.c_str());
	//Make sure the dialog only shows normal file system objects. 
	//Make the dialog a folder chooser as well, if needed.
	DWORD dialogOptions;
	openDialog->GetOptions(&dialogOptions);
	if (useFolderDialog)
		dialogOptions |= FOS_PICKFOLDERS;
	openDialog->SetOptions(dialogOptions | FOS_FORCEFILESYSTEM);
	//If this is a file dialog, and the file filter isn't null, set the filter.
	if(!useFolderDialog && fileFilter != nullptr) {
		openDialog->SetFileTypes(fileFilterLength, fileFilter);
	}
	//Show the open dialog.
	result = openDialog->Show(nullptr);
	if (FAILED(result)) {

		if (result != ERROR_CANCELLED) {

			//Something other than the user cancelling choosing a folder has occurred.
			cout << "Unable to show the open file dialog!" << endl;
		}
		return result;
	}
	//Get the selected folder.
	result = openDialog->GetResult(&selectedItem);
	if (FAILED(result)) {

		cout << "Unable to get selected file!" << endl;
		return result;
	}
	//Pull out the full path of the selected item. Use the full system path for a folder, and the
	//name for a file.
	if (useFolderDialog)
		result = selectedItem->GetDisplayName(SIGDN_FILESYSPATH, static_cast<PWSTR*>(&filePathPtr));
	else
		result = selectedItem->GetDisplayName(SIGDN_NORMALDISPLAY, static_cast<PWSTR*>(&filePathPtr));
	if (FAILED(result)) {

		cout << "Unable to get the display name of the selcted file!" << endl;
		return result;
	}
	//Store the string in the selected folder path.
	selectedItemPath = wstring(filePathPtr.m_pData);

	//Finished here. All dynamically allocated data is destroyed here by smart pointers.
	return S_OK;
}

bool CreateSettingsFile() {

	HRESULT result;
	/*cout << "Info file not found! Create one? Y/N: ";
	char response;
	cin >> response;

	if (response == 'N' || response == 'n' || response == 'No' || response == 'no') {

	cout << "\n\nYou must create an info file!";
	system("PAUSE");
	return 0;
	}*/

	//If the user has to create an info file, I don't see why you would ask if they want
	//to make one.
	cout << "Info file for this game not found! You will need to create one now." << endl;
	pause;

	wstring localOut;
	wstring cloudOut;
	wstring exeOut;
	wstring paramOut;
	wofstream output(L"backup_info.txt");

	//Check that we can write to this file. Creation of the settings file may fail.
	if (!output.good())
	{
		cerr << "Failed to create the settings file for this game. Is the game's root directory "
			<< "accessible only to administrators?" << endl;
		return false;
	}

	/*cout << "\n\nEnter local save folder path (use '\\' format):\n";
	getline(cin, localOut);
	output << localOut + "\n";*/
	cout << "Pick a local save folder to use." << endl;
	result = ShowFileSelectDialog(true, L"Local Game Save Folder", 0, nullptr, localOut);
	if (FAILED(result)) {

		//Did creation of the dialog fail, or did the user cancel?
		if (result == ERROR_CANCELLED) {

			//User cancelled.
			cout << "You need to choose a local save folder to use in order to use this program." << endl;
			pause;
			return false;
		}
		//Dialog creation failed.
		cerr << "An error occurred while trying to create the folder dialog." << endl;
		pause;
		return false;
	}
	wcout << L"\"" << localOut << L"\" selected as local save folder." << endl;

	/*cout << "\nEnter Google Drive save folder path (use '\\' format):\n";
	getline(cin, cloudOut);
	output << cloudOut + "\n";*/
	cout << "Choose the Google Drive save folder to use." << endl;
	result = ShowFileSelectDialog(true, L"Google Drive Save Folder", 0, nullptr, cloudOut);
	if (FAILED(result)) {

		//Did creation of the dialog fail, or did the user cancel?
		if (result == ERROR_CANCELLED) {

			//User cancelled.
			cout << "You need to choose a save folder in your Google Drive in order to use this program." << endl;
			pause;
			return false;
		}
		//Dialog creation failed.
		cerr << "An error occurred while trying to create the folder dialog." << endl;
		pause;
		return false;
	}
	wcout << L"\"" << cloudOut << L"\" selected as Google Drive save folder." << endl;

	/*wcout << "\nEnter game exe name:\n";
	getline(cin, exeOut);
	output << exeOut + "\n";*/
	cout << "Choose the game's primary executable to use." << endl;
	COMDLG_FILTERSPEC exeFilters[] = {

		{ L"Executable Files", L"*.exe" },
		{ L"All Files", L"*.*" }
	};
	result = ShowFileSelectDialog(false, L"Game Executable Name", ARRAYSIZE(exeFilters), exeFilters, exeOut);
	if (FAILED(result)) {

		//Did creation of the dialog fail, or did the user cancel?
		if (result == ERROR_CANCELLED) {

			//User cancelled.
			cout << "You need to select the game executable in order to use this program." << endl;
			pause;
			return false;
		}
		//Dialog creation failed.
		cerr << "An error occurred while trying to create the file dialog." << endl;
		pause;
		return false;
	}
	wcout << L"\"" << exeOut << L"\" selected as game exe." << endl;

	cout << "\n(OPTIONAL) Enter launch parameters for the game.\n";
	getline(wcin, paramOut);

	//In case the user cancels at any time, it is probably best to write all output once the user has finished
	//putting in all the settings, instead of immediately write it as the user selects each folder or the
	//executable file.
	output << localOut << endl << cloudOut << endl << exeOut << endl << paramOut;
	//I like flushing output streams before closing them. Flushing forces all buffered output to be
	//written to the file.
	output.flush();
	output.close();

	cout << "\nInfo file created successfully!\n";
	
	return true;
}

int main(int argc, char* argv[]) {

	DebugBreak();
	//Initialize COM support in the application. Use COMInitializer to ensure RAII is followed. Doing
	//this, it doesn't matter where in code the application ends, there will be a call to CoUnintialize.
	HRESULT result;
	COMInitializer comInit;
	result = comInit.InitializeCOM();
	if (FAILED(result)) {

		cout << "Failed to initialize COM for this application!" << endl;
		return -1;
	}

	//check for internet connection. if no internet display error and ask if still want to launch game 
	if (InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0)) {
		cout << "Connected to the Internet!\n";
	}
	else {
		cout << "WARNING: No Internet connection found. \n | Save files cannot be fetched but will still attempt to update on exit. Play anyway? Y/N\n";

		/*char response;
		cin >> response;

		if (response == 'N' || response == 'n' || response == 'No' || response == 'no')
			return 0;*/

			//If you want the user to be able to say no, response should be a string, as "no" is a string, not
			//a character. Additionally, it's a good idea to take whatever the response is, then make it lowercase.
			//This way, someone can type "no", "NO", "No", or "nO", and it still means the same thing.
		string response;
		cin >> response;
		std::transform(response.begin(), response.end(), response.begin(), ::tolower);
		if (response == "n" || response == "no")
			return 0;
	}


	////check if Google Drive directory exists
	//char googlePath[MAX_PATH];
	//char path[MAX_PATH];
	//SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, googlePath);
	//strcpy_s(path, googlePath);
	//strcat_s(googlePath, "\\Google Drive");
	//strcat_s(path, "\\Google Drive\\Game Saves");

	//Avoid using C style strings in C++ whenever you can. C++ offers the string and wstring classes to avoid the hassle
	//of C-Style strings. Windows offers the CString class, which, for Win32 functions, may be safer to use
	//than the STL string and wstring classes.
	CComHeapPtr<WCHAR> userProfileRoot;
	CString googleDrivePath, saveGamePath;
	//SHGetFolderPath is deprecated. Use SHGetKnownFolderPath instead. The strings produced by many of the Shell
	//functions are COM Memory allocated, and must be destroyed using CoTaskMemFree. CComHeapPtr is a smart pointer
	//that will automatically destroy the the COM allocated memory.
	result = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, static_cast<PWSTR*>(&userProfileRoot));
	//It's tedious, but you usually want to check and handle the case where an API function fails.
	if (FAILED(result)) {

		cerr << "Failed to locate the user's root profile folder path!" << endl;
		return 0;
	}

	//Check if the google drive folder exists.
	googleDrivePath = CString(userProfileRoot) + _T("\\Google Drive");
	if (!boost::filesystem::exists(googleDrivePath.GetString())) {
		
		cerr << "\nERROR: Could not locate Google Drive directory. Make sure it is installed and signed in, then try again.\n";
		pause;
		return 0;
	}
	cout << "\nGoogle Drive directory found! '" << googleDrivePath.GetString() << "'\n";

	//Check if the save game folder exists. If it does not exist, automatically created it.
	saveGamePath = googleDrivePath + "\\Game Saves";
	if (!boost::filesystem::exists(saveGamePath.GetString())) {
		
		cout << "Save game folder did not exist. Creating it automatically." << endl;
		boost::filesystem::create_directory(saveGamePath.GetString());
	}

	wstring localSavePath;
	wstring cloudSavePath;
	wstring exeName;
	wstring params;
	wstring settings;

	wifstream in_stream;

	settings = ExePath() + L"\\backup_info.txt";

	if (!boost::filesystem::exists(settings)) {

		if(!CreateSettingsFile())
		{
			cerr << "Game settings file not created." << endl;
			return 0;
		}
	}

	in_stream.open(settings);
	if (!in_stream.good()) {

		wcerr << "Cannot read " << settings << endl;
		pause;
		return 0;
	}

	cout << "Reading backup_settings.txt...\n";
	getline(in_stream, localSavePath);
	getline(in_stream, cloudSavePath);
	getline(in_stream, exeName);
	getline(in_stream, params);
	in_stream.close();

	//fetch files from Google Drive
	if (boost::filesystem::exists(cloudSavePath)) {

		cout << "\nLoading save files from Google Drive...\n";
		if (CopyDir(boost::filesystem::path(cloudSavePath), boost::filesystem::path(localSavePath))) {
			
			cout << "\nCopy complete!\n";
		}
		else {
			
			cerr << "ERROR: Copy failed! Something went wrong! Try again?\n";
			pause;
			return 0;
		}
	}

	wstring exePath = ExePath() + L"\\" + exeName;
	time_t writeTime1 = boost::filesystem::last_write_time(cloudSavePath);

	//launch game and wait for it to close
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = nullptr;
	//Do all games need to be run as an administrator? I think this should be an option on a
	//per-game basis.
	ShExecInfo.lpVerb = _T("runas");
	ShExecInfo.lpFile = exePath.c_str();
	ShExecInfo.lpParameters = params.c_str();
	ShExecInfo.lpDirectory = nullptr;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = nullptr;

	if (ShellExecuteEx(&ShExecInfo)) {

		cout << "\nStarting Game...\n";
		WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
		CloseHandle(ShExecInfo.hProcess);
		cout << "\nGame closing...\n";
	}
	else {

		wcerr << "\nERROR: Could not find game executable at: '" << exePath << "'\nCheck exe path in settings file and the launcher is in the game folder!\n\n";
		pause;
		return 0;
	}

	//deal with multiple users playing and saving at the same time
	time_t writeTime2 = boost::filesystem::last_write_time(cloudSavePath);
	if (writeTime2 != writeTime1) {

		cout << "\nERROR: Cloud save location has been modified since playing!!\n SAVES WILL NOT BE BACKED UP\n"
			<< "WARNING: Running this again will erase your data by fetching the current cloud save.\n";
		pause;
		return 0;
	}

	//check to make sure there is enough space to transfer files. 100 mb at least
	boost::filesystem::space_info s = boost::filesystem::space(saveGamePath.GetString());
	cout << "\nBytes\n"
		<< "Total: " << s.capacity << '\n'
		<< "Free: " << s.free << '\n'
		<< "Available: " << s.available << '\n';
	
	if (s.available < 104857600) {
		cout << "\nERROR: less than 100mb available on Google Drive!\n";
		pause;
		return 0;
	}


	//push files to Google Drive
	cout << "\nUploading save files to Google Drive...\n";
	if (CopyDir(boost::filesystem::path(localSavePath), boost::filesystem::path(cloudSavePath))) {

		cout << "\nYour save(s) have been copied! It is now safe to exit.\n\nMAKE SURE GOOGLE DRIVE SYNCS\n";
	}
	else {

		cerr << "\nERROR: Copy failed! Something went wrong! Try again?\n";
		pause;
		return 0;
	}

	pause;
	return 0;
}