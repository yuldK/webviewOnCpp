set(target main)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

make_executable(${target} "WINDOWS")

#add_pch(${target} pch.cpp pch.h)

add_sources(${target}
	"console"
	"main.cpp"
)

target_link_libraries(${target}
	PUBLIC
		${WINLIBS}
)
