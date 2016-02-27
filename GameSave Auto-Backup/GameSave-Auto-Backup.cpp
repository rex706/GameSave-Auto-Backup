//File renamed to GameSave-Auto-Backup.cpp. Source files and files used with source code should not have
//spaces in their names. This breaks some systems, and is generally not good practice.

// GameSave-Auto-Backup.cpp : Copies save files to and from a Google Drive account to keep saves up to date.

//You are compiling for windows only, so _CRT_SECURE_NO_WARNINGS is not necessary. When using the C API
//functions, if the compiler asks you to use a "_s" (safe) version of the function, use that instead.
//#ifdef _MSC_VER
//#define _CRT_SECURE_NO_WARNINGS
//#endif

//Includes moved to stdafx.h
#include "stdafx.h"
#include "COMInitializer.h"

using namespace std;
stringstream ss;

string ExePath() {

	char buffer[MAX_PATH];
	GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	string::size_type pos = string(buffer).find_last_of("\\/");
	return string(buffer).substr(0, pos);
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
		cerr << e.what() << '\n';
		return false;
	}

	// Iterate through the source directory
	for (boost::filesystem::directory_iterator file(source); file != boost::filesystem::directory_iterator(); ++file) {

		try
		{
			boost::filesystem::path current(file->path());
			if (boost::filesystem::is_directory(current)) {

				// Found directory: Recursion
				if (!CopyDir(current, destination / current.filename())) {
					return false;
				}
			}
			else {
				// Found file: Copy
				boost::filesystem::copy_file(current, destination / current.filename(), boost::filesystem::copy_option::overwrite_if_exists);
				cout << " | Copying files...\n";
			}
		}
		catch (boost::filesystem::filesystem_error const & e) {
			cerr << e.what() << '\n\n';
		}
	}

	return true;
}

HRESULT ShowFileSelectDialog(bool useFolderDialog, const wstring& dialogTitle, 
 	UINT fileFilterLength, COMDLG_FILTERSPEC* fileFilter, wstring& selectedItemPath)
{
	//Create a file open dialog object.
	HRESULT result;
	CComPtr<IFileOpenDialog> openDialog;
	CComPtr<IShellItem> selectedItem;
	CComHeapPtr<WCHAR> filePathPtr;
	result = CoCreateInstance(IID_IFileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&openDialog));
	if (FAILED(result))
	{
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
	if(!useFolderDialog && fileFilter != nullptr)
	{
		openDialog->SetFileTypes(fileFilterLength, fileFilter);
	}
	//Show the open dialog.
	result = openDialog->Show(nullptr);
	if (FAILED(result))
	{
		if (result != ERROR_CANCELLED)
		{
			//Something other than the user cancelling choosing a folder has occurred.
			cout << "Unable to show the open file dialog!" << endl;
		}
		return result;
	}
	//Get the selected folder.
	result = openDialog->GetResult(&selectedItem);
	if (FAILED(result))
	{
		cout << "Unable to get selected file!" << endl;
		return result;
	}
	//Pull out the full path of the selected item. Use the full system path for a folder, and the
	//name for a file.
	if (useFolderDialog)
		result = selectedItem->GetDisplayName(SIGDN_FILESYSPATH, static_cast<PWSTR*>(&filePathPtr));
	else
		result = selectedItem->GetDisplayName(SIGDN_NORMALDISPLAY, static_cast<PWSTR*>(&filePathPtr));
	if (FAILED(result))
	{
		cout << "Unable to get the display name of the selcted file!" << endl;
		return result;
	}
	//Store the string in the selected folder path.
	selectedItemPath = wstring(filePathPtr.m_pData);

	//Finished here. All dynamically allocated data is destroyed here by smart pointers.
	return S_OK;
}

int main(int argc, char* argv[]) {

	//Initialize COM support in the application. Use COMInitializer to ensure RAII is followed. Doing
	//this, it doesn't matter where in code the application ends, there will be a call to CoUnintialize.
	HRESULT result;
	COMInitializer comInit;
	result = comInit.InitializeCOM();
	if (FAILED(result))
	{
		cout << "Failed to initialize COM for this application!" << endl;
		return -1;
	}

	//check for internet connection. if no internet display error and ask if still want to launch game 
	if (InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0))
		cout << "Connected to the Internet!\n";

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
	if (FAILED(result))
	{
		cout << "Failed to locate the user's root profile folder path!" << endl;
		return -1;
	}

	//Check if the google drive folder exists.
	googleDrivePath = CString(userProfileRoot) + _T("\\Google Drive");
	if (!boost::filesystem::exists(googleDrivePath.GetString())) {
		cout << "\nERROR: Could not locate Google Drive directory. Make sure it is installed and signed in, then try again.\n";
		system("PAUSE");
		return 0;
	}
	cout << "\nGoogle Drive directory found! '" << googleDrivePath << "'\n";

	//Check if the save game folder exists. If it does not exist, automatically created it.
	saveGamePath = googleDrivePath + "\\Game Saves";
	if (!boost::filesystem::exists(saveGamePath.GetString())) {
		cout << "Save game folder did not exist. Creating it automatically.";
		boost::filesystem::create_directory(saveGamePath.GetString());
	}

	string localSavePath;
	string cloudSavePath;
	string exeName;
	string params;
	string settings;

	ifstream in_stream;

	settings = ExePath() + "\\backup_info.txt";

	if (!boost::filesystem::exists(settings)) {

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
		cout << "Info file not found! You will need to create one now." << endl;

		wstring localOut;
		wstring cloudOut;
		wstring exeOut;
		string paramOut;

		cin.sync();
		cin.get();

		ofstream output("backup_info.txt");

		/*cout << "\n\nEnter local save folder path (use '\\' format):\n";
		getline(cin, localOut);
		output << localOut + "\n";*/
		cout << "Pick a local save folder to use." << endl;
		result = ShowFileSelectDialog(true, L"Local Game Save Folder", 0, nullptr, localOut);
		if (FAILED(result))
		{
			//Did creation of the dialog fail, or did the user cancel?
			if (result == ERROR_CANCELLED)
			{
				//User cancelled.
			}
			else
			{
				//Dialog creation failed.
			}
		}
		wcout << L"\"" << localOut << L"\" selected as local save folder." << endl;

		/*cout << "\nEnter Google Drive save folder path (use '\\' format):\n";
		getline(cin, cloudOut);
		output << cloudOut + "\n";*/
		cout << "Choose the Google Drive save folder to use." << endl;
		result = ShowFileSelectDialog(true, L"Google Drive Save Folder", 0, nullptr, cloudOut);
		if (FAILED(result))
		{
			//Did creation of the dialog fail, or did the user cancel?
			if (result == ERROR_CANCELLED)
			{
				//User cancelled.
			}
			else
			{
				//Dialog creation failed.
			}
		}
		wcout << L"\"" << cloudOut << L"\" selected as Google Drive save folder." << endl;

		/*wcout << "\nEnter game exe name:\n";
		getline(cin, exeOut);
		output << exeOut + "\n";*/
		cout << "Choose the game's primary executable to use." << endl;
		COMDLG_FILTERSPEC exeFilters[] = 
		{
			{L"Executable Files", L"*.exe"},
			{L"All Files", L"*.*"}
		};
		result = ShowFileSelectDialog(false, L"Game Executable Name", ARRAYSIZE(exeFilters), exeFilters, exeOut);
		if (FAILED(result))
		{
			//Did creation of the dialog fail, or did the user cancel?
			if (result == ERROR_CANCELLED)
			{
				//User cancelled.
			}
			else
			{
				//Dialog creation failed.
			}
		}
		wcout << L"\"" << exeOut << L"\" selected as game exe." << endl;

		cout << "\n(OPTIONAL) Enter launch parameters for the game:\n";
		getline(cin, paramOut);
		output << paramOut;
		//I like flushing output streams before closing them. Flushing forces all buffered output to be
		//written to the file.
		output.flush();
		output.close();

		cout << "\nInfo file created successfully!\n";
	}

	in_stream.open(settings);
	if (!in_stream.good()) {
		cout << "Cannot read " << settings << endl;
		system("PAUSE");
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
			cout << "ERROR: Copy failed! Something went wrong! Try again?\n";
			system("PAUSE");
			return 0;
		}
	}
	string exePath = ExePath() + "\\" + exeName;

	std::wstring wExePath(exePath.begin(), exePath.end());
	std::wstring wParams(params.begin(), params.end());

	//launch game and wait for it to close
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = nullptr;
	ShExecInfo.lpVerb = _T("runas");
	ShExecInfo.lpFile = wExePath.c_str();
	ShExecInfo.lpParameters = wExePath.c_str();
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
		cout << "\nERROR: Could not find game executable at: '" << exePath << "'\nCheck exe path in settings file and the launcher is in the game folder!\n\n";
		system("PAUSE");
		return 0;
	}

	//TODO: Find a way to deal with multiple users playing and saving at the same time

	//check to make sure there is enough space to transfer files. 100 mb at least
	boost::filesystem::space_info s = boost::filesystem::space(saveGamePath.GetString());
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