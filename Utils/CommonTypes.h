#pragma once

// Maximum supported image dimensions
#define MAX_IMAGE_WIDTH     5120
#define MAX_IMAGE_HEIGHT    5120

// Custom Windows messages for sequence processing
#define WM_SEQUENCE_PROGRESS    (WM_USER + 100)
#define WM_SEQUENCE_COMPLETE    (WM_USER + 101)
#define WM_SEQUENCE_ERROR       (WM_USER + 102)
#define WM_SEQUENCE_STEP_DONE   (WM_USER + 103)

// MiniViewer click notification (WPARAM = viewer index 0-7)
#define WM_MINIVIEWER_CLICKED   (WM_USER + 200)

// ROI updated notification (WPARAM = total ROI count)
#define WM_ROI_UPDATED          (WM_USER + 201)

// Parameter changed notification (triggers real-time preview)
#define WM_PARAM_CHANGED        (WM_USER + 202)

// Supported image file formats
enum class ImageFormat {
    BMP,
    JPEG,
    PNG,
    TIFF
};
