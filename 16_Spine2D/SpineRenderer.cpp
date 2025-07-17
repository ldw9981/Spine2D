#include "Common.h"

#include "Float2.h"
#include "SpineRenderer.h"
#include "json.hpp" // nlohmann::json 사용

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




SpineRenderer::SpineRenderer() 
    : m_hwnd(nullptr), m_initialized(false), m_animationTime(0.0f) {
}

SpineRenderer::~SpineRenderer() {
    Shutdown();
}

bool SpineRenderer::Initialize(HWND hwnd,int width,int height) {
    m_hwnd = hwnd;
    
	m_UnityScreen = D2D1::Matrix3x2F::Scale(1.0f, -1.0f) *
		D2D1::Matrix3x2F::Translation(width / 2, height/2 );

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
    
    if (!InitializeSpine2D()) {
        std::cout << "Spine2D initialization failed" << std::endl;
        return false;
    }
    
    m_initialized = true;
    return true;
}

void SpineRenderer::Shutdown() {
    if (m_initialized) {
        ReleaseResources();
        m_initialized = false;
    }
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

bool SpineRenderer::InitializeSpine2D() {
    // 실제 Spine2D 라이브러리가 있다면 여기서 초기화
    // 예시 코드:
    /*
    m_atlas = new spine::Atlas("spine-data/spineboy.atlas", nullptr);
    if (!m_atlas) {
        return false;
    }
    
    spine::SkeletonJson json(m_atlas);
    m_skeletonData = json.readSkeletonDataFile("spine-data/spineboy.json");
    if (!m_skeletonData) {
        return false;
    }
    
    m_skeleton = new spine::Skeleton(m_skeletonData);
    m_animationState = new spine::AnimationState(new spine::AnimationStateData(m_skeletonData));
    */
    
    return true;
}

void SpineRenderer::ReleaseResources() {
    // Spine2D 리소스 해제
    /*
    if (m_animationState) {
        delete m_animationState;
        m_animationState = nullptr;
    }
    if (m_skeleton) {
        delete m_skeleton;
        m_skeleton = nullptr;
    }
    if (m_skeletonData) {
        delete m_skeletonData;
        m_skeletonData = nullptr;
    }
    if (m_atlas) {
        delete m_atlas;
        m_atlas = nullptr;
    }
    */
    
    // Direct2D 리소스 해제
    m_spriteBitmap.Reset();
    m_brush.Reset();
    m_renderTarget.Reset();
    m_factory.Reset();
    
    // Direct3D 11 리소스 해제
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

void SpineRenderer::RenderSpineAnimation(const std::string& atlasPath, const std::string& jsonPath, 
                                        float x, float y, float scale, float rotation) {
    if (!m_initialized) {
        std::cout << "SpineRenderer not initialized!" << std::endl;
        return;
    }
   
    
    // Atlas 로드
    if (m_atlasRegions.empty()) {
        std::cout << "Loading Atlas..." << std::endl;
        if (!LoadAtlas(atlasPath)) {
            std::cout << "Atlas loading failed!" << std::endl;
        } else {
            std::cout << "Atlas loading success! Region count: " << m_atlasRegions.size() << std::endl;
        }
    }
    
    // PNG 비트맵 로드
    if (!m_spineBitmap) {
        std::cout << "Loading Spine bitmap..." << std::endl;
        std::string imagePath = atlasPath.substr(0, atlasPath.find_last_of('.')) + ".png";
        std::cout << "Image path: " << imagePath << std::endl;
        if (!LoadSpineBitmap(imagePath)) {
            std::cout << "Spine bitmap loading failed!" << std::endl;
        } else {
            std::cout << "Spine bitmap loading success!" << std::endl;
        }
    }
    
    // 스켈레톤 데이터가 로드되어 있지 않으면 로드
    if (m_bones.empty()) {
        std::cout << "Loading skeleton..." << std::endl;
        if (!LoadSpineSkeleton(jsonPath)) {
            std::cout << "Skeleton loading failed!" << std::endl;
        } else {
            std::cout << "Skeleton loading success! Bone count: " << m_bones.size() << std::endl;
        }
    }
    
    // 변환 행렬 설정
    D2D1::Matrix3x2F transform =
        D2D1::Matrix3x2F::Scale(scale, scale) * 
        D2D1::Matrix3x2F::Rotation(rotation) * 
        D2D1::Matrix3x2F::Translation(x, y);
    
    m_renderTarget->SetTransform( transform * m_UnityScreen);    
    
    // 디버그 정보 표시
    RenderDebugInfo(atlasPath, jsonPath, x, y);
}


void SpineRenderer::RenderDebugInfo(const std::string& atlasPath, const std::string& jsonPath, float x, float y) {
    // 디버그 정보를 화면에 표시
    m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
    
    // 간단한 정보 박스
    D2D1_RECT_F infoRect = D2D1::RectF(10, 10, 300, 100);
    m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 0.7f));
    m_renderTarget->FillRectangle(infoRect, m_brush.Get());
    
    // 정보 텍스트 (실제로는 DirectWrite를 사용해야 함)
    m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
    
    // 간단한 텍스트 대신 도형으로 정보 표시
    D2D1_RECT_F textRect = D2D1::RectF(20, 20, 290, 90);
    m_renderTarget->DrawRectangle(textRect, m_brush.Get(), 1.0f);
    
    // 위치 표시
    D2D1_ELLIPSE posIndicator = D2D1::Ellipse(D2D1::Point2F(x, y), 5.0f, 5.0f);
    m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Cyan));
    m_renderTarget->FillEllipse(posIndicator, m_brush.Get());
}

void SpineRenderer::SetAnimation(const std::string& animationName) {
    // 실제 Spine2D 애니메이션 설정
    /*
    if (m_animationState && m_skeletonData) {
        m_animationState->setAnimation(0, animationName.c_str(), true);
    }
    */
    
    // 현재 애니메이션 설정
    m_currentAnimation = animationName;
    m_animationProgress = 0.0f;
    m_currentAnimationTime = 0.0f; // 애니메이션 시간 리셋
    
    // 애니메이션 인덱스 업데이트
    for (size_t i = 0; i < m_animationList.size(); ++i) {
        if (m_animationList[i] == animationName) {
            m_currentAnimationIndex = i;
            break;
        }
    }
    
    std::cout << "Animation changed: " << animationName << " (Index: " << m_currentAnimationIndex << ")" << std::endl;
}

void SpineRenderer::UpdateAnimation(float deltaTime) {
    // 실제 Spine2D 애니메이션 업데이트
    /*
    if (m_animationState && m_skeleton) {
        m_animationState->update(deltaTime);
        m_animationState->apply(*m_skeleton);
    }
    */
    
    // 애니메이션 시간 업데이트
    m_animationTime += deltaTime;
    
    // Spine2D 스켈레톤 애니메이션 업데이트
    UpdateSpineAnimation(deltaTime);
}

void SpineRenderer::SetAnimationTime(float time) {
    m_animationTime = time;
}

// WIC를 통해 PNG 등을 로드하여 ID2D1Bitmap1**으로 반환
bool SpineRenderer::CreateBitmapFromFile(const wchar_t* path, ID2D1Bitmap1** outBitmap)
{
	// WIC 팩토리 생성
	ComPtr<IWICImagingFactory> wicFactory;
	HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&wicFactory)
	);
    
    ComPtr<IWICBitmapDecoder>     decoder;
	ComPtr<IWICBitmapFrameDecode> frame;
	ComPtr<IWICFormatConverter>   converter;



	// ① 디코더 생성
	hr = wicFactory->CreateDecoderFromFilename(
		path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
	if (FAILED(hr)) 
        return false;

	// ② 첫 프레임 얻기
	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr)) 
        return false;

	// ③ 포맷 변환기 생성
	hr = wicFactory->CreateFormatConverter(&converter);
	if (FAILED(hr)) 
        return false;

	// ④ GUID_WICPixelFormat32bppPBGRA로 변환
	hr = converter->Initialize(
		frame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeCustom
	);
	if (FAILED(hr)) 
        return false;

	// ⑤ Direct2D 비트맵 속성 (premultiplied alpha, B8G8R8A8_UNORM)
	D2D1_BITMAP_PROPERTIES1 bmpProps = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_NONE,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
	);

	// ⑥ DeviceContext에서 WIC 비트맵으로부터 D2D1Bitmap1 생성
	//hr = m_renderTarget->CreateBitmapFromWicBitmap(converter.Get(), &bmpProps, outBitmap);
	if (FAILED(hr))
		return false;
	
    return true;
}


bool SpineRenderer::LoadSpriteBitmap(const std::string& imagePath) {
    if (!m_initialized || !m_renderTarget) {
        return false;
    }
    
    // WIC 팩토리 생성
    ComPtr<IWICImagingFactory> wicFactory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory)
    );
    
    if (FAILED(hr)) {
        std::cout << "WIC 팩토리 생성 실패" << std::endl;
        return false;
    }
    
    // 이미지 파일 로드
    ComPtr<IWICBitmapDecoder> decoder;
    std::wstring wImagePath(imagePath.begin(), imagePath.end());
    hr = wicFactory->CreateDecoderFromFilename(
        wImagePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder
    );
    
    if (FAILED(hr)) {
        std::cout << "이미지 파일 로드 실패: " << imagePath << std::endl;
        return false;
    }
    
    // 프레임 가져오기
    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
        std::cout << "이미지 프레임 가져오기 실패" << std::endl;
        return false;
    }
    
    ComPtr<IWICFormatConverter>   converter;
	hr = wicFactory->CreateFormatConverter(&converter);
	if (FAILED(hr)) 
        return false;

	// ④ GUID_WICPixelFormat32bppPBGRA로 변환
	hr = converter->Initialize(
		frame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeCustom
	);
	if (FAILED(hr)) 
        return false;

	D2D1_BITMAP_PROPERTIES bmpProps = D2D1::BitmapProperties(
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
	);


    // Direct2D 비트맵으로 변환
    hr = m_renderTarget->CreateBitmapFromWicBitmap(frame.Get(),bmpProps, m_spriteBitmap.GetAddressOf());
    if (FAILED(hr)) {
        std::cout << "Direct2D 비트맵 생성 실패" << std::endl;
        return false;
    }
    
    std::cout << "Sprite loaded successfully: " << imagePath << std::endl;
    return true;
}


bool SpineRenderer::LoadSpineSkeleton(const std::string& jsonPath) {
    std::cout << "Loading Spine skeleton from: " << jsonPath << std::endl;
    using json = nlohmann::json;
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        std::string errorMsg = "Failed to open JSON file: " + jsonPath;
        std::cout << errorMsg << std::endl;
        MessageBoxA(nullptr, errorMsg.c_str(), "File Error", MB_OK | MB_ICONERROR);
        return false;
    }
    std::cout << "JSON file opened successfully" << std::endl;
    json j; file >> j;
    // 본 파싱
    m_bones.clear();

	std::unordered_map<std::string, int> boneNameToIndex;
    int boneIdx = 0;
    for (const auto& b : j["bones"]) {
        SpineBone bone;
        bone.index = boneIdx;
        bone.name = b["name"].get<std::string>();
        boneNameToIndex[bone.name] = boneIdx; // 이름과 인덱스 매핑
        if (b.contains("x")) bone.x = b["x"].get<float>();
        if (b.contains("y")) bone.y = b["y"].get<float>();
        if (b.contains("rotation")) bone.rotation = b["rotation"].get<float>();
        if (b.contains("scaleX")) bone.scaleX = b["scaleX"].get<float>();
        if (b.contains("scaleY")) bone.scaleY = b["scaleY"].get<float>();
        if (b.contains("length")) bone.length = b["length"].get<float>();
        if (b.contains("color")) bone.color = b["color"].get<std::string>();
        if (b.contains("parent")) {
            bone.parentName = b["parent"].get<std::string>();
            auto it = boneNameToIndex.find(bone.parentName);
            if (it != boneNameToIndex.end()) {
                bone.parentIndex = it->second; // 부모 인덱스 설정
            }
        }       
        m_bones.push_back(bone);		
        ++boneIdx;
    }	

    // 슬롯 파싱
    m_slots.clear();
    for (const auto& s : j["slots"]) {
        SpineSlot slot;
        slot.name = s["name"].get<std::string>();
        slot.bone = s["bone"].get<std::string>();
        if (s.contains("attachment")) slot.attachment = s["attachment"].get<std::string>();
        m_slots.push_back(slot);
    }
    // 스킨 파싱
    m_skins.clear();
    for (const auto& skin : j["skins"]) {
        SpineSkin s;
        s.name = skin["name"].get<std::string>();
        if (skin.contains("attachments")) {
            for (auto& slotPair : skin["attachments"].items()) {
                std::string slotName = slotPair.key();
                auto& attMap = slotPair.value();
                for (auto& attPair : attMap.items()) {
                    std::string attName = attPair.key();
                    auto& attVal = attPair.value();
                    SpineAttachment att;
                    att.name = attName;
                    att.type = attVal.value("type", "region");
                    if (attVal.contains("x")) att.x = attVal["x"].get<float>();
                    if (attVal.contains("y")) att.y = attVal["y"].get<float>();
                    if (attVal.contains("rotation")) att.rotation = attVal["rotation"].get<float>();
                    if (attVal.contains("scaleX")) att.scaleX = attVal["scaleX"].get<float>();
                    if (attVal.contains("scaleY")) att.scaleY = attVal["scaleY"].get<float>();
                    if (attVal.contains("width")) att.width = attVal["width"].get<float>();
                    if (attVal.contains("height")) att.height = attVal["height"].get<float>();
                    if (attVal.contains("path")) att.path = attVal["path"].get<std::string>();
                    s.attachments[slotName][attName] = att;
                }
            }
        }
        m_skins.push_back(s);
    }
    // 애니메이션 파싱(간단화)
    m_animations.clear();
    if (j.contains("animations")) {
        for (auto& animPair : j["animations"].items()) {
            std::string animName = animPair.key();
            auto& animVal = animPair.value();
            Animation anim;
            anim.name = animName;
            if (animVal.contains("bones")) {
                for (auto& bonePair : animVal["bones"].items()) {
                    std::string boneName = bonePair.key();
                    auto& boneAnim = bonePair.value();
                    BoneTimeline timeline;
					if (boneAnim.contains("scale")) {
						for (auto& kf : boneAnim["scale"]) {
							KeyFrameFloat2 k;
							// 정적 포즈 처리: time이 없으면 0.0f로 설정                          
							if (kf.contains("time")) {
								k.time = kf["time"].get<float>();
							}
							if (kf.contains("x")) {
								k.x = kf["x"].get<float>();
							}
							if (kf.contains("y")) {
								k.y = kf["y"].get<float>();
							}
							if (kf.contains("curve")) {
								if (kf["curve"].is_array())
									for (auto& c : kf["curve"]) k.curve.push_back(c.get<float>());
							}
							timeline.scale.push_back(k);
						}
					}

                    if (boneAnim.contains("rotate")) {
                        for (auto& kf : boneAnim["rotate"]) {
                            KeyFrameFloat k;
                            // 정적 포즈 처리: time이 없으면 0.0f로 설정                          
                            if (kf.contains("time")) {
                                k.time = kf["time"].get<float>();
							}
                            if (kf.contains("value")) {
                                k.value = kf["value"].get<float>();
							}             
                            if (kf.contains("curve")) {
                                if (kf["curve"].is_array())
                                    for (auto& c : kf["curve"]) k.curve.push_back(c.get<float>());
                            }
                            timeline.rotate.push_back(k);
                        }
                    }
					if (boneAnim.contains("translate")) {
						for (auto& kf : boneAnim["translate"]) {
                            KeyFrameFloat2 k;
							// 정적 포즈 처리: time이 없으면 0.0f로 설정                          
							if (kf.contains("time")) {
								k.time = kf["time"].get<float>();
							}
							if (kf.contains("x")) {
								k.x = kf["x"].get<float>();
							}		
							if (kf.contains("y")) {
								k.y = kf["y"].get<float>();
							}
							if (kf.contains("curve")) {
								if (kf["curve"].is_array())
									for (auto& c : kf["curve"]) k.curve.push_back(c.get<float>());
							}
							timeline.translate.push_back(k);
						}
					}
                    // (translate, scale 등도 필요시 추가)
                    anim.boneTimelines[boneName] = timeline;
                }
            }
            // duration 계산(마지막 키프레임 시간)
            float maxTime = 0;
            for (const auto& kv : anim.boneTimelines) {
                const BoneTimeline& t = kv.second;
                if (!t.rotate.empty())
                    maxTime = std::fmax(maxTime, t.rotate.back().time);
            }
            anim.duration = maxTime;
            m_animations[animName] = anim;
        }
    }
    return true;
}



D2D1_SIZE_F SpineRenderer::GetRenderTargetSize() const {
    if (m_renderTarget) {
        return m_renderTarget->GetSize();
    }
    return D2D1::SizeF(m_clientWidth,m_clientHeight); // 기본 크기
}

void SpineRenderer::Clear(const D2D1_COLOR_F& color) {
    if (!m_initialized) return;
    
    m_renderTarget->Clear(color);
} 

// --- atlas 파싱 함수 ---
bool SpineRenderer::LoadAtlas(const std::string& atlasPath) 
{
    std::cout << "Loading Atlas from: " << atlasPath << std::endl;
    m_atlasRegions.clear();
    std::ifstream file(atlasPath);
    if (!file.is_open()) {
        std::string errorMsg = "Failed to open atlas file: " + atlasPath;
        std::cout << errorMsg << std::endl;
        MessageBoxA(nullptr, errorMsg.c_str(), "File Error", MB_OK | MB_ICONERROR);
        return false;
    }
    std::cout << "Atlas file opened successfully" << std::endl;
    
    std::string line, regionName;
    AtlasRegion region;
    
    while (std::getline(file, line)) {
        // 빈 줄이나 헤더 정보 건너뛰기
        if (line.empty() || line.find(".png") != std::string::npos || 
            line.find("size:") != std::string::npos || 
            line.find("filter:") != std::string::npos || 
            line.find("pma:") != std::string::npos) {
            continue;
        }
        
        // 새로운 region 시작 (들여쓰기 없는 줄이면서 특수 키워드가 아닌 경우)
        if (line[0] != ' ' && line[0] != '\t' && 
            line.find("bounds:") == std::string::npos &&
            line.find("rotate:") == std::string::npos &&
            line.find("offsets:") == std::string::npos) {
            // 이전 region 저장
            if (!regionName.empty()) {
                m_atlasRegions[regionName] = region;
                std::cout << "Saved region: " << regionName << " at (" << region.x << "," << region.y << "," << region.width << "," << region.height << ")" << std::endl;
            }
            
            // 새 region 시작
            regionName = line;
            region = AtlasRegion{};
            region.name = regionName;
            region.rotate = false;
            region.x = 0;
            region.y = 0;
            region.width = 0;
            region.height = 0;
            region.orig_w = 0;
            region.orig_h = 0;
            region.offset_x = 0;
            region.offset_y = 0;
            continue;
        }
        
        // region 속성 파싱
        std::cout << "  Parsing line: '" << line << "'" << std::endl;
        
        // : 로 키와 값 분리
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            std::cout << "  ERROR: No colon found in line" << std::endl;
            continue;
        }
        
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        std::cout << "  Key: '" << key << "', Value: '" << value << "'" << std::endl;
        
        if (key == "rotate") {
            region.rotate = (value == "90");
            std::cout << "  rotate: " << value << std::endl;
        } else if (key == "bounds") {
            // bounds:767,2,93,89 형식 파싱
            std::istringstream valueStream(value);
            char c1, c2, c3;
            if (valueStream >> region.x >> c1 >> region.y >> c2 >> region.width >> c3 >> region.height) {
                std::cout << "  bounds: " << region.x << "," << region.y << "," << region.width << "," << region.height << std::endl;
            } else {
                std::cout << "  ERROR: Failed to parse bounds: " << value << std::endl;
            }
        } else if (key == "offsets") {
            // offsets:0,1,126,69 형식 파싱
            std::istringstream valueStream(value);
            char c1, c2, c3;
            if (valueStream >> region.offset_x >> c1 >> region.offset_y >> c2 >> region.orig_w >> c3 >> region.orig_h) {
                std::cout << "  offsets: " << region.offset_x << "," << region.offset_y << "," << region.orig_w << "," << region.orig_h << std::endl;
            } else {
                std::cout << "  ERROR: Failed to parse offsets: " << value << std::endl;
            }
        }

		// 마지막 region 저장
		if (!regionName.empty()) {
			m_atlasRegions[regionName] = region;
			std::cout << "Saved region: " << regionName << " at (" << region.x << "," << region.y << "," << region.width << "," << region.height << ")" << std::endl;
		}
    }

    std::cout << "Total regions loaded: " << m_atlasRegions.size() << std::endl;
    return true;
}

// --- PNG 비트맵 로드 ---
bool SpineRenderer::LoadSpineBitmap(const std::string& imagePath) {
    std::cout << "Loading Spine bitmap from: " << imagePath << std::endl;
    if (!m_initialized || !m_renderTarget) {
        std::string errorMsg = "SpineRenderer not initialized or render target is null";
        std::cout << errorMsg << std::endl;
        MessageBoxA(nullptr, errorMsg.c_str(), "Initialization Error", MB_OK | MB_ICONERROR);
        return false;
    }
    Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
    if (FAILED(hr)) return false;
    std::wstring wImagePath(imagePath.begin(), imagePath.end());
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    hr = wicFactory->CreateDecoderFromFilename(wImagePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) return false;
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) return false;
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) return false;
    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return false;
    hr = m_renderTarget->CreateBitmapFromWicBitmap(converter.Get(), &m_spineBitmap);
    if (FAILED(hr)) return false;
    return true;
}

// --- 키프레임 보간 함수 ---
float SpineRenderer::InterpolateKeyFrames(const std::vector<KeyFrameFloat>& keyframes, float time) 
{
    if (keyframes.empty()) return 0.0f;
    
    // 정적 포즈 처리: 시간이 없는 키프레임이면 첫 번째 값 반환
    if (keyframes.size() == 1 || keyframes.front().time == 0.0f && keyframes.size() == 1) {
        return keyframes.front().value;
    }
    
    // 애니메이션 키프레임 처리
    if (time <= keyframes.front().time) return keyframes.front().value;
    if (time >= keyframes.back().time) return keyframes.back().value;
    
    for (size_t i = 1; i < keyframes.size(); ++i) {
        if (time < keyframes[i].time) {
            const KeyFrameFloat& prev = keyframes[i-1];
            const KeyFrameFloat& next = keyframes[i];
            float t = (time - prev.time) / (next.time - prev.time);
            // (베지어 곡선 등 curve 처리 생략, 선형 보간)
            return prev.value + (next.value - prev.value) * t;
        }
    }
    return keyframes.back().value;
}

Float2 SpineRenderer::InterpolateKeyFrames(const std::vector<KeyFrameFloat2>& keyframes, float time)
{
	if (keyframes.empty()) 
        return Float2(0.0f, 0.0f);

	// 정적 포즈 처리: 시간이 없는 키프레임이면 첫 번째 값 반환
	if (keyframes.size() == 1 || keyframes.front().time == 0.0f && keyframes.size() == 1) {
		return Float2(keyframes.front().x, keyframes.front().y);
	}

	// 애니메이션 키프레임 처리
	if (time <= keyframes.front().time) 
        return Float2(keyframes.front().x, keyframes.front().y);
	
    if (time >= keyframes.back().time) 
        return Float2(keyframes.back().x, keyframes.back().y);

	for (size_t i = 1; i < keyframes.size(); ++i) {
		if (time < keyframes[i].time) {
			const KeyFrameFloat2& prev = keyframes[i - 1];
			const KeyFrameFloat2& next = keyframes[i];

			float t = (time - prev.time) / (next.time - prev.time);
			return Float2(prev.x + (next.x - prev.x) * t , prev.y + (next.y - prev.y) * t);
		}
	}
	return Float2(keyframes.back().x, keyframes.back().y);
}


// --- 슬롯별 이미지 렌더링 ---
void SpineRenderer::RenderSpineSkeleton() 
{ 
    m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	std::wstring wAnimName(m_currentAnimation.begin(), m_currentAnimation.end());
	m_renderTarget->DrawTextW(
        wAnimName.c_str(), (UINT32)wAnimName.length(),
		m_textFormat.Get(),
		D2D1::RectF(0,0,100, 10),
		m_brush.Get()
	);

    
    // 애니메이션 적용: 각 본의 로컬 트랜스폼 계산
    std::vector<D2D1::Matrix3x2F> local(m_bones.size());
    const Animation* anim = nullptr;
    if (!m_currentAnimation.empty()) {
        std::map<std::string, Animation>::iterator itAnim = m_animations.find(m_currentAnimation);
        if (itAnim != m_animations.end()) {
            anim = &itAnim->second;
            std::cout << "Current animation: " << m_currentAnimation << std::endl;
        } else {
            std::cout << "Animation not found: " << m_currentAnimation << std::endl;
		}
	}
    for (size_t i = 0; i < m_bones.size(); ++i) {
        const auto& bone = m_bones[i];
        float rot = bone.rotation;
        Float2 scale = { bone.scaleX,bone.scaleY } ;
        Float2 translate = { bone.x,bone.y};
        if (anim) {
            auto itT = anim->boneTimelines.find(bone.name);
            if (itT != anim->boneTimelines.end()) {
                if (!itT->second.rotate.empty())
                {
                    float addRot = InterpolateKeyFrames(itT->second.rotate, m_currentAnimationTime);
                    rot += addRot;
                }
                   
                if (!itT->second.scale.empty())
                {
                    Float2 addScale = InterpolateKeyFrames(itT->second.scale, m_currentAnimationTime);
					scale.x *= addScale.x;
					scale.y *= addScale.y;
                }
                   
                if (!itT->second.translate.empty())
                {
                    Float2 addTranslate = InterpolateKeyFrames(itT->second.translate, m_currentAnimationTime);
                    translate.x += addTranslate.x;
					translate.y += addTranslate.y;
                }
            }
        }
        D2D1::Matrix3x2F localMatrix = 
            D2D1::Matrix3x2F::Scale(scale.x, scale.y) *
            D2D1::Matrix3x2F::Rotation(rot) *
            D2D1::Matrix3x2F::Translation(translate.x, translate.y);
        local[i] = localMatrix;
    }
    
    // 본의 월드 트랜스폼 계산
    std::vector<D2D1::Matrix3x2F> world(m_bones.size());

    D2D1::Matrix3x2F parentMatrix;
    for (size_t i = 0; i < m_bones.size(); ++i) 
    {
		// 부모가 있다면 부모의 월드 트랜스폼을 곱함
		if (m_bones[i].parentIndex >= 0) {
			parentMatrix = world[m_bones[i].parentIndex];
		}
		else {
			parentMatrix = D2D1::Matrix3x2F::Identity();
		}
        world[i] = local[i] * parentMatrix;
    }
    
    // 슬롯별로 이미지 렌더링
    int renderedSlots = 0;
    size_t size = m_slots.size();
    for (size_t i = 0; i < size; ++i) 
    {
        const auto& slot = m_slots[i];
        const SpineSkin* skin = nullptr;
        for (const auto& s : m_skins) if (s.name == m_currentSkin) skin = &s;
        if (!skin) 
            continue;
        
        auto itAttMap = skin->attachments.find(slot.name);
        if (itAttMap == skin->attachments.end()) 
            continue;       
        
        std::string attName = slot.attachment;
        if (attName.empty() && !itAttMap->second.empty()) attName = itAttMap->second.begin()->first;
        auto itAtt = itAttMap->second.find(attName);
        if (itAtt == itAttMap->second.end()) 
            continue;
        
        const SpineAttachment& att = itAtt->second;
        
        // atlas에서 region 찾기
        std::string regionName = att.path.empty() ? att.name : att.path;
        auto itRegion = m_atlasRegions.find(regionName);
        if (itRegion == m_atlasRegions.end()) 
			continue;

        const AtlasRegion& region = itRegion->second;
        // 본 트랜스폼
        int boneIndex = -1;
        for (size_t i = 0; i < m_bones.size(); ++i) {
            if (m_bones[i].name == slot.bone) 
            { 
                boneIndex = (int)i; 
                break; 
            }
        }
        if (boneIndex < 0) 
            continue;
        
        const D2D1::Matrix3x2F& boneMatrix = world[boneIndex];
        
        float rotate = region.rotate ? -90.0f : 0.0f;
        D2D1::Matrix3x2F renderMatrix = D2D1::Matrix3x2F::Scale(1.0f, -1.0f) * D2D1::Matrix3x2F::Rotation(rotate);

        // 이미지 변환 행렬 계산 (attachment 변환을 본 변환에 적용)
        D2D1::Matrix3x2F attachmentMatrix =
            D2D1::Matrix3x2F::Scale(att.scaleX,att.scaleY) * D2D1::Matrix3x2F::Rotation(att.rotation) *
            D2D1::Matrix3x2F::Translation(att.x,att.y);
                    
        D2D1::Matrix3x2F finalMatrix = renderMatrix * attachmentMatrix * boneMatrix * m_UnityScreen;
        m_renderTarget->SetTransform(finalMatrix);

        // atlas에서 해당 영역만 잘라서 렌더링
        D2D1_RECT_F srcRect;
        srcRect.left = (float)region.x;
		srcRect.top = (float)region.y;
		srcRect.right = srcRect.left + (float)(region.rotate ? region.height : region.width);
        srcRect.bottom = srcRect.top + (float)(region.rotate ? region.width : region.height);
           
        // offsets 정보를 사용해서 destRect 계산
        float destWidth = (region.orig_w > 0) ? region.orig_w : att.width;
        float destHeight = (region.orig_h > 0) ? region.orig_h : att.height;
        
        // attachment 위치와 offsets를 사용해서 이미지 위치 조정
        float offsetX = 0;// (float)(region.rotate ? region.offset_y : region.offset_x);
        float offsetY = 0;// (float)(region.rotate ? region.offset_x : region.offset_y);
        
        D2D1_RECT_F destRect;
        destRect.left = offsetX;
        destRect.top = offsetY;
        destRect.right = destRect.left + (float)(region.rotate ? destHeight : destWidth);
        destRect.bottom = destRect.top + (float)(region.rotate ? destWidth : destHeight);

        if (m_spineBitmap) {
            // 1. 슬롯 이미지 렌더링
            m_renderTarget->DrawBitmap(m_spineBitmap.Get(), destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);
            
			m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Red, 0.3f));
			m_renderTarget->FillRectangle(destRect, m_brush.Get());
			
            renderedSlots++;            
        } else {
            std::cout << "Spine bitmap is null!" << std::endl;
        }
    }
    
    
    m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    std::cout << "Rendered slots count: " << renderedSlots << std::endl;
    std::cout << "RenderSpineSkeleton completed" << std::endl;



    std::unordered_set<std::string> printBone;
	printBone.insert("root");
    printBone.insert("hip");
    printBone.insert("front-thigh");
    printBone.insert("rear-thigh");
    printBone.insert("torso");

	for (size_t i = 0; i < m_bones.size(); ++i) 
    {	
        std::string boneName = m_bones[i].name;

		//if (printBone.find(boneName) == printBone.end())
        //    continue;
        
        D2D1::Matrix3x2F boneMatrix = world[i];
        D2D1::Matrix3x2F renderMatrix = D2D1::Matrix3x2F::Scale(1.0f, -1.0f);
		D2D1::Matrix3x2F finalMatrix = renderMatrix * boneMatrix * m_UnityScreen;
		m_renderTarget->SetTransform(finalMatrix);
		
        D2D1_RECT_F destRect;
        destRect = { -5,-1,5,1 }; 
        m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Green, 1.0f));
        m_renderTarget->FillRectangle(destRect, m_brush.Get());
        destRect = { -1,-5,1,5 };
        m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Red, 1.0f));
		m_renderTarget->FillRectangle(destRect, m_brush.Get());
		D2D1_POINT_2F textPos = { 0,0 };
		m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue, 1.0f));


		
        std::wstring wBoneName(boneName.begin(), boneName.end());
		m_renderTarget->DrawTextW(
            wBoneName.c_str(), (UINT32)wBoneName.length(),
			m_textFormat.Get(),
			D2D1::RectF(textPos.x, textPos.y, textPos.x + 100, textPos.y + 10),
			m_brush.Get()
		);
	}

}



// --- 애니메이션 시간 업데이트 ---
void SpineRenderer::UpdateSpineAnimation(float deltaTime) {
    m_currentAnimationTime += deltaTime;
    auto it = m_animations.find(m_currentAnimation);
    if (it != m_animations.end() && it->second.duration > 0) {
        if (m_currentAnimationTime > it->second.duration)
            m_currentAnimationTime = 0.0f;
    }
}

// --- 키보드 입력 처리 ---
void SpineRenderer::HandleKeyInput(int keyCode) {
    std::cout << "Key pressed: " << keyCode << std::endl;
    
    switch (keyCode) {
    case '0': // 0번 키: 기본 포즈 <-> 애니메이션 토글 (이미지는 항상 표시)
   
        break;
        
    case '1': // 1번 키: aim 애니메이션
        SetAnimation("aim");
        break;
        
    case '2': // 2번 키: death 애니메이션
        SetAnimation("death");
        break;
        
    case '3': // 3번 키: hit 애니메이션
        SetAnimation("hit");
        break;
        
    case '4': // 4번 키: idle 애니메이션
        SetAnimation("idle");
        break;
        
    case '5': // 5번 키: jump 애니메이션
        SetAnimation("jump");
        break;
        
    case '6': // 6번 키: run 애니메이션
        SetAnimation("run");
        break;
        
    case '7': // 7번 키: shoot 애니메이션
        SetAnimation("shoot");
        break;
        
    case '8': // 8번 키: walk 애니메이션
        SetAnimation("walk");
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

void SpineRenderer::RenderAnimationInfo() {
    if (!m_renderTarget || !m_brush) return;
    
    // 배경 박스 그리기
    D2D1_RECT_F bgRect = D2D1::RectF(10, 10, 350, 200);
    m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Black, 0.7f));
    m_renderTarget->FillRectangle(bgRect, m_brush.Get());
    
    // 테두리 그리기
    m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::White, 0.8f));
    m_renderTarget->DrawRectangle(bgRect, m_brush.Get(), 2.0f);
    
    // 현재 애니메이션 정보 표시 (간단한 도형으로)
    m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Yellow));
    
    // 현재 애니메이션 인덱스 표시 (색상으로 구분)
    for (size_t i = 0; i < m_animationList.size(); ++i) {
        D2D1_RECT_F animRect = D2D1::RectF(20 + i * 40, 30, 20 + i * 40 + 30, 60);
        
        if (i == m_currentAnimationIndex) {
            // 현재 애니메이션은 밝은 색
            m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Cyan));
        } else {
            // 다른 애니메이션은 어두운 색
            m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Gray));
        }
        
        m_renderTarget->FillRectangle(animRect, m_brush.Get());
        
        // 번호 표시 (간단한 점으로)
        D2D1_ELLIPSE dot = D2D1::Ellipse(D2D1::Point2F(35 + i * 40, 45), 3.0f, 3.0f);
        m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
        m_renderTarget->FillEllipse(dot, m_brush.Get());
    }
    
    // 렌더링 모드 표시
    D2D1_RECT_F modeRect = D2D1::RectF(20, 80, 340, 110);
    m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::LightGreen));
    m_renderTarget->FillRectangle(modeRect, m_brush.Get());
    
    // 애니메이션 시간 표시
    D2D1_RECT_F timeRect = D2D1::RectF(20, 120, 340, 150);
    m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::Orange));
    m_renderTarget->FillRectangle(timeRect, m_brush.Get());
    
    // 키보드 조작법 표시
    D2D1_RECT_F keyRect = D2D1::RectF(20, 160, 340, 190);
    m_brush->SetColor(D2D1::ColorF(D2D1::ColorF::LightBlue));
    m_renderTarget->FillRectangle(keyRect, m_brush.Get());
} 
