cmake_minimum_required(VERSION 3.29)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  message(STATUS "Detected POLICY CMP0141, set it to NEW.")
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
else()
  message(STATUS "No POLICY CMP0141 detected.")
endif()

project("SoundLinuxAgent")

# Enforce out-of-source builds
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed. Please create a separate build directory.")
endif()

set(CMAKE_BUILD_TYPE Debug)

find_package(spdlog REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPULSE REQUIRED libpulse)
pkg_check_modules(LIBPULSE_MAINLOOP_GLIB REQUIRED libpulse-mainloop-glib)

# Set C++ standard at project level
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Add the pulseaudio library subdirectory
add_subdirectory(pulseaudio_lib)

# Add source to this project's executable.
add_executable(SoundLinuxAgent "SoundLinuxAgent.cpp")

# Include directories for the executable
target_include_directories(SoundLinuxAgent PRIVATE
    ${CMAKE_BINARY_DIR}
    ${LIBPULSE_INCLUDE_DIRS}
    ${LIBPULSE_MAINLOOP_GLIB_INCLUDE_DIRS}
    pulseaudio_lib # Include the library headers
)

# Link libraries for the executable
target_link_libraries(SoundLinuxAgent PRIVATE
    pulseaudio_lib # Link with our new library
    spdlog::spdlog
    ${LIBPULSE_LIBRARIES}
    ${LIBPULSE_MAINLOOP_GLIB_LIBRARIES}
)

# Set the package name and version
set(CPACK_PACKAGE_NAME "SoundLinuxAgent")
if(NOT DEFINED CPACK_PKG_VERSION)
  set(CPACK_PACKAGE_VERSION "1.0.0" CACHE STRING "Package version" FORCE)
else()
  set(CPACK_PACKAGE_VERSION ${CPACK_PKG_VERSION})
endif()

# Configure cpversion.h from cpversion.h.in, whereby @CPACK_PACKAGE_VERSION@ will be replaced. 
configure_file(${CMAKE_SOURCE_DIR}/cpversion.h.in ${CMAKE_BINARY_DIR}/cpversion.h @ONLY)

# Set the package contact information
set(CPACK_PACKAGE_CONTACT "Eduard Danziger <edanziger@gmx.de>")

# Specify the package type (e.g., DEB)
if(UNIX AND NOT APPLE)
  set(CPACK_GENERATOR "DEB")
endif()

# Specify the installation directory
set(CPACK_PACKAGE_INSTALL_DIRECTORY "SoundLinuxAgent")

# Specify the license file
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# Specify the readme file
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# Specify the description
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SoundLinuxAgent is a test application for audio processing.")

# Specify the dependencies (for DEB packages)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libasound2, libspdlog-dev")

# Specify the installation paths
install(TARGETS SoundLinuxAgent pulseaudio_lib DESTINATION bin)

# Include CPack to enable packaging
include(CPack)

add_custom_command(
    TARGET SoundLinuxAgent POST_BUILD
    COMMAND ${CMAKE_CPACK_COMMAND} -G DEB
    COMMENT "Generating DEB package"
)
