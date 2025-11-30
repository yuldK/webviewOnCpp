set(target main)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# nlohmann/json 라이브러리 추가 (헤더 전용)
include_directories("${CMAKE_SOURCE_DIR}/external/json/include")

# WIL (Windows Implementation Library) - 있으면 사용
if(EXISTS "${CMAKE_SOURCE_DIR}/external/wil/include")
    include_directories("${CMAKE_SOURCE_DIR}/external/wil/include")
    add_compile_definitions(USE_WIL)
    message(STATUS "WIL 라이브러리 발견 - wil::com_ptr 사용")
else()
    message(STATUS "WIL 없음 - Microsoft::WRL::ComPtr 사용")
endif()

# WebView2 SDK 경로 설정 (glob으로 버전 자동 탐지)
file(GLOB WEBVIEW2_PACKAGE_DIRS "${CMAKE_SOURCE_DIR}/packages/Microsoft.Web.WebView2.*")
if(WEBVIEW2_PACKAGE_DIRS)
    list(SORT WEBVIEW2_PACKAGE_DIRS ORDER DESCENDING)
    list(GET WEBVIEW2_PACKAGE_DIRS 0 WEBVIEW2_ROOT)
    set(WEBVIEW2_SDK_PATH "${WEBVIEW2_ROOT}/build/native")
    message(STATUS "WebView2 SDK 발견: ${WEBVIEW2_ROOT}")
else()
    message(FATAL_ERROR "WebView2 SDK를 찾을 수 없습니다. 'nuget install Microsoft.Web.WebView2 -OutputDirectory packages/' 실행 필요")
endif()

# WebView2 include 및 library 경로 추가
include_directories("${WEBVIEW2_SDK_PATH}/include")
link_directories("${WEBVIEW2_SDK_PATH}/x64")

make_executable(${target} "WINDOWS")

#add_pch(${target} pch.cpp pch.h)

add_sources(${target}
	"console"
	"main.cpp"
)

target_link_libraries(${target}
	PUBLIC
		${WINLIBS}
		# WebView2
		WebView2Loader.dll.lib
		# COM 라이브러리
		ole32.lib
		oleaut32.lib
		uuid.lib
		advapi32.lib
		shlwapi.lib
)

# WebView2Loader.dll을 출력 디렉터리에 복사
add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WEBVIEW2_SDK_PATH}/x64/WebView2Loader.dll"
        "$<TARGET_FILE_DIR:${target}>"
    COMMENT "WebView2Loader.dll 복사 중..."
)

# UI 빌드 결과물을 출력 디렉터리에 복사
add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/ui/dist"
        "$<TARGET_FILE_DIR:${target}>/ui"
    COMMENT "UI 빌드 결과물 복사 중..."
)
