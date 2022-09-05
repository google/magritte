#!/bin/bash

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

. /bin/using.sh

# Fail on any error.
set -e
# Display commands being run.
set -x

using bazel-5.2.0
using cmake-3.17.2
using clang-10.0.0
using ndk-r21e

# The following paths are relative to this script.
cd "$(dirname "$(readlink -f "$0")")"

# Try to build the Android targets.
./build_android.sh
