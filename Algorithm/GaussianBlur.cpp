#include "stdafx.h"
#include "Algorithm/GaussianBlur.h"
#include <cmath>
#include <vector>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

// method: 0=Gaussian, 1=Bilateral, 2=Median, 3=Box

CGaussianBlur::CGaussianBlur()
{
    AlgorithmParam paramMethod;
    paramMethod.strName        = _T("방식");
    paramMethod.strDescription = _T("블러 필터 방식을 선택하세요");
    paramMethod.dMinVal        = 0.0;
    paramMethod.dMaxVal        = 3.0;
    paramMethod.dDefaultVal    = 0.0;
    paramMethod.dCurrentVal    = 0.0;
    paramMethod.nPrecision     = 0;
    paramMethod.vecOptions     = { _T("가우시안"), _T("양방향(Bilateral)"),
                                    _T("미디언"), _T("박스(Box)") };
    m_params.push_back(paramMethod);

    AlgorithmParam paramKernelSize;
    paramKernelSize.strName        = _T("커널 크기");
    paramKernelSize.strDescription = _T("블러 적용 범위 (홀수값, 클수록 더 흐릿하게)");
    paramKernelSize.dMinVal        = 3.0;
    paramKernelSize.dMaxVal        = 31.0;
    paramKernelSize.dDefaultVal    = 5.0;
    paramKernelSize.dCurrentVal    = 5.0;
    paramKernelSize.nPrecision     = 0;
    m_params.push_back(paramKernelSize);

    AlgorithmParam paramSigma;
    paramSigma.strName        = _T("시그마");
    paramSigma.strDescription = _T("가우시안/양방향 필터 시그마 — 작을수록 선명하게 유지");
    paramSigma.dMinVal        = 0.1;
    paramSigma.dMaxVal        = 10.0;
    paramSigma.dDefaultVal    = 1.0;
    paramSigma.dCurrentVal    = 1.0;
    paramSigma.nPrecision     = 1;
    m_params.push_back(paramSigma);
}

CGaussianBlur::~CGaussianBlur() {}

CString CGaussianBlur::GetName() const        { return _T("Blur"); }
CString CGaussianBlur::GetDescription() const { return _T("Gaussian/Bilateral/Median/Box blur"); }
std::vector<AlgorithmParam>& CGaussianBlur::GetParams() { return m_params; }

void CGaussianBlur::GenerateGaussianKernel1D(int nKernelSize, double dSigma, std::vector<double>& kernel)
{
    kernel.resize(nKernelSize);
    int nHalf = nKernelSize / 2;
    double dSum = 0.0;
    for (int i = 0; i < nKernelSize; i++)
    {
        int x = i - nHalf;
        kernel[i] = exp(-(double)(x * x) / (2.0 * dSigma * dSigma));
        dSum += kernel[i];
    }
    if (dSum > 0.0)
        for (auto& v : kernel) v /= dSum;
}

int CGaussianBlur::ClampCoord(int val, int maxVal)
{
    if (val < 0)       return 0;
    if (val >= maxVal) return maxVal - 1;
    return val;
}

static bool ApplyGaussian(const CImageBuffer& input, CImageBuffer& output,
                           int nKernelSize, double dSigma)
{
    int nWidth    = input.GetWidth();
    int nHeight   = input.GetHeight();
    int nChannels = input.GetChannels();

    if (!output.Create(nWidth, nHeight, nChannels)) return false;

    int nHalf = nKernelSize / 2;
    std::vector<double> kernel;
    {
        kernel.resize(nKernelSize);
        int nHalf2 = nKernelSize / 2;
        double dSum = 0;
        for (int i = 0; i < nKernelSize; i++)
        {
            int x = i - nHalf2;
            kernel[i] = exp(-(double)(x * x) / (2.0 * dSigma * dSigma));
            dSum += kernel[i];
        }
        if (dSum > 0) for (auto& v : kernel) v /= dSum;
    }

    CImageBuffer temp;
    if (!temp.Create(nWidth, nHeight, nChannels)) return false;

    const BYTE* pSrc      = input.GetData();
    BYTE*       pTmp      = temp.GetData();
    BYTE*       pDst      = output.GetData();
    int         nSrcStride = input.GetStride();
    int         nTmpStride = temp.GetStride();
    int         nDstStride = output.GetStride();

    // Horizontal pass
#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        const BYTE* pSrcRow = pSrc + y * nSrcStride;
        BYTE*       pTmpRow = pTmp + y * nTmpStride;
        for (int x = 0; x < nWidth; x++)
        {
            for (int c = 0; c < nChannels; c++)
            {
                double dSum = 0.0;
                for (int k = -nHalf; k <= nHalf; k++)
                {
                    int nSrcX = max(0, min(nWidth - 1, x + k));
                    dSum += pSrcRow[nSrcX * nChannels + c] * kernel[k + nHalf];
                }
                int v = (int)(dSum + 0.5);
                pTmpRow[x * nChannels + c] = (BYTE)max(0, min(255, v));
            }
        }
    }

    // Vertical pass
#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        BYTE* pDstRow = pDst + y * nDstStride;
        for (int x = 0; x < nWidth; x++)
        {
            for (int c = 0; c < nChannels; c++)
            {
                double dSum = 0.0;
                for (int k = -nHalf; k <= nHalf; k++)
                {
                    int nSrcY = max(0, min(nHeight - 1, y + k));
                    dSum += pTmp[nSrcY * nTmpStride + x * nChannels + c] * kernel[k + nHalf];
                }
                int v = (int)(dSum + 0.5);
                pDstRow[x * nChannels + c] = (BYTE)max(0, min(255, v));
            }
        }
    }
    return true;
}

static bool ApplyBox(const CImageBuffer& input, CImageBuffer& output, int nKernelSize)
{
    int nWidth    = input.GetWidth();
    int nHeight   = input.GetHeight();
    int nChannels = input.GetChannels();
    if (!output.Create(nWidth, nHeight, nChannels)) return false;

    int nHalf = nKernelSize / 2;
    double inv = 1.0 / (nKernelSize * nKernelSize);

    const BYTE* pSrc      = input.GetData();
    BYTE*       pDst      = output.GetData();
    int         nSrcStride = input.GetStride();
    int         nDstStride = output.GetStride();

#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        BYTE* pDstRow = pDst + y * nDstStride;
        for (int x = 0; x < nWidth; x++)
        {
            for (int c = 0; c < nChannels; c++)
            {
                int sum = 0;
                for (int ky = -nHalf; ky <= nHalf; ky++)
                {
                    int sy = max(0, min(nHeight - 1, y + ky));
                    const BYTE* pRow = pSrc + sy * nSrcStride;
                    for (int kx = -nHalf; kx <= nHalf; kx++)
                    {
                        int sx = max(0, min(nWidth - 1, x + kx));
                        sum += pRow[sx * nChannels + c];
                    }
                }
                pDstRow[x * nChannels + c] = (BYTE)max(0, min(255, (int)(sum * inv + 0.5)));
            }
        }
    }
    return true;
}

static bool ApplyMedian(const CImageBuffer& input, CImageBuffer& output, int nKernelSize)
{
    int nWidth    = input.GetWidth();
    int nHeight   = input.GetHeight();
    int nChannels = input.GetChannels();
    if (!output.Create(nWidth, nHeight, nChannels)) return false;

    int nHalf   = nKernelSize / 2;
    int nPixels = nKernelSize * nKernelSize;

    const BYTE* pSrc      = input.GetData();
    BYTE*       pDst      = output.GetData();
    int         nSrcStride = input.GetStride();
    int         nDstStride = output.GetStride();

#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        std::vector<BYTE> buf(nPixels);
        BYTE* pDstRow = pDst + y * nDstStride;
        for (int x = 0; x < nWidth; x++)
        {
            for (int c = 0; c < nChannels; c++)
            {
                int n = 0;
                for (int ky = -nHalf; ky <= nHalf; ky++)
                {
                    int sy = max(0, min(nHeight - 1, y + ky));
                    const BYTE* pRow = pSrc + sy * nSrcStride;
                    for (int kx = -nHalf; kx <= nHalf; kx++)
                    {
                        int sx = max(0, min(nWidth - 1, x + kx));
                        buf[n++] = pRow[sx * nChannels + c];
                    }
                }
                std::nth_element(buf.begin(), buf.begin() + n / 2, buf.begin() + n);
                pDstRow[x * nChannels + c] = buf[n / 2];
            }
        }
    }
    return true;
}

static bool ApplyBilateral(const CImageBuffer& input, CImageBuffer& output,
                            int nKernelSize, double dSigmaS)
{
    int nWidth    = input.GetWidth();
    int nHeight   = input.GetHeight();
    int nChannels = input.GetChannels();
    if (!output.Create(nWidth, nHeight, nChannels)) return false;

    int    nHalf   = nKernelSize / 2;
    double sigmaR  = 30.0;  // range sigma (color similarity)
    double inv2SS  = 1.0 / (2.0 * dSigmaS * dSigmaS);
    double inv2SR  = 1.0 / (2.0 * sigmaR * sigmaR);

    const BYTE* pSrc      = input.GetData();
    BYTE*       pDst      = output.GetData();
    int         nSrcStride = input.GetStride();
    int         nDstStride = output.GetStride();

#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        BYTE* pDstRow = pDst + y * nDstStride;
        const BYTE* pCenterRow = pSrc + y * nSrcStride;
        for (int x = 0; x < nWidth; x++)
        {
            for (int c = 0; c < nChannels; c++)
            {
                double wSum = 0.0, vSum = 0.0;
                double centerVal = pCenterRow[x * nChannels + c];

                for (int ky = -nHalf; ky <= nHalf; ky++)
                {
                    int sy = max(0, min(nHeight - 1, y + ky));
                    const BYTE* pRow = pSrc + sy * nSrcStride;
                    for (int kx = -nHalf; kx <= nHalf; kx++)
                    {
                        int sx = max(0, min(nWidth - 1, x + kx));
                        double val  = pRow[sx * nChannels + c];
                        double diff = val - centerVal;
                        double ws   = exp(-(ky * ky + kx * kx) * inv2SS);
                        double wr   = exp(-diff * diff * inv2SR);
                        double w    = ws * wr;
                        wSum += w;
                        vSum += w * val;
                    }
                }
                int v = (wSum > 0) ? (int)(vSum / wSum + 0.5) : (int)centerVal;
                pDstRow[x * nChannels + c] = (BYTE)max(0, min(255, v));
            }
        }
    }
    return true;
}

bool CGaussianBlur::Process(const CImageBuffer& input, CImageBuffer& output)
{
    if (!input.IsValid()) return false;

    int nMethod     = (int)m_params[0].dCurrentVal;
    int nKernelSize = (int)m_params[1].dCurrentVal;
    double dSigma   = m_params[2].dCurrentVal;

    if (nKernelSize % 2 == 0) nKernelSize++;
    nKernelSize = max(3, min(31, nKernelSize));

    switch (nMethod)
    {
    case 1: return ApplyBilateral(input, output, nKernelSize, dSigma);
    case 2: return ApplyMedian(input, output, nKernelSize);
    case 3: return ApplyBox(input, output, nKernelSize);
    default: return ApplyGaussian(input, output, nKernelSize, dSigma);
    }
}

CAlgorithmBase* CGaussianBlur::Clone() const { return new CGaussianBlur(*this); }
