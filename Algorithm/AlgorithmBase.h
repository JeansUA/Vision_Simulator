#pragma once
#include "stdafx.h"
#include "Core/ImageBuffer.h"
#include <vector>

struct AlgorithmParam {
    CString strName;
    CString strDescription;
    double dMinVal;
    double dMaxVal;
    double dDefaultVal;
    double dCurrentVal;
    int nPrecision;                   // 0 for integer params
    std::vector<CString> vecOptions;  // 비어있으면 슬라이더, 있으면 콤보박스
};

class CAlgorithmBase {
public:
    virtual ~CAlgorithmBase() {}
    virtual CString GetName() const = 0;
    virtual CString GetDescription() const = 0;
    virtual std::vector<AlgorithmParam>& GetParams() = 0;
    virtual bool Process(const CImageBuffer& input, CImageBuffer& output) = 0;
    virtual CAlgorithmBase* Clone() const = 0;
};
