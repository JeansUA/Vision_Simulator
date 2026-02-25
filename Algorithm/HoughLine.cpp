#include "stdafx.h"
#include "Algorithm/HoughLine.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CHoughLine::CHoughLine()
{
    AlgorithmParam p;

    p.strName = _T("방식"); p.strDescription = _T("라인 검출 방식을 선택하세요");
    p.dMinVal = 0; p.dMaxVal = 1; p.dDefaultVal = 0; p.dCurrentVal = 0; p.nPrecision = 0;
    p.vecOptions = { _T("표준(무한선)"), _T("확률적(선분 추출)") };
    m_params.push_back(p);

    p.vecOptions.clear();
    p.strName = _T("검출 임계값"); p.strDescription = _T("라인 검출 기준 투표 수 — 높을수록 강한 라인만 검출");
    p.dMinVal = 10; p.dMaxVal = 300; p.dDefaultVal = 80; p.dCurrentVal = 80; p.nPrecision = 0;
    m_params.push_back(p);

    p.strName = _T("최소 선분 길이"); p.strDescription = _T("검출할 선분의 최소 길이 (확률적 방식 전용, 픽셀)");
    p.dMinVal = 5; p.dMaxVal = 500; p.dDefaultVal = 30; p.dCurrentVal = 30; p.nPrecision = 0;
    m_params.push_back(p);

    p.strName = _T("최대 허용 간격"); p.strDescription = _T("선분 사이 최대 허용 간격 (확률적 방식 전용, 픽셀)");
    p.dMinVal = 1; p.dMaxVal = 100; p.dDefaultVal = 10; p.dCurrentVal = 10; p.nPrecision = 0;
    m_params.push_back(p);
}

CHoughLine::~CHoughLine() {}
CString CHoughLine::GetName() const        { return _T("Hough Line"); }
CString CHoughLine::GetDescription() const { return _T("Line / angle detection via Hough transform"); }
std::vector<AlgorithmParam>& CHoughLine::GetParams() { return m_params; }

// Draw a line segment from (x0,y0) to (x1,y1) using Bresenham with thickness
static void DrawLine(CImageBuffer& img, int x0, int y0, int x1, int y1,
                     BYTE bR, BYTE bG, BYTE bB, int thickness = 2)
{
    int nWidth    = img.GetWidth();
    int nHeight   = img.GetHeight();
    int nChannels = img.GetChannels();
    int nStride   = img.GetStride();
    BYTE* pData   = img.GetData();

    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    auto setPixel = [&](int px, int py) {
        for (int t = -thickness/2; t <= thickness/2; t++)
        {
            int nx = px + (dy > dx ? t : 0);
            int ny = py + (dy <= dx ? t : 0);
            if (nx >= 0 && nx < nWidth && ny >= 0 && ny < nHeight)
            {
                BYTE* p = pData + ny * nStride + nx * nChannels;
                if (nChannels == 1) p[0] = bR;
                else { p[0] = bB; p[1] = bG; p[2] = bR; }
            }
        }
    };

    while (true)
    {
        setPixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

// -----------------------------------------------------------------------
// Standard Hough Transform (infinite lines via rho-theta space)
// -----------------------------------------------------------------------
static void RunStandardHough(
    const std::vector<BYTE>& edges,
    int nWidth, int nHeight,
    int nThreshold,
    CImageBuffer& output,
    const std::vector<BYTE>& gray)
{
    double maxRho = sqrt((double)(nWidth*nWidth + nHeight*nHeight));
    int nRho   = (int)(2 * maxRho) + 1;
    int nTheta = 180;

    std::vector<double> sinTab(nTheta), cosTab(nTheta);
    for (int t = 0; t < nTheta; t++)
    {
        double angle = t * M_PI / 180.0;
        sinTab[t] = sin(angle);
        cosTab[t] = cos(angle);
    }

    std::vector<int> acc(nRho * nTheta, 0);

#ifdef _OPENMP
    {
        int nT = omp_get_max_threads();
        if (nT > 8) nT = 8;
        std::vector<std::vector<int>> tAcc(nT, std::vector<int>(nRho * nTheta, 0));
#pragma omp parallel for schedule(dynamic, 8) num_threads(nT)
        for (int y = 0; y < nHeight; y++)
        {
            int tid = omp_get_thread_num();
            auto& myAcc = tAcc[tid];
            for (int x = 0; x < nWidth; x++)
                if (edges[y*nWidth+x])
                    for (int t = 0; t < nTheta; t++)
                    {
                        int rho = (int)(x * cosTab[t] + y * sinTab[t] + maxRho + 0.5);
                        if (rho >= 0 && rho < nRho)
                            myAcc[rho * nTheta + t]++;
                    }
        }
        for (int tid = 0; tid < nT; tid++)
            for (int i = 0; i < nRho * nTheta; i++)
                acc[i] += tAcc[tid][i];
    }
#else
    for (int y = 0; y < nHeight; y++)
        for (int x = 0; x < nWidth; x++)
            if (edges[y*nWidth+x])
                for (int t = 0; t < nTheta; t++)
                {
                    int rho = (int)(x * cosTab[t] + y * sinTab[t] + maxRho + 0.5);
                    if (rho >= 0 && rho < nRho)
                        acc[rho * nTheta + t]++;
                }
#endif

    // Collect local maxima
    struct Line { double rho, theta; int votes; };
    std::vector<Line> lines;

    for (int r = 1; r < nRho-1; r++)
        for (int t = 1; t < nTheta-1; t++)
        {
            int v = acc[r * nTheta + t];
            if (v < nThreshold) continue;
            bool bMax = true;
            for (int dr=-2; dr<=2 && bMax; dr++)
                for (int dt=-2; dt<=2 && bMax; dt++)
                    if ((dr||dt) && acc[(r+dr)*nTheta+t+dt] >= v) bMax = false;
            if (bMax)
                lines.push_back({r - maxRho, t * M_PI / 180.0, v});
        }

    std::sort(lines.begin(), lines.end(),
        [](const Line& a, const Line& b){ return a.votes > b.votes; });
    if (lines.size() > 100) lines.resize(100);

    static const BYTE lineColors[][3] = {
        {255,0,0},{0,255,0},{0,0,255},{255,255,0},{255,0,255},{0,255,255}
    };
    const int nColCount = 6;

    for (int i = 0; i < (int)lines.size(); i++)
    {
        double rho   = lines[i].rho;
        double theta = lines[i].theta;
        double cosT  = cos(theta), sinT = sin(theta);
        const BYTE* col = lineColors[i % nColCount];

        int x0, y0, x1, y1;
        if (fabs(sinT) > 0.01)
        {
            x0 = 0;          y0 = (int)((rho - x0 * cosT) / sinT + 0.5);
            x1 = nWidth - 1; y1 = (int)((rho - x1 * cosT) / sinT + 0.5);
        }
        else
        {
            y0 = 0;           x0 = (int)((rho - y0 * sinT) / cosT + 0.5);
            y1 = nHeight - 1; x1 = (int)((rho - y1 * sinT) / cosT + 0.5);
        }

        DrawLine(output, x0, y0, x1, y1, col[0], col[1], col[2], 2);
    }
}

// -----------------------------------------------------------------------
// Probabilistic Hough Transform (line segments)
// Based on PPHT: random edge sampling, theta-accumulator per angle,
// then scan along the line to extract segments.
// -----------------------------------------------------------------------
static void RunProbabilisticHough(
    const std::vector<BYTE>& edges,
    int nWidth, int nHeight,
    int nThreshold, int nMinLength, int nMaxGap,
    CImageBuffer& output)
{
    double maxRho = sqrt((double)(nWidth*nWidth + nHeight*nHeight));
    int nRho   = (int)(2 * maxRho) + 1;
    int nTheta = 180;

    std::vector<double> sinTab(nTheta), cosTab(nTheta);
    for (int t = 0; t < nTheta; t++)
    {
        double angle = t * M_PI / 180.0;
        sinTab[t] = sin(angle);
        cosTab[t] = cos(angle);
    }

    // Collect edge pixels
    std::vector<CPoint> edgePts;
    edgePts.reserve(nWidth * nHeight / 4);
    for (int y = 0; y < nHeight; y++)
        for (int x = 0; x < nWidth; x++)
            if (edges[y*nWidth+x])
                edgePts.push_back(CPoint(x, y));

    if (edgePts.empty()) return;

    // Shuffle for random sampling
    std::mt19937 rng(42);
    std::shuffle(edgePts.begin(), edgePts.end(), rng);

    // Working edge mask (to mark consumed pixels)
    std::vector<BYTE> mask(edges.begin(), edges.end());

    std::vector<int> acc(nRho * nTheta, 0);

    struct Segment { int x0,y0,x1,y1; };
    std::vector<Segment> segments;

    static const BYTE lineColors[][3] = {
        {255,0,0},{0,255,0},{0,0,255},{255,255,0},{255,0,255},{0,255,255}
    };
    const int nColCount = 6;

    for (const CPoint& pt : edgePts)
    {
        int px = pt.x, py = pt.y;
        if (!mask[py * nWidth + px]) continue;  // already consumed

        // Vote for this pixel
        for (int t = 0; t < nTheta; t++)
        {
            int rho = (int)(px * cosTab[t] + py * sinTab[t] + maxRho + 0.5);
            if (rho >= 0 && rho < nRho)
                acc[rho * nTheta + t]++;
        }

        // Find the best (rho,theta) for this pixel
        int bestVotes = -1, bestRho = -1, bestTheta = -1;
        for (int t = 0; t < nTheta; t++)
        {
            int rho = (int)(px * cosTab[t] + py * sinTab[t] + maxRho + 0.5);
            if (rho >= 0 && rho < nRho)
            {
                int v = acc[rho * nTheta + t];
                if (v > bestVotes) { bestVotes = v; bestRho = rho; bestTheta = t; }
            }
        }

        if (bestVotes < nThreshold) continue;

        // Found a line: extract segment along this rho/theta
        double theta = bestTheta * M_PI / 180.0;
        double cosT  = cosTab[bestTheta];
        double sinT  = sinTab[bestTheta];
        double rhoVal = bestRho - maxRho;

        // Walk along the line direction (perpendicular to normal)
        double lineX = -sinT, lineY = cosT;  // line direction vector

        // Project all edge pixels onto this line and find extents
        // Collect points near the (rho,theta) line
        std::vector<std::pair<double,CPoint>> onLine;
        for (int y2 = 0; y2 < nHeight; y2++)
            for (int x2 = 0; x2 < nWidth; x2++)
            {
                if (!mask[y2*nWidth+x2]) continue;
                double dist = fabs(x2 * cosT + y2 * sinT - rhoVal);
                if (dist < 1.5)
                {
                    double proj = x2 * lineX + y2 * lineY;
                    onLine.push_back({proj, CPoint(x2, y2)});
                }
            }

        if (onLine.empty()) continue;

        std::sort(onLine.begin(), onLine.end(),
            [](const std::pair<double,CPoint>& a, const std::pair<double,CPoint>& b){
                return a.first < b.first;
            });

        // Walk along sorted projections, extract segments (gap-limited)
        double segStart = onLine[0].first;
        double prevProj = onLine[0].first;
        CPoint ptStart  = onLine[0].second;
        CPoint ptEnd    = onLine[0].second;

        // Mark voted pixels as consumed
        for (auto& entry : onLine)
        {
            double proj = entry.first;
            CPoint p    = entry.second;

            if (proj - prevProj > nMaxGap)
            {
                // Gap exceeded - close current segment
                double segLen = prevProj - segStart;
                if (segLen >= nMinLength)
                    segments.push_back({ptStart.x, ptStart.y, ptEnd.x, ptEnd.y});
                segStart = proj;
                ptStart  = p;
            }

            prevProj = proj;
            ptEnd    = p;
            mask[p.y * nWidth + p.x] = 0;  // consume
        }

        // Close last segment
        double segLen = prevProj - segStart;
        if (segLen >= nMinLength)
            segments.push_back({ptStart.x, ptStart.y, ptEnd.x, ptEnd.y});

        // Remove this line's votes from accumulator
        for (auto& entry : onLine)
        {
            int x2 = entry.second.x, y2 = entry.second.y;
            for (int t = 0; t < nTheta; t++)
            {
                int rho2 = (int)(x2 * cosTab[t] + y2 * sinTab[t] + maxRho + 0.5);
                if (rho2 >= 0 && rho2 < nRho && acc[rho2 * nTheta + t] > 0)
                    acc[rho2 * nTheta + t]--;
            }
        }
    }

    // Draw detected segments
    for (int i = 0; i < (int)segments.size(); i++)
    {
        const Segment& s = segments[i];
        const BYTE* col  = lineColors[i % nColCount];
        DrawLine(output, s.x0, s.y0, s.x1, s.y1, col[0], col[1], col[2], 2);
    }
}

// -----------------------------------------------------------------------
bool CHoughLine::Process(const CImageBuffer& input, CImageBuffer& output)
{
    if (!input.IsValid()) return false;

    int nWidth    = input.GetWidth();
    int nHeight   = input.GetHeight();
    int nChannels = input.GetChannels();
    int nStride   = input.GetStride();

    int nMethod    = (int)m_params[0].dCurrentVal;
    int nThreshold = (int)m_params[1].dCurrentVal;
    int nMinLength = (int)m_params[2].dCurrentVal;
    int nMaxGap    = (int)m_params[3].dCurrentVal;

    // Convert to grayscale
    std::vector<BYTE> gray(nWidth * nHeight);
    const BYTE* pSrc = input.GetData();
    for (int y = 0; y < nHeight; y++)
    {
        const BYTE* pRow = pSrc + y * nStride;
        for (int x = 0; x < nWidth; x++)
        {
            if (nChannels == 1)
                gray[y*nWidth+x] = pRow[x];
            else
            {
                int g = (int)(0.299*pRow[x*3+2] + 0.587*pRow[x*3+1] + 0.114*pRow[x*3] + 0.5);
                gray[y*nWidth+x] = (BYTE)max(0, min(255, g));
            }
        }
    }

    // Edge detection: Sobel + threshold
    std::vector<BYTE> edges(nWidth * nHeight, 0);
    {
        static const int sobelGx[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
        static const int sobelGy[3][3] = {{-1,-2,-1},{0,0,0},{1,2,1}};
#pragma omp parallel for schedule(static)
        for (int y = 1; y < nHeight-1; y++)
            for (int x = 1; x < nWidth-1; x++)
            {
                int gx = 0, gy = 0;
                for (int ky = -1; ky <= 1; ky++)
                    for (int kx = -1; kx <= 1; kx++)
                    {
                        int p = gray[(y+ky)*nWidth+x+kx];
                        gx += p * sobelGx[ky+1][kx+1];
                        gy += p * sobelGy[ky+1][kx+1];
                    }
                int mag = (int)sqrt((double)(gx*gx + gy*gy));
                edges[y*nWidth+x] = (mag > 30) ? 255 : 0;
            }
    }

    // Create output image (3-channel for colored lines)
    if (nChannels == 1)
    {
        if (!output.Create(nWidth, nHeight, 3)) return false;
        BYTE* pDst   = output.GetData();
        int   nDstSt = output.GetStride();
        for (int y = 0; y < nHeight; y++)
        {
            BYTE* pRow = pDst + y * nDstSt;
            for (int x = 0; x < nWidth; x++)
            {
                BYTE v = gray[y*nWidth+x];
                pRow[x*3]=v; pRow[x*3+1]=v; pRow[x*3+2]=v;
            }
        }
    }
    else
    {
        output = input.Clone();
        if (!output.IsValid()) return false;
    }

    // Run selected method
    if (nMethod == 0)
        RunStandardHough(edges, nWidth, nHeight, nThreshold, output, gray);
    else
        RunProbabilisticHough(edges, nWidth, nHeight, nThreshold, nMinLength, nMaxGap, output);

    return true;
}

CAlgorithmBase* CHoughLine::Clone() const { return new CHoughLine(*this); }
