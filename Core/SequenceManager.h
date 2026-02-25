#pragma once
#include "stdafx.h"
#include "Core/ImageBuffer.h"
#include "Algorithm/AlgorithmBase.h"
#include "Utils/CommonTypes.h"
#include <vector>

class CSequenceManager {
public:
    CSequenceManager();
    ~CSequenceManager();

    void SetNotifyWnd(HWND hWnd) { m_hNotifyWnd = hWnd; }

    // Step management
    void AddStep(CAlgorithmBase* pAlg);  // takes ownership
    void RemoveStep(int index);
    void MoveStepUp(int index);
    void MoveStepDown(int index);
    void ClearSteps();
    int GetStepCount() const;
    CAlgorithmBase* GetStep(int index) const;

    // Execution
    bool StartExecution(const CImageBuffer& input);
    void StopExecution();
    bool IsRunning() const { return m_bRunning; }

    // Results
    const std::vector<CImageBuffer>& GetHistory() const { return m_history; }

    // Multi-ROI support
    void SetROIs(const std::vector<CRect>& rois) { m_rcROIs = rois; }
    void AddROI(const CRect& rc) { m_rcROIs.push_back(rc); }
    void RemoveROI(int index);
    void ClearROIs() { m_rcROIs.clear(); }
    bool HasROIs() const { return !m_rcROIs.empty(); }
    const std::vector<CRect>& GetROIs() const { return m_rcROIs; }
    int GetROICount() const { return (int)m_rcROIs.size(); }

    // Legacy single-ROI compatibility
    void SetROI(const CRect& rc) { m_rcROIs.clear(); if (!rc.IsRectEmpty()) m_rcROIs.push_back(rc); }
    bool HasROI() const { return !m_rcROIs.empty(); }
    CRect GetROI() const { return m_rcROIs.empty() ? CRect(0,0,0,0) : m_rcROIs[0]; }
    void ClearROI() { m_rcROIs.clear(); }

private:
    static UINT ExecuteThread(LPVOID pParam);
    void DoExecute();

    std::vector<CAlgorithmBase*> m_steps;
    std::vector<CImageBuffer>    m_history;
    std::vector<CRect>           m_rcROIs;
    CImageBuffer                 m_inputImage;
    CWinThread*                  m_pThread;
    volatile bool                m_bRunning;
    volatile bool                m_bStopRequested;
    HWND                         m_hNotifyWnd;
    mutable CCriticalSection     m_cs;
};
