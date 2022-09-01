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

# Fail on any error.
set -e
# Display commands being run.
set -x

sudo apt -y install cloud-guest-utils
sudo growpart /dev/sda 1
sudo resize2fs /dev/sda1

# ROOT_DIR is the initial Kokoro working directory and contains a sub directory
# with the git project.
ROOT_DIR=`pwd`

# SCRIPT_DIR is the directory that contains this executing script.
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd )"

# Run the actual script using the docker image that contains the toolchains.
docker run --rm -i \
  --volume "${ROOT_DIR}:${ROOT_DIR}" \
  --volume "${KOKORO_ARTIFACTS_DIR}:/mnt/artifacts" \
  --workdir "${ROOT_DIR}" \
  --env BUILD_SYSTEM=$BUILD_SYSTEM \
  --env BUILD_TARGET_ARCH=$BUILD_TARGET_ARCH \
  --entrypoint "${ROOT_DIR}/git/avdeid/test.sh" \
  "gcr.io/avdeid/avdeid-build:latest"