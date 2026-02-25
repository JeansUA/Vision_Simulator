#pragma once
#include "Core/ImageBuffer.h"

class CMiniViewer : public CStatic {
    DECLARE_DYNAMIC(CMiniViewer)
public:
    CMiniViewer();
    virtual ~CMiniViewer();

    void SetImage(const CImageBuffer& image);
    void SetLabel(const CString& label);
    void Clear();
    bool HasImage() const { return m_image.IsValid(); }
    const CImageBuffer& GetImage() const { return m_image; }
    const CString& GetLabel() const { return m_strLabel; }

    void SetSelected(bool bSelected);
    bool IsSelected() const { return m_bSelected; }
    void SetIndex(int nIndex) { m_nIndex = nIndex; }
    int GetIndex() const { return m_nIndex; }

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    DECLARE_MESSAGE_MAP()

private:
    CImageBuffer m_image;
    CString m_strLabel;
    HBITMAP m_hBitmap;
    bool m_bSelected;
    int m_nIndex;
    void UpdateBitmap();
};
