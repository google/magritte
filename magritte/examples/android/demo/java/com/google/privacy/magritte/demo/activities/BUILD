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
#



package(
    default_visibility = ["//magritte/examples/android/demo/java:magritte_visibility"],
)

licenses(["notice"])

cc_binary(
    name = "libmediapipe_jni.so",
    linkshared = 1,
    linkstatic = 1,
    deps = [
        "//magritte/graphs:face_pixelization_live_gpu",
        "@mediapipe//mediapipe/java/com/google/mediapipe/framework/jni:mediapipe_framework_jni",
    ],
)

cc_library(
    name = "mediapipe_jni_lib",
    srcs = [":libmediapipe_jni.so"],
    alwayslink = 1,
)

android_library(
    name = "activities",
    srcs = glob(["*.java"]),
    custom_package = "com.google.privacy.magritte.demo.activities",
    exports_manifest = 1,  # ensures library specific manifests are merged in the binary.
    manifest = "AndroidManifest.xml",
    resource_files = glob(["res/**"]),
    deps = [
        ":mediapipe_jni_lib",
        "@maven//:androidx_constraintlayout_constraintlayout",
        "@maven//:androidx_appcompat_appcompat",
        "@mediapipe//mediapipe/java/com/google/mediapipe/components:android_camerax_helper",
        "@mediapipe//mediapipe/java/com/google/mediapipe/components:android_components",
        "@mediapipe//mediapipe/java/com/google/mediapipe/framework:android_framework",
        "@mediapipe//mediapipe/java/com/google/mediapipe/glutil",
    ],
)
