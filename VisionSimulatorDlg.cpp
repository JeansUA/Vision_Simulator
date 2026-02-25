#include "stdafx.h"
#include "VisionSimulatorApp.h"
#include "VisionSimulatorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CVisionSimulatorDlg, CDialogEx)

BEGIN_MESSAGE_MAP(CVisionSimulatorDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_MESSAGE(WM_PREVIEW_RESULT, &CVisionSimulatorDlg::OnPreviewResult)
    ON_BN_CLICKED(IDC_BTN_LOAD,       &CVisionSimulatorDlg::OnBnClickedLoad)
    ON_BN_CLICKED(IDC_BTN_RUN,        &CVisionSimulatorDlg::OnBnClickedRun)
    ON_BN_CLICKED(IDC_BTN_STOP,       &CVisionSimulatorDlg::OnBnClickedStop)
    ON_BN_CLICKED(IDC_BTN_CLEAR,      &CVisionSimulatorDlg::OnBnClickedClear)
    ON_BN_CLICKED(IDC_BTN_SAVE,       &CVisionSimulatorDlg::OnBnClickedSave)
    ON_BN_CLICKED(IDC_BTN_FIT,        &CVisionSimulatorDlg::OnBnClickedFit)
    ON_BN_CLICKED(IDC_BTN_ADD_SEQ,    &CVisionSimulatorDlg::OnBnClickedAddSeq)
    ON_BN_CLICKED(IDC_BTN_REMOVE_SEQ, &CVisionSimulatorDlg::OnBnClickedRemoveSeq)
    ON_BN_CLICKED(IDC_BTN_SEQ_UP,     &CVisionSimulatorDlg::OnBnClickedSeqUp)
    ON_BN_CLICKED(IDC_BTN_SEQ_DOWN,   &CVisionSimulatorDlg::OnBnClickedSeqDown)
    ON_BN_CLICKED(IDC_BTN_CLEAR_SEQ,  &CVisionSimulatorDlg::OnBnClickedClearSeq)
    ON_BN_CLICKED(IDC_BTN_ROI,        &CVisionSimulatorDlg::OnBnClickedROI)
    ON_BN_CLICKED(IDC_BTN_CLEAR_ROI,  &CVisionSimulatorDlg::OnBnClickedClearROI)
    ON_BN_CLICKED(IDC_BTN_REMOVE_ROI, &CVisionSimulatorDlg::OnBnClickedRemoveROI)
    ON_BN_CLICKED(IDC_BTN_CLEAR_ROIS, &CVisionSimulatorDlg::OnBnClickedClearROIs)
    ON_CBN_SELCHANGE(IDC_COMBO_ALGORITHM, &CVisionSimulatorDlg::OnCbnSelchangeAlgorithm)
    ON_LBN_SELCHANGE(IDC_LIST_SEQUENCE,   &CVisionSimulatorDlg::OnLbnSelchangeSequence)
    ON_LBN_SELCHANGE(IDC_LIST_ROI,        &CVisionSimulatorDlg::OnLbnSelchangeROI)
    ON_MESSAGE(WM_SEQUENCE_PROGRESS,  &CVisionSimulatorDlg::OnSequenceProgress)
    ON_MESSAGE(WM_SEQUENCE_COMPLETE,  &CVisionSimulatorDlg::OnSequenceComplete)
    ON_MESSAGE(WM_SEQUENCE_ERROR,     &CVisionSimulatorDlg::OnSequenceError)
    ON_MESSAGE(WM_SEQUENCE_STEP_DONE, &CVisionSimulatorDlg::OnSequenceStepDone)
    ON_MESSAGE(WM_MINIVIEWER_CLICKED, &CVisionSimulatorDlg::OnMiniViewerClicked)
    ON_MESSAGE(WM_ROI_UPDATED,        &CVisionSimulatorDlg::OnROIUpdated)
    ON_MESSAGE(WM_PARAM_CHANGED,      &CVisionSimulatorDlg::OnParamChanged)
END_MESSAGE_MAP()

CVisionSimulatorDlg::CVisionSimulatorDlg(CWnd* pParent)
    : CDialogEx(IDD_VISIONSIMULATOR_DIALOG, pParent)
    , m_pCurrentAlgorithm(nullptr)
    , m_bInitialized(false)
    , m_nMinWidth(1100)
    , m_nMinHeight(768)
    , m_nSelectedMiniViewer(-1)
    , m_nEditingSequenceStep(-1)
    , m_bPreviewPending(false)
    , m_pPreviewThread(nullptr)
    , m_hPreviewReady(NULL)
    , m_hPreviewStop(NULL)
    , m_bPreviewCancel(false)
    , m_bNewPreviewPending(false)
    , m_pPreviewAlgCopy(nullptr)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CVisionSimulatorDlg::~CVisionSimulatorDlg()
{
    if (m_pCurrentAlgorithm) { delete m_pCurrentAlgorithm; m_pCurrentAlgorithm = nullptr; }
    CSingleLock lock(&m_csPreview, TRUE);
    if (m_pPreviewAlgCopy) { delete m_pPreviewAlgCopy; m_pPreviewAlgCopy = nullptr; }
}

void CVisionSimulatorDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BOOL CVisionSimulatorDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);
    SetWindowText(_T("Vision Simulator v2.0"));
    ModifyStyle(0, WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);

    // Fonts
    m_fontUI.CreateFont(-MulDiv(9, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));
    m_fontTitle.CreateFont(-MulDiv(10, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
        0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, _T("Segoe UI Semibold"));

    CreateControls();
    InitAlgorithmCombo();
    m_sequenceManager.SetNotifyWnd(GetSafeHwnd());

    SetWindowPos(NULL, 0, 0, 1280, 960, SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();

    m_bInitialized = true;
    LayoutControls();
    StartPreviewThread();
    SetStatus(_T("Ready - Load an image to begin | Right-click drag to add ROI"));
    return TRUE;
}

void CVisionSimulatorDlg::CreateControls()
{
    DWORD btnStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;

    // Toolbar
    m_btnLoad.Create(_T("Load Image"), btnStyle, CRect(0,0,10,10), this, IDC_BTN_LOAD);
    m_btnRun.Create(_T("Run"),         btnStyle, CRect(0,0,10,10), this, IDC_BTN_RUN);
    m_btnStop.Create(_T("Stop"),       btnStyle, CRect(0,0,10,10), this, IDC_BTN_STOP);
    m_btnClear.Create(_T("Clear"),     btnStyle, CRect(0,0,10,10), this, IDC_BTN_CLEAR);
    m_btnSave.Create(_T("Save"),       btnStyle, CRect(0,0,10,10), this, IDC_BTN_SAVE);
    m_btnROI.Create(_T("ROI Mode"),    btnStyle, CRect(0,0,10,10), this, IDC_BTN_ROI);
    m_btnClearROI.Create(_T("Clr ROI"),btnStyle, CRect(0,0,10,10), this, IDC_BTN_CLEAR_ROI);
    m_btnFit.Create(_T("Fit"),         btnStyle, CRect(0,0,10,10), this, IDC_BTN_FIT);

    // Main viewer
    m_mainViewer.Create(_T(""), WS_CHILD|WS_VISIBLE|SS_NOTIFY, CRect(0,0,10,10), this, IDC_MAIN_VIEWER);

    // Algorithm group
    m_grpAlgorithm.Create(_T("Algorithm"), WS_CHILD|WS_VISIBLE|BS_GROUPBOX, CRect(0,0,10,10), this, IDC_GRP_ALGORITHM);
    m_comboAlgorithm.Create(WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, CRect(0,0,10,10), this, IDC_COMBO_ALGORITHM);
    m_btnAddSeq.Create(_T("Add to Sequence"), btnStyle, CRect(0,0,10,10), this, IDC_BTN_ADD_SEQ);
    m_paramPanel.Create(WS_CHILD|WS_VISIBLE, CRect(0,0,10,10), this, IDC_GRP_PARAMS);

    // Sequence group
    m_grpSequence.Create(_T("Sequence"), WS_CHILD|WS_VISIBLE|BS_GROUPBOX, CRect(0,0,10,10), this, IDC_GRP_SEQUENCE);
    m_listSequence.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|LBS_NOTIFY|WS_VSCROLL|WS_HSCROLL, CRect(0,0,10,10), this, IDC_LIST_SEQUENCE);
    m_btnRemoveSeq.Create(_T("Remove"), btnStyle, CRect(0,0,10,10), this, IDC_BTN_REMOVE_SEQ);
    m_btnSeqUp.Create(_T("Up"),         btnStyle, CRect(0,0,10,10), this, IDC_BTN_SEQ_UP);
    m_btnSeqDown.Create(_T("Down"),     btnStyle, CRect(0,0,10,10), this, IDC_BTN_SEQ_DOWN);
    m_btnClearSeq.Create(_T("Clear"),   btnStyle, CRect(0,0,10,10), this, IDC_BTN_CLEAR_SEQ);

    // ROI group
    m_grpROI.Create(_T("ROI List"), WS_CHILD|WS_VISIBLE|BS_GROUPBOX, CRect(0,0,10,10), this, IDC_GRP_ROI);
    m_listROI.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|LBS_NOTIFY|WS_VSCROLL, CRect(0,0,10,10), this, IDC_LIST_ROI);
    m_btnRemoveROI.Create(_T("Remove"),   btnStyle, CRect(0,0,10,10), this, IDC_BTN_REMOVE_ROI);
    m_btnClearROIs.Create(_T("Clear All"),btnStyle, CRect(0,0,10,10), this, IDC_BTN_CLEAR_ROIS);

    // History
    m_grpHistory.Create(_T("Processing History"), WS_CHILD|WS_VISIBLE|BS_GROUPBOX, CRect(0,0,10,10), this, IDC_GRP_HISTORY);
    for (int i = 0; i < 8; i++)
    {
        m_miniViewers[i].Create(_T(""), WS_CHILD|WS_VISIBLE|SS_NOTIFY, CRect(0,0,10,10), this, IDC_MINI_VIEWER1+i);
        m_miniViewers[i].SetIndex(i);
        m_miniLabels[i].Create(_T(""), WS_CHILD|WS_VISIBLE|SS_CENTER, CRect(0,0,10,10), this, IDC_MINI_LABEL1+i);
    }

    // Log panel
    m_grpLog.Create(_T("처리 로그"), WS_CHILD|WS_VISIBLE|BS_GROUPBOX, CRect(0,0,10,10), this, IDC_GRP_LOG);
    m_editLog.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL|ES_LEFT,
        CRect(0,0,10,10), this, IDC_EDIT_LOG);

    // Status / Zoom
    m_staticStatus.Create(_T("Ready"), WS_CHILD|WS_VISIBLE|SS_LEFT|SS_SUNKEN, CRect(0,0,10,10), this, IDC_STATUS_TEXT);
    m_staticZoom.Create(_T("100%"),    WS_CHILD|WS_VISIBLE|SS_CENTER,          CRect(0,0,10,10), this, IDC_STATIC_ZOOM);

    // Apply fonts
    CWnd* pChild = GetWindow(GW_CHILD);
    while (pChild) { pChild->SetFont(&m_fontUI); pChild = pChild->GetNextWindow(); }
    m_grpAlgorithm.SetFont(&m_fontTitle);
    m_grpSequence.SetFont(&m_fontTitle);
    m_grpROI.SetFont(&m_fontTitle);
    m_grpHistory.SetFont(&m_fontTitle);
    m_grpLog.SetFont(&m_fontTitle);

    // Initial states
    m_btnStop.EnableWindow(FALSE);
    m_btnSave.EnableWindow(FALSE);
    m_btnRun.EnableWindow(FALSE);
}

void CVisionSimulatorDlg::LayoutControls()
{
    if (!m_bInitialized) return;

    CRect rcClient;
    GetClientRect(&rcClient);
    int W = rcClient.Width(), H = rcClient.Height();
    int margin    = 8;
    int toolbarH  = 36;
    int statusH   = 24;
    int historyH  = 160;
    int rightW    = 300;

    // Toolbar
    int tx = margin, ty = margin;
    int bH = 28;
    m_btnLoad.MoveWindow(tx, ty, 90, bH);     tx += 94;
    m_btnRun.MoveWindow(tx, ty, 55, bH);      tx += 59;
    m_btnStop.MoveWindow(tx, ty, 55, bH);     tx += 59;
    m_btnClear.MoveWindow(tx, ty, 60, bH);    tx += 64;
    m_btnSave.MoveWindow(tx, ty, 80, bH);     tx += 84;
    m_btnROI.MoveWindow(tx, ty, 75, bH);      tx += 79;
    m_btnClearROI.MoveWindow(tx, ty, 70, bH); tx += 74;
    m_btnFit.MoveWindow(W - rightW - margin, ty, 45, bH);
    m_staticZoom.MoveWindow(W - rightW - margin + 50, ty + 4, 65, bH - 4);

    // Content area
    int contentTop    = margin + toolbarH + margin;
    int contentBottom = H - statusH - margin - historyH - margin;
    int viewerW = W - rightW - margin * 3;
    int logH    = 110;
    int viewerH = (contentBottom - contentTop) - logH - margin;
    if (viewerH < 100) viewerH = 100;

    m_mainViewer.MoveWindow(margin, contentTop, viewerW, viewerH);

    // Log panel (below main viewer)
    int logTop = contentTop + viewerH + margin;
    m_grpLog.MoveWindow(margin, logTop, viewerW, logH);
    m_editLog.MoveWindow(margin + 8, logTop + 18, viewerW - 16, logH - 24);

    // Right panel
    int rx = W - rightW - margin;
    int ry = contentTop;
    int rpW = rightW;

    // Algorithm group (top)
    int algGroupH = 200;
    m_grpAlgorithm.MoveWindow(rx, ry, rpW, algGroupH);
    m_comboAlgorithm.MoveWindow(rx+10, ry+22, rpW-20, 200);
    m_btnAddSeq.MoveWindow(rx+10, ry+50, rpW-20, 26);
    m_paramPanel.MoveWindow(rx+10, ry+82, rpW-20, algGroupH-92);

    // Remaining height split between sequence and ROI
    int remaining = contentBottom - (ry + algGroupH + margin);
    int roiH      = 130;
    int seqH      = remaining - roiH - margin;
    if (seqH < 60) seqH = 60;

    // Sequence group
    int seqTop = ry + algGroupH + margin;
    m_grpSequence.MoveWindow(rx, seqTop, rpW, seqH);
    int seqBW = (rpW - 30) / 4;
    m_listSequence.MoveWindow(rx+10, seqTop+22, rpW-20, seqH-58);
    int sby = seqTop + seqH - 34;
    m_btnRemoveSeq.MoveWindow(rx+10,              sby, seqBW, 26);
    m_btnSeqUp.MoveWindow(rx+10+seqBW+2,          sby, seqBW, 26);
    m_btnSeqDown.MoveWindow(rx+10+(seqBW+2)*2,    sby, seqBW, 26);
    m_btnClearSeq.MoveWindow(rx+10+(seqBW+2)*3,   sby, seqBW, 26);

    // ROI group
    int roiTop = seqTop + seqH + margin;
    m_grpROI.MoveWindow(rx, roiTop, rpW, roiH);
    m_listROI.MoveWindow(rx+10, roiTop+22, rpW-20, roiH-60);
    int rby = roiTop + roiH - 34;
    m_btnRemoveROI.MoveWindow(rx+10,          rby, (rpW-24)/2, 26);
    m_btnClearROIs.MoveWindow(rx+14+(rpW-24)/2, rby, (rpW-24)/2, 26);

    // History
    int hy = H - statusH - margin - historyH;
    m_grpHistory.MoveWindow(margin, hy, W - margin*2, historyH);
    int mvMargin = 10;
    int mvTop    = hy + 20;
    int mvAvailW = W - margin*2 - mvMargin*2;
    int mvW      = (mvAvailW - 7*mvMargin) / 8;
    int mvH      = historyH - 50;
    int labelH   = 16;
    for (int i = 0; i < 8; i++)
    {
        int mx = margin + mvMargin + i*(mvW + mvMargin);
        m_miniViewers[i].MoveWindow(mx, mvTop, mvW, mvH - labelH - 2);
        m_miniLabels[i].MoveWindow(mx, mvTop + mvH - labelH, mvW, labelH);
    }

    // Status
    m_staticStatus.MoveWindow(margin, H - statusH - margin/2, W - margin*2, statusH);

    Invalidate();
}

void CVisionSimulatorDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON), cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect; GetClientRect(&rect);
        dc.DrawIcon((rect.Width()-cxIcon+1)/2, (rect.Height()-cyIcon+1)/2, m_hIcon);
    }
    else
        CDialogEx::OnPaint();
}

HCURSOR CVisionSimulatorDlg::OnQueryDragIcon() { return (HCURSOR)m_hIcon; }

void CVisionSimulatorDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    LayoutControls();
}

void CVisionSimulatorDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    lpMMI->ptMinTrackSize.x = m_nMinWidth;
    lpMMI->ptMinTrackSize.y = m_nMinHeight;
    CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CVisionSimulatorDlg::OnDestroy()
{
    KillTimer(TIMER_PREVIEW);
    m_sequenceManager.StopExecution();
    StopPreviewThread();
    CDialogEx::OnDestroy();
}

void CVisionSimulatorDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_PREVIEW)
    {
        KillTimer(TIMER_PREVIEW);
        m_bPreviewPending = false;
        QueuePreview();  // fire preview on background thread
    }
    CDialogEx::OnTimer(nIDEvent);
}

void CVisionSimulatorDlg::InitAlgorithmCombo()
{
    CAlgorithmManager& mgr = CAlgorithmManager::GetInstance();
    for (int i = 0; i < mgr.GetAlgorithmCount(); i++)
        m_comboAlgorithm.AddString(mgr.GetAlgorithmName(i));
    if (m_comboAlgorithm.GetCount() > 0)
    {
        m_comboAlgorithm.SetCurSel(0);
        OnCbnSelchangeAlgorithm();
    }
}

void CVisionSimulatorDlg::OnCbnSelchangeAlgorithm()
{
    int sel = m_comboAlgorithm.GetCurSel();
    if (sel == CB_ERR) return;

    if (m_pCurrentAlgorithm) { delete m_pCurrentAlgorithm; m_pCurrentAlgorithm = nullptr; }
    m_pCurrentAlgorithm = CAlgorithmManager::GetInstance().CreateAlgorithm(sel);

    if (m_pCurrentAlgorithm)
        m_paramPanel.SetAlgorithm(m_pCurrentAlgorithm);

    // Cancel sequence step editing
    m_nEditingSequenceStep = -1;
    m_listSequence.SetCurSel(-1);
}

void CVisionSimulatorDlg::OnLbnSelchangeSequence()
{
    int sel = m_listSequence.GetCurSel();
    if (sel == LB_ERR)
    {
        m_nEditingSequenceStep = -1;
        m_paramPanel.SetAlgorithm(m_pCurrentAlgorithm);
        return;
    }

    CAlgorithmBase* pStep = m_sequenceManager.GetStep(sel);
    if (pStep)
    {
        m_nEditingSequenceStep = sel;
        // Point panel directly to the sequence step (live editing)
        m_paramPanel.SetAlgorithm(pStep);
        // Update combo to reflect algorithm type
        CString name = pStep->GetName();
        for (int i = 0; i < m_comboAlgorithm.GetCount(); i++)
        {
            CString s; m_comboAlgorithm.GetLBText(i, s);
            if (s == name) { m_comboAlgorithm.SetCurSel(i); break; }
        }
        SetStatus(_T("Editing sequence step ") + pStep->GetName() + _T(" - adjust sliders for live preview"));
    }
}

void CVisionSimulatorDlg::OnLbnSelchangeROI()
{
    int sel = m_listROI.GetCurSel();
    if (sel != LB_ERR)
        m_mainViewer.SetSelectedROI(sel);
}

LRESULT CVisionSimulatorDlg::OnROIUpdated(WPARAM wParam, LPARAM lParam)
{
    UpdateROIList();
    // Sync sequence manager ROIs
    m_sequenceManager.SetROIs(m_mainViewer.GetROIs());
    return 0;
}

LRESULT CVisionSimulatorDlg::OnParamChanged(WPARAM wParam, LPARAM lParam)
{
    // Throttle preview: reset timer to 40ms
    if (!m_bPreviewPending)
    {
        m_bPreviewPending = true;
        SetTimer(TIMER_PREVIEW, 40, NULL);
    }
    else
    {
        KillTimer(TIMER_PREVIEW);
        SetTimer(TIMER_PREVIEW, 40, NULL);
    }
    // Also update the sequence list display if editing a step
    if (m_nEditingSequenceStep >= 0)
        UpdateSequenceList();
    return 0;
}

// ============================================================================
// Background Preview Thread
// ============================================================================

void CVisionSimulatorDlg::StartPreviewThread()
{
    m_hPreviewReady = ::CreateEvent(NULL, FALSE, FALSE, NULL);  // auto-reset
    m_hPreviewStop  = ::CreateEvent(NULL, TRUE,  FALSE, NULL);  // manual-reset

    m_pPreviewThread = AfxBeginThread(PreviewThreadProc, this,
        THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED, NULL);
    if (m_pPreviewThread)
    {
        m_pPreviewThread->m_bAutoDelete = FALSE;
        m_pPreviewThread->ResumeThread();
    }
}

void CVisionSimulatorDlg::StopPreviewThread()
{
    if (!m_pPreviewThread) return;

    m_bPreviewCancel = true;
    if (m_hPreviewStop) ::SetEvent(m_hPreviewStop);

    ::WaitForSingleObject(m_pPreviewThread->m_hThread, 3000);
    delete m_pPreviewThread;
    m_pPreviewThread = nullptr;

    if (m_hPreviewReady) { ::CloseHandle(m_hPreviewReady); m_hPreviewReady = NULL; }
    if (m_hPreviewStop)  { ::CloseHandle(m_hPreviewStop);  m_hPreviewStop  = NULL; }
}

void CVisionSimulatorDlg::QueuePreview()
{
    if (!m_originalImage.IsValid()) return;

    CAlgorithmBase* pAlg = m_paramPanel.GetAlgorithm();
    if (!pAlg) return;

    // Determine source image (no Clone — CopyDataFrom reuses m_previewInputBuf allocation)
    const CImageBuffer* pSrc = &m_originalImage;
    const auto& history = m_sequenceManager.GetHistory();
    if (m_nEditingSequenceStep > 0 && (int)history.size() > m_nEditingSequenceStep)
        pSrc = &history[m_nEditingSequenceStep];

    // Pack data for thread — CopyDataFrom avoids malloc after first call
    {
        CSingleLock lock(&m_csPreview, TRUE);
        m_previewInputBuf.CopyDataFrom(*pSrc);   // 0 malloc after 1st call (same size)
        if (m_pPreviewAlgCopy) { delete m_pPreviewAlgCopy; m_pPreviewAlgCopy = nullptr; }
        m_pPreviewAlgCopy = pAlg->Clone();
        m_previewROIs     = m_mainViewer.GetROIs();
        m_bNewPreviewPending = true;
    }

    m_bPreviewCancel = true;   // cancel any running preview
    if (m_hPreviewReady) ::SetEvent(m_hPreviewReady);
}

UINT CVisionSimulatorDlg::PreviewThreadProc(LPVOID pParam)
{
    CVisionSimulatorDlg* pDlg = reinterpret_cast<CVisionSimulatorDlg*>(pParam);
    HANDLE events[2] = { pDlg->m_hPreviewReady, pDlg->m_hPreviewStop };

    // Thread-local persistent buffers — survive between previews so OS pages stay warm.
    // After the first preview these are reused without any malloc or page fault.
    CImageBuffer              threadInput;
    CImageBuffer              threadOutput;
    std::vector<CImageBuffer> threadROIIn;
    std::vector<CImageBuffer> threadROIOut;

    while (true)
    {
        DWORD dw = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
        if (dw == WAIT_OBJECT_0 + 1) break;   // stop event
        if (dw != WAIT_OBJECT_0) continue;

restart:
        // Grab data — CopyDataFrom under lock (brief ~0.5ms hold for 6MB copy)
        CAlgorithmBase* pAlg = nullptr;
        std::vector<CRect> rois;
        {
            CSingleLock lock(&pDlg->m_csPreview, TRUE);
            if (!pDlg->m_bNewPreviewPending) continue;
            threadInput.CopyDataFrom(pDlg->m_previewInputBuf);  // 0 malloc after 1st call
            pAlg = pDlg->m_pPreviewAlgCopy;
            pDlg->m_pPreviewAlgCopy = nullptr;
            rois = pDlg->m_previewROIs;
            pDlg->m_bNewPreviewPending = false;
        }
        if (!threadInput.IsValid() || !pAlg) { delete pAlg; continue; }

        pDlg->m_bPreviewCancel = false;

        // Prepare ROI buffers (smart Create = 0 malloc if size unchanged)
        threadROIIn.resize(rois.size());
        threadROIOut.resize(rois.size());
        for (int j = 0; j < (int)rois.size(); j++)
        {
            threadROIIn[j].Create(rois[j].Width(), rois[j].Height(), threadInput.GetChannels());
            threadROIOut[j].Create(rois[j].Width(), rois[j].Height(), threadInput.GetChannels());
        }

        auto t0 = std::chrono::high_resolution_clock::now();
        bool success = false;

        if (rois.empty())
        {
            // Smart Create inside Process reuses threadOutput allocation
            success = pAlg->Process(threadInput, threadOutput);
        }
        else
        {
            success = threadOutput.CopyDataFrom(threadInput);  // composite base
            if (success)
            {
                for (int j = 0; j < (int)rois.size(); j++)
                {
                    if (pDlg->m_bPreviewCancel) { success = false; break; }
                    if (!threadInput.ExtractRegionInto(rois[j], threadROIIn[j])) continue;
                    if (pAlg->Process(threadROIIn[j], threadROIOut[j]) && threadROIOut[j].IsValid())
                        threadOutput.PasteRegion(threadROIOut[j], rois[j].left, rois[j].top);
                }
            }
        }

        delete pAlg;

        auto t1 = std::chrono::high_resolution_clock::now();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        if (success && !pDlg->m_bPreviewCancel && ::IsWindow(pDlg->GetSafeHwnd()))
        {
            // One malloc per result to hand off to UI thread (unavoidable — thread reuses threadOutput)
            CImageBuffer* pResult = new CImageBuffer();
            pResult->CopyDataFrom(threadOutput);
            ::PostMessage(pDlg->GetSafeHwnd(), WM_PREVIEW_RESULT,
                reinterpret_cast<WPARAM>(pResult), static_cast<LPARAM>(ms));
        }

        // If another preview was queued while we ran, restart immediately
        {
            CSingleLock lock(&pDlg->m_csPreview, TRUE);
            if (pDlg->m_bNewPreviewPending) goto restart;
        }
    }
    return 0;
}

LRESULT CVisionSimulatorDlg::OnPreviewResult(WPARAM wParam, LPARAM lParam)
{
    CImageBuffer* pOutput = reinterpret_cast<CImageBuffer*>(wParam);
    long long ms = static_cast<long long>(lParam);

    if (pOutput && pOutput->IsValid())
    {
        CAlgorithmBase* pAlg = m_paramPanel.GetAlgorithm();
        CString algName = pAlg ? pAlg->GetName() : _T("?");

        m_mainViewer.ClearOverlayInfo();
        m_mainViewer.SetImage(*pOutput);

        CString status;
        status.Format(_T("[미리보기] %s - %lldms"), (LPCTSTR)algName, ms);
        SetStatus(status);

        CString logMsg;
        logMsg.Format(_T("[미리보기] %s (%lldms)"), (LPCTSTR)algName, ms);
        AddLog(logMsg);
    }

    delete pOutput;
    return 0;
}

void CVisionSimulatorDlg::AddLog(const CString& text)
{
    if (!::IsWindow(m_editLog.GetSafeHwnd())) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    CString entry;
    entry.Format(_T("[%02d:%02d:%02d.%03d] %s\r\n"),
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, (LPCTSTR)text);

    int nLen = m_editLog.GetWindowTextLength();
    m_editLog.SetSel(nLen, nLen);
    m_editLog.ReplaceSel(entry);
    // Scroll to bottom
    m_editLog.SetSel(m_editLog.GetWindowTextLength(), m_editLog.GetWindowTextLength());
    m_editLog.SendMessage(EM_SCROLLCARET);
}

// ---------------------------------------------------------------------------
// Toolbar Button Handlers
// ---------------------------------------------------------------------------

void CVisionSimulatorDlg::OnBnClickedLoad()
{
    CString filter = _T("Image Files (*.bmp;*.jpg;*.jpeg;*.png;*.tif;*.tiff)|*.bmp;*.jpg;*.jpeg;*.png;*.tif;*.tiff|All Files (*.*)|*.*||");
    CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, filter, this);
    if (dlg.DoModal() == IDOK)
    {
        if (m_originalImage.LoadFromFile(dlg.GetPathName()))
        {
            m_mainViewer.ClearOverlayInfo();
            m_mainViewer.SetImage(m_originalImage);
            m_mainViewer.ClearROIs();
            m_sequenceManager.ClearROIs();
            ClearMiniViewers();
            UpdateROIList();
            m_editLog.SetWindowText(_T(""));
            AddLog(_T("이미지 로드: ") + dlg.GetFileName());

            CString s;
            s.Format(_T("Loaded: %s (%dx%d)"), (LPCTSTR)dlg.GetFileName(),
                m_originalImage.GetWidth(), m_originalImage.GetHeight());
            SetStatus(s);

            m_btnRun.EnableWindow(m_sequenceManager.GetStepCount() > 0);
            m_btnSave.EnableWindow(FALSE);
        }
        else
            MessageBox(_T("Failed to load image."), _T("Error"), MB_OK|MB_ICONERROR);
    }
}

void CVisionSimulatorDlg::OnBnClickedRun()
{
    if (!m_originalImage.IsValid())
    {
        MessageBox(_T("Please load an image first."), _T("Warning"), MB_OK|MB_ICONWARNING);
        return;
    }
    if (m_sequenceManager.GetStepCount() == 0)
    {
        MessageBox(_T("Please add at least one algorithm to the sequence."), _T("Warning"), MB_OK|MB_ICONWARNING);
        return;
    }

    // Sync ROIs to sequence manager
    m_sequenceManager.SetROIs(m_mainViewer.GetROIs());

    m_nSelectedMiniViewer = -1;
    for (int i = 0; i < 8; i++) m_miniViewers[i].SetSelected(false);

    m_mainViewer.ClearOverlayInfo();

    m_btnRun.EnableWindow(FALSE);
    m_btnStop.EnableWindow(TRUE);
    SetStatus(_T("Processing..."));

    // Reset log and record start
    m_editLog.SetWindowText(_T(""));
    CString logStart;
    logStart.Format(_T("=== 처리 시작 (총 %d 스텝) ==="), m_sequenceManager.GetStepCount());
    AddLog(logStart);

    m_sequenceManager.StartExecution(m_originalImage);
}

void CVisionSimulatorDlg::OnBnClickedStop()
{
    m_sequenceManager.StopExecution();
    m_btnRun.EnableWindow(TRUE);
    m_btnStop.EnableWindow(FALSE);
    SetStatus(_T("Stopped"));
}

void CVisionSimulatorDlg::OnBnClickedClear()
{
    m_mainViewer.ClearOverlayInfo();
    m_mainViewer.ClearImage();
    m_originalImage.Release();
    m_mainViewer.ClearROIs();
    m_sequenceManager.ClearROIs();
    ClearMiniViewers();
    UpdateROIList();
    m_editLog.SetWindowText(_T(""));
    SetStatus(_T("초기화 완료"));
    m_btnRun.EnableWindow(FALSE);
    m_btnSave.EnableWindow(FALSE);
}

void CVisionSimulatorDlg::OnBnClickedSave()
{
    CString filter = _T("PNG Files (*.png)|*.png|BMP Files (*.bmp)|*.bmp|JPEG Files (*.jpg)|*.jpg||");
    CFileDialog dlg(FALSE, _T("png"), _T("result"), OFN_OVERWRITEPROMPT, filter, this);
    if (dlg.DoModal() == IDOK)
    {
        const auto& history = m_sequenceManager.GetHistory();
        if (!history.empty())
        {
            // History already contains composited image
            CImageBuffer saveImage = history.back().Clone();
            if (saveImage.SaveToFile(dlg.GetPathName()))
                SetStatus(_T("Saved: ") + dlg.GetFileName());
            else
                MessageBox(_T("Failed to save image."), _T("Error"), MB_OK|MB_ICONERROR);
        }
    }
}

void CVisionSimulatorDlg::OnBnClickedFit()      { m_mainViewer.FitToWindow(); }

void CVisionSimulatorDlg::OnBnClickedROI()
{
    bool cur = m_mainViewer.IsROIMode();
    m_mainViewer.SetROIMode(!cur);
    m_btnROI.SetWindowText(!cur ? _T("Pan Mode") : _T("ROI Mode"));
    SetStatus(!cur ? _T("Left-click ROI draw mode | Right-click always draws ROI") : _T("Pan mode | Right-click still draws ROI"));
}

void CVisionSimulatorDlg::OnBnClickedClearROI()
{
    m_mainViewer.ClearROIs();
    m_sequenceManager.ClearROIs();
    UpdateROIList();
    SetStatus(_T("All ROIs cleared"));
}

void CVisionSimulatorDlg::OnBnClickedRemoveROI()
{
    int sel = m_listROI.GetCurSel();
    if (sel == LB_ERR) return;
    m_mainViewer.RemoveROI(sel);
    m_sequenceManager.SetROIs(m_mainViewer.GetROIs());
    UpdateROIList();
}

void CVisionSimulatorDlg::OnBnClickedClearROIs()
{
    m_mainViewer.ClearROIs();
    m_sequenceManager.ClearROIs();
    UpdateROIList();
}

void CVisionSimulatorDlg::OnBnClickedAddSeq()
{
    // Always add a new clone of m_pCurrentAlgorithm
    if (!m_pCurrentAlgorithm) return;
    CAlgorithmBase* pClone = m_pCurrentAlgorithm->Clone();
    if (pClone)
    {
        m_sequenceManager.AddStep(pClone);
        UpdateSequenceList();
        m_btnRun.EnableWindow(m_originalImage.IsValid());
    }
}

void CVisionSimulatorDlg::OnBnClickedRemoveSeq()
{
    int sel = m_listSequence.GetCurSel();
    if (sel == LB_ERR) return;

    // If we were editing this step, reset panel
    if (m_nEditingSequenceStep == sel)
    {
        m_nEditingSequenceStep = -1;
        m_paramPanel.SetAlgorithm(m_pCurrentAlgorithm);
    }
    else if (m_nEditingSequenceStep > sel)
        m_nEditingSequenceStep--;

    m_sequenceManager.RemoveStep(sel);
    UpdateSequenceList();
    if (m_listSequence.GetCount() > 0)
    {
        int newSel = min(sel, m_listSequence.GetCount() - 1);
        m_listSequence.SetCurSel(newSel);
    }
    m_btnRun.EnableWindow(m_sequenceManager.GetStepCount() > 0 && m_originalImage.IsValid());
}

void CVisionSimulatorDlg::OnBnClickedSeqUp()
{
    int sel = m_listSequence.GetCurSel();
    if (sel == LB_ERR || sel == 0) return;
    if (m_nEditingSequenceStep == sel) m_nEditingSequenceStep--;
    else if (m_nEditingSequenceStep == sel - 1) m_nEditingSequenceStep++;
    m_sequenceManager.MoveStepUp(sel);
    UpdateSequenceList();
    m_listSequence.SetCurSel(sel - 1);
}

void CVisionSimulatorDlg::OnBnClickedSeqDown()
{
    int sel = m_listSequence.GetCurSel();
    if (sel == LB_ERR || sel >= m_listSequence.GetCount() - 1) return;
    if (m_nEditingSequenceStep == sel) m_nEditingSequenceStep++;
    else if (m_nEditingSequenceStep == sel + 1) m_nEditingSequenceStep--;
    m_sequenceManager.MoveStepDown(sel);
    UpdateSequenceList();
    m_listSequence.SetCurSel(sel + 1);
}

void CVisionSimulatorDlg::OnBnClickedClearSeq()
{
    m_nEditingSequenceStep = -1;
    m_paramPanel.SetAlgorithm(m_pCurrentAlgorithm);
    m_sequenceManager.ClearSteps();
    UpdateSequenceList();
}

// ---------------------------------------------------------------------------
// UpdateSequenceList
// ---------------------------------------------------------------------------

void CVisionSimulatorDlg::UpdateSequenceList()
{
    int prevSel = m_listSequence.GetCurSel();
    m_listSequence.ResetContent();

    int maxWidth = 0;
    CDC* pDC = m_listSequence.GetDC();

    for (int i = 0; i < m_sequenceManager.GetStepCount(); i++)
    {
        CAlgorithmBase* pStep = m_sequenceManager.GetStep(i);
        CString text;
        text.Format(_T("%d. %s"), i+1, (LPCTSTR)pStep->GetName());

        auto& params = pStep->GetParams();
        if (!params.empty())
        {
            CString ps;
            for (int p = 0; p < (int)params.size(); p++)
            {
                CString vs;
                if (params[p].nPrecision == 0)
                    vs.Format(_T("%d"), (int)params[p].dCurrentVal);
                else
                {
                    CString fmt; fmt.Format(_T("%%.%df"), params[p].nPrecision);
                    vs.Format(fmt, params[p].dCurrentVal);
                }
                if (p > 0) ps += _T(", ");
                ps += params[p].strName + _T("=") + vs;
            }
            text += _T(" (") + ps + _T(")");
        }
        m_listSequence.AddString(text);

        if (pDC)
        {
            CSize sz = pDC->GetTextExtent(text);
            if (sz.cx > maxWidth) maxWidth = sz.cx;
        }
    }

    if (pDC)
    {
        m_listSequence.ReleaseDC(pDC);
        m_listSequence.SetHorizontalExtent(maxWidth + 10);
    }

    // Restore selection
    if (prevSel >= 0 && prevSel < m_listSequence.GetCount())
        m_listSequence.SetCurSel(prevSel);

    m_btnRun.EnableWindow(m_sequenceManager.GetStepCount() > 0 && m_originalImage.IsValid());
}

// ---------------------------------------------------------------------------
// UpdateROIList
// ---------------------------------------------------------------------------

void CVisionSimulatorDlg::UpdateROIList()
{
    int prevSel = m_listROI.GetCurSel();
    m_listROI.ResetContent();

    const auto& rois = m_mainViewer.GetROIs();
    for (int i = 0; i < (int)rois.size(); i++)
    {
        CString s;
        s.Format(_T("ROI %d: %dx%d @ (%d,%d)"),
            i+1, rois[i].Width(), rois[i].Height(), rois[i].left, rois[i].top);
        m_listROI.AddString(s);
    }

    // Restore selection
    if (prevSel >= 0 && prevSel < m_listROI.GetCount())
        m_listROI.SetCurSel(prevSel);

    // Update status with ROI count
    CString roiInfo;
    if (!rois.empty())
        roiInfo.Format(_T(" | %d ROI(s) active"), (int)rois.size());
}

// ---------------------------------------------------------------------------
// MiniViewer handlers
// ---------------------------------------------------------------------------

LRESULT CVisionSimulatorDlg::OnMiniViewerClicked(WPARAM wParam, LPARAM lParam)
{
    int nIndex = (int)wParam;
    if (nIndex < 0 || nIndex >= 8) return 0;

    if (m_nSelectedMiniViewer >= 0 && m_nSelectedMiniViewer < 8)
        m_miniViewers[m_nSelectedMiniViewer].SetSelected(false);

    m_nSelectedMiniViewer = nIndex;
    m_miniViewers[nIndex].SetSelected(true);

    if (m_miniViewers[nIndex].HasImage())
    {
        m_mainViewer.SetImage(m_miniViewers[nIndex].GetImage());

        // Build green overlay with applied algorithm + params
        CString overlayInfo;
        if (nIndex == 0)
        {
            overlayInfo = _T("원본 이미지");
        }
        else
        {
            int stepIdx = nIndex - 1;
            CAlgorithmBase* pStep = m_sequenceManager.GetStep(stepIdx);
            if (pStep)
            {
                overlayInfo = pStep->GetName();
                auto& params = pStep->GetParams();
                for (int p = 0; p < (int)params.size(); p++)
                {
                    CString pval;
                    if (!params[p].vecOptions.empty())
                    {
                        int iVal = (int)params[p].dCurrentVal;
                        if (iVal >= 0 && iVal < (int)params[p].vecOptions.size())
                            pval = params[p].vecOptions[iVal];
                        else
                            pval.Format(_T("%d"), iVal);
                    }
                    else if (params[p].nPrecision == 0)
                        pval.Format(_T("%d"), (int)params[p].dCurrentVal);
                    else
                    {
                        CString fmt;
                        fmt.Format(_T("%%.%df"), params[p].nPrecision);
                        pval.Format(fmt, params[p].dCurrentVal);
                    }
                    overlayInfo += _T("\n") + params[p].strName + _T(": ") + pval;
                }
            }
        }
        m_mainViewer.SetOverlayInfo(overlayInfo);

        SetStatus(_T("뷰: ") + m_miniViewers[nIndex].GetLabel());
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Sequence execution message handlers
// ---------------------------------------------------------------------------

LRESULT CVisionSimulatorDlg::OnSequenceStepDone(WPARAM wParam, LPARAM lParam)
{
    int stepIdx   = (int)wParam;
    long long elapsedMs = (long long)lParam;

    UpdateMiniViewers();
    const auto& history = m_sequenceManager.GetHistory();
    if (!history.empty())
    {
        m_mainViewer.ClearOverlayInfo();
        m_mainViewer.SetImage(history.back());
    }

    // Log step completion with timing
    CAlgorithmBase* pStep = m_sequenceManager.GetStep(stepIdx);
    CString algName = pStep ? pStep->GetName() : _T("Unknown");
    CString s;
    s.Format(_T("[완료] 스텝 %d - %s (%lldms)"), stepIdx + 1, (LPCTSTR)algName, elapsedMs);
    AddLog(s);

    return 0;
}

LRESULT CVisionSimulatorDlg::OnSequenceProgress(WPARAM wParam, LPARAM lParam)
{
    int stepNum   = (int)wParam;
    int stepTotal = (int)lParam;

    CString s;
    s.Format(_T("[스텝 %d/%d] 처리 중..."), stepNum, stepTotal);
    SetStatus(s);

    // Log step start with algorithm name
    CAlgorithmBase* pStep = m_sequenceManager.GetStep(stepNum - 1);
    CString algName = pStep ? pStep->GetName() : _T("Unknown");
    CString logMsg;
    logMsg.Format(_T("[시작] 스텝 %d/%d - %s"), stepNum, stepTotal, (LPCTSTR)algName);
    AddLog(logMsg);

    return 0;
}

LRESULT CVisionSimulatorDlg::OnSequenceComplete(WPARAM wParam, LPARAM lParam)
{
    m_btnRun.EnableWindow(TRUE);
    m_btnStop.EnableWindow(FALSE);
    m_btnSave.EnableWindow(TRUE);

    const auto& history = m_sequenceManager.GetHistory();
    if (!history.empty())
    {
        m_mainViewer.ClearOverlayInfo();
        m_mainViewer.SetImage(history.back());
    }

    UpdateMiniViewers();
    SetStatus(_T("처리 완료!"));
    AddLog(_T("=== 모든 처리 완료 ==="));
    return 0;
}

LRESULT CVisionSimulatorDlg::OnSequenceError(WPARAM wParam, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    CString errText = pMsg ? *pMsg : _T("Unknown error");
    SetStatus(_T("오류: ") + errText);
    m_btnRun.EnableWindow(TRUE);
    m_btnStop.EnableWindow(FALSE);
    CString logErr;
    logErr.Format(_T("[오류] 스텝 %d 처리 실패"), (int)wParam + 1);
    AddLog(logErr);
    MessageBox(errText, _T("Processing Error"), MB_OK|MB_ICONERROR);
    return 0;
}

// ---------------------------------------------------------------------------
// UpdateMiniViewers  (history images are already composited)
// ---------------------------------------------------------------------------

void CVisionSimulatorDlg::UpdateMiniViewers()
{
    const auto& history = m_sequenceManager.GetHistory();
    int histSize = (int)history.size();

    for (int i = 0; i < 8; i++)
    {
        if (i < histSize)
        {
            m_miniViewers[i].SetImage(history[i]);

            CString label;
            if (i == 0)
                label = _T("Original");
            else if (i - 1 < m_sequenceManager.GetStepCount())
                label = m_sequenceManager.GetStep(i - 1)->GetName();
            else
                label.Format(_T("Step %d"), i);

            m_miniViewers[i].SetLabel(label);
            m_miniLabels[i].SetWindowText(label);
        }
        else
        {
            m_miniViewers[i].Clear();
            m_miniLabels[i].SetWindowText(_T(""));
        }
    }
}

void CVisionSimulatorDlg::ClearMiniViewers()
{
    m_nSelectedMiniViewer = -1;
    for (int i = 0; i < 8; i++)
    {
        m_miniViewers[i].Clear();
        m_miniLabels[i].SetWindowText(_T(""));
    }
}

void CVisionSimulatorDlg::SetStatus(const CString& text)
{
    m_staticStatus.SetWindowText(text);
}
