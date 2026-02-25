#include "stdafx.h"
#include "Core/SequenceManager.h"
#include <chrono>

CSequenceManager::CSequenceManager()
    : m_pThread(nullptr)
    , m_bRunning(false)
    , m_bStopRequested(false)
    , m_hNotifyWnd(NULL)
{
}

CSequenceManager::~CSequenceManager()
{
    StopExecution();
    ClearSteps();
}

void CSequenceManager::AddStep(CAlgorithmBase* pAlg)
{
    CSingleLock lock(&m_cs, TRUE);
    m_steps.push_back(pAlg);
}

void CSequenceManager::RemoveStep(int index)
{
    CSingleLock lock(&m_cs, TRUE);
    if (index < 0 || index >= (int)m_steps.size()) return;
    delete m_steps[index];
    m_steps.erase(m_steps.begin() + index);
}

void CSequenceManager::MoveStepUp(int index)
{
    CSingleLock lock(&m_cs, TRUE);
    if (index <= 0 || index >= (int)m_steps.size()) return;
    std::swap(m_steps[index], m_steps[index - 1]);
}

void CSequenceManager::MoveStepDown(int index)
{
    CSingleLock lock(&m_cs, TRUE);
    if (index < 0 || index >= (int)m_steps.size() - 1) return;
    std::swap(m_steps[index], m_steps[index + 1]);
}

void CSequenceManager::ClearSteps()
{
    CSingleLock lock(&m_cs, TRUE);
    for (auto* p : m_steps) delete p;
    m_steps.clear();
}

int CSequenceManager::GetStepCount() const
{
    CSingleLock lock(&m_cs, TRUE);
    return (int)m_steps.size();
}

CAlgorithmBase* CSequenceManager::GetStep(int index) const
{
    CSingleLock lock(&m_cs, TRUE);
    if (index < 0 || index >= (int)m_steps.size()) return nullptr;
    return m_steps[index];
}

void CSequenceManager::RemoveROI(int index)
{
    if (index >= 0 && index < (int)m_rcROIs.size())
        m_rcROIs.erase(m_rcROIs.begin() + index);
}

bool CSequenceManager::StartExecution(const CImageBuffer& input)
{
    if (m_bRunning) return false;
    if (!input.IsValid()) return false;

    if (m_pThread != nullptr)
    {
        ::WaitForSingleObject(m_pThread->m_hThread, 0);
        delete m_pThread;
        m_pThread = nullptr;
    }

    m_inputImage.CopyDataFrom(input);  // reuse m_inputImage allocation (0 malloc after 1st run)
    m_history.clear();
    m_history.push_back(m_inputImage.Clone());  // history[0] = original

    m_bRunning = true;
    m_bStopRequested = false;

    m_pThread = AfxBeginThread(ExecuteThread, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED, NULL);
    if (m_pThread == nullptr)
    {
        m_bRunning = false;
        return false;
    }

    m_pThread->m_bAutoDelete = FALSE;
    m_pThread->ResumeThread();
    return true;
}

void CSequenceManager::StopExecution()
{
    if (!m_bRunning && m_pThread == nullptr) return;

    m_bStopRequested = true;

    if (m_pThread != nullptr)
    {
        ::WaitForSingleObject(m_pThread->m_hThread, 5000);
        delete m_pThread;
        m_pThread = nullptr;
    }

    m_bRunning = false;
}

UINT CSequenceManager::ExecuteThread(LPVOID pParam)
{
    CSequenceManager* pMgr = reinterpret_cast<CSequenceManager*>(pParam);
    if (pMgr) pMgr->DoExecute();
    return 0;
}

void CSequenceManager::DoExecute()
{
    int stepCount = 0;
    {
        CSingleLock lock(&m_cs, TRUE);
        stepCount = (int)m_steps.size();
    }

    // Snapshot the ROI list (thread-safe copy, no lock needed during processing)
    std::vector<CRect> rois = m_rcROIs;
    bool bHasROIs = !rois.empty();

    // Ensure we have enough pre-allocated buffer sets (persistent across runs)
    if ((int)m_stepBufs.size() < stepCount)
        m_stepBufs.resize(stepCount);

    for (int i = 0; i < stepCount; i++)
    {
        if (m_bStopRequested) break;

        // Post progress
        if (m_hNotifyWnd && ::IsWindow(m_hNotifyWnd))
            ::PostMessage(m_hNotifyWnd, WM_SEQUENCE_PROGRESS, (WPARAM)(i + 1), (LPARAM)stepCount);

        // Copy input from history into pre-allocated buffer (no malloc after 1st run)
        {
            CSingleLock lock(&m_cs, TRUE);
            if (!m_history.empty())
                m_stepBufs[i].stepInput.CopyDataFrom(m_history.back());
        }

        CImageBuffer& inp = m_stepBufs[i].stepInput;
        if (!inp.IsValid())
        {
            if (m_hNotifyWnd && ::IsWindow(m_hNotifyWnd))
                ::PostMessage(m_hNotifyWnd, WM_SEQUENCE_ERROR, (WPARAM)i, 0);
            m_bRunning = false;
            return;
        }

        // Get algorithm pointer (brief lock)
        CAlgorithmBase* pStep = nullptr;
        {
            CSingleLock lock(&m_cs, TRUE);
            if (i < (int)m_steps.size()) pStep = m_steps[i];
        }

        if (!pStep)
        {
            if (m_hNotifyWnd && ::IsWindow(m_hNotifyWnd))
                ::PostMessage(m_hNotifyWnd, WM_SEQUENCE_ERROR, (WPARAM)i, 0);
            m_bRunning = false;
            return;
        }

        // Prepare ROI buffers (smart Create = no alloc if size unchanged)
        m_stepBufs[i].Prepare(inp.GetWidth(), inp.GetHeight(), inp.GetChannels(), rois);
        CImageBuffer& outRef = m_stepBufs[i].stepOutput;

        // Run algorithm WITHOUT holding mutex (allows OpenMP parallelism inside)
        bool success = false;
        auto tStepStart = std::chrono::high_resolution_clock::now();

        if (!bHasROIs)
        {
            // No ROI: process full image into pre-alloc output (smart Create inside Process)
            success = pStep->Process(inp, outRef);
        }
        else
        {
            // Composite: copy input into output, then overwrite each ROI region
            success = outRef.CopyDataFrom(inp);
            if (success)
            {
                for (int j = 0; j < (int)rois.size(); j++)
                {
                    if (m_bStopRequested) { success = false; break; }
                    if (!inp.ExtractRegionInto(rois[j], m_stepBufs[i].roiIn[j])) continue;
                    if (pStep->Process(m_stepBufs[i].roiIn[j], m_stepBufs[i].roiOut[j])
                        && m_stepBufs[i].roiOut[j].IsValid())
                    {
                        outRef.PasteRegion(m_stepBufs[i].roiOut[j], rois[j].left, rois[j].top);
                    }
                    else { success = false; break; }
                }
            }
        }

        auto tStepEnd = std::chrono::high_resolution_clock::now();
        long long elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(tStepEnd - tStepStart).count();

        if (success && outRef.IsValid())
        {
            {
                CSingleLock lock(&m_cs, TRUE);
                // Clone outRef into history (outRef stays alive for next run's reuse)
                m_history.push_back(outRef.Clone());
            }
            if (m_hNotifyWnd && ::IsWindow(m_hNotifyWnd))
                ::PostMessage(m_hNotifyWnd, WM_SEQUENCE_STEP_DONE, (WPARAM)i, (LPARAM)elapsedMs);
        }
        else
        {
            if (m_hNotifyWnd && ::IsWindow(m_hNotifyWnd))
                ::PostMessage(m_hNotifyWnd, WM_SEQUENCE_ERROR, (WPARAM)i, 0);
            m_bRunning = false;
            return;
        }
    }

    m_bRunning = false;

    if (!m_bStopRequested && m_hNotifyWnd && ::IsWindow(m_hNotifyWnd))
        ::PostMessage(m_hNotifyWnd, WM_SEQUENCE_COMPLETE, 0, 0);
}
