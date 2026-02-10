cmake_minimum_required(VERSION 3.20)
project(GoogleKeepSync VERSION 1.0.0 LANGUAGES CXX)

# ARM64 Windows target
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ARM64)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC compiler settings - Unicode
if(MSVC)
    add_compile_options(/W4 /WX- /EHsc /Zc:__cplusplus /permissive- /MP /utf-8)
endif()

# Source files
set(SOURCES
    src/DllMain.cpp
    src/PluginCore.cpp
    src/ConfigDialog.cpp
    gkeep_bridge/PythonBridge.cpp
)

# Header files
set(HEADERS
    include/PluginInterface.h
    include/PluginCore.h
    include/ConfigDialog.h
    gkeep_bridge/PythonBridge.h
)

# Create DLL
add_library(GoogleKeepSync SHARED ${SOURCES} ${HEADERS})

# Notepad++ SDK - Auto-download if not present
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/Notepad_plus/include/PluginInterface.h")
    message(STATUS "Downloading Notepad++ SDK...")
    file(DOWNLOAD 
        "https://raw.githubusercontent.com/notepad-plus-plus/notepad-plus-plus/master/PowerEditor/src/Misc/Notepad_plus/include/PluginInterface.h"
        "${CMAKE_SOURCE_DIR}/Notepad_plus/include/PluginInterface.h"
    )
    file(DOWNLOAD 
        "https://raw.githubusercontent.com/notepad-plus-plus/notepad-plus-plus/master/PowerEditor/src/Misc/Notepad_plus/include/Scintilla.h"
        "${CMAKE_SOURCE_DIR}/Notepad_plus/include/Scintilla.h"
    )
    file(DOWNLOAD 
        "https://raw.githubusercontent.com/notepad-plus-plus/notepad-plus-plus/master/PowerEditor/src/Misc/Notepad_plus/include/Notepad_plus.h"
        "${CMAKE_SOURCE_DIR}/Notepad_plus/include/Notepad_plus.h"
    )
endif()

# Define UNICODE for Notepad++ compatibility
target_compile_definitions(GoogleKeepSync PRIVATE UNICODE _UNICODE)

# Include directories
target_include_directories(GoogleKeepSync PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/gkeep_bridge
    ${CMAKE_SOURCE_DIR}/Notepad_plus/include
)

# Link Windows libraries
target_link_libraries(GoogleKeepSync PRIVATE
    kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32 ole32 oleaut32 uuid odbc32 odbccp32 winhttp crypt32 ws2_32
)

# Set output name
set_target_properties(GoogleKeepSync PROPERTIES
    OUTPUT_NAME "GoogleKeepSync"
    PREFIX ""
    SUFFIX ".dll"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Package configuration
set(CPACK_PACKAGE_NAME "GoogleKeepSync")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Notepad++ plugin for syncing with Google Keep")
set(CPACK_PACKAGE_VENDOR "NppGoogleKeepSync")
include(CPack)
