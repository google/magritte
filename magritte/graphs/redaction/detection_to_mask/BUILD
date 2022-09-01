#
# Copyright 2021-2022 Google LLC
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
    name = "face_detection_to_render_data",
    graph = "face_detection_to_render_data.pbtxt",
    register_as = "FaceDetectionToRenderDataSubgraph",
    visibility = ["//visibility:private"],
    deps = [
        "//magritte/graphs/redaction:face_detection_to_normalized_rect",
        "@mediapipe//mediapipe/calculators/util:rect_to_render_data_calculator",
    ],
)

magritte_graph(
    name = "detection_to_render_data",
    graph = "detection_to_render_data.pbtxt",
    register_as = "DetectionToRenderDataSubgraph",
    visibility = ["//visibility:private"],
    deps = [
        "@mediapipe//mediapipe/calculators/util:detections_to_rects_calculator",
        "@mediapipe//mediapipe/calculators/util:rect_to_render_data_calculator",
        "@mediapipe//mediapipe/calculators/util:rect_transformation_calculator",
    ],
)

magritte_graph(
    name = "face_detection_to_mask_cpu",
    graph = "face_detection_to_mask_cpu.pbtxt",
    register_as = "FaceDetectionToMaskSubgraphCpu",
    deps = [
        ":face_detection_to_render_data",
        "//magritte/calculators:new_canvas_calculator",
        "@mediapipe//mediapipe/calculators/image:image_properties_calculator",
        "@mediapipe//mediapipe/calculators/util:annotation_overlay_calculator",
    ],
)

magritte_graph(
    name = "face_detection_to_mask_gpu",
    graph = "face_detection_to_mask_gpu.pbtxt",
    register_as = "FaceDetectionToMaskSubgraphGpu",
    deps = [
        ":face_detection_to_render_data",
        "//magritte/calculators:new_canvas_calculator",
        "@mediapipe//mediapipe/calculators/image:image_properties_calculator",
        "@mediapipe//mediapipe/calculators/util:annotation_overlay_calculator",
    ],
)
