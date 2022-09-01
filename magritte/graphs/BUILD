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
load(
    "//magritte:magritte_graph.bzl",
    "magritte_graph",
)

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

magritte_graph(
    name = "face_pixelization_offline_cpu",
    graph = "face_pixelization_offline_cpu.pbtxt",
    register_as = "FacePixelizationOfflineCpu",
    deps = [
        "//magritte/graphs/detection:face_detection_short_and_full_range_cpu",
        "//magritte/graphs/redaction:face_pixelization_cpu",
    ],
)

magritte_graph(
    name = "face_overlay_offline_cpu",
    graph = "face_overlay_offline_cpu.pbtxt",
    register_as = "FaceOverlayOfflineCpu",
    deps = [
        "//magritte/graphs/detection:face_detection_short_and_full_range_cpu",
        "//magritte/graphs/redaction:face_detection_overlay_cpu",
    ],
)

magritte_graph(
    name = "face_blur_with_tracking_offline_cpu",
    graph = "face_blur_with_tracking_offline_cpu.pbtxt",
    register_as = "FaceBlurWithTrackingOfflineCpu",
    deps = [
        "//magritte/calculators:simple_blur_calculator_cpu",
        "//magritte/graphs/detection:face_detection_360_short_and_full_range_cpu",
        "//magritte/graphs/tracking:tracking_cpu",
    ],
)

magritte_graph(
    name = "face_blur_with_tracking_live_cpu",
    graph = "face_blur_with_tracking_live_cpu.pbtxt",
    register_as = "FaceBlurWithTrackingLiveCpu",
    deps = [
        "//magritte/calculators:simple_blur_calculator_cpu",
        "//magritte/graphs/detection:face_detection_360_short_and_full_range_cpu",
        "//magritte/graphs/tracking:sampled_tracking_cpu",
        "@mediapipe//mediapipe/calculators/core:flow_limiter_calculator",
    ],
)

magritte_graph(
    name = "face_pixelization_live_gpu",
    graph = "face_pixelization_live_gpu.pbtxt",
    register_as = "FacePixelizationLiveGpu",
    deps = [
        "//magritte/graphs/detection:face_detection_full_range_gpu",
        "//magritte/graphs/redaction:face_pixelization_gpu",
        "@mediapipe//mediapipe/calculators/core:flow_limiter_calculator",
    ],
)

magritte_graph(
    name = "face_overlay_live_gpu",
    graph = "face_overlay_live_gpu.pbtxt",
    register_as = "FaceOverlayLiveGpu",
    deps = [
        "//magritte/graphs/detection:face_detection_full_range_gpu",
        "//magritte/graphs/redaction:face_detection_overlay_gpu",
        "@mediapipe//mediapipe/calculators/core:flow_limiter_calculator",
    ],
)

magritte_graph(
    name = "face_sticker_redaction_offline_cpu",
    graph = "face_sticker_redaction_offline_cpu.pbtxt",
    register_as = "FaceStickerRedactionOfflineCpu",
    deps = [
        "//magritte/graphs/detection:face_detection_360_short_range_by_roi_cpu",
        "//magritte/graphs/redaction:face_sticker_redaction_cpu",
    ],
)

magritte_graph(
    name = "face_sticker_redaction_live_gpu",
    graph = "face_sticker_redaction_live_gpu.pbtxt",
    register_as = "FaceStickerRedactionLiveGpu",
    deps = [
        "//magritte/graphs/detection:face_detection_full_range_gpu",
        "//magritte/graphs/redaction:face_sticker_redaction_gpu",
        "@mediapipe//mediapipe/calculators/core:flow_limiter_calculator",
    ],
)
