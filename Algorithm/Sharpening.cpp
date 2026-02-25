#include "stdafx.h"
#include "Algorithm/Sharpening.h"
#include <cmath>
#include <vector>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

CSharpening::CSharpening()
{
    AlgorithmParam paramMethod;
    paramMethod.strName        = _T("방식");
    paramMethod.strDescription = _T("선명화 방식을 선택하세요");
    paramMethod.dMinVal        = 0.0;
    paramMethod.dMaxVal        = 2.0;
    paramMethod.dDefaultVal    = 0.0;
    paramMethod.dCurrentVal    = 0.0;
    paramMethod.nPrecision     = 0;
    paramMethod.vecOptions     = { _T("언샤프 마스크"), _T("라플라시안"), _T("하이 부스트") };
    m_params.push_back(paramMethod);

    AlgorithmParam paramStrength;
    paramStrength.strName        = _T("강도");
    paramStrength.strDescription = _T("선명화 효과의 강도 (값이 클수록 더 선명하게)");
    paramStrength.dMinVal        = 0.1;
    paramStrength.dMaxVal        = 5.0;
    paramStrength.dDefaultVal    = 1.0;
    paramStrength.dCurrentVal    = 1.0;
    paramStrength.nPrecision     = 1;
    m_params.push_back(paramStrength);

    AlgorithmParam paramRadius;
    paramRadius.strName        = _T("반경");
    paramRadius.strDescription = _T("언샤프 마스크/하이 부스트의 블러 반경 (라플라시안 미사용)");
    paramRadius.dMinVal        = 1.0;
    paramRadius.dMaxVal        = 10.0;
    paramRadius.dDefaultVal    = 2.0;
    paramRadius.dCurrentVal    = 2.0;
    paramRadius.nPrecision     = 0;
    m_params.push_back(paramRadius);
}

CSharpening::~CSharpening() {}

CString CSharpening::GetName() const        { return _T("Sharpening"); }
CString CSharpening::GetDescription() const { return _T("Unsharp mask / Laplacian / High boost sharpening"); }
std::vector<AlgorithmParam>& CSharpening::GetParams() { return m_params; }

bool CSharpening::Process(const CImageBuffer& input, CImageBuffer& output)
{
    if (!input.IsValid()) return false;

    int    nWidth    = input.GetWidth();
    int    nHeight   = input.GetHeight();
    int    nChannels = input.GetChannels();
    int    nMethod   = (int)m_params[0].dCurrentVal;
    double dStrength = m_params[1].dCurrentVal;
    int    nRadius   = (int)m_params[2].dCurrentVal;

    if (!output.Create(nWidth, nHeight, nChannels)) return false;

    const BYTE* pSrc      = input.GetData();
    BYTE*       pDst      = output.GetData();
    int         nSrcStride = input.GetStride();
    int         nDstStride = output.GetStride();

    if (nMethod == 0)
    {
        // Unsharp mask: output = input + strength * (input - blurred)
        int nKernel = 2 * nRadius + 1;
        double sigma = nRadius / 2.0;
        if (sigma < 0.5) sigma = 0.5;

        // Build Gaussian kernel
        std::vector<double> kernel(nKernel);
        double sum = 0;
        for (int i = 0; i < nKernel; i++)
        {
            int x = i - nRadius;
            kernel[i] = exp(-(double)(x * x) / (2.0 * sigma * sigma));
            sum += kernel[i];
        }
        for (auto& v : kernel) v /= sum;

        // Horizontal pass
        CImageBuffer temp;
        if (!temp.Create(nWidth, nHeight, nChannels)) return false;
        BYTE* pTmp      = temp.GetData();
        int   nTmpStride = temp.GetStride();

#pragma omp parallel for schedule(static)
        for (int y = 0; y < nHeight; y++)
        {
            const BYTE* pSrcRow = pSrc + y * nSrcStride;
            BYTE*       pTmpRow = pTmp + y * nTmpStride;
            for (int x = 0; x < nWidth; x++)
                for (int c = 0; c < nChannels; c++)
                {
                    double s = 0;
                    for (int k = -nRadius; k <= nRadius; k++)
                    {
                        int sx = max(0, min(nWidth - 1, x + k));
                        s += pSrcRow[sx * nChannels + c] * kernel[k + nRadius];
                    }
                    pTmpRow[x * nChannels + c] = (BYTE)max(0, min(255, (int)(s + 0.5)));
                }
        }

        // Vertical pass + unsharp
#pragma omp parallel for schedule(static)
        for (int y = 0; y < nHeight; y++)
        {
            const BYTE* pSrcRow = pSrc + y * nSrcStride;
            BYTE*       pDstRow = pDst + y * nDstStride;
            for (int x = 0; x < nWidth; x++)
                for (int c = 0; c < nChannels; c++)
                {
                    double s = 0;
                    for (int k = -nRadius; k <= nRadius; k++)
                    {
                        int sy = max(0, min(nHeight - 1, y + k));
                        s += pTmp[sy * nTmpStride + x * nChannels + c] * kernel[k + nRadius];
                    }
                    int blurred = (int)(s + 0.5);
                    int orig    = pSrcRow[x * nChannels + c];
                    int v       = (int)(orig + dStrength * (orig - blurred) + 0.5);
                    pDstRow[x * nChannels + c] = (BYTE)max(0, min(255, v));
                }
        }
    }
    else if (nMethod == 1)
    {
        // Laplacian sharpening: output = input - strength * laplacian
        static const int lap[3][3] = {{0,-1,0},{-1,4,-1},{0,-1,0}};

#pragma omp parallel for schedule(static)
        for (int y = 0; y < nHeight; y++)
        {
            const BYTE* pSrcRow = pSrc + y * nSrcStride;
            BYTE*       pDstRow = pDst + y * nDstStride;
            for (int x = 0; x < nWidth; x++)
                for (int c = 0; c < nChannels; c++)
                {
                    int lapVal = 0;
                    for (int ky = -1; ky <= 1; ky++)
                    {
                        int sy = max(0, min(nHeight - 1, y + ky));
                        const BYTE* pRow = pSrc + sy * nSrcStride;
                        for (int kx = -1; kx <= 1; kx++)
                        {
                            int sx = max(0, min(nWidth - 1, x + kx));
                            lapVal += pRow[sx * nChannels + c] * lap[ky+1][kx+1];
                        }
                    }
                    int v = (int)(pSrcRow[x * nChannels + c] + dStrength * lapVal + 0.5);
                    pDstRow[x * nChannels + c] = (BYTE)max(0, min(255, v));
                }
        }
    }
    else
    {
        // High Boost: output = A * input - blurred  (A = 1 + strength)
        double A      = 1.0 + dStrength;
        int    nKernel = max(3, 2 * nRadius + 1);
        if (nKernel % 2 == 0) nKernel++;
        double sigma  = nRadius / 2.0;
        if (sigma < 0.5) sigma = 0.5;

        std::vector<double> kernel(nKernel);
        double sum = 0;
        int nHalf = nKernel / 2;
        for (int i = 0; i < nKernel; i++)
        {
            int x = i - nHalf;
            kernel[i] = exp(-(double)(x*x) / (2.0 * sigma * sigma));
            sum += kernel[i];
        }
        for (auto& v : kernel) v /= sum;

        CImageBuffer temp;
        if (!temp.Create(nWidth, nHeight, nChannels)) return false;
        BYTE* pTmp      = temp.GetData();
        int   nTmpStride = temp.GetStride();

#pragma omp parallel for schedule(static)
        for (int y = 0; y < nHeight; y++)
        {
            const BYTE* pSrcRow = pSrc + y * nSrcStride;
            BYTE*       pTmpRow = pTmp + y * nTmpStride;
            for (int x = 0; x < nWidth; x++)
                for (int c = 0; c < nChannels; c++)
                {
                    double s = 0;
                    for (int k = -nHalf; k <= nHalf; k++)
                    {
                        int sx = max(0, min(nWidth - 1, x + k));
                        s += pSrcRow[sx * nChannels + c] * kernel[k + nHalf];
                    }
                    pTmpRow[x * nChannels + c] = (BYTE)max(0, min(255, (int)(s + 0.5)));
                }
        }

#pragma omp parallel for schedule(static)
        for (int y = 0; y < nHeight; y++)
        {
            const BYTE* pSrcRow = pSrc + y * nSrcStride;
            BYTE*       pDstRow = pDst + y * nDstStride;
            for (int x = 0; x < nWidth; x++)
                for (int c = 0; c < nChannels; c++)
                {
                    double s = 0;
                    for (int k = -nHalf; k <= nHalf; k++)
                    {
                        int sy = max(0, min(nHeight - 1, y + k));
                        s += pTmp[sy * nTmpStride + x * nChannels + c] * kernel[k + nHalf];
                    }
                    int blurred = (int)(s + 0.5);
                    int v       = (int)(A * pSrcRow[x * nChannels + c] - blurred + 0.5);
                    pDstRow[x * nChannels + c] = (BYTE)max(0, min(255, v));
                }
        }
    }

    return true;
}

CAlgorithmBase* CSharpening::Clone() const { return new CSharpening(*this); }
