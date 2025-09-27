cmake_minimum_required(VERSION 3.1)

project(ImageDisplay)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# wxWidgets build toggle
if(WIN32)
    set(wxBUILD_SHARED OFF)
else()
    set(wxBUILD_SHARED ON)
endif()

add_subdirectory(dependency/wxWidgets)

# --- App sources (GUI) ---
set(SOURCES_APP
    src/Main.cpp
    src/Config.cpp
    src/ImageIO.cpp
    src/ColorSpace.cpp
    src/Quantizer.cpp
    src/Metrics.cpp
)
if(WIN32)
    list(APPEND SOURCES_APP manifest/MyImageApplication.rc)
endif()

# --- Analysis sources (CLI) ---
set(SOURCES_ANALYSIS
    src/Analysis.cpp
    src/Config.cpp
    src/ImageIO.cpp
    src/ColorSpace.cpp
    src/Quantizer.cpp
    src/Metrics.cpp
)

# Create executables
add_executable(MyImageApplication ${SOURCES_APP})
add_executable(Analysis ${SOURCES_ANALYSIS})

# X11 required on Linux (no-op on macOS/Windows)
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()

# Link libraries
target_link_libraries(MyImageApplication wx::core wx::base ${X11_LIBRARIES})
target_link_libraries(Analysis           wx::core wx::base ${X11_LIBRARIES})
