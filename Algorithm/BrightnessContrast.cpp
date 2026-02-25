#include "stdafx.h"
#include "Algorithm/BrightnessContrast.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>

#ifdef _OPENMP
#include <omp.h>
#endif

// method: 0=Brightness/Contrast, 1=Gamma, 2=HistogramEqualize

CBrightnessContrast::CBrightnessContrast()
{
    AlgorithmParam paramMethod;
    paramMethod.strName        = _T("방식");
    paramMethod.strDescription = _T("밝기/대비 처리 방식을 선택하세요");
    paramMethod.dMinVal        = 0.0;
    paramMethod.dMaxVal        = 2.0;
    paramMethod.dDefaultVal    = 0.0;
    paramMethod.dCurrentVal    = 0.0;
    paramMethod.nPrecision     = 0;
    paramMethod.vecOptions     = { _T("밝기/대비"), _T("감마 보정"), _T("히스토그램 균등화") };
    m_params.push_back(paramMethod);

    AlgorithmParam paramBrightness;
    paramBrightness.strName        = _T("밝기");
    paramBrightness.strDescription = _T("전체 밝기 조절 (-100: 어둡게, 0: 변화 없음, +100: 밝게)");
    paramBrightness.dMinVal        = -100.0;
    paramBrightness.dMaxVal        = 100.0;
    paramBrightness.dDefaultVal    = 0.0;
    paramBrightness.dCurrentVal    = 0.0;
    paramBrightness.nPrecision     = 0;
    m_params.push_back(paramBrightness);

    AlgorithmParam paramContrast;
    paramContrast.strName        = _T("대비");
    paramContrast.strDescription = _T("명암 차이 강도 (-100: 낮은 대비, 0: 변화 없음, +100: 높은 대비)");
    paramContrast.dMinVal        = -100.0;
    paramContrast.dMaxVal        = 100.0;
    paramContrast.dDefaultVal    = 0.0;
    paramContrast.dCurrentVal    = 0.0;
    paramContrast.nPrecision     = 0;
    m_params.push_back(paramContrast);

    AlgorithmParam paramGamma;
    paramGamma.strName        = _T("감마값");
    paramGamma.strDescription = _T("감마 보정값 (1.0=변화 없음, 1보다 작으면 어둡게, 크면 밝게)");
    paramGamma.dMinVal        = 0.1;
    paramGamma.dMaxVal        = 5.0;
    paramGamma.dDefaultVal    = 1.0;
    paramGamma.dCurrentVal    = 1.0;
    paramGamma.nPrecision     = 1;
    m_params.push_back(paramGamma);
}

CBrightnessContrast::~CBrightnessContrast() {}

CString CBrightnessContrast::GetName() const        { return _T("Brightness/Contrast"); }
CString CBrightnessContrast::GetDescription() const { return _T("BC / Gamma correction / Histogram equalization"); }
std::vector<AlgorithmParam>& CBrightnessContrast::GetParams() { return m_params; }

bool CBrightnessContrast::Process(const CImageBuffer& input, CImageBuffer& output)
{
    if (!input.IsValid()) return false;

    int    nWidth    = input.GetWidth();
    int    nHeight   = input.GetHeight();
    int    nChannels = input.GetChannels();
    int    nMethod   = (int)m_params[0].dCurrentVal;
    double dBright   = m_params[1].dCurrentVal;
    double dContrast = m_params[2].dCurrentVal;
    double dGamma    = m_params[3].dCurrentVal;

    if (!output.Create(nWidth, nHeight, nChannels)) return false;

    const BYTE* pSrc      = input.GetData();
    BYTE*       pDst      = output.GetData();
    int         nSrcStride = input.GetStride();
    int         nDstStride = output.GetStride();

    // Build lookup table
    BYTE lut[256];

    if (nMethod == 1)
    {
        // Gamma correction
        if (dGamma < 0.01) dGamma = 0.01;
        double invGamma = 1.0 / dGamma;
        for (int i = 0; i < 256; i++)
        {
            int v = (int)(pow(i / 255.0, invGamma) * 255.0 + 0.5);
            lut[i] = (BYTE)max(0, min(255, v));
        }
    }
    else if (nMethod == 2)
    {
        // Histogram equalization - build per-channel CDF
        // Process channel by channel
        for (int c = 0; c < nChannels; c++)
        {
            int hist[256] = {};
            for (int y = 0; y < nHeight; y++)
            {
                const BYTE* pRow = pSrc + y * nSrcStride;
                for (int x = 0; x < nWidth; x++)
                    hist[pRow[x * nChannels + c]]++;
            }

            // CDF
            int cdf[256] = {};
            cdf[0] = hist[0];
            for (int i = 1; i < 256; i++) cdf[i] = cdf[i-1] + hist[i];

            // Find first non-zero
            int cdfMin = 0;
            for (int i = 0; i < 256; i++) { if (cdf[i] > 0) { cdfMin = cdf[i]; break; } }

            int total = nWidth * nHeight;
            BYTE lutCh[256];
            for (int i = 0; i < 256; i++)
            {
                int v = (int)((double)(cdf[i] - cdfMin) / (total - cdfMin) * 255.0 + 0.5);
                lutCh[i] = (BYTE)max(0, min(255, v));
            }

#pragma omp parallel for schedule(static)
            for (int y = 0; y < nHeight; y++)
            {
                const BYTE* pSrcRow = pSrc + y * nSrcStride;
                BYTE*       pDstRow = pDst + y * nDstStride;
                for (int x = 0; x < nWidth; x++)
                    pDstRow[x * nChannels + c] = lutCh[pSrcRow[x * nChannels + c]];
            }
        }
        return true;
    }
    else
    {
        // Standard brightness + contrast
        double contrastFactor = (100.0 + dContrast) / 100.0;
        for (int i = 0; i < 256; i++)
        {
            double v = 128.0 + contrastFactor * ((double)i - 128.0) + dBright;
            lut[i] = (BYTE)max(0, min(255, (int)(v + 0.5)));
        }
    }

    // Apply LUT
#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        const BYTE* pSrcRow = pSrc + y * nSrcStride;
        BYTE*       pDstRow = pDst + y * nDstStride;
        for (int x = 0; x < nWidth; x++)
            for (int c = 0; c < nChannels; c++)
                pDstRow[x * nChannels + c] = lut[pSrcRow[x * nChannels + c]];
    }

    return true;
}

CAlgorithmBase* CBrightnessContrast::Clone() const { return new CBrightnessContrast(*this); }
