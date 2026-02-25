#pragma once
#include "Algorithm/AlgorithmBase.h"

class CGaussianBlur : public CAlgorithmBase {
public:
    CGaussianBlur();
    virtual ~CGaussianBlur();

    virtual CString GetName() const override;
    virtual CString GetDescription() const override;
    virtual std::vector<AlgorithmParam>& GetParams() override;
    virtual bool Process(const CImageBuffer& input, CImageBuffer& output) override;
    virtual CAlgorithmBase* Clone() const override;

private:
    std::vector<AlgorithmParam> m_params;

    void GenerateGaussianKernel1D(int nKernelSize, double dSigma, std::vector<double>& kernel);
    int ClampCoord(int val, int maxVal);
};
