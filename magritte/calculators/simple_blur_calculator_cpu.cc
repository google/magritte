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
#include <vector>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/annotation/locus.pb.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/framework/formats/location_data.pb.h"
#include "magritte/calculators/simple_blur_calculator.pb.h"
#include "mediapipe/framework/formats/location.h"
#include  <opencv2/imgproc.hpp>

namespace magritte {
using ::mediapipe::BoundingBox;
using ::mediapipe::CalculatorBase;
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::Detection;
using ::mediapipe::ImageFrame;
using ::mediapipe::Location;
using ::mediapipe::formats::MatView;
namespace {

constexpr char kDetectionsTag[] = "DETECTIONS";
constexpr char kFramesTag[] = "FRAMES";

typedef std::vector<Detection> Detections;
}  // namespace

// A calculator that applies box blurring or Gaussian blurring on an image. The
// type of blurring is configured via the calculator options.
//
// Inputs:
// - FRAMES: An ImageFrame stream, containing an input image.
// - DETECTIONS: A vector of detections, containing the detections to be blurred
//   onto the image from the first stream.  The type is vector<Detection>.
//
// Outputs:
// - FRAMES: An ImageFrame stream, containing the blurred images.
//
// Example config:
// node {
//   calculator: "SimpleBlurCalculatorCpu"
//   input_stream: "FRAMES:input_video"
//   input_stream: "DETECTIONS:tracked_detections"
//   output_stream: "FRAMES:output_video"
//   options: {
//     [magritte.SimpleBlurCalculatorOptions.ext] {
//       blur_type: GAUSSIAN_BLUR
//     }
//   }
// }
class SimpleBlurCalculatorCpu : public CalculatorBase {
 public:
  SimpleBlurCalculatorCpu() = default;
  ~SimpleBlurCalculatorCpu() override = default;

  static absl::Status GetContract(CalculatorContract* cc) {
    cc->Inputs().Tag(kFramesTag).Set<ImageFrame>();
    cc->Inputs().Tag(kDetectionsTag).Set<Detections>();
    cc->Outputs().Tag(kFramesTag).Set<ImageFrame>();

    // No input side packets.
    return absl::OkStatus();
  }

  absl::Status Open(CalculatorContext* cc) override { return absl::OkStatus(); }

  absl::Status Process(CalculatorContext* cc) override {
    const auto& options = cc->Options<SimpleBlurCalculatorOptions>();

    if (cc->Inputs().Tag(kFramesTag).Value().IsEmpty()) {
      LOG(WARNING) << "No image frame at " << cc->InputTimestamp();
      return absl::OkStatus();
    }
    if (cc->Inputs().Tag(kDetectionsTag).Value().IsEmpty()) {
      LOG(INFO) << "Empty detections at " << cc->InputTimestamp();
      cc->Outputs()
          .Tag(kFramesTag)
          .AddPacket(cc->Inputs().Tag(kFramesTag).Value());
      return absl::OkStatus();
    }

    const auto& frame = cc->Inputs().Tag(kFramesTag).Get<ImageFrame>();
    Detections detections = cc->Inputs().Tag(kDetectionsTag).Get<Detections>();

    std::unique_ptr<ImageFrame> output_frame(
        new ImageFrame(frame.Format(), frame.Width(), frame.Height()));

    cv::Mat src = MatView(&frame);

    const int width = frame.Width();
    const int height = frame.Height();
    for (const Detection& detection : detections) {
      auto box = Location(detection.location_data())
                     .ConvertToBBox<BoundingBox>(width, height);
      const int xmin = std::clamp(box.left_x(), 0, width - 1);
      const int xmax = std::clamp(box.right_x(), 0, width - 1);
      const int ymin = std::clamp(box.upper_y(), 0, height - 1);
      const int ymax = std::clamp(box.lower_y(), 0, height - 1);
      if (xmin >= xmax || ymin >= ymax) {
        LOG(WARNING) << "Invalid detection."
                     << " xmin = " << xmin << " xmax = " << xmax
                     << " ymin = " << ymin << " ymax = " << ymax;
        continue;
      }

      // We take as a blur mask size 30% of the detection size
      const float kMaskToDetectionRatio = 0.3f;
      int blur_size = static_cast<int>(std::round(
          std::max(ymax - ymin, xmax - xmin) * kMaskToDetectionRatio));
      if (blur_size % 2 == 0) {
        blur_size++;
      }

      cv::Mat submatrix = src(cv::Range(ymin, ymax), cv::Range(xmin, xmax));

      switch (options.blur_type()) {
        case SimpleBlurCalculatorOptions::BOX_BLUR: {
          // using a faster blur for manual testing
          cv::blur(submatrix, submatrix, cv::Size(blur_size, blur_size),
                   cv::Point(-(blur_size / 2), -(blur_size / 2)));
          break;
        }
        case SimpleBlurCalculatorOptions::GAUSSIAN_BLUR: {
          cv::GaussianBlur(submatrix, submatrix, cv::Size(blur_size, blur_size),
                           0, 0);
          break;
        }
      }
    }

    output_frame->CopyFrom(frame, ImageFrame::kDefaultAlignmentBoundary);

    cc->Outputs()
        .Tag(kFramesTag)
        .Add(output_frame.release(), cc->InputTimestamp());
    return absl::OkStatus();
  }
};

REGISTER_CALCULATOR(SimpleBlurCalculatorCpu);

}  // namespace magritte
