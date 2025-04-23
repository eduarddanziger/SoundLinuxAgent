find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPULSE REQUIRED libpulse)
pkg_check_modules(LIBPULSE_MAINLOOP_GLIB REQUIRED libpulse-mainloop-glib)

add_library(SoundLib
    impl/SoundAgent.cpp
    impl/PulseDeviceCollection.cpp
    impl/PulseDevice.cpp
)

# Make interface headers accessible to library users
target_include_directories(SoundLib 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}  # For public interface headers
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/impl  # For implementation headers
        ${LIBPULSE_INCLUDE_DIRS}
        ${LIBPULSE_MAINLOOP_GLIB_INCLUDE_DIRS}
)

target_link_libraries(SoundLib 
    PRIVATE
        ${LIBPULSE_LIBRARIES}  # Implementation detail
        ${LIBPULSE_MAINLOOP_GLIB_LIBRARIES}  # Implementation detail
)

# Set C++ standard
set_property(TARGET SoundLib PROPERTY CXX_STANDARD 20)