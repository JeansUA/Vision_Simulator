#include "stdafx.h"
#include "UI/ImageViewer.h"
#include "Utils/CommonTypes.h"

IMPLEMENT_DYNAMIC(CImageViewer, CStatic)

BEGIN_MESSAGE_MAP(CImageViewer, CStatic)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_MOUSEWHEEL()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

CImageViewer::CImageViewer()
    : m_hBitmap(NULL)
    , m_dZoom(1.0)
    , m_dMinZoom(0.01)
    , m_dMaxZoom(50.0)
    , m_bDragging(false)
    , m_clrBackground(RGB(45, 45, 48))
    , m_ptPan(0, 0)
    , m_ptDragStart(0, 0)
    , m_ptPanStart(0, 0)
    , m_nSelectedROI(-1)
    , m_bROIMode(false)
    , m_bDrawingROI(false)
    , m_bRightClickROI(false)
    , m_ptROIStart(0, 0)
    , m_ptROIEnd(0, 0)
    , m_bMouseInWindow(false)
    , m_bHasPixelInfo(false)
    , m_ptMouseImg(0, 0)
    , m_bPixelR(0), m_bPixelG(0), m_bPixelB(0)
{
}

CImageViewer::~CImageViewer()
{
    if (m_hBitmap != NULL)
    {
        ::DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }
}

// ============================================================================
// Image Management
// ============================================================================

void CImageViewer::SetImage(const CImageBuffer& image)
{
    m_image = image.Clone();
    UpdateBitmap();
    FitToWindow();
    Invalidate(FALSE);
}

void CImageViewer::ClearImage()
{
    m_image.Release();
    if (m_hBitmap != NULL)
    {
        ::DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }
    m_dZoom = 1.0;
    m_ptPan = CPoint(0, 0);
    m_rcROIs.clear();
    m_nSelectedROI = -1;
    m_bROIMode = false;
    m_bDrawingROI = false;
    m_bHasPixelInfo = false;
    Invalidate(FALSE);
}

void CImageViewer::UpdateBitmap()
{
    if (m_hBitmap != NULL)
    {
        ::DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }
    if (m_image.IsValid())
        m_hBitmap = m_image.CreateHBitmap();
}

// ============================================================================
// Zoom / Pan
// ============================================================================

void CImageViewer::FitToWindow()
{
    if (!m_image.IsValid()) return;

    CRect rcClient;
    GetClientRect(&rcClient);
    if (rcClient.Width() <= 0 || rcClient.Height() <= 0) return;

    int imgW = m_image.GetWidth();
    int imgH = m_image.GetHeight();
    if (imgW <= 0 || imgH <= 0) return;

    double zoomX = (double)rcClient.Width() / (double)imgW;
    double zoomY = (double)rcClient.Height() / (double)imgH;
    m_dZoom = min(zoomX, zoomY);
    if (m_dZoom < m_dMinZoom) m_dZoom = m_dMinZoom;
    if (m_dZoom > m_dMaxZoom) m_dZoom = m_dMaxZoom;

    int scaledW = (int)(imgW * m_dZoom);
    int scaledH = (int)(imgH * m_dZoom);
    m_ptPan.x = (rcClient.Width() - scaledW) / 2;
    m_ptPan.y = (rcClient.Height() - scaledH) / 2;

    Invalidate(FALSE);
}

void CImageViewer::ZoomIn()  { SetZoom(m_dZoom * 1.25); }
void CImageViewer::ZoomOut() { SetZoom(m_dZoom / 1.25); }

void CImageViewer::SetZoom(double zoom)
{
    if (zoom < m_dMinZoom) zoom = m_dMinZoom;
    if (zoom > m_dMaxZoom) zoom = m_dMaxZoom;

    if (!m_image.IsValid()) { m_dZoom = zoom; return; }

    CRect rcClient;
    GetClientRect(&rcClient);
    CPoint ptCenter(rcClient.Width() / 2, rcClient.Height() / 2);

    double imgX = (ptCenter.x - m_ptPan.x) / m_dZoom;
    double imgY = (ptCenter.y - m_ptPan.y) / m_dZoom;
    m_dZoom = zoom;
    m_ptPan.x = (int)(ptCenter.x - imgX * m_dZoom);
    m_ptPan.y = (int)(ptCenter.y - imgY * m_dZoom);

    ClampPan();
    Invalidate(FALSE);
}

CRect CImageViewer::GetImageRect()
{
    if (!m_image.IsValid()) return CRect(0, 0, 0, 0);
    int scaledW = (int)(m_image.GetWidth() * m_dZoom);
    int scaledH = (int)(m_image.GetHeight() * m_dZoom);
    return CRect(m_ptPan.x, m_ptPan.y, m_ptPan.x + scaledW, m_ptPan.y + scaledH);
}

void CImageViewer::ClampPan()
{
    if (!m_image.IsValid()) return;

    CRect rcClient;
    GetClientRect(&rcClient);
    int scaledW = (int)(m_image.GetWidth() * m_dZoom);
    int scaledH = (int)(m_image.GetHeight() * m_dZoom);
    int margin = 50;

    if (m_ptPan.x > rcClient.Width() - margin)  m_ptPan.x = rcClient.Width() - margin;
    if (m_ptPan.y > rcClient.Height() - margin)  m_ptPan.y = rcClient.Height() - margin;
    if (m_ptPan.x < -(scaledW - margin))          m_ptPan.x = -(scaledW - margin);
    if (m_ptPan.y < -(scaledH - margin))          m_ptPan.y = -(scaledH - margin);
}

// ============================================================================
// ROI Helpers
// ============================================================================

CPoint CImageViewer::ScreenToImage(CPoint ptScreen)
{
    int imgX = (int)((ptScreen.x - m_ptPan.x) / m_dZoom);
    int imgY = (int)((ptScreen.y - m_ptPan.y) / m_dZoom);
    if (imgX < 0) imgX = 0;
    if (imgY < 0) imgY = 0;
    if (m_image.IsValid())
    {
        if (imgX >= m_image.GetWidth())  imgX = m_image.GetWidth() - 1;
        if (imgY >= m_image.GetHeight()) imgY = m_image.GetHeight() - 1;
    }
    return CPoint(imgX, imgY);
}

CPoint CImageViewer::ImageToScreen(CPoint ptImage)
{
    return CPoint(
        (int)(ptImage.x * m_dZoom + m_ptPan.x),
        (int)(ptImage.y * m_dZoom + m_ptPan.y));
}

CRect CImageViewer::ImageRectToScreen(const CRect& rcImage)
{
    CPoint tl = ImageToScreen(CPoint(rcImage.left, rcImage.top));
    CPoint br = ImageToScreen(CPoint(rcImage.right, rcImage.bottom));
    return CRect(tl, br);
}

void CImageViewer::AddROI(const CRect& rcROI)
{
    m_rcROIs.push_back(rcROI);
    m_nSelectedROI = (int)m_rcROIs.size() - 1;
    NotifyParentROIUpdated();
    Invalidate(FALSE);
}

void CImageViewer::RemoveROI(int index)
{
    if (index < 0 || index >= (int)m_rcROIs.size()) return;
    m_rcROIs.erase(m_rcROIs.begin() + index);
    if (m_nSelectedROI >= (int)m_rcROIs.size())
        m_nSelectedROI = (int)m_rcROIs.size() - 1;
    NotifyParentROIUpdated();
    Invalidate(FALSE);
}

void CImageViewer::ClearROIs()
{
    m_rcROIs.clear();
    m_nSelectedROI = -1;
    NotifyParentROIUpdated();
    Invalidate(FALSE);
}

CRect CImageViewer::GetROI(int index) const
{
    if (index < 0 || index >= (int)m_rcROIs.size()) return CRect(0,0,0,0);
    return m_rcROIs[index];
}

void CImageViewer::SetSelectedROI(int index)
{
    m_nSelectedROI = index;
    Invalidate(FALSE);
}

void CImageViewer::SetROI(const CRect& rcROI)
{
    m_rcROIs.clear();
    if (!rcROI.IsRectEmpty())
    {
        m_rcROIs.push_back(rcROI);
        m_nSelectedROI = 0;
    }
    else
    {
        m_nSelectedROI = -1;
    }
    Invalidate(FALSE);
}

void CImageViewer::SetROIMode(bool bEnable)
{
    m_bROIMode = bEnable;
    if (!bEnable && m_bDrawingROI && !m_bRightClickROI)
    {
        m_bDrawingROI = false;
        ReleaseCapture();
    }
}

// Hit-test: returns ROI index if screen point is near ROI border/corner
int CImageViewer::HitTestROI(CPoint ptScreen) const
{
    // Check if inside any ROI bounding box (in screen space)
    for (int i = (int)m_rcROIs.size() - 1; i >= 0; i--)
    {
        CRect rcScr = const_cast<CImageViewer*>(this)->ImageRectToScreen(m_rcROIs[i]);
        rcScr.NormalizeRect();
        CRect rcInflated = rcScr;
        rcInflated.InflateRect(4, 4);
        if (rcInflated.PtInRect(ptScreen))
            return i;
    }
    return -1;
}

void CImageViewer::FinalizeROI(CPoint ptEnd)
{
    m_bDrawingROI = false;
    m_ptROIEnd = ptEnd;
    ReleaseCapture();

    CPoint imgStart = ScreenToImage(m_ptROIStart);
    CPoint imgEnd   = ScreenToImage(m_ptROIEnd);

    CRect rcNew;
    rcNew.left   = min(imgStart.x, imgEnd.x);
    rcNew.top    = min(imgStart.y, imgEnd.y);
    rcNew.right  = max(imgStart.x, imgEnd.x) + 1;
    rcNew.bottom = max(imgStart.y, imgEnd.y) + 1;

    // Clamp to image bounds
    if (m_image.IsValid())
    {
        rcNew.left   = max(rcNew.left, 0);
        rcNew.top    = max(rcNew.top, 0);
        rcNew.right  = min(rcNew.right,  m_image.GetWidth());
        rcNew.bottom = min(rcNew.bottom, m_image.GetHeight());
    }

    // Minimum 4x4 pixels
    if (rcNew.Width() >= 4 && rcNew.Height() >= 4)
    {
        AddROI(rcNew);
    }
    else
    {
        Invalidate(FALSE);
    }
}

void CImageViewer::NotifyParentROIUpdated()
{
    CWnd* pParent = GetParent();
    if (pParent && ::IsWindow(pParent->GetSafeHwnd()))
        pParent->PostMessage(WM_ROI_UPDATED, (WPARAM)m_rcROIs.size(), 0);
}

// ============================================================================
// Drawing
// ============================================================================

void CImageViewer::DrawROIOverlay(CDC* pDC, const CRect& /*rcClient*/)
{
    // Draw all committed ROIs
    static const COLORREF roiColors[] = {
        RGB(0, 255, 0),    RGB(255, 200, 0),   RGB(0, 200, 255),
        RGB(255, 0, 180),  RGB(180, 255, 0),   RGB(0, 255, 200),
        RGB(255, 100, 0),  RGB(100, 0, 255)
    };
    static const int nColors = sizeof(roiColors) / sizeof(roiColors[0]);

    for (int i = 0; i < (int)m_rcROIs.size(); i++)
    {
        CRect rcScr = ImageRectToScreen(m_rcROIs[i]);
        if (rcScr.Width() <= 0 || rcScr.Height() <= 0) continue;

        bool bSelected = (i == m_nSelectedROI);
        COLORREF col = roiColors[i % nColors];
        COLORREF dimCol = RGB(GetRValue(col)/2, GetGValue(col)/2, GetBValue(col)/2);

        // Outer dashed rectangle
        CPen penDash(PS_DASH, 1, bSelected ? col : dimCol);
        CPen* pOldPen = pDC->SelectObject(&penDash);
        CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
        pDC->Rectangle(rcScr);

        // Inner solid rectangle
        CPen penSolid(PS_SOLID, 1, bSelected ? col : dimCol);
        pDC->SelectObject(&penSolid);
        CRect rcInner = rcScr;
        rcInner.DeflateRect(1, 1);
        if (rcInner.Width() > 0 && rcInner.Height() > 0)
            pDC->Rectangle(rcInner);

        pDC->SelectObject(pOldPen);
        pDC->SelectObject(pOldBrush);

        // Label "ROI N: WxH"
        CString strLabel;
        strLabel.Format(_T("ROI %d: %dx%d"), i + 1, m_rcROIs[i].Width(), m_rcROIs[i].Height());
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(bSelected ? col : dimCol);

        CFont font;
        font.CreatePointFont(75, _T("Segoe UI"));
        CFont* pOldFont = pDC->SelectObject(&font);

        int textY = rcScr.top - 14;
        if (textY < 2) textY = rcScr.bottom + 2;
        pDC->TextOut(rcScr.left + 2, textY, strLabel);
        pDC->SelectObject(pOldFont);
    }

    // Draw ROI being dragged (dashed white rectangle)
    if (m_bDrawingROI)
    {
        CRect rcDrag(
            min(m_ptROIStart.x, m_ptROIEnd.x),
            min(m_ptROIStart.y, m_ptROIEnd.y),
            max(m_ptROIStart.x, m_ptROIEnd.x),
            max(m_ptROIStart.y, m_ptROIEnd.y));

        if (rcDrag.Width() > 0 && rcDrag.Height() > 0)
        {
            CPen penNew(PS_DASH, 1, RGB(255, 255, 255));
            CPen* pOldPen = pDC->SelectObject(&penNew);
            CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
            pDC->Rectangle(rcDrag);
            pDC->SelectObject(pOldPen);
            pDC->SelectObject(pOldBrush);

            // Show size while drawing
            CPoint imgStart = ScreenToImage(m_ptROIStart);
            CPoint imgEnd   = ScreenToImage(m_ptROIEnd);
            int w = abs(imgEnd.x - imgStart.x);
            int h = abs(imgEnd.y - imgStart.y);
            CString strSize;
            strSize.Format(_T("%dx%d"), w, h);
            pDC->SetBkMode(TRANSPARENT);
            pDC->SetTextColor(RGB(255, 255, 255));
            CFont font;
            font.CreatePointFont(75, _T("Segoe UI"));
            CFont* pOldFont = pDC->SelectObject(&font);
            pDC->TextOut(rcDrag.left + 2, rcDrag.top + 2, strSize);
            pDC->SelectObject(pOldFont);
        }
    }
}

void CImageViewer::DrawPixelInfo(CDC* pDC, const CRect& rcClient)
{
    if (!m_bMouseInWindow || !m_bHasPixelInfo) return;

    CString strInfo;
    strInfo.Format(_T("(%d,%d)  R:%d  G:%d  B:%d"),
        m_ptMouseImg.x, m_ptMouseImg.y,
        (int)m_bPixelR, (int)m_bPixelG, (int)m_bPixelB);

    CFont font;
    font.CreatePointFont(82, _T("Courier New"));
    CFont* pOldFont = pDC->SelectObject(&font);

    // Measure text
    CSize sz = pDC->GetTextExtent(strInfo);
    int padding = 6;
    int boxW = sz.cx + padding * 2;
    int boxH = sz.cy + padding;
    CRect rcBox(5, 5, 5 + boxW, 5 + boxH);

    // Background: semi-opaque black
    pDC->FillSolidRect(&rcBox, RGB(0, 0, 0));

    // Color swatch
    int swatchSize = boxH - 4;
    CRect rcSwatch(rcBox.right + 2, rcBox.top + 2, rcBox.right + 2 + swatchSize, rcBox.top + 2 + swatchSize);
    pDC->FillSolidRect(&rcSwatch, RGB(m_bPixelR, m_bPixelG, m_bPixelB));
    CPen penBorder(PS_SOLID, 1, RGB(200, 200, 200));
    CPen* pOldPen = pDC->SelectObject(&penBorder);
    CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
    pDC->Rectangle(rcSwatch);
    pDC->SelectObject(pOldPen);
    pDC->SelectObject(pOldBrush);

    // Text
    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(RGB(255, 220, 0));
    pDC->TextOut(rcBox.left + padding, rcBox.top + padding / 2, strInfo);
    pDC->SelectObject(pOldFont);
}

// ============================================================================
// Message Handlers
// ============================================================================

void CImageViewer::OnPaint()
{
    CPaintDC dc(this);

    CRect rcClient;
    GetClientRect(&rcClient);

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmpBuffer;
    bmpBuffer.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&bmpBuffer);

    memDC.FillSolidRect(&rcClient, m_clrBackground);

    if (!m_image.IsValid() || m_hBitmap == NULL)
    {
        memDC.SetBkMode(TRANSPARENT);
        memDC.SetTextColor(RGB(180, 180, 180));
        CFont font;
        font.CreatePointFont(120, _T("Segoe UI"));
        CFont* pOldFont = memDC.SelectObject(&font);
        memDC.DrawText(_T("No Image"), &rcClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        memDC.SelectObject(pOldFont);
    }
    else
    {
        CDC imgDC;
        imgDC.CreateCompatibleDC(&dc);
        HBITMAP hOldBitmap = (HBITMAP)imgDC.SelectObject(m_hBitmap);

        CRect rcImage = GetImageRect();
        int prevMode = memDC.SetStretchBltMode(HALFTONE);
        ::SetBrushOrgEx(memDC.GetSafeHdc(), 0, 0, NULL);

        memDC.StretchBlt(
            rcImage.left, rcImage.top, rcImage.Width(), rcImage.Height(),
            &imgDC, 0, 0, m_image.GetWidth(), m_image.GetHeight(), SRCCOPY);

        memDC.SetStretchBltMode(prevMode);
        imgDC.SelectObject(hOldBitmap);

        DrawROIOverlay(&memDC, rcClient);
        DrawPixelInfo(&memDC, rcClient);
    }

    dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

void CImageViewer::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (!m_image.IsValid())
    {
        CStatic::OnLButtonDown(nFlags, point);
        return;
    }

    if (m_bROIMode)
    {
        // Left-click ROI draw mode
        m_bDrawingROI   = true;
        m_bRightClickROI = false;
        m_ptROIStart    = point;
        m_ptROIEnd      = point;
        SetCapture();
    }
    else
    {
        // Check if clicking inside an existing ROI → select it
        int hitIdx = HitTestROI(point);
        if (hitIdx >= 0)
        {
            m_nSelectedROI = hitIdx;
            NotifyParentROIUpdated();
            Invalidate(FALSE);
        }
        else
        {
            // Pan
            m_bDragging    = true;
            m_ptDragStart  = point;
            m_ptPanStart   = m_ptPan;
            SetCapture();
        }
    }

    CStatic::OnLButtonDown(nFlags, point);
}

void CImageViewer::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_bDrawingROI && !m_bRightClickROI)
    {
        FinalizeROI(point);
    }
    else if (m_bDragging)
    {
        m_bDragging = false;
        ReleaseCapture();
    }

    CStatic::OnLButtonUp(nFlags, point);
}

void CImageViewer::OnRButtonDown(UINT nFlags, CPoint point)
{
    if (!m_image.IsValid()) return;

    // Right-click always starts a new ROI
    m_bDrawingROI    = true;
    m_bRightClickROI = true;
    m_ptROIStart     = point;
    m_ptROIEnd       = point;
    SetCapture();
    // Suppress context menu by not calling base
}

void CImageViewer::OnRButtonUp(UINT nFlags, CPoint point)
{
    if (m_bDrawingROI && m_bRightClickROI)
    {
        m_bRightClickROI = false;
        FinalizeROI(point);
    }
    // Don't call base → prevents context menu
}

void CImageViewer::OnMouseMove(UINT nFlags, CPoint point)
{
    // Track mouse for leave notification
    TRACKMOUSEEVENT tme = {};
    tme.cbSize    = sizeof(tme);
    tme.dwFlags   = TME_LEAVE;
    tme.hwndTrack = GetSafeHwnd();
    ::TrackMouseEvent(&tme);
    m_bMouseInWindow = true;

    if (m_bDrawingROI)
    {
        m_ptROIEnd = point;
        Invalidate(FALSE);
    }
    else if (m_bDragging)
    {
        m_ptPan.x = m_ptPanStart.x + (point.x - m_ptDragStart.x);
        m_ptPan.y = m_ptPanStart.y + (point.y - m_ptDragStart.y);
        ClampPan();
        Invalidate(FALSE);
    }

    // Update hover pixel info
    if (m_image.IsValid())
    {
        CPoint imgPt = ScreenToImage(point);
        int w = m_image.GetWidth();
        int h = m_image.GetHeight();

        if (imgPt.x >= 0 && imgPt.x < w && imgPt.y >= 0 && imgPt.y < h)
        {
            m_ptMouseImg    = imgPt;
            m_bHasPixelInfo = true;

            const BYTE* pSrc  = m_image.GetData();
            int nChannels      = m_image.GetChannels();
            int nStride        = m_image.GetStride();
            const BYTE* pPixel = pSrc + imgPt.y * nStride + imgPt.x * nChannels;

            if (nChannels == 1)
            {
                m_bPixelR = m_bPixelG = m_bPixelB = pPixel[0];
            }
            else
            {
                m_bPixelB = pPixel[0];
                m_bPixelG = pPixel[1];
                m_bPixelR = pPixel[2];
            }
        }
        else
        {
            m_bHasPixelInfo = false;
        }
        Invalidate(FALSE);
    }

    CStatic::OnMouseMove(nFlags, point);
}

void CImageViewer::OnMouseLeave()
{
    m_bMouseInWindow = false;
    m_bHasPixelInfo  = false;
    Invalidate(FALSE);
}

BOOL CImageViewer::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (!m_image.IsValid()) return CStatic::OnMouseWheel(nFlags, zDelta, pt);

    CPoint ptClient = pt;
    ScreenToClient(&ptClient);

    double imgX = (ptClient.x - m_ptPan.x) / m_dZoom;
    double imgY = (ptClient.y - m_ptPan.y) / m_dZoom;

    if (zDelta > 0) m_dZoom *= 1.25;
    else            m_dZoom /= 1.25;

    if (m_dZoom < m_dMinZoom) m_dZoom = m_dMinZoom;
    if (m_dZoom > m_dMaxZoom) m_dZoom = m_dMaxZoom;

    m_ptPan.x = (int)(ptClient.x - imgX * m_dZoom);
    m_ptPan.y = (int)(ptClient.y - imgY * m_dZoom);

    ClampPan();
    Invalidate(FALSE);
    return TRUE;
}

BOOL CImageViewer::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

BOOL CImageViewer::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (nHitTest == HTCLIENT)
    {
        if (m_bDrawingROI || m_bROIMode)
        {
            ::SetCursor(::LoadCursor(NULL, IDC_CROSS));
            return TRUE;
        }
    }
    return CStatic::OnSetCursor(pWnd, nHitTest, message);
}

void CImageViewer::OnSize(UINT nType, int cx, int cy)
{
    CStatic::OnSize(nType, cx, cy);
    if (m_image.IsValid())
        FitToWindow();
}
