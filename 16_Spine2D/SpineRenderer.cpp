#include "Common.h"

#include "SpineRenderer.h"
#include "json.hpp" // nlohmann::json 사용
#include <wincodec.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <spine/Atlas.h>
#include <spine/SkeletonJson.h>
#include <spine/AnimationState.h>
#include <spine/AnimationStateData.h>
#include <spine/Skeleton.h>
#include <spine/AtlasAttachmentLoader.h>
#include <spine/TextureLoader.h>
#include <spine/Slot.h>
#include <spine/RegionAttachment.h>
#include <spine/SkeletonClipping.h>
#include <spine/Bone.h>
#include <spine/Animation.h>
#include <spine/BoneData.h>

#pragma comment(lib, "d3d11.lib")

#include <d2d1_3.h> //ID2D1Factory8,ID2D1DeviceContext7
#pragma comment(lib, "d2d1.lib")

#include <dxgi1_6.h> // IDXGIFactory7
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib,"dwrite.lib")


// 콘솔 인코딩을 UTF-8로 설정
void SetConsoleUTF8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}




// Direct2DTextureLoader 구현
Direct2DTextureLoader::Direct2DTextureLoader(ID2D1RenderTarget* renderTarget)
    : m_renderTarget(renderTarget) {}
Direct2DTextureLoader::~Direct2DTextureLoader() {}
void Direct2DTextureLoader::load(spine::AtlasPage& page, const spine::String& path) {
    std::wstring wPath(path.buffer(), path.buffer() + path.length());
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
    if (FAILED(hr))
        return;
    
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    hr = wicFactory->CreateDecoderFromFilename(wPath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) 
        return;

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) 
        return;

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) 
        return;
    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) 
        return;
    
    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
    hr = m_renderTarget->CreateBitmapFromWicBitmap(converter.Get(), bitmap.GetAddressOf());
    if (FAILED(hr)) 
        return;
    
    page.texture = bitmap.Get();
    m_bitmapMap[path.buffer()] = bitmap;
    page.width = static_cast<int>(bitmap->GetSize().width);
    page.height = static_cast<int>(bitmap->GetSize().height);
}
void Direct2DTextureLoader::unload(void* texture) {
    // Direct2D 비트맵은 ComPtr로 관리하므로 별도 해제 불필요
}


SpineRenderer::SpineRenderer() : m_textureLoader(nullptr), m_atlas(nullptr), m_skeletonData(nullptr), m_skeleton(nullptr), m_stateData(nullptr), m_state(nullptr) {}
SpineRenderer::~SpineRenderer() { Shutdown(); }

bool SpineRenderer::Initialize(HWND hwnd,int width,int height) {
    m_hwnd = hwnd;
    
	m_UnityScreen = D2D1::Matrix3x2F::Scale(1.0f, -1.0f) *
		D2D1::Matrix3x2F::Translation((float)width / 2, (float)height/2 );


    // 콘솔 인코딩 설정
    SetConsoleUTF8();
  
    m_clientWidth = width;
    m_clientHeight = height;
    
    if (!InitializeD3D11()) {
        std::cout << "Direct3D 11 initialization failed" << std::endl;
        return false;
    }
    
    if (!InitializeD2D1()) {
        std::cout << "Direct2D 1 initialization failed" << std::endl;
        return false;
    }
    
    if (!InitializeDWrite()) {
        std::cout << "DirectWrite initialization failed" << std::endl;
        return false;
    }
    // spine-cpp 기반 Spine 데이터 로드
    if (!LoadSpine("../Resource/spineboy-ess.atlas", "../Resource/spineboy-ess.json")) {
        std::cout << "Spine2D(spine-cpp) initialization failed" << std::endl;
        return false;
    }
    m_initialized = true;
    return true;
}

void SpineRenderer::Shutdown() {
    ReleaseResources();
}

bool SpineRenderer::InitializeD3D11() {
    // Direct3D 11 디바이스 생성
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    
    D3D_FEATURE_LEVEL featureLevel;
    
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_device,
        &featureLevel,
        &m_deviceContext
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    // DXGI 팩토리 생성
    ComPtr<IDXGIFactory> dxgiFactory;
    hr = CreateDXGIFactory(__uuidof(IDXGIFactory), &dxgiFactory);
    if (FAILED(hr)) {
        return false;
    }
    
    // 스왑체인 생성
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = m_clientWidth;
    swapChainDesc.BufferDesc.Height = m_clientHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = m_hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    
    hr = dxgiFactory->CreateSwapChain(m_device.Get(), &swapChainDesc, &m_swapChain);
    if (FAILED(hr)) {
        return false;
    }
    
    return true;
}

bool SpineRenderer::InitializeD2D1() {
    // Direct2D 1 팩토리 생성
    D2D1_FACTORY_OPTIONS options = {};
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
    
    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        options,
        m_factory.GetAddressOf()
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    // Direct2D 렌더 타겟 생성
    ComPtr<IDXGISurface> dxgiSurface;
    hr = m_swapChain->GetBuffer(0, __uuidof(IDXGISurface), &dxgiSurface);
    if (FAILED(hr)) {
        return false;
    }
    
    D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = {};
    renderTargetProperties.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    renderTargetProperties.pixelFormat = D2D1::PixelFormat(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        D2D1_ALPHA_MODE_PREMULTIPLIED
    );
    
    hr = m_factory->CreateDxgiSurfaceRenderTarget(
        dxgiSurface.Get(),
        renderTargetProperties,
        &m_renderTarget
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    // 브러시 생성
    hr = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White),
        &m_brush
    );
    
    if (FAILED(hr)) {
        return false;
    }
    
    return true;
}

bool SpineRenderer::InitializeDWrite() {
    HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_dwriteFactory.GetAddressOf())
    );
    if (FAILED(hr)) {
        std::cout << "DWriteCreateFactory failed" << std::endl;
        return false;
    }
    hr = m_dwriteFactory->CreateTextFormat(
        L"맑은 고딕", // 한글 폰트
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        16.0f,
        L"ko-kr",
        &m_textFormat
    );
    if (FAILED(hr)) {
        std::cout << "CreateTextFormat failed" << std::endl;
        return false;
    }
    return true;
}


void SpineRenderer::ReleaseResources() {
    if (m_state) { delete m_state; m_state = nullptr; }
    if (m_stateData) { delete m_stateData; m_stateData = nullptr; }
    if (m_skeleton) { delete m_skeleton; m_skeleton = nullptr; }
    if (m_skeletonData) { delete m_skeletonData; m_skeletonData = nullptr; }
    if (m_atlas) { delete m_atlas; m_atlas = nullptr; }
    if (m_textureLoader) { delete m_textureLoader; m_textureLoader = nullptr; }
    m_spineBitmap.Reset();
    m_brush.Reset();
    m_renderTarget.Reset();
    m_factory.Reset();
    m_swapChain.Reset();
    m_deviceContext.Reset();
    m_device.Reset();
}

void SpineRenderer::BeginRender() {
    if (!m_initialized) return;
    
    // 렌더 타겟 시작
    m_renderTarget->BeginDraw();
}

void SpineRenderer::EndRender() {
    if (!m_initialized) return;
    
    // 렌더 타겟 종료
    HRESULT hr = m_renderTarget->EndDraw();
    if (SUCCEEDED(hr)) {
        // 스왑체인 프레젠트
        m_swapChain->Present(1, 0);
    }
}


void SpineRenderer::SetAnimation(const std::string& animationName) {
    if (m_state && m_skeletonData) {
        m_state->setAnimation(0, animationName.c_str(), true);
        m_currentAnimation = animationName;
        m_currentAnimationTime = 0.0f;
        m_animationTime = 0.0f;
        for (size_t i = 0; i < m_animationList.size(); ++i) {
            if (m_animationList[i] == animationName) {
                m_currentAnimationIndex = static_cast<int>(i);
                break;
            }
        }
        std::cout << "Animation changed: " << animationName << " (Index: " << m_currentAnimationIndex << ")" << std::endl;
    }
}

void SpineRenderer::UpdateAnimation(float deltaTime) {
    if (m_state && m_skeleton) {
        m_state->update(deltaTime);
        m_state->apply(*m_skeleton);
        m_skeleton->updateWorldTransform(spine::Physics::Physics_Pose);
        m_animationTime += deltaTime;
        m_currentAnimationTime += deltaTime;
    }
}

void SpineRenderer::SetAnimationTime(float time) {
    m_animationTime = time;
}


D2D1_SIZE_F SpineRenderer::GetRenderTargetSize() const {
    if (m_renderTarget) {
        return m_renderTarget->GetSize();
    }
    return D2D1::SizeF(static_cast<float>(m_clientWidth), static_cast<float>(m_clientHeight)); // 기본 크기
}

void SpineRenderer::Clear(const D2D1_COLOR_F& color) {
    if (!m_initialized) return;
    
    m_renderTarget->Clear(color);
} 



// --- 슬롯별 이미지 렌더링 ---
void SpineRenderer::Render() 
{
    if (!m_skeleton || !m_atlas || !m_renderTarget) 
        return;

    D2D1::Matrix3x2F worldTransform = D2D1::Matrix3x2F::Translation(m_CharacterPosition.x,m_CharacterPosition.y);
	D2D1::Matrix3x2F renderTransform = D2D1::Matrix3x2F::Scale(1.0f, -1.0f);
	D2D1::Matrix3x2F cameraInv = D2D1::Matrix3x2F::Translation(m_CameraPosition.x, m_CameraPosition.y);
	cameraInv.Invert();

    // 좌표계 축 그리기
    m_renderTarget->SetTransform(cameraInv * m_UnityScreen);
	m_renderTarget->DrawLine(D2D1::Point2F(-m_clientWidth ),D2D1::Point2F(+m_clientWidth ),m_brush.Get(), 1.0f);
	m_renderTarget->DrawLine(D2D1::Point2F(0.0f, -m_clientHeight ),D2D1::Point2F(0.0f, m_clientHeight ),m_brush.Get(), 1.0f);
        
    // 애니메이션 이름 표시
    m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());   
    std::wstring wAnimName(m_currentAnimation.begin(), m_currentAnimation.end());
    m_renderTarget->DrawTextW( wAnimName.c_str(), (UINT32)wAnimName.length(), 
        m_textFormat.Get(),D2D1::RectF(0, 0, 100, 10), m_brush.Get());

    // 슬롯별로 렌더링
    const auto& drawOrder = m_skeleton->getDrawOrder();  
    for (size_t i = 0; i < drawOrder.size(); ++i) 
    {	
        spine::Slot* slot = drawOrder[i];
        spine::Attachment* attachment = slot->getAttachment();
        if (!attachment) 
            continue;
        
        if (!attachment->getRTTI().isExactly(spine::RegionAttachment::rtti))
			continue; // RegionAttachment만 처리

		spine::RegionAttachment* regionAtt = static_cast<spine::RegionAttachment*>(attachment);
		spine::TextureRegion* region = regionAtt->getRegion();
		if (!region) 
            continue;
		
        spine::AtlasRegion* atlasRegion = static_cast<spine::AtlasRegion*>(region);
		if (!atlasRegion) 
            continue;
		
        ID2D1Bitmap* bitmap = reinterpret_cast<ID2D1Bitmap*>(region->rendererObject);
		if (!bitmap) 
            continue;
		// region.rotate(또는 degrees==90) 처리
		assert(atlasRegion->degrees == 0 || atlasRegion->degrees == 90);

        // 텍스처 이미지 배치 최적화로 이미지가 회전된 경우는 예외처리가 필요하다.
		bool rotated = (atlasRegion->degrees == 90);		
		float srcW = rotated ? atlasRegion->height : atlasRegion->width;
		float srcH = rotated ? atlasRegion->width : atlasRegion->height;
		D2D1_RECT_F srcRect = {  //OK
			(float)atlasRegion->x,
			(float)atlasRegion->y,
			(float)(atlasRegion->x + srcW),
			(float)(atlasRegion->y + srcH)
		};
		// 2. destRect + offset 적용
		float destW = rotated ? atlasRegion->originalHeight : atlasRegion->originalWidth;
		float destH = rotated ? atlasRegion->originalWidth : atlasRegion->originalHeight;
		float offsetX = rotated ? atlasRegion->offsetY : atlasRegion->offsetX;
		float offsetY = rotated ? atlasRegion->offsetX : atlasRegion->offsetY;
		float attachX = regionAtt->getX() + offsetX;
		float attachY = regionAtt->getY() + offsetY;
		float attachRot = regionAtt->getRotation() + (rotated ? -90.0f : 0.0f);

		D2D1::Matrix3x2F attachmentMatrix =
			D2D1::Matrix3x2F::Scale(regionAtt->getScaleX(), regionAtt->getScaleY()) *
			D2D1::Matrix3x2F::Rotation(attachRot) *
			D2D1::Matrix3x2F::Translation(attachX, attachY);

		D2D1_RECT_F destRect;
		destRect.left = -destW / 2;
		destRect.top = -destH / 2;
		destRect.right = destRect.left + destW;
		destRect.bottom = destRect.top + destH;

		// 4. 본(Bone)의 월드 변환
		spine::Bone& bone = slot->getBone();
		D2D1::Matrix3x2F boneWorldMatrix =
			D2D1::Matrix3x2F::Scale(bone.getScaleX(), bone.getScaleY()) *
			D2D1::Matrix3x2F::Rotation(bone.getWorldRotationX()) *
			D2D1::Matrix3x2F::Translation(bone.getWorldX(), bone.getWorldY());

		D2D1::Matrix3x2F finalMatrix = renderTransform * attachmentMatrix * boneWorldMatrix * worldTransform * cameraInv * m_UnityScreen;
		m_renderTarget->SetTransform(finalMatrix);

		// 확인용 영역 그리기
		if (rotated)
			m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue, 0.1f));
		else
			m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Green, 0.1f));
		m_renderTarget->FillRectangle(destRect, m_brush.Get());

		// 이미지 그리기
		m_renderTarget->DrawBitmap(bitmap, destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);

	}

    // --- 본의 원점에 cross(십자) 라인 그리기 ---
    if (m_skeleton) {
        auto& bones = m_skeleton->getBones();
        m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Red, 1.0f));
        for (size_t i = 0; i < bones.size(); ++i) {
            spine::Bone& bone = *bones[i];


			D2D1::Matrix3x2F boneWorldMatrix =
				D2D1::Matrix3x2F::Scale(bone.getScaleX(), bone.getScaleY()) *
				D2D1::Matrix3x2F::Rotation(bone.getWorldRotationX()) *
				D2D1::Matrix3x2F::Translation(bone.getWorldX(), bone.getWorldY());


			m_renderTarget->SetTransform(renderTransform * boneWorldMatrix * worldTransform * cameraInv  * m_UnityScreen);

            float x = 0;
            float y = 0;
            // 십자선 길이
            float crossLen = 8.0f;
            // 수평선
            m_renderTarget->DrawLine(
                D2D1::Point2F(x - crossLen, y),
                D2D1::Point2F(x + crossLen, y),
                m_brush.Get(), 2.0f
            );
            // 수직선
            m_renderTarget->DrawLine(
                D2D1::Point2F(x, y - crossLen),
                D2D1::Point2F(x, y + crossLen),
                m_brush.Get(), 2.0f
            );

			// 본 이름 표시
            std::wstring wBoneName(bone.getData().getName().buffer(), bone.getData().getName().buffer() + bone.getData().getName().length());
            m_renderTarget->DrawTextW(
                wBoneName.c_str(), (UINT32)wBoneName.length(),
                m_textFormat.Get(),
                D2D1::RectF(x + 10, y - 10, x + 100, y + 10),
                m_brush.Get()
			);
           
        }
    }
    m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

}



// --- 애니메이션 시간 업데이트 ---
// UpdateSpineAnimation 함수와 m_animations 등 spine-cpp 이전 구조체/멤버 관련 코드 완전 삭제

// --- 키보드 입력 처리 ---
void SpineRenderer::HandleKeyInput(int keyCode) {
    std::cout << "Key pressed: " << keyCode << std::endl;
    // 1~9: 해당 인덱스 애니메이션 재생
    if (keyCode >= '1' && keyCode <= '9') {
        int idx = keyCode - '1';
        if (idx >= 0 && idx < (int)m_animationList.size()) {
            SetAnimation(m_animationList[idx]);
        }
        return;
    }
    switch (keyCode) {
    case '0': // 0번 키: 애니메이션 없이 기본 포즈
        if (m_state) m_state->clearTracks();
        if (m_skeleton) m_skeleton->setToSetupPose();
        SetAnimationTime(0.0f);
        break;
    case VK_LEFT: // 왼쪽 화살표: 이전 애니메이션
        SetPreviousAnimation();
        break;
    case VK_RIGHT: // 오른쪽 화살표: 다음 애니메이션
        SetNextAnimation();
        break;
    default:
        std::cout << "Unhandled key: " << keyCode << std::endl;
        break;
    }
}

void SpineRenderer::SetNextAnimation() {
    if (m_animationList.empty()) return;
    
    m_currentAnimationIndex = (m_currentAnimationIndex + 1) % m_animationList.size();
    std::string newAnimation = m_animationList[m_currentAnimationIndex];
    SetAnimation(newAnimation);
    
    std::cout << "Next animation: " << newAnimation << " (Index: " << m_currentAnimationIndex << ")" << std::endl;
}

void SpineRenderer::SetPreviousAnimation() {
    if (m_animationList.empty()) return;
    
    m_currentAnimationIndex = (m_currentAnimationIndex - 1 + m_animationList.size()) % m_animationList.size();
    std::string newAnimation = m_animationList[m_currentAnimationIndex];
    SetAnimation(newAnimation);
    
    std::cout << "Previous animation: " << newAnimation << " (Index: " << m_currentAnimationIndex << ")" << std::endl;
}


bool SpineRenderer::LoadSpine(const std::string& atlasPath, const std::string& jsonPath) 
{   
    m_textureLoader = new Direct2DTextureLoader(m_renderTarget.Get());
    m_atlas = new spine::Atlas(atlasPath.c_str(), m_textureLoader);
    spine::SkeletonJson json(m_atlas);
    m_skeletonData = json.readSkeletonDataFile(jsonPath.c_str());
    if (!m_skeletonData) return false;
    m_skeleton = new spine::Skeleton(m_skeletonData);
    m_stateData = new spine::AnimationStateData(m_skeletonData);
    m_state = new spine::AnimationState(m_stateData);
    // 애니메이션 목록 채우기
    m_animationList.clear();
    for (int i = 0; i < m_skeletonData->getAnimations().size(); ++i) {
        m_animationList.push_back(m_skeletonData->getAnimations()[i]->getName().buffer());
    }
    return true;
} 
