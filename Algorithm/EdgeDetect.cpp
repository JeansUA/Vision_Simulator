#include "stdafx.h"
#include "Algorithm/EdgeDetect.h"
#include <cmath>
#include <vector>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// method: 0=Sobel, 1=Prewitt, 2=Canny, 3=Laplacian

CEdgeDetect::CEdgeDetect()
{
    AlgorithmParam paramMethod;
    paramMethod.strName        = _T("방식");
    paramMethod.strDescription = _T("엣지 검출 방식을 선택하세요");
    paramMethod.dMinVal        = 0.0;
    paramMethod.dMaxVal        = 3.0;
    paramMethod.dDefaultVal    = 0.0;
    paramMethod.dCurrentVal    = 0.0;
    paramMethod.nPrecision     = 0;
    paramMethod.vecOptions     = { _T("소벨(Sobel)"), _T("프리윗(Prewitt)"),
                                    _T("캐니(Canny)"), _T("라플라시안(Laplacian)") };
    m_params.push_back(paramMethod);

    AlgorithmParam paramThreshold;
    paramThreshold.strName        = _T("낮은 임계값");
    paramThreshold.strDescription = _T("엣지 검출 하한 임계값 (캐니: 낮은 임계값, 나머지: 단일 기준)");
    paramThreshold.dMinVal        = 0.0;
    paramThreshold.dMaxVal        = 255.0;
    paramThreshold.dDefaultVal    = 50.0;
    paramThreshold.dCurrentVal    = 50.0;
    paramThreshold.nPrecision     = 0;
    m_params.push_back(paramThreshold);

    AlgorithmParam paramHighThresh;
    paramHighThresh.strName        = _T("높은 임계값");
    paramHighThresh.strDescription = _T("캐니 방식 전용 상한 임계값 — 이 이상이면 강한 엣지로 확정");
    paramHighThresh.dMinVal        = 0.0;
    paramHighThresh.dMaxVal        = 255.0;
    paramHighThresh.dDefaultVal    = 150.0;
    paramHighThresh.dCurrentVal    = 150.0;
    paramHighThresh.nPrecision     = 0;
    m_params.push_back(paramHighThresh);
}

CEdgeDetect::~CEdgeDetect() {}

CString CEdgeDetect::GetName() const        { return _T("Edge Detect"); }
CString CEdgeDetect::GetDescription() const { return _T("Sobel/Prewitt/Canny/Laplacian edge detection"); }
std::vector<AlgorithmParam>& CEdgeDetect::GetParams() { return m_params; }

int CEdgeDetect::ClampCoord(int val, int maxVal)
{
    if (val < 0)       return 0;
    if (val >= maxVal) return maxVal - 1;
    return val;
}

BYTE CEdgeDetect::ComputeGrayscale(const BYTE* pRow, int x, int nChannels)
{
    if (nChannels == 1) return pRow[x];
    BYTE b = pRow[x * nChannels], g = pRow[x * nChannels + 1], r = pRow[x * nChannels + 2];
    return (BYTE)max(0, min(255, (int)(0.299 * r + 0.587 * g + 0.114 * b + 0.5)));
}

static void BuildGrayBuf(const CImageBuffer& input, std::vector<BYTE>& grayBuf)
{
    int nWidth    = input.GetWidth();
    int nHeight   = input.GetHeight();
    int nChannels = input.GetChannels();
    int nStride   = input.GetStride();
    const BYTE* pSrc = input.GetData();
    grayBuf.resize(nWidth * nHeight);

#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        const BYTE* pRow = pSrc + y * nStride;
        for (int x = 0; x < nWidth; x++)
        {
            if (nChannels == 1)
                grayBuf[y * nWidth + x] = pRow[x];
            else
            {
                int g = (int)(0.299 * pRow[x * nChannels + 2]
                            + 0.587 * pRow[x * nChannels + 1]
                            + 0.114 * pRow[x * nChannels] + 0.5);
                grayBuf[y * nWidth + x] = (BYTE)max(0, min(255, g));
            }
        }
    }
}

static bool ApplySobelPrewitt(const CImageBuffer& input, CImageBuffer& output,
                               int nMethod, int nThreshold)
{
    int nWidth  = input.GetWidth();
    int nHeight = input.GetHeight();

    std::vector<BYTE> grayBuf;
    BuildGrayBuf(input, grayBuf);

    if (!output.Create(nWidth, nHeight, 1)) return false;
    BYTE* pDst      = output.GetData();
    int   nDstStride = output.GetStride();

    static const int sobelGx[3][3]   = {{-1,0,1},{-2,0,2},{-1,0,1}};
    static const int sobelGy[3][3]   = {{-1,-2,-1},{0,0,0},{1,2,1}};
    static const int prewittGx[3][3] = {{-1,0,1},{-1,0,1},{-1,0,1}};
    static const int prewittGy[3][3] = {{-1,-1,-1},{0,0,0},{1,1,1}};

    const int (*kernelGx)[3] = (nMethod == 0) ? sobelGx : prewittGx;
    const int (*kernelGy)[3] = (nMethod == 0) ? sobelGy : prewittGy;

#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        BYTE* pDstRow = pDst + y * nDstStride;
        for (int x = 0; x < nWidth; x++)
        {
            int nSumGx = 0, nSumGy = 0;
            for (int ky = -1; ky <= 1; ky++)
            {
                int sy = max(0, min(nHeight - 1, y + ky));
                for (int kx = -1; kx <= 1; kx++)
                {
                    int sx  = max(0, min(nWidth - 1, x + kx));
                    int pix = grayBuf[sy * nWidth + sx];
                    nSumGx += pix * kernelGx[ky + 1][kx + 1];
                    nSumGy += pix * kernelGy[ky + 1][kx + 1];
                }
            }
            int mag = (int)(sqrt((double)(nSumGx * nSumGx + nSumGy * nSumGy)) + 0.5);
            mag = min(255, mag);
            pDstRow[x] = (mag >= nThreshold) ? (BYTE)mag : 0;
        }
    }
    return true;
}

static bool ApplyLaplacian(const CImageBuffer& input, CImageBuffer& output, int nThreshold)
{
    int nWidth  = input.GetWidth();
    int nHeight = input.GetHeight();

    std::vector<BYTE> grayBuf;
    BuildGrayBuf(input, grayBuf);

    if (!output.Create(nWidth, nHeight, 1)) return false;
    BYTE* pDst      = output.GetData();
    int   nDstStride = output.GetStride();

    // Laplacian of Gaussian kernel (approximation)
    static const int kernel[3][3] = {{0,1,0},{1,-4,1},{0,1,0}};

#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        BYTE* pDstRow = pDst + y * nDstStride;
        for (int x = 0; x < nWidth; x++)
        {
            int val = 0;
            for (int ky = -1; ky <= 1; ky++)
            {
                int sy = max(0, min(nHeight - 1, y + ky));
                for (int kx = -1; kx <= 1; kx++)
                {
                    int sx = max(0, min(nWidth - 1, x + kx));
                    val += grayBuf[sy * nWidth + sx] * kernel[ky + 1][kx + 1];
                }
            }
            int mag = min(255, abs(val));
            pDstRow[x] = (mag >= nThreshold) ? (BYTE)mag : 0;
        }
    }
    return true;
}

static bool ApplyCanny(const CImageBuffer& input, CImageBuffer& output,
                       int nLowThresh, int nHighThresh)
{
    int nWidth  = input.GetWidth();
    int nHeight = input.GetHeight();

    std::vector<BYTE> grayBuf;
    BuildGrayBuf(input, grayBuf);

    // Step 1: Gaussian smoothing (sigma=1.0, kernel=5)
    int nKernel = 5, nHalf = 2;
    double sigma = 1.0;
    double gaussK[25];
    {
        double sum = 0;
        for (int ky = -nHalf; ky <= nHalf; ky++)
            for (int kx = -nHalf; kx <= nHalf; kx++)
            {
                double v = exp(-(ky*ky + kx*kx) / (2.0 * sigma * sigma));
                gaussK[(ky + nHalf) * nKernel + kx + nHalf] = v;
                sum += v;
            }
        for (int i = 0; i < 25; i++) gaussK[i] /= sum;
    }

    std::vector<BYTE> smoothed(nWidth * nHeight);
#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
        for (int x = 0; x < nWidth; x++)
        {
            double s = 0;
            for (int ky = -nHalf; ky <= nHalf; ky++)
                for (int kx = -nHalf; kx <= nHalf; kx++)
                {
                    int sy = max(0, min(nHeight - 1, y + ky));
                    int sx = max(0, min(nWidth  - 1, x + kx));
                    s += grayBuf[sy * nWidth + sx] * gaussK[(ky+nHalf)*nKernel + kx+nHalf];
                }
            smoothed[y * nWidth + x] = (BYTE)(s + 0.5);
        }

    // Step 2: Sobel gradients
    std::vector<float> gMag(nWidth * nHeight);
    std::vector<float> gDir(nWidth * nHeight);
    static const int sobelGx[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
    static const int sobelGy[3][3] = {{-1,-2,-1},{0,0,0},{1,2,1}};

#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
        for (int x = 0; x < nWidth; x++)
        {
            int gx = 0, gy = 0;
            for (int ky = -1; ky <= 1; ky++)
                for (int kx = -1; kx <= 1; kx++)
                {
                    int sy = max(0, min(nHeight - 1, y + ky));
                    int sx = max(0, min(nWidth  - 1, x + kx));
                    int p  = smoothed[sy * nWidth + sx];
                    gx += p * sobelGx[ky+1][kx+1];
                    gy += p * sobelGy[ky+1][kx+1];
                }
            gMag[y * nWidth + x] = (float)sqrt((double)(gx*gx + gy*gy));
            gDir[y * nWidth + x] = (float)atan2((double)gy, (double)gx);
        }

    // Step 3: Non-maximum suppression
    std::vector<BYTE> nms(nWidth * nHeight, 0);
#pragma omp parallel for schedule(static)
    for (int y = 1; y < nHeight - 1; y++)
        for (int x = 1; x < nWidth - 1; x++)
        {
            float mag = gMag[y * nWidth + x];
            float dir = gDir[y * nWidth + x];
            // Round direction to 0, 45, 90, 135 degrees
            float angle = dir * 180.0f / (float)M_PI;
            if (angle < 0) angle += 180.0f;

            float n1, n2;
            if (angle < 22.5f || angle >= 157.5f)
            { n1 = gMag[y * nWidth + x + 1]; n2 = gMag[y * nWidth + x - 1]; }
            else if (angle < 67.5f)
            { n1 = gMag[(y+1) * nWidth + x - 1]; n2 = gMag[(y-1) * nWidth + x + 1]; }
            else if (angle < 112.5f)
            { n1 = gMag[(y+1) * nWidth + x]; n2 = gMag[(y-1) * nWidth + x]; }
            else
            { n1 = gMag[(y-1) * nWidth + x - 1]; n2 = gMag[(y+1) * nWidth + x + 1]; }

            nms[y * nWidth + x] = (mag >= n1 && mag >= n2) ? (BYTE)min(255.0f, mag) : 0;
        }

    // Step 4: Hysteresis thresholding
    if (!output.Create(nWidth, nHeight, 1)) return false;
    BYTE* pDst      = output.GetData();
    int   nDstStride = output.GetStride();

    // Mark strong/weak edges
    std::vector<BYTE> edges(nWidth * nHeight, 0);
    for (int i = 0; i < nWidth * nHeight; i++)
    {
        if (nms[i] >= nHighThresh)       edges[i] = 255;  // strong
        else if (nms[i] >= nLowThresh)   edges[i] = 128;  // weak
    }

    // Connect weak edges adjacent to strong edges
    for (int y = 1; y < nHeight - 1; y++)
        for (int x = 1; x < nWidth - 1; x++)
            if (edges[y * nWidth + x] == 128)
            {
                bool connected = false;
                for (int ky = -1; ky <= 1 && !connected; ky++)
                    for (int kx = -1; kx <= 1 && !connected; kx++)
                        if (edges[(y+ky)*nWidth + x+kx] == 255) connected = true;
                edges[y * nWidth + x] = connected ? 255 : 0;
            }

#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        BYTE* pRow = pDst + y * nDstStride;
        for (int x = 0; x < nWidth; x++)
            pRow[x] = (edges[y * nWidth + x] == 255) ? 255 : 0;
    }

    return true;
}

bool CEdgeDetect::Process(const CImageBuffer& input, CImageBuffer& output)
{
    if (!input.IsValid()) return false;

    int nMethod     = (int)m_params[0].dCurrentVal;
    int nThreshold  = (int)m_params[1].dCurrentVal;
    int nHighThresh = (int)m_params[2].dCurrentVal;

    if (nHighThresh < nThreshold) nHighThresh = nThreshold;

    switch (nMethod)
    {
    case 2: return ApplyCanny(input, output, nThreshold, nHighThresh);
    case 3: return ApplyLaplacian(input, output, nThreshold);
    default: return ApplySobelPrewitt(input, output, nMethod, nThreshold);
    }
}

CAlgorithmBase* CEdgeDetect::Clone() const { return new CEdgeDetect(*this); }
