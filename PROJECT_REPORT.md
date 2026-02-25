# Vision Simulator - Project Report

## Overview
MFC 기반 머신비전 알고리즘 테스트 프로그램. 이미지를 로드하고 다양한 영상처리 알고리즘을 시퀀스로 구성하여 실행할 수 있으며, 각 처리 단계의 결과를 미니 뷰어로 실시간 확인 가능.

## Key Features
- **이미지 뷰어**: Pan/Zoom 지원, 최대 5120x5120 해상도
- **6종 영상처리 알고리즘**: Grayscale, Binarize, Gaussian Blur, Edge Detect, Morphology, Brightness/Contrast
- **시퀀스 매니저**: 알고리즘을 순차적으로 조합하여 파이프라인 구성
- **파라미터 패널**: 슬라이더 기반 실시간 파라미터 조정
- **전처리 히스토리**: 8개 미니 뷰어로 각 처리 단계 결과 확인
- **동적 레이아웃**: 윈도우 크기에 따라 모든 컨트롤 자동 리사이징
- **멀티스레드**: UI 스레드와 시퀀스 처리 스레드 분리

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                  CVisionSimulatorApp                 │  Application Layer
│               (GDI+ Init, MFC Entry)                │
└──────────────────────┬──────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────┐
│                CVisionSimulatorDlg                   │  UI Layer
│    ┌──────────┬──────────────┬──────────────┐       │
│    │ImageViewer│ParameterPanel│  MiniViewer  │       │
│    │(Pan/Zoom) │ (Sliders)    │  (Thumbnail) │       │
│    └──────────┴──────────────┴──────────────┘       │
└──────────────────────┬──────────────────────────────┘
                       │ WM_SEQUENCE_* Messages
┌──────────────────────▼──────────────────────────────┐
│              CSequenceManager                       │  Core Layer
│         (Worker Thread Execution)                   │
└──────────────────────┬──────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────┐
│              CAlgorithmManager                      │  Algorithm Layer
│   ┌─────────┬─────────┬──────────┬────────────┐    │
│   │Grayscale│Binarize │GaussBlur │ EdgeDetect │    │
│   │Morphology│BrightContrast│    │            │    │
│   └─────────┴─────────┴──────────┴────────────┘    │
└─────────────────────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────┐
│              CImageBuffer                           │  Data Layer
│      (GDI+ Load/Save, Pixel Access, HBITMAP)        │
└─────────────────────────────────────────────────────┘
```

## File Structure (42 files)

```
Vision_Simulator/
│
├── VisionSimulator.sln                    # Visual Studio 솔루션 파일
├── VisionSimulator.vcxproj                # VS 프로젝트 파일 (v143, MFC Dynamic, Unicode)
├── VisionSimulator.vcxproj.filters        # VS 필터 구성
│
├── stdafx.h                               # 프리컴파일 헤더 (MFC, GDI+, STL)
├── stdafx.cpp                             # PCH 소스
├── targetver.h                            # Windows SDK 버전 타겟
├── resource.h                             # 리소스 ID 정의 (~50개)
├── VisionSimulator.rc                     # 리소스 스크립트 (다이얼로그, 아이콘, 버전)
│
├── VisionSimulatorApp.h                   # CWinApp 파생 애플리케이션 클래스
├── VisionSimulatorApp.cpp                 # GDI+ 초기화, 알고리즘 등록, 다이얼로그 생성
├── VisionSimulatorDlg.h                   # 메인 다이얼로그 클래스 (30+ 멤버 컨트롤)
├── VisionSimulatorDlg.cpp                 # 메인 UI 로직 (~430줄)
│                                          #   - 프로그래밍 방식 컨트롤 생성
│                                          #   - 동적 레이아웃 (OnSize)
│                                          #   - 시퀀스 실행/제어
│                                          #   - 미니뷰어 히스토리 업데이트
│
├── Core/                                  # 핵심 엔진
│   ├── ImageBuffer.h                      # 이미지 버퍼 클래스 선언
│   ├── ImageBuffer.cpp                    # 이미지 로드(GDI+), 저장, 썸네일 생성
│   │                                      #   - BMP/JPG/PNG/TIFF 지원
│   │                                      #   - 바이리니어 보간 썸네일
│   │                                      #   - 4바이트 정렬 stride
│   ├── SequenceManager.h                  # 시퀀스 매니저 선언
│   └── SequenceManager.cpp                # 워커 스레드 기반 시퀀스 실행
│                                          #   - CCriticalSection 동기화
│                                          #   - PostMessage 기반 진행상황 알림
│                                          #   - 중간 결과 히스토리 저장
│
├── Algorithm/                             # 영상처리 알고리즘
│   ├── AlgorithmBase.h                    # 알고리즘 추상 인터페이스
│   │                                      #   - AlgorithmParam 구조체
│   │                                      #   - Process(), Clone() 순수가상함수
│   ├── AlgorithmBase.cpp                  # (순수가상 - 빈 구현)
│   ├── AlgorithmManager.h                 # 싱글톤 알고리즘 팩토리
│   ├── AlgorithmManager.cpp               # Prototype 패턴 기반 알고리즘 생성
│   ├── Grayscale.h / .cpp                 # RGB→Gray (0.299R + 0.587G + 0.114B)
│   ├── Binarize.h / .cpp                  # 이진화 (Threshold: 0-255)
│   ├── GaussianBlur.h / .cpp              # 가우시안 블러 (Separable Convolution)
│   │                                      #   - KernelSize: 3-31 (홀수)
│   │                                      #   - Sigma: 0.1-10.0
│   ├── EdgeDetect.h / .cpp                # 에지 검출 (Sobel/Prewitt)
│   │                                      #   - Method: Sobel(0), Prewitt(1)
│   │                                      #   - Threshold: 0-255
│   ├── Morphology.h / .cpp                # 형태학 연산
│   │                                      #   - Operation: Erode/Dilate/Open/Close
│   │                                      #   - KernelSize: 3-21, Iterations: 1-10
│   └── BrightnessContrast.h / .cpp        # 밝기/대비 조절
│                                          #   - Brightness: -100~100
│                                          #   - Contrast: -100~100 (LUT 최적화)
│
├── UI/                                    # UI 컨트롤
│   ├── ImageViewer.h                      # 메인 이미지 뷰어 선언
│   ├── ImageViewer.cpp                    # Pan/Zoom 이미지 뷰어
│   │                                      #   - 마우스 휠 줌 (커서 중심)
│   │                                      #   - 드래그 팬
│   │                                      #   - HALFTONE 고품질 스케일링
│   │                                      #   - 다크 테마 배경 (RGB 45,45,48)
│   ├── MiniViewer.h                       # 미니 뷰어 선언
│   ├── MiniViewer.cpp                     # 썸네일 미니 뷰어
│   │                                      #   - 레이블 오버레이
│   │                                      #   - 더블 버퍼링
│   ├── ParameterPanel.h                   # 파라미터 패널 선언
│   └── ParameterPanel.cpp                 # 동적 파라미터 UI
│                                          #   - 알고리즘에 따라 자동 생성
│                                          #   - Label(35%) + Slider(45%) + Edit(20%)
│
├── Utils/                                 # 유틸리티
│   ├── CommonTypes.h                      # 공통 상수, 커스텀 메시지 정의
│   │                                      #   - WM_SEQUENCE_PROGRESS/COMPLETE/ERROR
│   │                                      #   - MAX_IMAGE_WIDTH/HEIGHT = 5120
│   └── Logger.h                           # OutputDebugString 기반 로거
│                                          #   - 타임스탬프 + 레벨 태그
│
└── res/                                   # 리소스
    ├── VisionSimulator.ico                # 애플리케이션 아이콘 (16x16 32bit)
    └── VisionSimulator.rc2                # 수동 리소스 편집용
```

## UI Layout

```
┌──────────────────────────────────────────────────────────────┐
│  [Load Image] [Run] [Stop] [Clear] [Save Result]  [Fit] 100%│  Toolbar
├──────────────────────────────────────┬───────────────────────┤
│                                      │ ┌─ Algorithm ───────┐│
│                                      │ │ [Combo Dropdown ▼] ││
│                                      │ │ [Add to Sequence]  ││
│                                      │ │ Param1: ═══■══ 128 ││
│        Main Image Viewer             │ │ Param2: ═══■══ 5   ││
│        (Pan/Zoom)                    │ │ Param3: ═══■══ 1.0 ││
│                                      │ └────────────────────┘│
│                                      │ ┌─ Sequence ────────┐│
│                                      │ │ 1. Grayscale       ││
│                                      │ │ 2. Gaussian Blur   ││
│                                      │ │ 3. Binarize        ││
│                                      │ │ [Remove][Up][Dn][C]││
│                                      │ └────────────────────┘│
├──────────────────────────────────────┴───────────────────────┤
│ ┌─ Processing History ────────────────────────────────────┐  │
│ │ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ... x8   │  │
│ │ │Orig  │ │Step1 │ │Step2 │ │Step3 │ │Result│           │  │
│ │ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘           │  │
│ └─────────────────────────────────────────────────────────┘  │
├──────────────────────────────────────────────────────────────┤
│ Status: Ready - Load an image to begin                       │
└──────────────────────────────────────────────────────────────┘
```

## Thread Architecture

```
┌─────────────────┐         ┌──────────────────────┐
│   UI Thread     │         │   Worker Thread       │
│   (Main)        │         │   (Sequence)          │
│                 │ Start   │                       │
│ OnBnClickedRun()├────────►│ CSequenceManager::    │
│                 │         │   DoExecute()         │
│                 │ WM_*    │                       │
│ OnSequence*()  ◄├─────────┤ PostMessage()         │
│                 │         │                       │
│ UpdateMiniViews│         │ Step1→Step2→...→Done  │
│ UpdateMainView │         │                       │
└─────────────────┘         └──────────────────────┘
```

## Design Patterns
| Pattern | Usage |
|---------|-------|
| **Singleton** | CAlgorithmManager - 전역 알고리즘 레지스트리 |
| **Prototype** | 알고리즘 Clone() - 파라미터 독립적 인스턴스 생성 |
| **Strategy** | CAlgorithmBase - 교체 가능한 알고리즘 인터페이스 |
| **Observer** | PostMessage - 워커 스레드→UI 스레드 알림 |
| **Template Method** | Process() - 각 알고리즘의 처리 로직 |

## Build Configuration
| Setting | Value |
|---------|-------|
| Platform Toolset | v143 (Visual Studio 2022) |
| MFC Usage | Dynamic Linking |
| Character Set | Unicode |
| Target Platform | Windows 10.0 |
| Configurations | Debug/Release x Win32/x64 |
| Additional Libs | gdiplus.lib |
| Precompiled Header | stdafx.h |

## How to Build
1. Visual Studio 2022에서 `VisionSimulator.sln` 열기
2. 구성 선택 (Debug/Release, Win32/x64)
3. 빌드 실행 (F7 또는 Ctrl+Shift+B)

## How to Use
1. **이미지 로드**: [Load Image] 버튼으로 BMP/JPG/PNG/TIFF 파일 선택
2. **알고리즘 선택**: 드롭다운에서 알고리즘 선택, 파라미터 조정
3. **시퀀스 구성**: [Add to Sequence] 버튼으로 알고리즘 추가, 순서 조정
4. **실행**: [Run] 버튼으로 시퀀스 실행
5. **결과 확인**: 메인 뷰어에서 최종 결과, 하단 미니 뷰어에서 각 단계 확인
6. **저장**: [Save Result] 버튼으로 결과 이미지 저장
