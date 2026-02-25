#pragma once
#include "resource.h"
#include "UI/ImageViewer.h"
#include "UI/MiniViewer.h"
#include "UI/ParameterPanel.h"
#include "Core/ImageBuffer.h"
#include "Core/SequenceManager.h"
#include "Algorithm/AlgorithmManager.h"
#include "Utils/CommonTypes.h"

class CVisionSimulatorDlg : public CDialogEx {
    DECLARE_DYNAMIC(CVisionSimulatorDlg)
public:
    CVisionSimulatorDlg(CWnd* pParent = nullptr);
    virtual ~CVisionSimulatorDlg();

    enum { IDD = IDD_VISIONSIMULATOR_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    // Toolbar button handlers
    afx_msg void OnBnClickedLoad();
    afx_msg void OnBnClickedRun();
    afx_msg void OnBnClickedStop();
    afx_msg void OnBnClickedClear();
    afx_msg void OnBnClickedSave();
    afx_msg void OnBnClickedFit();
    afx_msg void OnBnClickedAddSeq();
    afx_msg void OnBnClickedRemoveSeq();
    afx_msg void OnBnClickedSeqUp();
    afx_msg void OnBnClickedSeqDown();
    afx_msg void OnBnClickedClearSeq();
    afx_msg void OnBnClickedROI();
    afx_msg void OnBnClickedClearROI();
    afx_msg void OnBnClickedRemoveROI();
    afx_msg void OnBnClickedClearROIs();

    // Combo/List handlers
    afx_msg void OnCbnSelchangeAlgorithm();
    afx_msg void OnLbnSelchangeSequence();
    afx_msg void OnLbnSelchangeROI();

    // Custom messages
    afx_msg LRESULT OnSequenceProgress(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSequenceComplete(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSequenceError(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSequenceStepDone(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnMiniViewerClicked(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnROIUpdated(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnParamChanged(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    HICON m_hIcon;

    // Main viewer + history
    CImageViewer m_mainViewer;
    CMiniViewer  m_miniViewers[8];
    CStatic      m_miniLabels[8];

    // Algorithm controls
    CParameterPanel m_paramPanel;
    CComboBox       m_comboAlgorithm;
    CButton         m_btnAddSeq;

    // Sequence controls
    CListBox m_listSequence;
    CButton  m_btnRemoveSeq, m_btnSeqUp, m_btnSeqDown, m_btnClearSeq;

    // ROI list controls
    CListBox m_listROI;
    CButton  m_btnRemoveROI, m_btnClearROIs;

    // Toolbar buttons
    CButton m_btnLoad, m_btnRun, m_btnStop, m_btnClear, m_btnSave, m_btnFit;
    CButton m_btnROI, m_btnClearROI;

    // Status / zoom
    CStatic m_staticStatus;
    CStatic m_staticZoom;

    // Group boxes
    CStatic m_grpAlgorithm, m_grpSequence, m_grpROI, m_grpHistory;

    // Core
    CImageBuffer    m_originalImage;
    CSequenceManager m_sequenceManager;

    // Current algorithm for editing (new, from combo)
    CAlgorithmBase* m_pCurrentAlgorithm;

    // Sequence editing state
    int m_nEditingSequenceStep;  // -1=editing new alg, >=0=editing sequence step

    // Mini viewer state
    int m_nSelectedMiniViewer;

    // Layout helpers
    void LayoutControls();
    void CreateControls();
    void InitAlgorithmCombo();
    void UpdateSequenceList();
    void UpdateROIList();
    void SetStatus(const CString& text);
    void UpdateMiniViewers();
    void ClearMiniViewers();

    // Real-time preview
    void RunPreview();

    CFont m_fontUI;
    CFont m_fontTitle;
    bool  m_bInitialized;
    int   m_nMinWidth;
    int   m_nMinHeight;
    bool  m_bPreviewPending;
};
