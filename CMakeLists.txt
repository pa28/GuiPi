cmake_minimum_required(VERSION 3.10.0)

project("GuiPi" VERSION 0.1.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
        "${CMAKE_MODULE_PATH}")

# Required libraries for linking against nngui (all targets)
set(EXTRA_LIBS "")

include(CheckCXXCompilerFlag)
find_file(BCMHOST bcm_host.h PATHS /opt/vc/include NO_DEFAULT_PATH)
if (NOT BCMHOST)
    add_compile_definitions(X86HOST=1)
else()
    add_compile_definitions(BCMHOST=1)
endif()

find_package(Soci REQUIRED COMPONENTS sqlite3)
include_directories(${SOCI_INCLUDE_DIRS} "/usr/include")

set(CMAKE_VERBOSE_MAKEFILE OFF)

add_compile_definitions(VERSION=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}-${PROJECT_VERSION_TWEAK})

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(DIRECTORY_PREFIX="./")
    SET(DIRECTORY_PREFIX "./")
ELSE()
    add_compile_definitions(DIRECTORY_PREFIX="/var/lib/hamchrono/")
    SET(DIRECTORY_PREFIX "/var/lib/hamchrono/")
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

MESSAGE(STATUS "Dir prefix:  ${DIRECTORY_PREFIX}")

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
find_package(SDL2_IMAGE REQUIRED)
find_package(SDL2TTF REQUIRED)
find_package(CURLPP REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2TTF_INCLUDE_DIR})
include_directories(${SDL2_IMAGE_INCLUDE_DIR})
include_directories(${CURLPP_INCLUDE_DIR})

# Required core libraries on various platforms
list(APPEND EXTRA_LIBS GL Xxf86vm Xrandr Xinerama Xcursor Xi X11 pthread dl rt m)

message(${EXTRA_LIBS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR})

INCLUDE(sdlgui/CMakeLists.txt)
INCLUDE(guipi/CMakeLists.txt)

option(BUILD_EXAMPLE "Build example application" ON)
if (BUILD_EXAMPLE)

    add_executable(hamchrono ${GUIPI_SOURCES} ${SDLGUI_SOURCES} hamchrono.cpp)
    set_target_properties(
            hamchrono PROPERTIES
            VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    )

    target_link_libraries(hamchrono ${EXTRA_LIBS} ${SDL2_LIBRARY} ${SDL2IMAGE_LIBRARY} ${SDL2TTF_LIBRARY} ${CURLPP_LIBRARIES}
            ${SOCI_LIBRARY} ${SOCI_sqlite3_PLUGIN} -lsqlite3)

    # Copy icons for example application
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/maps)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/backgrounds)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/images)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fonts)
    file(COPY resources/maps DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY resources/backgrounds DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY resources/icons DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY resources/fonts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif ()

install(TARGETS
        hamchrono
        RUNTIME DESTINATION bin
        )

install(DIRECTORY resources/maps/ DESTINATION /var/lib/hamchrono/maps
        USE_SOURCE_PERMISSIONS
        PATTERN "maps/*")

install(DIRECTORY resources/backgrounds/ DESTINATION /var/lib/hamchrono/backgrounds
        USE_SOURCE_PERMISSIONS
        PATTERN "backgrounds/*")

install(DIRECTORY resources/fonts/ DESTINATION /var/lib/hamchrono/fonts
        USE_SOURCE_PERMISSIONS
        PATTERN "fonts/*")

install(DIRECTORY resources/images/ DESTINATION /var/lib/hamchrono/images
        USE_SOURCE_PERMISSIONS
        PATTERN "images/*")

set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_RELEASE ${PROJECT_VERSION_TWEAK})
set(CPACK_RPM_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "hamchrono")
set(CPACK_DEBIAN_PACKAGE_SECTION "hamradio")
set(CPACK_RPM_PACKAGE_LICENSE "GPL")
set(CPACK_RPM_PACKAGE_URL "http://example.com")
set(CPACK_RPM_PACKAGE_SUMMARY "A multipurpose time piece for amateur radio operators, targeting the Raspberry Pi.")
set(CPACK_RPM_PACKAGE_DESCRIPTION "A multipurpose time piece for amateur radio operators, targeting the Raspberry Pi")
set(CPACK_RPM_PACKAGE_VENDOR "www.hrbuckley.net")
set(CPACK_PACKAGE_CONTACT "Richard Buckley")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_EXECUTABLES hamchrono)
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        /usr/local
        /usr/local/bin
        /usr/local/lib64
        /usr/local/share
        /usr/local/include
        /usr/lib/systemd
        /usr/lib/systemd/system
        )
include(CPack)

