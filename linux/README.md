# uvc-pipeline
UVC capture, processing, and rendering pipelines for Android and Linux platforms

# platform configuration

## package installation
update package list and installed packages
```console
$ sudo apt-get update
$ sudo apt-get upgrade -y
```
cmake for building libuvc
```console
$ sudo apt-get install pkg-config
$ sudo apt-get install libusb-1.0-0-dev
$ sudo apt-get install cmake
```

tkinter for python
```console
$ sudo apt-get install python3-tk
$ sudo apt-get install python2-tk
```

imagemagick for image processing
```console
$ sudo apt-get install imagemagick
```

v4l-utils for camera interogation
```console
$ sudo apt-get install v4l-utils
```

yavta for v4l2 image capture
```console
$ git clone http://git.ideasonboard.org/git/yavta.git
$ cd yavta
$ make
```

gtk+mm for graphics library
```console
apt-get install libgtkmm-3.0-dev
```

other usefull packages
```console
$ sudo apt-get install -y gedit
$ sudo apt-get install -y aptitude
$ sudo apt-get install -y synaptic
```
## debugger configuration

- Eclipse SimRel 2018‑09
- create a new workspace
- right click new "Project..."" - > "C/C++ Project" -> "Makefile Project with Existing Code"
- select "Linux GCC"
- override default location with the linux "project" directory
- right click project to select Properties -> "C/C++ Build" and uncheck "Use default build command"
- right click project "index" -> rebuild

# camera interogation
each camera creates a media device node and video device node
```console
$ sudo media-ctl -d /dev/media<n> -p
```
list video devices
```console
$ sudo v4l2-ctl --list-devices
```
list supported formats of a camera n
```console
$ sudo v4l2-ctl -d /dev/video<n> --list-formats-ext
```

yavta frame capture from campera n
-s, WxH Set the frame size, 
--buffer-size Buffer size in bytes,
```console
$ ./yavta -s120x720 --buffer-size 345600 -c2 -n4 -F./image#.raw /dev/video<n>
```

# uvc-pipeline app build instructions

clone the repos and submodules
```console
$ git clone --recurse-submodules https://github.com/forevers/uvc-pipeline.git
$ cd uvc-pipeline/linux/libuvc
```
or

```console
$ git clone https://github.com/forevers/uvc-pipeline.git
$ cd uvc-pipeline/linux/libuvc
$ git submodule init
$ git submodule update
```

build libuvc static and shared libraries
```console
$ mkdir build
$ cd build
$ cmake ..
$ make
```

build project
```console
$ cd ../..
$ make
```

# TODO
- linux app : consistent scaling of controls when streaming and not streaming
- linux/android apps : video source api consistent for android/linux ... enumerate yuyv and rgb output modes
- linux app : enuerate video modes during start streaming
- linux : udev for usb
