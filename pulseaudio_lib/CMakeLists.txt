# pulseaudio_lib/CMakeLists.txt
add_library(pulseaudio_lib
    impl/PulseDeviceCollection.cpp
    impl/SoundAgent.cpp
    impl/PulseDevice.cpp
)

target_include_directories(pulseaudio_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${LIBPULSE_INCLUDE_DIRS}
    ${LIBPULSE_MAINLOOP_GLIB_INCLUDE_DIRS}
)

target_link_libraries(pulseaudio_lib PUBLIC
    spdlog::spdlog
    ${LIBPULSE_LIBRARIES}
    ${LIBPULSE_MAINLOOP_GLIB_LIBRARIES}
)

# Set proper C++ standard for the library
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET pulseaudio_lib PROPERTY CXX_STANDARD 20)
endif()
