// main.cpp - WebView2를 사용한 Windows GUI 애플리케이션
#include <iostream>
#include <string>
#include <algorithm>

#include <windows.h>
// PathRemoveFileSpecW
#include <shlwapi.h>
#include <WebView2.h>

// JSON 파싱
#include <nlohmann/json.hpp>

// WRL (Callback 헬퍼 필수)
#include <wrl.h>
using Microsoft::WRL::Callback;

// COM 스마트 포인터 (WIL 우선, 없으면 WRL)
#ifdef USE_WIL
#include <wil/com.h>
#define COM_PTR wil::com_ptr
#else
using namespace Microsoft::WRL;
#define COM_PTR ComPtr
#endif

using json = nlohmann::json;

// 전역 변수
HINSTANCE g_hInst;
HWND g_hWnd;
COM_PTR<ICoreWebView2Controller> g_webviewController;
COM_PTR<ICoreWebView2> g_webview;

// 함수 선언
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool InitializeWebView2(HWND hwnd);
void ResizeWebView();
HRESULT CheckWebView2Runtime();
HRESULT OnWebView2EnvironmentCreated(HRESULT result, ICoreWebView2Environment* env, HWND hwnd);
HRESULT OnWebView2ControllerCreated(HRESULT result, ICoreWebView2Controller* controller, HWND hwnd);
HRESULT OnWebMessageReceived(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args);

// WinMain 진입점
int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ [[maybe_unused]] HINSTANCE hPrevInstance,
	_In_ [[maybe_unused]] PWSTR pCmdLine,
	_In_ int nCmdShow
)
{
	// 1. COM 초기화 (WebView2 필수)
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
	{
		MessageBoxW(nullptr, L"COM 초기화 실패", L"오류", MB_ICONERROR);
		return -1;
	}

	// 2. WebView2 Runtime 확인
	if (FAILED(CheckWebView2Runtime()))
	{
		CoUninitialize();
		return -1;
	}

	g_hInst = hInstance;

	// 3. 윈도우 클래스 등록
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = L"WebViewOnCppWindowClass";

	if (RegisterClassExW(&wc) == 0)
	{
		MessageBoxW(nullptr, L"윈도우 클래스 등록 실패", L"오류", MB_ICONERROR);
		CoUninitialize();
		return -1;
	}

	// 4. 메인 윈도우 생성
	g_hWnd = CreateWindowExW(NULL
		, wc.lpszClassName
		, L"WebView on C++"
		, WS_OVERLAPPEDWINDOW
		, CW_USEDEFAULT, CW_USEDEFAULT
		, 1280, 720
		, nullptr, nullptr, hInstance, nullptr
	);

	if (g_hWnd == nullptr)
	{
		MessageBoxW(nullptr, L"윈도우 생성 실패", L"오류", MB_ICONERROR);
		CoUninitialize();
		return -1;
	}

	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);

	// 5. WebView2 초기화 (비동기)
	if (InitializeWebView2(g_hWnd) == false)
	{
		MessageBoxW(nullptr, L"WebView2 초기화 실패", L"오류", MB_ICONERROR);
		CoUninitialize();
		return -1;
	}

	// 6. 메시지 루프
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// 7. 정리
	CoUninitialize();
	return (int)msg.wParam;
}

// 윈도우 프로시저
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		// WebView 크기 조정
		if (g_webviewController)
		{
			ResizeWebView();
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

// WebView2 초기화
bool InitializeWebView2(HWND hwnd)
{
	// UserDataFolder 경로 설정
	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	PathRemoveFileSpecW(exePath);
	std::wstring userDataFolder = std::wstring(exePath) + L"\\WebView2Data";

	// WebView2 환경 생성 (비동기)
	HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr
		, userDataFolder.c_str()
		, nullptr
		, Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[hwnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
			{
				return OnWebView2EnvironmentCreated(result, env, hwnd);
			}
		).Get()
	);

	return SUCCEEDED(hr);
}

// WebView2 환경 생성 완료 핸들러
HRESULT OnWebView2EnvironmentCreated(HRESULT result, ICoreWebView2Environment* env, HWND hwnd)
{
	if (FAILED(result))
	{
		MessageBoxW(nullptr, L"WebView2 환경 생성 실패", L"오류", MB_ICONERROR);
		return result;
	}

	// WebView2 컨트롤러 생성
	env->CreateCoreWebView2Controller(hwnd
		, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
			[hwnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
			{
				return OnWebView2ControllerCreated(result, controller, hwnd);
			}
		).Get()
	);

	return S_OK;
}

// WebView2 컨트롤러 생성 완료 핸들러
HRESULT OnWebView2ControllerCreated(HRESULT result, ICoreWebView2Controller* controller, [[maybe_unused]] HWND hwnd)
{
	if (FAILED(result))
	{
		MessageBoxW(nullptr, L"WebView2 컨트롤러 생성 실패", L"오류", MB_ICONERROR);
		return result;
	}

	g_webviewController = controller;
	g_webviewController->get_CoreWebView2(&g_webview);

	// WebView2 설정
	ICoreWebView2Settings* settings;
	g_webview->get_Settings(&settings);
	settings->put_IsScriptEnabled(TRUE);
	settings->put_IsWebMessageEnabled(TRUE);
	settings->put_AreDevToolsEnabled(TRUE);
	settings->put_AreDefaultScriptDialogsEnabled(TRUE);

	// 크기 조정
	ResizeWebView();

	// JS → C++ 메시지 수신 핸들러 등록
	g_webview->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
			[](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
			{
				return OnWebMessageReceived(sender, args);
			}
		).Get()
		, nullptr
	);

	// HTML 로드
	wchar_t exePathLocal[MAX_PATH];
	GetModuleFileNameW(nullptr, exePathLocal, MAX_PATH);
	PathRemoveFileSpecW(exePathLocal);

	std::wstring uri = L"file:///" + std::wstring(exePathLocal) + L"/ui/index.html";
	std::replace(uri.begin(), uri.end(), L'\\', L'/');

	std::wcout << L"로딩 URI: " << uri << std::endl;
	g_webview->Navigate(uri.c_str());

	return S_OK;
}

// JS → C++ 메시지 수신 핸들러
HRESULT OnWebMessageReceived(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
{
	LPWSTR messageRaw;
	args->get_WebMessageAsJson(&messageRaw);

	// UTF-16 → UTF-8 변환
	int size = WideCharToMultiByte(CP_UTF8, 0, messageRaw, -1, nullptr, 0, nullptr, nullptr);
	std::string utf8Message(size, 0);
	WideCharToMultiByte(CP_UTF8, 0, messageRaw, -1, &utf8Message[0], size, nullptr, nullptr);
	CoTaskMemFree(messageRaw);

	try
	{
		// JSON 파싱
		json j = json::parse(utf8Message);
		std::string type = j.value("type", "");
		std::string payload = j.value("payload", "");

		std::cout << "[C++ 수신] type: " << type
			<< ", payload: " << payload << std::endl;

		// 응답 생성
		json response = {
			{"type", "pong"},
			{"payload", "Hello from C++"},
			{"receivedType", type},
			{"timestamp", std::to_string(GetTickCount64())}
		};

		// UTF-8 → UTF-16 변환
		std::string responseStr = response.dump();
		int wsize = MultiByteToWideChar(CP_UTF8, 0, responseStr.c_str(), -1, nullptr, 0);
		std::wstring wresponse(wsize, 0);
		MultiByteToWideChar(CP_UTF8, 0, responseStr.c_str(), -1, &wresponse[0], wsize);

		sender->PostWebMessageAsJson(wresponse.c_str());
	}
	catch (const json::exception& e)
	{
		std::cerr << "[C++ 오류] JSON 파싱 실패: " << e.what() << std::endl;
		MessageBoxA(nullptr, e.what(), "JSON 파싱 오류", MB_ICONWARNING);
	}

	return S_OK;
}

// WebView 크기 조정
void ResizeWebView()
{
	if (g_webviewController)
	{
		RECT bounds;
		GetClientRect(g_hWnd, &bounds);
		g_webviewController->put_Bounds(bounds);
	}
}

// WebView2 런타임 확인
HRESULT CheckWebView2Runtime()
{
	wchar_t* version;
	HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version);

	if (SUCCEEDED(hr))
	{
		std::wcout << L"WebView2 Runtime 버전: " << version << std::endl;
		CoTaskMemFree(version);
		return S_OK;
	}
	else
	{
		MessageBoxW(nullptr
			, L"WebView2 Runtime이 설치되지 않았습니다.\n\n다운로드: https://go.microsoft.com/fwlink/?linkid=2124701"
			, L"WebView2 Runtime 필요"
			, MB_ICONERROR | MB_OK
		);
		return E_FAIL;
	}
}
