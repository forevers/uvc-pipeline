# Camera OpenCV Pipeline

This repository contains the **Camera_OpenCV_Pipeline** Android Studio project. It contains JNI and Java source to construct a multi-stage pipeline to acquire video frames delivered by a platform or external UVC compliant camera, perform processing on them and render the results to Android 2D and 3D Surface Views.

Below are a few videos for object tracking and face detection features.

Object Tracking (Hue Based):

[![Watch the video](https://img.youtube.com/vi/ogLN6q8gaY4/2.jpg)](https://www.youtube.com/watch?v=ogLN6q8gaY4&feature=youtu.be "object tracking")

[![Watch the video](https://img.youtube.com/vi/0gUOx5ktNkA/2.jpg)](https://www.youtube.com/watch?v=0gUOx5ktNkA&feature=youtu.be "ball tracking")

Face Detection :

[![Watch the video](https://img.youtube.com/vi/ygKc487v4tI/2.jpg)](https://www.youtube.com/watch?v=ygKc487v4tI&feature=youtu.be "face detection")

# Architecture

The Android Studio project is composed of 2 primary elements. **libWebCam** provides library access to various stages of the camera pipeline, while pipeline_opencv provides an example of how to utilize the components in application development. The libWebCam library modules are mainly composed of JNI class implementation with a light reflection through Java side classes.

## libWebCam Elements

### Camera

The **Camera** class provides a source pipeline stage for connecting to the camera through conventional UVC operations. It implements the **ICamera** control interface to provide pipeline construction and asynchronous control.

Camera frames are accumulated in a queue, available for pull requests from downstream pipeline clients.

### Opencv
The **Opencv** class provides a stage to implement OpenCV processing on camera frames. It implements the **IOpenCv** control interface to provide pipeline construction and asynchronous control. Touch and Swipe on the view surface controls the processing state.

### Renderer
The **Renderer** class provides a sink stage to render camera frames into Application provided SurfaceViews. It implements the **IRenderer** control interface to provide pipeline construction and asynchronous control.

## Camera Pipeline Application
**Camera Pipeline** is an Android application which constructs and controls a pipeline composed of a Camera source, an Opencv processing stage, and a Renderer applied to the output of the Opencv stage.

## Pipeline Layout

- Below is a high-level layout of the pipeline and abstraction stack :
![Pipeline Image](https://github.com/forevers/uvc-pipeline/blob/master/android/images/pipeline.png "Pipeline Image"). 

- Below is a sequence description of the pipeline contruction and destruction and the native level :
![Pipeline Construct Destruct Image](https://github.com/forevers/uvc-pipeline/blob/master/android/images/pipeline_construct_destruct.png "Pipeline Construction and Destruction Image"). 

## Build Environment

- Ubuntu 16.04 64 bit.
- Android Studio 3.4.1
-    File -> Settings -> Build, Execution, Deployment -> Compiler set Command-line Options: to --debug --stacktrace
- Gradle version 5.1.1
- OpenCV 4.1.0 Android libraries
	- opencv-4.1.0-android-sdk.zip or https://github.com/opencv/opencv/tree/4.1.0

### Build Instructions
- Obtain Android Studio project from this site. 
- Download latest OpenCV SDK for Android from OpenCV.org and decompress the zip file.
    - https://opencv.org/releases/
- in libWebCam/src/main/cpp create symlink to OpenCV library
	- ln -sfn <path to /OpenCV-android-sdk/sdk/native> opencv\_4\_1\_0\_native


## Supported Devices

- Pixel 3 XL

## License

Copyright (c) 2018-19 Embedifying Software Services  [embedifying@gmail.com ](mailto:embedifying@gmail.com )

Licensed to the Apache Software Foundation (ASF) under one or more contributor license agreements.  See the NOTICE file distributed with this work for additional information regarding copyright ownership.  The ASF licenses this file to you under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.  You may obtain a copy of the License at

```
 http://www.apache.org/licenses/LICENSE-2.0
```
Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the License for the specific language governing permissions and limitations under the License.

All files in the folder are under this Apache License, Version 2.0. Files in the 
- /libWebCam/src/main/cpp/UVCCamera_477aee8
- /libWebCam/src/main/cpp/rapidjson
- /libWebCam/src/main/cpp/boost
- /libWebCam/src/main/java/com/serenegiant
folders may have a different license, see the respective files.

Modified 3rd party files are :

/libWebCam/src/main/cpp/UVCCamera_477aee8/libuvc/include/libuvc/libuv.h
/libWebCam/src/main/cpp/UVCCamera_477aee8/libuvc/include/libuvc/libuvc_internal.h
/libWebCam/src/main/cpp/UVCCamera_477aee8/libuvc/src/stream.c
/libWebCam/src/main/cpp/UVCCamera_477aee8/localdefines.h

See /libWebCam modification comments for changes.
