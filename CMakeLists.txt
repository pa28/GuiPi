cmake_minimum_required(VERSION 3.10.0)

project("GuiPi")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Required libraries for linking against nngui (all targets)
set(EXTRA_LIBS "")

include(CheckCXXCompilerFlag)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
            "MinSizeRel" "RelWithDebInfo")
endif ()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Quench annoying deprecation warnings when compiling GLFW on OSX
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
endif ()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(SDL2 REQUIRED)
find_package(SDL2IMAGE REQUIRED)
find_package(SDL2TTF REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2TTF_INCLUDE_DIR})
include_directories(${SDL2_IMAGE_INCLUDE_DIR})

# Required core libraries on various platforms
list(APPEND EXTRA_LIBS GL Xxf86vm Xrandr Xinerama Xcursor Xi X11 pthread dl rt m)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR})

message(${EXTRA_LIBS})

INCLUDE(sdlgui/CMakeLists.txt)
INCLUDE(GuiPi/CMakeLists.txt)

option(BUILD_EXAMPLE "Build example application" ON)
if (BUILD_EXAMPLE)

    add_executable(hamchrono ${GUIPI_SOURCES} ${SDLGUI_SOURCES} hamchrono.cpp)
    set_target_properties(
            hamchrono PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build/debug")
    target_link_libraries(hamchrono ${EXTRA_LIBS} ${SDL2_LIBRARY} ${SDL2IMAGE_LIBRARY} ${SDL2TTF_LIBRARY})

    # Copy icons for example application
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/maps)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/backgrounds)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/images)
    file(COPY resources/maps DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY resources/images DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY resources/backgrounds DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY resources/icons DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY resources/entypo.ttf DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif ()
