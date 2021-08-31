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