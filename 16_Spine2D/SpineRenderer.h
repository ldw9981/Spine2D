#pragma once

#include "Float2.h"

// 본 구조체
struct Bone {
    float x, y;
    float width, height;
    float rotation;
    D2D1_COLOR_F color;
};

// Spine2D 렌더러 클래스
class SpineRenderer {
public:
    SpineRenderer();
    ~SpineRenderer();

    // 초기화
    bool Initialize(HWND hwnd, int width, int height);
    void Shutdown();

    // 렌더링 시작/종료
    void BeginRender();
    void EndRender();
      
    // 애니메이션 제어
    void SetAnimation(const std::string& animationName);
    void UpdateAnimation(float deltaTime);
    
    // 키보드 입력 처리
    void HandleKeyInput(int keyCode);
    void SetNextAnimation();
    void SetPreviousAnimation();
    
    // UI 렌더링
    void RenderAnimationInfo();
    
    // 애니메이션 시간 설정
    void SetAnimationTime(float time);
    float GetAnimationTime() const { return m_animationTime; }

    // 화면 지우기
    void Clear(const D2D1_COLOR_F& color = { 0.1f, 0.1f, 0.1f, 1.0f });

    // Direct2D 리소스 접근자
    ComPtr<ID2D1RenderTarget> GetRenderTarget() const { return m_renderTarget; }
    ComPtr<ID2D1Factory> GetFactory() const { return m_factory; }


    bool CreateBitmapFromFile(const wchar_t* path, ID2D1Bitmap1** outBitmap);
private:
    // Direct2D 리소스들
    ComPtr<ID2D1Factory> m_factory;
    ComPtr<ID2D1RenderTarget> m_renderTarget;
    ComPtr<ID2D1SolidColorBrush> m_brush;
    ComPtr<ID2D1Bitmap> m_spriteBitmap;
    ComPtr<ID2D1Bitmap> m_spineBitmap;

    // DirectWrite 리소스들 (텍스트 출력용)
    Microsoft::WRL::ComPtr<IDWriteFactory> m_dwriteFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;

    // Direct3D 11 리소스들 (Direct2D와 연동)
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_deviceContext;
    ComPtr<IDXGISwapChain> m_swapChain;

	D2D1::Matrix3x2F m_UnityScreen; // 변환 행렬
public:
    // Spine2D 스켈레톤 데이터
    struct AtlasRegion {
        std::string name;
        int x, y;           // 아틀라스 내 위치 (좌상단)
        int width, height;  // 트림된 실제 저장 크기
        bool rotate;        // 이미지가 회전 저장되었는지 여부
        int orig_w, orig_h; // 원본 이미지 크기
        int offset_x, offset_y; // 트림 위치 (원본 이미지 안에서의 위치)
    };
    struct SpineBone {
        int index = -1; // 자신의 인덱스
        std::string name;
        int parentIndex = -1; // 부모 인덱스(-1이면 root)
		std::string parentName; // 부모 이름
        float x = 0, y = 0;
        float rotation = 0;
        float scaleX = 1, scaleY = 1;
        float length = 0;
        std::string color;
    };
    struct SpineSlot {
        std::string name;
        std::string bone;
        std::string attachment;
    };
    struct SpineAttachment {
        std::string name;
        std::string type;       
        float x = 0, y = 0;     // 슬롯 기준 위치
        float rotation = 0;     // 사용자 설정 회전
        float scaleX = 1, scaleY = 1;
        float width = 0, height = 0;
        std::string path;       // atlas의 region 이름
    };
    struct SpineSkin {
        std::string name;
        std::map<std::string, std::map<std::string, SpineAttachment>> attachments;
    };
	struct KeyFrameFloat2 {
		float time = 0;
		float x = 0;
		float y = 0;
		std::vector<float> curve;
	};
    struct KeyFrameFloat {
        float time = 0;
        float value = 0;
        std::vector<float> curve;
    };	

    struct BoneTimeline {       
        std::vector<KeyFrameFloat2> translate;
        std::vector<KeyFrameFloat> rotate;
        std::vector<KeyFrameFloat2> scale;
    };
    struct Animation {
        std::string name;
        std::map<std::string, BoneTimeline> boneTimelines;
        float duration = 0;
    };

public:

    // Spine 데이터
    std::vector<SpineBone> m_bones; // 0번 인덱스가 root, 각 본은 parentIndex로 부모 접근
    std::vector<SpineSlot> m_slots;
    std::vector<SpineSkin> m_skins;
    std::map<std::string, AtlasRegion> m_atlasRegions;
    std::map<std::string, Animation> m_animations;
    std::string m_currentSkin = "default";
    std::string m_currentAnimation = "idle";
    float m_currentAnimationTime = 0.0f;
    std::string m_prevAnimation = ""; // 0번 토글용 이전 애니메이션 저장
    
    // 윈도우 핸들
    HWND m_hwnd;
    // 클라이언트 영역 크기
    int m_clientWidth = 0;
    int m_clientHeight = 0;

    // 렌더링 상태
    bool m_initialized;
    
    // 애니메이션 시간
    float m_animationTime;
    float m_animationProgress;
   
	
    // 애니메이션 목록
    std::vector<std::string> m_animationList = {
        "aim",      // 1번 키
        "death",    // 2번 키
        "hit",      // 3번 키
        "idle",     // 4번 키
        "jump",     // 5번 키
        "run",      // 6번 키
        "shoot",    // 7번 키
        "walk"      // 8번 키
    };
    int m_currentAnimationIndex = 0;

    // 렌더 타겟 크기 가져오기
    D2D1_SIZE_F GetRenderTargetSize() const;

    // Direct3D 11 초기화
    bool InitializeD3D11();
    
    // Direct2D 초기화
    bool InitializeD2D1();
    
    // DirectWrite 초기화
    bool InitializeDWrite();
    
    // Spine2D 초기화 (실제 구현시)
    bool InitializeSpine2D();
    
    // 리소스 해제
    void ReleaseResources();
  
    
    // 디버그 정보 렌더링
    void RenderDebugInfo(const std::string& atlasPath, const std::string& jsonPath, float x, float y);
    
    // 스프라이트 비트맵 로드
    bool LoadSpriteBitmap(const std::string& imagePath);
    
    // Spine2D 스켈레톤 로드
    bool LoadSpineSkeleton(const std::string& jsonPath);
    
    // Spine2D 애니메이션 업데이트
    void UpdateSpineAnimation(float deltaTime);
    
    // Spine2D 스켈레톤 렌더링
    void Render();
    
    // 키프레임 보간
    float InterpolateKeyFrames(const std::vector<KeyFrameFloat>& keyframes, float time);
    Float2 InterpolateKeyFrames(const std::vector<KeyFrameFloat2>& keyframes, float time);
   
    bool LoadAtlas(const std::string& atlasPath);
    bool LoadSpineBitmap(const std::string& imagePath);
}; 