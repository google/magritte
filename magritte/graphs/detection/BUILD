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
    name = "face_detection_short_range_cpu",
    data = ["@mediapipe//mediapipe/modules/face_detection:face_detection_short_range.tflite"],
    graph = "face_detection_short_range_cpu.pbtxt",
    register_as = "FaceDetectionShortRangeSubgraphCpu",
    deps = [
        "@mediapipe//mediapipe/modules/face_detection:face_detection_short_range_cpu",
    ],
)

magritte_graph(
    name = "face_detection_short_range_gpu",
    data = ["@mediapipe//mediapipe/modules/face_detection:face_detection_short_range.tflite"],
    graph = "face_detection_short_range_gpu.pbtxt",
    register_as = "FaceDetectionShortRangeSubgraphGpu",
    deps = [
        "@mediapipe//mediapipe/modules/face_detection:face_detection_short_range_gpu",
    ],
)

magritte_graph(
    name = "face_detection_full_range_cpu",
    data = [
        "@mediapipe//mediapipe/modules/face_detection:face_detection_full_range.tflite",
        "@mediapipe//mediapipe/modules/face_detection:face_detection_full_range_sparse.tflite",
    ],
    graph = "face_detection_full_range_cpu.pbtxt",
    register_as = "FaceDetectionFullRangeSubgraphCpu",
    deps = [
        "@mediapipe//mediapipe/modules/face_detection:face_detection_full_range_cpu",  #sparse
    ],
)

magritte_graph(
    name = "face_detection_full_range_gpu",
    data = [
        "@mediapipe//mediapipe/modules/face_detection:face_detection_full_range.tflite",
        "@mediapipe//mediapipe/modules/face_detection:face_detection_full_range_sparse.tflite",
    ],
    graph = "face_detection_full_range_gpu.pbtxt",
    register_as = "FaceDetectionFullRangeSubgraphGpu",
    deps = [
        "@mediapipe//mediapipe/modules/face_detection:face_detection_full_range_gpu",  #sparse
    ],
)

magritte_graph(
    name = "face_detection_short_and_full_range_cpu",
    graph = "face_detection_short_and_full_range_cpu.pbtxt",
    register_as = "FaceDetectionShortAndFullRangeSubgraphCpu",
    deps = [
        ":face_detection_full_range_cpu",
        ":face_detection_short_range_cpu",
        "@mediapipe//mediapipe/calculators/util:non_max_suppression_calculator",
    ],
)

magritte_graph(
    name = "face_detection_short_and_full_range_gpu",
    graph = "face_detection_short_and_full_range_gpu.pbtxt",
    register_as = "FaceDetectionShortAndFullRangeSubgraphGpu",
    deps = [
        ":face_detection_full_range_gpu",
        ":face_detection_short_range_gpu",
        "@mediapipe//mediapipe/calculators/util:non_max_suppression_calculator",
    ],
)

magritte_graph(
    name = "face_detection_360_short_and_full_range_cpu",
    graph = "face_detection_360_short_and_full_range_cpu.pbtxt",
    register_as = "FaceDetection360ShortAndFullRangeSubgraphCpu",
    deps = [
        ":face_detection_short_and_full_range_cpu",
        "//magritte/calculators:detection_transformation_calculator",
        "@mediapipe//mediapipe/calculators/image:image_properties_calculator",
        "@mediapipe//mediapipe/calculators/image:image_transformation_calculator",
        "@mediapipe//mediapipe/calculators/util:non_max_suppression_calculator",
    ],
)

magritte_graph(
    name = "face_detection_360_short_and_full_range_gpu",
    graph = "face_detection_360_short_and_full_range_gpu.pbtxt",
    register_as = "FaceDetection360ShortAndFullRangeSubgraphGpu",
    deps = [
        ":face_detection_short_and_full_range_gpu",
        "//magritte/calculators:detection_transformation_calculator",
        "@mediapipe//mediapipe/calculators/image:image_properties_calculator",
        "@mediapipe//mediapipe/calculators/image:image_transformation_calculator",
        "@mediapipe//mediapipe/calculators/util:non_max_suppression_calculator",
    ],
)

magritte_graph(
    name = "face_detection_360_short_range_by_roi_cpu",
    data = [
        "@mediapipe//mediapipe/modules/face_detection:face_detection_short_range.tflite",
    ],
    graph = "face_detection_360_short_range_by_roi_cpu.pbtxt",
    register_as = "FaceDetection360ShortRangeByRoiSubgraphCpu",
    deps = [
        "//magritte/calculators:rotation_roi_calculator",
        "@mediapipe//mediapipe/calculators/util:non_max_suppression_calculator",
        "@mediapipe//mediapipe/modules/face_detection:face_detection_short_range_by_roi_cpu",
    ],
)

magritte_graph(
    name = "face_detection_360_short_range_by_roi_gpu",
    data = [
        "@mediapipe//mediapipe/modules/face_detection:face_detection_short_range.tflite",
    ],
    graph = "face_detection_360_short_range_by_roi_gpu.pbtxt",
    register_as = "FaceDetection360ShortRangeByRoiSubgraphGpu",
    deps = [
        "//magritte/calculators:rotation_roi_calculator",
        "@mediapipe//mediapipe/calculators/util:non_max_suppression_calculator",
        "@mediapipe//mediapipe/modules/face_detection:face_detection_short_range_by_roi_gpu",
    ],
)

magritte_graph(
    name = "face_detection_rotated_full_range_gpu",
    graph = "face_detection_rotated_full_range_gpu.pbtxt",
    register_as = "FaceDetectionRotatedFullRangeSubgraphGpu",
    deps = [
        ":face_detection_full_range_gpu",  #sparse
        "//magritte/calculators:detection_transformation_calculator",
        "@mediapipe//mediapipe/calculators/image:image_properties_calculator",
        "@mediapipe//mediapipe/calculators/image:image_transformation_calculator",
    ],
)
