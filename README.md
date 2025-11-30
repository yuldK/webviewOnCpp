# WebView on C++ (woc)

WebView2 + C++ + TypeScript/Vite를 사용한 최소한의 Windows GUI 애플리케이션

## 📋 프로젝트 개요

순수 Win32 + WebView2를 기반으로 한 경량 Windows GUI 도구입니다. Electron이나 Node.js 런타임 없이 단일 실행 파일과 정적 리소스만으로 배포 가능합니다.

**특징:**
- ✅ C++20 네이티브 성능
- ✅ WebView2 기반 현대적 UI (Chromium)
- ✅ TypeScript + Vite 프론트엔드
- ✅ JSON 기반 양방향 통신 (C++ ↔ JS)
- ✅ 단일 exe + 정적 리소스 배포
- ✅ 외부 HTTP 서버 불필요

## 🔧 빌드 요구사항

- **OS:** Windows 10/11
- **컴파일러:** MSVC 2026 (Visual Studio 2026)
- **CMake:** 4.2 이상
- **Node.js:** 18.x 이상 (프론트엔드 빌드용)
- **WebView2 Runtime:** [다운로드](https://go.microsoft.com/fwlink/?linkid=2124701)

## 🚀 빌드 순서

### 1단계: 의존성 확인

이미 설치되어 있어야 합니다 (git submodule로 추가됨):
- `external/json/` - nlohmann/json
- `external/wil/` - Windows Implementation Library
- `packages/Microsoft.Web.WebView2.*` - WebView2 SDK

### 2단계: 프론트엔드 빌드

```bash
cd ui
npm install
npm run build
```

결과: `ui/dist/` 디렉터리 생성

### 3단계: C++ 빌드

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 18 2026" -A x64
cmake --build . --config Debug
```

결과: `bin/main.exe` 생성

### 4단계: 실행

```bash
cd bin
main.exe
```

## 📁 프로젝트 구조

```
webviewOnCpp/
├── external/                    # Git submodules
│   ├── json/                    # nlohmann/json
│   └── wil/                     # Windows Implementation Library
├── packages/                    # NuGet 패키지
│   └── Microsoft.Web.WebView2.*/
├── ui/                          # 프론트엔드 (Vite + TypeScript)
│   ├── src/
│   │   ├── main.ts             # 메인 로직
│   │   ├── style.css
│   │   └── vite-env.d.ts       # WebView2 타입 정의
│   ├── index.html
│   ├── package.json
│   ├── vite.config.ts          # base: './' 필수!
│   └── tsconfig.json
├── code/main/
│   ├── main.cpp                 # C++ 메인 구현
│   └── CMakeLists.txt           # 빌드 설정
├── cmake/                       # CMake 설정 파일
└── bin/                         # 출력 디렉터리
    ├── main.exe
    ├── WebView2Loader.dll
    └── ui/                      # 프론트엔드 빌드 결과
```

## 💬 메시지 통신

### JS → C++ 메시지 전송

```typescript
window.chrome.webview.postMessage({
  type: 'ping',
  payload: 'hello from js',
  timestamp: new Date().toISOString()
});
```

### C++ → JS 메시지 전송

```cpp
json response = {
    {"type", "pong"},
    {"payload", "Hello from C++"},
    {"timestamp", std::to_string(GetTickCount64())}
};

std::string responseStr = response.dump();
// UTF-8 → UTF-16 변환 후
g_webview->PostWebMessageAsJson(wresponse.c_str());
```

### 메시지 수신 (JS)

```typescript
window.chrome.webview.addEventListener('message', (event) => {
  console.log('C++에서 수신:', event.data);
  // { type: "pong", payload: "Hello from C++", ... }
});
```

## 🎨 주요 기능

1. **WebView2 초기화**
   - COM 기반 비동기 초기화
   - 런타임 자동 탐지
   - file:/// 프로토콜로 로컬 HTML 로드

2. **JSON 통신**
   - nlohmann/json을 사용한 안전한 파싱
   - UTF-8 ↔ UTF-16 자동 변환
   - 오류 처리 (try-catch + MessageBox)

3. **스마트 포인터**
   - WIL 우선 사용 (wil::com_ptr)
   - 없으면 WRL 사용 (ComPtr)
   - 자동 메모리 관리

4. **개발자 도구**
   - F12로 DevTools 열기
   - 콘솔 로그 확인
   - 네트워크 디버깅

## ⚙️ 설정 파일 주요 내용

### vite.config.ts

```typescript
export default defineConfig({
  base: './',  // ⚠️ 필수: file:/// 로딩을 위한 상대 경로
  build: {
    outDir: 'dist',
    assetsDir: 'assets'
  }
});
```

### CMakeLists.txt (code/main/)

```cmake
# nlohmann/json 추가
include_directories("${CMAKE_SOURCE_DIR}/external/json/include")

# WIL 추가 (있으면)
if(EXISTS "${CMAKE_SOURCE_DIR}/external/wil/include")
    add_compile_definitions(USE_WIL)
endif()

# WebView2 링크
target_link_libraries(${target}
    WebView2Loader.dll.lib
    ole32.lib oleaut32.lib uuid.lib ...
)
```

## 🐛 문제 해결

### 문제: 빈 화면만 표시됨

**원인:** HTML 로드 실패

**해결:**
1. `vite.config.ts`에서 `base: './'` 확인
2. `ui/dist/` 폴더가 `bin/ui/`로 복사되었는지 확인
3. 콘솔 로그 확인: "로딩 URI: file:///..."

### 문제: `chrome.webview` is undefined

**원인:** WebView2 설정 누락

**해결:**
```cpp
settings->put_IsWebMessageEnabled(TRUE);
```

### 문제: WebView2Loader.dll 오류

**원인:** DLL이 복사되지 않음

**해결:** CMake에서 post-build 커맨드 확인
```cmake
add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ...
)
```

### 문제: 앱이 즉시 종료됨

**원인:** WebView2 Runtime 미설치 또는 COM 초기화 실패

**해결:**
1. [WebView2 Runtime 설치](https://go.microsoft.com/fwlink/?linkid=2124701)
2. 콘솔 오류 메시지 확인

### 문제: JSON 파싱 오류

**원인:** 인코딩 문제 또는 잘못된 JSON

**해결:** UTF-8 ↔ UTF-16 변환 확인
```cpp
WideCharToMultiByte(CP_UTF8, 0, messageRaw, -1, ...);
```

## 📦 배포

### 배포 패키지 구조

```
배포폴더/
├── main.exe
├── WebView2Loader.dll
└── ui/
    ├── index.html
    └── assets/
        ├── main-[hash].js
        └── main-[hash].css
```

### 배포 시 주의사항

1. **WebView2 Runtime 필요**
   - 사용자 PC에 WebView2 Runtime 설치 필요
   - [다운로드 링크](https://go.microsoft.com/fwlink/?linkid=2124701)

2. **DLL 포함**
   - `WebView2Loader.dll`을 exe와 같은 폴더에 배치

3. **UI 리소스**
   - `ui/` 폴더 전체를 exe와 같은 폴더에 배치

### ZIP 패키지 생성

```powershell
Compress-Archive -Path bin/* -DestinationPath woc-v1.0.0.zip
```

## 🔗 참고 자료

- [WebView2 공식 문서](https://learn.microsoft.com/en-us/microsoft-edge/webview2/)
- [nlohmann/json](https://github.com/nlohmann/json)
- [Windows Implementation Library](https://github.com/microsoft/wil)
- [Vite 공식 문서](https://vite.dev/)

## 📝 라이선스

이 프로젝트는 MIT 라이선스를 따릅니다.

## 🤝 기여

Issue나 Pull Request를 통해 기여해주세요!
