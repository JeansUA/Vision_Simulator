#include "stdafx.h"
#include "VisionSimulatorApp.h"
#include "VisionSimulatorDlg.h"
#include "Algorithm/AlgorithmManager.h"

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CVisionSimulatorApp

BEGIN_MESSAGE_MAP(CVisionSimulatorApp, CWinApp)
END_MESSAGE_MAP()

// CVisionSimulatorApp construction

CVisionSimulatorApp::CVisionSimulatorApp()
    : m_gdiplusToken(0)
{
}

// The one and only CVisionSimulatorApp object
CVisionSimulatorApp theApp;

// CVisionSimulatorApp initialization

BOOL CVisionSimulatorApp::InitInstance()
{
    AfxEnableControlContainer();

    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

    // Initialize common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    CWinApp::InitInstance();

    // Register all algorithms
    CAlgorithmManager::GetInstance().RegisterAlgorithms();

    // Create and show the main dialog
    CVisionSimulatorDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    // Dialog-based app exits after modal dialog closes
    return FALSE;
}

int CVisionSimulatorApp::ExitInstance()
{
    Gdiplus::GdiplusShutdown(m_gdiplusToken);
    return CWinApp::ExitInstance();
}
