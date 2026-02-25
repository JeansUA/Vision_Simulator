#pragma once
#include "Algorithm/AlgorithmBase.h"

class CHoughLine : public CAlgorithmBase {
public:
    CHoughLine();
    virtual ~CHoughLine();
    virtual CString GetName() const override;
    virtual CString GetDescription() const override;
    virtual std::vector<AlgorithmParam>& GetParams() override;
    virtual bool Process(const CImageBuffer& input, CImageBuffer& output) override;
    virtual CAlgorithmBase* Clone() const override;
    // method: 0=Standard Hough (infinite lines), 1=Probabilistic Hough (segments)
private:
    std::vector<AlgorithmParam> m_params;
};
