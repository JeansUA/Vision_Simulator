#pragma once
#include "Algorithm/AlgorithmBase.h"

class CBinarize : public CAlgorithmBase {
public:
    CBinarize();
    virtual ~CBinarize();

    virtual CString GetName() const override;
    virtual CString GetDescription() const override;
    virtual std::vector<AlgorithmParam>& GetParams() override;
    virtual bool Process(const CImageBuffer& input, CImageBuffer& output) override;
    virtual CAlgorithmBase* Clone() const override;

private:
    std::vector<AlgorithmParam> m_params;
    std::vector<BYTE>           m_grayBuf;    // persists between calls (avoids page faults)
    std::vector<int>            m_localThresh; // persists between calls (adaptive mode)
};
