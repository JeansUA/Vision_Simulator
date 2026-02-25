#pragma once
#include "Algorithm/AlgorithmBase.h"

class CGrayscale : public CAlgorithmBase {
public:
    CGrayscale();
    virtual ~CGrayscale();

    virtual CString GetName() const override;
    virtual CString GetDescription() const override;
    virtual std::vector<AlgorithmParam>& GetParams() override;
    virtual bool Process(const CImageBuffer& input, CImageBuffer& output) override;
    virtual CAlgorithmBase* Clone() const override;

    // Channel modes:
    // 0 = Luminance (0.299R + 0.587G + 0.114B)
    // 1 = R channel
    // 2 = G channel
    // 3 = B channel
    // 4 = H (Hue)
    // 5 = S (Saturation)
    // 6 = V (Value/Brightness)

private:
    std::vector<AlgorithmParam> m_params;

    static void RGBtoHSV(BYTE r, BYTE g, BYTE b, BYTE& h, BYTE& s, BYTE& v);
};
