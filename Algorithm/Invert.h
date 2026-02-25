#pragma once
#include "Algorithm/AlgorithmBase.h"

class CInvert : public CAlgorithmBase {
public:
    CInvert();
    virtual ~CInvert();
    virtual CString GetName() const override;
    virtual CString GetDescription() const override;
    virtual std::vector<AlgorithmParam>& GetParams() override;
    virtual bool Process(const CImageBuffer& input, CImageBuffer& output) override;
    virtual CAlgorithmBase* Clone() const override;
private:
    std::vector<AlgorithmParam> m_params;
};
