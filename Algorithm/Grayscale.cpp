#include "stdafx.h"
#include "Algorithm/Grayscale.h"
#include <cmath>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

CGrayscale::CGrayscale()
{
    // Channel mode parameter
    // 0=Luminance, 1=R, 2=G, 3=B, 4=H(Hue), 5=S(Sat), 6=V(Val)
    AlgorithmParam paramMode;
    paramMode.strName        = _T("채널");
    paramMode.strDescription = _T("추출할 채널을 선택하세요");
    paramMode.dMinVal        = 0.0;
    paramMode.dMaxVal        = 6.0;
    paramMode.dDefaultVal    = 0.0;
    paramMode.dCurrentVal    = 0.0;
    paramMode.nPrecision     = 0;
    paramMode.vecOptions     = { _T("휘도(Luminance)"), _T("빨강(R)"), _T("녹색(G)"),
                                  _T("파랑(B)"), _T("색조(H)"), _T("채도(S)"), _T("명도(V)") };
    m_params.push_back(paramMode);
}

CGrayscale::~CGrayscale()
{
}

CString CGrayscale::GetName() const
{
    return _T("Grayscale");
}

CString CGrayscale::GetDescription() const
{
    return _T("Channel separation: Luminance / R / G / B / H / S / V");
}

std::vector<AlgorithmParam>& CGrayscale::GetParams()
{
    return m_params;
}

void CGrayscale::RGBtoHSV(BYTE r, BYTE g, BYTE b, BYTE& h, BYTE& s, BYTE& v)
{
    double rf = r / 255.0, gf = g / 255.0, bf = b / 255.0;
    double maxC = rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf);
    double minC = rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf);
    double delta = maxC - minC;

    // Value
    v = (BYTE)(maxC * 255.0 + 0.5);

    // Saturation
    s = (maxC > 1e-6) ? (BYTE)(delta / maxC * 255.0 + 0.5) : 0;

    // Hue
    double hf = 0.0;
    if (delta > 1e-6)
    {
        if (maxC == rf)       hf = (gf - bf) / delta;
        else if (maxC == gf)  hf = 2.0 + (bf - rf) / delta;
        else                  hf = 4.0 + (rf - gf) / delta;

        hf /= 6.0;
        if (hf < 0.0) hf += 1.0;
    }
    h = (BYTE)(hf * 255.0 + 0.5);
}

bool CGrayscale::Process(const CImageBuffer& input, CImageBuffer& output)
{
    if (!input.IsValid()) return false;

    int nWidth    = input.GetWidth();
    int nHeight   = input.GetHeight();
    int nChannels = input.GetChannels();
    int nMode     = (int)m_params[0].dCurrentVal;

    // Clamp mode
    if (nMode < 0) nMode = 0;
    if (nMode > 6) nMode = 6;

    // If already 1-channel and mode is luminance → clone
    if (nChannels == 1 && nMode == 0)
    {
        output = input.Clone();
        return output.IsValid();
    }

    if (!output.Create(nWidth, nHeight, 1))
        return false;

    const BYTE* pSrc   = input.GetData();
    BYTE*       pDst   = output.GetData();
    int nSrcStride     = input.GetStride();
    int nDstStride     = output.GetStride();

#pragma omp parallel for schedule(static)
    for (int y = 0; y < nHeight; y++)
    {
        const BYTE* pSrcRow = pSrc + y * nSrcStride;
        BYTE*       pDstRow = pDst + y * nDstStride;

        for (int x = 0; x < nWidth; x++)
        {
            BYTE bOut = 0;

            if (nChannels == 1)
            {
                // Already 1-channel → use as-is for any channel mode
                bOut = pSrcRow[x];
            }
            else
            {
                BYTE bB = pSrcRow[x * nChannels + 0];
                BYTE bG = pSrcRow[x * nChannels + 1];
                BYTE bR = pSrcRow[x * nChannels + 2];

                switch (nMode)
                {
                case 0: // Luminance — integer: (77R+150G+29B)>>8 ≈ 0.301R+0.586G+0.113B
                    bOut = (BYTE)((77 * bR + 150 * bG + 29 * bB) >> 8);
                    break;
                case 1: bOut = bR; break;   // R channel
                case 2: bOut = bG; break;   // G channel
                case 3: bOut = bB; break;   // B channel
                case 4: case 5: case 6:     // HSV
                {
                    BYTE bH, bS, bV;
                    RGBtoHSV(bR, bG, bB, bH, bS, bV);
                    bOut = (nMode == 4) ? bH : (nMode == 5) ? bS : bV;
                    break;
                }
                default: bOut = 0; break;
                }
            }
            pDstRow[x] = bOut;
        }
    }

    return true;
}

CAlgorithmBase* CGrayscale::Clone() const
{
    return new CGrayscale(*this);
}
