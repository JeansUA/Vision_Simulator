#include "stdafx.h"
#include "Algorithm/Morphology.h"
#include <cmath>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

CMorphology::CMorphology()
{
    AlgorithmParam paramOperation;
    paramOperation.strName        = _T("연산");
    paramOperation.strDescription = _T("모폴로지 연산을 선택하세요");
    paramOperation.dMinVal        = 0.0;
    paramOperation.dMaxVal        = 5.0;
    paramOperation.dDefaultVal    = 0.0;
    paramOperation.dCurrentVal    = 0.0;
    paramOperation.nPrecision     = 0;
    paramOperation.vecOptions     = { _T("침식(Erode)"), _T("팽창(Dilate)"),
                                       _T("열기(Open)"), _T("닫기(Close)"),
                                       _T("탑햇(Top Hat)"), _T("블랙햇(Black Hat)") };
    m_params.push_back(paramOperation);

    AlgorithmParam paramKernelSize;
    paramKernelSize.strName        = _T("커널 크기");
    paramKernelSize.strDescription = _T("구조 요소(커널) 크기 (홀수, 클수록 효과가 강함)");
    paramKernelSize.dMinVal        = 3.0;
    paramKernelSize.dMaxVal        = 21.0;
    paramKernelSize.dDefaultVal    = 3.0;
    paramKernelSize.dCurrentVal    = 3.0;
    paramKernelSize.nPrecision     = 0;
    m_params.push_back(paramKernelSize);

    AlgorithmParam paramIterations;
    paramIterations.strName        = _T("반복 횟수");
    paramIterations.strDescription = _T("연산 반복 횟수 (횟수가 많을수록 효과가 강해짐)");
    paramIterations.dMinVal        = 1.0;
    paramIterations.dMaxVal        = 10.0;
    paramIterations.dDefaultVal    = 1.0;
    paramIterations.dCurrentVal    = 1.0;
    paramIterations.nPrecision     = 0;
    m_params.push_back(paramIterations);
}

CMorphology::~CMorphology() {}

CString CMorphology::GetName() const        { return _T("Morphology"); }
CString CMorphology::GetDescription() const { return _T("Erode/Dilate/Open/Close/TopHat/BlackHat"); }
std::vector<AlgorithmParam>& CMorphology::GetParams() { return m_params; }

int CMorphology::ClampCoord(int val, int maxVal)
{
    if (val < 0)       return 0;
    if (val >= maxVal) return maxVal - 1;
    return val;
}

bool CMorphology::ConvertToGrayscale(const CImageBuffer& input, CImageBuffer& output)
{
    int nWidth    = input.GetWidth();
    int nHeight   = input.GetHeight();
    int nChannels = input.GetChannels();

    if (nChannels == 1) { output = input.Clone(); return output.IsValid(); }

    if (!output.Create(nWidth, nHeight, 1)) return false;

    const BYTE* pSrc      = input.GetData();
    BYTE*       pDst      = output.GetData();
    int         nSrcStride = input.GetStride();
    int         nDstStride = output.GetStride();

#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        const BYTE* pSrcRow = pSrc + y * nSrcStride;
        BYTE*       pDstRow = pDst + y * nDstStride;
        for (int x = 0; x < nWidth; x++)
        {
            int g = (int)(0.299 * pSrcRow[x*3+2] + 0.587 * pSrcRow[x*3+1] + 0.114 * pSrcRow[x*3] + 0.5);
            pDstRow[x] = (BYTE)max(0, min(255, g));
        }
    }
    return true;
}

bool CMorphology::Erode(const CImageBuffer& input, CImageBuffer& output, int nKernelSize)
{
    int nWidth  = input.GetWidth();
    int nHeight = input.GetHeight();
    int nHalf   = nKernelSize / 2;

    if (!output.Create(nWidth, nHeight, 1)) return false;

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
            BYTE bMin = 255;
            for (int ky = -nHalf; ky <= nHalf; ky++)
            {
                int sy = max(0, min(nHeight - 1, y + ky));
                const BYTE* pSrcRow = pSrc + sy * nSrcStride;
                for (int kx = -nHalf; kx <= nHalf; kx++)
                {
                    int sx = max(0, min(nWidth - 1, x + kx));
                    if (pSrcRow[sx] < bMin) bMin = pSrcRow[sx];
                }
            }
            pDstRow[x] = bMin;
        }
    }
    return true;
}

bool CMorphology::Dilate(const CImageBuffer& input, CImageBuffer& output, int nKernelSize)
{
    int nWidth  = input.GetWidth();
    int nHeight = input.GetHeight();
    int nHalf   = nKernelSize / 2;

    if (!output.Create(nWidth, nHeight, 1)) return false;

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
            BYTE bMax = 0;
            for (int ky = -nHalf; ky <= nHalf; ky++)
            {
                int sy = max(0, min(nHeight - 1, y + ky));
                const BYTE* pSrcRow = pSrc + sy * nSrcStride;
                for (int kx = -nHalf; kx <= nHalf; kx++)
                {
                    int sx = max(0, min(nWidth - 1, x + kx));
                    if (pSrcRow[sx] > bMax) bMax = pSrcRow[sx];
                }
            }
            pDstRow[x] = bMax;
        }
    }
    return true;
}

bool CMorphology::Process(const CImageBuffer& input, CImageBuffer& output)
{
    if (!input.IsValid()) return false;

    int nOperation  = (int)m_params[0].dCurrentVal;
    int nKernelSize = (int)m_params[1].dCurrentVal;
    int nIterations = (int)m_params[2].dCurrentVal;

    if (nKernelSize % 2 == 0) nKernelSize++;
    nKernelSize = max(3, min(21, nKernelSize));
    nIterations = max(1, min(10, nIterations));

    CImageBuffer grayInput;
    if (!ConvertToGrayscale(input, grayInput)) return false;

    CImageBuffer current = grayInput;
    CImageBuffer result;

    auto ApplyOp = [&](int op) -> bool {
        CImageBuffer tmp = current;
        result = CImageBuffer();
        for (int it = 0; it < nIterations; it++)
        {
            bool ok = (op == 0) ? Erode(tmp, result, nKernelSize) : Dilate(tmp, result, nKernelSize);
            if (!ok) return false;
            if (it < nIterations - 1) { tmp = result; result = CImageBuffer(); }
        }
        return true;
    };

    switch (nOperation)
    {
    case 0: // Erode
        if (!ApplyOp(0)) return false;
        break;
    case 1: // Dilate
        if (!ApplyOp(1)) return false;
        break;
    case 2: // Open = Erode then Dilate
    {
        if (!ApplyOp(0)) return false;
        CImageBuffer eroded = result;
        current = eroded;
        if (!ApplyOp(1)) return false;
        break;
    }
    case 3: // Close = Dilate then Erode
    {
        if (!ApplyOp(1)) return false;
        CImageBuffer dilated = result;
        current = dilated;
        if (!ApplyOp(0)) return false;
        break;
    }
    case 4: // Top Hat = Original - Open
    {
        if (!ApplyOp(0)) return false;
        CImageBuffer eroded = result;
        CImageBuffer opened;
        current = eroded;
        if (!ApplyOp(1)) return false;
        CImageBuffer& openResult = result;

        int nWidth  = grayInput.GetWidth();
        int nHeight = grayInput.GetHeight();
        if (!opened.Create(nWidth, nHeight, 1)) return false;
        for (int i = 0; i < nWidth * nHeight; i++)
        {
            int v = (int)grayInput.GetData()[i] - (int)openResult.GetData()[i];
            opened.GetData()[i] = (BYTE)max(0, min(255, v));
        }
        result = opened;
        break;
    }
    case 5: // Black Hat = Close - Original
    {
        if (!ApplyOp(1)) return false;
        CImageBuffer dilated = result;
        current = dilated;
        if (!ApplyOp(0)) return false;
        CImageBuffer& closeResult = result;

        int nWidth  = grayInput.GetWidth();
        int nHeight = grayInput.GetHeight();
        CImageBuffer bh;
        if (!bh.Create(nWidth, nHeight, 1)) return false;
        for (int i = 0; i < nWidth * nHeight; i++)
        {
            int v = (int)closeResult.GetData()[i] - (int)grayInput.GetData()[i];
            bh.GetData()[i] = (BYTE)max(0, min(255, v));
        }
        result = bh;
        break;
    }
    default: return false;
    }

    output = result;
    return true;
}

CAlgorithmBase* CMorphology::Clone() const { return new CMorphology(*this); }
