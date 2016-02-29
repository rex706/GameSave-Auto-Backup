# GameSave-Auto-Backup
Automatically backup game saves to a Google Drive folder to be accessed from anywhere.

*Now works with all games and loads paths from an input file.*

Make sure to be logged into your Google Drive account

Files will be saved to "\Google Drive\Game Saves\\[GameName]". Directory will be created on first use if not found. 

Place the program exe next to your game exe and follow instructions to create info text file.

Make sure to use double slash '\\\' format between folders instead of the standard single slash or it will not work.

Also make sure not to include a slash at the end of the path.

info txt example:

C:\\\Users\\\rex706\\\Documents\\\Electronic Arts\\\The Sims 4\\\saves

The Sims 4

TS4.exe




Currenlty utilizing the boost c++ libraries.
- boost http://www.boost.org/ 



TODO:
- Find a way to deal with multiple users playing and saving at the same time.
- Check to make sure there is enough space to transfer files. (May already do this by default?)
- Find a way to check if files need to be updated or not