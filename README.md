# uvc-pipeline
UVC capture, processing, and rendering pipelines for Android and Linux platforms

# platform configuration

update package list and installed packages
```console
$ sudo apt-get update
$ sudo apt-get upgrade -y
```
cmake for building libuvc
```console
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

other usefull packages
```console
$ sudo apt-get install -y gedit
$ sudo apt-get install -y aptitude
$ sudo apt-get install -y synaptic
```

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
python script to deinterlace raw yavta created images and call imagemagic png creator
```console
$ python2 parse_raw_frames.py
```

# uvc-pipeline app build instructions

clone the repos and submodules
```console
$ git clone --recursive-submodules https://github.com/forevers/uvc-pipeline.git
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
