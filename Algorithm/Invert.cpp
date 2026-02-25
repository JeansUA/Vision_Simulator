#include "stdafx.h"
#include "Algorithm/Invert.h"

#ifdef _OPENMP
#include <omp.h>
#endif

CInvert::CInvert() {}
CInvert::~CInvert() {}
CString CInvert::GetName() const        { return _T("Invert"); }
CString CInvert::GetDescription() const { return _T("Invert pixel values"); }
std::vector<AlgorithmParam>& CInvert::GetParams() { return m_params; }

bool CInvert::Process(const CImageBuffer& input, CImageBuffer& output)
{
    if (!input.IsValid()) return false;

    int nWidth    = input.GetWidth();
    int nHeight   = input.GetHeight();
    int nChannels = input.GetChannels();

    if (!output.Create(nWidth, nHeight, nChannels)) return false;

    const BYTE* pSrc      = input.GetData();
    BYTE*       pDst      = output.GetData();
    int         nSrcStride = input.GetStride();
    int         nDstStride = output.GetStride();

#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        const BYTE* pSrcRow = pSrc + y * nSrcStride;
        BYTE*       pDstRow = pDst + y * nDstStride;
        for (int x = 0; x < nWidth * nChannels; x++)
            pDstRow[x] = 255 - pSrcRow[x];
    }

    return true;
}

CAlgorithmBase* CInvert::Clone() const { return new CInvert(*this); }
