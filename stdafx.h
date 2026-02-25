#pragma once

#include "targetver.h"

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC OLE automation (AfxEnableControlContainer)
#include <afxcmn.h>         // MFC support for Windows Common Controls
#include <afxdialogex.h>    // MFC Dialog extensions
#include <afxmt.h>          // MFC Multithreading (CCriticalSection, CSingleLock, etc.)

// GDI+
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// STL
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <functional>

// Application-wide constants
#define MAX_IMAGE_SIZE 5120
