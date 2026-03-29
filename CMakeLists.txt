cmake_minimum_required(VERSION 3.10)
project(ModularCheat)

set(CMAKE_CXX_STANDARD 17)

# Global Defines for Consistency
add_definitions(-DSTRICT -DWIN32_LEAN_AND_MEAN)

# Specific Modular Files only
set(SOURCES 
    "src/main.cpp"
    "src/Features/LocalPlayer.cpp"
    "src/Features/ESP.cpp"
    "src/UI/Menu.cpp"
    "src/UI/Overlay.cpp"
)

add_library(ModularEngine SHARED ${SOURCES})

target_include_directories(ModularEngine PRIVATE src)

if(MSVC)
    # Use static CRT (/MT) to avoid "DLL not found" errors
    add_compile_options(/MT /GS- /sdl- /GR- /wd4267)
    set_target_properties(ModularEngine PROPERTIES 
        LINK_FLAGS "/MANIFEST:NO /DYNAMICBASE /NXCOMPAT"
        OUTPUT_NAME "ModularEngine"
    )
endif()

target_link_libraries(ModularEngine PRIVATE user32.lib kernel32.lib gdi32.lib)
