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
#ifndef MAGRITTE_CALCULATORS_ROTATION_ROI_CALCULATOR_H
#define MAGRITTE_CALCULATORS_ROTATION_ROI_CALCULATOR_H

#include "mediapipe/framework/calculator_framework.h"

namespace magritte {

// A calculator that, given an input image, creates a Region Of Interest (ROI)
// consisting of a rotation of the whole image.
//
// Inputs:
// - (No tag required): A packet of any type, containing the timestamp.
//
// Outputs:
// - ROI: a NormalizedRect stream, containing the rotated region of interest.
//
// Options:
// - RotationMode to define the rotation angle.
// - (optional) clockwise/counter-clockwise rotation
//   (default: counter-clockwise, equivalent to rotating the image clockwise).
//
// Example config:
// node {
//   calculator: "RotationRoiCalculator"
//   input_stream: "input_video"
//   output_stream: "ROI:output_roi"
//   options: {
//     [magritte.RotationCalculatorOptions.ext] {
//       rotation_mode: ROTATION_90
//     }
//   }
// }
class RotationRoiCalculator : public mediapipe::CalculatorBase {
 public:
  RotationRoiCalculator() = default;
  ~RotationRoiCalculator() override = default;

  static absl::Status GetContract(mediapipe::CalculatorContract* cc);
  absl::Status Open(mediapipe::CalculatorContext* cc) override;
  absl::Status Process(mediapipe::CalculatorContext* cc) override;
};

}  // namespace magritte

#endif  // MAGRITTE_CALCULATORS_ROTATION_ROI_CALCULATOR_H
