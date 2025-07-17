#pragma once
#include <windows.h>
#include <iostream>

#include <dxgi.h>
#include <wrl/client.h>  // ComPtr
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <wincodec.h>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <chrono>
#include <thread>



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
using Microsoft::WRL::ComPtr;