#include <windows.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <iostream>
#include <chrono>
#include <thread>

#include "SpineRenderer.h"
#include "SpineAnimation.h"

using Microsoft::WRL::ComPtr;

#include <windows.h>
#include <wrl.h>  // ComPtr 사용을 위한 헤더

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include <d2d1_3.h> //ID2D1Factory8,ID2D1DeviceContext7
#pragma comment(lib, "d2d1.lib")

#include <dxgi1_6.h> // IDXGIFactory7
#pragma comment(lib, "dxgi.lib")

#include <dwrite.h>
#pragma comment(lib,"dwrite.lib")

using namespace Microsoft::WRL;

// 전역 변수
SpineRenderer g_spineRenderer;
SpineAnimation g_spineAnimation;
HWND g_hwnd = nullptr;
bool g_running = true;

int g_windowWidth = 1920;
int g_windowHeight = 1080;

// 윈도우 프로시저
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        g_running = false;
        PostQuitMessage(0);
        return 0;
        
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE:
            g_running = false;
            PostQuitMessage(0);
            return 0;
            
        default:
            // SpineRenderer의 키보드 입력 처리 함수 호출
            g_spineRenderer.HandleKeyInput(wParam);
            return 0;
        }
        break;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 윈도우 생성
HWND CreateGameWindow(const wchar_t* title, int width, int height) {
    // 윈도우 클래스 등록
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"Spine2DWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClass(&wc);
    
    // 윈도우 스타일 설정
    DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
    
    // 윈도우 크기 계산
    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, style, FALSE);
    
    // 윈도우 생성
    HWND hwnd = CreateWindowEx(
        0,
        L"Spine2DWindow",
        title,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
    
    return hwnd;
}


// 메인 함수
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 콘솔 출력 활성화 (디버깅용)
    
    std::cout << "Spine2D 프로젝트 시작" << std::endl;
    
    // 윈도우 생성
    g_hwnd = CreateGameWindow(L"Spine2D Demo", g_windowWidth, g_windowHeight);
    if (!g_hwnd) {
        std::cout << "윈도우 생성 실패" << std::endl;
        MessageBoxA(nullptr, "윈도우 생성 실패", "Initialization Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // Spine 렌더러 초기화
    if (!g_spineRenderer.Initialize(g_hwnd, g_windowWidth, g_windowHeight)) {
        std::cout << "Spine 렌더러 초기화 실패" << std::endl;
        MessageBoxA(nullptr, "Spine 렌더러 초기화 실패", "Initialization Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // Spineboy 데이터 로드
    if (!g_spineRenderer.LoadSpineSkeleton("../Resource/spineboy.json")) {
        std::cout << "Spine 스켈레톤 로드 실패" << std::endl;
        MessageBoxA(nullptr, "Spine 스켈레톤 로드 실패", "Load Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    if (!g_spineRenderer.LoadAtlas("../Resource/spineboy.atlas")) {
        std::cout << "Spine atlas 로드 실패" << std::endl;
        MessageBoxA(nullptr, "Spine atlas 로드 실패", "Load Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    if (!g_spineRenderer.LoadSpineBitmap("../Resource/spineboy.png")) {
        std::cout << "Spine 이미지 로드 실패" << std::endl;
        MessageBoxA(nullptr, "Spine 이미지 로드 실패", "Load Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // 모든 파일 로드 성공
    //MessageBoxA(nullptr, "모든 Spine2D 파일 로드 성공!\n\n키보드 조작:\n0: 이미지(본박스포함)/박스스켈레톤 토글\n1: Aim\n2: Death\n3: Hit\n4: Idle\n5: Jump\n6: Run\n7: Shoot\n8: Walk\n←→: 이전/다음 애니메이션\nESC: 종료", "Load Success", MB_OK | MB_ICONINFORMATION);
    
    g_spineRenderer.m_currentAnimation = "aim"; // 기본 애니메이션
    g_spineRenderer.m_currentAnimationTime = 0.0f;

    // 게임 루프
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (g_running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // 애니메이션 업데이트
        g_spineRenderer.UpdateSpineAnimation(deltaTime);
        // 렌더링
        g_spineRenderer.BeginRender();
        g_spineRenderer.Clear(D2D1::ColorF(D2D1::ColorF::LightGray));
        
        g_spineRenderer.RenderSpineSkeleton();
        
        g_spineRenderer.EndRender();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    g_spineRenderer.Shutdown();
    std::cout << "Spine2D 프로젝트 종료" << std::endl;
   
    return 0;
} 