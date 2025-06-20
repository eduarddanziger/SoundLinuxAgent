cmake_minimum_required(VERSION 3.29)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  message(STATUS "Detected POLICY CMP0141, set it to NEW.")
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
else()
  message(STATUS "No POLICY CMP0141 detected.")
endif()

if (POLICY CMP0167)
  message(STATUS "Detected POLICY CMP0167, set it to OLD.")
  cmake_policy(SET CMP0167 OLD)
endif()

project("SoundLinuxAgent")

# Enforce out-of-source builds
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed. Please create a separate build directory.")
endif()

# set(CMAKE_BUILD_TYPE Debug)

find_package(spdlog 1.15.1 REQUIRED)
message(STATUS "Found spdlog version: ${spdlog_VERSION}")

find_package(fmt 10.2.1 REQUIRED)
message(STATUS "Found fmt version: ${fmt_VERSION}")

find_package(Poco REQUIRED Foundation Util Net)
find_package(cpprestsdk REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

add_subdirectory(SoundLib)

add_executable(SoundLinuxCli "SoundLinuxCli.cpp")
set_property(TARGET SoundLinuxCli PROPERTY CXX_STANDARD 20)
target_compile_definitions(SoundLinuxCli PRIVATE SPDLOG_HEADER_ONLY)
target_include_directories(SoundLinuxCli PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(SoundLinuxCli PRIVATE
    spdlog::spdlog_header_only
    fmt::fmt
    SoundLib
)

add_executable(SoundLinuxDaemon
    "SoundLinuxDaemon.cpp"
    "SodiumCrypt.cpp"
    "ServiceObserver.cpp"
    "SoundRepoApiClient/AudioDeviceApiClient.cpp"
    "SoundRepoApiClient/HttpRequestProcessor.cpp"
)
set_property(TARGET SoundLinuxDaemon PROPERTY CXX_STANDARD 20)
target_compile_definitions(SoundLinuxDaemon PRIVATE SPDLOG_HEADER_ONLY)

target_include_directories(SoundLinuxDaemon PRIVATE
    .
    ${CMAKE_BINARY_DIR}
    ${Poco_INCLUDE_DIRS}
    ${SODIUM_INCLUDE_DIRS}
    ${CPPREST_INCLUDE_DIRS}
)

# Link libraries to SoundLinuxDaemon
target_link_libraries(SoundLinuxDaemon PRIVATE
    spdlog::spdlog_header_only
    fmt::fmt
    SoundLib
    Poco::Foundation
    Poco::Util
    Poco::Net
    ${SODIUM_LIBRARIES}
    cpprestsdk::cpprest
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
)

install(TARGETS SoundLinuxCli
    DESTINATION bin
    COMPONENT SoundLinuxCli)

install(TARGETS SoundLinuxDaemon
    DESTINATION bin
    COMPONENT SoundLinuxDaemon)

install(FILES SoundLinuxDaemon.xml
  DESTINATION bin
  COMPONENT SoundLinuxDaemon)

# The package name and version
set(CPACK_PACKAGE_NAME "SoundLinuxAgent")
if(NOT DEFINED PKG_VERSION)
  set(CPACK_PACKAGE_VERSION "1.0.0" CACHE STRING "Package version" FORCE)
else()
  set(CPACK_PACKAGE_VERSION ${PKG_VERSION})
endif()

# Configure cpversion.h from cpversion.h.using substitution of CPACK_PACKAGE_VERSION
configure_file(${CMAKE_SOURCE_DIR}/cpversion.h.in ${CMAKE_BINARY_DIR}/cpversion.h @ONLY)
file(READ "${CMAKE_BINARY_DIR}/cpversion.h" CPVERSION_CONTENT)

set(CPACK_PACKAGE_CONTACT "Eduard Danziger <edanziger@gmx.de>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SoundLinuxAgent is an Linux Sound Device agent.")

set(CPACK_COMPONENTS_ALL SoundLinuxCli SoundLinuxDaemon)

if(UNIX AND NOT APPLE)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEB_COMPONENT_INSTALL ON)  # This enables component-based DEB packaging
  set(CPACK_DEBIAN_SOUNDLINUXCLI_PACKAGE_NAME "SoundLinuxCli")
  set(CPACK_DEBIAN_SOUNDLINUXDAEMON_PACKAGE_NAME "SoundLinuxDaemon")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libasound2, libsodium23, libcpprest, libssl3")
endif()

# Specify the installation directory
set(CPACK_PACKAGE_INSTALL_DIRECTORY "SoundLinuxAgent")

# Include CPack last
include(CPack)

add_custom_command(
    TARGET SoundLinuxCli SoundLinuxDaemon POST_BUILD
    COMMAND ${CMAKE_CPACK_COMMAND} -G DEB
    COMMENT "Generating DEB package"
)
