# futures-pipeline
UVC/MIPI camera capture, processing, and rendering pipelines for Linux platforms

Common camera selection/configuration/acquisition code. QT5 and GTKmm rendering layers.

Below are a few videos for object tracking and face detection features.

GTKmm and Qt5 rpi4 64bit OS video rendering:

[![Watch the video](https://img.youtube.com/vi/3hmjHRkfg6w/2.jpg)](https://www.youtube.com/watch?v=3hmjHRkfg6w&feature=youtu.be "rpi gtkmm/qt5 video streaming")


# BOOST Library
- Download boost tar.gz
https://dl.bintray.com/boostorg/release/1.74.0/source/boost_1_74_0.tar.gz

- Install Boost
    ```console
    $ cd <boost download dir>
    $ /bootstrap.sh
    $ ./b2 --prefix=<absolute path to boost install dir>
    ```

- Build the bcp utility
    ```console
    ./b2 tools/bcp
    ```

- Locate b2c exec
    ```console
    <boost download dir>/boost_1_74_0/bin.v2/tools/bcp/gcc-7/release/link-static
    ```

- Install circular buffer template library with dependencies into project
    ```console
    user@machine:~/projects/boost_1_74_0$ <bcp dir>/bcp --boost=<absolute path to boost install dir>/include circular_buffer <project base dir>/uvc-pipeline/futures_pipeline/utils/boost_1_74_0
    ```

# Rendering

## cmake
    ```console
    $ clang --version
    $ mkdir build && cd build
    $ cmake -D CMAKE_C_COMPILER=clang-6.0 -D CMAKE_CXX_COMPILER=clang++-6.0 ..
    ```

## gtkmm Rendering

## Qt Rendering
Qt Creator 4.5.2 on Ubuntu Qt Creator 4.5.2
    ```console
    user@machine: sudo apt install build-essential
    user@machine: sudo apt install qtcreator
    user@machine: sudo apt install qt5-default
    user@machine: sudo apt install qt5-doc
    user@machine: sudo apt install qt5-doc-html qtbase5-doc-html
    user@machine: sudo apt install qt5-doc-html qtbase5-doc-html
    ```
Qt Creator 4.14.1 on rpi4 64 bit Bullseye OS
    ```console
    user@machine: sudo apt-get install qtbase5-dev qtchooser
    user@machine: sudo apt-get install qt5-qmake qtbase5-dev-tools
    user@machine: sudo apt-get install qtcreator
    user@machine: sudo apt-get install qtdeclarative5-dev
    ```

### Qt Project File
uvc-pipeline/futures_pipeline/renderer/qt/CameraPipeline/CameraPipeline.pro

### Qt Building
- \<ctl-B\> ... etc

### Running
- Insert USB Webcam
- Run from Qt Creator IDE
- CL run
    - /futures_pipeline/renderer/qt/build-CameraPipeline-Desktop-Debug/CameraPipeline

### Qt threading
https://wiki.qt.io/Threads_Events_QObjects
https://www.kdab.com/wp-content/uploads/stories/multithreading-with-qt-1.pdf

### TODO
- test pattern generation interface and native impl
- expose interface to client rather than raw fsrame struct sshared ptr

# nfs mount

## client
sudo mount -t nfs -o proto=tcp,port=2049 <server ip>:/ <mount directory>

## server
configure exports

# vsc remote
install Remote Development vsc extension
enable ssh on rpi target
Ctrl+Shift+P to enter rpi@host, enter password at prompt

# mipi camera
- https://www.raspberrypi.com/documentation/accessories/camera.html

## os version

```console
$ cat /etc/os-release
```

keeping platform configuration up to date
```console
$ sudo apt update
$ sudo apt full-upgrade
```
update package list and installed packages
```console
$ sudo apt-get update
$ sudo apt-get upgrade -y
```

## current OS bullseye

### libcamera
moving to libcamera for all bullseye releases and later
https://libcamera.org/

https://www.youtube.com/watch?v=0kr_yS9OhLM
https://www.raspberrypi.com/news/raspberry-pi-os-debian-bullseye/
https://www.raspberrypi.com/documentation/accessories/camera.html#if-you-do-need-to-alter-the-configuration
https://datasheets.raspberrypi.com/camera/raspberry-pi-camera-guide.pdf
https://www.raspberrypi.com/news/bullseye-camera-system/

#### rpi libcamera-hello
list camera
```console
$ libcamera-hello --list-cameras
```

#### v4l2
https://www.kernel.org/doc/html/v4.9/media/uapi/v4l/v4l2.html
Providing two output nodes: Bayer image data on the /dev/video0 node and sensor embedded data on the /dev/video1 node.
Providing one input node on the /dev/video13 for the bayer input framebuffer.
Providing three output nodes: Two image outputs on the /dev/video14 and /dev/video15 nodes, and statistics output on the /dev/video16 node.

##### v4l2-utils
list devices
```console
$ v4l2-ctl --list-devices
```
list supported camera controls
```console
$ v4l2-ctl -d /dev/video0 --list-ctrls
```
list supported camera formats
```console
$ v4l2-ctl -d /dev/video0 --list-formats
$ v4l2-ctl -d /dev/video0 --list-formats-ext
```

##### media controller
https://forums.raspberrypi.com/viewtopic.php?f=43&t=322076


### package installation


## legacy (buster and earlier)

### package installation

update package list and installed packages
```console
$ sudo apt-get update
$ sudo apt-get upgrade -y
```

v4l-utils for camera interrogation
```console
$ sudo apt-get install v4l-utils
```

clang
```console
$ sudo apt-get install clang llvm
```

gtk+mm for graphics library
```console
apt-get install libgtkmm-3.0-dev
```