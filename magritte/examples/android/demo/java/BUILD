#
# Copyright 2020-2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


load("//magritte:magritte_graph.bzl", "magritte_runtime_data")

package_group(
    name = "magritte_visibility",
    packages = [
        "//magritte/examples/android/demo/java/...",
    ],
)

package(
    default_visibility = ["//magritte/examples/android/demo/java:magritte_visibility"],
)

licenses(["notice"])

android_library(
    name = "lib",
    exports = [
        "//magritte/examples/android/demo/java/com/google/privacy/magritte/demo/activities",
        "@mediapipe//mediapipe/java/com/google/mediapipe/glutil",
    ],
)

magritte_runtime_data(
    name = "graph_runtime_data",
    deps = [
        "//magritte/graphs:face_pixelization_live_gpu",
    ],
)

android_binary(
    name = "magritte_demo",
    assets = [
        "//magritte/graphs:face_pixelization_live_gpu_graph",
        ":graph_runtime_data",
    ],
    assets_dir = "",
    custom_package = "com.google.privacy.magritte.demo",
    manifest = "AndroidManifest.xml",
    manifest_values = {
        "applicationId": "com.google.privacy.magritte.demo",
        "appName": "Magritte Demo",
        "mainActivity": "com.google.privacy.magritte.demo.activities.AppMainActivity",
    },
    multidex = "native",
    deps = [":lib"],
)
