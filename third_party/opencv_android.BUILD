# Description:
#   OpenCV libraries for video/image processing on Android

licenses(["notice"])  # BSD license

OPENCV_LIBRARY_NAME = "libopencv_java3.so"

OPENCV_CORE = "libopencv_core.a"

OPENCV_IMGPROC = "libopencv_imgproc.a"

OPENCV_IMGCODECS = "libopencv_imgcodecs.a"

LIBTBB = "libtbb.a"

LIBTEGRA_HAL = "libtegra_hal.a"

LIBCPUFEATURES = "libcpufeatures.a"

LIBILMIMF = "libIlmImf.a"

LIBITTNOTIFY = "libittnotify.a"

LIBIPPIW = "libippiw.a"

LIBIPPICV = "libippicv.a"

OPENCVANDROIDSDK_NATIVELIBS_PATH = "sdk/native/staticlibs/"

OPENCVANDROIDSDK_THIRD_PARTY_LIB_PATH = "sdk/native/3rdparty/libs/"

OPENCVANDROIDSDK_JNI_PATH = "sdk/native/jni/"

[cc_library(
    name = "libopencv_" + arch,
    srcs = [
        OPENCVANDROIDSDK_NATIVELIBS_PATH + arch + "/" + OPENCV_IMGPROC,
        OPENCVANDROIDSDK_NATIVELIBS_PATH + arch + "/" + OPENCV_IMGCODECS,
        OPENCVANDROIDSDK_NATIVELIBS_PATH + arch + "/" + OPENCV_CORE,
        OPENCVANDROIDSDK_THIRD_PARTY_LIB_PATH + arch + "/" + LIBCPUFEATURES,
        OPENCVANDROIDSDK_THIRD_PARTY_LIB_PATH + arch + "/" + LIBILMIMF,
        OPENCVANDROIDSDK_THIRD_PARTY_LIB_PATH + arch + "/" + LIBTBB,
        OPENCVANDROIDSDK_THIRD_PARTY_LIB_PATH + arch + "/" + LIBITTNOTIFY,
    ] + ([
        OPENCVANDROIDSDK_THIRD_PARTY_LIB_PATH + arch + "/" + LIBIPPIW,
        OPENCVANDROIDSDK_THIRD_PARTY_LIB_PATH + arch + "/" + LIBIPPICV,
    ] if (arch == "x86" or arch == "x86_64") else [OPENCVANDROIDSDK_THIRD_PARTY_LIB_PATH + arch + "/" + LIBTEGRA_HAL]),
    hdrs = glob([
        OPENCVANDROIDSDK_JNI_PATH + "include/opencv2/imgproc/**/*.h",
        OPENCVANDROIDSDK_JNI_PATH + "include/opencv2/imgproc/**/*.hpp",
        OPENCVANDROIDSDK_JNI_PATH + "include/opencv2/core/**/*.h",
        OPENCVANDROIDSDK_JNI_PATH + "include/opencv2/core/**/*.hpp",
        OPENCVANDROIDSDK_JNI_PATH + "include/opencv2/core.hpp",
        OPENCVANDROIDSDK_JNI_PATH + "include/opencv2/imgproc.hpp",
        OPENCVANDROIDSDK_JNI_PATH + "include/opencv2/cvconfig.h",
        OPENCVANDROIDSDK_JNI_PATH + "include/opencv2/opencv_modules.hpp",
    ]),
    includes = [
        OPENCVANDROIDSDK_JNI_PATH + "include",
    ],
    linkopts = [
        "-ldl",
        "-lm",
        "-lz",
        "-landroid",
        "-Wl,--no-undefined",
    ],
    linkstatic = 1,
    visibility = ["//visibility:public"],
) for arch in [
    "arm64-v8a",
    "armeabi-v7a",
    "x86",
    "x86_64",
]]