#
# Copyright 2020-2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
load("@mediapipe//mediapipe/framework/port:build_config.bzl", "mediapipe_proto_library")
load("@bazel_skylib//rules:build_test.bzl", "build_test")

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

mediapipe_proto_library(
    name = "new_canvas_calculator_proto",
    srcs = ["new_canvas_calculator.proto"],
    def_options_lib = False,
    deps = [
        "@mediapipe//mediapipe/framework:calculator_options_proto",
        "@mediapipe//mediapipe/framework:calculator_proto",
        "@mediapipe//mediapipe/util:color_proto",
    ],
)

cc_library(
    name = "new_canvas_calculator",
    srcs = ["new_canvas_calculator.cc"],
    hdrs = ["new_canvas_calculator.h"],
    deps = [
        ":new_canvas_calculator_cc_proto",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@mediapipe//mediapipe/util:color_cc_proto",
        "@mediapipe//mediapipe/framework/port:opencv_core",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
        "@com_google_absl//absl/memory",
    ] + select({
        "@mediapipe//mediapipe/gpu:disable_gpu": [],
        "//conditions:default": [
            "@mediapipe//mediapipe/gpu:gl_calculator_helper",
            "@mediapipe//mediapipe/gpu:gl_simple_shaders",
            "@mediapipe//mediapipe/gpu:shader_util",
        ],
    }),
    alwayslink = 1,
)

mediapipe_proto_library(
    name = "simple_blur_calculator_proto",
    srcs = ["simple_blur_calculator.proto"],
    def_options_lib = False,
    deps = [
        "@mediapipe//mediapipe/framework:calculator_options_proto",
        "@mediapipe//mediapipe/framework:calculator_proto",
    ],
)

cc_library(
    name = "simple_blur_calculator_cpu",
    srcs = ["simple_blur_calculator_cpu.cc"],
    deps = [
        ":simple_blur_calculator_cc_proto",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:detection_cc_proto",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@mediapipe//mediapipe/framework/formats:location_data_cc_proto",
        "@mediapipe//mediapipe/framework/formats/annotation:locus_cc_proto",
        "@mediapipe//mediapipe/framework/formats:location",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
    ],
    alwayslink = 1,
)

cc_library(
    name = "pixelization_calculator_cpu",
    srcs = ["pixelization_calculator_cpu.cc"],
    deps = [
        ":pixelization_calculator_cc_proto",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@mediapipe//mediapipe/framework/port:opencv_core",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
    ],
    alwayslink = 1,
)

mediapipe_proto_library(
    name = "pixelization_calculator_proto",
    srcs = ["pixelization_calculator.proto"],
    def_options_lib = False,
    deps = [
        "@mediapipe//mediapipe/framework:calculator_options_proto",
        "@mediapipe//mediapipe/framework:calculator_proto",
    ],
)

cc_library(
    name = "pixelization_calculator_gpu",
    srcs = ["pixelization_calculator_gpu.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":pixelization_calculator_cc_proto",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/gpu:gl_calculator_helper",
        "@mediapipe//mediapipe/gpu:gl_simple_shaders",
        "@mediapipe//mediapipe/gpu:shader_util",
    ],
    alwayslink = 1,
)

cc_library(
    name = "pixelization_by_roi_calculator_gpu",
    srcs = ["pixelization_by_roi_calculator_gpu.cc"],
    deps = [
        ":pixelization_calculator_cc_proto",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:rect_cc_proto",
        "@mediapipe//mediapipe/gpu:gl_calculator_helper",
        "@mediapipe//mediapipe/gpu:gl_simple_shaders",
        "@mediapipe//mediapipe/gpu:shader_util",
    ],
    alwayslink = 1,
)

cc_library(
    name = "pixelization_by_roi_calculator_experimental_gpu",
    srcs = ["pixelization_by_roi_calculator_experimental_gpu.cc"],
    deps = [
        ":pixelization_calculator_cc_proto",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:rect_cc_proto",
        "@mediapipe//mediapipe/gpu:gl_calculator_helper",
        "@mediapipe//mediapipe/gpu:gl_simple_shaders",
        "@mediapipe//mediapipe/gpu:shader_util",
    ],
    alwayslink = 1,
)

cc_library(
    name = "blend_calculator",
    srcs = ["blend_calculator.cc"],
    hdrs = ["blend_calculator.h"],
    visibility = ["//visibility:public"],
    deps = [
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@com_google_absl//absl/status",
        "@mediapipe//mediapipe/framework/port:opencv_core",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
    ],
    alwayslink = 1,
)

cc_test(
    name = "blend_calculator_test",
    srcs = ["blend_calculator_test.cc"],
    deps = [
        ":blend_calculator",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@com_google_googletest//:gtest_main",
        "@mediapipe//mediapipe/framework/port:opencv_core",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework:calculator_runner",
        "@mediapipe//mediapipe/framework:packet",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/port:gtest_main",
        "@mediapipe//mediapipe/framework/port:parse_text_proto",
        "@mediapipe//mediapipe/framework/tool:test_util",
    ],
)

mediapipe_proto_library(
    name = "rotation_calculator_options_proto",
    srcs = ["rotation_calculator_options.proto"],
    def_options_lib = False,
    deps = [
        "@mediapipe//mediapipe/framework:calculator_options_proto",
        "@mediapipe//mediapipe/framework:calculator_proto",
    ],
)

cc_library(
    name = "detection_transformation_calculator",
    srcs = ["detection_transformation_calculator.cc"],
    hdrs = ["detection_transformation_calculator.h"],
    deps = [
        ":rotation_calculator_options_cc_proto",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:detection_cc_proto",
        "@mediapipe//mediapipe/framework/formats:location_data_cc_proto",
    ],
    alwayslink = 1,
)

cc_test(
    name = "new_canvas_calculator_test",
    srcs = ["new_canvas_calculator_test.cc"],
    deps = [
        ":new_canvas_calculator",
        ":new_canvas_calculator_cc_proto",
        "@mediapipe//mediapipe/framework/port:gtest_main",
        "@mediapipe//mediapipe/framework/port:parse_text_proto",
    ],
)

cc_library(
    name = "detection_list_to_detections_calculator",
    srcs = ["detection_list_to_detections_calculator.cc"],
    deps = [
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:detection_cc_proto",
    ],
    alwayslink = 1,
)

cc_library(
    name = "rotation_roi_calculator",
    srcs = ["rotation_roi_calculator.cc"],
    hdrs = ["rotation_roi_calculator.h"],
    deps = [
        ":rotation_calculator_options_cc_proto",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@com_google_absl//absl/memory",
        "@mediapipe//mediapipe/framework/formats:rect_cc_proto",
    ],
    alwayslink = 1,
)

cc_test(
    name = "rotation_roi_calculator_test",
    srcs = ["rotation_roi_calculator_test.cc"],
    deps = [
        ":rotation_calculator_options_cc_proto",
        ":rotation_roi_calculator",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework:calculator_runner",
        "@mediapipe//mediapipe/framework/formats:rect_cc_proto",
        "@mediapipe//mediapipe/framework/port:gtest_main",
        "@mediapipe//mediapipe/framework/port:parse_text_proto",
    ],
)

mediapipe_proto_library(
    name = "sprite_pose_proto",
    srcs = ["sprite_pose.proto"],
    def_options_lib = False,
)

cc_library(
    name = "sprite_list",
    hdrs = ["sprite_list.h"],
    deps = [
        ":sprite_pose_cc_proto",
        "@mediapipe//mediapipe/framework:packet",
    ],
)

cc_library(
    name = "sprite_calculator_cpu",
    srcs = ["sprite_calculator_cpu.cc"],
    deps = [
        ":sprite_list",
        ":sprite_pose_cc_proto",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
    ],
    alwayslink = 1,
)

cc_test(
    name = "sprite_calculator_cpu_test",
    srcs = ["sprite_calculator_cpu_test.cc"],
    data = [
        "//magritte/test_data:sprite_background.png",
        "//magritte/test_data:sprite_composited.png",
        "//magritte/test_data:sprite_premultiplied.png",
    ],
    deps = [
        ":sprite_calculator_cpu",
        ":sprite_list",
        ":sprite_pose_cc_proto",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@mediapipe//mediapipe/framework/port:opencv_imgcodecs",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
        "@com_google_absl//absl/memory",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework:calculator_runner",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/port:gtest_main",
        "@mediapipe//mediapipe/framework/port:parse_text_proto",
        "@mediapipe//mediapipe/framework/tool:test_util",
    ],
)

cc_library(
    name = "sprite_calculator_gpu",
    srcs = ["sprite_calculator_gpu.cc"],
    deps = [
        ":sprite_list",
        ":sprite_pose_cc_proto",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/gpu:gl_calculator_helper",
        "@mediapipe//mediapipe/gpu:gl_simple_shaders",
        "@mediapipe//mediapipe/gpu:shader_util",
    ],
    alwayslink = 1,
)

mediapipe_proto_library(
    name = "rois_to_sprite_list_calculator_proto",
    srcs = ["rois_to_sprite_list_calculator.proto"],
    def_options_lib = False,
    deps = [
        "@mediapipe//mediapipe/framework:calculator_options_proto",
        "@mediapipe//mediapipe/framework:calculator_proto",
    ],
)

cc_library(
    name = "rois_to_sprite_list_calculator",
    srcs = ["rois_to_sprite_list_calculator.cc"],
    hdrs = ["rois_to_sprite_list_calculator.h"],
    deps = [
        ":rois_to_sprite_list_calculator_cc_proto",
        ":sprite_list",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@com_google_absl//absl/memory",
        "@mediapipe//mediapipe/framework/formats:rect_cc_proto",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
    ] + select({
        "@mediapipe//mediapipe/gpu:disable_gpu": [],
        "//conditions:default": [
            "@mediapipe//mediapipe/gpu:gl_calculator_helper",
            "@mediapipe//mediapipe/gpu:gl_simple_shaders",
            "@mediapipe//mediapipe/gpu:shader_util",
        ],
    }),
    alwayslink = 1,
)

cc_test(
    name = "rois_to_sprite_list_calculator_test",
    srcs = ["rois_to_sprite_list_calculator_test.cc"],
    data = [
        "//magritte/test_data:sprite_premultiplied.png",
        "//magritte/test_data:sprite_transparent.png",
    ],
    tags = ["cpu_only"],
    deps = [
        ":rois_to_sprite_list_calculator",
        ":rois_to_sprite_list_calculator_cc_proto",
        ":sprite_list",
        ":sprite_pose_cc_proto",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@mediapipe//mediapipe/framework/port:opencv_imgcodecs",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
        "@com_google_absl//absl/memory",
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework:calculator_runner",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/formats:rect_cc_proto",
        "@mediapipe//mediapipe/framework/port:gtest_main",
        "@mediapipe//mediapipe/framework/port:parse_text_proto",
        "@mediapipe//mediapipe/framework/tool:test_util",
    ],
)

build_test(
    name = "calculators_build_test",
    targets = [
        ":new_canvas_calculator_proto",
        ":new_canvas_calculator",
        ":blend_calculator",
        ":simple_blur_calculator_proto",
        ":simple_blur_calculator_cpu",
        ":pixelization_calculator_cpu",
        ":pixelization_calculator_proto",
        ":pixelization_calculator_gpu",
        ":pixelization_by_roi_calculator_gpu",
        ":rotation_calculator_options_proto",
        ":detection_transformation_calculator",
        ":detection_list_to_detections_calculator",
        ":rotation_roi_calculator",
        ":sprite_pose_proto",
        ":sprite_list",
        ":sprite_calculator_cpu",
        ":sprite_calculator_gpu",
        ":rois_to_sprite_list_calculator",
    ],
)

build_test(
    name = "calculators_with_dual_cpu_gpu_build_test",
    tags = ["cpu_only"],
    targets = [
        ":new_canvas_calculator",
        ":rois_to_sprite_list_calculator",
    ],
)

test_suite(
    name = "calculator_tests_cpu_only",
    tags = ["cpu_only"],
)

test_suite(
    name = "calculator_tests_cpu_or_gpu",
    tags = ["-cpu_only"],
)
