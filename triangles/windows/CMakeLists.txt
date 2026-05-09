# ═══════════════════════════════════════════════════════════════════════════════
#  Target: win32-d3d12  (Win32 + D3D12)
# ═══════════════════════════════════════════════════════════════════════════════
add_executable(win32-d3d12 win32_d3d12.cpp)
target_compile_definitions(win32-d3d12 PRIVATE UNICODE _UNICODE)
target_compile_options(win32-d3d12 PRIVATE /utf-8)
target_link_libraries(win32-d3d12 PRIVATE d3d12 dxgi d3dcompiler)

message(STATUS "Targets: win32-d3d12")
