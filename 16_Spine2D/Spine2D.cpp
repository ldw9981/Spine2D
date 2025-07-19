#include "Common.h"

#include "SpineRenderer.h"
#include <commdlg.h>  //파일 대화상자 API 사용
#include <filesystem>

// 전역 버퍼(또는 지역 변수 사용 가능)

#define MENU_ID_LOAD 1 // 메뉴 ID 정의

// 전역 변수
std::unique_ptr<SpineRenderer> g_spineRenderer;

HWND g_hwnd = nullptr;
bool g_running = true;

int g_windowWidth = 1280;
int g_windowHeight = 960;


// 윈도우 프로시저
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case 1: { // 메뉴 ID = 1 ("Load")
			OPENFILENAMEA ofn = {};
			char filePath[MAX_PATH] = { 0 };
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hwnd;
			ofn.lpstrFilter = "Atlas Files (*.atlas)\0*.atlas\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile = filePath;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
			ofn.lpstrTitle = "Open Atlas File";

			if (GetOpenFileNameA(&ofn)) 
            {			
				std::string selectedFileStr(filePath);

                g_spineRenderer->LoadSpine(
                    std::filesystem::path(selectedFileStr).replace_extension(".atlas").string(),
                    std::filesystem::path(selectedFileStr).replace_extension(".json").string()
				);

				// TODO: 선택된 파일 사용 로직 추가
			}
			return 0;
		}
		}
		break;

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
            g_spineRenderer->HandleKeyInput(wParam);
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
    

	// 메뉴 생성
	HMENU hMenu = CreateMenu();
	HMENU hFileMenu = CreatePopupMenu();
	AppendMenu(hFileMenu, MF_STRING, MENU_ID_LOAD, L"&Load");  // ID = 1
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");


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
        hMenu,
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
    
    ::CoInitialize(nullptr);

	g_spineRenderer = std::make_unique<SpineRenderer>();

    std::cout << "Spine2D 프로젝트 시작" << std::endl;
    
    // 윈도우 생성
    g_hwnd = CreateGameWindow(L"Spine2D Demo", g_windowWidth, g_windowHeight);
    if (!g_hwnd) {
        std::cout << "윈도우 생성 실패" << std::endl;
        MessageBoxA(nullptr, "윈도우 생성 실패", "Initialization Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // Spine 렌더러 초기화
    if (!g_spineRenderer->Initialize(g_hwnd, g_windowWidth, g_windowHeight)) {
        std::cout << "Spine 렌더러 초기화 실패" << std::endl;
        MessageBoxA(nullptr, "Spine 렌더러 초기화 실패", "Initialization Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    // spine-cpp 기반에서는 별도 데이터 로드 함수가 필요 없음(Initialize에서 모두 처리)
    // 기본 애니메이션 설정
  
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
        g_spineRenderer->UpdateAnimation(deltaTime);
        // 렌더링
        g_spineRenderer->BeginRender();
        g_spineRenderer->Clear(D2D1::ColorF(D2D1::ColorF::LightGray));
        g_spineRenderer->Render();
        g_spineRenderer->EndRender();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    g_spineRenderer->Shutdown();
    std::cout << "Spine2D 프로젝트 종료" << std::endl;
    g_spineRenderer.reset();

    ::CoUninitialize();
    return 0;
} 