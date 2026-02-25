#pragma once
#include "Core/ImageBuffer.h"
#include <vector>

class CImageViewer : public CStatic {
    DECLARE_DYNAMIC(CImageViewer)
public:
    CImageViewer();
    virtual ~CImageViewer();

    void SetImage(const CImageBuffer& image);
    void ClearImage();
    void FitToWindow();
    void ZoomIn();
    void ZoomOut();
    void SetZoom(double zoom);
    double GetZoom() const { return m_dZoom; }
    bool HasImage() const { return m_image.IsValid(); }

    // Multi-ROI management
    void AddROI(const CRect& rcROI);
    void RemoveROI(int index);
    void ClearROIs();
    bool HasROIs() const { return !m_rcROIs.empty(); }
    int GetROICount() const { return (int)m_rcROIs.size(); }
    const std::vector<CRect>& GetROIs() const { return m_rcROIs; }
    CRect GetROI(int index) const;
    void SetSelectedROI(int index);
    int GetSelectedROI() const { return m_nSelectedROI; }

    // Legacy single-ROI (for compat)
    void SetROI(const CRect& rcROI);
    bool HasROI() const { return !m_rcROIs.empty(); }
    void ClearROI() { ClearROIs(); }

    // ROI draw mode (left-click draw mode)
    void SetROIMode(bool bEnable);
    bool IsROIMode() const { return m_bROIMode; }

protected:
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    DECLARE_MESSAGE_MAP()

private:
    CImageBuffer m_image;
    HBITMAP      m_hBitmap;
    double       m_dZoom;
    double       m_dMinZoom;
    double       m_dMaxZoom;
    CPoint       m_ptPan;
    CPoint       m_ptDragStart;
    CPoint       m_ptPanStart;
    bool         m_bDragging;
    COLORREF     m_clrBackground;

    // Multi-ROI state
    std::vector<CRect> m_rcROIs;       // all ROIs in image-space coordinates
    int          m_nSelectedROI;       // index of selected ROI (-1 = none)

    // Drawing new ROI state (shared by left/right click ROI drawing)
    bool         m_bROIMode;           // left-click draws ROI (button toggle)
    bool         m_bDrawingROI;        // currently drawing a new ROI
    bool         m_bRightClickROI;     // current drawing is from right-click
    CPoint       m_ptROIStart;         // screen-space drag start
    CPoint       m_ptROIEnd;           // screen-space drag end

    // Hover pixel info
    bool         m_bMouseInWindow;
    bool         m_bHasPixelInfo;
    CPoint       m_ptMouseImg;         // image-space coordinates
    BYTE         m_bPixelR, m_bPixelG, m_bPixelB;

    void UpdateBitmap();
    void ClampPan();
    CRect GetImageRect();

    // Coordinate helpers
    CPoint ScreenToImage(CPoint ptScreen);
    CPoint ImageToScreen(CPoint ptImage);
    CRect ImageRectToScreen(const CRect& rcImage);

    // Drawing
    void DrawROIOverlay(CDC* pDC, const CRect& rcClient);
    void DrawPixelInfo(CDC* pDC, const CRect& rcClient);

    // ROI hit-test
    int HitTestROI(CPoint ptScreen) const;  // returns ROI index or -1
    void FinalizeROI(CPoint ptEnd);          // common finalize for left/right click
    void NotifyParentROIUpdated();
};
