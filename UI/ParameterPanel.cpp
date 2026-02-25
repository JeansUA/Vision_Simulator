#include "stdafx.h"
#include "UI/ParameterPanel.h"
#include "Utils/CommonTypes.h"
#include <cmath>

IMPLEMENT_DYNAMIC(CParameterPanel, CWnd)

BEGIN_MESSAGE_MAP(CParameterPanel, CWnd)
    ON_WM_PAINT()
    ON_WM_HSCROLL()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
END_MESSAGE_MAP()

// Slider control IDs start at this base
static const UINT SLIDER_ID_BASE = 3000;
static const UINT EDIT_ID_BASE   = 4000;
static const UINT LABEL_ID_BASE  = 5000;

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
    // Register a window class for the parameter panel
    CString strClassName = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW,
        ::LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE + 1),
        NULL);

    return CWnd::Create(strClassName, _T("ParameterPanel"), dwStyle, rect, pParentWnd, nID);
}

void CParameterPanel::SetAlgorithm(CAlgorithmBase* pAlgorithm)
{
    m_pAlgorithm = pAlgorithm;
    DestroyParamControls();

    if (m_pAlgorithm != nullptr)
    {
        CreateParamControls();
        LayoutControls();
    }

    Invalidate(FALSE);
}

void CParameterPanel::CreateParamControls()
{
    if (m_pAlgorithm == nullptr)
        return;

    std::vector<AlgorithmParam>& params = m_pAlgorithm->GetParams();

    for (int i = 0; i < (int)params.size(); i++)
    {
        const AlgorithmParam& param = params[i];
        ParamControl ctrl;
        ctrl.paramIndex = i;

        // Create label
        ctrl.pLabel = new CStatic();
        ctrl.pLabel->Create(param.strName, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                            CRect(0, 0, 10, 10), this, LABEL_ID_BASE + i);

        // Create slider
        ctrl.pSlider = new CSliderCtrl();
        ctrl.pSlider->Create(WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                             CRect(0, 0, 10, 10), this, SLIDER_ID_BASE + i);

        // Map floating-point range to integer slider range using precision
        int multiplier = (int)pow(10.0, param.nPrecision);
        int sliderMin = (int)(param.dMinVal * multiplier);
        int sliderMax = (int)(param.dMaxVal * multiplier);
        int sliderPos = (int)(param.dCurrentVal * multiplier);

        ctrl.pSlider->SetRange(sliderMin, sliderMax, TRUE);
        ctrl.pSlider->SetPos(sliderPos);

        // Create edit box
        ctrl.pEdit = new CEdit();
        ctrl.pEdit->Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_READONLY,
                           CRect(0, 0, 10, 10), this, EDIT_ID_BASE + i);

        // Format the value text based on precision
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

        m_controls.push_back(ctrl);
    }
}

void CParameterPanel::DestroyParamControls()
{
    for (size_t i = 0; i < m_controls.size(); i++)
    {
        if (m_controls[i].pLabel != nullptr)
        {
            m_controls[i].pLabel->DestroyWindow();
            delete m_controls[i].pLabel;
        }
        if (m_controls[i].pSlider != nullptr)
        {
            m_controls[i].pSlider->DestroyWindow();
            delete m_controls[i].pSlider;
        }
        if (m_controls[i].pEdit != nullptr)
        {
            m_controls[i].pEdit->DestroyWindow();
            delete m_controls[i].pEdit;
        }
    }
    m_controls.clear();
}

void CParameterPanel::LayoutControls()
{
    if (m_controls.empty())
        return;

    CRect rcClient;
    GetClientRect(&rcClient);

    int totalWidth = rcClient.Width();
    int rowHeight = 28;
    int gap = 4;
    int leftMargin = 4;
    int rightMargin = 4;
    int usableWidth = totalWidth - leftMargin - rightMargin;

    int labelWidth = (usableWidth * LABEL_WIDTH_RATIO) / 100;
    int editWidth = (usableWidth * 20) / 100;
    int sliderWidth = usableWidth - labelWidth - editWidth - gap * 2;

    for (int i = 0; i < (int)m_controls.size(); i++)
    {
        int yPos = gap + i * (rowHeight + gap);
        int xPos = leftMargin;

        // Label
        if (m_controls[i].pLabel != nullptr && ::IsWindow(m_controls[i].pLabel->GetSafeHwnd()))
        {
            m_controls[i].pLabel->MoveWindow(xPos, yPos, labelWidth, rowHeight);
        }
        xPos += labelWidth + gap;

        // Slider
        if (m_controls[i].pSlider != nullptr && ::IsWindow(m_controls[i].pSlider->GetSafeHwnd()))
        {
            m_controls[i].pSlider->MoveWindow(xPos, yPos, sliderWidth, rowHeight);
        }
        xPos += sliderWidth + gap;

        // Edit
        if (m_controls[i].pEdit != nullptr && ::IsWindow(m_controls[i].pEdit->GetSafeHwnd()))
        {
            m_controls[i].pEdit->MoveWindow(xPos, yPos, editWidth, rowHeight);
        }
    }
}

void CParameterPanel::UpdateEditFromSlider(int index)
{
    if (index < 0 || index >= (int)m_controls.size())
        return;

    if (m_pAlgorithm == nullptr)
        return;

    std::vector<AlgorithmParam>& params = m_pAlgorithm->GetParams();
    if (m_controls[index].paramIndex >= (int)params.size())
        return;

    const AlgorithmParam& param = params[m_controls[index].paramIndex];

    int sliderPos = m_controls[index].pSlider->GetPos();
    int multiplier = (int)pow(10.0, param.nPrecision);
    double actualValue = (double)sliderPos / (double)multiplier;

    // Update the algorithm parameter
    params[m_controls[index].paramIndex].dCurrentVal = actualValue;

    // Update the edit box text
    CString strValue;
    if (param.nPrecision == 0)
        strValue.Format(_T("%d"), (int)actualValue);
    else
    {
        CString strFmt;
        strFmt.Format(_T("%%.%df"), param.nPrecision);
        strValue.Format(strFmt, actualValue);
    }
    m_controls[index].pEdit->SetWindowText(strValue);

    // Notify parent for real-time preview
    CWnd* pParent = GetParent();
    if (pParent && ::IsWindow(pParent->GetSafeHwnd()))
        pParent->PostMessage(WM_PARAM_CHANGED, 0, 0);
}

void CParameterPanel::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    // Determine which slider sent the message
    if (pScrollBar != nullptr)
    {
        UINT nID = pScrollBar->GetDlgCtrlID();
        if (nID >= SLIDER_ID_BASE && nID < SLIDER_ID_BASE + m_controls.size())
        {
            int index = nID - SLIDER_ID_BASE;
            UpdateEditFromSlider(index);
        }
    }

    CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CParameterPanel::OnPaint()
{
    CPaintDC dc(this);

    CRect rcClient;
    GetClientRect(&rcClient);

    // Fill with dialog face color
    dc.FillSolidRect(&rcClient, RGB(240, 240, 240));
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

void CParameterPanel::UpdateFromAlgorithm()
{
    if (m_pAlgorithm == nullptr)
        return;

    std::vector<AlgorithmParam>& params = m_pAlgorithm->GetParams();

    for (int i = 0; i < (int)m_controls.size(); i++)
    {
        int paramIdx = m_controls[i].paramIndex;
        if (paramIdx >= (int)params.size())
            continue;

        const AlgorithmParam& param = params[paramIdx];
        int multiplier = (int)pow(10.0, param.nPrecision);
        int sliderPos = (int)(param.dCurrentVal * multiplier);

        if (m_controls[i].pSlider != nullptr && ::IsWindow(m_controls[i].pSlider->GetSafeHwnd()))
        {
            m_controls[i].pSlider->SetPos(sliderPos);
        }

        CString strValue;
        if (param.nPrecision == 0)
            strValue.Format(_T("%d"), (int)param.dCurrentVal);
        else
        {
            CString strFmt;
            strFmt.Format(_T("%%.%df"), param.nPrecision);
            strValue.Format(strFmt, param.dCurrentVal);
        }

        if (m_controls[i].pEdit != nullptr && ::IsWindow(m_controls[i].pEdit->GetSafeHwnd()))
        {
            m_controls[i].pEdit->SetWindowText(strValue);
        }
    }
}

void CParameterPanel::Clear()
{
    DestroyParamControls();
    m_pAlgorithm = nullptr;
    Invalidate(FALSE);
}
