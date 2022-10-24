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
#include "magritte/calculators/rotation_roi_calculator.h"

#include <memory>

#include "mediapipe/framework/calculator_framework.h"
#include "absl/memory/memory.h"
#include "magritte/calculators/rotation_calculator_options.pb.h"
#include "mediapipe/framework/formats/rect.pb.h"

namespace magritte {

namespace {
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::NormalizedRect;

constexpr char kRegionOfInterestTag[] = "ROI";
}  // namespace

absl::Status RotationRoiCalculator::GetContract(CalculatorContract* cc) {
  RET_CHECK(cc != nullptr) << "CalculatorContract is nullptr";
  RET_CHECK(cc->Inputs().NumEntries() == 1)
      << "Calculator must have one input (of any type).";
  RET_CHECK(cc->Outputs().HasTag(kRegionOfInterestTag))
        << "Calculator must have an output ROI.";

  cc->Inputs().Get(cc->Inputs().BeginId()).SetAny();
  cc->Outputs().Tag(kRegionOfInterestTag).Set<NormalizedRect>();

  // No input side packets.
  return absl::OkStatus();
}

absl::Status RotationRoiCalculator::Open(CalculatorContext* cc) {
  return absl::OkStatus();
}

absl::Status RotationRoiCalculator::Process(CalculatorContext* cc) {
  const RotationCalculatorOptions& options =
      cc->Options<RotationCalculatorOptions>();
  RotationMode::Mode rotation_mode = options.rotation_mode();

  if (!options.clockwise()) {
    switch (rotation_mode) {
      case RotationMode::ROTATION_90:
        rotation_mode = RotationMode::ROTATION_270;
        break;
      case RotationMode::ROTATION_270:
        rotation_mode = RotationMode::ROTATION_90;
        break;
      default:
        break;
    }
  }

  // NormalizedRect::rotation is clockwise in radians.
  float rotation_radians = 0.0;
  switch (rotation_mode) {
    case RotationMode::ROTATION_90:
      rotation_radians = M_PI / 2;
      break;
    case RotationMode::ROTATION_180:
      rotation_radians = M_PI;
      break;
    case RotationMode::ROTATION_270:
      rotation_radians = 3 * M_PI / 2;
      break;
    default:
      break;
  }

  auto rect = std::make_unique<NormalizedRect>();
  rect->set_x_center(0.5);
  rect->set_y_center(0.5);
  rect->set_height(1.0);
  rect->set_width(1.0);
  rect->set_rotation(rotation_radians);

  auto& output = cc->Outputs().Tag(kRegionOfInterestTag);
  output.Add(rect.release(), cc->InputTimestamp());

  return absl::OkStatus();
}

REGISTER_CALCULATOR(RotationRoiCalculator);

}  // namespace magritte
