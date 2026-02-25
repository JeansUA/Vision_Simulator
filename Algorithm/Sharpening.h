#pragma once
#include "Algorithm/AlgorithmBase.h"

class CSharpening : public CAlgorithmBase {
public:
    CSharpening();
    virtual ~CSharpening();
    virtual CString GetName() const override;
    virtual CString GetDescription() const override;
    virtual std::vector<AlgorithmParam>& GetParams() override;
    virtual bool Process(const CImageBuffer& input, CImageBuffer& output) override;
    virtual CAlgorithmBase* Clone() const override;
    // method: 0=UnsharpMask, 1=LaplacianSharpen, 2=HighBoost
private:
    std::vector<AlgorithmParam> m_params;
};
