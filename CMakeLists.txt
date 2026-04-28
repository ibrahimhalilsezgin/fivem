cmake_minimum_required(VERSION 3.10)
project(SvcHost) # Innocent project name

set(CMAKE_CXX_STANDARD 17)

# Strip debug info and optimize
add_definitions(-DSTRICT -DWIN32_LEAN_AND_MEAN -DNDEBUG)

# Source files
set(SOURCES 
    "src/main.cpp"
    "src/Features/LocalPlayer.cpp"
    "src/Features/ESP.cpp"
    "src/UI/Menu.cpp"
    "src/UI/Overlay.cpp"
    "src/Game/Offsets.cpp"
    "src/Hooks.cpp"
    "src/LuaEngine.cpp"
    "src/Console.cpp"
    "src/Gui.cpp"
    "src/Memory.cpp"
    "src/Offsets.cpp"
    "src/Render/D3DRenderer.cpp"
)

add_library(SvcHost SHARED ${SOURCES})

target_include_directories(SvcHost PRIVATE src)

if(MSVC)
    # /GS- : __security_cookie kaldır
    # /GR- : RTTI kaldır (type_info string gizle)
    # /GL  : Whole program optimization (crypto inlining)
    # /O2  : Maksimum optimizasyon
    # /Ob2 : Aggressive inlining (forceinline uyumu)
    add_compile_options(/MT /GS- /sdl- /GR- /O2 /GL /Ob2 /wd4267 /wd4244)
    
    set_target_properties(SvcHost PROPERTIES 
        LINK_FLAGS "/MANIFEST:NO /DYNAMICBASE /NXCOMPAT /LTCG /OPT:REF /OPT:ICF /SAFESEH:NO"
        OUTPUT_NAME "svchost_x64"
    )
endif()

# D3D11 + DXGI kütüphaneleri eklendi (internal rendering için)
target_link_libraries(SvcHost PRIVATE 
    user32.lib 
    kernel32.lib 
    gdi32.lib 
    psapi.lib
    d3d11.lib
    dxgi.lib
)

# Injector executable
add_executable(Injector "injector/main.cpp")
target_compile_features(Injector PRIVATE cxx_std_17)

