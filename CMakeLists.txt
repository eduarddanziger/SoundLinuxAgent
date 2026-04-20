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

# Debug: Show environment variables
message(STATUS "HOME=$ENV{HOME}")

message(STATUS "VCPKG_ROOT=$ENV{VCPKG_ROOT} (initial)")
if(NOT DEFINED ENV{VCPKG_ROOT} OR "$ENV{VCPKG_ROOT}" STREQUAL "")
  set(ENV{VCPKG_ROOT} "$ENV{HOME}/vcpkg")
  message(STATUS "VCPKG_ROOT=$ENV{VCPKG_ROOT} (changed)")
endif()
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")


project("LinuxSoundScanner")

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
    "AudioDeviceApiClient.cpp"
    "RabbitMqHttpRequestDispatcher.cpp"
    "RequestPublisher.cpp"
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

# rmqcpp::rmq's INTERFACE_LINK_LIBRARIES contains bare -lbdl and -lbal
# (without a -L search path). Add the vcpkg lib dirs so those resolve:
if(VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
  target_link_directories(LinuxSoundScanner PRIVATE
          "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
          "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<$<CONFIG:Debug>:debug/>lib"
  )
endif()

# Resolve cyclic static-link references between bdl/bal and pcre2.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(_vcpkg_lib_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib")
else()
  set(_vcpkg_lib_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
endif()

# Wrap the cyclic static archives in an RESCAN group. LINK_GROUP emits them
# as libraries (after objects, in library position) rather than as link options.
target_link_libraries(LinuxSoundScanner PRIVATE
        "$<LINK_GROUP:RESCAN,${_vcpkg_lib_dir}/libbdl.a,${_vcpkg_lib_dir}/libbal.a,${_vcpkg_lib_dir}/libpcre2-8.a>"
)

install(TARGETS LinuxSoundScanner
    DESTINATION bin)

install(FILES LinuxSoundScanner.xml
  DESTINATION bin)

# The application version
if(NOT DEFINED PKG_VERSION)
  set(APP_VERSION "1.0.0")
else()
  set(APP_VERSION ${PKG_VERSION})
endif()

# Configure cpversion.h from cpversion.h.in using APP_VERSION.
configure_file(${CMAKE_SOURCE_DIR}/cpversion.h.in ${CMAKE_BINARY_DIR}/cpversion.h @ONLY)
