vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO bloomberg/rmqcpp
    REF 11859eb470f31008da522b59e96899585b4e94ce
    SHA512 f82cc1696d370e81dc410442465ecbe06940cd50ae8c93215e19a4b7de57ee7581a1d4f59d9775e08c646b63496ac18528b29edb852e0b9fb9cab7f761151b25
    HEAD_REF main
)

vcpkg_replace_string(
  "${SOURCE_PATH}/CMakeLists.txt"
  "add_subdirectory(src)\nadd_subdirectory(examples)"
  "add_subdirectory(src)"
)

vcpkg_replace_string(
  "${SOURCE_PATH}/src/CMakeLists.txt"
  "add_subdirectory(rmq)\nadd_subdirectory(rmqtestmocks)\nadd_subdirectory(tests)"
  "add_subdirectory(rmq)"
)

vcpkg_replace_string(
  "${SOURCE_PATH}/src/rmq/rmqio/rmqio_asioconnection.cpp"
  "    for (boost::asio::streambuf::const_buffers_type::const_iterator i =\n             bufs.begin();\n         i != bufs.end();\n         ++i) {\n        boost::asio::const_buffer buf(*i);\n        Decoder::ReturnCode rcode =\n            d_frameDecoder->appendBytes(&readFrames, buf.data(), buf.size());\n        if (rcode != Decoder::OK) {\n            BALL_LOG_WARN << \"Bad rcode from decoder: \" << rcode;\n            // Fail but we still want to process frames we were able to decode\n            success = false;\n            break;\n        };\n        bytes_decoded += buf.size();\n    }"
  "    boost::asio::const_buffer buf(bufs);\n    Decoder::ReturnCode rcode =\n        d_frameDecoder->appendBytes(&readFrames, buf.data(), buf.size());\n    if (rcode != Decoder::OK) {\n        BALL_LOG_WARN << \"Bad rcode from decoder: \" << rcode;\n        // Fail but we still want to process frames we were able to decode\n        success = false;\n    };\n    bytes_decoded += buf.size();"
)

vcpkg_replace_string(
  "${SOURCE_PATH}/src/rmq/rmqio/rmqio_asioeventloop.cpp"
  "void AsioEventLoop::postImpl(const Item& item) { d_context.post(item); }\nvoid AsioEventLoop::dispatchImpl(const Item& item) { d_context.dispatch(item); }"
  "void AsioEventLoop::postImpl(const Item& item) { boost::asio::post(d_context, item); }\nvoid AsioEventLoop::dispatchImpl(const Item& item) { boost::asio::dispatch(d_context, item); }"
)

vcpkg_replace_string(
  "${SOURCE_PATH}/src/rmq/rmqio/rmqio_asiotimer.h"
  "#include <boost/asio.hpp>"
  "#include <boost/asio.hpp>\n#include <boost/asio/deadline_timer.hpp>"
)

vcpkg_cmake_configure(
  SOURCE_PATH "${SOURCE_PATH}"
  OPTIONS
    -DBDE_BUILD_TARGET_CPP17=ON
    -DCMAKE_CXX_STANDARD=17
    -DCMAKE_CXX_STANDARD_REQUIRED=ON
    -DBDE_BUILD_TARGET_SAFE=ON
    -DCMAKE_INSTALL_LIBDIR=lib64
  MAYBE_UNUSED_VARIABLES
    BDE_BUILD_TARGET_CPP17
    BDE_BUILD_TARGET_SAFE
)

vcpkg_cmake_build()

vcpkg_cmake_install()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" COPYONLY)

vcpkg_cmake_config_fixup()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")