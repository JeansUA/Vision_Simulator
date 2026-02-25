#pragma once
#include "stdafx.h"
#include "Core/ImageBuffer.h"
#include "Algorithm/AlgorithmBase.h"
#include "Utils/CommonTypes.h"
#include <vector>

// Pre-allocated buffer set for one pipeline step.
// Buffers are kept alive between runs so OS pages stay warm (eliminates page faults).
struct PipelineBuffers
{
    CImageBuffer              stepInput;   // copy of previous step's output
    CImageBuffer              stepOutput;  // this step's result
    std::vector<CImageBuffer> roiIn;       // per-ROI extracted inputs
    std::vector<CImageBuffer> roiOut;      // per-ROI algorithm outputs

    // Ensure all buffers are at least the right size; reallocates only on dimension change.
    void Prepare(int w, int h, int ch, const std::vector<CRect>& rois)
    {
        stepInput.Create(w, h, ch);
        stepOutput.Create(w, h, ch);
        if (roiIn.size()  != rois.size()) roiIn.resize(rois.size());
        if (roiOut.size() != rois.size()) roiOut.resize(rois.size());
        for (int j = 0; j < (int)rois.size(); j++)
        {
            roiIn[j].Create(rois[j].Width(), rois[j].Height(), ch);
            roiOut[j].Create(rois[j].Width(), rois[j].Height(), ch);
        }
    }
};

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
    std::vector<PipelineBuffers> m_stepBufs;   // persistent per-step buffer sets
    CWinThread*                  m_pThread;
    volatile bool                m_bRunning;
    volatile bool                m_bStopRequested;
    HWND                         m_hNotifyWnd;
    mutable CCriticalSection     m_cs;
};
