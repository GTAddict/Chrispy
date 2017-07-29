// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//



#include "targetver.h"
#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// Custom headers required by program
#include <WinSock2.h>				// Network connectivity
#include <MSWSock.h>				// Obviously, MS has to have it's own header
#include <strsafe.h>				// String
#include <time.h>					// Time
#include<Shlwapi.h>					// Path functions
#include "definitions.h"			// Our constants
