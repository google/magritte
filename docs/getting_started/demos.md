---
layout: default
title: Demos
parent: Getting started
nav_order: 1
---

# Magritte demos
{: .no_toc }

1. TOC
{:toc}
---

This page describes how to
install some prerequisites, build and run
the demo applications from the Magritte library. For more details on how to use
the actual library in your code, start with the
[codelab](https://google.github.io/magritte/getting_started/codelab.html).

## Installation

Follow these steps to build the Magritte library from source. This guide is for
Ubuntu, but building on other linux distributions should work similarly.

We also provide a
[`Dockerfile`](https://github.com/google/magritte/blob/main/Dockerfile)
with all dependencies installed.

### Install Bazel 5.2.0
Follow the
[Bazel documentation](https://docs.bazel.build/versions/master/install-ubuntu.html#install-on-ubuntu)
to install Bazel from its apt repository. Make sure you use version 5.2.0.

Another option is to
[install Bazelisk](https://docs.bazel.build/versions/main/install-bazelisk.html),
which manages Bazel versions for you.

### Install Python3, NumPy and Pip

Those are required for working with TensorFlow. Make sure Python is version 3.7
or later.

```shell
sudo apt-get install python3 python3-numpy python3-pip python-is-python3
```

### Install OpenCV and FFmpeg.

Magritte builds on top of [MediaPipe](https://github.com/google/mediapipe) which
requires OpenCV and FFmpeg.

```shell
sudo apt-get install libopencv-core-dev libopencv-highgui-dev \
                     libopencv-calib3d-dev libopencv-features2d-dev \
                     libopencv-imgproc-dev libopencv-video-dev
```

If you are using Debian 11 or later, or Ubuntu 20.04 or later, then this will
install OpenCV 4. In that case, you might also need to install `libopencv-contrib-dev`
(see [this discussion](https://github.com/google/mediapipe/issues/970#issuecomment-1044388088)).

### Install mesa for EGL GPU support

GPU-based processing graphs require EGL support, which is provided by mesa. If you don't need GPU support, you can skip this step.

```shell
sudo apt-get install mesa-common-dev libegl1-mesa-dev libgles2-mesa-dev
```

### Clone Magritte repository

```shell
git clone https://github.com/google/magritte.git
cd magritte
```

If you are using OpenCV 4, you must also execute the following command:
```shell
sed -i '/include\/opencv4/s/#//g' third_party/opencv_linux.BUILD
```

After that you're ready to build the Magritte library.

## Demos

Magritte comes with several demo applications for Desktop and Android.

### Build and run desktop example {#build-desktop}

The desktop example, in its simplest form, reads a video file, processes it with
a Magritte graph, and saves the result in another video file.

Before you can run it, you need to build the resources folder (see the
[developer's guide](https://google.github.io/magritte/technical_guide/dev_guide.html#resource-folders)
for more information), which you do by running the following command.


```shell
bazel build //magritte/examples/desktop:desktop_resources_folder \
  --experimental_repo_remote_exec
```

After that, you can run the desktop demo with the following command:

```shell
bazel run //magritte/examples/desktop \
  --cxxopt='-std=c++17' --experimental_repo_remote_exec \
  -- \
  --resource_root_dir=$(bazel info bazel-bin --experimental_repo_remote_exec)/magritte/examples/desktop/desktop_resource_folder \
  --graph_type=FacePixelizationOfflineCpu \
  --input_video=<input_video_file> --output_video=<output_video_file>
```
Here, the flag `graph_type` tells the demo which Magritte top-level graph it
should use to process the video. You can try out different ones from the
[graphs folder](https://github.com/google/magritte/blob/master/magritte/graphs/) or
[build your own](https://google.github.io/magritte/technical_guide/dev_guide.html#building-your-own-magritte-graphs).

The flags `--input_video` and `--output_video` are absolute paths to the
input/output video files. If the `--input_video` flag is not provided, the demo
will try to read input from a camera. If the `--output_video` flag is not
provided, the result will be displayed in a separate window. Using
`--output_video` without `--input_video` is not supported.

Maybe you need to build the example without GPU support. In that case, add an
option `--define MEDIAPIPE_DISABLE_GPU=1` to
your command before the `--`.

If you want to build optimized code, we also recommend adding `-c opt` to your
command before the `--`.

If you have an OpenCV version that has not been compiled with X11 support for
the EGL backend, you will also need the following flags before the `--`:
```
--copt -DBOOST_ERROR_CODE_HEADER_ONLY \
--copt -DMESA_EGL_NO_X11_HEADERS \
--copt -DEGL_NO_X11
```

If you use the face sticker redaction graphs, you can choose the sticker image
and a default zoom for the sticker, using the CLI parameters
`--sticker_image=<sticker_image_file>` and `--sticker_zoom=<zoom_value>`. Those
parameters are optional, and default to a smile emoji and `1.0` if not provided.
The image file must be in the resources folder that you provide
with the `--resource_root_dir` flag, and the path given via the
`--sticker_image` flag should be relative to that folder.

### Build and run Android demos

Magritte contains several Android apps that allow you to test out Magritte
graphs in real-time.

The following dependencies are necessary:
* Java Runtime
* Android SDK (release 30.0.0 or above)
* Android NDK 21

We recommend installing Android SDK and NDK through
[Android Studio](https://developer.android.com/studio).

Another option is installing the
[Android Command Line Tools](https://developer.android.com/studio#command-tools),
and installing the NDK through `sdkmanager`:

```shell
sdkmanager --install "ndk;21.0.6113669"
```

This may also be useful if you have a higher version of the NDK installed and
you have trouble downgrading through Android Studio.

Once the installation is complete, set $ANDROID_HOME and $ANDROID_NDK_HOME to
point to the installed SDK and NDK.

```shell
export ANDROID_HOME=<path to the Android SDK>
export ANDROID_NDK_HOME=<path to the Android NDK>
```

One third option is to run MediaPipe's
[setup_android_sdk_and_ndk.sh](https://github.com/google/mediapipe/blob/master/setup_android_sdk_and_ndk.sh)
script to automate installing Android SDK and NDK.

Connect your Android device, make sure to
[enable USB debugging](https://developer.android.com/studio/debug/dev-options#enable)
and confirm that it shows up under `adb devices`:

```shell
$ adb devices
List of devices attached
XXXXXXXXXXXXX   device

```

After that, you can use the `mobile-install` build command to build the
application and install it onto your phone over adb. Alternatively, if you only
want to build the APK file without installing it on the phone, use the `build`
command. For example:

```shell
bazel mobile-install --cxxopt='-std=c++17' --experimental_repo_remote_exec \
  --crosstool_top=//external:android/crosstool \
  --host_crosstool_top=@bazel_tools//tools/cpp:toolchain \
  --fat_apk_cpu=arm64-v8a --cpu=arm64-v8a \
  --define EXCLUDE_OPENCV_SO_LIB=1 \
  //magritte/examples/android/demo/java:magritte_demo
```

If your device architecture is not ARM64, then replace `arm64-v8a` with
`armeabi-v7a`. If you want to run on an emulator, use `x86` instead. See the
[Bazel documentation](https://bazel.build/docs/android-ndk#configuring-target-abi)
for more information.
