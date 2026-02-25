#include "stdafx.h"
#include "Algorithm/HoughCircle.h"
#include <cmath>
#include <vector>
#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CHoughCircle::CHoughCircle()
{
    AlgorithmParam p;

    p.strName = _T("minRadius"); p.strDescription = _T("Min circle radius (px)");
    p.dMinVal = 5.0; p.dMaxVal = 300.0; p.dDefaultVal = 20.0; p.dCurrentVal = 20.0; p.nPrecision = 0;
    m_params.push_back(p);

    p.strName = _T("maxRadius"); p.strDescription = _T("Max circle radius (px)");
    p.dMinVal = 10.0; p.dMaxVal = 500.0; p.dDefaultVal = 100.0; p.dCurrentVal = 100.0; p.nPrecision = 0;
    m_params.push_back(p);

    p.strName = _T("threshold"); p.strDescription = _T("Accumulator threshold");
    p.dMinVal = 10.0; p.dMaxVal = 255.0; p.dDefaultVal = 80.0; p.dCurrentVal = 80.0; p.nPrecision = 0;
    m_params.push_back(p);

    p.strName = _T("minDist"); p.strDescription = _T("Min distance between circles");
    p.dMinVal = 5.0; p.dMaxVal = 500.0; p.dDefaultVal = 30.0; p.dCurrentVal = 30.0; p.nPrecision = 0;
    m_params.push_back(p);
}

CHoughCircle::~CHoughCircle() {}
CString CHoughCircle::GetName() const        { return _T("Hough Circle"); }
CString CHoughCircle::GetDescription() const { return _T("Circle detection via Hough transform"); }
std::vector<AlgorithmParam>& CHoughCircle::GetParams() { return m_params; }

// Draw anti-aliased circle outline on image buffer
static void DrawCircle(CImageBuffer& img, int cx, int cy, int r,
                       BYTE bR, BYTE bG, BYTE bB, int thickness = 2)
{
    int nWidth    = img.GetWidth();
    int nHeight   = img.GetHeight();
    int nChannels = img.GetChannels();
    int nStride   = img.GetStride();
    BYTE* pData   = img.GetData();

    // Bresenham circle drawing
    int x = 0, y = r, d = 3 - 2 * r;
    auto plotCirclePoints = [&](int px, int py) {
        int pts[8][2] = {
            {cx+px, cy+py}, {cx-px, cy+py}, {cx+px, cy-py}, {cx-px, cy-py},
            {cx+py, cy+px}, {cx-py, cy+px}, {cx+py, cy-px}, {cx-py, cy-px}
        };
        for (auto& pt : pts)
        {
            for (int t = -thickness/2; t <= thickness/2; t++)
            {
                int gx = pt[0] + t, gy = pt[1];
                if (gx >= 0 && gx < nWidth && gy >= 0 && gy < nHeight)
                {
                    BYTE* p = pData + gy * nStride + gx * nChannels;
                    if (nChannels == 1) p[0] = bR;
                    else { p[0] = bB; p[1] = bG; p[2] = bR; }
                }
            }
        }
    };
    while (x <= y)
    {
        plotCirclePoints(x, y);
        if (d < 0) d += 4 * x + 6;
        else { d += 4 * (x - y) + 10; y--; }
        x++;
    }
}

bool CHoughCircle::Process(const CImageBuffer& input, CImageBuffer& output)
{
    if (!input.IsValid()) return false;

    int nWidth    = input.GetWidth();
    int nHeight   = input.GetHeight();
    int nChannels = input.GetChannels();
    int nStride   = input.GetStride();

    int nMinR    = (int)m_params[0].dCurrentVal;
    int nMaxR    = (int)m_params[1].dCurrentVal;
    int nThresh  = (int)m_params[2].dCurrentVal;
    int nMinDist = (int)m_params[3].dCurrentVal;

    nMinR = max(2, nMinR);
    nMaxR = max(nMinR + 1, nMaxR);

    // Convert to grayscale
    std::vector<BYTE> gray(nWidth * nHeight);
    const BYTE* pSrc = input.GetData();
    for (int y = 0; y < nHeight; y++)
    {
        const BYTE* pRow = pSrc + y * nStride;
        for (int x = 0; x < nWidth; x++)
        {
            if (nChannels == 1)
                gray[y * nWidth + x] = pRow[x];
            else
            {
                int g = (int)(0.299 * pRow[x*3+2] + 0.587 * pRow[x*3+1] + 0.114 * pRow[x*3] + 0.5);
                gray[y * nWidth + x] = (BYTE)max(0, min(255, g));
            }
        }
    }

    // Simple edge detection using Sobel
    std::vector<BYTE> edges(nWidth * nHeight, 0);
    {
        static const int sobelGx[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
        static const int sobelGy[3][3] = {{-1,-2,-1},{0,0,0},{1,2,1}};
        int edgeThresh = 30;
        for (int y = 1; y < nHeight - 1; y++)
            for (int x = 1; x < nWidth - 1; x++)
            {
                int gx = 0, gy = 0;
                for (int ky = -1; ky <= 1; ky++)
                    for (int kx = -1; kx <= 1; kx++)
                    {
                        int p = gray[(y+ky)*nWidth + x+kx];
                        gx += p * sobelGx[ky+1][kx+1];
                        gy += p * sobelGy[ky+1][kx+1];
                    }
                int mag = (int)sqrt((double)(gx*gx + gy*gy));
                edges[y * nWidth + x] = (mag > edgeThresh) ? 255 : 0;
            }
    }

    // Hough accumulator (indexed by [cy][cx] per radius)
    // Use radius stride to save memory: process each r separately
    int nRadiiStep = max(1, (nMaxR - nMinR) / 30 + 1);  // limit radius resolution

    std::vector<std::tuple<int,int,int,int>> detectedCircles;  // (x, y, r, votes)

    for (int r = nMinR; r <= nMaxR; r += nRadiiStep)
    {
        std::vector<int> acc(nWidth * nHeight, 0);

        // Precompute sin/cos for this radius
        int nAngles = max(36, (int)(2.0 * M_PI * r));
        std::vector<int> cosTab(nAngles), sinTab(nAngles);
        for (int a = 0; a < nAngles; a++)
        {
            double angle = 2.0 * M_PI * a / nAngles;
            cosTab[a] = (int)(cos(angle) * r + 0.5);
            sinTab[a] = (int)(sin(angle) * r + 0.5);
        }

        // Vote
        for (int y = 1; y < nHeight - 1; y++)
            for (int x = 1; x < nWidth - 1; x++)
                if (edges[y * nWidth + x])
                    for (int a = 0; a < nAngles; a++)
                    {
                        int cy = y - sinTab[a];
                        int cx = x - cosTab[a];
                        if (cx >= 0 && cx < nWidth && cy >= 0 && cy < nHeight)
                            acc[cy * nWidth + cx]++;
                    }

        // Normalize threshold by number of edge points
        int localThresh = (int)(nThresh * nAngles / 100);

        // Find peaks with NMS
        for (int cy = r; cy < nHeight - r; cy++)
            for (int cx = r; cx < nWidth - r; cx++)
            {
                int v = acc[cy * nWidth + cx];
                if (v < localThresh) continue;

                // Check local maximum
                bool bMax = true;
                for (int dy = -2; dy <= 2 && bMax; dy++)
                    for (int dx = -2; dx <= 2 && bMax; dx++)
                        if ((dx || dy) && acc[(cy+dy)*nWidth + cx+dx] >= v) bMax = false;

                if (!bMax) continue;

                // Check min distance from already detected
                bool tooClose = false;
                for (auto& c : detectedCircles)
                {
                    int dx = cx - std::get<0>(c), dy = cy - std::get<1>(c);
                    if (dx*dx + dy*dy < nMinDist * nMinDist) { tooClose = true; break; }
                }
                if (!tooClose)
                    detectedCircles.push_back({ cx, cy, r, v });
            }
    }

    // Sort by votes descending, keep top 50
    std::sort(detectedCircles.begin(), detectedCircles.end(),
        [](const auto& a, const auto& b) { return std::get<3>(a) > std::get<3>(b); });
    if (detectedCircles.size() > 50) detectedCircles.resize(50);

    // Output: clone input (convert to 3-channel if needed) and draw circles
    if (nChannels == 1)
    {
        if (!output.Create(nWidth, nHeight, 3)) return false;
        // Expand grayscale to RGB
        BYTE* pDst      = output.GetData();
        int   nDstStride = output.GetStride();
        for (int y = 0; y < nHeight; y++)
        {
            BYTE* pDstRow = pDst + y * nDstStride;
            for (int x = 0; x < nWidth; x++)
            {
                BYTE v = gray[y * nWidth + x];
                pDstRow[x*3] = v; pDstRow[x*3+1] = v; pDstRow[x*3+2] = v;
            }
        }
    }
    else
    {
        output = input.Clone();
        if (!output.IsValid()) return false;
    }

    // Draw detected circles
    static const BYTE circleColors[][3] = {
        {255,0,0}, {0,255,0}, {0,0,255}, {255,255,0},
        {255,0,255}, {0,255,255}, {255,128,0}, {128,0,255}
    };
    int nColors = sizeof(circleColors) / sizeof(circleColors[0]);

    for (int i = 0; i < (int)detectedCircles.size(); i++)
    {
        int cx = std::get<0>(detectedCircles[i]);
        int cy = std::get<1>(detectedCircles[i]);
        int r  = std::get<2>(detectedCircles[i]);
        const BYTE* col = circleColors[i % nColors];
        DrawCircle(output, cx, cy, r, col[0], col[1], col[2], 2);
        // Draw center dot
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -2; dx <= 2; dx++)
            {
                int px = cx + dx, py = cy + dy;
                if (px >= 0 && px < nWidth && py >= 0 && py < nHeight)
                {
                    BYTE* p = output.GetData() + py * output.GetStride() + px * output.GetChannels();
                    if (output.GetChannels() == 1) p[0] = col[0];
                    else { p[0] = col[2]; p[1] = col[1]; p[2] = col[0]; }
                }
            }
    }

    return true;
}

CAlgorithmBase* CHoughCircle::Clone() const { return new CHoughCircle(*this); }
