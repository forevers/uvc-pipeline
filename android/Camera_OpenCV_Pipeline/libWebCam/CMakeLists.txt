# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(libuvc_VERSION_MAJOR 0)
set(libuvc_VERSION_MINOR 0)
set(libuvc_VERSION_PATCH 6)
set(libuvc_VERSION ${libuvc_VERSION_MAJOR}.${libuvc_VERSION_MINOR}.${libuvc_VERSION_PATCH})

# select either UVCCamera libuvc or direct libuvc with UVCCamera patch sequence
set(BUILD_WITH_UVCCAMERA_LIBUVC ON)

if(${BUILD_WITH_UVCCAMERA_LIBUVC})
  # use UVCCamera projects modified libusb and libuvc modules
  message(STATUS "Build with UVCCamera libuvc and libusb libraries")
  set(uvc_source_dir "src/main/cpp/UVCCamera_477aee8")
  set(usb_source_dir "src/main/cpp/UVCCamera_477aee8")
else()
  # libusb and libuvc submodule
  # https://github.com/ktossell/libuvc
  message(STATUS "Build with current libuvc and libusb libraries")
  set(uvc_source_dir "src/main/cpp/libuvc_v0_0_6_f3f6d8a")
  set(usb_source_dir "src/main/cpp/libusb_v1_0_22_0034b2a")
endif()

# native source base directory
set(camera_source_dir "src/main/cpp/camera_pipeline_modules")

# in libWebCam/src/main/cpp create symlink to OpenCV library
# ln -sfn <path to /OpenCV-android-sdk/sdk/native/libs> opencv_native_lib


# shared opencv library https://sourceforge.net/projects/opencvlibrary/files/opencv-android/
add_library(lib_opencv SHARED IMPORTED)
#set_target_properties(lib_opencv PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libopencv_java3.so)
set_target_properties(lib_opencv PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/src/main/opencv_native_lib/${ANDROID_ABI}/libopencv_java3.so)

include_directories($project.projectDir.toString()

                    # UVCCamera utility
                    "src/main/cpp/UVCCamera_477aee8"

                    # libusb
                    "${usb_source_dir}/libusb/android"
                    "${usb_source_dir}/libusb"
                    "${usb_source_dir}/libusb/libusb"

                    # libuvc
                    "${uvc_source_dir}/libuvc/include"
                    "${uvc_source_dir}/libuvc/include/libuvc"

                    # for utilbase.h
                    "src/main/cpp/UVCCamera_477aee8"

                    # camera modular libraries
                    ${camera_source_dir}
                    "${camera_source_dir}/webcam/src"
                    "${camera_source_dir}/webcam/include"
                    "${camera_source_dir}/renderer/src"
                    "${camera_source_dir}/renderer/include"
                    "${camera_source_dir}/webcam_include"
                    "${camera_source_dir}/webcam_util"
                    "${camera_source_dir}/opencv/src"
                    "${camera_source_dir}/opencv/include"

                    # opencv libraries
                    #"${CMAKE_CURRENT_SOURCE_DIR}/src/main/jniLibs/../jni/include"
                    "${CMAKE_CURRENT_SOURCE_DIR}/src/main/opencv_native_lib/../jni/include"

                    "src/main/cpp"
                    )

# create main jni shared library
add_library( native-pipeline-lib
             SHARED

             # source
             src/main/cpp/native-pipeline-lib.cpp)

# libusb static library
add_library( libusb-1.0
             STATIC

             # libusb
             ${usb_source_dir}/libusb/libusb/core.c
             ${usb_source_dir}/libusb/libusb/descriptor.c
             ${usb_source_dir}/libusb/libusb/hotplug.c
             ${usb_source_dir}/libusb/libusb/io.c
             ${usb_source_dir}/libusb/libusb/sync.c
             ${usb_source_dir}/libusb/libusb/strerror.c
             ${usb_source_dir}/libusb/libusb/os/poll_posix.c
             ${usb_source_dir}/libusb/libusb/os/threads_posix.c
             ${usb_source_dir}/libusb/libusb/os/android_usbfs.c
             ${usb_source_dir}/libusb/libusb/os/android_netlink.c)
             # if using current libusb
             #src/main/cpp/libusb/libusb/os/linux_usbfs.c
             #src/main/cpp/libusb/libusb/os/linux_netlink.c)

# camera_uvc_static static library
add_library( camera_uvc_static
             STATIC

             # source
             # libuvc
             ${uvc_source_dir}/libuvc/src/ctrl.c
             ${uvc_source_dir}/libuvc/src/device.c
             ${uvc_source_dir}/libuvc/src/diag.c
             ${uvc_source_dir}/libuvc/src/frame.c
             ${uvc_source_dir}/libuvc/src/init.c
             ${uvc_source_dir}/libuvc/src/stream.c

             # camera frame buffering modules
             ${camera_source_dir}/webcam_util/webcam_util.cpp
             ${camera_source_dir}/webcam/src/webcam.cpp
             ${camera_source_dir}/webcam/src/webcam_fifo.cpp
             ${camera_source_dir}/webcam/src/webcam_status_callback.cpp
             src/main/cpp/util.cpp)

# opencv_static static library
add_library( opencv_static
             STATIC

             # source
             ${camera_source_dir}/opencv/src/opencv.cpp)

# renderer_static static library
add_library( renderer_static
             STATIC

             # source
             ${camera_source_dir}/webcam_util/webcam_util.cpp
             ${camera_source_dir}/renderer/src/renderer.cpp
             src/main/cpp/util.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

find_library( # Sets the name of the path variable.
              z-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              z )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                        native-pipeline-lib

                        opencv_static

                        # opencv shared library
                        lib_opencv

                        -landroid
                        renderer_static
                        camera_uvc_static

                        libusb-1.0
                        ${log-lib} )
