#pragma once

#include <spine/Atlas.h>
#include <spine/Skeleton.h>
#include <spine/AnimationState.h>
#include <spine/AnimationStateData.h>
#include <spine/SkeletonData.h>
#include <spine/AtlasAttachmentLoader.h>
#include <spine/SkeletonJson.h>
#include <spine/TextureLoader.h>
#include <wrl/client.h>
#include <d2d1_3.h>
#include <string>
#include <vector>
#include <map>

// Direct2D용 TextureLoader 구현
class Direct2DTextureLoader : public spine::TextureLoader {
public:
    Direct2DTextureLoader(ID2D1RenderTarget* renderTarget);
    virtual ~Direct2DTextureLoader();
    void load(spine::AtlasPage& page, const spine::String& path) override;
    void unload(void* texture) override;
private:
    ID2D1RenderTarget* m_renderTarget;
    std::map<std::string, Microsoft::WRL::ComPtr<ID2D1Bitmap>> m_bitmapMap;
};

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
    void SetAnimationTime(float time);
    float GetAnimationTime() const { return m_animationTime; }
    void SetNextAnimation();
    void SetPreviousAnimation();
          
    void HandleKeyInput(int keyCode);
    void Clear(const D2D1_COLOR_F& color = { 0.1f, 0.1f, 0.1f, 1.0f });
    D2D1_SIZE_F GetRenderTargetSize() const;

    // Direct2D 리소스 접근자
    Microsoft::WRL::ComPtr<ID2D1RenderTarget> GetRenderTarget() const { return m_renderTarget; }
    Microsoft::WRL::ComPtr<ID2D1Factory> GetFactory() const { return m_factory; }

    // 렌더링
    void Render();

	bool LoadSpine(const std::string& atlasPath, const std::string& jsonPath);

	void ReleaseSpine();

private:
    // Direct2D 리소스들
    Microsoft::WRL::ComPtr<ID2D1Factory> m_factory;
    Microsoft::WRL::ComPtr<ID2D1RenderTarget> m_renderTarget;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_brush;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_spineBitmap;

    // DirectWrite 리소스들
    Microsoft::WRL::ComPtr<IDWriteFactory> m_dwriteFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;

    // Direct3D 11 리소스들
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;

    D2D1::Matrix3x2F m_UnityScreen;

    // Spine-cpp 객체
    std::unique_ptr<Direct2DTextureLoader> m_textureLoader;

    std::unique_ptr<spine::Atlas> m_atlas;
    std::unique_ptr<spine::SkeletonData> m_skeletonData;
    std::unique_ptr<spine::Skeleton> m_skeleton;
    std::unique_ptr<spine::AnimationStateData> m_stateData;
    std::unique_ptr<spine::AnimationState> m_state;

    // 애니메이션 목록 및 상태
    std::vector<std::string> m_animationList;
    int m_currentAnimationIndex = 0;
    float m_animationTime = 0.0f;
    std::string m_currentAnimation = "idle";
    float m_currentAnimationTime = 0.0f;

    // 윈도우 핸들 및 크기
    HWND m_hwnd;
    int m_clientWidth = 0;
    int m_clientHeight = 0;
    bool m_initialized = false;
	D2D1_VECTOR_2F m_CharacterPosition = D2D1::Vector2F(0.0f, 0.0f);
    D2D1_VECTOR_2F m_CameraPosition = D2D1::Vector2F(0.0f, 300.0f);

    // 내부 함수들(Direct2D/3D 초기화 등)
    bool InitializeD3D11();
    bool InitializeD2D1();
    bool InitializeDWrite();
    void ReleaseResources();

	void ReleaseDirect2D();
}; 