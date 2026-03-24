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

find_package(Poco REQUIRED COMPONENTS Foundation Util Net)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(OpenSSL REQUIRED)

# rmqcpp is provided by the vcpkg manifest for this repo.
find_package(rmqcpp CONFIG REQUIRED)

if (NOT TARGET rmqcpp::rmq)
  message(FATAL_ERROR "rmqcpp was found, but the expected imported target rmqcpp::rmq was not exported")
endif()

find_package(PkgConfig REQUIRED)

add_subdirectory(SoundLib)

add_executable(LinuxSoundScanner
    "LinuxSoundScanner.cpp"
    "ServiceObserver.cpp"
    "ApiClient/AudioDeviceApiClient.cpp"
    "ApiClient/RabbitMqHttpRequestDispatcher.cpp"
    "ApiClient/RequestPublisher.cpp"
)

set_property(TARGET LinuxSoundScanner PROPERTY CXX_STANDARD 20)
target_compile_definitions(LinuxSoundScanner PRIVATE SPDLOG_HEADER_ONLY SPDLOG_FMT_EXTERNAL)

target_include_directories(LinuxSoundScanner PRIVATE
    .
    ${CMAKE_BINARY_DIR}
    ${Poco_INCLUDE_DIRS}
)

# Link libraries to LinuxSoundScanner
target_link_libraries(LinuxSoundScanner PRIVATE
    spdlog::spdlog_header_only
    fmt::fmt
    SoundLib
    Poco::Foundation
    Poco::Util
    Poco::Net
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    rmqcpp::rmq
)

install(TARGETS LinuxSoundScanner
    DESTINATION bin
    COMPONENT LinuxSoundScanner)

install(FILES LinuxSoundScanner.xml
  DESTINATION bin
  COMPONENT LinuxSoundScanner)

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

set(CPACK_COMPONENTS_ALL LinuxSoundScanner)

if(UNIX AND NOT APPLE)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEB_COMPONENT_INSTALL ON)  # This enables component-based DEB packaging
  set(CPACK_DEBIAN_LINUXSOUNDSCANNER_PACKAGE_NAME "LinuxSoundScanner")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libasound2, libssl3")
endif()

# Specify the installation directory
set(CPACK_PACKAGE_INSTALL_DIRECTORY "SoundLinuxAgent")

# Include CPack last
include(CPack)

add_custom_command(
  TARGET LinuxSoundScanner POST_BUILD
    COMMAND ${CMAKE_CPACK_COMMAND} -G DEB
    COMMENT "Generating DEB package"
)
