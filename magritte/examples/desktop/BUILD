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
    name = "demo_run_graph_main",
    srcs = ["demo_run_graph_main.cc"],
    deps = [
        "@mediapipe//mediapipe/framework:calculator_framework",
        "@mediapipe//mediapipe/framework/formats:image_frame",
        "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
        "@mediapipe//mediapipe/framework/port:opencv_highgui",
        "@mediapipe//mediapipe/framework/port:opencv_imgproc",
        "@mediapipe//mediapipe/framework/port:opencv_video",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/status",
        "@mediapipe//mediapipe/framework:subgraph",
        "@mediapipe//mediapipe/framework/port:status",
    ],
)

# List of all graphs that can be used in the demo.
_top_level_graph_targets = [
    "//magritte/graphs:face_blur_with_tracking_live_cpu",
    "//magritte/graphs:face_blur_with_tracking_offline_cpu",
    "//magritte/graphs:face_overlay_offline_cpu",
    "//magritte/graphs:face_tracking_overlay_offline_cpu",
    "//magritte/graphs:face_pixelization_offline_cpu",
    "//magritte/graphs:face_sticker_redaction_offline_cpu",
] + select({
    "@mediapipe//mediapipe/gpu:disable_gpu": [],
    "//conditions:default": [
        "//magritte/graphs:face_overlay_live_gpu",
        "//magritte/graphs:face_pixelization_live_gpu",
        "//magritte/graphs:face_sticker_redaction_live_gpu",
    ],
})

# Build rule for example desktop app.
#
# Run face obfuscation:
#
#   bazel run -c opt --define MEDIAPIPE_DISABLE_GPU=1 examples/desktop:desktop \
#   --experimental_repo_remote_exec --verbose_failures -c opt \
#   --copt -DBOOST_ERROR_CODE_HEADER_ONLY --copt -DMESA_EGL_NO_X11_HEADERS \
#   --copt -DEGL_NO_X11 --cxxopt='-std=c++17'
#   -- \
#   --graph_type=FacePixelizationOfflineCpu \
#   --input_video=<input_video_file> --output_video=<output_video_file>
#
# For the sticker redaction graphs, you can also pass a sticker image and a default sticker zoom via
# the parameters --sticker_image and --sticker_zoom. If not provided, a default smile emoji and a
# zoom factor of 1.0 are used.
cc_binary(
    name = "desktop",
    deps = [
        ":demo_run_graph_main",
    ] + _top_level_graph_targets,
)

magritte_runtime_data(
    name = "desktop_runtime_data",
    deps = _top_level_graph_targets,
)

magritte_resources_folder(
    name = "desktop_resources_folder",
    runtime_data = ":desktop_runtime_data",
)

build_test(
    name = "examples_desktop_build_test",
    targets = [
        ":desktop",
        ":desktop_runtime_data",
        ":desktop_resources_folder",
    ],
)
