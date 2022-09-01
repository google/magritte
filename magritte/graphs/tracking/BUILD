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
load(
    "//magritte:magritte_graph.bzl",
    "magritte_graph",
)

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

magritte_graph(
    name = "sampled_tracking_cpu",
    graph = "sampled_tracking_cpu.pbtxt",
    register_as = "SampledTrackingSubgraphCpu",
    deps = [
        ":tracking_cpu",
        "@mediapipe//mediapipe/calculators/core:packet_resampler_calculator",
    ],
)

magritte_graph(
    name = "sampled_tracking_gpu",
    graph = "sampled_tracking_gpu.pbtxt",
    register_as = "SampledTrackingSubgraphGpu",
    deps = [
        ":tracking_gpu",
        "@mediapipe//mediapipe/calculators/core:packet_resampler_calculator",
    ],
)

magritte_graph(
    name = "tracking_cpu",
    graph = "tracking_cpu.pbtxt",
    register_as = "TrackingSubgraphCpu",
    deps = [
        "@mediapipe//mediapipe/graphs/tracking/subgraphs:object_tracking_cpu",
    ],
)

magritte_graph(
    name = "tracking_gpu",
    graph = "tracking_gpu.pbtxt",
    register_as = "TrackingSubgraphGpu",
    deps = [
        "@mediapipe//mediapipe/graphs/tracking/subgraphs:object_tracking_gpu",
    ],
)
