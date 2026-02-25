#pragma once
#include "Algorithm/AlgorithmBase.h"

class CEdgeDetect : public CAlgorithmBase {
public:
    CEdgeDetect();
    virtual ~CEdgeDetect();

    virtual CString GetName() const override;
    virtual CString GetDescription() const override;
    virtual std::vector<AlgorithmParam>& GetParams() override;
    virtual bool Process(const CImageBuffer& input, CImageBuffer& output) override;
    virtual CAlgorithmBase* Clone() const override;

private:
    std::vector<AlgorithmParam> m_params;

    int ClampCoord(int val, int maxVal);
    BYTE ComputeGrayscale(const BYTE* pRow, int x, int nChannels);
};
