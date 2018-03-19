// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1  //解决warning C4996 与 Security Enhancements in the CRT问题,例如用fscanf_s替换fscanf的警告信息
#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO: reference additional headers your program requires here
