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
#include "magritte/examples/codelab/image_io_util.h"

#include <memory>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_format.pb.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include  <opencv2/imgcodecs.hpp>
#include  <opencv2/imgproc.hpp>
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace magritte {

absl::StatusOr<std::unique_ptr<mediapipe::ImageFrame>> LoadFromFile(
    const std::string& file_path) {
  cv::Mat mat = cv::imread(file_path, cv::IMREAD_UNCHANGED);
  if (mat.empty()) {
    return absl::InternalError("Could not read image");
  }
  if (mat.type() != CV_8UC3) {
    return absl::UnimplementedError("Expected image in CV_8UC3 format");
  }
  std::unique_ptr<mediapipe::ImageFrame> image_frame =
      std::make_unique<mediapipe::ImageFrame>(
          mediapipe::ImageFormat::SRGB, mat.size().width, mat.size().height);
  // OpenCV returns color images in BGR format, so we need to convert.
  cv::cvtColor(mat, mediapipe::formats::MatView(image_frame.get()),
               cv::COLOR_BGR2RGB);
  return image_frame;
}

absl::Status SaveToFile(const std::string& file_path,
                        const mediapipe::ImageFrame& image_frame) {
  if (image_frame.Format() != mediapipe::ImageFormat::SRGB) {
    return absl::UnimplementedError("Expected ImageFrame in SRGB format");
  }
  // OpenCV can only write color images in BGR format, so we need to convert.
  cv::Mat output_mat;
  cv::cvtColor(mediapipe::formats::MatView(&image_frame), output_mat,
               cv::COLOR_RGB2BGR);
  if (!cv::imwrite(file_path, output_mat)) {
    return absl::InternalError("Could not write image");
  }
  return absl::OkStatus();
}

}  // namespace magritte
