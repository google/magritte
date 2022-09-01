//
// Copyright 2021 Google LLC
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
#ifndef MAGRITTE_CALCULATORS_BLEND_CALCULATOR_H_
#define MAGRITTE_CALCULATORS_BLEND_CALCULATOR_H_

#include <algorithm>
#include <memory>
#include <vector>

#include "mediapipe/framework/calculator_framework.h"
#include "absl/status/status.h"
#include  <opencv2/core.hpp>

namespace magritte {
// A calculator that takes two ImageFrame input streams and blends them
// according to a mask.
//
// Inputs:
// - FRAMES_BG: An ImageFrame stream, containing a background image. The
//   background and foreground image streams must be of the same dimension.
// - FRAMES_FG: An ImageFrame stream, containing a foreground image. The
//   background and foreground image streams must be of the same dimension.
// - MASK: An ImageFrame stream, containing a mask in ImageFormat::VEC32F1
//   format. This determines how the background and foreground images will be
//   blended: 0 means using the background value, 255 means using the forground
//   value, and intermediate value will result in the weigted average between
//   the two.
//
// Outputs:
// - FRAMES: An ImageFrame stream containing the result of the blending as
//   described above.
//
// Example config:
// node {
//   calculator: "BlendCalculator"
//   input_stream: "FRAMES_BG:frames_bg"
//   input_stream: "FRAMES_FG:frames_fg"
//   input_stream: "MASK:mask"
//   output_stream: "FRAMES:output_video"
// }
class BlendCalculator : public ::mediapipe::CalculatorBase {
 public:
  BlendCalculator() = default;
  ~BlendCalculator() override = default;

  static absl::Status GetContract(mediapipe::CalculatorContract* cc);
  absl::Status Open(mediapipe::CalculatorContext* cc) override;
  absl::Status Process(mediapipe::CalculatorContext* cc) override;

 private:
  static absl::Status blend(cv::Mat bg, cv::Mat fg, cv::Mat mask);
};
}  // namespace magritte

#endif  // MAGRITTE_CALCULATORS_BLEND_CALCULATOR_H_
