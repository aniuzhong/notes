cmake_minimum_required(VERSION 3.13)

project(Interposition)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
add_definitions(-D_WINDOWS)
add_definitions(-D_UNICODE -DUNICODE -DXxxxPLAYER__LIBRARY)
endif()

# XxxxPlayer dll/so

set(SRC_XxxxPLAYER
    ${PROJECT_SOURCE_DIR}/XxxxPlayer/XxxxPlayerExport.cpp
    ${PROJECT_SOURCE_DIR}/XxxxPlayer/XxxxAudioCapturePlayerExport.cpp
    ${PROJECT_SOURCE_DIR}/XxxxPlayer/Common.cpp
    ${PROJECT_SOURCE_DIR}/CommonFile/XxxxPlayerLog.cpp
)

if (MSVC)
    list(APPEND SRC_XxxxPLAYER
        ${PROJECT_SOURCE_DIR}/XxxxPlayer/dllmain.cpp
    )
endif()

add_library(XxxxPlayer SHARED
    ${SRC_XxxxPLAYER}
)

target_include_directories(XxxxPlayer PRIVATE
    ${PROJECT_SOURCE_DIR}
)

# XxxxAudioCaptureCore dll/so

set(SRC_Xxxx_AUDIOCAPTURE_CORE
    ${PROJECT_SOURCE_DIR}/XxxxAudioCaptureCore/AudioCaptureExport.cpp
    ${PROJECT_SOURCE_DIR}/CommonFile/XxxxPlayerLog.cpp
)

add_library(XxxxAudioCaptureCore SHARED
    ${SRC_Xxxx_AUDIOCAPTURE_CORE}
)

target_include_directories(XxxxAudioCaptureCore PRIVATE
    ${PROJECT_SOURCE_DIR}
)

if (NOT MSVC)
target_link_options(XxxxAudioCaptureCore PRIVATE "-Wl,-Bsymbolic")
endif()

# XxxxPlayerDemo exe

set(SRC_XxxxPLAYER_DEMO
    ${PROJECT_SOURCE_DIR}/XxxxPlayerDemo/main.cpp
    "${PROJECT_SOURCE_DIR}/XxxxPlayerDemo/Log.cpp"
)

add_executable(XxxxPlayerDemo
    ${SRC_XxxxPLAYER_DEMO}
)

target_include_directories(XxxxPlayerDemo PRIVATE
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(XxxxPlayerDemo PRIVATE XxxxPlayer)