# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

WebView on C++ (woc) is a lightweight Windows GUI application using Win32 + WebView2 with a TypeScript/Vite frontend. It demonstrates bidirectional JSON communication between C++ and JavaScript without requiring Electron, Node.js runtime, or external HTTP servers.

## Build Commands

### Frontend Build
```bash
cd ui
npm install        # First time only
npm run build      # Builds to ui/dist/
```

### C++ Build
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 18 2026" -A x64
cmake --build . --config Debug    # or Release/Profile
```

### Run
```bash
cd bin/Debug      # or bin/Release
./main.exe
```

### Clean Build
```bash
rm -rf build bin
# Then rebuild from scratch
```

## Architecture

### Communication Layer
The core architecture is a **message-passing bridge** between C++ and JavaScript:

**C++ → JS:** Uses `ICoreWebView2::PostWebMessageAsJson()` to send JSON messages
**JS → C++:** Uses `window.chrome.webview.postMessage()` captured by `add_WebMessageReceived` handler
**Encoding:** All messages undergo UTF-16 ↔ UTF-8 conversion using `WideCharToMultiByte` and `MultiByteToWideChar`

### Async Initialization Chain
WebView2 initialization is fully asynchronous with nested callbacks:
1. `CreateCoreWebView2EnvironmentWithOptions` → Environment created
2. `CreateCoreWebView2Controller` → Controller created
3. Configure settings, register message handlers, navigate to HTML

This callback chain is critical - the window must be shown before WebView2 initialization, and all WebView operations must occur after the controller is created.

### Resource Loading Strategy
- Frontend builds to `ui/dist/` with **relative paths** (`base: './'` in vite.config.ts)
- CMake post-build copies `ui/dist/` → `bin/ui/`
- C++ loads via `file:///` protocol with path normalization (backslashes → forward slashes)
- WebView2Loader.dll is also auto-copied to output directory

### Smart Pointer Selection
The codebase conditionally uses either:
- **WIL** (`wil::com_ptr`) if `external/wil/` exists → `USE_WIL` defined
- **WRL** (`Microsoft::WRL::ComPtr`) as fallback

Both use the `COM_PTR` macro for abstraction. Note: `Callback<>` always comes from WRL regardless.

## Critical Configuration

### Vite Base Path
`vite.config.ts` **must** set `base: './'` for relative asset paths. Absolute paths (`base: '/'`) will break `file:///` loading.

### CMake Custom Commands
Two post-build commands are essential:
1. Copy `WebView2Loader.dll` to output dir
2. Copy `ui/dist/` to output `ui/` subdirectory

If either fails, the app won't run properly.

### MSVC Compiler Flag
`cmake/compiler/msvc.cmake` disables C4819 warning (`/wd4819`) to prevent UTF-8 encoding warnings from being treated as errors.

## Dependencies

**Git Submodules:**
- `external/json/` - nlohmann/json (header-only)
- `external/wil/` - Windows Implementation Library (optional)

**NuGet Package:**
- `packages/Microsoft.Web.WebView2.*` - Downloaded via nuget.exe, auto-detected by CMake glob

**Runtime:**
- WebView2 Runtime must be installed on target machine

## Common Issues

**Blank window:** Check `ui/dist/` was copied to `bin/ui/`, verify `base: './'` in vite.config.ts, check console for file:/// URI

**`chrome.webview` undefined:** Ensure `settings->put_IsWebMessageEnabled(TRUE)` is called in WebView2 initialization

**DLL missing:** Verify CMake post-build command copied WebView2Loader.dll

**Immediate crash:** Check WebView2 Runtime is installed, verify COM initialized with `CoInitializeEx(COINIT_APARTMENTTHREADED)`

**JSON parse errors:** Check UTF-8/UTF-16 conversion in message handlers

## Development Workflow

1. Make frontend changes in `ui/src/`
2. Run `npm run build` from `ui/`
3. Rebuild C++ (CMake will auto-copy new `ui/dist/`)
4. Run `main.exe` from `bin/Debug/`
5. Press F12 to open DevTools for JS debugging

For C++ changes, only CMake rebuild is needed. The frontend is loaded from disk at runtime.

## Coding Style Guidelines

### C++ Code Style
- **Brace Style:** Allman style (braces on new lines)
- **Indentation:** Tab size 4 (use tabs, not spaces)
- **Comments:** Korean comments are allowed and encouraged for clarity
- **Parameter Line Breaks:** When breaking function parameters across lines, place comma before parameter: `, param` not `param,`
- **Explicit Comparisons:** Avoid `!` operator. Use explicit comparisons: `== false`, `== nullptr`, `!= 0` instead of `!value`
  - **Exception:** Simple variable checks (just variable name, no operators) are acceptable: `if (ptr)` or `if (controller)` is OK

**Example:**
```cpp
void ExampleFunction()
{
	if (condition)
	{
		// 한글 주석 사용 가능
		DoSomething();
	}
	else
	{
		DoSomethingElse();
	}
}

// 함수 매개변수 줄 넘김 (쉼표가 앞에)
CreateWindowExW(NULL
	, wc.lpszClassName
	, L"Window Title"
	, WS_OVERLAPPEDWINDOW
	, CW_USEDEFAULT, CW_USEDEFAULT
	, 1280, 720
	, nullptr, nullptr, hInstance, nullptr
);

// 명시적 비교 연산 사용
if (ptr == nullptr)       // Good: explicit comparison
if (result == false)      // Good: explicit comparison
if (count != 0)           // Good: explicit comparison

// 단순 변수 체크 예외 (허용됨)
if (ptr)                  // OK: simple variable check (no negation)
if (g_webviewController)  // OK: simple variable check

// 피해야 할 패턴
if (!ptr)                 // Avoid: negation operator
if (!result)              // Avoid: negation operator
```

### TypeScript/JavaScript Code Style
- Follow project conventions (see existing `ui/src/main.ts`)
- Use meaningful variable names
- Add comments for complex logic

## File Encoding

All C++ source files should handle Korean comments. The build system disables C4819 warnings to allow UTF-8 encoded source files with BOM.
