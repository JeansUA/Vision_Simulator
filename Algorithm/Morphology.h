#pragma once
#include "Algorithm/AlgorithmBase.h"

class CMorphology : public CAlgorithmBase {
public:
    CMorphology();
    virtual ~CMorphology();

    virtual CString GetName() const override;
    virtual CString GetDescription() const override;
    virtual std::vector<AlgorithmParam>& GetParams() override;
    virtual bool Process(const CImageBuffer& input, CImageBuffer& output) override;
    virtual CAlgorithmBase* Clone() const override;

private:
    std::vector<AlgorithmParam> m_params;

    int ClampCoord(int val, int maxVal);
    bool Erode(const CImageBuffer& input, CImageBuffer& output, int nKernelSize);
    bool Dilate(const CImageBuffer& input, CImageBuffer& output, int nKernelSize);
    bool ConvertToGrayscale(const CImageBuffer& input, CImageBuffer& output);
};
