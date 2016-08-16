// GameSave Auto-Backup.cpp : Copies save files to and from a Google Drive account to keep saves up to date.

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "stdafx.h"
#include "COMInitializer.h"

#define pause system("pause")

using namespace std;
stringstream ss;

wstring ExePath() {

	wchar_t buffer[MAX_PATH];
	GetModuleFileName(nullptr, buffer, MAX_PATH);
	wstring::size_type pos = wstring(buffer).find_last_of(L"\\/");
	return wstring(buffer).substr(0, pos);
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
				cout << " | Copying file...\n";
			}
		}
		catch (boost::filesystem::filesystem_error const & e){
			cerr << e.what() << '\n\n';
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
	if (!useFolderDialog && fileFilter != nullptr) {
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
	
	result = selectedItem->GetDisplayName(SIGDN_FILESYSPATH, static_cast<PWSTR*>(&filePathPtr));
	
	if (FAILED(result)) {

		cout << "Unable to get the display name of the selcted file!" << endl;
		return result;
	}
	//Store the string in the selected folder path.
	selectedItemPath = wstring(filePathPtr.m_pData);

	//Finished here. All dynamically allocated data is destroyed here by smart pointers.
	return S_OK;
}

bool CreateSettingsFile(wstring path) {

	HRESULT result;

	//If the user has to create an info file, I don't see why you would ask if they want
	//to make one.
	cout << "Info file for this game not found! You will need to create one now.\n" << endl;
	pause;

	wstring localOut;
	wstring exeOut;
	wstring paramOut;
	wofstream output(path);

	//Check that we can write to this file. Creation of the settings file may fail.
	if (!output.good())
	{
		cerr << "Failed to create the settings file for this game. Is the game's root directory "
			<< "accessible only to administrators?" << endl;
		return false;
	}

	cout << "\nPick a local save folder to use." << endl;
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

	cout << "\nChoose the game's primary executable to use." << endl;
	COMDLG_FILTERSPEC exeFilters[] = {

		{ L"Executable Files", L"*.exe" },
		{ L"All Files", L"*.*" }
	};
	result = ShowFileSelectDialog(false, L"Game Executable", ARRAYSIZE(exeFilters), exeFilters, exeOut);
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

	cout << "\n(OPTIONAL) Enter launch parameters for the game." << endl;
	cin.get();
	getline(wcin, paramOut);

	//In case the user cancels at any time, it is probably best to write all output once the user has finished
	//putting in all the settings, instead of immediately write it as the user selects each folder or the
	//executable file.
	output << localOut << endl << exeOut << endl << paramOut;
	//I like flushing output streams before closing them. Flushing forces all buffered output to be
	//written to the file.
	output.flush();
	output.close();

	cout << "\nInfo file created successfully!\n";

	return true;
}

int main(int argc, char* argv[]){

	HRESULT result;
	COMInitializer comInit;
	result = comInit.InitializeCOM();
	if (FAILED(result)) {

		cerr << "Failed to initialize COM for this application!" << endl;
		return -1;
	}

	//check for internet connection. if no internet display error and ask if still want to launch game 
	if (InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0))
		cout << "Connected to the Internet!\n";

	else {
		cout << "WARNING: No Internet connection found. \n | Save files cannot be fetched but will still attempt to update on exit. Play anyway? Y/N\n";
	
		string response;
		cin >> response;

		std::transform(response.begin(), response.end(), response.begin(), ::tolower);
		if (response == "n" || response == "no")
			return 0;
	}

	//check if Google Drive directory exists
	CComHeapPtr<WCHAR> userProfileRoot;
	CString googleDrivePath;

	result = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, static_cast<PWSTR*>(&userProfileRoot));
	//It's tedious, but you usually want to check and handle the case where an API function fails.
	if (FAILED(result)) {

		cerr << "Failed to locate the user's root profile folder path!" << endl;
		return 0;
	}

	wstring path;
	googleDrivePath = CString(userProfileRoot) + _T("\\Google Drive");
	path = googleDrivePath + L"\\Game Saves";
	
	if (!boost::filesystem::exists(googleDrivePath.GetString())) {

		cerr << "\nERROR: Could not locate Google Drive directory. Make sure it is installed and signed in, then try again.\n";
		pause;
		return 0;
	}
	cout << "\nGoogle Drive directory found! '" << googleDrivePath.GetString() << "'\n";

	if (!boost::filesystem::exists(path)) {

		cout << "\nGame Save folder not found! Creating directory...\n";
		boost::filesystem::create_directory(path);
	}

	//list foldres in Google Drive 'Game Saves' directory
	//if file not found, make one and ask for directories. write and save the file
	boost::filesystem::directory_iterator end_itr;
	int k = 0;
	vector<wstring> games;
	vector<wstring> cloudSavePath;
	wstring name;
	wstring temp;

	cout << "\nSave folders found:\n";
	for (boost::filesystem::directory_iterator itr(path); itr != end_itr; ++itr){

			if (itr->path().string().length() > 2) {

				const size_t last_slash_idx = itr->path().wstring().rfind('\\');
				if (wstring::npos != last_slash_idx)
				{
					name = itr->path().wstring().substr(last_slash_idx+1);
				}

				games.push_back(name);
				cloudSavePath.push_back(itr->path().wstring());

				wcout << k + 1 << ". " << games[k] << "\n";
				k++;
		}
	}

	int choice = 0;
	wstring settings;

	if (k >= 2) {
		cout << "\nEnter the corresponding number of the game you would like to load: ";
		cin >> choice;
		choice = choice - 1;

		settings = cloudSavePath[choice] + L"\\backup_info.txt";
	}
	//if there is only one option, use as default
	else {
		settings = cloudSavePath[choice] + L"\\backup_info.txt";
	}
	wcout << "\n CLOUD PATH: " << cloudSavePath[choice] << "\n" << endl;

	wstring localSavePath;
	wstring exePath;
	wstring params;

	wifstream in_stream;

	if (!boost::filesystem::exists(settings)) {

		if (!CreateSettingsFile(settings))
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
	getline(in_stream, exePath);
	getline(in_stream, params);
	in_stream.close();

	//get working directory
	wstring wDirectory;
	const size_t last_slash_idx = exePath.rfind('\\');
		if (std::wstring::npos != last_slash_idx)
		{
			wDirectory = exePath.substr(0, last_slash_idx);
		}

	//fetch files from Google Drive
	if (boost::filesystem::exists(cloudSavePath[choice])) {
		cout << "\nLoading save files from Google Drive...\n";
		if (CopyDir(boost::filesystem::path(cloudSavePath[choice]), boost::filesystem::path(localSavePath))) {
			cout << "\nCopy complete!\n";
		}
		else {
			cerr << "ERROR: Copy failed! Something went wrong! Try again?\n";
			pause;
			return 0;
		}
	}

	time_t writeTime1 = boost::filesystem::last_write_time(cloudSavePath[choice]);

	//launch game and wait for it to close
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = _T("runas");
	ShExecInfo.lpFile = exePath.c_str();
	ShExecInfo.lpParameters = params.c_str();
	ShExecInfo.lpDirectory = wDirectory.c_str();
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;

	if (ShellExecuteEx(&ShExecInfo)) {
		wcout << "\nStarting " << games[choice] << "..." << endl;
		WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
		CloseHandle(ShExecInfo.hProcess);
		cout << "\nGame closing...\n";
	}
	else {
		wcerr << "\nERROR: Could not find game executable at: '" << exePath << "'\nCheck exe path in settings file and the launcher is in the game folder!\n" << endl;
		pause;
		return 0;
	}

	//deal with multiple users playing and saving at the same time
	time_t writeTime2 = boost::filesystem::last_write_time(cloudSavePath[choice]);

	if (writeTime2 != writeTime1) {

		cerr << "\nERROR: Cloud save location has been modified since playing!!\n SAVES WILL NOT BE BACKED UP\n"
			 << "WARNING: Running this again will erase your data by fetching the current cloud save.\n";
		pause;
		return 0;
	}

	//check to make sure there is enough space to transfer files. 100 mb at least
	boost::filesystem::space_info s = boost::filesystem::space(path);
	cout << "\nBytes\n"
	     << "Total: " << s.capacity << '\n'
	     << "Free: " << s.free << '\n'
	     << "Available: " << s.available << '\n';

	if (s.available < 104857600) {
		cerr << "\nERROR: less than 100mb available on Google Drive!\n";
		pause;
		return 0;
	}

	//push files to Google Drive
	cout << "\nUploading save files to Google Drive...\n";
	if (CopyDir(boost::filesystem::path(localSavePath), boost::filesystem::path(cloudSavePath[choice]))) {
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