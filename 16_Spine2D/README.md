# 16_Spine2D - Spine2D 애니메이션 시스템

## 개요
이 프로젝트는 Direct2D와 Spine2D를 연동하여 2D 스켈레톤 애니메이션을 구현하는 예제입니다.

## 주요 기능
- **Spine2D 렌더러**: Direct2D를 사용한 Spine2D 애니메이션 렌더링
- **애니메이션 관리**: 스켈레톤 로드, 애니메이션 재생/정지/일시정지
- **키보드 컨트롤**: 다양한 애니메이션 전환 및 제어

## 프로젝트 구조
```
16_Spine2D/
├── Spine2D.cpp              # 메인 애플리케이션
├── SpineRenderer.h/cpp      # Spine2D 렌더러
├── SpineAnimation.h/cpp     # 애니메이션 관리
├── spine-data/              # Spine2D 데이터 파일
│   ├── spineboy.atlas       # 아틀라스 파일
│   ├── spineboy.json        # 스켈레톤 데이터
│   └── spineboy.png         # 스프라이트 이미지
└── 16_Spine2D.vcxproj       # Visual Studio 프로젝트
```

## 사용법

### 키보드 컨트롤
- **1**: Idle 애니메이션 재생
- **2**: Walk 애니메이션 재생
- **3**: Jump 애니메이션 재생
- **S**: 애니메이션 정지
- **P**: 애니메이션 일시정지/재개
- **ESC**: 프로그램 종료

### 코드 예시

```cpp
// Spine 렌더러 초기화
SpineRenderer renderer;
renderer.Initialize(hwnd);

// 애니메이션 관리자 초기화
SpineAnimation animManager;
animManager.LoadSkeleton("spineboy", "spine-data/spineboy.atlas", "spine-data/spineboy.json");

// 애니메이션 재생
animManager.PlayAnimation("spineboy", "walk");

// 게임 루프에서 업데이트
animManager.Update(deltaTime);
renderer.RenderSpineAnimation("spine-data/spineboy.atlas", "spine-data/spineboy.json", x, y, scale, rotation);
```

## Spine2D 라이브러리 설정

### 필요한 라이브러리
- Spine2D C++ 런타임
- Direct2D
- Direct3D 11
- DXGI

### 프로젝트 설정
1. Spine2D 라이브러리 다운로드 및 설치
2. 프로젝트에 include 경로 추가: `$(ProjectDir)spine-cpp/include`
3. 라이브러리 경로 추가: `$(ProjectDir)lib`
4. 링크 라이브러리 추가: `spine-cpp.lib`

## 구현 세부사항

### SpineRenderer 클래스
- Direct3D 11과 Direct2D 연동
- Spine2D 스켈레톤 렌더링
- 애니메이션 상태 관리

### SpineAnimation 클래스
- 스켈레톤 데이터 로드
- 애니메이션 재생/정지/일시정지
- 애니메이션 시간 관리
- 루프/비루프 애니메이션 지원

## 주의사항
- 실제 Spine2D 라이브러리가 필요합니다
- spineboy.png 파일은 실제 이미지 파일로 교체해야 합니다
- 현재는 기본 구조만 구현되어 있으며, 실제 Spine2D 렌더링은 추가 구현이 필요합니다

## 확장 가능한 기능
- 다중 스켈레톤 지원
- 애니메이션 블렌딩
- 이벤트 시스템
- 물리 연동
- 파티클 시스템 연동 