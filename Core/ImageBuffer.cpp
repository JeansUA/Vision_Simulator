#include "stdafx.h"
#include "Core/ImageBuffer.h"
#include "Utils/Logger.h"
#include "Utils/CommonTypes.h"

// ============================================================================
// Construction / Destruction
// ============================================================================

CImageBuffer::CImageBuffer()
    : m_pData(nullptr)
    , m_nWidth(0)
    , m_nHeight(0)
    , m_nChannels(0)
    , m_nStride(0)
{
}

CImageBuffer::CImageBuffer(const CImageBuffer& other)
    : m_pData(nullptr)
    , m_nWidth(0)
    , m_nHeight(0)
    , m_nChannels(0)
    , m_nStride(0)
{
    CopyFrom(other);
}

CImageBuffer& CImageBuffer::operator=(const CImageBuffer& other)
{
    if (this != &other)
    {
        CopyFrom(other);
    }
    return *this;
}

CImageBuffer::CImageBuffer(CImageBuffer&& other) noexcept
    : m_pData(other.m_pData)
    , m_nWidth(other.m_nWidth)
    , m_nHeight(other.m_nHeight)
    , m_nChannels(other.m_nChannels)
    , m_nStride(other.m_nStride)
{
    other.m_pData    = nullptr;
    other.m_nWidth   = 0;
    other.m_nHeight  = 0;
    other.m_nChannels = 0;
    other.m_nStride  = 0;
}

CImageBuffer& CImageBuffer::operator=(CImageBuffer&& other) noexcept
{
    if (this != &other)
    {
        Release();
        m_pData    = other.m_pData;
        m_nWidth   = other.m_nWidth;
        m_nHeight  = other.m_nHeight;
        m_nChannels = other.m_nChannels;
        m_nStride  = other.m_nStride;
        other.m_pData    = nullptr;
        other.m_nWidth   = 0;
        other.m_nHeight  = 0;
        other.m_nChannels = 0;
        other.m_nStride  = 0;
    }
    return *this;
}

CImageBuffer::~CImageBuffer()
{
    Release();
}

// ============================================================================
// Core Operations
// ============================================================================

bool CImageBuffer::Create(int width, int height, int channels)
{
    if (width <= 0 || height <= 0 || channels <= 0)
    {
        CLogger::Error(_T("CImageBuffer::Create - Invalid parameters: %d x %d x %d"), width, height, channels);
        return false;
    }

    if (width > MAX_IMAGE_WIDTH || height > MAX_IMAGE_HEIGHT)
    {
        CLogger::Error(_T("CImageBuffer::Create - Dimensions exceed maximum: %d x %d (max %d x %d)"),
            width, height, MAX_IMAGE_WIDTH, MAX_IMAGE_HEIGHT);
        return false;
    }

    // Smart reuse: skip reallocation if dimensions unchanged (eliminates page faults)
    if (m_pData && m_nWidth == width && m_nHeight == height && m_nChannels == channels)
        return true;

    Release();

    m_nWidth = width;
    m_nHeight = height;
    m_nChannels = channels;
    // Stride aligned to 4-byte boundary
    m_nStride = (width * channels + 3) & ~3;

    size_t bufferSize = static_cast<size_t>(m_nStride) * m_nHeight;

    try
    {
        m_pData = new BYTE[bufferSize];
    }
    catch (const std::bad_alloc&)
    {
        m_pData = nullptr;
        m_nWidth = 0;
        m_nHeight = 0;
        m_nChannels = 0;
        m_nStride = 0;
        return false;
    }

    return true;
}

bool CImageBuffer::LoadFromFile(const CString& filePath)
{
    if (filePath.IsEmpty())
    {
        CLogger::Error(_T("CImageBuffer::LoadFromFile - Empty file path"));
        return false;
    }

    Gdiplus::Bitmap bmp(filePath);
    if (bmp.GetLastStatus() != Gdiplus::Ok)
    {
        CLogger::Error(_T("CImageBuffer::LoadFromFile - Failed to load: %s"), (LPCTSTR)filePath);
        return false;
    }

    int srcWidth = static_cast<int>(bmp.GetWidth());
    int srcHeight = static_cast<int>(bmp.GetHeight());

    if (srcWidth <= 0 || srcHeight <= 0)
    {
        CLogger::Error(_T("CImageBuffer::LoadFromFile - Invalid image dimensions: %d x %d"), srcWidth, srcHeight);
        return false;
    }

    if (srcWidth > MAX_IMAGE_WIDTH || srcHeight > MAX_IMAGE_HEIGHT)
    {
        CLogger::Error(_T("CImageBuffer::LoadFromFile - Image too large: %d x %d (max %d x %d)"),
            srcWidth, srcHeight, MAX_IMAGE_WIDTH, MAX_IMAGE_HEIGHT);
        return false;
    }

    // Determine source pixel format and target channels
    Gdiplus::PixelFormat srcFormat = bmp.GetPixelFormat();
    int targetChannels = 3; // Default to RGB

    // Check if source is grayscale
    bool isGrayscale = (srcFormat == PixelFormat8bppIndexed);

    if (isGrayscale)
    {
        targetChannels = 1;
    }

    if (!Create(srcWidth, srcHeight, targetChannels))
    {
        return false;
    }

    // Lock the source bitmap for reading
    Gdiplus::Rect lockRect(0, 0, srcWidth, srcHeight);
    Gdiplus::BitmapData bmpData;

    Gdiplus::PixelFormat lockFormat;
    if (targetChannels == 1)
    {
        // For grayscale, we need to handle carefully
        // Lock as 24bpp and convert, since 8bpp indexed LockBits can be tricky
        lockFormat = PixelFormat24bppRGB;
    }
    else
    {
        lockFormat = PixelFormat24bppRGB;
    }

    Gdiplus::Status lockStatus = bmp.LockBits(&lockRect, Gdiplus::ImageLockModeRead, lockFormat, &bmpData);
    if (lockStatus != Gdiplus::Ok)
    {
        CLogger::Error(_T("CImageBuffer::LoadFromFile - LockBits failed with status %d"), lockStatus);
        Release();
        return false;
    }

    BYTE* pSrcData = static_cast<BYTE*>(bmpData.Scan0);
    int srcStride = bmpData.Stride;

    if (targetChannels == 1 && isGrayscale)
    {
        // Convert from 24bpp RGB to grayscale (average of R,G,B -- which should be equal for grayscale)
        for (int y = 0; y < srcHeight; y++)
        {
            BYTE* pSrcRow = pSrcData + y * srcStride;
            BYTE* pDstRow = m_pData + y * m_nStride;
            for (int x = 0; x < srcWidth; x++)
            {
                BYTE b = pSrcRow[x * 3 + 0];
                BYTE g = pSrcRow[x * 3 + 1];
                BYTE r = pSrcRow[x * 3 + 2];
                // Standard luminance conversion
                pDstRow[x] = static_cast<BYTE>((r * 299 + g * 587 + b * 114) / 1000);
            }
        }
    }
    else
    {
        // Copy 24bpp RGB data (GDI+ stores as BGR)
        // We store as RGB internally
        for (int y = 0; y < srcHeight; y++)
        {
            BYTE* pSrcRow = pSrcData + y * srcStride;
            BYTE* pDstRow = m_pData + y * m_nStride;
            for (int x = 0; x < srcWidth; x++)
            {
                pDstRow[x * 3 + 0] = pSrcRow[x * 3 + 2]; // R (from B position in BGR)
                pDstRow[x * 3 + 1] = pSrcRow[x * 3 + 1]; // G
                pDstRow[x * 3 + 2] = pSrcRow[x * 3 + 0]; // B (from R position in BGR)
            }
        }
    }

    bmp.UnlockBits(&bmpData);

    CLogger::Info(_T("CImageBuffer::LoadFromFile - Loaded %s (%d x %d, %d ch)"),
        (LPCTSTR)filePath, m_nWidth, m_nHeight, m_nChannels);
    return true;
}

bool CImageBuffer::SaveToFile(const CString& filePath)
{
    if (!IsValid())
    {
        CLogger::Error(_T("CImageBuffer::SaveToFile - Buffer is not valid"));
        return false;
    }

    if (filePath.IsEmpty())
    {
        CLogger::Error(_T("CImageBuffer::SaveToFile - Empty file path"));
        return false;
    }

    // Create GDI+ bitmap from our data
    Gdiplus::Bitmap* pBitmap = nullptr;

    if (m_nChannels == 1)
    {
        // Grayscale: create 24bpp bitmap and fill with gray values
        pBitmap = new Gdiplus::Bitmap(m_nWidth, m_nHeight, PixelFormat24bppRGB);
        if (pBitmap->GetLastStatus() != Gdiplus::Ok)
        {
            CLogger::Error(_T("CImageBuffer::SaveToFile - Failed to create GDI+ bitmap"));
            delete pBitmap;
            return false;
        }

        Gdiplus::Rect lockRect(0, 0, m_nWidth, m_nHeight);
        Gdiplus::BitmapData bmpData;
        pBitmap->LockBits(&lockRect, Gdiplus::ImageLockModeWrite, PixelFormat24bppRGB, &bmpData);

        BYTE* pDst = static_cast<BYTE*>(bmpData.Scan0);
        for (int y = 0; y < m_nHeight; y++)
        {
            BYTE* pSrcRow = m_pData + y * m_nStride;
            BYTE* pDstRow = pDst + y * bmpData.Stride;
            for (int x = 0; x < m_nWidth; x++)
            {
                BYTE gray = pSrcRow[x];
                pDstRow[x * 3 + 0] = gray; // B
                pDstRow[x * 3 + 1] = gray; // G
                pDstRow[x * 3 + 2] = gray; // R
            }
        }
        pBitmap->UnlockBits(&bmpData);
    }
    else if (m_nChannels == 3)
    {
        // RGB: create 24bpp bitmap and convert RGB to BGR
        pBitmap = new Gdiplus::Bitmap(m_nWidth, m_nHeight, PixelFormat24bppRGB);
        if (pBitmap->GetLastStatus() != Gdiplus::Ok)
        {
            CLogger::Error(_T("CImageBuffer::SaveToFile - Failed to create GDI+ bitmap"));
            delete pBitmap;
            return false;
        }

        Gdiplus::Rect lockRect(0, 0, m_nWidth, m_nHeight);
        Gdiplus::BitmapData bmpData;
        pBitmap->LockBits(&lockRect, Gdiplus::ImageLockModeWrite, PixelFormat24bppRGB, &bmpData);

        BYTE* pDst = static_cast<BYTE*>(bmpData.Scan0);
        for (int y = 0; y < m_nHeight; y++)
        {
            BYTE* pSrcRow = m_pData + y * m_nStride;
            BYTE* pDstRow = pDst + y * bmpData.Stride;
            for (int x = 0; x < m_nWidth; x++)
            {
                pDstRow[x * 3 + 0] = pSrcRow[x * 3 + 2]; // B (from our R)
                pDstRow[x * 3 + 1] = pSrcRow[x * 3 + 1]; // G
                pDstRow[x * 3 + 2] = pSrcRow[x * 3 + 0]; // R (from our B)
            }
        }
        pBitmap->UnlockBits(&bmpData);
    }
    else
    {
        CLogger::Error(_T("CImageBuffer::SaveToFile - Unsupported channel count: %d"), m_nChannels);
        return false;
    }

    // Determine encoder CLSID based on file extension
    CString ext = filePath.Mid(filePath.ReverseFind(_T('.')) + 1);
    ext.MakeLower();

    CLSID encoderClsid;
    bool foundEncoder = false;

    UINT numEncoders = 0;
    UINT encoderSize = 0;
    Gdiplus::GetImageEncodersSize(&numEncoders, &encoderSize);

    if (encoderSize > 0)
    {
        Gdiplus::ImageCodecInfo* pEncoders = static_cast<Gdiplus::ImageCodecInfo*>(malloc(encoderSize));
        if (pEncoders != nullptr)
        {
            Gdiplus::GetImageEncoders(numEncoders, encoderSize, pEncoders);

            CString mimeType;
            if (ext == _T("bmp"))
                mimeType = _T("image/bmp");
            else if (ext == _T("jpg") || ext == _T("jpeg"))
                mimeType = _T("image/jpeg");
            else if (ext == _T("png"))
                mimeType = _T("image/png");
            else if (ext == _T("tif") || ext == _T("tiff"))
                mimeType = _T("image/tiff");
            else
                mimeType = _T("image/bmp"); // Default to BMP

            for (UINT i = 0; i < numEncoders; i++)
            {
                if (CString(pEncoders[i].MimeType) == mimeType)
                {
                    encoderClsid = pEncoders[i].Clsid;
                    foundEncoder = true;
                    break;
                }
            }
            free(pEncoders);
        }
    }

    bool success = false;
    if (foundEncoder)
    {
        Gdiplus::Status status = pBitmap->Save(filePath, &encoderClsid, nullptr);
        success = (status == Gdiplus::Ok);
        if (!success)
        {
            CLogger::Error(_T("CImageBuffer::SaveToFile - Save failed with status %d"), status);
        }
    }
    else
    {
        CLogger::Error(_T("CImageBuffer::SaveToFile - No encoder found for extension: %s"), (LPCTSTR)ext);
    }

    delete pBitmap;

    if (success)
    {
        CLogger::Info(_T("CImageBuffer::SaveToFile - Saved %s"), (LPCTSTR)filePath);
    }
    return success;
}

CImageBuffer CImageBuffer::Clone() const
{
    CImageBuffer clone;
    if (IsValid())
    {
        clone.CopyFrom(*this);
    }
    return clone;
}

void CImageBuffer::Release()
{
    if (m_pData != nullptr)
    {
        delete[] m_pData;
        m_pData = nullptr;
    }
    m_nWidth = 0;
    m_nHeight = 0;
    m_nChannels = 0;
    m_nStride = 0;
}

// ============================================================================
// Pixel Access
// ============================================================================

BYTE CImageBuffer::GetPixel(int x, int y, int ch) const
{
    if (!IsValid())
        return 0;

    if (x < 0 || x >= m_nWidth || y < 0 || y >= m_nHeight || ch < 0 || ch >= m_nChannels)
        return 0;

    return m_pData[y * m_nStride + x * m_nChannels + ch];
}

void CImageBuffer::SetPixel(int x, int y, int ch, BYTE value)
{
    if (!IsValid())
        return;

    if (x < 0 || x >= m_nWidth || y < 0 || y >= m_nHeight || ch < 0 || ch >= m_nChannels)
        return;

    m_pData[y * m_nStride + x * m_nChannels + ch] = value;
}

// ============================================================================
// Bitmap Conversion
// ============================================================================

HBITMAP CImageBuffer::CreateHBitmap() const
{
    if (!IsValid())
        return nullptr;

    HDC hDC = ::GetDC(nullptr);
    if (hDC == nullptr)
        return nullptr;

    HBITMAP hBitmap = nullptr;

    if (m_nChannels == 1)
    {
        // Grayscale image: create bitmap with 256-entry palette
        size_t infoSize = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
        BITMAPINFO* pBmi = static_cast<BITMAPINFO*>(malloc(infoSize));
        if (pBmi == nullptr)
        {
            ::ReleaseDC(nullptr, hDC);
            return nullptr;
        }

        memset(pBmi, 0, infoSize);
        pBmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        pBmi->bmiHeader.biWidth = m_nWidth;
        pBmi->bmiHeader.biHeight = -m_nHeight; // Top-down
        pBmi->bmiHeader.biPlanes = 1;
        pBmi->bmiHeader.biBitCount = 8;
        pBmi->bmiHeader.biCompression = BI_RGB;
        pBmi->bmiHeader.biSizeImage = m_nStride * m_nHeight;

        // Create grayscale palette
        for (int i = 0; i < 256; i++)
        {
            pBmi->bmiColors[i].rgbRed = static_cast<BYTE>(i);
            pBmi->bmiColors[i].rgbGreen = static_cast<BYTE>(i);
            pBmi->bmiColors[i].rgbBlue = static_cast<BYTE>(i);
            pBmi->bmiColors[i].rgbReserved = 0;
        }

        BYTE* pBits = nullptr;
        hBitmap = ::CreateDIBSection(hDC, pBmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBits), nullptr, 0);

        if (hBitmap != nullptr && pBits != nullptr)
        {
            // DIB stride for 8bpp
            int dibStride = (m_nWidth + 3) & ~3;
            for (int y = 0; y < m_nHeight; y++)
            {
                memcpy(pBits + y * dibStride, m_pData + y * m_nStride, m_nWidth);
            }
        }

        free(pBmi);
    }
    else if (m_nChannels == 3)
    {
        // RGB image: create 24bpp bitmap, convert RGB to BGR
        BITMAPINFO bmi;
        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = m_nWidth;
        bmi.bmiHeader.biHeight = -m_nHeight; // Top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;

        BYTE* pBits = nullptr;
        hBitmap = ::CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBits), nullptr, 0);

        if (hBitmap != nullptr && pBits != nullptr)
        {
            // DIB stride for 24bpp
            int dibStride = (m_nWidth * 3 + 3) & ~3;
            for (int y = 0; y < m_nHeight; y++)
            {
                BYTE* pSrcRow = m_pData + y * m_nStride;
                BYTE* pDstRow = pBits + y * dibStride;
                for (int x = 0; x < m_nWidth; x++)
                {
                    pDstRow[x * 3 + 0] = pSrcRow[x * 3 + 2]; // B (from our R position)
                    pDstRow[x * 3 + 1] = pSrcRow[x * 3 + 1]; // G
                    pDstRow[x * 3 + 2] = pSrcRow[x * 3 + 0]; // R (from our B position)
                }
            }
        }
    }

    ::ReleaseDC(nullptr, hDC);
    return hBitmap;
}

// ============================================================================
// Thumbnail Generation
// ============================================================================

CImageBuffer CImageBuffer::CreateThumbnail(int maxWidth, int maxHeight) const
{
    CImageBuffer thumbnail;

    if (!IsValid() || maxWidth <= 0 || maxHeight <= 0)
        return thumbnail;

    // Calculate scale factor preserving aspect ratio
    double scaleX = static_cast<double>(maxWidth) / m_nWidth;
    double scaleY = static_cast<double>(maxHeight) / m_nHeight;
    double scale = min(scaleX, scaleY);

    // Clamp scale to not enlarge
    if (scale > 1.0)
        scale = 1.0;

    int newWidth = static_cast<int>(m_nWidth * scale);
    int newHeight = static_cast<int>(m_nHeight * scale);

    if (newWidth <= 0) newWidth = 1;
    if (newHeight <= 0) newHeight = 1;

    if (!thumbnail.Create(newWidth, newHeight, m_nChannels))
        return thumbnail;

    // Bilinear interpolation
    for (int y = 0; y < newHeight; y++)
    {
        double srcY = static_cast<double>(y) / scale;
        int y0 = static_cast<int>(srcY);
        int y1 = min(y0 + 1, m_nHeight - 1);
        double fy = srcY - y0;

        for (int x = 0; x < newWidth; x++)
        {
            double srcX = static_cast<double>(x) / scale;
            int x0 = static_cast<int>(srcX);
            int x1 = min(x0 + 1, m_nWidth - 1);
            double fx = srcX - x0;

            for (int ch = 0; ch < m_nChannels; ch++)
            {
                // Four neighboring pixels
                double v00 = GetPixel(x0, y0, ch);
                double v10 = GetPixel(x1, y0, ch);
                double v01 = GetPixel(x0, y1, ch);
                double v11 = GetPixel(x1, y1, ch);

                // Bilinear interpolation
                double value = v00 * (1.0 - fx) * (1.0 - fy)
                             + v10 * fx * (1.0 - fy)
                             + v01 * (1.0 - fx) * fy
                             + v11 * fx * fy;

                // Clamp to valid range
                int iValue = static_cast<int>(value + 0.5);
                if (iValue < 0) iValue = 0;
                if (iValue > 255) iValue = 255;

                thumbnail.SetPixel(x, y, ch, static_cast<BYTE>(iValue));
            }
        }
    }

    CLogger::Info(_T("CImageBuffer::CreateThumbnail - Created %d x %d thumbnail from %d x %d source"),
        newWidth, newHeight, m_nWidth, m_nHeight);
    return thumbnail;
}

// ============================================================================
// ROI Region Operations
// ============================================================================

CImageBuffer CImageBuffer::ExtractRegion(const CRect& rcRegion) const
{
    CImageBuffer region;
    if (!IsValid())
        return region;

    // Clamp region to image bounds
    int x0 = max(0, (int)rcRegion.left);
    int y0 = max(0, (int)rcRegion.top);
    int x1 = min(m_nWidth, (int)rcRegion.right);
    int y1 = min(m_nHeight, (int)rcRegion.bottom);

    int regW = x1 - x0;
    int regH = y1 - y0;

    if (regW <= 0 || regH <= 0)
        return region;

    if (!region.Create(regW, regH, m_nChannels))
        return region;

    int bytesPerRow = regW * m_nChannels;
    for (int y = 0; y < regH; y++)
    {
        const BYTE* pSrcRow = m_pData + (y + y0) * m_nStride + x0 * m_nChannels;
        BYTE* pDstRow = region.m_pData + y * region.m_nStride;
        memcpy(pDstRow, pSrcRow, bytesPerRow);
    }

    return region;
}

// Copy pixel data into this buffer reusing existing allocation when possible
bool CImageBuffer::CopyDataFrom(const CImageBuffer& src)
{
    if (!src.IsValid()) return false;
    if (!Create(src.m_nWidth, src.m_nHeight, src.m_nChannels)) return false;
    const int rowBytes = m_nWidth * m_nChannels;
    for (int y = 0; y < m_nHeight; y++)
        memcpy(m_pData + y * m_nStride, src.m_pData + y * src.m_nStride, rowBytes);
    return true;
}

// Extract a ROI into an existing buffer (no allocation if dst already matches size)
bool CImageBuffer::ExtractRegionInto(const CRect& rc, CImageBuffer& dst) const
{
    if (!IsValid()) return false;
    int x0 = max(0, (int)rc.left),  y0 = max(0, (int)rc.top);
    int x1 = min(m_nWidth, (int)rc.right), y1 = min(m_nHeight, (int)rc.bottom);
    int regW = x1 - x0, regH = y1 - y0;
    if (regW <= 0 || regH <= 0) return false;
    if (!dst.Create(regW, regH, m_nChannels)) return false;
    const int rowBytes = regW * m_nChannels;
    for (int y = 0; y < regH; y++)
        memcpy(dst.m_pData + y * dst.m_nStride,
               m_pData + (y + y0) * m_nStride + x0 * m_nChannels, rowBytes);
    return true;
}

void CImageBuffer::PasteRegion(const CImageBuffer& source, int destX, int destY)
{
    if (!IsValid() || !source.IsValid())
        return;

    int srcW = source.GetWidth();
    int srcH = source.GetHeight();
    int srcCh = source.GetChannels();
    int dstCh = m_nChannels;

    // Fast path: same channel count - row-based memcpy
    if (srcCh == dstCh)
    {
        for (int y = 0; y < srcH; y++)
        {
            int dy = destY + y;
            if (dy < 0 || dy >= m_nHeight)
                continue;

            // Calculate horizontal clipping
            int srcStartX = 0;
            int dstStartX = destX;
            if (dstStartX < 0) { srcStartX = -dstStartX; dstStartX = 0; }
            int copyW = srcW - srcStartX;
            if (dstStartX + copyW > m_nWidth) copyW = m_nWidth - dstStartX;
            if (copyW <= 0)
                continue;

            const BYTE* pSrc = source.m_pData + y * source.m_nStride + srcStartX * srcCh;
            BYTE* pDst = m_pData + dy * m_nStride + dstStartX * dstCh;
            memcpy(pDst, pSrc, copyW * dstCh);
        }
    }
    else if (srcCh == 1 && dstCh == 3)
    {
        // Grayscale -> RGB: replicate gray to all channels
        for (int y = 0; y < srcH; y++)
        {
            int dy = destY + y;
            if (dy < 0 || dy >= m_nHeight)
                continue;

            for (int x = 0; x < srcW; x++)
            {
                int dx = destX + x;
                if (dx < 0 || dx >= m_nWidth)
                    continue;

                BYTE gray = source.m_pData[y * source.m_nStride + x];
                BYTE* pDst = m_pData + dy * m_nStride + dx * 3;
                pDst[0] = gray;
                pDst[1] = gray;
                pDst[2] = gray;
            }
        }
    }
    else if (srcCh == 3 && dstCh == 1)
    {
        // RGB -> Grayscale: luminance conversion
        for (int y = 0; y < srcH; y++)
        {
            int dy = destY + y;
            if (dy < 0 || dy >= m_nHeight)
                continue;

            for (int x = 0; x < srcW; x++)
            {
                int dx = destX + x;
                if (dx < 0 || dx >= m_nWidth)
                    continue;

                const BYTE* pSrc = source.m_pData + y * source.m_nStride + x * 3;
                BYTE gray = (BYTE)((pSrc[0] * 299 + pSrc[1] * 587 + pSrc[2] * 114) / 1000);
                m_pData[dy * m_nStride + dx] = gray;
            }
        }
    }
}

// ============================================================================
// Private Helpers
// ============================================================================

void CImageBuffer::CopyFrom(const CImageBuffer& other)
{
    if (!other.IsValid())
    {
        Release();
        return;
    }

    if (!Create(other.m_nWidth, other.m_nHeight, other.m_nChannels))
    {
        return;
    }

    // Copy row by row in case strides differ (they shouldn't, but be safe)
    if (m_nStride == other.m_nStride)
    {
        memcpy(m_pData, other.m_pData, static_cast<size_t>(m_nStride) * m_nHeight);
    }
    else
    {
        int copyBytes = min(m_nStride, other.m_nStride);
        for (int y = 0; y < m_nHeight; y++)
        {
            memcpy(m_pData + y * m_nStride, other.m_pData + y * other.m_nStride, copyBytes);
        }
    }
}
