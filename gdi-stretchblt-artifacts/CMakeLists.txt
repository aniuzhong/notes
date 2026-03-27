cmake_minimum_required(VERSION 3.20)

project(gdi-stretchblt-artifacts LANGUAGES CXX)

add_executable(gdi-stretchblt-artifacts WIN32
    main.cpp
)

target_compile_features(gdi-stretchblt-artifacts PRIVATE cxx_std_17)
target_compile_definitions(gdi-stretchblt-artifacts PRIVATE
    UNICODE
    _UNICODE
)
target_link_libraries(gdi-stretchblt-artifacts PRIVATE
    comdlg32
    shlwapi
)

if(MSVC)
    target_compile_options(gdi-stretchblt-artifacts PRIVATE /W3 /permissive- /sdl)
endif()
