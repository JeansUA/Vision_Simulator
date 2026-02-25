#pragma once
#include "stdafx.h"

class CImageBuffer {
public:
    CImageBuffer();
    CImageBuffer(const CImageBuffer& other);
    CImageBuffer& operator=(const CImageBuffer& other);
    CImageBuffer(CImageBuffer&& other) noexcept;
    CImageBuffer& operator=(CImageBuffer&& other) noexcept;
    ~CImageBuffer();

    bool Create(int width, int height, int channels = 3);
    bool LoadFromFile(const CString& filePath);
    bool SaveToFile(const CString& filePath);
    CImageBuffer Clone() const;
    void Release();

    BYTE* GetData() const { return m_pData; }
    int GetWidth() const { return m_nWidth; }
    int GetHeight() const { return m_nHeight; }
    int GetChannels() const { return m_nChannels; }
    int GetStride() const { return m_nStride; }
    bool IsValid() const { return m_pData != nullptr && m_nWidth > 0 && m_nHeight > 0; }

    BYTE GetPixel(int x, int y, int ch = 0) const;
    void SetPixel(int x, int y, int ch, BYTE value);

    HBITMAP CreateHBitmap() const;
    CImageBuffer CreateThumbnail(int maxWidth, int maxHeight) const;

    // ROI region operations
    CImageBuffer ExtractRegion(const CRect& rcRegion) const;
    void PasteRegion(const CImageBuffer& source, int destX, int destY);

private:
    BYTE* m_pData;
    int m_nWidth;
    int m_nHeight;
    int m_nChannels;
    int m_nStride;

    void CopyFrom(const CImageBuffer& other);
};
