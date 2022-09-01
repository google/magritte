//
// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef MAGRITTE_EXAMPLES_CODELAB_IMAGE_IO_UTIL_H_
#define MAGRITTE_EXAMPLES_CODELAB_IMAGE_IO_UTIL_H_

#include <memory>

#include "mediapipe/framework/formats/image_frame.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace magritte {

// Reads an image file and returns it as an ImageFrame.
//
// This function only supports 3-channel RGB images with 8 bits of depth.
//
// This function uses OpenCV to read image files. It therefore supports the same
// file formats as OpenCV. See
// https://docs.opencv.org/4.6.0/d4/da8/group__imgcodecs.html#ga288b8b3da0892bd651fce07b3bbd3a56.
absl::StatusOr<std::unique_ptr<mediapipe::ImageFrame>> LoadFromFile(
    const std::string& file_path);

// Saves a given ImageFrame into a file.
//
// This function only supports ImageFrames in mediapipe::ImageFormat::SRGB format.
//
// This function uses OpenCV to write image files. It therefore supports the
// same file formats as OpenCV. See
// https://docs.opencv.org/4.6.0/d4/da8/group__imgcodecs.html#ga288b8b3da0892bd651fce07b3bbd3a56.
absl::Status SaveToFile(const std::string& file_path,
                        const mediapipe::ImageFrame& image_frame);

}  // namespace magritte

#endif  // MAGRITTE_EXAMPLES_CODELAB_IMAGE_IO_UTIL_H_
