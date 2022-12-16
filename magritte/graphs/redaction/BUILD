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
    name = "face_pixelization_cpu",
    graph = "face_pixelization_cpu.pbtxt",
    register_as = "FacePixelizationSubgraphCpu",
    deps = [
        "//magritte/graphs/redaction/detection_to_mask:face_detection_to_mask_cpu",
        "//magritte/graphs/redaction/mask_redaction:mask_pixelization_cpu",
    ],
)

magritte_graph(
    name = "face_pixelization_gpu",
    graph = "face_pixelization_gpu.pbtxt",
    register_as = "FacePixelizationSubgraphGpu",
    deps = [
        ":face_detection_to_normalized_rect",
        "//magritte/calculators:pixelization_by_roi_calculator_gpu",
        "@mediapipe//mediapipe/calculators/image:image_properties_calculator",
    ],
)

magritte_graph(
    name = "face_detection_overlay_cpu",
    graph = "face_detection_overlay_cpu.pbtxt",
    register_as = "FaceDetectionOverlaySubgraphCpu",
    deps = [
        ":face_detection_to_normalized_rect",
        "@mediapipe//mediapipe/calculators/image:image_properties_calculator",
        "@mediapipe//mediapipe/calculators/util:annotation_overlay_calculator",
        "@mediapipe//mediapipe/calculators/util:detections_to_render_data_calculator",
        "@mediapipe//mediapipe/calculators/util:rect_to_render_data_calculator",
    ],
)

magritte_graph(
    name = "face_detection_overlay_gpu",
    graph = "face_detection_overlay_gpu.pbtxt",
    register_as = "FaceDetectionOverlaySubgraphGpu",
    deps = [
        ":face_detection_to_normalized_rect",
        "@mediapipe//mediapipe/calculators/image:image_properties_calculator",
        "@mediapipe//mediapipe/calculators/util:annotation_overlay_calculator",
        "@mediapipe//mediapipe/calculators/util:detections_to_render_data_calculator",
        "@mediapipe//mediapipe/calculators/util:rect_to_render_data_calculator",
    ],
)

magritte_graph(
    name = "detection_tracking_overlay_cpu",
    graph = "detection_tracking_overlay_cpu.pbtxt",
    register_as = "DetectionTrackingOverlaySubgraphCpu",
    deps = [
        "@mediapipe//mediapipe/calculators/util:annotation_overlay_calculator",
        "@mediapipe//mediapipe/calculators/util:detections_to_render_data_calculator",
    ],
)

magritte_graph(
    name = "detection_tracking_overlay_gpu",
    graph = "detection_tracking_overlay_gpu.pbtxt",
    register_as = "DetectionTrackingOverlaySubgraphGpu",
    deps = [
        "@mediapipe//mediapipe/calculators/util:annotation_overlay_calculator",
        "@mediapipe//mediapipe/calculators/util:detections_to_render_data_calculator",
    ],
)

magritte_graph(
    name = "face_sticker_redaction_cpu",
    data = ["//magritte/test_data:emoji.png"],
    graph = "face_sticker_redaction_cpu.pbtxt",
    register_as = "FaceStickerRedactionSubgraphCpu",
    deps = [
        ":face_detection_to_normalized_rect",
        "//magritte/calculators:rois_to_sprite_list_calculator",
        "//magritte/calculators:sprite_calculator_cpu",
        "@mediapipe//mediapipe/calculators/core:constant_side_packet_calculator",
        "@mediapipe//mediapipe/calculators/core:default_side_packet_calculator",
        "@mediapipe//mediapipe/calculators/core:side_packet_to_stream_calculator",
        "@mediapipe//mediapipe/calculators/core:stream_to_side_packet_calculator",
        "@mediapipe//mediapipe/calculators/image:image_properties_calculator",
        "@mediapipe//mediapipe/calculators/image:opencv_encoded_image_to_image_frame_calculator",
        "@mediapipe//mediapipe/calculators/util:local_file_contents_calculator",
    ],
)

magritte_graph(
    name = "face_sticker_redaction_gpu",
    data = ["//magritte/test_data:emoji.png"],
    graph = "face_sticker_redaction_gpu.pbtxt",
    register_as = "FaceStickerRedactionSubgraphGpu",
    deps = [
        ":face_detection_to_normalized_rect",
        "//magritte/calculators:rois_to_sprite_list_calculator",
        "//magritte/calculators:sprite_calculator_gpu",
        "@mediapipe//mediapipe/calculators/core:constant_side_packet_calculator",
        "@mediapipe//mediapipe/calculators/core:default_side_packet_calculator",
        "@mediapipe//mediapipe/calculators/core:side_packet_to_stream_calculator",
        "@mediapipe//mediapipe/calculators/core:stream_to_side_packet_calculator",
        "@mediapipe//mediapipe/calculators/image:image_properties_calculator",
        "@mediapipe//mediapipe/calculators/image:opencv_encoded_image_to_image_frame_calculator",
        "@mediapipe//mediapipe/calculators/util:local_file_contents_calculator",
        "@mediapipe//mediapipe/gpu:image_frame_to_gpu_buffer_calculator",
    ],
)

magritte_graph(
    name = "face_detection_to_normalized_rect",
    graph = "face_detection_to_normalized_rect.pbtxt",
    register_as = "FaceDetectionToNormalizedRectSubgraph",
    visibility = [":__subpackages__"],
    deps = [
        "@mediapipe//mediapipe/calculators/util:detections_to_rects_calculator",
        "@mediapipe//mediapipe/calculators/util:rect_transformation_calculator",
    ],
)
