#
# Copyright 2022 Google LLC
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
load("@bazel_skylib//rules:build_test.bzl", "build_test")
load(
    "//magritte:magritte_graph.bzl",
    "magritte_resources_folder",
    "magritte_runtime_data",
)

package(
    default_visibility = ["//visibility:private"],
)

licenses(["notice"])

cc_library(
    name = "image_io_util",
    srcs = ["image_io_util.cc"],
    hdrs = ["image_io_util.h"],
    visibility = ["//visibility:public"],
    deps = [
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:image_format_cc_proto",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@mediapipe//mediapipe/framework/port:opencv_imgcodecs",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
    ],
)

cc_binary(
    name = "magritte_deidentify_image",
    srcs = ["magritte_deidentify_image.cc"],
    deps = [
        ":image_io_util",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "//magritte/api:magritte_api",
        "//magritte/api:magritte_api_factory",
        "//magritte/graphs:face_pixelization_offline_cpu",
        "@mediapipe//mediapipe/framework:calculator_cc_proto",
        "@mediapipe//mediapipe/framework/port:status",
        "@mediapipe//mediapipe/framework/port:logging",
        "@com_google_absl//absl/flags:parse",
    ],
)

cc_binary(
    name = "magritte_deidentify_video",
    srcs = ["magritte_deidentify_video.cc"],
    deps = [
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "//magritte/api:magritte_api",
        "//magritte/api:magritte_api_factory",
        "//magritte/examples/codelab:image_io_util",
        "//magritte/graphs:face_pixelization_offline_cpu",
        "@mediapipe//mediapipe/framework:calculator_cc_proto",
        "@mediapipe//mediapipe/framework/port:status",
        "@mediapipe//mediapipe/framework/port:opencv_highgui",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
        "@mediapipe//mediapipe/framework/port:opencv_video",
        "@mediapipe//mediapipe/framework/port:logging",
        "@com_google_absl//absl/flags:parse",
    ],
)

magritte_runtime_data(
    name = "runtime_data",
    deps = ["//magritte/graphs:face_pixelization_offline_cpu"],
)

magritte_resources_folder(
    name = "resources_folder",
    runtime_data = ":runtime_data",
)

build_test(
    name = "examples_codelab_build_test",
    targets = [
        ":magritte_deidentify_image",
        ":magritte_deidentify_video",
        ":runtime_data",
        ":resources_folder",
    ],
)
