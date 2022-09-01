//
// Copyright 2020-2022 Google LLC
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
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "magritte/calculators/pixelization_calculator.pb.h"
#include  <opencv2/core.hpp>
#include  <opencv2/imgproc.hpp>

namespace magritte {

constexpr char kFramesTag[] = "FRAMES";

using ::mediapipe::CalculatorBase;
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::ImageFrame;
using ::mediapipe::formats::MatView;
// A calculator that applies pixelization to the whole input image. The total
// number of pixels that should have the same color after pixelization is given
// as a parameter. The ignore_mask parameter is ignored.
//
// Inputs:
// - FRAMES: An ImageFrame stream, containing the input images.
//
// Outputs:
// - FRAMES: An ImageFrame stream, containing the pixelized images.
//
// Example config:
// node {
//   calculator: "PixelizationCalculatorCpu"
//   input_stream: "FRAMES:input_video"
//   output_stream: "FRAMES:output_video"
//   options: {
//     [magritte.PixelizationCalculatorOptions.ext] {
//       total_nb_pixels: 576
//       blend_method: PIXELIZATION
//     }
//   }
// }
class PixelizationCalculatorCpu : public CalculatorBase {
 public:
  PixelizationCalculatorCpu() = default;
  ~PixelizationCalculatorCpu() override = default;

  static absl::Status GetContract(CalculatorContract* cc) {
    const auto& options = cc->Options<PixelizationCalculatorOptions>();
    cc->Inputs().Tag(kFramesTag).Set<ImageFrame>();
    cc->Outputs().Tag(kFramesTag).Set<ImageFrame>();
    // No input side packets.
    // Check if Median filter options are set correctly
    RET_CHECK(!options.median_filter_enabled() ||
        options.median_filter_ksize() % 2 == 1 &&
        options.median_filter_ksize() > 0);
    return absl::OkStatus();
  }

  absl::Status Open(CalculatorContext* cc) override {
    return absl::OkStatus();
  }

  static std::pair<int, int> getScaledDownSize(
      int width, int height,
      const magritte::PixelizationCalculatorOptions& options) {
    int x, y;
    if (options.has_max_resolution()) {
      const int max_side = options.max_resolution();
      if (width > height) {
        x = max_side;
        y = x * height / width;
      } else {
        y = max_side;
        x = y * width / height;
      }
    } else {
      // Computes x and y to keep the subdivisions square
      // with x*y = total_pixels.
      const float total_pixels = options.total_nb_pixels();
      x = static_cast<int>(
          std::round(sqrt(total_pixels * (float)width / (float)height)));
      y = static_cast<int>(
          std::round(sqrt(total_pixels * (float)height / (float)width)));
    }
    return std::make_pair(x, y);
  }

  absl::Status Process(CalculatorContext* cc) override {
    const auto& options = cc->Options<PixelizationCalculatorOptions>();

    if (cc->Inputs().Tag(kFramesTag).Value().IsEmpty()) {
      LOG(WARNING) << "No image frame at " << cc->InputTimestamp();
      return absl::OkStatus();
    }

    const auto& frame = cc->Inputs().Tag(kFramesTag).Get<ImageFrame>();

    // We need to copy the original frame because other calculators might want
    // to access it still.
    std::unique_ptr<ImageFrame> output_frame(
        new ImageFrame(frame.Format(), frame.Width(), frame.Height()));
    output_frame->CopyFrom(frame, ImageFrame::kDefaultAlignmentBoundary);

    const int width = frame.Width();
    const int height = frame.Height();
    // Subdivide the screen into x by y regions.
    std::pair<int, int> scaled_down_size =
        getScaledDownSize(width, height, options);
    int x = scaled_down_size.first;
    int y = scaled_down_size.second;
    // Apply resizing to pixelize the image.
    cv::Mat src = MatView(output_frame.get());
    cv::Mat dest(cv::Size(x, y), src.type());
    cv::resize(src, dest, cv::Size(x, y), 1, 1, cv::INTER_NEAREST);
    // Apply Median Filter
    if (options.median_filter_enabled()) {
      cv::medianBlur(dest, dest, options.median_filter_ksize());
    }
    switch (options.blend_method()) {
      case PixelizationCalculatorOptions::DEFAULT:
      case PixelizationCalculatorOptions::PIXELIZATION:
        cv::resize(dest, src, cv::Size(width, height), 1, 1, cv::INTER_NEAREST);
        break;
      case PixelizationCalculatorOptions::LINEAR_INTERPOLATION:
        cv::resize(dest, src, cv::Size(width, height), 1, 1, cv::INTER_LINEAR);
        break;
      case PixelizationCalculatorOptions::CUBIC_INTERPOLATION:
        cv::resize(dest, src, cv::Size(width, height), 1, 1, cv::INTER_CUBIC);
        break;
    }

    cc->Outputs().Tag(kFramesTag).Add(output_frame.release(),
                                    cc->InputTimestamp());
    return absl::OkStatus();
  }
};

REGISTER_CALCULATOR(PixelizationCalculatorCpu);
}  // namespace magritte
