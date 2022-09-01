#
# Copyright 2021 Google LLC
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
    name = "mask_pixelization_cpu",
    graph = "mask_pixelization_cpu.pbtxt",
    register_as = "MaskPixelizationSubgraphCpu",
    deps = [
        "//magritte/calculators:blend_calculator",
        "//magritte/calculators:pixelization_calculator_cpu",
    ],
)

magritte_graph(
    name = "mask_pixelization_gpu",
    graph = "mask_pixelization_gpu.pbtxt",
    register_as = "MaskPixelizationSubgraphGpu",
    deps = [
        "//magritte/calculators:pixelization_calculator_gpu",
    ],
)
