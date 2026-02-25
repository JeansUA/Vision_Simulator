#include "stdafx.h"
#include "UI/ParameterPanel.h"
#include "Utils/CommonTypes.h"
#include <cmath>

IMPLEMENT_DYNAMIC(CParameterPanel, CWnd)

// Control ID ranges
#define SLIDER_ID_BASE  3000
#define EDIT_ID_BASE    4000
#define LABEL_ID_BASE   5000
#define COMBO_ID_BASE   6000
#define DESC_ID_BASE    7000

static const int ROW_H  = 26;   // label + control row height
static const int DESC_H = 15;   // description text row height
static const int GAP    = 5;    // gap between params
static const int MARGIN = 5;    // left/right margin

BEGIN_MESSAGE_MAP(CParameterPanel, CWnd)
    ON_WM_PAINT()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_MOUSEWHEEL()
    ON_CONTROL_RANGE(CBN_SELCHANGE, COMBO_ID_BASE, COMBO_ID_BASE + 30, &CParameterPanel::OnComboSelChange)
END_MESSAGE_MAP()

CParameterPanel::CParameterPanel()
    : m_pAlgorithm(nullptr)
{
}

CParameterPanel::~CParameterPanel()
{
    DestroyParamControls();
}

BOOL CParameterPanel::Create(DWORD dwStyle, const CRect& rect, CWnd* pParentWnd, UINT nID)
{
    CString strClassName = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW,
        ::LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE + 1),
        NULL);

    dwStyle |= WS_VSCROLL;
    return CWnd::Create(strClassName, _T("ParameterPanel"), dwStyle, rect, pParentWnd, nID);
}

// ============================================================================
// Public Interface
// ============================================================================

void CParameterPanel::SetAlgorithm(CAlgorithmBase* pAlgorithm)
{
    m_pAlgorithm = pAlgorithm;
    DestroyParamControls();

    if (m_pAlgorithm != nullptr)
    {
        CreateParamControls();
        UpdateParamVisibility();
    }

    Invalidate(FALSE);
}

void CParameterPanel::UpdateFromAlgorithm()
{
    if (m_pAlgorithm == nullptr) return;

    std::vector<AlgorithmParam>& params = m_pAlgorithm->GetParams();

    for (int i = 0; i < (int)m_controls.size(); i++)
    {
        int paramIdx = m_controls[i].paramIndex;
        if (paramIdx >= (int)params.size()) continue;

        const AlgorithmParam& param = params[paramIdx];

        if (m_controls[i].pCombo && ::IsWindow(m_controls[i].pCombo->GetSafeHwnd()))
        {
            m_controls[i].pCombo->SetCurSel((int)param.dCurrentVal);
        }
        else if (m_controls[i].pSlider && ::IsWindow(m_controls[i].pSlider->GetSafeHwnd()))
        {
            int multiplier = (int)pow(10.0, param.nPrecision);
            m_controls[i].pSlider->SetPos((int)(param.dCurrentVal * multiplier));

            CString strValue;
            if (param.nPrecision == 0)
                strValue.Format(_T("%d"), (int)param.dCurrentVal);
            else
            {
                CString strFmt;
                strFmt.Format(_T("%%.%df"), param.nPrecision);
                strValue.Format(strFmt, param.dCurrentVal);
            }
            if (m_controls[i].pEdit && ::IsWindow(m_controls[i].pEdit->GetSafeHwnd()))
                m_controls[i].pEdit->SetWindowText(strValue);
        }
    }
}

void CParameterPanel::Clear()
{
    DestroyParamControls();
    m_pAlgorithm = nullptr;
    SetScrollPos(SB_VERT, 0, FALSE);
    Invalidate(FALSE);
}

// ============================================================================
// Control Creation / Destruction
// ============================================================================

void CParameterPanel::CreateParamControls()
{
    if (m_pAlgorithm == nullptr) return;

    std::vector<AlgorithmParam>& params = m_pAlgorithm->GetParams();

    for (int i = 0; i < (int)params.size(); i++)
    {
        const AlgorithmParam& param = params[i];
        ParamControl ctrl = {};
        ctrl.paramIndex = i;
        ctrl.bVisible   = true;

        // Label
        ctrl.pLabel = new CStatic();
        ctrl.pLabel->Create(param.strName,
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
            CRect(0,0,10,10), this, LABEL_ID_BASE + i);
        ctrl.pLabel->SetFont(GetFont());

        // Description text (small, gray)
        ctrl.pDesc = new CStatic();
        ctrl.pDesc->Create(param.strDescription,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            CRect(0,0,10,10), this, DESC_ID_BASE + i);
        ctrl.pDesc->SetFont(GetFont());

        if (!param.vecOptions.empty())
        {
            // Combo box
            ctrl.pCombo = new CComboBox();
            ctrl.pCombo->Create(
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL,
                CRect(0,0,10,10), this, COMBO_ID_BASE + i);
            ctrl.pCombo->SetFont(GetFont());

            for (const CString& opt : param.vecOptions)
                ctrl.pCombo->AddString(opt);
            ctrl.pCombo->SetCurSel((int)param.dCurrentVal);

            ctrl.pSlider = nullptr;
            ctrl.pEdit   = nullptr;
        }
        else
        {
            // Slider + edit
            ctrl.pSlider = new CSliderCtrl();
            ctrl.pSlider->Create(WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                CRect(0,0,10,10), this, SLIDER_ID_BASE + i);

            int multiplier = (int)pow(10.0, param.nPrecision);
            int sliderMin  = (int)(param.dMinVal * multiplier);
            int sliderMax  = (int)(param.dMaxVal * multiplier);
            int sliderPos  = (int)(param.dCurrentVal * multiplier);
            ctrl.pSlider->SetRange(sliderMin, sliderMax, TRUE);
            ctrl.pSlider->SetPos(sliderPos);

            ctrl.pEdit = new CEdit();
            ctrl.pEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_READONLY,
                CRect(0,0,10,10), this, EDIT_ID_BASE + i);
            ctrl.pEdit->SetFont(GetFont());

            CString strValue;
            if (param.nPrecision == 0)
                strValue.Format(_T("%d"), (int)param.dCurrentVal);
            else
            {
                CString strFmt;
                strFmt.Format(_T("%%.%df"), param.nPrecision);
                strValue.Format(strFmt, param.dCurrentVal);
            }
            ctrl.pEdit->SetWindowText(strValue);

            ctrl.pCombo = nullptr;
        }

        m_controls.push_back(ctrl);
    }
}

void CParameterPanel::DestroyParamControls()
{
    for (auto& ctrl : m_controls)
    {
        auto DestroyCtrl = [](CWnd* p) {
            if (p) { p->DestroyWindow(); delete p; }
        };
        DestroyCtrl(ctrl.pLabel);
        DestroyCtrl(ctrl.pDesc);
        DestroyCtrl(ctrl.pSlider);
        DestroyCtrl(ctrl.pEdit);
        DestroyCtrl(ctrl.pCombo);
    }
    m_controls.clear();
}

// ============================================================================
// Layout
// ============================================================================

int CParameterPanel::GetTotalContentHeight() const
{
    int total = MARGIN;
    for (const auto& ctrl : m_controls)
    {
        if (!ctrl.bVisible) continue;
        total += ROW_H + DESC_H + GAP;
    }
    return total + MARGIN;
}

void CParameterPanel::LayoutControls()
{
    if (m_controls.empty()) return;

    CRect rcClient;
    GetClientRect(&rcClient);
    int w = rcClient.Width() - MARGIN * 2;
    if (w < 10) w = 10;

    int scrollY  = GetScrollPos(SB_VERT);
    int labelW   = (w * LABEL_WIDTH_RATIO) / 100;
    int editW    = (w * 18) / 100;
    int sliderW  = w - labelW - editW - GAP * 2;

    // Update scrollbar
    int totalH  = GetTotalContentHeight();
    int clientH = rcClient.Height();
    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin   = 0;
    si.nMax   = max(0, totalH - 1);
    si.nPage  = clientH;
    si.nPos   = scrollY;
    SetScrollInfo(SB_VERT, &si, TRUE);

    int yPos = MARGIN - scrollY;

    for (int i = 0; i < (int)m_controls.size(); i++)
    {
        ParamControl& ctrl = m_controls[i];
        if (!ctrl.bVisible)
        {
            // Move off-screen
            auto Hide = [](CWnd* p) {
                if (p && ::IsWindow(p->GetSafeHwnd()))
                    p->MoveWindow(-9999, -9999, 0, 0);
            };
            Hide(ctrl.pLabel);
            Hide(ctrl.pDesc);
            Hide(ctrl.pSlider);
            Hide(ctrl.pEdit);
            Hide(ctrl.pCombo);
            continue;
        }

        int x = MARGIN;

        // Label
        if (ctrl.pLabel && ::IsWindow(ctrl.pLabel->GetSafeHwnd()))
            ctrl.pLabel->MoveWindow(x, yPos, labelW, ROW_H);

        int ctrlX = x + labelW + GAP;

        if (ctrl.pCombo && ::IsWindow(ctrl.pCombo->GetSafeHwnd()))
        {
            // Combo: fills remaining width
            ctrl.pCombo->MoveWindow(ctrlX, yPos, w - labelW - GAP, ROW_H + 120);
        }
        else
        {
            // Slider + edit
            if (ctrl.pSlider && ::IsWindow(ctrl.pSlider->GetSafeHwnd()))
                ctrl.pSlider->MoveWindow(ctrlX, yPos, sliderW, ROW_H);

            if (ctrl.pEdit && ::IsWindow(ctrl.pEdit->GetSafeHwnd()))
                ctrl.pEdit->MoveWindow(ctrlX + sliderW + GAP, yPos, editW, ROW_H);
        }

        // Description (spans full width below)
        if (ctrl.pDesc && ::IsWindow(ctrl.pDesc->GetSafeHwnd()))
            ctrl.pDesc->MoveWindow(x, yPos + ROW_H + 1, w, DESC_H);

        yPos += ROW_H + DESC_H + GAP;
    }
}

// ============================================================================
// Visibility logic
// ============================================================================

void CParameterPanel::UpdateParamVisibility()
{
    if (!m_pAlgorithm) return;

    std::vector<AlgorithmParam>& params = m_pAlgorithm->GetParams();
    int n = (int)params.size();

    // Default: all visible
    std::vector<bool> vis(n, true);

    // Get first enum (method/channel/operation) param value
    int method = -1;
    for (int i = 0; i < n; i++)
    {
        if (!params[i].vecOptions.empty())
        {
            method = (int)params[i].dCurrentVal;
            break;
        }
    }

    CString algName = m_pAlgorithm->GetName();

    if (algName == _T("Binarize") && method >= 0 && n >= 4)
    {
        // [0]=method  [1]=threshold  [2]=threshold2  [3]=blockSize
        vis[1] = (method <= 2);   // 표준/반전/이중 임계값만 threshold 사용
        vis[2] = (method == 2);   // threshold2: 이중 임계값만
        vis[3] = (method == 3);   // blockSize: 적응형만
    }
    else if (algName == _T("Blur") && method >= 0 && n >= 3)
    {
        // [0]=method  [1]=kernelSize  [2]=sigma
        vis[2] = (method <= 1);   // sigma: 가우시안/양방향만
    }
    else if (algName == _T("Edge Detect") && method >= 0 && n >= 3)
    {
        // [0]=method  [1]=threshold  [2]=highThresh
        vis[2] = (method == 2);   // highThresh: 캐니만
    }
    else if (algName == _T("Brightness/Contrast") && method >= 0 && n >= 4)
    {
        // [0]=method  [1]=brightness  [2]=contrast  [3]=gamma
        vis[1] = (method == 0);
        vis[2] = (method == 0);
        vis[3] = (method == 1);
        // method==2 (히스토그램): 추가 파라미터 없음
    }
    else if (algName == _T("Sharpening") && method >= 0 && n >= 3)
    {
        // [0]=method  [1]=strength  [2]=radius
        vis[2] = (method != 1);   // radius: 라플라시안 제외
    }
    else if (algName == _T("Hough Line") && method >= 0 && n >= 4)
    {
        // [0]=method  [1]=threshold  [2]=minLength  [3]=maxGap
        vis[2] = (method == 1);
        vis[3] = (method == 1);
    }

    // Apply
    for (int i = 0; i < (int)m_controls.size(); i++)
    {
        int pIdx = m_controls[i].paramIndex;
        m_controls[i].bVisible = (pIdx < n) ? vis[pIdx] : true;
    }

    LayoutControls();
}

// ============================================================================
// Slider change
// ============================================================================

void CParameterPanel::UpdateEditFromSlider(int index)
{
    if (index < 0 || index >= (int)m_controls.size()) return;
    if (!m_pAlgorithm) return;

    std::vector<AlgorithmParam>& params = m_pAlgorithm->GetParams();
    int paramIdx = m_controls[index].paramIndex;
    if (paramIdx >= (int)params.size()) return;

    AlgorithmParam& param = params[paramIdx];
    int sliderPos = m_controls[index].pSlider->GetPos();
    int multiplier = (int)pow(10.0, param.nPrecision);
    double actualValue = (double)sliderPos / (double)multiplier;
    param.dCurrentVal = actualValue;

    CString strValue;
    if (param.nPrecision == 0)
        strValue.Format(_T("%d"), (int)actualValue);
    else
    {
        CString strFmt;
        strFmt.Format(_T("%%.%df"), param.nPrecision);
        strValue.Format(strFmt, actualValue);
    }
    if (m_controls[index].pEdit && ::IsWindow(m_controls[index].pEdit->GetSafeHwnd()))
        m_controls[index].pEdit->SetWindowText(strValue);

    NotifyParent();
}

void CParameterPanel::NotifyParent()
{
    CWnd* pParent = GetParent();
    if (pParent && ::IsWindow(pParent->GetSafeHwnd()))
        pParent->PostMessage(WM_PARAM_CHANGED, 0, 0);
}

// ============================================================================
// Message Handlers
// ============================================================================

void CParameterPanel::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    if (pScrollBar != nullptr)
    {
        UINT nID = pScrollBar->GetDlgCtrlID();
        if (nID >= SLIDER_ID_BASE && nID < SLIDER_ID_BASE + (UINT)m_controls.size())
        {
            int index = nID - SLIDER_ID_BASE;
            UpdateEditFromSlider(index);
        }
    }
    CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CParameterPanel::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask  = SIF_ALL;
    GetScrollInfo(SB_VERT, &si);

    int pos = si.nPos;
    switch (nSBCode)
    {
    case SB_LINEUP:       pos -= 20;          break;
    case SB_LINEDOWN:     pos += 20;          break;
    case SB_PAGEUP:       pos -= si.nPage;    break;
    case SB_PAGEDOWN:     pos += si.nPage;    break;
    case SB_THUMBTRACK:   pos = si.nTrackPos; break;
    case SB_THUMBPOSITION:pos = nPos;         break;
    case SB_TOP:          pos = si.nMin;      break;
    case SB_BOTTOM:       pos = si.nMax;      break;
    default: break;
    }

    int maxPos = max(0, si.nMax - (int)si.nPage + 1);
    pos = max(0, min(maxPos, pos));

    if (pos != si.nPos)
    {
        SetScrollPos(SB_VERT, pos, TRUE);
        LayoutControls();
    }

    CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CParameterPanel::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    int pos = GetScrollPos(SB_VERT);
    pos -= (zDelta / WHEEL_DELTA) * 30;

    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask  = SIF_ALL;
    GetScrollInfo(SB_VERT, &si);
    int maxPos = max(0, si.nMax - (int)si.nPage + 1);
    pos = max(0, min(maxPos, pos));

    SetScrollPos(SB_VERT, pos, TRUE);
    LayoutControls();
    return TRUE;
}

void CParameterPanel::OnComboSelChange(UINT nID)
{
    int index = nID - COMBO_ID_BASE;
    if (index < 0 || index >= (int)m_controls.size()) return;

    ParamControl& ctrl = m_controls[index];
    if (!ctrl.pCombo || !::IsWindow(ctrl.pCombo->GetSafeHwnd())) return;

    int sel = ctrl.pCombo->GetCurSel();
    if (sel == CB_ERR) return;

    std::vector<AlgorithmParam>& params = m_pAlgorithm->GetParams();
    if (ctrl.paramIndex < (int)params.size())
        params[ctrl.paramIndex].dCurrentVal = (double)sel;

    // Re-evaluate param visibility when method combo changes
    UpdateParamVisibility();
    NotifyParent();
}

void CParameterPanel::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);
    dc.FillSolidRect(&rcClient, RGB(240, 240, 240));

    // Draw description texts with small gray font
    CFont fontDesc;
    fontDesc.CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("Segoe UI"));

    CFont* pOldFont = dc.SelectObject(&fontDesc);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(RGB(100, 100, 100));

    int scrollY = GetScrollPos(SB_VERT);
    int yPos = MARGIN - scrollY;

    for (int i = 0; i < (int)m_controls.size(); i++)
    {
        if (!m_controls[i].bVisible) { continue; }
        yPos += ROW_H + 1;  // skip the control row

        // Description is drawn directly (the CStatic pDesc handles it, just ensure font)
        yPos += DESC_H + GAP;
    }

    dc.SelectObject(pOldFont);
}

BOOL CParameterPanel::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void CParameterPanel::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    LayoutControls();
}
