#include "stdafx.h"
#include "Algorithm/Binarize.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>

#ifdef _OPENMP
#include <omp.h>
#endif

// method: 0=Standard, 1=Reverse, 2=DoubleThresh, 3=Adaptive, 4=Otsu

CBinarize::CBinarize()
{
    AlgorithmParam paramMethod;
    paramMethod.strName        = _T("방식");
    paramMethod.strDescription = _T("이진화 방식을 선택하세요");
    paramMethod.dMinVal        = 0.0;
    paramMethod.dMaxVal        = 4.0;
    paramMethod.dDefaultVal    = 0.0;
    paramMethod.dCurrentVal    = 0.0;
    paramMethod.nPrecision     = 0;
    paramMethod.vecOptions     = { _T("표준"), _T("반전"), _T("이중 임계값"),
                                    _T("적응형"), _T("오츠(자동)") };
    m_params.push_back(paramMethod);

    AlgorithmParam paramThreshold;
    paramThreshold.strName        = _T("낮은 임계값");
    paramThreshold.strDescription = _T("픽셀을 흑/백으로 나누는 기준값 (0~255)");
    paramThreshold.dMinVal        = 0.0;
    paramThreshold.dMaxVal        = 255.0;
    paramThreshold.dDefaultVal    = 128.0;
    paramThreshold.dCurrentVal    = 128.0;
    paramThreshold.nPrecision     = 0;
    m_params.push_back(paramThreshold);

    AlgorithmParam paramThreshold2;
    paramThreshold2.strName        = _T("높은 임계값");
    paramThreshold2.strDescription = _T("이중 임계값 방식의 상한값 — 이 범위 내 픽셀만 흰색");
    paramThreshold2.dMinVal        = 0.0;
    paramThreshold2.dMaxVal        = 255.0;
    paramThreshold2.dDefaultVal    = 200.0;
    paramThreshold2.dCurrentVal    = 200.0;
    paramThreshold2.nPrecision     = 0;
    m_params.push_back(paramThreshold2);

    AlgorithmParam paramBlockSize;
    paramBlockSize.strName        = _T("블록 크기");
    paramBlockSize.strDescription = _T("적응형 방식의 지역 분석 블록 크기 (홀수, 3~99)");
    paramBlockSize.dMinVal        = 3.0;
    paramBlockSize.dMaxVal        = 99.0;
    paramBlockSize.dDefaultVal    = 11.0;
    paramBlockSize.dCurrentVal    = 11.0;
    paramBlockSize.nPrecision     = 0;
    m_params.push_back(paramBlockSize);
}

CBinarize::~CBinarize() {}

CString CBinarize::GetName() const        { return _T("Binarize"); }
CString CBinarize::GetDescription() const { return _T("Standard/Reverse/Double/Adaptive/Otsu"); }
std::vector<AlgorithmParam>& CBinarize::GetParams() { return m_params; }

// Compute grayscale value from a row pointer
static inline BYTE toGray(const BYTE* pRow, int x, int nChannels)
{
    if (nChannels == 1) return pRow[x];
    BYTE b = pRow[x * nChannels], g = pRow[x * nChannels + 1], r = pRow[x * nChannels + 2];
    return (BYTE)max(0, min(255, (int)(0.299 * r + 0.587 * g + 0.114 * b + 0.5)));
}

// Compute Otsu threshold from histogram
static int ComputeOtsu(const int hist[256], int total)
{
    double sum = 0;
    for (int i = 0; i < 256; i++) sum += i * hist[i];

    double sumB = 0, wB = 0, wF;
    double varMax = 0;
    int threshold = 0;

    for (int t = 0; t < 256; t++)
    {
        wB += hist[t];
        if (wB == 0) continue;
        wF = total - wB;
        if (wF == 0) break;

        sumB += t * hist[t];
        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;
        double varBetween = wB * wF * (mB - mF) * (mB - mF);

        if (varBetween > varMax)
        {
            varMax = varBetween;
            threshold = t;
        }
    }
    return threshold;
}

bool CBinarize::Process(const CImageBuffer& input, CImageBuffer& output)
{
    if (!input.IsValid()) return false;

    int nWidth    = input.GetWidth();
    int nHeight   = input.GetHeight();
    int nChannels = input.GetChannels();

    int nMethod     = (int)m_params[0].dCurrentVal;
    int nThreshold  = (int)m_params[1].dCurrentVal;
    int nThreshold2 = (int)m_params[2].dCurrentVal;
    int nBlockSize  = (int)m_params[3].dCurrentVal;

    if (nBlockSize % 2 == 0) nBlockSize++;
    nBlockSize = max(3, min(99, nBlockSize));
    if (nThreshold2 < nThreshold) nThreshold2 = nThreshold;

    if (!output.Create(nWidth, nHeight, 1)) return false;

    const BYTE* pSrc      = input.GetData();
    BYTE*       pDst      = output.GetData();
    int         nSrcStride = input.GetStride();
    int         nDstStride = output.GetStride();

    // Build grayscale buffer (needed for Otsu and Adaptive)
    std::vector<BYTE> grayBuf(nWidth * nHeight);
#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        const BYTE* pRow = pSrc + y * nSrcStride;
        for (int x = 0; x < nWidth; x++)
            grayBuf[y * nWidth + x] = toGray(pRow, x, nChannels);
    }

    // Compute Otsu threshold if needed
    if (nMethod == 4)
    {
        int hist[256] = {};
        for (int i = 0; i < nWidth * nHeight; i++)
            hist[grayBuf[i]]++;
        nThreshold = ComputeOtsu(hist, nWidth * nHeight);
    }

    // Adaptive: compute local mean thresholds
    std::vector<int> localThresh;
    if (nMethod == 3)
    {
        localThresh.resize(nWidth * nHeight);
        int nHalf = nBlockSize / 2;
        int C     = nThreshold / 8;  // constant subtracted from mean

#pragma omp parallel for schedule(static)
        for (int y = 0; y < nHeight; y++)
        {
            for (int x = 0; x < nWidth; x++)
            {
                long long sum = 0;
                int cnt = 0;
                for (int ky = -nHalf; ky <= nHalf; ky++)
                {
                    int sy = max(0, min(nHeight - 1, y + ky));
                    for (int kx = -nHalf; kx <= nHalf; kx++)
                    {
                        int sx = max(0, min(nWidth - 1, x + kx));
                        sum += grayBuf[sy * nWidth + sx];
                        cnt++;
                    }
                }
                localThresh[y * nWidth + x] = (int)(sum / cnt) - C;
            }
        }
    }

    // Apply thresholding
#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        BYTE* pDstRow = pDst + y * nDstStride;
        for (int x = 0; x < nWidth; x++)
        {
            int gray = grayBuf[y * nWidth + x];
            BYTE bOut = 0;

            switch (nMethod)
            {
            case 0: // Standard
                bOut = (gray > nThreshold) ? 255 : 0;
                break;
            case 1: // Reverse
                bOut = (gray > nThreshold) ? 0 : 255;
                break;
            case 2: // Double threshold
                bOut = (gray >= nThreshold && gray <= nThreshold2) ? 255 : 0;
                break;
            case 3: // Adaptive
                bOut = (gray > localThresh[y * nWidth + x]) ? 255 : 0;
                break;
            case 4: // Otsu (threshold already computed)
                bOut = (gray > nThreshold) ? 255 : 0;
                break;
            default:
                bOut = (gray > nThreshold) ? 255 : 0;
                break;
            }
            pDstRow[x] = bOut;
        }
    }

    return true;
}

CAlgorithmBase* CBinarize::Clone() const { return new CBinarize(*this); }
