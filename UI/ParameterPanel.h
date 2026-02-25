#pragma once
#include "Algorithm/AlgorithmBase.h"

class CParameterPanel : public CWnd {
    DECLARE_DYNAMIC(CParameterPanel)
public:
    CParameterPanel();
    virtual ~CParameterPanel();

    BOOL Create(DWORD dwStyle, const CRect& rect, CWnd* pParentWnd, UINT nID);
    void SetAlgorithm(CAlgorithmBase* pAlgorithm);
    void UpdateFromAlgorithm();
    void Clear();
    CAlgorithmBase* GetAlgorithm() const { return m_pAlgorithm; }

protected:
    afx_msg void OnPaint();
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnComboSelChange(UINT nID);
    DECLARE_MESSAGE_MAP()

private:
    CAlgorithmBase* m_pAlgorithm;  // not owned

    struct ParamControl {
        CStatic*     pLabel;
        CStatic*     pDesc;     // Korean description text
        CSliderCtrl* pSlider;   // nullptr if combo param
        CEdit*       pEdit;     // nullptr if combo param
        CComboBox*   pCombo;    // nullptr if slider param
        int          paramIndex;
        bool         bVisible;
    };
    std::vector<ParamControl> m_controls;

    void CreateParamControls();
    void DestroyParamControls();
    void LayoutControls();
    void UpdateEditFromSlider(int index);
    void UpdateParamVisibility();
    void NotifyParent();
    int  GetTotalContentHeight() const;

    static const int LABEL_WIDTH_RATIO = 35; // percentage
};
