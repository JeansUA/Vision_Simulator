#include "stdafx.h"
#include "UI/MiniViewer.h"
#include "Utils/CommonTypes.h"

IMPLEMENT_DYNAMIC(CMiniViewer, CStatic)

BEGIN_MESSAGE_MAP(CMiniViewer, CStatic)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

CMiniViewer::CMiniViewer()
    : m_hBitmap(NULL)
    , m_bSelected(false)
    , m_nIndex(-1)
{
}

CMiniViewer::~CMiniViewer()
{
    if (m_hBitmap != NULL)
    {
        ::DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }
}

void CMiniViewer::SetImage(const CImageBuffer& image)
{
    m_image = image.Clone();
    UpdateBitmap();
    Invalidate(FALSE);
}

void CMiniViewer::SetLabel(const CString& label)
{
    m_strLabel = label;
    Invalidate(FALSE);
}

void CMiniViewer::Clear()
{
    m_image.Release();
    if (m_hBitmap != NULL)
    {
        ::DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }
    m_strLabel.Empty();
    m_bSelected = false;
    Invalidate(FALSE);
}

void CMiniViewer::SetSelected(bool bSelected)
{
    if (m_bSelected != bSelected)
    {
        m_bSelected = bSelected;
        Invalidate(FALSE);
    }
}

void CMiniViewer::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (m_image.IsValid())
    {
        CWnd* pParent = GetParent();
        if (pParent != nullptr && ::IsWindow(pParent->GetSafeHwnd()))
        {
            pParent->PostMessage(WM_MINIVIEWER_CLICKED, (WPARAM)m_nIndex, 0);
        }
    }
    CStatic::OnLButtonDown(nFlags, point);
}

void CMiniViewer::UpdateBitmap()
{
    if (m_hBitmap != NULL)
    {
        ::DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }

    if (!m_image.IsValid())
        return;

    CRect rcClient;
    GetClientRect(&rcClient);

    int maxW = rcClient.Width();
    int maxH = rcClient.Height();

    if (maxW <= 0 || maxH <= 0)
    {
        maxW = 160;
        maxH = 120;
    }

    CImageBuffer thumbnail = m_image.CreateThumbnail(maxW, maxH);
    if (thumbnail.IsValid())
    {
        m_hBitmap = thumbnail.CreateHBitmap();
    }
}

void CMiniViewer::OnPaint()
{
    CPaintDC dc(this);

    CRect rcClient;
    GetClientRect(&rcClient);

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmpBuffer;
    bmpBuffer.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&bmpBuffer);

    memDC.FillSolidRect(&rcClient, RGB(50, 50, 50));

    if (m_image.IsValid() && m_hBitmap != NULL)
    {
        BITMAP bm;
        ::GetObject(m_hBitmap, sizeof(BITMAP), &bm);

        int imgW = bm.bmWidth;
        int imgH = bm.bmHeight;

        int labelBarHeight = 18;
        int availH = rcClient.Height() - labelBarHeight;

        double scaleX = (double)rcClient.Width() / (double)m_image.GetWidth();
        double scaleY = (double)availH / (double)m_image.GetHeight();
        double scale = min(scaleX, scaleY);

        int scaledW = (int)(m_image.GetWidth() * scale);
        int scaledH = (int)(m_image.GetHeight() * scale);
        int drawX = (rcClient.Width() - scaledW) / 2;
        int drawY = (availH - scaledH) / 2;

        CDC imgDC;
        imgDC.CreateCompatibleDC(&dc);
        HBITMAP hOldBitmap = (HBITMAP)imgDC.SelectObject(m_hBitmap);

        int prevMode = memDC.SetStretchBltMode(HALFTONE);
        ::SetBrushOrgEx(memDC.GetSafeHdc(), 0, 0, NULL);

        memDC.StretchBlt(
            drawX, drawY, scaledW, scaledH,
            &imgDC,
            0, 0, imgW, imgH,
            SRCCOPY);

        memDC.SetStretchBltMode(prevMode);
        imgDC.SelectObject(hOldBitmap);

        // Draw border (normal or selected)
        if (m_bSelected)
        {
            CPen penSelected(PS_SOLID, 3, RGB(0, 200, 255));
            CPen* pOldPen = memDC.SelectObject(&penSelected);
            CBrush* pOldBrush = (CBrush*)memDC.SelectStockObject(NULL_BRUSH);
            CRect rcSel = rcClient;
            rcSel.DeflateRect(1, 1);
            memDC.Rectangle(rcSel);
            memDC.SelectObject(pOldPen);
            memDC.SelectObject(pOldBrush);
        }
        else
        {
            CPen penBorder(PS_SOLID, 1, RGB(100, 100, 100));
            CPen* pOldPen = memDC.SelectObject(&penBorder);
            CBrush* pOldBrush = (CBrush*)memDC.SelectStockObject(NULL_BRUSH);
            memDC.Rectangle(rcClient);
            memDC.SelectObject(pOldPen);
            memDC.SelectObject(pOldBrush);
        }

        // Draw label bar at bottom
        if (!m_strLabel.IsEmpty())
        {
            CRect rcLabel = rcClient;
            rcLabel.top = rcLabel.bottom - labelBarHeight;

            memDC.FillSolidRect(&rcLabel, RGB(30, 30, 30));

            memDC.SetBkMode(TRANSPARENT);
            memDC.SetTextColor(RGB(255, 255, 255));

            CFont font;
            font.CreatePointFont(75, _T("Segoe UI"));
            CFont* pOldFont = memDC.SelectObject(&font);

            CRect rcText = rcLabel;
            rcText.left += 4;
            rcText.right -= 4;
            memDC.DrawText(m_strLabel, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

            memDC.SelectObject(pOldFont);
        }
    }
    else
    {
        CPen penDash(PS_DASH, 1, RGB(100, 100, 100));
        CPen* pOldPen = memDC.SelectObject(&penDash);
        CBrush* pOldBrush = (CBrush*)memDC.SelectStockObject(NULL_BRUSH);

        CRect rcDash = rcClient;
        rcDash.DeflateRect(4, 4);
        memDC.Rectangle(rcDash);

        memDC.SelectObject(pOldPen);
        memDC.SelectObject(pOldBrush);

        memDC.SetBkMode(TRANSPARENT);
        memDC.SetTextColor(RGB(128, 128, 128));

        CFont font;
        font.CreatePointFont(80, _T("Segoe UI"));
        CFont* pOldFont = memDC.SelectObject(&font);

        memDC.DrawText(_T("Empty"), &rcClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        memDC.SelectObject(pOldFont);
    }

    dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

BOOL CMiniViewer::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}
