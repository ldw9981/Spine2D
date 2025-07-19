#include "Common.h"

#include "SpineRenderer.h"
#include <commdlg.h>  //���� ��ȭ���� API ���
#include <filesystem>

// ���� ����(�Ǵ� ���� ���� ��� ����)

#define MENU_ID_LOAD 1 // �޴� ID ����

// ���� ����
std::unique_ptr<SpineRenderer> g_spineRenderer;

HWND g_hwnd = nullptr;
bool g_running = true;

int g_windowWidth = 1280;
int g_windowHeight = 960;


// ������ ���ν���
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case 1: { // �޴� ID = 1 ("Load")
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

				// TODO: ���õ� ���� ��� ���� �߰�
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
            // SpineRenderer�� Ű���� �Է� ó�� �Լ� ȣ��
            g_spineRenderer->HandleKeyInput(wParam);
            return 0;
        }
        break;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ������ ����
HWND CreateGameWindow(const wchar_t* title, int width, int height) {
    // ������ Ŭ���� ���
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"Spine2DWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClass(&wc);
    

	// �޴� ����
	HMENU hMenu = CreateMenu();
	HMENU hFileMenu = CreatePopupMenu();
	AppendMenu(hFileMenu, MF_STRING, MENU_ID_LOAD, L"&Load");  // ID = 1
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");


    // ������ ��Ÿ�� ����
    DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
    
    // ������ ũ�� ���
    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, style, FALSE);
    
    // ������ ����
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


// ���� �Լ�
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // �ܼ� ��� Ȱ��ȭ (������)
    
    ::CoInitialize(nullptr);

	g_spineRenderer = std::make_unique<SpineRenderer>();

    std::cout << "Spine2D ������Ʈ ����" << std::endl;
    
    // ������ ����
    g_hwnd = CreateGameWindow(L"Spine2D Demo", g_windowWidth, g_windowHeight);
    if (!g_hwnd) {
        std::cout << "������ ���� ����" << std::endl;
        MessageBoxA(nullptr, "������ ���� ����", "Initialization Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    // Spine ������ �ʱ�ȭ
    if (!g_spineRenderer->Initialize(g_hwnd, g_windowWidth, g_windowHeight)) {
        std::cout << "Spine ������ �ʱ�ȭ ����" << std::endl;
        MessageBoxA(nullptr, "Spine ������ �ʱ�ȭ ����", "Initialization Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    // spine-cpp ��ݿ����� ���� ������ �ε� �Լ��� �ʿ� ����(Initialize���� ��� ó��)
    // �⺻ �ִϸ��̼� ����
  
    // ���� ����
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
        // �ִϸ��̼� ������Ʈ
        g_spineRenderer->UpdateAnimation(deltaTime);
        // ������
        g_spineRenderer->BeginRender();
        g_spineRenderer->Clear(D2D1::ColorF(D2D1::ColorF::LightGray));
        g_spineRenderer->Render();
        g_spineRenderer->EndRender();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    g_spineRenderer->Shutdown();
    std::cout << "Spine2D ������Ʈ ����" << std::endl;
    g_spineRenderer.reset();

    ::CoUninitialize();
    return 0;
} 