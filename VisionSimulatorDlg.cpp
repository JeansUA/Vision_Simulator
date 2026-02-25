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
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CVisionSimulatorDlg::~CVisionSimulatorDlg()
{
    if (m_pCurrentAlgorithm) { delete m_pCurrentAlgorithm; m_pCurrentAlgorithm = nullptr; }
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
    int viewerH = contentBottom - contentTop;

    m_mainViewer.MoveWindow(margin, contentTop, viewerW, viewerH);

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
    CDialogEx::OnDestroy();
}

void CVisionSimulatorDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_PREVIEW)
    {
        KillTimer(TIMER_PREVIEW);
        m_bPreviewPending = false;
        RunPreview();
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

void CVisionSimulatorDlg::RunPreview()
{
    if (!m_originalImage.IsValid()) return;

    CAlgorithmBase* pAlg = m_paramPanel.GetAlgorithm();
    if (!pAlg) return;

    // Determine input
    CImageBuffer previewInput;
    if (m_nEditingSequenceStep > 0)
    {
        const auto& history = m_sequenceManager.GetHistory();
        if ((int)history.size() > m_nEditingSequenceStep)
            previewInput = history[m_nEditingSequenceStep].Clone();
        else
            previewInput = m_originalImage.Clone();
    }
    else if (m_nEditingSequenceStep == 0)
    {
        previewInput = m_originalImage.Clone();
    }
    else
    {
        previewInput = m_originalImage.Clone();
    }

    if (!previewInput.IsValid()) return;

    const std::vector<CRect>& rois = m_mainViewer.GetROIs();
    CImageBuffer previewOutput;
    bool success = false;

    if (rois.empty())
    {
        success = pAlg->Process(previewInput, previewOutput);
    }
    else
    {
        previewOutput = previewInput.Clone();
        success = previewOutput.IsValid();
        if (success)
        {
            for (const CRect& roi : rois)
            {
                CImageBuffer roiIn = previewInput.ExtractRegion(roi);
                if (!roiIn.IsValid()) continue;
                CImageBuffer roiOut;
                if (pAlg->Process(roiIn, roiOut) && roiOut.IsValid())
                    previewOutput.PasteRegion(roiOut, roi.left, roi.top);
            }
        }
    }

    if (success && previewOutput.IsValid())
    {
        m_mainViewer.SetImage(previewOutput);
        SetStatus(_T("[Preview] ") + pAlg->GetName() + _T(" - Run to update history"));
    }
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
            m_mainViewer.SetImage(m_originalImage);
            m_mainViewer.ClearROIs();
            m_sequenceManager.ClearROIs();
            ClearMiniViewers();
            UpdateROIList();

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

    m_btnRun.EnableWindow(FALSE);
    m_btnStop.EnableWindow(TRUE);
    SetStatus(_T("Processing..."));

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
    m_mainViewer.ClearImage();
    m_originalImage.Release();
    m_mainViewer.ClearROIs();
    m_sequenceManager.ClearROIs();
    ClearMiniViewers();
    UpdateROIList();
    SetStatus(_T("Cleared"));
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
        SetStatus(_T("Viewing: ") + m_miniViewers[nIndex].GetLabel());
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Sequence execution message handlers
// ---------------------------------------------------------------------------

LRESULT CVisionSimulatorDlg::OnSequenceStepDone(WPARAM wParam, LPARAM lParam)
{
    UpdateMiniViewers();
    const auto& history = m_sequenceManager.GetHistory();
    if (!history.empty())
        m_mainViewer.SetImage(history.back());
    return 0;
}

LRESULT CVisionSimulatorDlg::OnSequenceProgress(WPARAM wParam, LPARAM lParam)
{
    CString s;
    s.Format(_T("Processing step %d of %d..."), (int)wParam, (int)lParam);
    SetStatus(s);
    return 0;
}

LRESULT CVisionSimulatorDlg::OnSequenceComplete(WPARAM wParam, LPARAM lParam)
{
    m_btnRun.EnableWindow(TRUE);
    m_btnStop.EnableWindow(FALSE);
    m_btnSave.EnableWindow(TRUE);

    const auto& history = m_sequenceManager.GetHistory();
    if (!history.empty())
        m_mainViewer.SetImage(history.back());

    UpdateMiniViewers();
    SetStatus(_T("Processing complete!"));
    return 0;
}

LRESULT CVisionSimulatorDlg::OnSequenceError(WPARAM wParam, LPARAM lParam)
{
    CString* pMsg = reinterpret_cast<CString*>(lParam);
    CString errText = pMsg ? *pMsg : _T("Unknown error");
    SetStatus(_T("Error: ") + errText);
    m_btnRun.EnableWindow(TRUE);
    m_btnStop.EnableWindow(FALSE);
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
