cmake_minimum_required(VERSION 3.20)
project(GoogleKeepSync VERSION 1.0.0 LANGUAGES CXX)

# ARM64 Windows target
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ARM64)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC ARM64 compiler settings
if(MSVC)
    add_compile_options(
        /W4
        /WX-
        /EHsc
        /Zc:__cplusplus
        /permissive-
        /MP             # Multi-processor compilation
    )
    
    # ARM64 specific
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64" OR CMAKE_VS_PLATFORM_NAME STREQUAL "ARM64")
        message(STATUS "Building for ARM64 Windows")
        add_compile_options(/favor:ARM64)
    endif()
endif()

# Source files
set(SOURCES
    src/DllMain.cpp
    src/PluginCore.cpp
    src/GoogleKeepClient.cpp
    src/OAuthManager.cpp
    src/WinHttpClient.cpp
    src/ConfigDialog.cpp
)

# Header files
set(HEADERS
    include/PluginInterface.h
    include/GoogleKeepAPI.h
    include/PluginCore.h
    include/ConfigDialog.h
)

# Create DLL
add_library(GoogleKeepSync SHARED ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(GoogleKeepSync PRIVATE 
    include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link Windows libraries
target_link_libraries(GoogleKeepSync PRIVATE
    kernel32
    user32
    gdi32
    winspool
    comdlg32
    advapi32
    shell32
    ole32
    oleaut32
    uuid
    odbc32
    odbccp32
    winhttp
    crypt32
    ws2_32
)

# Define UNICODE
add_definitions(-DUNICODE -D_UNICODE)

# Disable compiler warnings that are not relevant for this project
if(MSVC)
    add_compile_options(/wd4996)  # Disable warning about deprecated functions
endif()

# Set output name
set_target_properties(GoogleKeepSync PROPERTIES
    OUTPUT_NAME "GoogleKeepSync"
    PREFIX ""
    SUFFIX ".dll"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Notepad++ plugin directory (for debugging)
set(NPP_PLUGIN_DIR "${CMAKE_SOURCE_DIR}/dist" CACHE PATH "Notepad++ plugin directory")

# Copy to Notepad++ directory for testing
add_custom_command(TARGET GoogleKeepSync POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NPP_PLUGIN_DIR}/plugins/GoogleKeepSync"
    COMMAND ${CMAKE_COMMAND} -E copy 
        "$<TARGET_FILE:GoogleKeepSync>"
        "${NPP_PLUGIN_DIR}/plugins/GoogleKeepSync/"
    COMMENT "Copying plugin to distribution directory"
)

# Install
install(TARGETS GoogleKeepSync
    RUNTIME DESTINATION plugins/GoogleKeepSync
    LIBRARY DESTINATION plugins/GoogleKeepSync
)

# Package configuration
set(CPACK_PACKAGE_NAME "GoogleKeepSync")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Notepad++ plugin for syncing with Google Keep")
set(CPACK_PACKAGE_VENDOR "NppGoogleKeepSync")
include(CPack)
