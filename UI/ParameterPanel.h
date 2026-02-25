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
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    DECLARE_MESSAGE_MAP()

private:
    CAlgorithmBase* m_pAlgorithm;  // not owned

    struct ParamControl {
        CStatic* pLabel;
        CSliderCtrl* pSlider;
        CEdit* pEdit;
        int paramIndex;
    };
    std::vector<ParamControl> m_controls;

    void CreateParamControls();
    void DestroyParamControls();
    void LayoutControls();
    void UpdateEditFromSlider(int index);

    static const int PARAM_ROW_HEIGHT = 30;
    static const int LABEL_WIDTH_RATIO = 35; // percentage
};
